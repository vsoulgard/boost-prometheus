//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_IMPL_HISTOGRAM_INTERNAL_IPP
#define BOOST_PROMETHEUS_IMPL_HISTOGRAM_INTERNAL_IPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/histogram_internal.hpp>

#include <boost/assert.hpp>

namespace boost::prometheus::detail {

template <typename AtomicPolicyType, bool HasBaseString, bool HasPrefixSVs>
histogram_internal<AtomicPolicyType, HasBaseString, HasPrefixSVs>::buffers_type
histogram_internal<AtomicPolicyType, HasBaseString, HasPrefixSVs>::init_buffers() noexcept
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

template <typename AtomicPolicyType, bool HasBaseString, bool HasPrefixSVs>
std::vector<std::string_view>
histogram_internal<AtomicPolicyType, HasBaseString, HasPrefixSVs>::init_bucket_names(
    const std::string& base_histogram_string,
    const std::vector<std::size_t>& bucket_offsets,
    std::size_t sum_offset)
{
    BOOST_ASSERT(bucket_offsets.size() > 0);
    std::vector<std::string_view> result;
    result.reserve(bucket_offsets.size() + 1);
    // Put +Inf in the end
    std::string_view inf_bucket(
        std::string_view(base_histogram_string).substr(
            0, bucket_offsets[0]));
    for (std::size_t i = 0; i < bucket_offsets.size() - 1; i++)
    {
        std::size_t off_start = bucket_offsets[i];
        std::size_t off_end = bucket_offsets[i + 1];
        result.push_back(
        std::string_view(base_histogram_string).substr(
            off_start, off_end - off_start));
    }
    // Last bucket
    {
        std::size_t off_start = bucket_offsets.back();
        std::size_t off_end = sum_offset;
        result.push_back(
        std::string_view(base_histogram_string).substr(
            off_start, off_end - off_start));
    }
    result.push_back(inf_bucket);
    return result;
}

template <typename AtomicPolicyType, bool HasBaseString, bool HasPrefixSVs>
histogram_internal<AtomicPolicyType, HasBaseString, HasPrefixSVs>::histogram_internal(
    std::span<const double> bucket_boundaries,
    std::string&& base_histogram_string,
    const std::vector<std::size_t>& bucket_offsets,
    std::size_t sum_offset,
    std::size_t count_offset
) requires (!(HasBaseString && HasPrefixSVs))
: bucket_boundaries_(bucket_boundaries),
buffers_(init_buffers()),
base_histogram_string_(std::move(base_histogram_string)),
bucket_names_svs_(init_bucket_names(base_histogram_string_, bucket_offsets, sum_offset)),
sum_name_sv_(std::string_view(base_histogram_string_).substr(
    sum_offset, count_offset - sum_offset
)),
count_name_sv_(std::string_view(base_histogram_string_).substr(
    count_offset, base_histogram_string_.size() - count_offset
)) {}

template <typename AtomicPolicyType, bool HasBaseString, bool HasPrefixSVs>
histogram_internal<AtomicPolicyType, HasBaseString, HasPrefixSVs>::histogram_internal(
    std::string&& base_string,
    std::size_t openmetrics_prefix_offset,
    std::span<const double> bucket_boundaries,
    std::string&& base_histogram_string,
    const std::vector<std::size_t>& bucket_offsets,
    std::size_t sum_offset,
    std::size_t count_offset
) requires (HasBaseString && HasPrefixSVs)
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

template <typename AtomicPolicyType, bool HasBaseString, bool HasPrefixSVs>
void histogram_internal<AtomicPolicyType, HasBaseString, HasPrefixSVs>::observe(double value, std::uint64_t count) noexcept
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

#ifdef BOOST_PROMETHEUS_SEPARATE_COMPILATION

#define BOOST_PROMETHEUS_INSTANTIATE_HISTOGRAM(Policy)       \
    template struct histogram_internal<Policy, false, false>;\
    template struct histogram_internal<Policy, false, true>; \
    template struct histogram_internal<Policy, true, false>; \
    template struct histogram_internal<Policy, true, true>;

BOOST_PROMETHEUS_INSTANTIATE_HISTOGRAM(atomic_policy<true>)
BOOST_PROMETHEUS_INSTANTIATE_HISTOGRAM(atomic_policy<false>)

#endif

} // namespace boost::prometheus::detail

#endif
