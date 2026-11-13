//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_PUSH_OP_HPP
#define BOOST_PROMETHEUS_DETAIL_PUSH_OP_HPP

#include <boost/prometheus/detail/metric_traits.hpp>
#include <boost/prometheus/detail/ssl_traits.hpp>
#include <boost/prometheus/detail/valid_job_name.hpp>

#include <boost/prometheus/gzip_encode.hpp>
#include <boost/prometheus/pushgateway_errc.hpp>
#include <boost/prometheus/serialize.hpp>

#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>

#include <string_view>
#include <tuple>
#include <type_traits>

namespace boost::prometheus::detail {

// Pushgateway push operation
template <bool UsesAuth, typename GzipPolicy, typename ResponseParser, typename Socket, typename Request,
    typename DynamicBuffer, typename GzipBuffer, typename AllocBuffer, typename Registry>
struct push_op
{
    // gzip compression threshold
    static constexpr std::size_t gzip_min_size = 1024;

    using authorization_type = std::conditional_t<
        UsesAuth,
        std::string,
        std::tuple<>>;

    static constexpr bool is_static_registry = static_registry_specialization<Registry>;

    using registry_type = std::conditional_t<
        is_static_registry,
        std::tuple<>,
        Registry&>;

    ResponseParser& response_parser;
    Socket& socket;
    Request& request;
    DynamicBuffer& buffer;
    GzipBuffer& gzip_buffer;
    AllocBuffer& alloc_buffer;
    // Optional, only for dynamic registries.
    [[no_unique_address]]
    registry_type registry;
    boost::asio::ip::tcp::endpoint endpoint;
    std::string job_name;
    // Optional, only if authorization value is provided.
    [[no_unique_address]]
    authorization_type authorization;
    // PUT or POST method
    beast::http::verb method;

    enum class state_t
    {
        starting,
        connecting,
        handshaking, // SSL only
        writing,
        reading,
        shutting_down // SSL only
    };
    state_t state = state_t::starting;
    boost::system::error_code result_ec;

    push_op(ResponseParser& response_parser, Socket& socket, Request& request,
        DynamicBuffer& buffer, GzipBuffer& gzip_buffer, AllocBuffer& alloc_buffer, Registry& registry,
        boost::asio::ip::tcp::endpoint endpoint,
        std::string job_name,
        beast::http::verb method) requires (!UsesAuth && !is_static_registry)
        : response_parser(response_parser), socket(socket), request(request),
        buffer(buffer), gzip_buffer(gzip_buffer), alloc_buffer(alloc_buffer), registry(registry),
        endpoint(std::move(endpoint)), job_name(std::move(job_name)), method(method) {}

    push_op(ResponseParser& response_parser, Socket& socket, Request& request,
        DynamicBuffer& buffer, GzipBuffer& gzip_buffer, AllocBuffer& alloc_buffer, Registry& registry,
        boost::asio::ip::tcp::endpoint endpoint,
        std::string job_name,
        std::string authorization,
        beast::http::verb method) requires (UsesAuth && !is_static_registry)
        : response_parser(response_parser), socket(socket), request(request),
        buffer(buffer), gzip_buffer(gzip_buffer), alloc_buffer(alloc_buffer), registry(registry),
        endpoint(std::move(endpoint)), job_name(std::move(job_name)), authorization(std::move(authorization)),
        method(method) {}
    // Static registry
    push_op(ResponseParser& response_parser, Socket& socket, Request& request,
        DynamicBuffer& buffer, GzipBuffer& gzip_buffer, AllocBuffer& alloc_buffer,
        boost::asio::ip::tcp::endpoint endpoint,
        std::string job_name,
        beast::http::verb method) requires (!UsesAuth && is_static_registry)
        : response_parser(response_parser), socket(socket), request(request),
        buffer(buffer), gzip_buffer(gzip_buffer), alloc_buffer(alloc_buffer),
        endpoint(std::move(endpoint)), job_name(std::move(job_name)),
        method(method) {}
    // Static registry with auth
    push_op(ResponseParser& response_parser, Socket& socket, Request& request,
        DynamicBuffer& buffer, GzipBuffer& gzip_buffer, AllocBuffer& alloc_buffer,
        boost::asio::ip::tcp::endpoint endpoint,
        std::string job_name,
        std::string authorization,
        beast::http::verb method) requires (UsesAuth && is_static_registry)
        : response_parser(response_parser), socket(socket), request(request),
        buffer(buffer), gzip_buffer(gzip_buffer), alloc_buffer(alloc_buffer),
        endpoint(std::move(endpoint)), job_name(std::move(job_name)), authorization(std::move(authorization)),
        method(method) {}

