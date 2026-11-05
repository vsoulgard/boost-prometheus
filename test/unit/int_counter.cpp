//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/int_counter.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_int_counter)

BOOST_AUTO_TEST_CASE(default_constructor_is_deleted)
{
    static_assert(!std::is_default_constructible_v<int_counter<atomic_policy<true>>>);
    static_assert(!std::is_default_constructible_v<int_counter<atomic_policy<false>>>);
}

BOOST_AUTO_TEST_CASE(construction_default_options)
{
    int_counter c("http_requests");
    BOOST_TEST(c.get_value() == 0u);
}

BOOST_AUTO_TEST_CASE(construction_custom_options)
{
    int_counter c("request_duration_seconds", {.help = "help string", .unit = "seconds"});
    BOOST_TEST(c.get_value() == 0u);
}

BOOST_AUTO_TEST_CASE(inc_default)
{
    int_counter c("http_requests");
    c.inc();
    BOOST_TEST(c.get_value() == 1u);
    c.inc();
    BOOST_TEST(c.get_value() == 2u);
    c.inc();
    BOOST_TEST(c.get_value() == 3u);
    c.inc();
    BOOST_TEST(c.get_value() == 4u);
    c.inc();
    BOOST_TEST(c.get_value() == 5u);
}

BOOST_AUTO_TEST_CASE(inc_custom_value)
{
    int_counter c("http_requests");
    c.inc(5u);
    BOOST_TEST(c.get_value() == 5u);
    c.inc(2u);
    BOOST_TEST(c.get_value() == 7u);
    c.inc(13u);
    BOOST_TEST(c.get_value() == 20u);
}

BOOST_AUTO_TEST_CASE(non_atomic_construction)
{
    int_counter<atomic_policy<false>> c("non_atomic_c_bytes", {"help text", "bytes"});
    BOOST_TEST(c.get_value() == 0u);
    BOOST_TEST(c.is_atomic() == false);
}

BOOST_AUTO_TEST_CASE(non_atomic_inc, * boost::unit_test::tolerance(1e-9))
{
    int_counter<atomic_policy<false>> c("non_atomic_c");
    c.inc(10u);
    BOOST_TEST(c.get_value() == 10u);
    c.inc();
    BOOST_TEST(c.get_value() == 11u);
    c.inc(5u);
    BOOST_TEST(c.get_value() == 16u);
    c.inc(4u);
    BOOST_TEST(c.get_value() == 20u);
}

BOOST_AUTO_TEST_SUITE_END()
