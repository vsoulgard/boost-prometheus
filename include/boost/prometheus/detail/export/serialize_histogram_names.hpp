//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_EXPORT_SERIALIZE_HISTOGRAM_NAMES_HPP
#define BOOST_PROMETHEUS_DETAIL_EXPORT_SERIALIZE_HISTOGRAM_NAMES_HPP

#include <boost/assert.hpp>

#include <array>
#include <charconv>
#include <cstring>
#include <limits>
#include <vector>
#include <string>
#include <string_view>
#include <span>
#include <type_traits>

namespace boost::prometheus::detail {

// Result is the same for Prometheus 0.0.4 and OpenMetrics 1.0
struct serialize_histogram_names_result
{
    std::string base_string;
    // +Inf bucket always starts from 0 offset
    static constexpr std::size_t inf_bucket_offset = 0;
    // Points to buckets strings (without +Inf) inside of base_string
    std::vector<std::size_t> buckets_offsets;
    // Points to sum string inside of base_string
    std::size_t sum_offset;
    // Points to count string inside of base_string
    std::size_t count_offset;
};

template <bool IsGaugeHistogram>
auto serialize_histogram_names_impl(
    std::string_view name,
    std::span<const double> buckets)
{
    BOOST_ASSERT_MSG(name.size() > 0, "Metric name cannot be empty");

    serialize_histogram_names_result result;

    constexpr std::size_t max_double_size = 
        std::numeric_limits<double>::max_digits10 +
        std::string_view("-").size() + std::string_view(".").size() +
        std::string_view("e").size() + std::string_view("+").size()
        + 3 // exponent
    ;

    constexpr std::string_view sum_suffix = []() consteval
    {
        if constexpr (IsGaugeHistogram)
            return "_gsum";
        else
            return "_sum";
    }();
    constexpr std::string_view count_suffix = []() consteval
    {
        if constexpr (IsGaugeHistogram)
            return "_gcount";
        else
            return "_count";
    }();

    const std::size_t required_size = name.size() + 1;
    const std::size_t bucket_max_size = required_size +
        std::string_view("_bucket").size() +
        std::string_view("le").size() +
        max_double_size + std::string_view("{\"\"}").size();
    const std::size_t sum_size = required_size + sum_suffix.size();
    const std::size_t count_size = required_size + count_suffix.size();
    const std::size_t inf_size =
        name.size() + std::string_view("_bucket{le=\"+Inf\"} ").size();
    const std::size_t base_string_total_size = (buckets.size() * bucket_max_size) +
        inf_size + sum_size + count_size;
    result.base_string.resize(base_string_total_size);
    result.buckets_offsets.reserve(buckets.size());

    // Build the +Inf bucket first, it will be used as a template for the others
    {
        char* inf_bucket_data = result.base_string.data() + result.inf_bucket_offset;
        std::size_t off = 0;
        std::memcpy(inf_bucket_data + off, name.data(), name.size());
        off += name.size();
        std::memcpy(inf_bucket_data + off, "_bucket{le=\"+Inf\"} ", 19);
        off += 19;
        BOOST_ASSERT(off == inf_size);
    }

    // Reusable prefix from the +Inf bucket: "name_bucket{le=\""
    const std::size_t before_value_size =
        name.size() + std::string_view("_bucket{le=\"").size();
    std::string_view before_value(result.base_string.data(), before_value_size);

    // Create the other buckets
    std::size_t bucket_offset = inf_size;
    for (std::size_t i = 0; i < buckets.size(); i++)
    {
        std::array<char, max_double_size> buf;
        auto [ptr, ec] = std::to_chars(
            buf.data(), buf.data() + buf.size(), buckets[i], std::chars_format::general
        );
        BOOST_ASSERT(ec == std::errc());
        std::string_view le_value(buf.data(), ptr - buf.data());

        result.buckets_offsets.push_back(bucket_offset);
        std::size_t off = bucket_offset;
        char* bucket_data = result.base_string.data();

        std::memcpy(bucket_data + off, before_value.data(), before_value.size());
        off += before_value.size();
        std::memcpy(bucket_data + off, le_value.data(), le_value.size());
        off += le_value.size();
        std::memcpy(bucket_data + off, "\"} ", 3);
        off += 3;
        BOOST_ASSERT(off - bucket_offset <= bucket_max_size);
        bucket_offset = off;
    }
    BOOST_ASSERT(result.buckets_offsets.size() == buckets.size());
    result.sum_offset = bucket_offset;
    result.count_offset = result.sum_offset + sum_size;
    // Build the sum name
    {
        std::size_t off = result.sum_offset;
        char* sum_data = result.base_string.data();

        std::memcpy(sum_data + off, name.data(), name.size());
        off += name.size();
        std::memcpy(sum_data + off, sum_suffix.data(), sum_suffix.size());
        off += sum_suffix.size();
        std::memcpy(sum_data + off, " ", 1);
        off += 1;

        BOOST_ASSERT(off - result.sum_offset == sum_size);
    }
    // Build the count name
    {
        std::size_t off = result.count_offset;
        char* count_data = result.base_string.data();

        std::memcpy(count_data + off, name.data(), name.size());
        off += name.size();
        std::memcpy(count_data + off, count_suffix.data(), count_suffix.size());
        off += count_suffix.size();
        std::memcpy(count_data + off, " ", 1);
        off += 1;

        BOOST_ASSERT(off - result.count_offset == count_size);
        BOOST_ASSERT(off <= base_string_total_size);
        result.base_string.resize(off);
    }

    return result;
}

inline serialize_histogram_names_result serialize_histogram_names(
    std::string_view name,
    std::span<const double> buckets)
{
    return serialize_histogram_names_impl<false>(name, buckets);
}

inline serialize_histogram_names_result serialize_gaugehistogram_names(
    std::string_view name,
    std::span<const double> buckets)
{
    return serialize_histogram_names_impl<true>(name, buckets);
}

} // namespace boost::prometheus::detail

#endif