    template <typename Self>
    void operator()(
        Self& self,
        boost::system::error_code ec = {},
        std::size_t = 0)
    {
        if (state == state_t::shutting_down)
        {
            close_socket();
            self.complete(result_ec);
            return;
        }

        if (ec)
        {
            result_ec = ec;
            return finish(self);
        }

        switch (state)
        {
        case state_t::starting:
            if (!valid_job_name(job_name))
            {
                result_ec = make_error_code(pushgateway_errc::incorrect_job_name);
                return finish(self);
            }
            prepare_request();
            state = state_t::connecting;
            if constexpr (is_ssl_stream_v<Socket>)
                socket.next_layer().async_connect(endpoint, std::move(self));
            else
                socket.async_connect(endpoint, std::move(self));
            return;

        case state_t::connecting:
            if constexpr (is_ssl_stream_v<Socket>)
            {
                state = state_t::handshaking;
                socket.async_handshake(boost::asio::ssl::stream_base::client,
                    std::move(self));
            }
            else
            {
                state = state_t::writing;
                beast::http::async_write(socket, request, std::move(self));
            }
            return;

        case state_t::handshaking:
            state = state_t::writing;
            beast::http::async_write(socket, request, std::move(self));
            return;

        case state_t::writing:
            state = state_t::reading;
            beast::http::async_read_header(socket, buffer, response_parser,
                std::move(self));
            return;
        case state_t::reading:
        {
            int status = response_parser.get().result_int();
            if (status < 200 || status >= 300)
                result_ec = error::make_http_status_error(status);
            return finish(self);
        }
        case state_t::shutting_down:
            return;
        }
    }

private:

    void prepare_request()
    {
        // Clear everything
        request.clear();
        request.body().clear();
        response_parser.get().clear();
        buffer.clear();
        alloc_buffer.reset();

        request.method(method);
        request.target("/metrics/job/" + job_name);
        request.version(11);

        std::string host = endpoint.address().to_string();
        if (endpoint.address().is_v6())
            host = "[" + host + "]";
        host += std::string_view(":");
        host += std::to_string(endpoint.port());
        request.set(beast::http::field::host, host);

        request.set(beast::http::field::content_type,
            format::prometheus_0_0_4::content_type);

        if constexpr (UsesAuth)
            request.set(beast::http::field::authorization, authorization);

        if constexpr (GzipPolicy::enabled)
        {
            serialize_registry();
            if (request.body().size() > gzip_min_size)
            {
                request.set(beast::http::field::content_encoding, "gzip");
                gzip_encode(request.body(), gzip_buffer);
                std::swap(request.body(), gzip_buffer);
            }
        }
        else
        {
            serialize_registry();
        }

        request.prepare_payload();
    }

    void serialize_registry()
    {
        if constexpr (is_static_registry)
            serialize<format::prometheus_0_0_4, Registry>(request.body());
        else
            serialize<format::prometheus_0_0_4>(registry, request.body());
    }

    void close_socket() noexcept
    {
        boost::system::error_code ignored;
        if constexpr (is_ssl_stream_v<Socket>)
            socket.next_layer().close(ignored);
        else
            socket.close(ignored);
    }

    template <typename Self>
    void finish(Self& self)
    {
        if constexpr (GzipPolicy::enabled)
        {
            if (gzip_buffer.size() > 0)
            {
                // Swap buffers back.
                std::swap(request.body(), gzip_buffer);
                gzip_buffer.clear();
            }
        }
        if constexpr (is_ssl_stream_v<Socket>)
        {
            state = state_t::shutting_down;
            socket.async_shutdown(std::move(self));
        }
        else
        {
            close_socket();
            self.complete(result_ec);
        }
    }
};

} // namespace boost::prometheus::detail

#endif
