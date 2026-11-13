//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_TEST_PUSHGATEWAY_UTILS_HPP
#define BOOST_PROMETHEUS_TEST_PUSHGATEWAY_UTILS_HPP

#include <boost/test/unit_test.hpp>

#include <boost/prometheus/error_code.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>

#include <string>
#include <string_view>

namespace boost::prometheus::test {

namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace asio = boost::asio;
namespace prometheus = boost::prometheus;

inline auto coro_push = [](
    auto& pgw,
    std::string job_name,
    auto& registry,
    const asio::ip::tcp::endpoint& target_ep) -> boost::asio::awaitable<void>
{
    for (std::size_t i = 0; i < 10u; i++)
    {
        prometheus::error_code ec;
        co_await pgw.async_push(target_ep, job_name, registry,
            asio::redirect_error(asio::use_awaitable, ec));

        BOOST_TEST(ec == prometheus::error_code());
    }
};

inline auto coro_clear = [](
    auto& pgw,
    std::string job_name,
    const asio::ip::tcp::endpoint& target_ep) -> boost::asio::awaitable<void>
{
    for (std::size_t i = 0; i < 10u; i++)
    {
        prometheus::error_code ec;
        co_await pgw.async_clear(target_ep, job_name,
            asio::redirect_error(asio::use_awaitable, ec));

        BOOST_TEST(ec == prometheus::error_code());
    }
};

inline auto coro_clear_and_push = [](
    auto& pgw,
    std::string job_name,
    auto& registry,
    const asio::ip::tcp::endpoint& target_ep) -> boost::asio::awaitable<void>
{
    for (std::size_t i = 0; i < 10u; i++)
    {
        prometheus::error_code ec;
        co_await pgw.async_clear_and_push(target_ep, job_name, registry,
            asio::redirect_error(asio::use_awaitable, ec));

        BOOST_TEST(ec == prometheus::error_code());
    }
};

} // namespace boost::prometheus::test

#endif
