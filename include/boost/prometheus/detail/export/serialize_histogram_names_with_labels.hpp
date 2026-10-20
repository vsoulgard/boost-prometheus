//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_EXPORT_SERIALIZE_HISTOGRAM_NAMES_WITH_LABELS_HPP
#define BOOST_PROMETHEUS_DETAIL_EXPORT_SERIALIZE_HISTOGRAM_NAMES_WITH_LABELS_HPP

#include <boost/assert.hpp>

#include <algorithm>
#include <charconv>
#include <cstring>
#include <limits>
#include <vector>
#include <string>
#include <string_view>
#include <span>

namespace boost::prometheus::detail {

struct serialize_histogram_names_with_labels_result
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

template <bool IsGaugeHistogram, typename LabelsWithIndexType, typename LabelValuesType>
serialize_histogram_names_with_labels_result serialize_histogram_names_with_labels_impl(
    std::string_view name,
    std::span<const double> buckets,
    const LabelsWithIndexType& labels,
    const LabelValuesType& labelvalues
)
{
    BOOST_ASSERT_MSG(name.size() > 0, "Metric name cannot be empty");
    BOOST_ASSERT_MSG(std::is_sorted(labels.cbegin(), labels.cend()), "Label names must be sorted");
    BOOST_ASSERT_MSG([&] {
        for (auto const& [name, idx] : labels)
            if (name == "le")
                return false;
        return true;
    }(), "\"le\" label is reserved for histogram");

    serialize_histogram_names_with_labels_result result;

    std::size_t le_insert_pos = labels.size();
    for (std::size_t i = 0; i < labels.size(); i++)
    {
        if (labels[i].first > "le")
        {
            le_insert_pos = i;
            break;
        }
    }

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

    // Compute the exact size of the labels part before and after "le"
    std::size_t pre_size = 0;
    for (std::size_t i = 0; i < le_insert_pos; i++)
    {
        pre_size += labels[i].first.size() + labelvalues[labels[i].second].size() + 4;
    }
    std::size_t post_size = 0;
    for (std::size_t i = le_insert_pos; i < labels.size(); i++)
    {
        post_size += labels[i].first.size() + labelvalues[labels[i].second].size() + 4;
    }

    // Exact sizes of the resulting strings
    const std::size_t bucket_max_size = name.size() +
        std::string_view("_bucket").size() + pre_size + post_size + max_double_size +
        std::string_view("{\"\"}").size() + std::string_view("le").size() + 1;
    const std::size_t sum_size =
        name.size() + sum_suffix.size() + pre_size + post_size + 2;
    const std::size_t count_size =
        name.size() + count_suffix.size() + pre_size + post_size + 2;
    const std::size_t inf_size = name.size() +
        std::string_view("_bucket").size() + pre_size + post_size +
        std::string_view("{\"+Inf\"}").size() + std::string_view("le").size() + 2;

    const std::size_t base_string_total_size =
        buckets.size() * bucket_max_size + inf_size + sum_size + count_size;

    result.base_string.resize(base_string_total_size);
    result.buckets_offsets.reserve(buckets.size());

    // Build the +Inf bucket first, it will be used as a template for the others
    {
        char* data = result.base_string.data() + result.inf_bucket_offset;
        std::size_t off = 0;

        std::memcpy(data + off, name.data(), name.size());
        off += name.size();
        std::memcpy(data + off, "_bucket{", 8);
        off += 8;

        for (std::size_t i = 0; i < le_insert_pos; i++)
        {
            const auto& label_name = labels[i].first;
            const auto& label_value = labelvalues[labels[i].second];
            std::memcpy(data + off, label_name.data(), label_name.size());
            off += label_name.size();
            std::memcpy(data + off, "=\"", 2);
            off += 2;
            std::memcpy(data + off, label_value.data(), label_value.size());
            off += label_value.size();
            std::memcpy(data + off, "\",", 2);
            off += 2;
        }

        std::memcpy(data + off, "le=\"+Inf\",", 10);
        off += 10;

        for (std::size_t i = le_insert_pos; i < labels.size(); i++)
        {
            const auto& label_name = labels[i].first;
            const auto& label_value = labelvalues[labels[i].second];
            std::memcpy(data + off, label_name.data(), label_name.size());
            off += label_name.size();
            std::memcpy(data + off, "=\"", 2);
            off += 2;
            std::memcpy(data + off, label_value.data(), label_value.size());
            off += label_value.size();
            std::memcpy(data + off, "\",", 2);
            off += 2;
        }

        BOOST_ASSERT(off > 0);
        data[off - 1] = '}';
        data[off] = ' ';
        off += 1;

        BOOST_ASSERT(off == inf_size);
    }

    // Views into the +Inf bucket used to build other buckets and sum/count names
    const char* inf_data = result.base_string.data();
    std::string_view before_le_value(
        inf_data, name.size() + 8 + pre_size + 4
    );
    std::string_view after_le_value(
        inf_data + name.size() + 8 + pre_size + 8,
        inf_size - (name.size() + 8 + pre_size + 8)
    );
    std::string_view labels_before(
        // Take "{" at least
        inf_data + name.size() + 7, pre_size > 0 ? pre_size : 1
    );
    std::string_view labels_after = after_le_value.substr(
        // Take "," if needed
        pre_size == 0 ? 2 : 1
    );
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
        const std::size_t bucket_size =
            before_le_value.size() + le_value.size() + after_le_value.size();

        std::memcpy(bucket_data + off, before_le_value.data(), before_le_value.size());
        off += before_le_value.size();
        std::memcpy(bucket_data + off, le_value.data(), le_value.size());
        off += le_value.size();
        std::memcpy(bucket_data + off, after_le_value.data(), after_le_value.size());
        off += after_le_value.size();

        BOOST_ASSERT(off - bucket_offset == bucket_size);
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
        std::memcpy(sum_data + off, labels_before.data(), labels_before.size());
        off += labels_before.size();
        std::memcpy(sum_data + off, labels_after.data(), labels_after.size());
        off += labels_after.size();

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
        std::memcpy(count_data + off, labels_before.data(), labels_before.size());
        off += labels_before.size();
        std::memcpy(count_data + off, labels_after.data(), labels_after.size());
        off += labels_after.size();

        BOOST_ASSERT(off - result.count_offset == count_size);
        BOOST_ASSERT(off <= base_string_total_size);
        result.base_string.resize(off);
    }

    return result;
}

template <typename LabelsWithIndexType, typename LabelValuesType>
serialize_histogram_names_with_labels_result serialize_histogram_names_with_labels(
    std::string_view name,
    std::span<const double> buckets,
    const LabelsWithIndexType& labels,
    const LabelValuesType& labelvalues
)
{
    return serialize_histogram_names_with_labels_impl<false>(name, buckets, labels, labelvalues);
}

template <typename LabelsWithIndexType, typename LabelValuesType>
serialize_histogram_names_with_labels_result serialize_gaugehistogram_names_with_labels(
    std::string_view name,
    std::span<const double> buckets,
    const LabelsWithIndexType& labels,
    const LabelValuesType& labelvalues
)
{
    return serialize_histogram_names_with_labels_impl<true>(name, buckets, labels, labelvalues);
}

} // namespace boost::prometheus::detail

#endif
