//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// Integration tests against a real Pushgateway on localhost:9091.

#include <boost/prometheus/error_code.hpp>
#include <boost/prometheus/pushgateway.hpp>
#include <boost/prometheus/registry.hpp>

#include <boost/test/unit_test.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>

#include "test_common/make_request.hpp"
#include "test_common/pushgateway_utils.hpp"
#include "test_common/setup_registry.hpp"

using namespace boost;
using namespace beast;
using namespace boost::asio;
using namespace boost::prometheus;
using namespace boost::prometheus::test;

const ip::tcp::endpoint target_ep(asio::ip::address_v4::loopback(), 9091);
const std::string test_job_name = "test";

template <typename PushgatewayType>
void test_push()
{
    boost::asio::io_context ioc;
    PushgatewayType pgw(ioc.get_executor());

    boost::asio::co_spawn(
        ioc.get_executor(),
        coro_push(pgw, test_job_name, get_registry(), target_ep),
        [&ioc](std::exception_ptr eptr)
        {
            if (eptr)
            {
                try { std::rethrow_exception(eptr); }
                catch (const std::exception& e) {
                    BOOST_FAIL(std::string("fail: ") + e.what());
                }
            }
            ioc.stop();
        }
    );
    ioc.run();

    beast::tcp_stream stream(ioc);
    beast::error_code ec;
    stream.connect(target_ep, ec);
    if (ec) BOOST_FAIL("Failed to connect: " << ec.message());
    std::string result = make_request(stream, "127.0.0.1:9091", "/metrics", "text/plain;version=0.0.4");
    // Parse scrapped metrics.
    // Pushgateway uses metadata order: # HELP # TYPE.
    // Pushgateway adds instance and job labels.
    BOOST_CHECK(result.find(
        "# HELP requests_total Number of HTTP requests\n"
        "# TYPE requests_total counter\n"
        "requests_total{instance=\"\",job=\"test\",method=\"GET\"} 5\n") != std::string_view::npos);
    BOOST_CHECK(result.find(
        "# HELP applications_active Current number of applications\n"
        "# TYPE applications_active gauge\n"
        "applications_active{instance=\"\",job=\"test\"} 100\n") != std::string_view::npos);
    BOOST_CHECK(result.find(
        "# HELP request_duration_seconds Duration of HTTP requests in seconds\n"
        "# TYPE request_duration_seconds histogram\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"0.005\"} 0\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"0.01\"} 0\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"0.025\"} 0\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"0.05\"} 0\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"0.1\"} 0\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"0.25\"} 0\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"0.5\"} 1\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"1\"} 1\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"2.5\"} 1\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"5\"} 1\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"10\"} 1\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"+Inf\"} 1\n"
        "request_duration_seconds_sum{instance=\"\",job=\"test\"} 0.5\n"
        "request_duration_seconds_count{instance=\"\",job=\"test\"} 1\n") != std::string_view::npos);
    BOOST_CHECK(result.find(
        "# HELP cpu_temperature_celsius CPU core temperature\n"
        "# TYPE cpu_temperature_celsius histogram\n"
        "cpu_temperature_celsius_bucket{instance=\"\",job=\"test\",le=\"25\"} 0\n"
        "cpu_temperature_celsius_bucket{instance=\"\",job=\"test\",le=\"50\"} 1\n"
        "cpu_temperature_celsius_bucket{instance=\"\",job=\"test\",le=\"75\"} 1\n"
        "cpu_temperature_celsius_bucket{instance=\"\",job=\"test\",le=\"90\"} 1\n"
        "cpu_temperature_celsius_bucket{instance=\"\",job=\"test\",le=\"+Inf\"} 1\n") != std::string_view::npos);
    BOOST_CHECK(result.find(
        "# TYPE cpu_temperature_celsius_gsum gauge\n"
        "cpu_temperature_celsius_gsum{instance=\"\",job=\"test\"} 33\n") != std::string_view::npos);
        BOOST_CHECK(result.find(
        "# TYPE cpu_temperature_celsius_gcount gauge\n"
        "cpu_temperature_celsius_gcount{instance=\"\",job=\"test\"} 1\n") != std::string_view::npos);
}

