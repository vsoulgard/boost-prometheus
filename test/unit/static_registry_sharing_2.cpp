//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/static_registry.hpp>
#include <boost/prometheus/static_counter.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_static_registry_sharing)

// Test static_registry sharing between different TU
BOOST_AUTO_TEST_CASE(sharing_2)
{
    using shared_registry = static_registry<"shared_registry">;
    using counter5 = static_counter<"test_shared_c5">;
    using counter6 = static_counter<"test_shared_c6">;
    using counter7 = static_counter<"test_shared_c7">;
    using counter8 = static_counter<"test_shared_c8">;
    shared_registry::register_metric<counter5>();
    shared_registry::register_metric<counter6>();
    shared_registry::register_metric<counter7>();
    shared_registry::register_metric<counter8>();

    BOOST_TEST(shared_registry::size() == 8u);
}

BOOST_AUTO_TEST_SUITE_END()
