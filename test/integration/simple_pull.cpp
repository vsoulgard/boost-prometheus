//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/exposer.hpp>
#include <boost/prometheus/exposer_ssl_policy.hpp>
#include <boost/prometheus/static_counter.hpp>
#include <boost/prometheus/static_histogram.hpp>
#include <boost/prometheus/static_info.hpp>
#include <boost/prometheus/static_registry.hpp>

#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>

#include <boost/asio/thread_pool.hpp>

#include <boost/test/unit_test.hpp>

#include "test_common/gzip_decode.hpp"
#include "test_common/client_certificate.hpp"
#include "test_common/server_certificate.hpp"

#include <string_view>
#include <thread>

using namespace boost;
using namespace beast;
using namespace boost::asio;
using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_exposer_simple_pull)

std::string make_request(
    beast::tcp_stream& stream,
    std::string_view host,
    std::string_view route,
    std::string_view accept)
{
    http::request<http::empty_body> req{http::verb::get, route, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, "Prometheus/2.51.0");
    req.set(http::field::accept, accept);
    req.keep_alive(true);

    beast::error_code ec;
    http::write(stream, req, ec);
    if (ec)
    {
        BOOST_FAIL("Failed to write request: " << ec.message());
    }

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    
    http::read(stream, buffer, res, ec);
    if (ec)
    {
        BOOST_FAIL("Failed to read response: " << ec.message());
    }

    BOOST_TEST(res.keep_alive() == true);
    BOOST_TEST(res.result() == http::status::ok);

    return std::move(res.body());
}

