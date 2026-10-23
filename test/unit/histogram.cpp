//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/histogram.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_histogram)

BOOST_AUTO_TEST_CASE(default_constructor_is_deleted)
{
    static_assert(!std::is_default_constructible_v<histogram<>>);
    static_assert(!std::is_default_constructible_v<histogram<atomic_policy<false>>>);
}

BOOST_AUTO_TEST_CASE(construction_default_options)
{
    histogram h("my_histogram");
}

BOOST_AUTO_TEST_CASE(construction_custom_options)
{
    histogram h("request_duration_seconds", {.help = "help string", .unit = "seconds", .buckets = {}});
}

BOOST_AUTO_TEST_CASE(observe_default_options)
{
    histogram h("request_duration_seconds");
    h.observe(0.0);
    h.observe(0.1);
    h.observe(0.2);
    h.observe(0.3);
    h.observe(0.4);
    h.observe(0.5);
    h.observe(0.6);
    h.observe(0.7);
    h.observe(0.8);
    h.observe(0.9);
    h.observe(1.0);
    h.observe(2.0);
    h.observe(10.0);
    h.observe(100.0);
}

BOOST_AUTO_TEST_CASE(observe_custom_options)
{
    histogram h("request_duration_seconds",
        {.help = "help string", .unit = "seconds", .buckets = {0.01, 0.1, 0.5, 1.0, 2.0, 5.0}});
    h.observe(0.0);
    h.observe(0.1);
    h.observe(0.2);
    h.observe(0.3);
    h.observe(0.4);
    h.observe(0.5);
    h.observe(0.6);
    h.observe(0.7);
    h.observe(0.8);
    h.observe(0.9);
    h.observe(1.0);
    h.observe(2.0);
    h.observe(10.0);
    h.observe(100.0);
}

BOOST_AUTO_TEST_CASE(observe_non_atomic)
{
    histogram<atomic_policy<false>> h("request_duration_seconds");
    BOOST_TEST(h.is_atomic() == false);
    h.observe(0.0);
    h.observe(0.1);
    h.observe(0.2);
    h.observe(0.3);
    h.observe(0.4);
    h.observe(0.5);
    h.observe(0.6);
    h.observe(0.7);
    h.observe(0.8);
    h.observe(0.9);
    h.observe(1.0);
    h.observe(2.0);
    h.observe(10.0);
    h.observe(100.0);
}

BOOST_AUTO_TEST_SUITE_END()