BOOST_AUTO_TEST_SUITE(test_pushgateway_real_push)

BOOST_AUTO_TEST_CASE(push_basic)
{
    test_push<
        pushgateway<boost::asio::io_context::executor_type>>();
}

BOOST_AUTO_TEST_CASE(push_gzip_disabled)
{
    test_push<
        pushgateway<boost::asio::io_context::executor_type,
        gzip_policy<false>>>();
}

BOOST_AUTO_TEST_CASE(push_static_registry)
{
    boost::asio::io_context ioc;
    pushgateway pgw(ioc.get_executor());
    setup_static_registry();

    auto coro = [&]() -> boost::asio::awaitable<void>
    {
        for (std::size_t i = 0; i < 10u; i++)
        {
            prometheus::error_code ec;
            co_await pgw.async_push<premade_static_registry>(target_ep, test_job_name,
                asio::redirect_error(asio::use_awaitable, ec));

            BOOST_TEST(ec == prometheus::error_code());
        }
    };
    boost::asio::co_spawn(
        ioc.get_executor(),
        coro(),
        [&ioc](std::exception_ptr eptr)
        {
            if (eptr)
            {
                try { std::rethrow_exception(eptr); }
                catch (const std::exception& e) {
                    BOOST_FAIL(std::string("fail: ") + e.what());
                }
            }
            ioc.stop();
        }
    );
    ioc.run();

    beast::tcp_stream stream(ioc);
    beast::error_code ec;
    stream.connect(target_ep, ec);
    if (ec) BOOST_FAIL("Failed to connect: " << ec.message());
    std::string result = make_request(stream, "127.0.0.1:9091", "/metrics", "text/plain;version=0.0.4");
    // Parse scrapped metrics.
    // Pushgateway uses metadata order: # HELP # TYPE.
    // Pushgateway adds instance and job labels.
    BOOST_CHECK(result.find(
        "# HELP requests_total Number of HTTP requests\n"
        "# TYPE requests_total counter\n"
        "requests_total{instance=\"\",job=\"test\",method=\"POST\"} 15\n") != std::string_view::npos);
    BOOST_CHECK(result.find(
        "# HELP applications_active Current number of applications\n"
        "# TYPE applications_active gauge\n"
        "applications_active{instance=\"\",job=\"test\"} 50\n") != std::string_view::npos);
    BOOST_CHECK(result.find(
        "# HELP request_duration_seconds Duration of HTTP requests in seconds\n"
        "# TYPE request_duration_seconds histogram\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"0.005\"} 0\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"0.01\"} 0\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"0.025\"} 0\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"0.05\"} 0\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"0.1\"} 1\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"0.25\"} 1\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"0.5\"} 1\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"1\"} 1\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"2.5\"} 1\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"5\"} 1\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"10\"} 1\n"
        "request_duration_seconds_bucket{instance=\"\",job=\"test\",le=\"+Inf\"} 1\n"
        "request_duration_seconds_sum{instance=\"\",job=\"test\"} 0.1\n"
        "request_duration_seconds_count{instance=\"\",job=\"test\"} 1\n") != std::string_view::npos);
}

BOOST_AUTO_TEST_CASE(clear_basic)
{
    boost::asio::io_context ioc;
    pushgateway pgw(ioc.get_executor());

    boost::asio::co_spawn(
        ioc.get_executor(),
        coro_clear(pgw, test_job_name, target_ep),
        [&ioc](std::exception_ptr eptr)
        {
            if (eptr)
            {
                try { std::rethrow_exception(eptr); }
                catch (const std::exception& e) {
                    BOOST_FAIL(std::string("fail: ") + e.what());
                }
            }
            ioc.stop();
        }
    );
    ioc.run();
}

BOOST_AUTO_TEST_SUITE_END()
