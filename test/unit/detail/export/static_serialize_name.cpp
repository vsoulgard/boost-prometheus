//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/detail/export/static_serialize_name.hpp>

#include <boost/prometheus/static_counter.hpp>
#include <boost/prometheus/static_gauge.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

#include <string_view>

using namespace boost::prometheus;
using namespace boost::prometheus::detail;

BOOST_AUTO_TEST_SUITE(test_static_serialize_header)

BOOST_AUTO_TEST_CASE(counter_name)
{
    // Without labels
    using test_counter = static_counter<"http_requests">;
    using serialized_name = static_serialize_name<test_counter>;
    static_assert(serialized_name::value == std::string_view(
        "http_requests_total "));
    
    // With labels
    {
        using test_counter_with_labels = static_counter<"http_requests",
            labels<"first">>;
        using serialized_name_with_labels = static_serialize_name<test_counter_with_labels, "1">;
        static_assert(serialized_name_with_labels::value == std::string_view(
            "http_requests_total{first=\"1\"} "));
    }
    {
        using test_counter_with_labels = static_counter<"http_requests",
            labels<"first", "second">>;
        using serialized_name_with_labels = static_serialize_name<test_counter_with_labels, "1", "2">;
        static_assert(serialized_name_with_labels::value == std::string_view(
            "http_requests_total{first=\"1\",second=\"2\"} "));
    }
    {
        using test_counter_with_labels = static_counter<"http_requests",
            labels<"first", "second", "third">>;
        using serialized_name_with_labels = static_serialize_name<test_counter_with_labels, "1", "2", "3">;
        static_assert(serialized_name_with_labels::value == std::string_view(
            "http_requests_total{first=\"1\",second=\"2\",third=\"3\"} "));
    }
    {
        using test_counter_with_labels = static_counter<"http_requests",
            labels<"first", "second", "third", "fourth">>;
        using serialized_name_with_labels = static_serialize_name<test_counter_with_labels, "1", "2", "3", "4">;
        static_assert(serialized_name_with_labels::value == std::string_view(
            "http_requests_total{first=\"1\",fourth=\"4\",second=\"2\",third=\"3\"} "));
    }
    {
        using test_counter_with_labels = static_counter<"http_requests",
            labels<"first", "second", "third", "fourth", "fifth">>;
        using serialized_name_with_labels = static_serialize_name<test_counter_with_labels, "1", "2", "3", "4", "5">;
        static_assert(serialized_name_with_labels::value == std::string_view(
            "http_requests_total{fifth=\"5\",first=\"1\",fourth=\"4\",second=\"2\",third=\"3\"} "));
    }
}

BOOST_AUTO_TEST_CASE(gauge_name)
{
    // Without labels
    using test_gauge= static_gauge<"test_g">;
    using serialized_name = static_serialize_name<test_gauge>;
    static_assert(serialized_name::value == std::string_view(
        "test_g "));
    
    // With labels
    {
        using test_gauge_with_labels = static_gauge<"test_g",
            labels<"first">>;
        using serialized_name_with_labels = static_serialize_name<test_gauge_with_labels, "1">;
        static_assert(serialized_name_with_labels::value == std::string_view(
            "test_g{first=\"1\"} "));
    }
    {
        using test_gauge_with_labels = static_gauge<"test_g",
            labels<"first", "second">>;
        using serialized_name_with_labels = static_serialize_name<test_gauge_with_labels, "1", "2">;
        static_assert(serialized_name_with_labels::value == std::string_view(
            "test_g{first=\"1\",second=\"2\"} "));
    }
    {
        using test_gauge_with_labels = static_gauge<"test_g",
            labels<"first", "second", "third">>;
        using serialized_name_with_labels = static_serialize_name<test_gauge_with_labels, "1", "2", "3">;
        static_assert(serialized_name_with_labels::value == std::string_view(
            "test_g{first=\"1\",second=\"2\",third=\"3\"} "));
    }
    {
        using test_gauge_with_labels = static_gauge<"test_g",
            labels<"first", "second", "third", "fourth">>;
        using serialized_name_with_labels = static_serialize_name<test_gauge_with_labels, "1", "2", "3", "4">;
        static_assert(serialized_name_with_labels::value == std::string_view(
            "test_g{first=\"1\",fourth=\"4\",second=\"2\",third=\"3\"} "));
    }
    {
        using test_gauge_with_labels = static_gauge<"test_g",
            labels<"first", "second", "third", "fourth", "fifth">>;
        using serialized_name_with_labels = static_serialize_name<test_gauge_with_labels, "1", "2", "3", "4", "5">;
        static_assert(serialized_name_with_labels::value == std::string_view(
            "test_g{fifth=\"5\",first=\"1\",fourth=\"4\",second=\"2\",third=\"3\"} "));
    }
}

BOOST_AUTO_TEST_SUITE_END()
