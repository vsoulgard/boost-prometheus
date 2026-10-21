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

BOOST_AUTO_TEST_SUITE(test_static_registry)

BOOST_AUTO_TEST_CASE(name)
{
    using default_registry = static_registry<>;
    static_assert(std::string_view(default_registry::name()) == std::string_view(""));

    using test_registry = static_registry<"test_registry">;
    static_assert(std::string_view(test_registry::name()) == std::string_view("test_registry"));
}

BOOST_AUTO_TEST_CASE(size)
{   
    using empty_registry = static_registry<"empty_registry">;
    BOOST_TEST(empty_registry::size() == 0u);

    using test_registry = static_registry<"test_registry">;
    using counter = static_counter<"registered_c">;
    BOOST_TEST(test_registry::size() == 1u);
    test_registry::register_metric<counter>();
    BOOST_TEST(test_registry::size() == 1u);
    test_registry::register_metric<counter>();
    BOOST_TEST(test_registry::size() == 1u);

    using test_registry_multiple = static_registry<"test_registry_multiple">;
    using counter1 = static_counter<"registered_c1">;
    using counter2 = static_counter<"registered_c2">;
    using counter3 = static_counter<"registered_c3">;
    using counter4 = static_counter<"registered_c4">;
    BOOST_TEST(test_registry_multiple::size() == 4u);
    test_registry_multiple::register_metric<counter1>();
    test_registry_multiple::register_metric<counter2>();
    test_registry_multiple::register_metric<counter3>();
    test_registry_multiple::register_metric<counter4>();
    BOOST_TEST(test_registry_multiple::size() == 4u);

    BOOST_TEST(empty_registry::size() == 0u);
    BOOST_TEST(test_registry::size() == 1u);
    BOOST_TEST(test_registry_multiple::size() == 4u);
}

BOOST_AUTO_TEST_SUITE_END()
