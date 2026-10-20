//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/detail/export/serialize_name_with_labels.hpp>

#include <vector>
#include <span>
#include <utility>

#include <boost/test/unit_test.hpp>

using namespace boost::prometheus;
using namespace boost::prometheus::detail;

BOOST_AUTO_TEST_SUITE(test_serialize_name_with_labels)

BOOST_AUTO_TEST_CASE(basic)
{
    {
        std::vector<std::pair<std::string, std::size_t>> labels{
            {"first", 1}, {"second", 0}
        };
        std::vector<std::string_view> labelvalues{
            "2", "1"
        };
        std::string result = serialize_name_with_labels(
            "free_bytes",
            labels,
            labelvalues
        );
        BOOST_TEST(result == std::string_view("free_bytes{first=\"1\",second=\"2\"} "));
    }
}

BOOST_AUTO_TEST_CASE(with_suffix)
{
    {
        std::vector<std::pair<std::string, std::size_t>> labels{
            {"first", 0}
        };
        std::vector<std::string_view> labelvalues{
            "1"
        };
        std::string result = serialize_name_with_labels<"_info">(
            "build",
            labels,
            labelvalues
        );
        BOOST_TEST(result == std::string_view("build_info{first=\"1\"} "));
    }
    {
        std::vector<std::pair<std::string, std::size_t>> labels{
            {"first", 0}
        };
        std::vector<std::string_view> labelvalues{
            "1"
        };
        std::string result = serialize_name_with_labels<"_total">(
            "test_counter",
            labels,
            labelvalues
        );
        BOOST_TEST(result == std::string_view("test_counter_total{first=\"1\"} "));
    }
    {
        std::vector<std::pair<std::string, std::size_t>> labels{
            {"first", 1}, {"second", 0}
        };
        std::vector<std::string_view> labelvalues{
            "2", "1"
        };
        std::string result = serialize_name_with_labels<"_total">(
            "test_counter",
            labels,
            labelvalues
        );
        BOOST_TEST(result == std::string_view("test_counter_total{first=\"1\",second=\"2\"} "));
    }
    {
        std::vector<std::pair<std::string, std::size_t>> labels{
            {"first", 1}, {"second", 0}, {"third", 2}
        };
        std::vector<std::string_view> labelvalues{
            "2", "1", "3"
        };
        std::string result = serialize_name_with_labels<"_total">(
            "test_counter",
            labels,
            labelvalues
        );
        BOOST_TEST(result == std::string_view("test_counter_total{first=\"1\",second=\"2\",third=\"3\"} "));
    }
    {
        std::vector<std::pair<std::string, std::size_t>> labels{
            {"first", 1}, {"fourth", 2}, {"second", 0}, {"third", 3}
        };
        std::vector<std::string_view> labelvalues{
            "2", "1", "4", "3"
        };
        std::string result = serialize_name_with_labels<"_total">(
            "test_counter",
            labels,
            labelvalues
        );
        BOOST_TEST(result == std::string_view("test_counter_total{first=\"1\",fourth=\"4\",second=\"2\",third=\"3\"} "));
    }
    {
        std::vector<std::pair<std::string, std::size_t>> labels{
            {"fifth", 4},{"first", 1}, {"fourth", 2}, {"second", 0}, {"third", 3}
        };
        std::vector<std::string_view> labelvalues{
            "2", "1", "4", "3", "5"
        };
        std::string result = serialize_name_with_labels<"_total">(
            "test_counter",
            labels,
            labelvalues
        );
        BOOST_TEST(result == std::string_view("test_counter_total{fifth=\"5\",first=\"1\",fourth=\"4\",second=\"2\",third=\"3\"} "));
    }
}

BOOST_AUTO_TEST_SUITE_END()
