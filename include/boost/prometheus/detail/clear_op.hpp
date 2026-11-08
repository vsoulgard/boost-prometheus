//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_CLEAR_OP_HPP
#define BOOST_PROMETHEUS_DETAIL_CLEAR_OP_HPP

#include <boost/prometheus/detail/ssl_traits.hpp>
#include <boost/prometheus/detail/valid_job_name.hpp>

#include <boost/prometheus/pushgateway_errc.hpp>

#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
#include <boost/config.hpp>

#include <string_view>
#include <tuple>
#include <type_traits>

namespace boost::prometheus::detail {

// Disable false positive on gcc 12
#if defined(BOOST_GCC) && BOOST_GCC_VERSION >= 12000 && BOOST_GCC_VERSION < 13000
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wrestrict"
#endif
// Pushgateway clear operation
template <bool UsesAuth, typename ResponseParser, typename Socket, typename Request,
    typename DynamicBuffer, typename AllocBuffer>
struct clear_op
{
    using authorization_type = std::conditional_t<
        UsesAuth,
        std::string,
        std::tuple<>>;

    ResponseParser& response_parser;
    Socket& socket;
    Request& request;
    DynamicBuffer& buffer;
    AllocBuffer& alloc_buffer;
    boost::asio::ip::tcp::endpoint endpoint;
    std::string job_name;
    [[no_unique_address]]
    authorization_type authorization;

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

    clear_op(ResponseParser& response_parser, Socket& socket, Request& request,
        DynamicBuffer& buffer, AllocBuffer& alloc_buffer,
        boost::asio::ip::tcp::endpoint endpoint,
        std::string job_name) requires (!UsesAuth)
        : response_parser(response_parser), socket(socket), request(request),
        buffer(buffer), alloc_buffer(alloc_buffer),
        endpoint(std::move(endpoint)), job_name(std::move(job_name)) {}

    clear_op(ResponseParser& response_parser, Socket& socket, Request& request,
        DynamicBuffer& buffer, AllocBuffer& alloc_buffer,
        boost::asio::ip::tcp::endpoint endpoint,
        std::string job_name, std::string authorization) requires (UsesAuth)
        : response_parser(response_parser), socket(socket), request(request),
        buffer(buffer), alloc_buffer(alloc_buffer),
        endpoint(std::move(endpoint)), job_name(std::move(job_name)), authorization(std::move(authorization)) {}

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
            // For DELETE success is always has 202 code.
            if (status != 202)
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

        request.method(beast::http::verb::delete_);
        request.target("/metrics/job/" + job_name);
        request.version(11);

        std::string host = endpoint.address().to_string();
        if (endpoint.address().is_v6())
            host = "[" + host + "]";
        host += ":" + std::to_string(endpoint.port());
        request.set(beast::http::field::host, host);
        if constexpr (UsesAuth)
            request.set(beast::http::field::authorization, authorization);

        request.prepare_payload();
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
#if defined(BOOST_GCC) && BOOST_GCC_VERSION >= 12000 && BOOST_GCC_VERSION < 13000
    #pragma GCC diagnostic pop
#endif

} // namespace boost::prometheus::detail

#endif
