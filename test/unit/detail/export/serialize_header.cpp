//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/detail/export/serialize_header.hpp>

#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::prometheus;
using namespace boost::prometheus::detail;

BOOST_AUTO_TEST_SUITE(test_serialize_header)

BOOST_AUTO_TEST_CASE(basic)
{
    {
        auto [base_string, om_header_offset] = serialize_header<"gauge", "gauge">(
            "test_g", metric_opts{});
        BOOST_TEST(base_string == std::string_view(
            "# TYPE test_g gauge\n"));
        BOOST_TEST(om_header_offset == 0);
    }
    // Counter (special case)
    {
        auto [base_string, om_header_offset] = serialize_header<"counter", "counter">(
            "http_requests", metric_opts{});
        std::string_view prom_header = std::string_view(base_string).substr(
            0,
            om_header_offset
        );
        std::string_view om_header = std::string_view(base_string).substr(
            om_header_offset,
            base_string.size() - om_header_offset
        );
        BOOST_TEST(prom_header == std::string_view(
            "# TYPE http_requests_total counter\n"));
        BOOST_TEST(om_header == std::string_view(
            "# TYPE http_requests counter\n"));
    }
}

BOOST_AUTO_TEST_CASE(basic_with_params)
{
    {
        auto [base_string, om_header_offset] = serialize_header<"gauge", "gauge">(
            "test_g_seconds",
            metric_opts{.help = "help text", .unit = "seconds"});
        std::string_view prom_header = std::string_view(base_string).substr(
            0,
            om_header_offset
        );
        std::string_view om_header = std::string_view(base_string).substr(
            om_header_offset,
            base_string.size() - om_header_offset
        );
        BOOST_TEST(prom_header == std::string_view(
            "# TYPE test_g_seconds gauge\n"
            "# HELP test_g_seconds help text\n"));
        BOOST_TEST(om_header == std::string_view(
            "# TYPE test_g_seconds gauge\n"
            "# UNIT test_g_seconds seconds\n"
            "# HELP test_g_seconds help text\n"));
    }
    // Counter (special case)
    {
        auto [base_string, om_header_offset] = serialize_header<"counter", "counter">(
            "duration_seconds",
            metric_opts{.help = "help text", .unit = "seconds"});
        std::string_view prom_header = std::string_view(base_string).substr(
            0,
            om_header_offset
        );
        std::string_view om_header = std::string_view(base_string).substr(
            om_header_offset,
            base_string.size() - om_header_offset
        );
        BOOST_TEST(prom_header == std::string_view(
            "# TYPE duration_seconds_total counter\n"
            "# HELP duration_seconds_total help text\n"));
        BOOST_TEST(om_header == std::string_view(
            "# TYPE duration_seconds counter\n"
            "# UNIT duration_seconds seconds\n"
            "# HELP duration_seconds help text\n"));
    }
}

BOOST_AUTO_TEST_CASE(different_types)
{
    // Info (special case)
    auto [base_string, om_header_offset] = serialize_header<"gauge", "info">("foo", metric_opts{});
    std::string_view prom_header = std::string_view(base_string).substr(
        0,
        om_header_offset
    );
    std::string_view om_header = std::string_view(base_string).substr(
        om_header_offset,
        base_string.size() - om_header_offset
    );
    BOOST_TEST(prom_header == std::string_view(
        "# TYPE foo_info gauge\n"));
    BOOST_TEST(om_header == std::string_view(
        "# TYPE foo info\n"));
}

BOOST_AUTO_TEST_CASE(different_types_with_params)
{
    auto [base_string, om_header_offset] = serialize_header<"histogram", "gaugehistogram">(
        "file_size_bytes",
        metric_opts{.help = "help text", .unit = "bytes"});
    std::string_view prom_header = std::string_view(base_string).substr(
        0,
        om_header_offset
    );
    std::string_view om_header = std::string_view(base_string).substr(
        om_header_offset,
        base_string.size() - om_header_offset
    );
    BOOST_TEST(prom_header == std::string_view(
        "# TYPE file_size_bytes histogram\n"
        "# HELP file_size_bytes help text\n"));
    BOOST_TEST(om_header == std::string_view(
        "# TYPE file_size_bytes gaugehistogram\n"
        "# UNIT file_size_bytes bytes\n"
        "# HELP file_size_bytes help text\n"));
}

BOOST_AUTO_TEST_SUITE_END()
