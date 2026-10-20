//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/info_vec.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

#include <array>
#include <vector>

using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_info_vec)

BOOST_AUTO_TEST_CASE(default_constructor_is_deleted)
{
    static_assert(!std::is_default_constructible_v<info_vec<>>);
    static_assert(!std::is_default_constructible_v<info_vec<atomic_policy<false>>>);
}

BOOST_AUTO_TEST_CASE(construction_default_options)
{
    info_vec in("build", {"first", "second"});
}

BOOST_AUTO_TEST_CASE(construction_custom_options)
{
    info_vec in("build", {.help = "help string"}, {"first", "second"});
}

BOOST_AUTO_TEST_SUITE_END()
