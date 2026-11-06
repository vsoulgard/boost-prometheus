//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/static_int_gauge.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_static_gauge)

BOOST_AUTO_TEST_CASE(simple_inc_dec)
{
    using test_gauge = static_int_gauge<"static_int_g">;
    BOOST_TEST(test_gauge::get_value() == 0);
    test_gauge::inc();
    BOOST_TEST(test_gauge::get_value() == 1);
    test_gauge::inc();
    BOOST_TEST(test_gauge::get_value() == 2);
    test_gauge::inc();
    BOOST_TEST(test_gauge::get_value() == 3);
    test_gauge::inc();
    BOOST_TEST(test_gauge::get_value() == 4);

    test_gauge::inc(2);
    BOOST_TEST(test_gauge::get_value() == 6);
    test_gauge::inc(3);
    BOOST_TEST(test_gauge::get_value() == 9);
    test_gauge::inc(5);
    BOOST_TEST(test_gauge::get_value() == 14);
    test_gauge::inc(10);
    BOOST_TEST(test_gauge::get_value() == 24);
    test_gauge::inc(12345);
    BOOST_TEST(test_gauge::get_value() == 12369);

    test_gauge::dec();
    BOOST_TEST(test_gauge::get_value() == 12368);
    test_gauge::dec();
    BOOST_TEST(test_gauge::get_value() == 12367);
    test_gauge::dec();
    BOOST_TEST(test_gauge::get_value() == 12366);
    test_gauge::dec();
    BOOST_TEST(test_gauge::get_value() == 12365);
    test_gauge::dec();
    BOOST_TEST(test_gauge::get_value() == 12364);

    test_gauge::dec(12345);
    BOOST_TEST(test_gauge::get_value() == 19);
    test_gauge::dec(10);
    BOOST_TEST(test_gauge::get_value() == 9);
    test_gauge::dec(5);
    BOOST_TEST(test_gauge::get_value() == 4);
    test_gauge::dec(12);
    BOOST_TEST(test_gauge::get_value() == -8);
    test_gauge::dec(2);
    BOOST_TEST(test_gauge::get_value() == -10);
}

BOOST_AUTO_TEST_CASE(with_labels_inc_dec)
{
    using test_gauge = static_int_gauge<"static_gauge_c", labels<"label_1", "label_2">>;
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 0);
    test_gauge::inc<"value_1", "value_2">();
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 1);
    test_gauge::inc<"value_1", "value_2">();
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 2);
    test_gauge::inc<"value_1", "value_2">();
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 3);
    test_gauge::inc<"value_1", "value_2">();
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 4);

    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 0);
    test_gauge::inc<"value_3", "value_4">();
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 1);
    test_gauge::inc<"value_3", "value_4">();
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 2);
    test_gauge::inc<"value_3", "value_4">();
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 3);
    test_gauge::inc<"value_3", "value_4">();
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 4);

    test_gauge::inc<"value_1", "value_2">(10);
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 14);
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 4);
    BOOST_TEST((test_gauge::get_value<"value_5", "value_6">()) == 0);
    test_gauge::inc<"value_3", "value_4">(10);
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 14);
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 14);
    BOOST_TEST((test_gauge::get_value<"value_5", "value_6">()) == 0);

    test_gauge::dec<"value_1", "value_2">();
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 13);
    test_gauge::dec<"value_1", "value_2">();
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 12);
    test_gauge::dec<"value_1", "value_2">();
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 11);
    test_gauge::dec<"value_1", "value_2">();
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 10);

    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 14);
    test_gauge::dec<"value_3", "value_4">();
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 13);
    test_gauge::dec<"value_3", "value_4">();
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 12);
    test_gauge::dec<"value_3", "value_4">();
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 11);
    test_gauge::dec<"value_3", "value_4">();
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 10);

    test_gauge::dec<"value_1", "value_2">(20);
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == -10);
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 10);
    BOOST_TEST((test_gauge::get_value<"value_5", "value_6">()) == 0);
    test_gauge::dec<"value_3", "value_4">(20);
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == -10);
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == -10);
    BOOST_TEST((test_gauge::get_value<"value_5", "value_6">()) == 0);
}

