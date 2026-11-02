//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/error_code.hpp>
#include <boost/prometheus/exposer_ssl_policy.hpp>
#include <boost/prometheus/pushgateway.hpp>
#include <boost/prometheus/registry.hpp>

#include <boost/test/unit_test.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>

#include "test_common/client_certificate.hpp"
#include "test_common/gzip_decode.hpp"
#include "test_common/pushgateway_utils.hpp"
#include "test_common/server_certificate.hpp"
#include "test_common/setup_registry.hpp"

#include <memory>
#include <thread>

using namespace boost;
using namespace beast;
using namespace boost::asio;
using namespace boost::prometheus;
using namespace boost::prometheus::test;

const ip::tcp::endpoint target_ep(asio::ip::address_v4::loopback(), 9092);
const std::string test_job_name = "test";

template <typename Stream>
asio::awaitable<void> serve_one(
    Stream stream,
    bool expect_no_gzip,
    bool expect_auth,
    bool expect_static_registry,
    bool return_not_found)
{
    beast::flat_buffer buf;
    beast::http::request<beast::http::string_body> req;
    co_await beast::http::async_read(stream, buf, req, asio::use_awaitable);

    BOOST_TEST(req.target() == "/metrics/job/" + test_job_name);
    BOOST_CHECK(req.method() == http::verb::post ||
        req.method() == http::verb::delete_);
    beast::http::response<http::string_body> res{http::status::ok, req.version()};
    if (return_not_found)
    {
        res.result(http::status::not_found);
    }
    else if (req.method() == http::verb::post)
    {
        if (expect_no_gzip)
            BOOST_TEST(req[http::field::content_encoding].empty());

        std::string_view result;
        std::string decoded;
        if (expect_auth)
        {
            BOOST_TEST(req[http::field::authorization] == "Bearer secret-key");
        }
        if (req[http::field::content_encoding] == "gzip")
        {
            test::gzip_decode(req.body(), decoded);
            result = decoded;
            BOOST_TEST(decoded.size() > 1024);
        }
        else
        {
            result = req.body();
            if (!expect_no_gzip)
                BOOST_TEST(result.size() <= 1024);
        }
        // Static registry has another payload.
        if (expect_static_registry)
        {
            BOOST_CHECK(result.find(
                "# TYPE requests_total counter\n"
                "# HELP requests_total Number of HTTP requests\n"
                "requests_total{method=\"POST\"} 15\n") != std::string_view::npos);
            BOOST_CHECK(result.find(
                "# TYPE applications_active gauge\n"
                "# HELP applications_active Current number of applications\n"
                "applications_active 50\n") != std::string_view::npos);
            BOOST_CHECK(result.find(
                "# TYPE request_duration_seconds histogram\n"
                "# HELP request_duration_seconds Duration of HTTP requests in seconds\n"
                "request_duration_seconds_bucket{le=\"0.005\"} 0\n"
                "request_duration_seconds_bucket{le=\"0.01\"} 0\n"
                "request_duration_seconds_bucket{le=\"0.025\"} 0\n"
                "request_duration_seconds_bucket{le=\"0.05\"} 0\n"
                "request_duration_seconds_bucket{le=\"0.1\"} 1\n"
                "request_duration_seconds_bucket{le=\"0.25\"} 1\n"
                "request_duration_seconds_bucket{le=\"0.5\"} 1\n"
                "request_duration_seconds_bucket{le=\"1\"} 1\n"
                "request_duration_seconds_bucket{le=\"2.5\"} 1\n"
                "request_duration_seconds_bucket{le=\"5\"} 1\n"
                "request_duration_seconds_bucket{le=\"10\"} 1\n"
                "request_duration_seconds_bucket{le=\"+Inf\"} 1\n"
                "request_duration_seconds_sum 0.1\n"
                "request_duration_seconds_count 1\n") != std::string_view::npos);
            BOOST_TEST(result.size() == 109u + 114u + 708u);
        }
        else
        {
            BOOST_CHECK(result.find(
                "# TYPE requests_total counter\n"
                "# HELP requests_total Number of HTTP requests\n"
                "requests_total{method=\"GET\"} 5\n") != std::string_view::npos);
            BOOST_CHECK(result.find(
                "# TYPE applications_active gauge\n"
                "# HELP applications_active Current number of applications\n"
                "applications_active 100\n") != std::string_view::npos);
            BOOST_CHECK(result.find(
                "# TYPE request_duration_seconds histogram\n"
                "# HELP request_duration_seconds Duration of HTTP requests in seconds\n"
                "request_duration_seconds_bucket{le=\"0.005\"} 0\n"
                "request_duration_seconds_bucket{le=\"0.01\"} 0\n"
                "request_duration_seconds_bucket{le=\"0.025\"} 0\n"
                "request_duration_seconds_bucket{le=\"0.05\"} 0\n"
                "request_duration_seconds_bucket{le=\"0.1\"} 0\n"
                "request_duration_seconds_bucket{le=\"0.25\"} 0\n"
                "request_duration_seconds_bucket{le=\"0.5\"} 1\n"
                "request_duration_seconds_bucket{le=\"1\"} 1\n"
                "request_duration_seconds_bucket{le=\"2.5\"} 1\n"
                "request_duration_seconds_bucket{le=\"5\"} 1\n"
                "request_duration_seconds_bucket{le=\"10\"} 1\n"
                "request_duration_seconds_bucket{le=\"+Inf\"} 1\n"
                "request_duration_seconds_sum 0.5\n"
                "request_duration_seconds_count 1\n") != std::string_view::npos);
            BOOST_CHECK(result.find(
                "# TYPE cpu_temperature_celsius histogram\n"
                "# HELP cpu_temperature_celsius CPU core temperature\n"
                "cpu_temperature_celsius_bucket{le=\"25\"} 0\n"
                "cpu_temperature_celsius_bucket{le=\"50\"} 1\n"
                "cpu_temperature_celsius_bucket{le=\"75\"} 1\n"
                "cpu_temperature_celsius_bucket{le=\"90\"} 1\n"
                "cpu_temperature_celsius_bucket{le=\"+Inf\"} 1\n"
                "# TYPE cpu_temperature_celsius_gsum gauge\n"
                "cpu_temperature_celsius_gsum 33\n"
                "# TYPE cpu_temperature_celsius_gcount gauge\n"
                "cpu_temperature_celsius_gcount 1\n") != std::string_view::npos);
            BOOST_TEST(result.size() == 107u + 115u + 708u + 456u);
        }
    }
    else
    {
        // For DELETE success is always has 202 code.
        res.result(http::status::accepted);
        BOOST_CHECK(req.body().empty());
    }

    res.prepare_payload();
    co_await beast::http::async_write(stream, res, asio::use_awaitable);

    beast::error_code ec;
    if constexpr (prometheus::detail::is_ssl_stream_v<Stream>)
    {
        co_await stream.async_shutdown(
            boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    }
    else
    {
        stream.shutdown(asio::ip::tcp::socket::shutdown_send, ec);
    }
}

asio::awaitable<void> listener(
    std::shared_ptr<asio::ip::tcp::acceptor> acc,
    bool expect_no_gzip,
    bool expect_auth,
    bool expect_static_registry,
    bool return_not_found)
{
    for (;;)
    {
        asio::ip::tcp::socket sock = co_await acc->async_accept(use_awaitable);
        try
        {
            co_await serve_one(std::move(sock), expect_no_gzip, expect_auth, expect_static_registry, return_not_found);
        }
        catch (const boost::system::system_error&) {}
    }
}

asio::awaitable<void> ssl_listener(
    std::shared_ptr<asio::ip::tcp::acceptor> acc,
    std::shared_ptr<boost::asio::ssl::context> ctx,
    bool expect_no_gzip,
    bool expect_auth,
    bool expect_static_registry,
    bool return_not_found)
{
    for (;;)
    {
        asio::ip::tcp::socket raw_sock = co_await acc->async_accept(use_awaitable);
        boost::asio::ssl::stream<asio::ip::tcp::socket> stream(std::move(raw_sock), *ctx);
        try
        {
            co_await stream.async_handshake(
                boost::asio::ssl::stream_base::server, asio::use_awaitable);
            co_await serve_one(std::move(stream), expect_no_gzip, expect_auth, expect_static_registry, return_not_found);
        }
        catch (const boost::system::system_error&) {}
    }
}

void start_listening_server(
    io_context& ioc,
    bool expect_no_gzip = false,
    bool expect_auth = false,
    bool expect_static_registry = false,
    bool return_not_found = false)
{
    auto acc = std::make_shared<asio::ip::tcp::acceptor>(ioc, target_ep);
    asio::co_spawn(
        ioc,
        listener(std::move(acc), expect_no_gzip, expect_auth, expect_static_registry, return_not_found),
        asio::detached);
}

void start_listening_ssl_server(io_context& ioc)
{
    auto ctx = std::make_shared<boost::asio::ssl::context>(
        boost::asio::ssl::context::tls_server);
    test::load_server_certificate(*ctx);
    auto acc = std::make_shared<asio::ip::tcp::acceptor>(ioc, target_ep);
    asio::co_spawn(ioc, ssl_listener(std::move(acc), std::move(ctx),
        false, false, false, false), asio::detached);
}

BOOST_AUTO_TEST_SUITE(test_pushgateway_simple_push)

BOOST_AUTO_TEST_CASE(push_incorrect_job_name)
{
    io_context ioc;
    pushgateway pgw(ioc.get_executor());
    auto test_with_name = [&](std::string job_name)
    {
        std::size_t counter = 0;
        auto do_push = std::make_shared<std::function<void()>>();
        std::weak_ptr<std::function<void()>> weak = do_push;
        *do_push = [&, weak] {
            pgw.async_push(target_ep, job_name, get_registry(),
                [&, weak](prometheus::error_code ec) {
                    BOOST_CHECK(ec.category() == prometheus::error::get_pushgateway_category());
                    BOOST_TEST(ec == prometheus::pushgateway_errc::incorrect_job_name);
                    BOOST_TEST(ec.message() == "incorrect job name");
                    if (++counter == 10u) { ioc.stop(); return; }
                    if (auto p = weak.lock()) (*p)();
                });
        };
        (*do_push)();

        ioc.run();
    };
    test_with_name("");
    test_with_name("test\r");
    test_with_name("te\nst");
    test_with_name("te st");
    test_with_name("te   st");
}

BOOST_AUTO_TEST_CASE(clear_incorrect_job_name)
{
    io_context ioc;
    pushgateway pgw(ioc.get_executor());
    auto test_with_name = [&](std::string job_name)
    {
        std::size_t counter = 0;
        auto do_clear = std::make_shared<std::function<void()>>();
        std::weak_ptr<std::function<void()>> weak = do_clear;
        *do_clear = [&, weak] {
            pgw.async_clear(target_ep, job_name,
                [&, weak](prometheus::error_code ec) {
                    BOOST_CHECK(ec.category() == prometheus::error::get_pushgateway_category());
                    BOOST_TEST(ec == prometheus::pushgateway_errc::incorrect_job_name);
                    BOOST_TEST(ec.message() == "incorrect job name");
                    if (++counter == 10u) { ioc.stop(); return; }
                    if (auto p = weak.lock()) (*p)();
                });
        };
        (*do_clear)();

        ioc.run();
    };
    test_with_name("");
    test_with_name("test\r");
    test_with_name("te\nst");
    test_with_name("te st");
    test_with_name("te   st");
}

BOOST_AUTO_TEST_CASE(push_connection_refused)
{
    io_context ioc;
    pushgateway pgw(ioc.get_executor());

    std::size_t counter = 0;
    auto do_push = std::make_shared<std::function<void()>>();
    std::weak_ptr<std::function<void()>> weak = do_push;
    *do_push = [&, weak] {
        pgw.async_push(target_ep, test_job_name, get_registry(),
            [&, weak](prometheus::error_code ec) {
                BOOST_CHECK(ec.category() == asio::error::get_system_category());
                BOOST_TEST(ec == asio::error::connection_refused);
                if (++counter == 10u) { ioc.stop(); return; }
                if (auto p = weak.lock()) (*p)();
            });
    };
    (*do_push)();

    ioc.run();
}

BOOST_AUTO_TEST_CASE(clear_connection_refused)
{
    io_context ioc;
    pushgateway pgw(ioc.get_executor());

    std::size_t counter = 0;
    auto do_clear = std::make_shared<std::function<void()>>();
    std::weak_ptr<std::function<void()>> weak = do_clear;
    *do_clear = [&, weak] {
        pgw.async_clear(target_ep, test_job_name,
            [&, weak](prometheus::error_code ec) {
                BOOST_CHECK(ec.category() == asio::error::get_system_category());
                BOOST_TEST(ec == asio::error::connection_refused);
                if (++counter == 10u) { ioc.stop(); return; }
                if (auto p = weak.lock()) (*p)();
            });
    };
    (*do_clear)();

    ioc.run();
}

BOOST_AUTO_TEST_CASE(push_not_found)
{
    io_context ioc;
    start_listening_server(ioc, false, false, false, true);
    pushgateway pgw(ioc.get_executor());

    std::size_t counter = 0;
    auto do_push = std::make_shared<std::function<void()>>();
    std::weak_ptr<std::function<void()>> weak = do_push;
    *do_push = [&, weak] {
        pgw.async_push(target_ep, test_job_name, get_registry(),
            [&, weak](prometheus::error_code ec) {
                BOOST_CHECK(ec.category() == prometheus::error::get_http_status_category());
                BOOST_TEST(ec.message() == "404 Not Found");
                BOOST_TEST(ec.value() == static_cast<int>(beast::http::status::not_found));
                if (++counter == 10u) { ioc.stop(); return; }
                if (auto p = weak.lock()) (*p)();
            });
    };
    (*do_push)();

    ioc.run();
}

BOOST_AUTO_TEST_CASE(clear_not_found)
{
    io_context ioc;
    start_listening_server(ioc, false, false, false, true);
    pushgateway pgw(ioc.get_executor());

    std::size_t counter = 0;
    auto do_clear= std::make_shared<std::function<void()>>();
    std::weak_ptr<std::function<void()>> weak = do_clear;
    *do_clear = [&, weak] {
        pgw.async_clear(target_ep, test_job_name,
            [&, weak](prometheus::error_code ec) {
                BOOST_CHECK(ec.category() == prometheus::error::get_http_status_category());
                BOOST_TEST(ec.message() == "404 Not Found");
                BOOST_TEST(ec.value() == static_cast<int>(beast::http::status::not_found));
                if (++counter == 10u) { ioc.stop(); return; }
                if (auto p = weak.lock()) (*p)();
            });
    };
    (*do_clear)();

    ioc.run();
}

BOOST_AUTO_TEST_CASE(push_basic)
{
    boost::asio::io_context ioc;
    start_listening_server(ioc);
    pushgateway pgw(ioc.get_executor());

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
}

BOOST_AUTO_TEST_CASE(clear_basic)
{
    boost::asio::io_context ioc;
    start_listening_server(ioc);
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

BOOST_AUTO_TEST_CASE(push_disabled_gzip)
{
    boost::asio::io_context ioc;
    start_listening_server(ioc, true);
    pushgateway<
    decltype(ioc.get_executor()), gzip_policy<false>>
        pgw(ioc.get_executor());

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
}

BOOST_AUTO_TEST_CASE(push_auth)
{
    boost::asio::io_context ioc;
    start_listening_server(ioc, false, true);
    pushgateway pgw(ioc.get_executor());

    auto coro_push_auth = [&]() -> boost::asio::awaitable<void>
    {
        for (std::size_t i = 0; i < 10u; i++)
        {
            prometheus::error_code ec;
            co_await pgw.async_push(target_ep, test_job_name, "Bearer secret-key", get_registry(),
                asio::redirect_error(asio::use_awaitable, ec));

            BOOST_TEST(ec == prometheus::error_code());
        }
    };
    boost::asio::co_spawn(
        ioc.get_executor(),
        coro_push_auth(),
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

BOOST_AUTO_TEST_CASE(clear_auth)
{
    boost::asio::io_context ioc;
    start_listening_server(ioc, false, true);
    pushgateway pgw(ioc.get_executor());

    auto coro_clear_auth = [&]() -> boost::asio::awaitable<void>
    {
        for (std::size_t i = 0; i < 10u; i++)
        {
            prometheus::error_code ec;
            co_await pgw.async_clear(target_ep, test_job_name, "Bearer secret-key",
                asio::redirect_error(asio::use_awaitable, ec));

            BOOST_TEST(ec == prometheus::error_code());
        }
    };
    boost::asio::co_spawn(
        ioc.get_executor(),
        coro_clear_auth(),
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

BOOST_AUTO_TEST_CASE(push_ssl_mode)
{
    boost::asio::io_context ioc;
    start_listening_ssl_server(ioc);
    boost::asio::ssl::context client_ctx{boost::asio::ssl::context::tls_client};
    test::load_client_certificate(client_ctx);
    pushgateway<
    decltype(ioc.get_executor()), ssl_policy<true>>
        pgw(ioc.get_executor(), client_ctx);

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
}

BOOST_AUTO_TEST_CASE(push_static_registry)
{
    boost::asio::io_context ioc;
    start_listening_server(ioc, false, false, true);
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
}

BOOST_AUTO_TEST_CASE(push_auth_static_registry)
{
    boost::asio::io_context ioc;
    start_listening_server(ioc, false, true, true);
    pushgateway pgw(ioc.get_executor());
    setup_static_registry();

    auto coro = [&]() -> boost::asio::awaitable<void>
    {
        for (std::size_t i = 0; i < 10u; i++)
        {
            prometheus::error_code ec;
            co_await pgw.async_push<premade_static_registry>(target_ep, test_job_name, "Bearer secret-key",
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
}

BOOST_AUTO_TEST_CASE(clear_ssl_mode)
{
    boost::asio::io_context ioc;
    start_listening_ssl_server(ioc);
    boost::asio::ssl::context client_ctx{boost::asio::ssl::context::tls_client};
    test::load_client_certificate(client_ctx);
    pushgateway<
    decltype(ioc.get_executor()), ssl_policy<true>>
        pgw(ioc.get_executor(), client_ctx);

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
