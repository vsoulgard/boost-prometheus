//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/static_int_counter.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_static_int_counter)

BOOST_AUTO_TEST_CASE(simple_increment)
{
    using test_counter = static_int_counter<"static_int_c_cpp">;
    BOOST_TEST(test_counter::get_value() == 0u);
    test_counter::inc();
    BOOST_TEST(test_counter::get_value() == 1u);
    test_counter::inc();
    BOOST_TEST(test_counter::get_value() == 2u);
    test_counter::inc();
    BOOST_TEST(test_counter::get_value() == 3u);
    test_counter::inc();
    BOOST_TEST(test_counter::get_value() == 4u);

    test_counter::inc(2u);
    BOOST_TEST(test_counter::get_value() == 6u);
    test_counter::inc(3u);
    BOOST_TEST(test_counter::get_value() == 9u);
    test_counter::inc(5u);
    BOOST_TEST(test_counter::get_value() == 14u);
    test_counter::inc(10);
    BOOST_TEST(test_counter::get_value() == 24u);
    test_counter::inc(12345u);
    BOOST_TEST(test_counter::get_value() == 12369u);
}

BOOST_AUTO_TEST_CASE(with_labels_increment)
{
    using test_counter = static_int_counter<"static_int_c", labels<"label_1", "label_2">>;
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 0u);
    test_counter::inc<"value_1", "value_2">();
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 1u);
    test_counter::inc<"value_1", "value_2">();
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 2u);
    test_counter::inc<"value_1", "value_2">();
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 3u);
    test_counter::inc<"value_1", "value_2">();
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 4u);

    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 0u);
    test_counter::inc<"value_3", "value_4">();
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 1u);
    test_counter::inc<"value_3", "value_4">();
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 2u);
    test_counter::inc<"value_3", "value_4">();
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 3u);
    test_counter::inc<"value_3", "value_4">();
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 4u);

    test_counter::inc<"value_1", "value_2">(10u);
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 14u);
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 4u);
    BOOST_TEST((test_counter::get_value<"value_5", "value_6">()) == 0u);
    test_counter::inc<"value_3", "value_4">(10u);
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 14u);
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 14u);
    BOOST_TEST((test_counter::get_value<"value_5", "value_6">()) == 0u);
}

BOOST_AUTO_TEST_CASE(non_atomic_increment)
{
    using test_counter = static_int_counter<
        "non_atomic_test_int_static_c",
        labels<"label_1", "label_2">,
        atomic_policy<false>>;
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 0u);
    test_counter::inc<"value_1", "value_2">();
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 1u);
    test_counter::inc<"value_1", "value_2">();
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 2u);
    test_counter::inc<"value_1", "value_2">();
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 3u);
    test_counter::inc<"value_1", "value_2">();
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 4u);

    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 0u);
    test_counter::inc<"value_3", "value_4">();
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 1u);
    test_counter::inc<"value_3", "value_4">();
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 2u);
    test_counter::inc<"value_3", "value_4">();
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 3u);
    test_counter::inc<"value_3", "value_4">();
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 4u);

    test_counter::inc<"value_1", "value_2">(10u);
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 14u);
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 4u);
    BOOST_TEST((test_counter::get_value<"value_5", "value_6">()) == 0u);
    test_counter::inc<"value_3", "value_4">(10u);
    BOOST_TEST((test_counter::get_value<"value_1", "value_2">()) == 14u);
    BOOST_TEST((test_counter::get_value<"value_3", "value_4">()) == 14u);
    BOOST_TEST((test_counter::get_value<"value_5", "value_6">()) == 0u);
}

BOOST_AUTO_TEST_CASE(size)
{
    using empty_counter = static_int_counter<"size_test_empty_test_int_static_c">;
    BOOST_TEST(empty_counter::size() == 0u);

    using test_counter = static_int_counter<"size_test">;
    BOOST_TEST(test_counter::size() == 1u);
    test_counter::inc();
    BOOST_TEST(test_counter::size() == 1u);

    using test_counter_labels = static_int_counter<"size_test_labels", labels<"label_1", "label_2">>;
    // 3 different label sequences were created
    BOOST_TEST(test_counter_labels::size() == 3u);

    test_counter_labels::inc<"value_1", "value_2">();
    test_counter_labels::inc<"value_1", "value_2">();
    test_counter_labels::inc<"value_1", "value_2">();

    test_counter_labels::inc<"value_3", "value_4">();
    test_counter_labels::inc<"value_3", "value_4">();

    test_counter_labels::inc<"value_5", "value_6">();

    BOOST_TEST(test_counter_labels::size() == 3u);
}

BOOST_AUTO_TEST_CASE(is_atomic)
{
    using test_counter_default = static_int_counter<"test_int_c">;
    static_assert(test_counter_default::is_atomic() == true);
    using test_counter_atomic = static_int_counter<"test_int_c", atomic_policy<true>>;
    static_assert(test_counter_atomic::is_atomic() == true);
    using test_counter_non_atomic = static_int_counter<"test_int_c", atomic_policy<false>>;
    static_assert(test_counter_non_atomic::is_atomic() == false);
}

BOOST_AUTO_TEST_SUITE_END()
