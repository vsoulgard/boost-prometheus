//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/counter_vec.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

#include <array>
#include <vector>

using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_counter_vec)

BOOST_AUTO_TEST_CASE(default_constructor_is_deleted)
{
    static_assert(!std::is_default_constructible_v<counter_vec<>>);
    static_assert(!std::is_default_constructible_v<counter_vec<atomic_policy<false>>>);
}

BOOST_AUTO_TEST_CASE(construction_default_options)
{
    counter_vec c("http_requests", {"first", "second"});
}

BOOST_AUTO_TEST_CASE(construction_custom_options)
{
    counter_vec c("request_duration_seconds", {.help = "help string", .unit = "seconds"}, {"first", "second"});
}

BOOST_AUTO_TEST_CASE(with_labels_equal)
{
    {
        counter_vec c("http_requests", {"first"});
        auto& counter1 = c.with_labels({"1"});
        auto& counter2 = c.with_labels({"1"});
        BOOST_TEST(&counter1 == &counter2);
        auto& counter3 = c.with_labels({"3"});
        BOOST_TEST(&counter3 != &counter2);
        auto& counter4 = c.with_labels({"5"});
        BOOST_TEST(&counter3 != &counter4);
        // Test with containers
        std::vector<std::string> vec = {"1"};
        auto& counter5 = c.with_labels(vec);
        BOOST_TEST(&counter5 == &counter2);
        auto& counter6 = c.with_labels(std::move(vec));
        BOOST_TEST(&counter6 == &counter2);
        std::array<std::string_view, 1> arr = {"1"};
        auto& counter7 = c.with_labels(arr);
        BOOST_TEST(&counter7 == &counter2);
    }
    {
        counter_vec c("http_requests", {"first", "second"});
        auto& counter1 = c.with_labels({"1", "2"});
        auto& counter2 = c.with_labels({"1", "2"});
        BOOST_TEST(&counter1 == &counter2);
        auto& counter3 = c.with_labels({"3", "4"});
        BOOST_TEST(&counter3 != &counter2);
        auto& counter4 = c.with_labels({"5", "6"});
        BOOST_TEST(&counter3 != &counter4);
        // Test with containers
        std::vector<std::string> vec = {"1", "2"};
        auto& counter5 = c.with_labels(vec);
        BOOST_TEST(&counter5 == &counter2);
        auto& counter6 = c.with_labels(std::move(vec));
        BOOST_TEST(&counter6 == &counter2);
        std::array<std::string_view, 2> arr = {"1", "2"};
        auto& counter7 = c.with_labels(arr);
        BOOST_TEST(&counter7 == &counter2);
    }
    {
        counter_vec c("http_requests", {"first", "second", "third"});
        auto& counter1 = c.with_labels({"1", "2", "3"});
        auto& counter2 = c.with_labels({"1", "2", "3"});
        BOOST_TEST(&counter1 == &counter2);
        auto& counter3 = c.with_labels({"3", "4", "5"});
        BOOST_TEST(&counter3 != &counter2);
        auto& counter4 = c.with_labels({"5", "6", "7"});
        BOOST_TEST(&counter3 != &counter4);
        // Test with containers
        std::vector<std::string> vec = {"1", "2", "3"};
        auto& counter5 = c.with_labels(vec);
        BOOST_TEST(&counter5 == &counter2);
        auto& counter6 = c.with_labels(std::move(vec));
        BOOST_TEST(&counter6 == &counter2);
        std::array<std::string_view, 3> arr = {"1", "2", "3"};
        auto& counter7 = c.with_labels(arr);
        BOOST_TEST(&counter7 == &counter2);
    }
}

BOOST_AUTO_TEST_CASE(with_labels_inc, * boost::unit_test::tolerance(1e-9))
{
    counter_vec c("http_requests", {"first", "second"});
    auto& counter1 = c.with_labels({"1", "2"});
    auto& counter2 = c.with_labels({"1", "2"});
    counter1.inc();
    counter2.inc(0.5);
    BOOST_TEST(counter1.get_value() == 1.5);
    auto& counter3 = c.with_labels({"3", "4"});
    counter3.inc();
    BOOST_TEST(counter3.get_value() == 1.0);
    auto& counter4 = c.with_labels({"5", "6"});
    counter4.inc();
    BOOST_TEST(counter3.get_value() == 1.0);
    // Test with containers
    std::vector<std::string> vec = {"1", "2"};
    auto& counter5 = c.with_labels(vec);
    counter5.inc();
    BOOST_TEST(counter5.get_value() == 2.5);
    auto& counter6 = c.with_labels(std::move(vec));
    counter6.inc();
    BOOST_TEST(counter6.get_value() == 3.5);
    std::array<std::string_view, 2> arr = {"1", "2"};
    auto& counter7 = c.with_labels(arr);
    counter7.inc();
    BOOST_TEST(counter7.get_value() == 4.5);
}

BOOST_AUTO_TEST_CASE(with_labels_rvalue)
{
    // Construct by lvalue
    {
        // std::vector
        std::vector<std::string> vec = {"1", "2"};
        counter_vec c("http_requests", {"first", "second"});
        c.with_labels(vec);
        BOOST_TEST(vec == std::vector<std::string>({"1", "2"}));
        c.with_labels(vec);
        BOOST_TEST(vec == std::vector<std::string>({"1", "2"}));
        c.with_labels(std::move(vec));
        // If exists, vec isn't moved (single thread only).
        BOOST_TEST(vec == std::vector<std::string>({"1", "2"}));

        // basic_metric_vec::key_type
        counter_vec<>::key_type key = {"3", "4"};
        c.with_labels(key);
        BOOST_TEST(key == counter_vec<>::key_type({"3", "4"}));
        c.with_labels(key);
        BOOST_TEST(key == counter_vec<>::key_type({"3", "4"}));
        c.with_labels(std::move(key));
        BOOST_TEST(key == counter_vec<>::key_type({"3", "4"}));
    }
    // Construct by rvalue
    {
        // std::vector
        std::vector<std::string> vec1 = {"1", "2"};
        counter_vec c("http_requests", {"first", "second"});
        c.with_labels(std::move(vec1));
        // All elements are moved.
        BOOST_TEST(vec1 == std::vector<std::string>(2));
        std::vector<std::string> vec2 = {"1", "2"};
        c.with_labels(std::move(vec2));
        // If exists, vec isn't moved (single thread only).
        BOOST_TEST(vec2 == std::vector<std::string>({"1", "2"}));

        // basic_metric_vec::key_type
        counter_vec<>::key_type key1 = {"3", "4"};
        c.with_labels(std::move(key1));
        // Whole key is moved.
        BOOST_TEST(key1.empty());
        counter_vec<>::key_type key2 = {"3", "4"};
        c.with_labels(std::move(key2));
        BOOST_TEST(key2 == counter_vec<>::key_type({"3", "4"}));
    }
}

BOOST_AUTO_TEST_SUITE_END()