BOOST_AUTO_TEST_CASE(prometheus_0_0_4)
{
    io_context ioc;
    ip::tcp::endpoint target_ep(asio::ip::address_v4::loopback(), 8080);
    exposer exp(ioc.get_executor(), target_ep, "/metrics");

    // Run exposer in other thread
    std::thread ioc_thread([&ioc]() {
        ioc.run();
    });

    beast::tcp_stream stream(ioc);

    beast::error_code ec;
    stream.connect(target_ep, ec);
    if (ec)
    {
        BOOST_FAIL("Failed to connect: " << ec.message());
    }

    // Empty exposer
    std::string result = make_request(stream, "127.0.0.1:8080", "/metrics", "text/plain;version=0.0.4");
    BOOST_TEST(result.empty());
    // Empty registry
    registry reg;
    exp.expose(reg);
    result = make_request(stream, "127.0.0.1:8080", "/metrics", "text/plain;version=0.0.4");
    BOOST_TEST(result.empty());
    // Add one counter
    counter c("test_seconds", {.help = "help text", .unit = "seconds"});
    reg.register_metric(c);
    result = make_request(stream, "127.0.0.1:8080", "/metrics", "text/plain;version=0.0.4");
    BOOST_TEST(result == std::string_view(
        "# TYPE test_seconds_total counter\n"
        "# HELP test_seconds_total help text\n"
        "test_seconds_total 0\n"
    ));
    c.inc(0.56);
    reg.register_metric(c);
    result = make_request(stream, "127.0.0.1:8080", "/metrics", "text/plain;version=0.0.4");
    BOOST_TEST(result == std::string_view(
        "# TYPE test_seconds_total counter\n"
        "# HELP test_seconds_total help text\n"
        "test_seconds_total 0.56\n"
    ));
    // Add different metrics
    histogram h("request_duration_seconds", {.help = "", .unit = "seconds", .buckets = {}});
    gauge g("test_g");
    counter_vec cv("http_requests", {"method"});
    cv.with_labels({"GET"}).inc();
    g.set(900.0);
    reg.register_metric(h);
    reg.register_metric(g);
    reg.register_metric(cv);
    result = make_request(stream, "127.0.0.1:8080", "/metrics", "text/plain;version=0.0.4");
    BOOST_TEST(result.find(
        "# TYPE test_seconds_total counter\n"
        "# HELP test_seconds_total help text\n"
        "test_seconds_total 0.56\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.find(
        "# TYPE request_duration_seconds histogram\n"
        "request_duration_seconds_bucket{le=\"0.005\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.01\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.025\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.05\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.1\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.25\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.5\"} 0\n"
        "request_duration_seconds_bucket{le=\"1\"} 0\n"
        "request_duration_seconds_bucket{le=\"2.5\"} 0\n"
        "request_duration_seconds_bucket{le=\"5\"} 0\n"
        "request_duration_seconds_bucket{le=\"10\"} 0\n"
        "request_duration_seconds_bucket{le=\"+Inf\"} 0\n"
        "request_duration_seconds_sum 0\n"
        "request_duration_seconds_count 0\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.find(
        "# TYPE test_g gauge\n"
        "test_g 900\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.find(
        "# TYPE http_requests_total counter\n"
        "http_requests_total{method=\"GET\"} 1\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.size() == 94u + 637u + 31u + 71u);

    stream.socket().shutdown(ip::tcp::socket::shutdown_both, ec);

    ioc.stop();
    if (ioc_thread.joinable())
        ioc_thread.join();
    BOOST_TEST(ioc.stopped());
}

BOOST_AUTO_TEST_CASE(openmetrics_1_0)
{
    io_context ioc;
    ip::tcp::endpoint target_ep(asio::ip::address_v4::loopback(), 8080);
    exposer exp(ioc.get_executor(), target_ep, "/metrics");

    // Run exposer in other thread
    std::thread ioc_thread([&ioc]() {
        ioc.run();
    });

    beast::tcp_stream stream(ioc);

    beast::error_code ec;
    stream.connect(target_ep, ec);
    if (ec)
    {
        BOOST_FAIL("Failed to connect: " << ec.message());
    }

    // Empty exposer
    std::string result = make_request(stream, "127.0.0.1:8080", "/metrics", "application/openmetrics-text;version=1.0.0");
    BOOST_TEST(result == "# EOF\n");
    // Empty registry
    registry reg;
    exp.expose(reg);
    result = make_request(stream, "127.0.0.1:8080", "/metrics", "application/openmetrics-text;version=1.0.0");
    BOOST_TEST(result == "# EOF\n");
    // Add one counter
    counter c("test_seconds", {.help = "help text", .unit = "seconds"});
    reg.register_metric(c);
    result = make_request(stream, "127.0.0.1:8080", "/metrics", "application/openmetrics-text;version=1.0.0");
    BOOST_TEST(result == std::string_view(
        "# TYPE test_seconds counter\n"
        "# UNIT test_seconds seconds\n"
        "# HELP test_seconds help text\n"
        "test_seconds_total 0\n"
        "# EOF\n"
    ));
    c.inc(0.56);
    reg.register_metric(c);
    result = make_request(stream, "127.0.0.1:8080", "/metrics", "application/openmetrics-text;version=1.0.0");
    BOOST_TEST(result == std::string_view(
        "# TYPE test_seconds counter\n"
        "# UNIT test_seconds seconds\n"
        "# HELP test_seconds help text\n"
        "test_seconds_total 0.56\n"
        "# EOF\n"
    ));
    // Add different metrics
    histogram h("request_duration_seconds", {.help = "", .unit = "seconds", .buckets = {}});
    gauge g("test_g");
    counter_vec cv("http_requests", {"method"});
    cv.with_labels({"GET"}).inc();
    g.set(900.0);
    reg.register_metric(h);
    reg.register_metric(g);
    reg.register_metric(cv);
    result = make_request(stream, "127.0.0.1:8080", "/metrics", "application/openmetrics-text;version=1.0.0");
    BOOST_TEST(result.find(
        "# TYPE test_seconds counter\n"
        "# UNIT test_seconds seconds\n"
        "# HELP test_seconds help text\n"
        "test_seconds_total 0.56\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.find(
        "# TYPE request_duration_seconds histogram\n"
        "# UNIT request_duration_seconds seconds\n"
        "request_duration_seconds_bucket{le=\"0.005\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.01\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.025\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.05\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.1\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.25\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.5\"} 0\n"
        "request_duration_seconds_bucket{le=\"1\"} 0\n"
        "request_duration_seconds_bucket{le=\"2.5\"} 0\n"
        "request_duration_seconds_bucket{le=\"5\"} 0\n"
        "request_duration_seconds_bucket{le=\"10\"} 0\n"
        "request_duration_seconds_bucket{le=\"+Inf\"} 0\n"
        "request_duration_seconds_sum 0\n"
        "request_duration_seconds_count 0\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.find(
        "# TYPE test_g gauge\n"
        "test_g 900\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.find(
        "# TYPE http_requests counter\n"
        "http_requests_total{method=\"GET\"} 1\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.substr(result.size() - 6u, 6u) == std::string_view(
        "# EOF\n"
    ));
    BOOST_TEST(result.size() == 110u + 677u + 31u + 65u + 6u);

    stream.socket().shutdown(ip::tcp::socket::shutdown_both, ec);

    ioc.stop();
    if (ioc_thread.joinable())
        ioc_thread.join();
    BOOST_TEST(ioc.stopped());
}

BOOST_AUTO_TEST_CASE(registry_swap)
{
    io_context ioc;
    ip::tcp::endpoint target_ep(asio::ip::address_v4::loopback(), 8080);
    exposer exp(ioc.get_executor(), target_ep, "/metrics");

    // Run exposer in other thread
    std::thread ioc_thread([&ioc]() {
        ioc.run();
    });

    beast::tcp_stream stream(ioc);

    beast::error_code ec;
    stream.connect(target_ep, ec);
    if (ec)
    {
        BOOST_FAIL("Failed to connect: " << ec.message());
    }

    registry reg_1, reg_2;
    exp.expose(reg_1);
    counter c("test_seconds", {.help = "help text", .unit = "seconds"});
    reg_1.register_metric(c);
    c.inc(0.56);
    reg_1.register_metric(c);
    histogram h("request_duration_seconds", {.help = "", .unit = "seconds", .buckets = {}});
    gauge g("test_g");
    counter_vec cv("http_requests", {"method"});
    cv.with_labels({"GET"}).inc();
    g.set(900.0);
    reg_1.register_metric(h);
    reg_1.register_metric(g);
    reg_1.register_metric(cv);
    std::string result = make_request(stream, "127.0.0.1:8080", "/metrics", "application/openmetrics-text;version=1.0.0");
    BOOST_TEST(result.find(
        "# TYPE test_seconds counter\n"
        "# UNIT test_seconds seconds\n"
        "# HELP test_seconds help text\n"
        "test_seconds_total 0.56\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.find(
        "# TYPE request_duration_seconds histogram\n"
        "# UNIT request_duration_seconds seconds\n"
        "request_duration_seconds_bucket{le=\"0.005\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.01\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.025\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.05\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.1\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.25\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.5\"} 0\n"
        "request_duration_seconds_bucket{le=\"1\"} 0\n"
        "request_duration_seconds_bucket{le=\"2.5\"} 0\n"
        "request_duration_seconds_bucket{le=\"5\"} 0\n"
        "request_duration_seconds_bucket{le=\"10\"} 0\n"
        "request_duration_seconds_bucket{le=\"+Inf\"} 0\n"
        "request_duration_seconds_sum 0\n"
        "request_duration_seconds_count 0\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.find(
        "# TYPE test_g gauge\n"
        "test_g 900\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.find(
        "# TYPE http_requests counter\n"
        "http_requests_total{method=\"GET\"} 1\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.substr(result.size() - 6u, 6u) == std::string_view(
        "# EOF\n"
    ));
    BOOST_TEST(result.size() == 110u + 677u + 31u + 65u + 6u);

    // Swap registry
    exp.expose(reg_2);
    result = make_request(stream, "127.0.0.1:8080", "/metrics", "application/openmetrics-text;version=1.0.0");
    BOOST_TEST(result == "# EOF\n");
    // Add one counter
    reg_2.register_metric(c);
    result = make_request(stream, "127.0.0.1:8080", "/metrics", "application/openmetrics-text;version=1.0.0");
    BOOST_TEST(result == std::string_view(
        "# TYPE test_seconds counter\n"
        "# UNIT test_seconds seconds\n"
        "# HELP test_seconds help text\n"
        "test_seconds_total 0.56\n"
        "# EOF\n"
    ));
    // Add different metrics
    reg_2.register_metric(h);
    reg_2.register_metric(g);
    reg_2.register_metric(cv);
    result = make_request(stream, "127.0.0.1:8080", "/metrics", "application/openmetrics-text;version=1.0.0");
    BOOST_TEST(result.find(
        "# TYPE test_seconds counter\n"
        "# UNIT test_seconds seconds\n"
        "# HELP test_seconds help text\n"
        "test_seconds_total 0.56\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.find(
        "# TYPE request_duration_seconds histogram\n"
        "# UNIT request_duration_seconds seconds\n"
        "request_duration_seconds_bucket{le=\"0.005\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.01\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.025\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.05\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.1\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.25\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.5\"} 0\n"
        "request_duration_seconds_bucket{le=\"1\"} 0\n"
        "request_duration_seconds_bucket{le=\"2.5\"} 0\n"
        "request_duration_seconds_bucket{le=\"5\"} 0\n"
        "request_duration_seconds_bucket{le=\"10\"} 0\n"
        "request_duration_seconds_bucket{le=\"+Inf\"} 0\n"
        "request_duration_seconds_sum 0\n"
        "request_duration_seconds_count 0\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.find(
        "# TYPE test_g gauge\n"
        "test_g 900\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.find(
        "# TYPE http_requests counter\n"
        "http_requests_total{method=\"GET\"} 1\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.substr(result.size() - 6u, 6u) == std::string_view(
        "# EOF\n"
    ));
    BOOST_TEST(result.size() == 110u + 677u + 31u + 65u+ 6u);

    stream.socket().shutdown(ip::tcp::socket::shutdown_both, ec);

    ioc.stop();
    if (ioc_thread.joinable())
        ioc_thread.join();
    BOOST_TEST(ioc.stopped());
}

BOOST_AUTO_TEST_CASE(concurrent_scraping)
{
    constexpr std::size_t num_server_threads = 4u;
    constexpr std::size_t num_scrapers = 4u;
    constexpr std::size_t scrapes_per_thread = 500u;

    asio::thread_pool pool(num_server_threads);

    ip::tcp::endpoint target_ep(asio::ip::address_v4::loopback(), 8080);
    exposer exp(pool.get_executor(), target_ep, "/metrics");

    registry reg;
    counter c("test_seconds", {.help = "help text", .unit = "seconds"});
    c.inc(0.56);
    gauge g("test_g");
    g.set(900.0);
    histogram h("request_duration_seconds", {.help = "", .unit = "seconds", .buckets = {}});
    reg.register_metric(c);
    reg.register_metric(g);
    reg.register_metric(h);
    exp.expose(reg);

    std::atomic<std::size_t> successes{0u};
    std::atomic<std::size_t> failures{0u};

    std::vector<std::thread> scrapers;
    scrapers.reserve(num_scrapers);

    for (std::size_t i = 0u; i < num_scrapers; i++)
    {
        scrapers.emplace_back([&]() {
            io_context local_ioc;
            beast::tcp_stream stream(local_ioc);

            beast::error_code ec;
            stream.connect(target_ep, ec);
            if (ec)
            {
                failures.fetch_add(1u, std::memory_order_release);
                return;
            }

            for (std::size_t j = 0u; j < scrapes_per_thread; j++)
            {
                http::request<http::empty_body> req{
                    http::verb::get, "/metrics", 11};
                req.set(http::field::host, "127.0.0.1:8083");
                req.set(http::field::user_agent, "Prometheus/2.51.0");
                req.set(http::field::accept,
                        "application/openmetrics-text;version=1.0.0");
                req.keep_alive(true);

                http::write(stream, req, ec);
                if (ec)
                {
                    failures.fetch_add(1u, std::memory_order_release);
                    return;
                }

                beast::flat_buffer buffer;
                http::response<http::string_body> res;
                http::read(stream, buffer, res, ec);
                if (ec)
                {
                    failures.fetch_add(1u, std::memory_order_release);
                    return;
                }

                if (res.result() != http::status::ok)
                {
                    failures.fetch_add(1u, std::memory_order_release);
                    return;
                }

                auto const& body = res.body();
                // Body must end with the OpenMetrics EOF marker.
                if (body.size() < 6u ||
                    body.substr(body.size() - 6u) != "# EOF\n")
                {
                    failures.fetch_add(1, std::memory_order_release);
                    return;
                }
                // Body must contain the expected metrics.
                if (body.find("test_seconds_total 0.56\n") == std::string::npos ||
                    body.find("test_g 900\n")               == std::string::npos ||
                    body.find("# TYPE request_duration_seconds histogram\n")
                        == std::string::npos)
                {
                    failures.fetch_add(1u, std::memory_order_release);
                    return;
                }
                successes.fetch_add(1u, std::memory_order_release);
            }

            stream.socket().shutdown(ip::tcp::socket::shutdown_both, ec);
        });
    }

    for (auto& t : scrapers)
    {
        if (t.joinable())
            t.join();
    }

    pool.stop();
    pool.join();
    BOOST_TEST(failures.load(std::memory_order_acquire) == 0u);
    BOOST_TEST(successes.load(std::memory_order_acquire) == (num_scrapers * scrapes_per_thread));
}

BOOST_AUTO_TEST_CASE(authorization)
{
    io_context ioc;
    ip::tcp::endpoint target_ep(asio::ip::address_v4::loopback(), 8080);
    exposer<decltype(ioc.get_executor()),
    authorization_policy<"Bearer", [](std::string_view auth)
    {
        return auth == std::string_view("Bearer secret-key");
    }>>
    exp(ioc.get_executor(), target_ep, "/metrics");

    // Run exposer in other thread
    std::thread ioc_thread([&ioc]() {
        ioc.run();
    });

    beast::tcp_stream stream(ioc);

    beast::error_code ec;
    stream.connect(target_ep, ec);
    if (ec)
    {
        BOOST_FAIL("Failed to connect: " << ec.message());
    }

    // Without a token
    {
        http::request<http::empty_body> req{http::verb::get, "/metrics", 11};
        req.set(http::field::host, "127.0.0.1:8080");
        req.set(http::field::user_agent, "Prometheus/2.51.0");
        req.set(http::field::accept, "text/plain;version=0.0.4");
        req.keep_alive(true);

        http::write(stream, req, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to write request: " << ec.message());
        }

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to read response: " << ec.message());
        }

        BOOST_TEST(res.result() == http::status::unauthorized);
        auto www_auth = res.find(http::field::www_authenticate);
        BOOST_CHECK(www_auth != res.end());
        BOOST_TEST(www_auth->value() == "Bearer");
    }
    // Invalid token
    {
        http::request<http::empty_body> req{http::verb::get, "/metrics", 11};
        req.set(http::field::host, "127.0.0.1:8080");
        req.set(http::field::user_agent, "Prometheus/2.51.0");
        req.set(http::field::accept, "text/plain;version=0.0.4");
        req.set(http::field::authorization, "Bearer wrong-key");
        req.keep_alive(true);

        http::write(stream, req, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to write request: " << ec.message());
        }

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to read response: " << ec.message());
        }

        BOOST_TEST(res.result() == http::status::unauthorized);
        auto www_auth = res.find(http::field::www_authenticate);
        BOOST_CHECK(www_auth != res.end());
        BOOST_TEST(www_auth->value() == "Bearer");
    }
    // Success
    {
        http::request<http::empty_body> req{http::verb::get, "/metrics", 11};
        req.set(http::field::host, "127.0.0.1:8080");
        req.set(http::field::user_agent, "Prometheus/2.51.0");
        req.set(http::field::accept, "text/plain;version=0.0.4");
        req.set(http::field::authorization, "Bearer secret-key");
        req.keep_alive(true);

        http::write(stream, req, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to write request: " << ec.message());
        }

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to read response: " << ec.message());
        }

        BOOST_TEST(res.result() == http::status::ok);
        auto www_auth = res.find(http::field::www_authenticate);
        BOOST_CHECK(www_auth == res.end());
    }

    stream.socket().shutdown(ip::tcp::socket::shutdown_both, ec);

    ioc.stop();
    if (ioc_thread.joinable())
        ioc_thread.join();
    BOOST_TEST(ioc.stopped());
}

