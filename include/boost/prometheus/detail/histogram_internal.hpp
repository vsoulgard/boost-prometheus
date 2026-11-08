//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_HISTOGRAM_INTERNAL_HPP
#define BOOST_PROMETHEUS_DETAIL_HISTOGRAM_INTERNAL_HPP

#include <boost/prometheus/detail/basic_metric_internal.hpp>
#include <boost/prometheus/detail/config.hpp>
#include <boost/prometheus/detail/histogram_bucket_value.hpp>
#include <boost/prometheus/detail/string_buffer.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <array>
#include <atomic>
#include <mutex>
#include <vector>
#include <string>
#include <span>

namespace boost::prometheus::detail {

BOOST_PROMETHEUS_MSVC_DISABLE_ALIGN_WARNING
template <typename AtomicPolicyType, bool HasBaseString = true, bool HasPrefixSVs = true>
struct histogram_internal : public basic_metric_internal<AtomicPolicyType, HasBaseString, HasPrefixSVs>
{
    using base_type = basic_metric_internal<AtomicPolicyType, HasBaseString, HasPrefixSVs>;

    using sum_type = std::conditional_t<
        AtomicPolicyType::enabled,
        std::atomic<double>,
        double>;

    using count_type = std::conditional_t<
        AtomicPolicyType::enabled,
        std::atomic<std::uint64_t>,
        std::tuple<>>;

    using mutex_type = std::conditional_t<
        AtomicPolicyType::enabled,
        std::mutex,
        std::tuple<>>;

    struct histogram_buffer
    {
        using bucket_value_type = histogram_bucket_value<
            AtomicPolicyType,
            base_type::template alignment<std::uint64_t>>;
        using buffer_count_type = std::conditional_t<
            AtomicPolicyType::enabled,
            std::atomic<std::uint64_t>,
            std::tuple<>>;

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
    using buffers_type = std::array<histogram_buffer, buffers_count>;
    // Active and passive buffers for bucket values
    buffers_type buffers_;
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

    buffers_type init_buffers() noexcept;

    std::vector<std::string_view> init_bucket_names(
        const std::string& base_histogram_string,
        const std::vector<std::size_t>& bucket_offsets,
        std::size_t sum_offset);

    histogram_internal(
        std::span<const double> bucket_boundaries,
        std::string&& base_histogram_string,
        const std::vector<std::size_t>& bucket_offsets,
        std::size_t sum_offset,
        std::size_t count_offset
    ) requires (!(HasBaseString && HasPrefixSVs));

    histogram_internal(
        std::string&& base_string,
        std::size_t openmetrics_prefix_offset,
        std::span<const double> bucket_boundaries,
        std::string&& base_histogram_string,
        const std::vector<std::size_t>& bucket_offsets,
        std::size_t sum_offset,
        std::size_t count_offset
    ) requires (HasBaseString && HasPrefixSVs);

    consteval static bool is_atomic() { return AtomicPolicyType::enabled; }

    void observe(double value, std::uint64_t count) noexcept;
};
BOOST_PROMETHEUS_MSVC_RESTORE_WARNING

} // namespace boost::prometheus::detail

#ifdef BOOST_PROMETHEUS_HEADER_ONLY
#include <boost/prometheus/impl/histogram_internal.ipp>
#endif

#endif
