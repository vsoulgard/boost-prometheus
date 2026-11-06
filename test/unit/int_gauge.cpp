//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/int_gauge.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_int_gauge)

BOOST_AUTO_TEST_CASE(default_constructor_is_deleted)
{
    static_assert(!std::is_default_constructible_v<int_gauge<atomic_policy<true>>>);
    static_assert(!std::is_default_constructible_v<int_gauge<atomic_policy<false>>>);
}

BOOST_AUTO_TEST_CASE(construction_default_options)
{
    int_gauge g("test_int_g");
    BOOST_TEST(g.get_value() == 0);
}

BOOST_AUTO_TEST_CASE(construction_custom_options)
{
    int_gauge g("test_int_g_seconds", {.help = "help string", .unit = "seconds"});
    BOOST_TEST(g.get_value() == 0);
}

BOOST_AUTO_TEST_CASE(inc_dec_default)
{
    int_gauge g("test_int_g");
    g.inc();
    BOOST_TEST(g.get_value() == 1);
    g.inc();
    BOOST_TEST(g.get_value() == 2);
    g.inc();
    BOOST_TEST(g.get_value() == 3);
    g.inc();
    BOOST_TEST(g.get_value() == 4);
    g.inc();
    BOOST_TEST(g.get_value() == 5);
    g.dec();
    BOOST_TEST(g.get_value() == 4);
    g.dec();
    BOOST_TEST(g.get_value() == 3);
    g.dec();
    BOOST_TEST(g.get_value() == 2);
    g.dec();
    BOOST_TEST(g.get_value() == 1);
    g.dec();
    BOOST_TEST(g.get_value() == 0);
    g.dec();
    BOOST_TEST(g.get_value() == -1);
}

BOOST_AUTO_TEST_CASE(set)
{
    int_gauge g("test_int_g");
    g.set(100);
    BOOST_TEST(g.get_value() == 100);
    g.set(-100);
    BOOST_TEST(g.get_value() == -100);
    g.set(123);
    BOOST_TEST(g.get_value() == 123);
    g.set(999);
    BOOST_TEST(g.get_value() == 999);
    g.set(-10);
    BOOST_TEST(g.get_value() == -10);
}

BOOST_AUTO_TEST_CASE(inc_dec_custom_value)
{
    int_gauge g("test_int_g");
    g.inc(5);
    BOOST_TEST(g.get_value() == 5);
    g.inc(3);
    BOOST_TEST(g.get_value() == 8);
    g.inc(13);
    BOOST_TEST(g.get_value() == 21);
    g.dec(1);
    BOOST_TEST(g.get_value() == 20);
    g.dec(7);
    BOOST_TEST(g.get_value() == 13);
    g.dec(15);
    BOOST_TEST(g.get_value() == -2);
}

BOOST_AUTO_TEST_CASE(non_atomic_construction)
{
    int_gauge<atomic_policy<false>> g("non_atomic_int_g_bytes", {"help text", "bytes"});
    BOOST_TEST(g.get_value() == 0);
    BOOST_TEST(g.is_atomic() == false);
}

BOOST_AUTO_TEST_CASE(non_atomic_inc_dec)
{
    int_gauge<atomic_policy<false>> g("non_atomic_int_g_total");
    g.inc(5);
    BOOST_TEST(g.get_value() == 5);
    g.inc(3);
    BOOST_TEST(g.get_value() == 8);
    g.inc(13);
    BOOST_TEST(g.get_value() == 21);
    g.dec(1);
    BOOST_TEST(g.get_value() == 20);
    g.dec(7);
    BOOST_TEST(g.get_value() == 13);
    g.dec(15);
    BOOST_TEST(g.get_value() == -2);
}

BOOST_AUTO_TEST_CASE(non_atomic_set)
{
    int_gauge<atomic_policy<false>> g("test_int_g");
    g.set(100);
    BOOST_TEST(g.get_value() == 100);
    g.set(-100);
    BOOST_TEST(g.get_value() == -100);
    g.set(123);
    BOOST_TEST(g.get_value() == 123);
    g.set(999);
    BOOST_TEST(g.get_value() == 999);
    g.set(-10);
    BOOST_TEST(g.get_value() == -10);
}

BOOST_AUTO_TEST_SUITE_END()