BOOST_AUTO_TEST_CASE(authorization_with_realm)
{
    io_context ioc;
    ip::tcp::endpoint target_ep(asio::ip::address_v4::loopback(), 8080);
    exposer<decltype(ioc.get_executor()),
    authorization_policy<"Bearer", [](std::string_view auth)
    {
        return auth == std::string_view("Bearer secret-key");
    }, "Some realm">>
    exp(ioc.get_executor(), target_ep, "/metrics");

    // Run exposer in other thread
    std::thread ioc_thread([&ioc]() {
        ioc.run();
    });

    beast::tcp_stream stream(ioc);

    beast::error_code ec;
    stream.connect(target_ep, ec);
    if (ec)
    {
        BOOST_FAIL("Failed to connect: " << ec.message());
    }

    // Without a token
    {
        http::request<http::empty_body> req{http::verb::get, "/metrics", 11};
        req.set(http::field::host, "127.0.0.1:8080");
        req.set(http::field::user_agent, "Prometheus/2.51.0");
        req.set(http::field::accept, "text/plain;version=0.0.4");
        req.keep_alive(true);

        http::write(stream, req, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to write request: " << ec.message());
        }

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to read response: " << ec.message());
        }

        BOOST_TEST(res.result() == http::status::unauthorized);
        auto www_auth = res.find(http::field::www_authenticate);
        BOOST_CHECK(www_auth != res.end());
        BOOST_TEST(www_auth->value() == "Bearer realm=\"Some realm\"");
    }
    // Invalid token
    {
        http::request<http::empty_body> req{http::verb::get, "/metrics", 11};
        req.set(http::field::host, "127.0.0.1:8080");
        req.set(http::field::user_agent, "Prometheus/2.51.0");
        req.set(http::field::accept, "text/plain;version=0.0.4");
        req.set(http::field::authorization, "Bearer wrong-key");
        req.keep_alive(true);

        http::write(stream, req, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to write request: " << ec.message());
        }

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to read response: " << ec.message());
        }

        BOOST_TEST(res.result() == http::status::unauthorized);
        auto www_auth = res.find(http::field::www_authenticate);
        BOOST_CHECK(www_auth != res.end());
        BOOST_TEST(www_auth->value() == "Bearer realm=\"Some realm\"");
    }
    // Success
    {
        http::request<http::empty_body> req{http::verb::get, "/metrics", 11};
        req.set(http::field::host, "127.0.0.1:8080");
        req.set(http::field::user_agent, "Prometheus/2.51.0");
        req.set(http::field::accept, "text/plain;version=0.0.4");
        req.set(http::field::authorization, "Bearer secret-key");
        req.keep_alive(true);

        http::write(stream, req, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to write request: " << ec.message());
        }

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to read response: " << ec.message());
        }

        BOOST_TEST(res.result() == http::status::ok);
        auto www_auth = res.find(http::field::www_authenticate);
        BOOST_CHECK(www_auth == res.end());
    }

    stream.socket().shutdown(ip::tcp::socket::shutdown_both, ec);

    ioc.stop();
    if (ioc_thread.joinable())
        ioc_thread.join();
    BOOST_TEST(ioc.stopped());
}

