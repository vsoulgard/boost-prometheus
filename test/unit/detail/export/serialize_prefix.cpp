//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/detail/export/serialize_prefix.hpp>

#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::prometheus;
using namespace boost::prometheus::detail;

BOOST_AUTO_TEST_SUITE(test_serialize_prefix)

BOOST_AUTO_TEST_CASE(basic)
{
    {
        auto [base_string, om_prefix_offset] = serialize_prefix<"gauge", "gauge">(
            "free_bytes", metric_opts{});
        BOOST_TEST(base_string == std::string_view(
            "# TYPE free_bytes gauge\n"
            "free_bytes "));
        BOOST_TEST(om_prefix_offset == 0);
    }
    // Counter (special case)
    {
        auto [base_string, om_prefix_offset] = serialize_prefix<"counter", "counter">(
            "http_requests", metric_opts{});
        std::string_view prom_prefix = std::string_view(base_string).substr(
            0,
            om_prefix_offset
        );
        std::string_view om_prefix = std::string_view(base_string).substr(
            om_prefix_offset,
            base_string.size() - om_prefix_offset
        );
        BOOST_TEST(prom_prefix == std::string_view(
            "# TYPE http_requests_total counter\n"
            "http_requests_total "));
        BOOST_TEST(om_prefix == std::string_view(
            "# TYPE http_requests counter\n"
            "http_requests_total "));
    }
}

BOOST_AUTO_TEST_CASE(basic_with_params)
{
    {
        auto [base_string, om_prefix_offset] = serialize_prefix<"gauge", "gauge">(
            "free_bytes",
            metric_opts{.help = "help text", .unit = "bytes"});
        std::string_view prom_prefix = std::string_view(base_string).substr(
            0,
            om_prefix_offset
        );
        std::string_view om_prefix = std::string_view(base_string).substr(
            om_prefix_offset,
            base_string.size() - om_prefix_offset
        );
        BOOST_TEST(prom_prefix == std::string_view(
            "# TYPE free_bytes gauge\n"
            "# HELP free_bytes help text\n"
            "free_bytes "));
        BOOST_TEST(om_prefix == std::string_view(
            "# TYPE free_bytes gauge\n"
            "# UNIT free_bytes bytes\n"
            "# HELP free_bytes help text\n"
            "free_bytes "));
    }
    // Counter (special case)
    {
        auto [base_string, om_prefix_offset] = serialize_prefix<"counter", "counter">(
            "duration_seconds",
            metric_opts{.help = "help text", .unit = "seconds"});
        std::string_view prom_prefix = std::string_view(base_string).substr(
            0,
            om_prefix_offset
        );
        std::string_view om_prefix = std::string_view(base_string).substr(
            om_prefix_offset,
            base_string.size() - om_prefix_offset
        );
        BOOST_TEST(prom_prefix == std::string_view(
            "# TYPE duration_seconds_total counter\n"
            "# HELP duration_seconds_total help text\n"
            "duration_seconds_total "));
        BOOST_TEST(om_prefix == std::string_view(
            "# TYPE duration_seconds counter\n"
            "# UNIT duration_seconds seconds\n"
            "# HELP duration_seconds help text\n"
            "duration_seconds_total "));
    }
}

BOOST_AUTO_TEST_CASE(different_types)
{
    // Info (special case)
    auto [base_string, om_prefix_offset] = serialize_prefix<"gauge", "info">(
        "foo", metric_opts{});
    std::string_view prom_prefix = std::string_view(base_string).substr(
        0,
        om_prefix_offset
    );
    std::string_view om_prefix = std::string_view(base_string).substr(
        om_prefix_offset,
        base_string.size() - om_prefix_offset
    );
    BOOST_TEST(prom_prefix == std::string_view(
        "# TYPE foo_info gauge\n"
        "foo_info "));
    BOOST_TEST(om_prefix == std::string_view(
        "# TYPE foo info\n"
        "foo_info "));
}

BOOST_AUTO_TEST_CASE(different_types_with_params)
{
    auto [base_string, om_prefix_offset] = serialize_prefix<"histogram", "gaugehistogram">(
        "file_size_bytes",
        metric_opts{.help = "help text", .unit = "bytes"});
    std::string_view prom_prefix = std::string_view(base_string).substr(
        0,
        om_prefix_offset
    );
    std::string_view om_prefix = std::string_view(base_string).substr(
        om_prefix_offset,
        base_string.size() - om_prefix_offset
    );
    BOOST_TEST(prom_prefix == std::string_view(
        "# TYPE file_size_bytes histogram\n"
        "# HELP file_size_bytes help text\n"
        "file_size_bytes "));
    BOOST_TEST(om_prefix == std::string_view(
        "# TYPE file_size_bytes gaugehistogram\n"
        "# UNIT file_size_bytes bytes\n"
        "# HELP file_size_bytes help text\n"
        "file_size_bytes "));
}

BOOST_AUTO_TEST_SUITE_END()
