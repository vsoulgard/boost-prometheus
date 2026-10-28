//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/gauge.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_gauge)

BOOST_AUTO_TEST_CASE(default_constructor_is_deleted)
{
    static_assert(!std::is_default_constructible_v<gauge<>>);
    static_assert(!std::is_default_constructible_v<gauge<atomic_policy<false>>>);
}

BOOST_AUTO_TEST_CASE(construction_default_options)
{
    gauge g("test_gauge");
    BOOST_TEST(g.get_value() == 0.0);
}

BOOST_AUTO_TEST_CASE(construction_custom_options)
{
    gauge g("request_duration_seconds", {.help = "help string", .unit = "seconds"});
    BOOST_TEST(g.get_value() == 0.0);
}

BOOST_AUTO_TEST_CASE(inc_dec_default, * boost::unit_test::tolerance(1e-9))
{
    gauge g("test_gauge");
    g.inc();
    BOOST_TEST(g.get_value() == 1.0);
    g.inc();
    BOOST_TEST(g.get_value() == 2.0);
    g.inc();
    BOOST_TEST(g.get_value() == 3.0);
    g.inc();
    BOOST_TEST(g.get_value() == 4.0);
    g.inc();
    BOOST_TEST(g.get_value() == 5.0);
    g.dec();
    BOOST_TEST(g.get_value() == 4.0);
    g.dec();
    BOOST_TEST(g.get_value() == 3.0);
    g.dec();
    BOOST_TEST(g.get_value() == 2.0);
    g.dec();
    BOOST_TEST(g.get_value() == 1.0);
    g.dec();
    BOOST_TEST(g.get_value() == 0.0);
    g.dec();
    BOOST_TEST(g.get_value() == -1.0);
}

BOOST_AUTO_TEST_CASE(set, * boost::unit_test::tolerance(1e-9))
{
    gauge g("test_gauge");
    g.set(100.0);
    BOOST_TEST(g.get_value() == 100.0);
    g.set(-100.0);
    BOOST_TEST(g.get_value() == -100.0);
    g.set(123.45);
    BOOST_TEST(g.get_value() == 123.45);
    g.set(999.01);
    BOOST_TEST(g.get_value() == 999.01);
    g.set(-0.9);
    BOOST_TEST(g.get_value() == -0.9);
}

BOOST_AUTO_TEST_CASE(inc_dec_custom_value, * boost::unit_test::tolerance(1e-9))
{
    gauge g("test_gauge");
    g.inc(5.5);
    BOOST_TEST(g.get_value() == 5.5);
    g.inc(2.5);
    BOOST_TEST(g.get_value() == 8.0);
    g.inc(13.2);
    BOOST_TEST(g.get_value() == 21.2);
    g.dec(1.1);
    BOOST_TEST(g.get_value() == 20.1);
    g.dec(6.7);
    BOOST_TEST(g.get_value() == 13.4);
    g.dec(15.0);
    BOOST_TEST(g.get_value() == -1.6);
}

BOOST_AUTO_TEST_CASE(non_atomic_construction)
{
    gauge<atomic_policy<false>> g("non_atomic_gauge_bytes_total", {"help text", "bytes"});
    BOOST_TEST(g.get_value() == 0.0);
    BOOST_TEST(g.is_atomic() == false);
}

BOOST_AUTO_TEST_CASE(non_atomic_inc_dec, * boost::unit_test::tolerance(1e-9))
{
    gauge<atomic_policy<false>> g("non_atomic_gauge_total");
    g.inc(5.5);
    BOOST_TEST(g.get_value() == 5.5);
    g.inc(2.5);
    BOOST_TEST(g.get_value() == 8.0);
    g.inc(13.2);
    BOOST_TEST(g.get_value() == 21.2);
    g.dec(1.1);
    BOOST_TEST(g.get_value() == 20.1);
    g.dec(6.7);
    BOOST_TEST(g.get_value() == 13.4);
    g.dec(15.0);
    BOOST_TEST(g.get_value() == -1.6);
}

BOOST_AUTO_TEST_CASE(non_atomic_set, * boost::unit_test::tolerance(1e-9))
{
    gauge<atomic_policy<false>> g("test_gauge");
    g.set(100.0);
    BOOST_TEST(g.get_value() == 100.0);
    g.set(-100.0);
    BOOST_TEST(g.get_value() == -100.0);
    g.set(123.45);
    BOOST_TEST(g.get_value() == 123.45);
    g.set(999.01);
    BOOST_TEST(g.get_value() == 999.01);
    g.set(-0.9);
    BOOST_TEST(g.get_value() == -0.9);
}

BOOST_AUTO_TEST_SUITE_END()