BOOST_AUTO_TEST_CASE(authorization_with_not_found)
{
    io_context ioc;
    ip::tcp::endpoint target_ep(asio::ip::address_v4::loopback(), 8080);
    exposer<decltype(ioc.get_executor()),
    authorization_policy<"Bearer", [](std::string_view auth)
    {
        return auth == std::string_view("Bearer secret-key");
    }, "", true>>
    exp(ioc.get_executor(), target_ep, "/metrics");

    // Run exposer in other thread
    std::thread ioc_thread([&ioc]() {
        ioc.run();
    });

    beast::tcp_stream stream(ioc);

    beast::error_code ec;
    stream.connect(target_ep, ec);
    if (ec)
    {
        BOOST_FAIL("Failed to connect: " << ec.message());
    }

    // Without a token
    {
        http::request<http::empty_body> req{http::verb::get, "/metrics", 11};
        req.set(http::field::host, "127.0.0.1:8080");
        req.set(http::field::user_agent, "Prometheus/2.51.0");
        req.set(http::field::accept, "text/plain;version=0.0.4");
        req.keep_alive(true);

        http::write(stream, req, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to write request: " << ec.message());
        }

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to read response: " << ec.message());
        }

        BOOST_TEST(res.result() == http::status::not_found);
        auto www_auth = res.find(http::field::www_authenticate);
        BOOST_CHECK(www_auth == res.end());
    }
    // Invalid token
    {
        http::request<http::empty_body> req{http::verb::get, "/metrics", 11};
        req.set(http::field::host, "127.0.0.1:8080");
        req.set(http::field::user_agent, "Prometheus/2.51.0");
        req.set(http::field::accept, "text/plain;version=0.0.4");
        req.set(http::field::authorization, "Bearer wrong-key");
        req.keep_alive(true);

        http::write(stream, req, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to write request: " << ec.message());
        }

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to read response: " << ec.message());
        }

        BOOST_TEST(res.result() == http::status::not_found);
        auto www_auth = res.find(http::field::www_authenticate);
        BOOST_CHECK(www_auth == res.end());
    }
    // Success
    {
        http::request<http::empty_body> req{http::verb::get, "/metrics", 11};
        req.set(http::field::host, "127.0.0.1:8080");
        req.set(http::field::user_agent, "Prometheus/2.51.0");
        req.set(http::field::accept, "text/plain;version=0.0.4");
        req.set(http::field::authorization, "Bearer secret-key");
        req.keep_alive(true);

        http::write(stream, req, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to write request: " << ec.message());
        }

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to read response: " << ec.message());
        }

        BOOST_TEST(res.result() == http::status::ok);
        auto www_auth = res.find(http::field::www_authenticate);
        BOOST_CHECK(www_auth == res.end());
    }

    stream.socket().shutdown(ip::tcp::socket::shutdown_both, ec);

    ioc.stop();
    if (ioc_thread.joinable())
        ioc_thread.join();
    BOOST_TEST(ioc.stopped());
}

BOOST_AUTO_TEST_CASE(invalid_header)
{
    // Request with very big header
    io_context ioc;
    ip::tcp::endpoint target_ep(asio::ip::address_v4::loopback(), 8080);
    exposer exp(ioc.get_executor(), target_ep, "/metrics");

    // Run exposer in other thread
    std::thread ioc_thread([&ioc]() {
        ioc.run();
    });

    beast::tcp_stream stream(ioc);

    beast::error_code ec;
    stream.connect(target_ep, ec);
    if (ec)
    {
        BOOST_FAIL("Failed to connect: " << ec.message());
    }

    // Create request with a huge header
    http::request<http::empty_body> req{http::verb::get, "/metrics", 11};
    req.set(http::field::host, "127.0.0.1:8080");
    req.set(http::field::user_agent, "Prometheus/2.51.0");
    req.set(http::field::accept, "text/plain;version=0.0.4");
    req.keep_alive(true);
    // Add a huge header, e.g., 64 KB
    auto const max_value = beast::http::fields::max_value_size;
    std::string huge_value(max_value, 'x');
    req.set("X-Huge-Header", huge_value);

    // Try to write the request. The server might close the connection during write.
    http::write(stream, req, ec);
    if (!ec)
    {
        // If write succeeded, try to read response. We expect an error because
        // the server should close the connection without sending a response.
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res, ec);
        // The error should indicate end of stream or connection reset.
        BOOST_CHECK(ec == beast::http::error::end_of_stream || ec == asio::error::connection_reset);
    }
    else
    {
        // Write failed, which means the server closed the connection.
        // This is also acceptable.
        BOOST_CHECK(ec == asio::error::connection_reset || ec == asio::error::broken_pipe);
    }

    stream.socket().shutdown(ip::tcp::socket::shutdown_both, ec);

    ioc.stop();
    if (ioc_thread.joinable())
        ioc_thread.join();
    BOOST_TEST(ioc.stopped());
}

BOOST_AUTO_TEST_CASE(invalid_method)
{
    // A request with any method other than GET must
    // receive a response with HTTP status code method_not_allowed
    // and an Allow header set to GET.
    io_context ioc;
    ip::tcp::endpoint target_ep(asio::ip::address_v4::loopback(), 8080);
    exposer exp(ioc.get_executor(), target_ep, "/metrics");

    // Run exposer in other thread
    std::thread ioc_thread([&ioc]() {
        ioc.run();
    });

    beast::tcp_stream stream(ioc);

    beast::error_code ec;
    stream.connect(target_ep, ec);
    if (ec)
    {
        BOOST_FAIL("Failed to connect: " << ec.message());
    }

    // Send POST request
    {
        http::request<http::empty_body> req{http::verb::post, "/metrics", 11};
        req.set(http::field::host, "127.0.0.1:8080");
        req.set(http::field::user_agent, "Prometheus/2.51.0");
        req.set(http::field::accept, "text/plain;version=0.0.4");
        req.keep_alive(true);
    
        http::write(stream, req, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to write request: " << ec.message());
        }
    
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to read response: " << ec.message());
        }
    
        BOOST_TEST(res.result() == http::status::method_not_allowed);
        auto allow = res.find(http::field::allow);
        BOOST_CHECK(allow != res.end());
        BOOST_TEST(allow->value() == "GET");
    }
    // Send HEAD request
    {
        http::request<http::empty_body> req{http::verb::head, "/metrics", 11};
        req.set(http::field::host, "127.0.0.1:8080");
        req.set(http::field::user_agent, "Prometheus/2.51.0");
        req.set(http::field::accept, "text/plain;version=0.0.4");
        req.keep_alive(true);
    
        http::write(stream, req, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to write request: " << ec.message());
        }
    
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res, ec);
        if (ec)
        {
            BOOST_FAIL("Failed to read response: " << ec.message());
        }
    
        BOOST_TEST(res.result() == http::status::method_not_allowed);
        auto allow = res.find(http::field::allow);
        BOOST_CHECK(allow != res.end());
        BOOST_TEST(allow->value() == "GET");
    }

    stream.socket().shutdown(ip::tcp::socket::shutdown_both, ec);

    ioc.stop();
    if (ioc_thread.joinable())
        ioc_thread.join();
    BOOST_TEST(ioc.stopped());
}