BOOST_AUTO_TEST_CASE(non_atomic_inc_dec)
{
    using test_gauge = static_int_gauge<
        "non_atomic_test_int_static_g",
        labels<"label_1", "label_2">,
        atomic_policy<false>>;
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 0);
    test_gauge::inc<"value_1", "value_2">();
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 1);
    test_gauge::inc<"value_1", "value_2">();
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 2);
    test_gauge::inc<"value_1", "value_2">();
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 3);
    test_gauge::inc<"value_1", "value_2">();
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 4);

    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 0);
    test_gauge::inc<"value_3", "value_4">();
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 1);
    test_gauge::inc<"value_3", "value_4">();
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 2);
    test_gauge::inc<"value_3", "value_4">();
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 3);
    test_gauge::inc<"value_3", "value_4">();
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 4);

    test_gauge::inc<"value_1", "value_2">(10);
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 14);
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 4);
    BOOST_TEST((test_gauge::get_value<"value_5", "value_6">()) == 0);
    test_gauge::inc<"value_3", "value_4">(10);
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 14);
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 14);
    BOOST_TEST((test_gauge::get_value<"value_5", "value_6">()) == 0);

    test_gauge::dec<"value_1", "value_2">();
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 13);
    test_gauge::dec<"value_1", "value_2">();
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 12);
    test_gauge::dec<"value_1", "value_2">();
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 11);
    test_gauge::dec<"value_1", "value_2">();
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == 10);

    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 14);
    test_gauge::dec<"value_3", "value_4">();
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 13);
    test_gauge::dec<"value_3", "value_4">();
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 12);
    test_gauge::dec<"value_3", "value_4">();
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 11);
    test_gauge::dec<"value_3", "value_4">();
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 10);

    test_gauge::dec<"value_1", "value_2">(20);
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == -10);
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == 10);
    BOOST_TEST((test_gauge::get_value<"value_5", "value_6">()) == 0);
    test_gauge::dec<"value_3", "value_4">(20);
    BOOST_TEST((test_gauge::get_value<"value_1", "value_2">()) == -10);
    BOOST_TEST((test_gauge::get_value<"value_3", "value_4">()) == -10);
    BOOST_TEST((test_gauge::get_value<"value_5", "value_6">()) == 0);
}

BOOST_AUTO_TEST_CASE(size)
{
    using empty_gauge = static_int_gauge<"size_test_empty_test_int_static_g">;
    BOOST_TEST(empty_gauge::size() == 0u);

    using test_gauge = static_int_gauge<"size_test">;
    BOOST_TEST(test_gauge::size() == 1u);
    test_gauge::inc();
    BOOST_TEST(test_gauge::size() == 1u);

    using test_gauge_labels = static_int_gauge<"size_test_labels", labels<"label_1", "label_2">>;
    // 3 different label sequences were created
    BOOST_TEST(test_gauge_labels::size() == 3u);

    test_gauge_labels::inc<"value_1", "value_2">();
    test_gauge_labels::inc<"value_1", "value_2">();
    test_gauge_labels::inc<"value_1", "value_2">();

    test_gauge_labels::inc<"value_3", "value_4">();
    test_gauge_labels::inc<"value_3", "value_4">();

    test_gauge_labels::inc<"value_5", "value_6">();

    BOOST_TEST(test_gauge_labels::size() == 3u);
}

BOOST_AUTO_TEST_CASE(is_atomic)
{
    using test_gauge_default = static_int_gauge<"test_int_c">;
    static_assert(test_gauge_default::is_atomic() == true);
    using test_gauge_atomic = static_int_gauge<"test_int_c", atomic_policy<true>>;
    static_assert(test_gauge_atomic::is_atomic() == true);
    using test_gauge_non_atomic = static_int_gauge<"test_int_c", atomic_policy<false>>;
    static_assert(test_gauge_non_atomic::is_atomic() == false);
}

BOOST_AUTO_TEST_SUITE_END()
