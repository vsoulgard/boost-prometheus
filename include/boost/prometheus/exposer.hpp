//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_EXPOSER_HPP
#define BOOST_PROMETHEUS_EXPOSER_HPP

#include <boost/prometheus/detail/exposer_session.hpp>
#include <boost/prometheus/detail/extract_option.hpp>
#include <boost/prometheus/detail/ssl_fwd.hpp>

#include <boost/prometheus/exposer_options.hpp>
#include <boost/prometheus/registry.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/basic_stream_socket.hpp>

#include <atomic>
#include <stop_token>
#include <string>
#include <tuple>

namespace boost::prometheus {

// Lifetime contract:
//   The caller must guarantee that both the registry and the exposer
//   outlive every connection created through them.
//   Typical usage: create exposer and registry before io_context.run(),
//   destroy them after io_context.run() returns and all connections are closed.
// Responses larger than 1024 bytes are transparently compressed with gzip
// when the client sends a compatible Accept-Encoding header. Smaller
// responses are sent uncompressed
template <typename Executor, typename... Options>
class exposer
{
    using atomic_policy_type = detail::fallback_if_void_t<
        typename detail::extract_option<atomic_policy, Options...>::type,
        default_atomic_policy>;

    using gzip_policy_type = detail::fallback_if_void_t<
        typename detail::extract_option<gzip_policy, Options...>::type,
        default_gzip_policy>;

    using ssl_policy_type = detail::fallback_if_void_t<
        typename detail::extract_option<ssl_policy, Options...>::type,
        default_ssl_policy>;

    using parse_buffer_size_type = detail::fallback_if_void_t<
        typename detail::extract_option<parse_buffer_size, Options...>::type,
        default_parse_buffer_size>;

    using header_limit_size_type = detail::fallback_if_void_t<
        typename detail::extract_option<header_limit_size, Options...>::type,
        default_header_limit_size>;

    using static_registry_type =
        detail::extract_type_option<expose_static_registry, Options...>::type;

    using authorization_policy_type =
        detail::extract_option<authorization_policy, Options...>::type;

    using ssl_ctx_ptr_type = std::conditional_t<
        ssl_policy_type::enabled,
        typename boost::asio::ssl::context*,
        std::tuple<>>;

public:

    using socket_type = boost::asio::basic_stream_socket<boost::asio::ip::tcp, Executor>;
    using acceptor_type = boost::asio::basic_socket_acceptor<boost::asio::ip::tcp, Executor>;
    using registry_type = registry<atomic_policy_type>;

private:

    using registry_ptr_type = std::conditional_t<
        atomic_policy_type::enabled,
        std::atomic<registry<atomic_policy_type>*>,
        registry<atomic_policy_type>*>;

    Executor executor_;
    [[no_unique_address]]
    ssl_ctx_ptr_type ssl_ctx_;
    acceptor_type acceptor_;
    std::stop_source stop_source_;
    std::string route_;

    // The user is responsible for keeping the registry alive
    registry_ptr_type registry_;

    using session_type = detail::exposer_session<
        socket_type,
        exposer<Executor, Options...>,
        gzip_policy_type,
        ssl_policy_type,
        parse_buffer_size_type,
        header_limit_size_type,
        authorization_policy_type,
        static_registry_type>;

    friend session_type;

    registry<atomic_policy_type>* get_registry()
    {
        if constexpr (atomic_policy_type::enabled)
        {
            return registry_.load(std::memory_order_acquire);
        }
        else
        {
            return registry_;
        }
    }

    boost::asio::awaitable<void> ssl_session_runner(socket_type socket)
    {
        try
        {
            session_type session(std::move(socket), *this, *ssl_ctx_);
            co_await session.run();
        }
        catch (const std::exception& e) {}
    }

    boost::asio::awaitable<void> session_runner(socket_type socket)
    {
        try
        {
            session_type session(std::move(socket), *this);
            co_await session.run();
        }
        catch (const std::exception& e) {}
    }

    boost::asio::awaitable<void> listen()
    {
        while (!stop_source_.stop_requested())
        {
            beast::error_code ec;
            socket_type socket = co_await acceptor_.async_accept(
                boost::asio::redirect_error(boost::asio::use_awaitable, ec));

            if (ec)
            {
                // operation_aborted - acceptor stop()
                if (ec == boost::asio::error::operation_aborted) break;
                continue;
            }

            if (stop_source_.stop_requested()) break;

            if constexpr (ssl_policy_type::enabled)
            {
                boost::asio::co_spawn(
                    executor_,
                    ssl_session_runner(std::move(socket)),
                    boost::asio::detached);
            }
            else
            {
                boost::asio::co_spawn(
                    executor_,
                    session_runner(std::move(socket)),
                    boost::asio::detached);
            }
        }
    }

    void init(boost::asio::ip::tcp::endpoint& endpoint)
    {
        BOOST_ASSERT_MSG(route_.starts_with('/'), "Route must start with \"/\"");

        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen(boost::asio::socket_base::max_listen_connections);

        // Start listening
        boost::asio::co_spawn(executor_, listen(), boost::asio::detached);
    }

public:

    explicit exposer(
        Executor ex,
        boost::asio::ip::tcp::endpoint endpoint,
        std::string route = "/metrics")
    requires (!ssl_policy_type::enabled)
    : executor_(std::move(ex)),
    acceptor_(executor_),
    route_(std::move(route)),
    registry_(nullptr)
    {
        init(endpoint);
    }

    explicit exposer(
        Executor ex,
        boost::asio::ssl::context& ssl_ctx,
        boost::asio::ip::tcp::endpoint endpoint,
        std::string route = "/metrics")
    requires (ssl_policy_type::enabled)
    : executor_(std::move(ex)),
    ssl_ctx_(&ssl_ctx),
    acceptor_(executor_),
    route_(std::move(route)),
    registry_(nullptr)
    {
        init(endpoint);
    }

    exposer(const exposer&)            = delete;
    exposer& operator=(const exposer&) = delete;
    exposer(exposer&&)                 = delete;
    exposer& operator=(exposer&&)      = delete;

    static consteval bool is_atomic()
    {
        return atomic_policy_type::enabled;
    }

    // Note: rebinds previous registry
    template <typename registry_type>
    void expose(registry_type& registry)
    {
        if constexpr (atomic_policy_type::enabled)
        {
            registry_.store(&registry, std::memory_order_release);
        }
        else
        {
            registry_ = &registry;
        }
    }

    std::string_view route()
    {
        return route_;
    }

    // Shutdown
    void stop() noexcept
    {
        stop_source_.request_stop();

        boost::beast::error_code ec;
        acceptor_.close(ec);
        acceptor_.cancel(ec);
    }

    ~exposer()
    {
        stop();
    }

    unsigned short port() const noexcept
    {
        boost::beast::error_code ec;
        auto ep = acceptor_.local_endpoint(ec);
        return ec ? 0 : ep.port();
    }
};

} // namespace boost::prometheus

#endif