BOOST_AUTO_TEST_CASE(invalid_route)
{
    // A request with invalid route must
    // receive a response with HTTP status code not_found
    io_context ioc;
    ip::tcp::endpoint target_ep(asio::ip::address_v4::loopback(), 8080);
    exposer exp(ioc.get_executor(), target_ep, "/metrics");

    // Run exposer in other thread
    std::thread ioc_thread([&ioc]() {
        ioc.run();
    });

    beast::tcp_stream stream(ioc);

    beast::error_code ec;
    stream.connect(target_ep, ec);
    if (ec)
    {
        BOOST_FAIL("Failed to connect: " << ec.message());
    }

    http::request<http::empty_body> req{http::verb::get, "/home", 11};
    req.set(http::field::host, "127.0.0.1:8080");
    req.set(http::field::user_agent, "Prometheus/2.51.0");
    req.set(http::field::accept, "text/plain;version=0.0.4");
    req.keep_alive(true);

    http::write(stream, req, ec);
    if (ec)
    {
        BOOST_FAIL("Failed to write request: " << ec.message());
    }

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res, ec);
    if (ec)
    {
        BOOST_FAIL("Failed to read response: " << ec.message());
    }

    BOOST_TEST(res.result() == http::status::not_found);

    stream.socket().shutdown(ip::tcp::socket::shutdown_both, ec);

    ioc.stop();
    if (ioc_thread.joinable())
        ioc_thread.join();
    BOOST_TEST(ioc.stopped());
}

BOOST_AUTO_TEST_CASE(gzip_encoding)
{
    // Since gzip compression threshold is 1024 bytes,
    // we expect that smaller responses are still in
    // plain text format.
    io_context ioc;
    ip::tcp::endpoint target_ep(asio::ip::address_v4::loopback(), 8080);
    exposer exp(ioc.get_executor(), target_ep, "/metrics");

    // Run exposer in other thread
    std::thread ioc_thread([&ioc]() {
        ioc.run();
    });

    beast::tcp_stream stream(ioc);

    beast::error_code ec;
    stream.connect(target_ep, ec);
    if (ec) BOOST_FAIL("Failed to connect: " << ec.message());
    // Empty exposer
    std::string result = make_request(stream, "127.0.0.1:8080", "/metrics", "text/plain;version=0.0.4");
    BOOST_TEST(result.empty());
    // Empty registry
    registry reg;
    exp.expose(reg);
    BOOST_TEST(result.empty());
    // Add one counter
    counter c("test_seconds", {.help = "help text", .unit = "seconds"});
    reg.register_metric(c);
    c.inc(0.56);
    reg.register_metric(c);
    histogram h("request_duration_seconds", {.help = "", .unit = "seconds", .buckets = {}});
    gauge g("test_g");
    counter_vec cv("http_requests", {"method"});
    cv.with_labels({"GET"}).inc();
    g.set(900.0);
    reg.register_metric(h);
    reg.register_metric(g);
    reg.register_metric(cv);
    // Expect plain text
    {
        http::request<http::empty_body> req{http::verb::get, "/metrics", 11};
        req.set(http::field::host, "127.0.0.1:8080");
        req.set(http::field::user_agent, "Prometheus/2.51.0");
        req.set(http::field::accept, "text/plain;version=0.0.4");
        req.set(http::field::accept_encoding, "gzip");
        req.keep_alive(true);

        http::write(stream, req, ec);
        if (ec) BOOST_FAIL("Failed to write request: " << ec.message());

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res, ec);
        if (ec) BOOST_FAIL("Failed to read response: " << ec.message());

        BOOST_TEST(res.result() == http::status::ok);
        auto encoding = res.find(http::field::content_encoding);
        BOOST_REQUIRE(encoding == res.end());

        BOOST_TEST(res.body().find(
            "# TYPE test_seconds_total counter\n"
            "# HELP test_seconds_total help text\n"
            "test_seconds_total 0.56\n"
        ) != std::string_view::npos);
        BOOST_TEST(res.body().find(
            "# TYPE request_duration_seconds histogram\n"
            "request_duration_seconds_bucket{le=\"0.005\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.01\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.025\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.05\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.1\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.25\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.5\"} 0\n"
            "request_duration_seconds_bucket{le=\"1\"} 0\n"
            "request_duration_seconds_bucket{le=\"2.5\"} 0\n"
            "request_duration_seconds_bucket{le=\"5\"} 0\n"
            "request_duration_seconds_bucket{le=\"10\"} 0\n"
            "request_duration_seconds_bucket{le=\"+Inf\"} 0\n"
            "request_duration_seconds_sum 0\n"
            "request_duration_seconds_count 0\n"
        ) != std::string_view::npos);
        BOOST_TEST(res.body().find(
            "# TYPE test_g gauge\n"
            "test_g 900\n"
        ) != std::string_view::npos);
        BOOST_TEST(res.body().find(
            "# TYPE http_requests_total counter\n"
            "http_requests_total{method=\"GET\"} 1\n"
        ) != std::string_view::npos);
        BOOST_TEST(res.body().size() == 94u + 637u + 31u + 71u);
    }
    // Add more metrics to force the exposer to encode
    histogram h2("some_distribution", {.help = "another hist", .unit = "", .buckets = {}});
    reg.register_metric(h2);
    // Expect gzip encoded
    {
        http::request<http::empty_body> req{http::verb::get, "/metrics", 11};
        req.set(http::field::host, "127.0.0.1:8080");
        req.set(http::field::user_agent, "Prometheus/2.51.0");
        req.set(http::field::accept, "text/plain;version=0.0.4");
        req.set(http::field::accept_encoding, "gzip");
        req.keep_alive(true);

        http::write(stream, req, ec);
        if (ec) BOOST_FAIL("Failed to write request: " << ec.message());

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res, ec);
        if (ec) BOOST_FAIL("Failed to read response: " << ec.message());

        BOOST_TEST(res.result() == http::status::ok);
        auto encoding = res.find(http::field::content_encoding);
        BOOST_CHECK(encoding != res.end());
        BOOST_TEST(encoding->value() == "gzip");

        std::string decoded;
        BOOST_CHECK(test::gzip_decode(res.body(), decoded));

        BOOST_TEST(decoded.find(
            "# TYPE test_seconds_total counter\n"
            "# HELP test_seconds_total help text\n"
            "test_seconds_total 0.56\n"
        ) != std::string_view::npos);
        BOOST_TEST(decoded.find(
            "# TYPE request_duration_seconds histogram\n"
            "request_duration_seconds_bucket{le=\"0.005\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.01\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.025\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.05\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.1\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.25\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.5\"} 0\n"
            "request_duration_seconds_bucket{le=\"1\"} 0\n"
            "request_duration_seconds_bucket{le=\"2.5\"} 0\n"
            "request_duration_seconds_bucket{le=\"5\"} 0\n"
            "request_duration_seconds_bucket{le=\"10\"} 0\n"
            "request_duration_seconds_bucket{le=\"+Inf\"} 0\n"
            "request_duration_seconds_sum 0\n"
            "request_duration_seconds_count 0\n"
        ) != std::string_view::npos);
        BOOST_TEST(decoded.find(
            "# TYPE test_g gauge\n"
            "test_g 900\n"
        ) != std::string_view::npos);
        BOOST_TEST(decoded.find(
            "# TYPE http_requests_total counter\n"
            "http_requests_total{method=\"GET\"} 1\n"
        ) != std::string_view::npos);
        BOOST_TEST(decoded.find(
            "# TYPE some_distribution histogram\n"
            "# HELP some_distribution another hist\n"
            "some_distribution_bucket{le=\"0.005\"} 0\n"
            "some_distribution_bucket{le=\"0.01\"} 0\n"
            "some_distribution_bucket{le=\"0.025\"} 0\n"
            "some_distribution_bucket{le=\"0.05\"} 0\n"
            "some_distribution_bucket{le=\"0.1\"} 0\n"
            "some_distribution_bucket{le=\"0.25\"} 0\n"
            "some_distribution_bucket{le=\"0.5\"} 0\n"
            "some_distribution_bucket{le=\"1\"} 0\n"
            "some_distribution_bucket{le=\"2.5\"} 0\n"
            "some_distribution_bucket{le=\"5\"} 0\n"
            "some_distribution_bucket{le=\"10\"} 0\n"
            "some_distribution_bucket{le=\"+Inf\"} 0\n"
            "some_distribution_sum 0\n"
            "some_distribution_count 0\n"
        ) != std::string_view::npos);
        BOOST_TEST(decoded.size() == 94u + 637u + 31u + 71u + 570u);
    }

    stream.socket().shutdown(ip::tcp::socket::shutdown_both, ec);

    ioc.stop();
    if (ioc_thread.joinable())
        ioc_thread.join();
    BOOST_TEST(ioc.stopped());
}

