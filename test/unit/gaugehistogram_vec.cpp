//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/gaugehistogram_vec.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_gaugehistogram_vec)

BOOST_AUTO_TEST_CASE(default_constructor_is_deleted)
{
    static_assert(!std::is_default_constructible_v<gaugehistogram_vec<>>);
    static_assert(!std::is_default_constructible_v<gaugehistogram_vec<atomic_policy<false>>>);
}

BOOST_AUTO_TEST_CASE(construction_default_options)
{
    gaugehistogram_vec h("temperature_dist_celsius", {"first", "second"});
}

BOOST_AUTO_TEST_CASE(construction_custom_options)
{
    gaugehistogram_vec h("temperature_dist_celsius", {.help = "help string", .unit = "celsius"}, {"first", "second"});
}

BOOST_AUTO_TEST_SUITE_END()
