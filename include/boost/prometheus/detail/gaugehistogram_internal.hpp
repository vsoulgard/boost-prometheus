//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_GAUGEHISTOGRAM_INTERNAL_HPP
#define BOOST_PROMETHEUS_DETAIL_GAUGEHISTOGRAM_INTERNAL_HPP

#include <boost/prometheus/detail/config.hpp>
#include <boost/prometheus/detail/histogram_bucket_value.hpp>
#include <boost/prometheus/detail/histogram_internal.hpp>
#include <boost/prometheus/detail/string_buffer.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <boost/assert.hpp>

#include <array>
#include <atomic>
#include <mutex>
#include <vector>
#include <string>
#include <span>

namespace boost::prometheus::detail {

template <typename AtomicPolicyType, bool HasBaseString = true, bool HasPrefixSVs = true>
struct gaugehistogram_internal : public histogram_internal<AtomicPolicyType, HasBaseString, HasPrefixSVs>
{
    using base_type = histogram_internal<AtomicPolicyType, HasBaseString, HasPrefixSVs>;

    // Falback 0.0.4 headers
    const std::string sum_header;
    const std::string count_header;

    gaugehistogram_internal(
        std::string&& base_string,
        std::size_t openmetrics_prefix_offset,
        std::span<const double> bucket_boundaries,
        std::string&& base_histogram_string,
        const std::vector<std::size_t>& bucket_offsets,
        std::size_t sum_offset,
        std::size_t count_offset,
        std::string&& sum_header,
        std::string&& count_header
    )
    : base_type(
        std::move(base_string),
        openmetrics_prefix_offset,
        bucket_boundaries,
        std::move(base_histogram_string),
        bucket_offsets,
        sum_offset,
        count_offset),
    sum_header(std::move(sum_header)),
    count_header(std::move(count_header)) {}

    gaugehistogram_internal(
        std::span<const double> bucket_boundaries,
        std::string&& base_histogram_string,
        const std::vector<std::size_t>& bucket_offsets,
        std::size_t sum_offset,
        std::size_t count_offset,
        std::string&& sum_header,
        std::string&& count_header
    )
    : base_type(
        bucket_boundaries,
        std::move(base_histogram_string),
        bucket_offsets,
        sum_offset,
        count_offset),
    sum_header(std::move(sum_header)),
    count_header(std::move(count_header)) {}

    void sub(double value, std::uint64_t count) noexcept
    {
        auto it = std::lower_bound(base_type::bucket_boundaries_.begin(), base_type::bucket_boundaries_.end(), value);
        std::size_t bucket_idx = std::distance(base_type::bucket_boundaries_.begin(), it);
        if constexpr (AtomicPolicyType::enabled)
        {
            // Get current buffer index
            std::uint64_t n = base_type::count_and_idx_.fetch_add(count, std::memory_order_acquire);
            std::size_t buffer_idx = static_cast<std::size_t>(n >> 63);
            auto& buffer = base_type::buffers_[buffer_idx];
            buffer.buckets[bucket_idx].value.fetch_sub(count, std::memory_order_relaxed);
            buffer.sum.fetch_sub(value * count, std::memory_order_relaxed);
            // Commit changes
            buffer.count.fetch_add(count, std::memory_order_release);
        }
        else
        {
            auto& buffer = base_type::buffers_[0];
            buffer.buckets[bucket_idx].value -= count;
            buffer.sum -= value * count;
        }
    }
};

} // namespace boost::prometheus::detail

#endif