BOOST_AUTO_TEST_CASE(gzip_encoding_disabled)
{
    io_context ioc;
    ip::tcp::endpoint target_ep(asio::ip::address_v4::loopback(), 8080);
    exposer<decltype(ioc.get_executor()), gzip_policy<false>> exp(ioc.get_executor(), target_ep, "/metrics");

    // Run exposer in other thread
    std::thread ioc_thread([&ioc]() {
        ioc.run();
    });

    beast::tcp_stream stream(ioc);

    beast::error_code ec;
    stream.connect(target_ep, ec);
    if (ec) BOOST_FAIL("Failed to connect: " << ec.message());
    // Empty exposer
    std::string result = make_request(stream, "127.0.0.1:8080", "/metrics", "text/plain;version=0.0.4");
    BOOST_TEST(result.empty());
    // Empty registry
    registry reg;
    exp.expose(reg);
    BOOST_TEST(result.empty());
    // Add one counter
    counter c("test_seconds", {.help = "help text", .unit = "seconds"});
    reg.register_metric(c);
    c.inc(0.56);
    reg.register_metric(c);
    histogram h("request_duration_seconds", {.help = "", .unit = "seconds", .buckets = {}});
    gauge g("test_g");
    counter_vec cv("http_requests", {"method"});
    cv.with_labels({"GET"}).inc();
    g.set(900.0);
    reg.register_metric(h);
    reg.register_metric(g);
    reg.register_metric(cv);
    // Small response (<1024)
    {
        http::request<http::empty_body> req{http::verb::get, "/metrics", 11};
        req.set(http::field::host, "127.0.0.1:8080");
        req.set(http::field::user_agent, "Prometheus/2.51.0");
        req.set(http::field::accept, "text/plain;version=0.0.4");
        req.set(http::field::accept_encoding, "gzip");
        req.keep_alive(true);

        http::write(stream, req, ec);
        if (ec) BOOST_FAIL("Failed to write request: " << ec.message());

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res, ec);
        if (ec) BOOST_FAIL("Failed to read response: " << ec.message());

        BOOST_TEST(res.result() == http::status::ok);
        auto encoding = res.find(http::field::content_encoding);
        BOOST_REQUIRE(encoding == res.end());

        BOOST_TEST(res.body().find(
            "# TYPE test_seconds_total counter\n"
            "# HELP test_seconds_total help text\n"
            "test_seconds_total 0.56\n"
        ) != std::string_view::npos);
        BOOST_TEST(res.body().find(
            "# TYPE request_duration_seconds histogram\n"
            "request_duration_seconds_bucket{le=\"0.005\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.01\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.025\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.05\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.1\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.25\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.5\"} 0\n"
            "request_duration_seconds_bucket{le=\"1\"} 0\n"
            "request_duration_seconds_bucket{le=\"2.5\"} 0\n"
            "request_duration_seconds_bucket{le=\"5\"} 0\n"
            "request_duration_seconds_bucket{le=\"10\"} 0\n"
            "request_duration_seconds_bucket{le=\"+Inf\"} 0\n"
            "request_duration_seconds_sum 0\n"
            "request_duration_seconds_count 0\n"
        ) != std::string_view::npos);
        BOOST_TEST(res.body().find(
            "# TYPE test_g gauge\n"
            "test_g 900\n"
        ) != std::string_view::npos);
        BOOST_TEST(res.body().find(
            "# TYPE http_requests_total counter\n"
            "http_requests_total{method=\"GET\"} 1\n"
        ) != std::string_view::npos);
        BOOST_TEST(res.body().size() == 94u + 637u + 31u + 71u);
    }
    // Add more metrics to force the exposer to compress
    gaugehistogram h2("temperature_dist_celsius", {.help = "another hist", .unit = "", .buckets = {}});
    reg.register_metric(h2);
    // Big response (>1024)
    {
        http::request<http::empty_body> req{http::verb::get, "/metrics", 11};
        req.set(http::field::host, "127.0.0.1:8080");
        req.set(http::field::user_agent, "Prometheus/2.51.0");
        req.set(http::field::accept, "text/plain;version=0.0.4");
        req.set(http::field::accept_encoding, "gzip");
        req.keep_alive(true);

        http::write(stream, req, ec);
        if (ec) BOOST_FAIL("Failed to write request: " << ec.message());

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res, ec);
        if (ec) BOOST_FAIL("Failed to read response: " << ec.message());

        BOOST_TEST(res.result() == http::status::ok);
        auto encoding = res.find(http::field::content_encoding);
        BOOST_REQUIRE(encoding == res.end());

        BOOST_TEST(res.body().find(
            "# TYPE test_seconds_total counter\n"
            "# HELP test_seconds_total help text\n"
            "test_seconds_total 0.56\n"
        ) != std::string_view::npos);
        BOOST_TEST(res.body().find(
            "# TYPE request_duration_seconds histogram\n"
            "request_duration_seconds_bucket{le=\"0.005\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.01\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.025\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.05\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.1\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.25\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.5\"} 0\n"
            "request_duration_seconds_bucket{le=\"1\"} 0\n"
            "request_duration_seconds_bucket{le=\"2.5\"} 0\n"
            "request_duration_seconds_bucket{le=\"5\"} 0\n"
            "request_duration_seconds_bucket{le=\"10\"} 0\n"
            "request_duration_seconds_bucket{le=\"+Inf\"} 0\n"
            "request_duration_seconds_sum 0\n"
            "request_duration_seconds_count 0\n"
        ) != std::string_view::npos);
        BOOST_TEST(res.body().find(
            "# TYPE test_g gauge\n"
            "test_g 900\n"
        ) != std::string_view::npos);
        BOOST_TEST(res.body().find(
            "# TYPE http_requests_total counter\n"
            "http_requests_total{method=\"GET\"} 1\n"
        ) != std::string_view::npos);
        BOOST_TEST(res.body().find(
            "# TYPE temperature_dist_celsius histogram\n"
            "# HELP temperature_dist_celsius another hist\n"
            "temperature_dist_celsius_bucket{le=\"0.005\"} 0\n"
            "temperature_dist_celsius_bucket{le=\"0.01\"} 0\n"
            "temperature_dist_celsius_bucket{le=\"0.025\"} 0\n"
            "temperature_dist_celsius_bucket{le=\"0.05\"} 0\n"
            "temperature_dist_celsius_bucket{le=\"0.1\"} 0\n"
            "temperature_dist_celsius_bucket{le=\"0.25\"} 0\n"
            "temperature_dist_celsius_bucket{le=\"0.5\"} 0\n"
            "temperature_dist_celsius_bucket{le=\"1\"} 0\n"
            "temperature_dist_celsius_bucket{le=\"2.5\"} 0\n"
            "temperature_dist_celsius_bucket{le=\"5\"} 0\n"
            "temperature_dist_celsius_bucket{le=\"10\"} 0\n"
            "temperature_dist_celsius_bucket{le=\"+Inf\"} 0\n"
            "# TYPE temperature_dist_celsius_gsum gauge\n"
            "temperature_dist_celsius_gsum 0\n"
            "# TYPE temperature_dist_celsius_gcount gauge\n"
            "temperature_dist_celsius_gcount 0\n"
        ) != std::string_view::npos);
        BOOST_TEST(res.body().size() == 94u + 637u + 31u + 71u + 772u);
    }

    stream.socket().shutdown(ip::tcp::socket::shutdown_both, ec);

    ioc.stop();
    if (ioc_thread.joinable())
        ioc_thread.join();
    BOOST_TEST(ioc.stopped());
}

