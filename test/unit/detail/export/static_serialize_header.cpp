//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/detail/export/static_serialize_header.hpp>

#include <boost/prometheus/static_counter.hpp>
#include <boost/prometheus/static_histogram.hpp>
#include <boost/prometheus/static_gauge.hpp>
#include <boost/prometheus/static_info.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

#include <string_view>

using namespace boost::prometheus;
using namespace boost::prometheus::detail;

BOOST_AUTO_TEST_SUITE(test_static_serialize_header)

BOOST_AUTO_TEST_CASE(counter_header)
{
    using test_counter = static_counter<"http_requests">;
    using serialized_header = static_serialize_header<test_counter, "http_requests">;
    static_assert(serialized_header::prometheus_format == std::string_view(
        "# TYPE http_requests_total counter\n"));
    static_assert(serialized_header::openmetrics_format == std::string_view(
        "# TYPE http_requests counter\n"));

    using test_counter_with_help = static_counter<"http_requests", help<"help text">>;
    using serialized_header_help = static_serialize_header<test_counter_with_help, "http_requests", help<"help text">>;
    static_assert(serialized_header_help::prometheus_format == std::string_view(
        "# TYPE http_requests_total counter\n"
        "# HELP http_requests_total help text\n"));
    static_assert(serialized_header_help::openmetrics_format == std::string_view(
        "# TYPE http_requests counter\n"
        "# HELP http_requests help text\n"));

    using test_counter_with_unit = static_counter<"boot_seconds", help<"help text">, unit<"seconds">>;
    using serialized_header_unit = static_serialize_header<test_counter_with_unit, "boot_seconds", help<"help text">, unit<"seconds">>;
    static_assert(serialized_header_unit::prometheus_format == std::string_view(
        "# TYPE boot_seconds_total counter\n"
        "# HELP boot_seconds_total help text\n"));
    static_assert(serialized_header_unit::openmetrics_format == std::string_view(
        "# TYPE boot_seconds counter\n"
        "# UNIT boot_seconds seconds\n"
        "# HELP boot_seconds help text\n"));
}

BOOST_AUTO_TEST_CASE(gauge_header)
{
    using test_gauge = static_gauge<"test_g">;
    using serialized_header = static_serialize_header<test_gauge, "test_g">;
    static_assert(serialized_header::prometheus_format == std::string_view(
        "# TYPE test_g gauge\n"));
    static_assert(serialized_header::openmetrics_format == std::string_view(
        "# TYPE test_g gauge\n"));

    using test_gauge_with_help = static_gauge<"test_g", help<"help text">>;
    using serialized_header_help = static_serialize_header<test_gauge_with_help, "test_g", help<"help text">>;
    static_assert(serialized_header_help::prometheus_format == std::string_view(
        "# TYPE test_g gauge\n"
        "# HELP test_g help text\n"));
    static_assert(serialized_header_help::openmetrics_format == std::string_view(
        "# TYPE test_g gauge\n"
        "# HELP test_g help text\n"));

    using test_gauge_with_unit = static_gauge<"test_g_seconds", help<"help text">, unit<"seconds">>;
    using serialized_header_unit = static_serialize_header<test_gauge_with_unit, "test_g_seconds", help<"help text">, unit<"seconds">>;
    static_assert(serialized_header_unit::prometheus_format == std::string_view(
        "# TYPE test_g_seconds gauge\n"
        "# HELP test_g_seconds help text\n"));
    static_assert(serialized_header_unit::openmetrics_format == std::string_view(
        "# TYPE test_g_seconds gauge\n"
        "# UNIT test_g_seconds seconds\n"
        "# HELP test_g_seconds help text\n"));
}

BOOST_AUTO_TEST_CASE(histogram_header)
{
    using test_histogram = static_histogram<"request_duration">;
    using serialized_header = static_serialize_header<test_histogram, "request_duration">;
    static_assert(serialized_header::prometheus_format == std::string_view(
        "# TYPE request_duration histogram\n"));
    static_assert(serialized_header::openmetrics_format == std::string_view(
        "# TYPE request_duration histogram\n"));

    using test_histogram_with_help = static_histogram<"request_duration", help<"help text">>;
    using serialized_header_help = static_serialize_header<test_histogram_with_help, "request_duration", help<"help text">>;
    static_assert(serialized_header_help::prometheus_format == std::string_view(
        "# TYPE request_duration histogram\n"
        "# HELP request_duration help text\n"));
    static_assert(serialized_header_help::openmetrics_format == std::string_view(
        "# TYPE request_duration histogram\n"
        "# HELP request_duration help text\n"));

    using test_histogram_with_unit = static_histogram<"request_duration_seconds", help<"help text">, unit<"seconds">>;
    using serialized_header_unit = static_serialize_header<test_histogram_with_unit, "request_duration_seconds", help<"help text">, unit<"seconds">>;
    static_assert(serialized_header_unit::prometheus_format == std::string_view(
        "# TYPE request_duration_seconds histogram\n"
        "# HELP request_duration_seconds help text\n"));
    static_assert(serialized_header_unit::openmetrics_format == std::string_view(
        "# TYPE request_duration_seconds histogram\n"
        "# UNIT request_duration_seconds seconds\n"
        "# HELP request_duration_seconds help text\n"));
}

BOOST_AUTO_TEST_CASE(info_header)
{
    using test_info = static_info<"build">;
    using serialized_header = static_serialize_header<test_info, "build">;
    static_assert(serialized_header::prometheus_format == std::string_view(
        "# TYPE build_info gauge\n"));
    static_assert(serialized_header::openmetrics_format == std::string_view(
        "# TYPE build info\n"));

    using test_info_with_help = static_info<"build", help<"help text">>;
    using serialized_header_help = static_serialize_header<test_info_with_help, "build", help<"help text">>;
    static_assert(serialized_header_help::prometheus_format == std::string_view(
        "# TYPE build_info gauge\n"
        "# HELP build_info help text\n"));
    static_assert(serialized_header_help::openmetrics_format == std::string_view(
        "# TYPE build info\n"
        "# HELP build help text\n"));
}

BOOST_AUTO_TEST_SUITE_END()
