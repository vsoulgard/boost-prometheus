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
BOOST_AUTO_TEST_CASE(sharing_1)
{
    using shared_registry = static_registry<"shared_registry">;
    using counter1 = static_counter<"test_shared_c1">;
    using counter2 = static_counter<"test_shared_c2">;
    using counter3 = static_counter<"test_shared_c3">;
    using counter4 = static_counter<"test_shared_c4">;
    shared_registry::register_metric<counter1>();
    shared_registry::register_metric<counter2>();
    shared_registry::register_metric<counter3>();
    shared_registry::register_metric<counter4>();

    BOOST_TEST(shared_registry::size() == 8);
}

BOOST_AUTO_TEST_SUITE_END()