BOOST_AUTO_TEST_CASE(ssl_mode)
{
    boost::asio::ssl::context server_ctx{boost::asio::ssl::context::tls_server};
    test::load_server_certificate(server_ctx);

    io_context ioc;
    ip::tcp::endpoint target_ep(asio::ip::address_v4::loopback(), 8080);
    exposer<decltype(ioc.get_executor()), ssl_policy<true>> exp(
        ioc.get_executor(), server_ctx, target_ep, "/metrics");

    // Run exposer in other thread
    std::thread ioc_thread([&ioc]() {
        ioc.run();
    });

    registry reg;
    exp.expose(reg);
    counter c("test_seconds", {.help = "help text", .unit = "seconds"});
    reg.register_metric(c);
    c.inc(0.56);
    histogram h("request_duration_seconds", {.help = "", .unit = "seconds", .buckets = {}});
    gauge g("test_g");
    counter_vec cv("http_requests", {"method"});
    cv.with_labels({"GET"}).inc();
    g.set(900.0);
    reg.register_metric(h);
    reg.register_metric(g);
    reg.register_metric(cv);

    // Without ssl
    {
        beast::tcp_stream stream(ioc);
    
        beast::error_code ec;
        stream.connect(target_ep, ec);
        if (ec) BOOST_FAIL("Failed to connect: " << ec.message());

        http::request<http::empty_body> req{http::verb::get, "/metrics", 11};
        req.set(http::field::host, "127.0.0.1:8080");
        req.set(http::field::user_agent, "Prometheus/2.51.0");
        req.set(http::field::accept, "text/plain;version=0.0.4");
        req.keep_alive(true);

        http::write(stream, req, ec);
        BOOST_CHECK(ec == beast::error_code{});

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res, ec);
        // Server closes insecure connection
        BOOST_CHECK(ec == beast::http::error::end_of_stream);

        BOOST_TEST(res.body().empty());
    
        stream.socket().shutdown(ip::tcp::socket::shutdown_both, ec);
    }
    // With incorrect root certificate
    {
        boost::asio::ssl::context client_ctx{boost::asio::ssl::context::tls_client};
        client_ctx.set_verify_mode(boost::asio::ssl::verify_peer);

        ssl::stream<beast::tcp_stream> stream(ioc, client_ctx);
        beast::error_code ec;

        stream.next_layer().connect(target_ep, ec);
        if (ec) BOOST_FAIL("Failed to connect: " << ec.message());

        stream.handshake(ssl::stream_base::client, ec);
        BOOST_CHECK(ec.category() == boost::asio::error::get_ssl_category());

        stream.shutdown(ec);
    }
    // With correct root certificate
    {
        boost::asio::ssl::context client_ctx{boost::asio::ssl::context::tls_client};
        test::load_client_certificate(client_ctx);

        boost::asio::ssl::stream<beast::tcp_stream> stream(ioc, client_ctx);

        beast::error_code ec;
        stream.next_layer().connect(target_ep, ec);
        if (ec) BOOST_FAIL("Failed to connect: " << ec.message());

        stream.handshake(boost::asio::ssl::stream_base::client, ec);
        if (ec) BOOST_FAIL("Handshake failed: " << ec.message());

        http::request<http::empty_body> req{http::verb::get, "/metrics", 11};
        req.set(http::field::host, "localhost:8080");
        req.set(http::field::user_agent, "Prometheus/2.51.0");
        req.set(http::field::accept, "text/plain;version=0.0.4");
        req.keep_alive(true);

        http::write(stream, req, ec);
        BOOST_CHECK(!ec);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res, ec);
        BOOST_CHECK(!ec);
        BOOST_CHECK_EQUAL(res.result(), http::status::ok);

        BOOST_TEST(res.body().find(
            "# TYPE test_seconds_total counter\n"
            "# HELP test_seconds_total help text\n"
            "test_seconds_total 0.56\n"
        ) != std::string_view::npos);
        BOOST_TEST(res.body().find(
            "# TYPE request_duration_seconds histogram\n"
            "request_duration_seconds_bucket{le=\"0.005\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.01\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.025\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.05\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.1\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.25\"} 0\n"
            "request_duration_seconds_bucket{le=\"0.5\"} 0\n"
            "request_duration_seconds_bucket{le=\"1\"} 0\n"
            "request_duration_seconds_bucket{le=\"2.5\"} 0\n"
            "request_duration_seconds_bucket{le=\"5\"} 0\n"
            "request_duration_seconds_bucket{le=\"10\"} 0\n"
            "request_duration_seconds_bucket{le=\"+Inf\"} 0\n"
            "request_duration_seconds_sum 0\n"
            "request_duration_seconds_count 0\n"
        ) != std::string_view::npos);
        BOOST_TEST(res.body().find(
            "# TYPE test_g gauge\n"
            "test_g 900\n"
        ) != std::string_view::npos);
        BOOST_TEST(res.body().find(
            "# TYPE http_requests_total counter\n"
            "http_requests_total{method=\"GET\"} 1\n"
        ) != std::string_view::npos);
        BOOST_TEST(res.body().size() == 94u + 637u + 31u + 71u);

        stream.shutdown(ec);
        if (ec == boost::asio::ssl::error::stream_truncated) ec = {};
        BOOST_CHECK(!ec);
    }

    ioc.stop();
    if (ioc_thread.joinable())
        ioc_thread.join();
    BOOST_TEST(ioc.stopped());
}

