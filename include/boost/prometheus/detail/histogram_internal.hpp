//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_HISTOGRAM_INTERNAL_HPP
#define BOOST_PROMETHEUS_DETAIL_HISTOGRAM_INTERNAL_HPP

#include <boost/prometheus/detail/basic_metric_internal.hpp>
#include <boost/prometheus/detail/histogram_bucket_value.hpp>
#include <boost/prometheus/detail/string_buffer.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <boost/assert.hpp>

#include <atomic>
#include <mutex>
#include <vector>
#include <string>
#include <span>

namespace boost::prometheus::detail {

template <typename AtomicPolicyType, bool HasBaseString = true, bool HasPrefixSVs = true>
struct histogram_internal : public basic_metric_internal<AtomicPolicyType, HasBaseString, HasPrefixSVs>
{
    static constexpr string_buffer prometheus_type = "histogram";
    static constexpr string_buffer openmetrics_type = "histogram";

    using base_type = basic_metric_internal<AtomicPolicyType, HasBaseString, HasPrefixSVs>;

    using sum_type = std::conditional_t<
        AtomicPolicyType::enabled,
        std::atomic<double>,
        double
    >;

    using count_type = std::conditional_t<
        AtomicPolicyType::enabled,
        std::atomic<std::uint64_t>,
        std::tuple<>
    >;

    using mutex_type = std::conditional_t<
        AtomicPolicyType::enabled,
        std::mutex,
        std::tuple<>
    >;

    struct histogram_buffer
    {
        using bucket_value_type = histogram_bucket_value<
            AtomicPolicyType,
            base_type::template alignment<std::uint64_t>>;
        using buffer_count_type = std::conditional_t<
            AtomicPolicyType::enabled,
            std::atomic<std::uint64_t>,
            std::tuple<>
        >;

        std::vector<bucket_value_type> buckets;
        alignas(base_type::template alignment<sum_type>)
        sum_type sum;
        // count is used for synchronization as a counter with release semantics
        [[no_unique_address]] alignas(base_type::template alignment<buffer_count_type>)
        buffer_count_type count;

        histogram_buffer() = delete;

        histogram_buffer(std::size_t bucket_count)
        : buckets(bucket_count), sum(), count() {}
    };

    std::span<const double> bucket_boundaries_;

    static constexpr std::size_t buffers_count = AtomicPolicyType::enabled ? 2 : 1;
    // Active and passive buffers for bucket values
    std::array<histogram_buffer, buffers_count> buffers_;
    [[no_unique_address]] alignas(base_type::template alignment<count_type>)
    count_type count_and_idx_;

    // Used in serialization
    [[no_unique_address]] alignas(base_type::template alignment<mutex_type>)
    mutex_type mtx_;

    // Serialized names
    const std::string base_histogram_string_;
    const std::vector<std::string_view> bucket_names_svs_;
    const std::string_view sum_name_sv_;
    const std::string_view count_name_sv_;

    decltype(buffers_) init_buffers()
    {
        BOOST_ASSERT_MSG(
            std::is_sorted(bucket_boundaries_.begin(), bucket_boundaries_.end(), std::less_equal<>{}),
            "Bucket boundaries must be strictly increasing!"
        );

        std::size_t bucket_count = bucket_boundaries_.size() + 1; // +inf bucket

        if constexpr (buffers_count == 2)
        {
            return { bucket_count, bucket_count };
        }
        else
        {
            return { bucket_count };
        }
    }

    std::vector<std::string_view> init_bucket_names(
        const std::string& base_histogram_string_,
        const std::vector<std::size_t>& bucket_offsets,
        std::size_t sum_offset)
    {
        BOOST_ASSERT(bucket_offsets.size() > 0);
        std::vector<std::string_view> result;
        result.reserve(bucket_offsets.size() + 1);
        // Put +Inf in the end
        std::string_view inf_bucket(
            std::string_view(base_histogram_string_).substr(
                0, bucket_offsets[0]));
        for (std::size_t i = 0; i < bucket_offsets.size() - 1; i++)
        {
            std::size_t off_start = bucket_offsets[i];
            std::size_t off_end = bucket_offsets[i + 1];
            result.push_back(
            std::string_view(base_histogram_string_).substr(
                off_start, off_end - off_start));
        }
        // Last bucket
        {
            std::size_t off_start = bucket_offsets.back();
            std::size_t off_end = sum_offset;
            result.push_back(
            std::string_view(base_histogram_string_).substr(
                off_start, off_end - off_start));
        }
        result.push_back(inf_bucket);
        return result;
    }

    histogram_internal(
        std::string&& base_string,
        std::size_t openmetrics_prefix_offset,
        std::span<const double> bucket_boundaries,
        std::string&& base_histogram_string,
        const std::vector<std::size_t>& bucket_offsets,
        std::size_t sum_offset,
        std::size_t count_offset
    )
    : base_type(std::move(base_string), openmetrics_prefix_offset),
    bucket_boundaries_(bucket_boundaries),
    buffers_(init_buffers()),
    base_histogram_string_(std::move(base_histogram_string)),
    bucket_names_svs_(init_bucket_names(base_histogram_string_, bucket_offsets, sum_offset)),
    sum_name_sv_(std::string_view(base_histogram_string_).substr(
        sum_offset, count_offset - sum_offset
    )),
    count_name_sv_(std::string_view(base_histogram_string_).substr(
        count_offset, base_histogram_string_.size() - count_offset
    )) {}

    histogram_internal(
        std::span<const double> bucket_boundaries,
        std::string&& base_histogram_string,
        const std::vector<std::size_t>& bucket_offsets,
        std::size_t sum_offset,
        std::size_t count_offset
    )
    : base_type(),
    bucket_boundaries_(bucket_boundaries),
    buffers_(init_buffers()),
    base_histogram_string_(std::move(base_histogram_string)),
    bucket_names_svs_(init_bucket_names(base_histogram_string_, bucket_offsets, sum_offset)),
    sum_name_sv_(std::string_view(base_histogram_string_).substr(
        sum_offset, count_offset - sum_offset
    )),
    count_name_sv_(std::string_view(base_histogram_string_).substr(
        count_offset, base_histogram_string_.size() - count_offset
    )) {}

    consteval static bool is_atomic()
    {
        return AtomicPolicyType::enabled;
    }

    void observe(double value, std::uint64_t count) noexcept
    {
        auto it = std::lower_bound(bucket_boundaries_.begin(), bucket_boundaries_.end(), value);
        std::size_t bucket_idx = std::distance(bucket_boundaries_.begin(), it);
        if constexpr (AtomicPolicyType::enabled)
        {
            // Get current buffer index
            std::uint64_t n = count_and_idx_.fetch_add(count, std::memory_order_acquire);
            std::size_t buffer_idx = static_cast<std::size_t>(n >> 63);
            auto& buffer = buffers_[buffer_idx];
            buffer.buckets[bucket_idx].value.fetch_add(count, std::memory_order_relaxed);
            buffer.sum.fetch_add(value * count, std::memory_order_relaxed);
            // Commit changes
            buffer.count.fetch_add(count, std::memory_order_release);
        }
        else
        {
            auto& buffer = buffers_[0];
            buffer.buckets[bucket_idx].value += count;
            buffer.sum += value * count;
        }
    }
};

} // namespace boost::prometheus::detail

#endif
