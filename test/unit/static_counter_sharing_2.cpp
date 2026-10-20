//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/static_counter.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_static_counter_sharing)

void inc_from_tu_2()
{
    using counter = static_counter<"test_shared_c">;
    counter::inc();
}

void inc_from_tu_1();

// Test static_counter sharing between different TU
BOOST_AUTO_TEST_CASE(sharing_2, * boost::unit_test::tolerance(1e-9))
{
    using counter = static_counter<"test_shared_c">;
    double base_value = counter::get_value();
    inc_from_tu_1();
    inc_from_tu_2();
    inc_from_tu_1();
    inc_from_tu_2();

    BOOST_CHECK(counter::get_value() == base_value + 4.0);
}

BOOST_AUTO_TEST_SUITE_END()
