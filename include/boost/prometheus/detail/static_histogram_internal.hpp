//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_STATIC_HISTOGRAM_INTERNAL_HPP
#define BOOST_PROMETHEUS_DETAIL_STATIC_HISTOGRAM_INTERNAL_HPP

#include <boost/prometheus/detail/config.hpp>
#include <boost/prometheus/detail/histogram_bucket_value.hpp>

#include <boost/container/small_vector.hpp>

#include <array>
#include <memory>
#include <mutex>

namespace boost::prometheus::detail {

BOOST_PROMETHEUS_MSVC_DISABLE_ALIGN_WARNING
template <typename AtomicPolicyType>
struct static_histogram_internal
{
    template <typename T>
    static constexpr std::size_t alignment = AtomicPolicyType::enabled ? cache_line_size : alignof(T);

    using mutex_ptr_type = std::conditional_t<AtomicPolicyType::enabled, std::unique_ptr<std::mutex>, std::tuple<>>;

    using count_type = std::conditional_t<
        AtomicPolicyType::enabled,
        std::uint64_t,
        std::tuple<>
    >;

    struct histogram_entry
    {
        struct histogram_buffer
        {
            using bucket_value_type = histogram_bucket_value<AtomicPolicyType, alignment<std::uint64_t>>;
            using buffer_count_type = std::conditional_t<
                AtomicPolicyType::enabled,
                std::uint64_t,
                std::tuple<>
            >;

            std::vector<bucket_value_type> buckets;
            alignas(alignment<double>)
            double sum;
            // Instead of summing the values for each bucket, we use a single counter for the buffer,
            // as this is a compromise solution for case involving hundreds of buckets.
            // With disabled atomic policy, we don't need this.
            [[no_unique_address]] alignas(alignment<buffer_count_type>)
            buffer_count_type count;

            histogram_buffer() = delete;

            histogram_buffer(std::size_t bucket_count)
            : buckets(bucket_count), sum(), count() {}
        };

        static constexpr std::size_t buffers_count = AtomicPolicyType::enabled ? 2 : 1;
        // Active and passive buffers for bucket values
        std::array<histogram_buffer, buffers_count> buffers_;

        [[no_unique_address]] alignas(alignment<count_type>)
        count_type count_and_idx_;

        [[no_unique_address]] alignas(alignment<mutex_ptr_type>)
        mutex_ptr_type mtx_;

        // Serialized names
        std::vector<std::string_view> bucket_names_svs_;
        std::string_view sum_name_sv_;
        std::string_view count_name_sv_;

        decltype(buffers_) init_buffers(std::size_t bucket_count)
        {
            if constexpr (buffers_count == 2)
            {
                return { bucket_count, bucket_count };
            }
            else
            {
                return { bucket_count };
            }
        }

        histogram_entry(std::size_t bucket_count)
        : buffers_(init_buffers(bucket_count)), count_and_idx_()
        {
            if constexpr (AtomicPolicyType::enabled)
            {
                mtx_ = std::make_unique<std::mutex>();
            }
        }

        void observe(double value, std::size_t count, std::size_t bucket_idx)
        {
            if constexpr (AtomicPolicyType::enabled)
            {
                std::atomic_ref<count_type> atomic_count_and_idx(count_and_idx_);
                // Get current buffer index
                std::uint64_t n = atomic_count_and_idx.fetch_add(count, std::memory_order_acquire);
                std::size_t buffer_idx = static_cast<std::size_t>(n >> 63);
                auto& buffer = buffers_[buffer_idx];
                buffer.buckets[bucket_idx].value.fetch_add(count, std::memory_order_relaxed);
                std::atomic_ref<double> atomic_sum(buffer.sum);
                atomic_sum.fetch_add(value * count, std::memory_order_relaxed);
                // Commit changes
                std::atomic_ref<std::uint64_t> atomic_count(buffer.count);
                atomic_count.fetch_add(count, std::memory_order_release);
            }
            else
            {
                auto& buffer = buffers_[0];
                buffer.buckets[bucket_idx].value += count;
                buffer.sum += value * count;
            }
        }

        consteval static bool is_atomic()
        {
            return AtomicPolicyType::enabled;
        }
    };

    std::string_view prometheus_header_;
    std::string_view openmetrics_header_;
    boost::container::small_vector<histogram_entry, 1> entries_;

    static_histogram_internal(std::string_view prometheus_header, std::string_view openmetrics_header) :
        prometheus_header_(prometheus_header),
        openmetrics_header_(openmetrics_header) {}

    std::size_t size() const noexcept
    {
        return entries_.size();
    }

    consteval static bool is_atomic()
    {
        return AtomicPolicyType::enabled;
    }

    // Returns index of an entry
    std::size_t add_entry(std::size_t bucket_count)
    {
        entries_.push_back(histogram_entry(bucket_count));
        return entries_.size() - 1;
    }

    histogram_entry& value_ref(std::size_t index)
    {
        return entries_[index];
    }
};
BOOST_PROMETHEUS_MSVC_RESTORE_WARNING

} // namespace boost::prometheus::detail

#endif
