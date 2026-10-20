//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/counter.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_counter)

BOOST_AUTO_TEST_CASE(default_constructor_is_deleted)
{
    static_assert(!std::is_default_constructible_v<counter<>>);
    static_assert(!std::is_default_constructible_v<counter<atomic_policy<false>>>);
}

BOOST_AUTO_TEST_CASE(construction_default_options)
{
    counter c("http_requests");
    BOOST_TEST(c.get_value() == 0.0);
}

BOOST_AUTO_TEST_CASE(construction_custom_options)
{
    counter c("request_duration_seconds", {.help = "help string", .unit = "seconds"});
    BOOST_TEST(c.get_value() == 0.0);
}

BOOST_AUTO_TEST_CASE(inc_default, * boost::unit_test::tolerance(1e-9))
{
    counter c("http_requests");
    c.inc();
    BOOST_TEST(c.get_value() == 1.0);
    c.inc();
    BOOST_TEST(c.get_value() == 2.0);
    c.inc();
    BOOST_TEST(c.get_value() == 3.0);
    c.inc();
    BOOST_TEST(c.get_value() == 4.0);
    c.inc();
    BOOST_TEST(c.get_value() == 5.0);
}

BOOST_AUTO_TEST_CASE(inc_custom_value, * boost::unit_test::tolerance(1e-9))
{
    counter c("http_requests");
    c.inc(5.5);
    BOOST_TEST(c.get_value() == 5.5);
    c.inc(2.5);
    BOOST_TEST(c.get_value() == 8.0);
    c.inc(13.2);
    BOOST_TEST(c.get_value() == 21.2);
}

BOOST_AUTO_TEST_CASE(non_atomic_construction)
{
    counter<atomic_policy<false>> c("non_atomic_counter_bytes", {"help text", "bytes"});
    BOOST_TEST(c.get_value() == 0.0);
    BOOST_TEST(c.is_atomic() == false);
}

BOOST_AUTO_TEST_CASE(non_atomic_inc, * boost::unit_test::tolerance(1e-9))
{
    counter<atomic_policy<false>> c("non_atomic_counter");
    c.inc(10.0);
    BOOST_TEST(c.get_value() == 10.0);
    c.inc();
    BOOST_TEST(c.get_value() == 11.0);
    c.inc(0.5);
    BOOST_TEST(c.get_value() == 11.5);
    c.inc(4.5);
    BOOST_TEST(c.get_value() == 16.0);
}

BOOST_AUTO_TEST_SUITE_END()
