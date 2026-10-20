//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/detail/export/serialize_histogram_names.hpp>

#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::prometheus;
using namespace boost::prometheus::detail;

BOOST_AUTO_TEST_SUITE(test_serialize_histogram_names)

BOOST_AUTO_TEST_CASE(basic)
{
    std::span<const double> buckets = default_buckets::values;
    auto [base_string, buckets_offsets, sum_offset, count_offset] = serialize_histogram_names(
        "test_histogram", default_buckets::values);
    BOOST_TEST(buckets_offsets.size() == buckets.size());
    for (std::size_t i = 0; i < buckets.size() - 1; i++)
    {
        std::string_view bucket = std::string_view(base_string).substr(
            buckets_offsets[i], buckets_offsets[i + 1] - buckets_offsets[i]);
        std::array<char, 1024> buf;
        auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), buckets[i]);
        std::string bucket_fmt(buf.data(), ptr - buf.data());
        BOOST_TEST(bucket ==
            (std::string_view("test_histogram_bucket{le=\"" + bucket_fmt + "\"} ")));
    }
    // Last bucket
    {
        std::string_view bucket = std::string_view(base_string).substr(
            buckets_offsets.back(), sum_offset - buckets_offsets.back());
        std::array<char, 1024> buf;
        auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), buckets.back());
        std::string bucket_fmt(buf.data(), ptr - buf.data());
        BOOST_TEST(bucket ==
            (std::string_view("test_histogram_bucket{le=\"" + bucket_fmt + "\"} ")));
    }
    std::string_view inf_bucket = std::string_view(base_string).substr(
        0, buckets_offsets[0]);
    std::string_view sum_name = std::string_view(base_string).substr(
        sum_offset, count_offset - sum_offset);
    std::string_view count_name = std::string_view(base_string).substr(
        count_offset, base_string.size() - count_offset);
    BOOST_TEST(inf_bucket == std::string_view("test_histogram_bucket{le=\"+Inf\"} "));
    BOOST_TEST(sum_name == std::string_view("test_histogram_sum "));
    BOOST_TEST(count_name == std::string_view("test_histogram_count "));
}

BOOST_AUTO_TEST_CASE(custom_buckets)
{
    std::vector<double> buckets{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1};
    auto [base_string, buckets_offsets, sum_offset, count_offset] = serialize_histogram_names(
        "test_histogram", buckets);
    BOOST_TEST(buckets_offsets.size() == buckets.size());
    for (std::size_t i = 0; i < buckets.size() - 1; i++)
    {
        std::string_view bucket = std::string_view(base_string).substr(
            buckets_offsets[i], buckets_offsets[i + 1] - buckets_offsets[i]);
        std::array<char, 1024> buf;
        auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), buckets[i]);
        std::string bucket_fmt(buf.data(), ptr - buf.data());
        BOOST_TEST(bucket ==
            (std::string_view("test_histogram_bucket{le=\"" + bucket_fmt + "\"} ")));
    }
    // Last bucket
    {
        std::string_view bucket = std::string_view(base_string).substr(
            buckets_offsets.back(), sum_offset - buckets_offsets.back());
        std::array<char, 1024> buf;
        auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), buckets.back());
        std::string bucket_fmt(buf.data(), ptr - buf.data());
        BOOST_TEST(bucket ==
            (std::string_view("test_histogram_bucket{le=\"" + bucket_fmt + "\"} ")));
    }
    std::string_view inf_bucket = std::string_view(base_string).substr(
        0, buckets_offsets[0]);
    std::string_view sum_name = std::string_view(base_string).substr(
        sum_offset, count_offset - sum_offset);
    std::string_view count_name = std::string_view(base_string).substr(
        count_offset, base_string.size() - count_offset);
    BOOST_TEST(inf_bucket == std::string_view("test_histogram_bucket{le=\"+Inf\"} "));
    BOOST_TEST(sum_name == std::string_view("test_histogram_sum "));
    BOOST_TEST(count_name == std::string_view("test_histogram_count "));
}

BOOST_AUTO_TEST_CASE(gaugehistogram)
{
    std::span<const double> buckets = default_buckets::values;
    auto [base_string, buckets_offsets, sum_offset, count_offset] = serialize_gaugehistogram_names(
        "test_histogram", default_buckets::values);
    BOOST_TEST(buckets_offsets.size() == buckets.size());
    for (std::size_t i = 0; i < buckets.size() - 1; i++)
    {
        std::string_view bucket = std::string_view(base_string).substr(
            buckets_offsets[i], buckets_offsets[i + 1] - buckets_offsets[i]);
        std::array<char, 1024> buf;
        auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), buckets[i]);
        std::string bucket_fmt(buf.data(), ptr - buf.data());
        BOOST_TEST(bucket ==
            (std::string_view("test_histogram_bucket{le=\"" + bucket_fmt + "\"} ")));
    }
    // Last bucket
    {
        std::string_view bucket = std::string_view(base_string).substr(
            buckets_offsets.back(), sum_offset - buckets_offsets.back());
        std::array<char, 1024> buf;
        auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), buckets.back());
        std::string bucket_fmt(buf.data(), ptr - buf.data());
        BOOST_TEST(bucket ==
            (std::string_view("test_histogram_bucket{le=\"" + bucket_fmt + "\"} ")));
    }
    std::string_view inf_bucket = std::string_view(base_string).substr(
        0, buckets_offsets[0]);
    std::string_view sum_name = std::string_view(base_string).substr(
        sum_offset, count_offset - sum_offset);
    std::string_view count_name = std::string_view(base_string).substr(
        count_offset, base_string.size() - count_offset);
    BOOST_TEST(inf_bucket == std::string_view("test_histogram_bucket{le=\"+Inf\"} "));
    BOOST_TEST(sum_name == std::string_view("test_histogram_gsum "));
    BOOST_TEST(count_name == std::string_view("test_histogram_gcount "));
}

BOOST_AUTO_TEST_SUITE_END()
