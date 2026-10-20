//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_EXPOSER_SESSION_HPP
#define BOOST_PROMETHEUS_DETAIL_EXPOSER_SESSION_HPP

#include <boost/prometheus/gzip_encode.hpp>
#include <boost/prometheus/serialize.hpp>

#include <boost/prometheus/detail/ssl_fwd.hpp>
#include <boost/prometheus/detail/static_allocator.hpp>
#include <boost/prometheus/detail/string_buffer.hpp>

#include <boost/assert.hpp>

#include <boost/beast/http.hpp>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <coroutine>
#include <tuple>
#include <type_traits>
#include <utility>

namespace boost::prometheus::detail {

namespace beast = boost::beast;
namespace http  = beast::http;
namespace asio  = boost::asio;
using tcp       = asio::ip::tcp;

template <
    typename SocketType,
    typename ExposerType,
    typename GzipPolicy,
    typename SSLPolicy,
    typename ParseBufferSize,
    typename HeaderLimitSize,
    typename AuthorizationPolicy,
    typename StaticRegistry>
class exposer_session
{
    static constexpr std::size_t static_buffer_size = ParseBufferSize::value;
    using alloc_t = static_allocator<char, static_buffer_size>;

    using response_type = http::response<http::vector_body<char>, http::basic_fields<alloc_t>>;

    using socket_type = std::conditional_t<
        SSLPolicy::enabled,
        typename boost::asio::ssl::stream<SocketType>,
        SocketType>;

    using gzip_buffer_type = std::conditional_t<
        GzipPolicy::enabled,
        typename response_type::body_type::value_type,
        std::tuple<>>;

    using ssl_ctx_ptr_type = std::conditional_t<
        SSLPolicy::enabled,
        typename boost::asio::ssl::context*,
        std::tuple<>>;

    // gzip compression threshold
    static constexpr std::size_t gzip_min_size = 1024;

    static constexpr bool has_auth = !std::is_void_v<AuthorizationPolicy>;
    static constexpr bool has_static_registry = !std::is_void_v<StaticRegistry>;

    socket_type socket_;
    boost::beast::flat_static_buffer<static_buffer_size> buffer_;
    allocator_buffer<static_buffer_size> alloc_buffer_;
    alloc_t alloc_;

    ExposerType* exposer_;
    http::response<http::vector_body<char>, http::basic_fields<alloc_t>> response_;
    // Buffer to store the gzip result to.
    [[no_unique_address]]
    gzip_buffer_type gzip_buffer_;

public:

    explicit exposer_session(SocketType&& socket, ExposerType& exposer)
    requires (!SSLPolicy::enabled)
    : socket_(std::move(socket)),
    alloc_(alloc_buffer_),
    exposer_(&exposer),
    response_(
        std::piecewise_construct,
        std::make_tuple(),
        std::make_tuple(alloc_)
    )
    {
        BOOST_ASSERT(exposer_ != nullptr);
        response_.body().reserve(4096);
    }

    explicit exposer_session(
        SocketType&& socket,
        ExposerType& exposer,
        boost::asio::ssl::context& ssl_ctx)
    requires (SSLPolicy::enabled)
    : socket_(std::move(socket), ssl_ctx),
    alloc_(alloc_buffer_),
    exposer_(&exposer),
    response_(
        std::piecewise_construct,
        std::make_tuple(),
        std::make_tuple(alloc_)
    )
    {
        BOOST_ASSERT(exposer_ != nullptr);
        response_.body().reserve(4096);
    }

    exposer_session(const exposer_session&)            = delete;
    exposer_session& operator=(const exposer_session&) = delete;
    exposer_session(exposer_session&&)                 = delete;
    exposer_session& operator=(exposer_session&&)      = delete;

    constexpr bool authorize(const auto& request)
    {
        if constexpr (has_auth == false)
        {
            return true;
        }
        else
        {
            auto it = request.find(http::field::authorization);
            if (it == request.end())
                return false;

            return AuthorizationPolicy::validate(it->value());
        }
    }

    void assign_not_found()
    {
        response_.result(http::status::not_found);
        response_.set(http::field::content_type, "text/plain");
        constexpr std::string_view page_not_found = "Page Not Found";
        response_.body().assign(page_not_found.begin(), page_not_found.end()); 
    }

    void assign_unauthorized()
    {
        response_.result(http::status::unauthorized);
        constexpr auto www_auth = []() consteval {
            if constexpr (std::string_view(AuthorizationPolicy::realm).empty())
                return AuthorizationPolicy::auth_type;
            else
                return AuthorizationPolicy::auth_type +
                    string_buffer(" realm=\"") +
                    AuthorizationPolicy::realm +
                    string_buffer("\"");
        }();
        response_.set(http::field::www_authenticate, std::string_view(www_auth));
        response_.set(http::field::content_type, "text/plain");
        constexpr std::string_view page_not_found = "Unauthorized";
        response_.body().assign(page_not_found.begin(), page_not_found.end());
    }