BOOST_AUTO_TEST_CASE(with_static_registry_prometheus_0_0_4)
{
    using static_reg = static_registry<"simple_pull_registry">;
    io_context ioc;
    ip::tcp::endpoint target_ep(asio::ip::address_v4::loopback(), 8080);
    exposer<decltype(ioc.get_executor()), expose_static_registry<static_reg>> exp(
        ioc.get_executor(), target_ep, "/metrics");

    // Run exposer in other thread
    std::thread ioc_thread([&ioc]() {
        ioc.run();
    });

    beast::tcp_stream stream(ioc);

    beast::error_code ec;
    stream.connect(target_ep, ec);
    if (ec) BOOST_FAIL("Failed to connect: " << ec.message());

    std::string result = make_request(stream, "127.0.0.1:8080", "/metrics", "text/plain;version=0.0.4");
    BOOST_TEST(result ==
        "# TYPE active_users_total counter\n"
        "# HELP active_users_total Number of connected users\n"
        "active_users_total 0\n"
        "# TYPE request_duration_seconds histogram\n"
        "request_duration_seconds_bucket{le=\"0.005\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.01\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.025\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.05\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.1\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.25\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.5\"} 0\n"
        "request_duration_seconds_bucket{le=\"1\"} 0\n"
        "request_duration_seconds_bucket{le=\"2.5\"} 0\n"
        "request_duration_seconds_bucket{le=\"5\"} 0\n"
        "request_duration_seconds_bucket{le=\"10\"} 0\n"
        "request_duration_seconds_bucket{le=\"+Inf\"} 0\n"
        "request_duration_seconds_sum 0\n"
        "request_duration_seconds_count 0\n"
        "# TYPE build_info gauge\n"
        "build_info{build_type=\"integration_tests\",version=\"1.0\"} 1\n");
    using build_info = static_info<"build", labels<"version", "build_type">>;
    using c = static_counter<"active_users", help<"Number of connected users">>;
    using h = static_histogram<"request_duration_seconds", unit<"seconds">>;
    static_reg::register_metric<build_info>();
    static_reg::register_metric<c>();
    static_reg::register_metric<h>();
    build_info::create<"1.0", "integration_tests">();
    c::inc(2.0);
    h::observe(0.1);
    result = make_request(stream, "127.0.0.1:8080", "/metrics", "text/plain;version=0.0.4");
    BOOST_TEST(result ==
        "# TYPE active_users_total counter\n"
        "# HELP active_users_total Number of connected users\n"
        "active_users_total 2\n"
        "# TYPE request_duration_seconds histogram\n"
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
        "request_duration_seconds_count 1\n"
        "# TYPE build_info gauge\n"
        "build_info{build_type=\"integration_tests\",version=\"1.0\"} 1\n");
    registry reg;
    exp.expose(reg);
    result = make_request(stream, "127.0.0.1:8080", "/metrics", "text/plain;version=0.0.4");
    BOOST_TEST(result ==
        "# TYPE active_users_total counter\n"
        "# HELP active_users_total Number of connected users\n"
        "active_users_total 2\n"
        "# TYPE request_duration_seconds histogram\n"
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
        "request_duration_seconds_count 1\n"
        "# TYPE build_info gauge\n"
        "build_info{build_type=\"integration_tests\",version=\"1.0\"} 1\n");
    gauge g("test_g");
    counter_vec cv("http_requests", {"method"});
    cv.with_labels({"GET"}).inc();
    g.set(900.0);
    reg.register_metric(g);
    reg.register_metric(cv);
    c::inc(2.0);
    result = make_request(stream, "127.0.0.1:8080", "/metrics", "text/plain;version=0.0.4");
    BOOST_TEST(result.starts_with(
        "# TYPE active_users_total counter\n"
        "# HELP active_users_total Number of connected users\n"
        "active_users_total 4\n"
        "# TYPE request_duration_seconds histogram\n"
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
        "request_duration_seconds_count 1\n"
        "# TYPE build_info gauge\n"
        "build_info{build_type=\"integration_tests\",version=\"1.0\"} 1\n"));
    BOOST_TEST(result.find(
        "# TYPE test_g gauge\n"
        "test_g 900\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.find(
        "# TYPE http_requests_total counter\n"
        "http_requests_total{method=\"GET\"} 1\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.size() == 829u + 31u + 71u);

    stream.socket().shutdown(ip::tcp::socket::shutdown_both, ec);

    ioc.stop();
    if (ioc_thread.joinable())
        ioc_thread.join();
    BOOST_TEST(ioc.stopped());
}

BOOST_AUTO_TEST_CASE(with_static_registry_openmetrics_1_0)
{
    using static_reg = static_registry<"simple_pull_registry_om">;
    io_context ioc;
    ip::tcp::endpoint target_ep(asio::ip::address_v4::loopback(), 8080);
    exposer<decltype(ioc.get_executor()), expose_static_registry<static_reg>> exp(
        ioc.get_executor(), target_ep, "/metrics");

    // Run exposer in other thread
    std::thread ioc_thread([&ioc]() {
        ioc.run();
    });

    beast::tcp_stream stream(ioc);

    beast::error_code ec;
    stream.connect(target_ep, ec);
    if (ec) BOOST_FAIL("Failed to connect: " << ec.message());

    std::string result = make_request(stream, "127.0.0.1:8080", "/metrics", "application/openmetrics-text;version=1.0.0");
    BOOST_TEST(result ==
        "# TYPE active_users_om counter\n"
        "# HELP active_users_om Number of connected users\n"
        "active_users_om_total 0\n"
        "# TYPE request_duration_om_seconds histogram\n"
        "# UNIT request_duration_om_seconds seconds\n"
        "request_duration_om_seconds_bucket{le=\"0.005\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"0.01\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"0.025\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"0.05\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"0.1\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"0.25\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"0.5\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"1\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"2.5\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"5\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"10\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"+Inf\"} 0\n"
        "request_duration_om_seconds_sum 0\n"
        "request_duration_om_seconds_count 0\n"
        "# TYPE build_om info\n"
        "build_om_info{build_type=\"integration_tests\",version=\"1.0\"} 1\n"
        "# EOF\n");
    using build_info = static_info<"build_om", labels<"version", "build_type">>;
    using c = static_counter<"active_users_om", help<"Number of connected users">>;
    using h = static_histogram<"request_duration_om_seconds", unit<"seconds">>;
    static_reg::register_metric<build_info>();
    static_reg::register_metric<c>();
    static_reg::register_metric<h>();
    build_info::create<"1.0", "integration_tests">();
    c::inc(2.0);
    h::observe(0.1);
    result = make_request(stream, "127.0.0.1:8080", "/metrics", "application/openmetrics-text;version=1.0.0");
    BOOST_TEST(result ==
        "# TYPE active_users_om counter\n"
        "# HELP active_users_om Number of connected users\n"
        "active_users_om_total 2\n"
        "# TYPE request_duration_om_seconds histogram\n"
        "# UNIT request_duration_om_seconds seconds\n"
        "request_duration_om_seconds_bucket{le=\"0.005\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"0.01\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"0.025\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"0.05\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"0.1\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"0.25\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"0.5\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"1\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"2.5\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"5\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"10\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"+Inf\"} 1\n"
        "request_duration_om_seconds_sum 0.1\n"
        "request_duration_om_seconds_count 1\n"
        "# TYPE build_om info\n"
        "build_om_info{build_type=\"integration_tests\",version=\"1.0\"} 1\n"
        "# EOF\n");
    registry reg;
    exp.expose(reg);
    result = make_request(stream, "127.0.0.1:8080", "/metrics", "application/openmetrics-text;version=1.0.0");
    BOOST_TEST(result ==
        "# TYPE active_users_om counter\n"
        "# HELP active_users_om Number of connected users\n"
        "active_users_om_total 2\n"
        "# TYPE request_duration_om_seconds histogram\n"
        "# UNIT request_duration_om_seconds seconds\n"
        "request_duration_om_seconds_bucket{le=\"0.005\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"0.01\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"0.025\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"0.05\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"0.1\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"0.25\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"0.5\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"1\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"2.5\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"5\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"10\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"+Inf\"} 1\n"
        "request_duration_om_seconds_sum 0.1\n"
        "request_duration_om_seconds_count 1\n"
        "# TYPE build_om info\n"
        "build_om_info{build_type=\"integration_tests\",version=\"1.0\"} 1\n"
        "# EOF\n");
    gauge g("test_g");
    counter_vec cv("http_requests", {"method"});
    cv.with_labels({"GET"}).inc();
    g.set(900.0);
    reg.register_metric(g);
    reg.register_metric(cv);
    c::inc(2.0);
    result = make_request(stream, "127.0.0.1:8080", "/metrics", "application/openmetrics-text;version=1.0.0");
    BOOST_TEST(result.starts_with(
        "# TYPE active_users_om counter\n"
        "# HELP active_users_om Number of connected users\n"
        "active_users_om_total 4\n"
        "# TYPE request_duration_om_seconds histogram\n"
        "# UNIT request_duration_om_seconds seconds\n"
        "request_duration_om_seconds_bucket{le=\"0.005\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"0.01\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"0.025\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"0.05\"} 0\n"
        "request_duration_om_seconds_bucket{le=\"0.1\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"0.25\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"0.5\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"1\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"2.5\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"5\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"10\"} 1\n"
        "request_duration_om_seconds_bucket{le=\"+Inf\"} 1\n"
        "request_duration_om_seconds_sum 0.1\n"
        "request_duration_om_seconds_count 1\n"
        "# TYPE build_om info\n"
        "build_om_info{build_type=\"integration_tests\",version=\"1.0\"} 1\n"));
    BOOST_TEST(result.find(
        "# TYPE test_g gauge\n"
        "test_g 900\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.find(
        "# TYPE http_requests counter\n"
        "http_requests_total{method=\"GET\"} 1\n"
    ) != std::string_view::npos);
    BOOST_TEST(result.substr(result.size() - 6u, 6u) == std::string_view(
        "# EOF\n"
    ));
    BOOST_TEST(result.size() == 914u + 31u + 65u + 6u);

    stream.socket().shutdown(ip::tcp::socket::shutdown_both, ec);

    ioc.stop();
    if (ioc_thread.joinable())
        ioc_thread.join();
    BOOST_TEST(ioc.stopped());
}

BOOST_AUTO_TEST_SUITE_END()
