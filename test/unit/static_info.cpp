//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/static_info.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_static_info)

BOOST_AUTO_TEST_CASE(create)
{
    using build_info = static_info<"build", labels<"version", "component">>;
    build_info::create<"0.1", "auth">();
    build_info::create<"0.3", "session">();
}

BOOST_AUTO_TEST_SUITE_END()