    void assign_method_not_allowed()
    {
        response_.result(http::status::method_not_allowed);
        response_.set(http::field::content_type, "text/plain");
        response_.set(http::field::allow, "GET");
        constexpr std::string_view method_not_allowed = "Method Not Allowed";
        response_.body().assign(method_not_allowed.begin(), method_not_allowed.end());
    }

    void gzip_response(const auto& request)
    {
        if (response_.body().size() > gzip_min_size)
        {
            auto it = request.find(http::field::accept_encoding);
            if (it != request.end() && it->value().find("gzip") != std::string_view::npos)
            {
                // encode response
                bool result = gzip_encode(response_.body(), gzip_buffer_);
                BOOST_ASSERT(result);
                if (result)
                {
                    response_.set(http::field::content_encoding, "gzip");
                    std::swap(response_.body(), gzip_buffer_);
                }
            }
        }
    }

    asio::awaitable<void> run()
    {
        while (true)
        {
            beast::error_code ec;
            if constexpr (SSLPolicy::enabled)
            {
                // SSL handshake
                co_await socket_.async_handshake(
                    boost::asio::ssl::stream_base::server,
                    boost::asio::redirect_error(boost::asio::use_awaitable, ec));
                if (ec) break;
            }

            response_.clear();
            response_.body().clear();
            alloc_buffer_.reset();

            http::request_parser<http::empty_body, alloc_t> parser(
                std::piecewise_construct,
                std::make_tuple(),
                std::make_tuple(alloc_)
            );

            parser.header_limit(HeaderLimitSize::value);
            parser.body_limit(0);

            buffer_.consume(buffer_.size());

            co_await http::async_read_header(
                socket_, buffer_, parser,
                asio::redirect_error(asio::use_awaitable, ec));

            if (ec) break;

            auto& req = parser.get();

            response_.version(req.version());

            if (req.target() != exposer_->route())
            {
                assign_not_found();
            }
            else
            {
                bool auth_result = authorize(req);
                bool is_get = req.method() == http::verb::get;
                if (auth_result == false)
                {
                    if constexpr (has_auth)
                    {
                        if constexpr (AuthorizationPolicy::return_not_found)
                        {
                            assign_not_found();
                        }
                        else
                        {
                            assign_unauthorized();
                        }
                    }
                }
                else if (is_get == false)
                {
                    assign_method_not_allowed();
                }
                else
                {
                    response_.result(http::status::ok);
                    auto* registry_ptr = exposer_->get_registry();
                    auto it = req.find(http::field::accept);
                    if (it != req.end() && it->value().find("application/openmetrics-text") != std::string_view::npos)
                    {
                        response_.set(http::field::content_type, format::openmetrics_1_0::content_type);
                        if constexpr (has_static_registry)
                        {
                            serialize<format::openmetrics_1_0, typename StaticRegistry::type, false>(response_.body());
                        }
                        if (registry_ptr)
                        {
                            serialize<format::openmetrics_1_0>(*registry_ptr, response_.body());
                        }
                        else
                        {
                            constexpr std::string_view eof = "# EOF\n";
                            format::append(eof, response_.body());
                        }
                    }
                    else
                    {
                        // Fallback: Prometheus 0.0.4 format
                        response_.set(http::field::content_type, format::prometheus_0_0_4::content_type);
                        if constexpr (has_static_registry)
                        {
                            serialize<format::prometheus_0_0_4, typename StaticRegistry::type>(response_.body());
                        }
                        if (registry_ptr)
                        {
                            serialize<format::prometheus_0_0_4>(*registry_ptr, response_.body());
                        }
                    }
                    if constexpr (GzipPolicy::enabled)
                    {
                        gzip_response(req);
                    }
                }
            }

            response_.keep_alive(req.keep_alive());

            response_.prepare_payload();

            co_await http::async_write(
                socket_, response_,
                asio::redirect_error(asio::use_awaitable, ec));

            if constexpr (GzipPolicy::enabled)
            {
                if (gzip_buffer_.size() > 0)
                {
                    // Swap buffers back
                    std::swap(response_.body(), gzip_buffer_);
                    gzip_buffer_.clear();
                }
            }

            if (ec || !response_.keep_alive()) break;
        }
        boost::beast::error_code ec;
        if constexpr (SSLPolicy::enabled)
        {
            // Shutdown SSL stream
            co_await socket_.async_shutdown(
                boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        }
        else
        {
            // Shutdown regular TCP stream
            socket_.shutdown(tcp::socket::shutdown_send, ec);
        }
    }
};

} // namespace boost::prometheus::detail

#endif
