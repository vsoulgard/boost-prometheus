//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/static_histogram.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_static_histogram)

BOOST_AUTO_TEST_CASE(observe)
{
    using test_histogram = static_histogram<"test_histogram">;
    test_histogram::observe(0.0);
    test_histogram::observe(0.1);
    test_histogram::observe(0.2);
    test_histogram::observe(0.3);
    test_histogram::observe(0.4);
    test_histogram::observe(0.5);
    test_histogram::observe(0.6);
    test_histogram::observe(0.7);
    test_histogram::observe(0.8);
    test_histogram::observe(0.9);
    test_histogram::observe(1.0);
    test_histogram::observe(2.0);
    test_histogram::observe(5.0);
    test_histogram::observe(10.0);
    test_histogram::observe(100.0);
}

BOOST_AUTO_TEST_CASE(observe_with_labels)
{
    using test_histogram = static_histogram<"test_histogram", labels<"label_1", "label_2">>;
    test_histogram::observe<"first", "second">(0.0);
    test_histogram::observe<"first", "second">(0.1);
    test_histogram::observe<"first", "second">(0.2);
    test_histogram::observe<"first", "second">(0.3);
    test_histogram::observe<"first", "second">(0.4);
    test_histogram::observe<"first", "second">(0.5);
    test_histogram::observe<"first", "second">(0.6);
    test_histogram::observe<"first", "second">(0.7);
    test_histogram::observe<"first", "second">(0.8);
    test_histogram::observe<"first", "second">(0.9);
    test_histogram::observe<"first", "second">(1.0);
    test_histogram::observe<"first", "second">(2.0);
    test_histogram::observe<"first", "second">(5.0);
    test_histogram::observe<"first", "second">(10.0);
    test_histogram::observe<"first", "second">(100.0);
}

BOOST_AUTO_TEST_CASE(custom_buckets_observe)
{
    using test_histogram = static_histogram<"test_histogram", buckets<0.1, 0.2, 0.3, 0.4, 0.5>>;
    test_histogram::observe(0.0);
    test_histogram::observe(0.1);
    test_histogram::observe(0.2);
    test_histogram::observe(0.3);
    test_histogram::observe(0.4);
    test_histogram::observe(0.5);
    test_histogram::observe(0.6);
    test_histogram::observe(0.7);
    test_histogram::observe(0.8);
    test_histogram::observe(0.9);
    test_histogram::observe(1.0);
    test_histogram::observe(2.0);
    test_histogram::observe(5.0);
    test_histogram::observe(10.0);
    test_histogram::observe(100.0);
}

BOOST_AUTO_TEST_CASE(non_atomic_observe)
{
    using test_histogram = static_histogram<"non_atomic_test_counter_total", labels<"label_1", "label_2">, atomic_policy<false>>;
    test_histogram::observe<"first", "second">(0.0);
    test_histogram::observe<"first", "second">(0.1);
    test_histogram::observe<"first", "second">(0.2);
    test_histogram::observe<"first", "second">(0.3);
    test_histogram::observe<"first", "second">(0.4);
    test_histogram::observe<"first", "second">(0.5);
    test_histogram::observe<"first", "second">(0.6);
    test_histogram::observe<"first", "second">(0.7);
    test_histogram::observe<"first", "second">(0.8);
    test_histogram::observe<"first", "second">(0.9);
    test_histogram::observe<"first", "second">(1.0);
    test_histogram::observe<"first", "second">(2.0);
    test_histogram::observe<"first", "second">(5.0);
    test_histogram::observe<"first", "second">(10.0);
    test_histogram::observe<"first", "second">(100.0);
}

BOOST_AUTO_TEST_CASE(size)
{
    using empty_histogram = static_histogram<"size_test_empty_histogram">;
    BOOST_TEST(empty_histogram::size() == 0u);

    using test_histogram = static_histogram<"size_test_histogram">;
    BOOST_TEST(test_histogram::size() == 1u);
    test_histogram::observe(1.0);
    BOOST_TEST(test_histogram::size() == 1u);

    using test_histogram_labels = static_histogram<"size_test_histogram_labels", labels<"label_1", "label_2">>;
    // 3 different label sequences were created
    BOOST_TEST(test_histogram_labels::size() == 3u);

    test_histogram_labels::observe<"value_1", "value_2">(1.0);
    test_histogram_labels::observe<"value_1", "value_2">(1.0);
    test_histogram_labels::observe<"value_1", "value_2">(1.0);

    test_histogram_labels::observe<"value_3", "value_4">(1.0);
    test_histogram_labels::observe<"value_3", "value_4">(1.0);

    test_histogram_labels::observe<"value_5", "value_6">(1.0);

    BOOST_TEST(test_histogram_labels::size() == 3u);
}

BOOST_AUTO_TEST_CASE(is_atomic)
{
    using test_histogram_default = static_histogram<"test_histogram">;
    static_assert(test_histogram_default::is_atomic() == true);
    using test_histogram_atomic = static_histogram<"test_histogram", atomic_policy<true>>;
    static_assert(test_histogram_atomic::is_atomic() == true);
    using test_histogram_non_atomic = static_histogram<"test_histogram", atomic_policy<false>>;
    static_assert(test_histogram_non_atomic::is_atomic() == false);
}

BOOST_AUTO_TEST_SUITE_END()
