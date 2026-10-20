//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/static_counter.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_static_counter)

BOOST_AUTO_TEST_CASE(simple_increment, * boost::unit_test::tolerance(1e-9))
{
    using test_counter = static_counter<"static_counter_cpp">;
    BOOST_TEST(test_counter::get_value() == 0.0);
    test_counter::inc();
    BOOST_TEST(test_counter::get_value() == 1.0);
    test_counter::inc();
    BOOST_TEST(test_counter::get_value() == 2.0);
    test_counter::inc();
    BOOST_TEST(test_counter::get_value() == 3.0);
    test_counter::inc();
    BOOST_TEST(test_counter::get_value() == 4.0);

    test_counter::inc(1.2);
    BOOST_TEST(test_counter::get_value() == 5.2);
    test_counter::inc(2.4);
    BOOST_TEST(test_counter::get_value() == 7.6);
    test_counter::inc(5.0);
    BOOST_TEST(test_counter::get_value() == 12.6);
    test_counter::inc(9.9);
    BOOST_TEST(test_counter::get_value() == 22.5);
    test_counter::inc(12345.6789);
    BOOST_TEST(test_counter::get_value() == 12368.1789);
}

BOOST_AUTO_TEST_CASE(with_labels_increment, * boost::unit_test::tolerance(1e-9))
{
    using test_counter = static_counter<"static_counter_cpp", labels<"label_1", "label_2">>;
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 0.0);
    test_counter::inc<"value_1", "value_2">();
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 1.0);
    test_counter::inc<"value_1", "value_2">();
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 2.0);
    test_counter::inc<"value_1", "value_2">();
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 3.0);
    test_counter::inc<"value_1", "value_2">();
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 4.0);

    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 0.0);
    test_counter::inc<"value_3", "value_4">();
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 1.0);
    test_counter::inc<"value_3", "value_4">();
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 2.0);
    test_counter::inc<"value_3", "value_4">();
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 3.0);
    test_counter::inc<"value_3", "value_4">();
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 4.0);

    test_counter::inc<"value_1", "value_2">(10.0);
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 14.0);
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 4.0);
    BOOST_TEST((test_counter::get_value<"value_5", "value_6">()) == 0.0);
    test_counter::inc<"value_3", "value_4">(10.0);
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 14.0);
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 14.0);
    BOOST_TEST((test_counter::get_value<"value_5", "value_6">()) == 0.0);
}

BOOST_AUTO_TEST_CASE(non_atomic_increment, * boost::unit_test::tolerance(1e-9))
{
    using test_counter = static_counter<"non_atomic_test_static_counter_cpp", labels<"label_1", "label_2">, atomic_policy<false>>;
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 0.0);
    test_counter::inc<"value_1", "value_2">();
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 1.0);
    test_counter::inc<"value_1", "value_2">();
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 2.0);
    test_counter::inc<"value_1", "value_2">();
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 3.0);
    test_counter::inc<"value_1", "value_2">();
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 4.0);

    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 0.0);
    test_counter::inc<"value_3", "value_4">();
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 1.0);
    test_counter::inc<"value_3", "value_4">();
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 2.0);
    test_counter::inc<"value_3", "value_4">();
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 3.0);
    test_counter::inc<"value_3", "value_4">();
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 4.0);

    test_counter::inc<"value_1", "value_2">(10.0);
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 14.0);
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 4.0);
    BOOST_TEST((test_counter::get_value<"value_5", "value_6">()) == 0.0);
    test_counter::inc<"value_3", "value_4">(10.0);
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 14.0);
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 14.0);
    BOOST_TEST((test_counter::get_value<"value_5", "value_6">()) == 0.0);
}

BOOST_AUTO_TEST_CASE(size)
{
    using empty_counter = static_counter<"size_test_empty_test_static_counter_cpp">;
    BOOST_TEST(empty_counter::size() == 0);

    using test_counter = static_counter<"size_test">;
    BOOST_TEST(test_counter::size() == 1);
    test_counter::inc();
    BOOST_TEST(test_counter::size() == 1);

    using test_counter_labels = static_counter<"size_test_labels", labels<"label_1", "label_2">>;
    // 3 different label sequences were created
    BOOST_TEST(test_counter_labels::size() == 3);

    test_counter_labels::inc<"value_1", "value_2">();
    test_counter_labels::inc<"value_1", "value_2">();
    test_counter_labels::inc<"value_1", "value_2">();

    test_counter_labels::inc<"value_3", "value_4">();
    test_counter_labels::inc<"value_3", "value_4">();

    test_counter_labels::inc<"value_5", "value_6">();

    BOOST_TEST(test_counter_labels::size() == 3);
}

BOOST_AUTO_TEST_CASE(is_atomic)
{
    using test_counter_default = static_counter<"test_c">;
    static_assert(test_counter_default::is_atomic() == true);
    using test_counter_atomic = static_counter<"test_c", atomic_policy<true>>;
    static_assert(test_counter_atomic::is_atomic() == true);
    using test_counter_non_atomic = static_counter<"test_c", atomic_policy<false>>;
    static_assert(test_counter_non_atomic::is_atomic() == false);
}

BOOST_AUTO_TEST_SUITE_END()
