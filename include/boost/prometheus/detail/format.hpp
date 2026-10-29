//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_FORMAT_HPP
#define BOOST_PROMETHEUS_DETAIL_FORMAT_HPP

#include <boost/prometheus/detail/metric_traits.hpp>

#include <boost/prometheus/info.hpp>

#include <boost/assert.hpp>
#include <boost/core/yield_primitives.hpp>

#include <array>
#include <atomic>
#include <charconv>
#include <cstring>
#include <limits>
#include <string_view>

namespace boost::prometheus {

struct format
{
    template <typename BufferType>
    static void append(std::string_view data, BufferType& out_buffer)
    {
        static_assert(sizeof(typename BufferType::value_type) == 1,
            "Buffer must contain bytes");
        static_assert(std::is_trivially_copyable_v<typename BufferType::value_type>,
            "Buffer value type must be trivially copyable");

        const auto offset = out_buffer.size();
        BOOST_ASSERT_MSG(data.size() <= std::numeric_limits<std::size_t>::max() - offset,
            "append: resulting size overflows std::size_t");
        const auto new_size = offset + data.size();
        if (new_size > out_buffer.capacity())
        {
            // 2x growth factor
            const auto capacity = out_buffer.capacity();
            const std::size_t grown =
            (capacity > std::numeric_limits<std::size_t>::max() / 2)
                ? std::numeric_limits<std::size_t>::max()
                : capacity * 2;
            out_buffer.reserve(grown);
        }
        out_buffer.resize(new_size);
        std::memcpy(out_buffer.data() + offset, data.data(), data.size());
    }

    template <typename BufferType>
    static void append(double data, BufferType& out_buffer)
    {
        constexpr std::size_t max_double_size = 
        std::numeric_limits<double>::max_digits10 +
        std::string_view("-").size() + std::string_view(".").size() +
        std::string_view("e").size() + std::string_view("+").size()
        + 3 // exponent
        ;
        std::array<char, max_double_size> buf;
        auto [ptr, ec] = std::to_chars(
            buf.data(), buf.data() + buf.size(), data
        );
        BOOST_ASSERT(ec == std::errc());
        std::string_view value(buf.data(), ptr - buf.data());
        append(value, out_buffer);
    }

    template <typename BufferType>
    static void append(std::uint64_t data, BufferType& out_buffer)
    {
        constexpr std::size_t max_uint64_size =
            std::numeric_limits<std::uint64_t>::digits10 + 1;
        std::array<char, max_uint64_size> buf;
        auto [ptr, ec] = std::to_chars(
            buf.data(), buf.data() + buf.size(), data
        );
        BOOST_ASSERT(ec == std::errc());
        std::string_view value(buf.data(), ptr - buf.data());
        append(value, out_buffer);
    }

    template <typename Buckets, typename BufferType>
    static std::uint64_t serialize_histogram_buckets_atomic(
        std::span<const std::string_view> bucket_names,
        const Buckets& buckets,
        BufferType& out_buffer)
    {
        std::uint64_t cum_count = 0;
        for (std::size_t i = 0; i < buckets.size(); i++)
        {
            cum_count += buckets[i].value.load(std::memory_order_relaxed);
            append(bucket_names[i], out_buffer);
            append(cum_count, out_buffer);
            append("\n", out_buffer);
        }
        return cum_count;
    }

    template <typename Buckets, typename BufferType>
    static std::uint64_t serialize_histogram_buckets_non_atomic(
        std::span<const std::string_view> bucket_names,
        const Buckets& buckets,
        BufferType& out_buffer)
    {
        std::uint64_t cum_count = 0;
        for (std::size_t i = 0; i < buckets.size(); i++)
        {
            cum_count += buckets[i].value;
            append(bucket_names[i], out_buffer);
            append(cum_count, out_buffer);
            append("\n", out_buffer);
        }
        return cum_count;
    }

    template <typename BufferType>
    static void serialize_histogram_count_and_sum(
        std::string_view sum_name,
        double sum_value,
        std::string_view count_name,
        std::uint64_t count_value,
        BufferType& out_buffer)
    {
        append(sum_name, out_buffer);
        append(sum_value, out_buffer);
        append("\n", out_buffer);
        append(count_name, out_buffer);
        append(count_value, out_buffer);
        append("\n", out_buffer);
    }

    template <typename HistogramBuffer>
    static void reset_histogram_buffer(
        HistogramBuffer& buffer,
        HistogramBuffer& other_buffer,
        double sum_value,
        auto count_value)
    {
        BOOST_ASSERT(other_buffer.buckets.size() == buffer.buckets.size());
        for (std::size_t i = 0; i < buffer.buckets.size(); i++)
        {
            auto value = buffer.buckets[i].value.load(std::memory_order_relaxed);
            other_buffer.buckets[i].value.fetch_add(value, std::memory_order_relaxed);
            buffer.buckets[i].value.store(0, std::memory_order_relaxed);
        }
        other_buffer.sum.fetch_add(sum_value, std::memory_order_relaxed);
        buffer.sum.store(0.0, std::memory_order_relaxed);
        other_buffer.count.fetch_add(count_value, std::memory_order_relaxed);
        buffer.count.store(0, std::memory_order_relaxed);
    }

    template <typename HistogramType, typename BufferType>
    static void serialize_histogram(HistogramType& histogram_internal, BufferType& out_buffer)
    {
        if constexpr (HistogramType::is_atomic())
        {
            std::lock_guard lock(histogram_internal.mtx_);
            // Switch active buffer idx
            std::uint64_t n = histogram_internal.count_and_idx_.fetch_add(std::uint64_t{1} << 63, std::memory_order_acquire);
            std::uint64_t count = n & ((std::uint64_t{1} << 63) - 1);
            std::size_t buffer_idx = static_cast<std::size_t>(n >> 63);
            auto& buffer = histogram_internal.buffers_[buffer_idx];
            // Wait until buffer is ready
            while (count != buffer.count.load(std::memory_order_acquire))
            {
                boost::core::sp_thread_pause();
            }
            double sum_value = buffer.sum.load(std::memory_order_relaxed);
            // Serialize buckets
            BOOST_ASSERT(histogram_internal.bucket_names_svs_.size() == buffer.buckets.size());
            std::uint64_t cum_count = serialize_histogram_buckets_atomic(
                histogram_internal.bucket_names_svs_,
                buffer.buckets,
                out_buffer);
            serialize_histogram_count_and_sum(
                histogram_internal.sum_name_sv_,
                sum_value,
                histogram_internal.count_name_sv_,
                cum_count,
                out_buffer);
            // Update other buffer and reset current buffer
            std::size_t other_buffer_idx = static_cast<std::size_t>((~n) >> 63);
            auto& other_buffer = histogram_internal.buffers_[other_buffer_idx];
            reset_histogram_buffer(buffer, other_buffer, sum_value, count);
        }
        else
        {
            auto& buffer = histogram_internal.buffers_[0];
            // Serialize buckets
            BOOST_ASSERT(histogram_internal.bucket_names_svs_.size() == buffer.buckets.size());
            std::uint64_t cum_count = serialize_histogram_buckets_non_atomic(
                histogram_internal.bucket_names_svs_,
                buffer.buckets,
                out_buffer);
            serialize_histogram_count_and_sum(
                histogram_internal.sum_name_sv_,
                buffer.sum,
                histogram_internal.count_name_sv_,
                cum_count,
                out_buffer);
        }
    }

    template <typename HistogramType, typename BufferType>
    static void serialize_info_item(HistogramType& info_internal, BufferType& out_buffer)
    {
        append(info_internal.base_string_, out_buffer);
        append("1\n", out_buffer);
    }

    template <typename HistogramType, typename BufferType>
    static void serialize_static_histogram(HistogramType& histogram_internal, BufferType& out_buffer)
    {
        if constexpr (HistogramType::is_atomic())
        {
            std::lock_guard lock(*histogram_internal.mtx_);
            // Switch active buffer idx
            std::atomic_ref<std::uint64_t> count_and_idx(histogram_internal.count_and_idx_);
            std::uint64_t n = count_and_idx.fetch_add(std::uint64_t{1} << 63, std::memory_order_acquire);
            std::uint64_t count = n & ((std::uint64_t{1} << 63) - 1);
            std::size_t buffer_idx = static_cast<std::size_t>(n >> 63);
            auto& buffer = histogram_internal.buffers_[buffer_idx];
            // Wait until buffer is ready
            std::atomic_ref<std::uint64_t> atomic_count(buffer.count);
            std::atomic_ref<double> atomic_sum(buffer.sum);
            while (count != atomic_count.load(std::memory_order_acquire))
            {
                boost::core::sp_thread_pause();
            }
            double sum = atomic_sum.load(std::memory_order_relaxed);
            // Serialize buckets
            BOOST_ASSERT(histogram_internal.bucket_names_svs_.size() == buffer.buckets.size());
            std::uint64_t cum_count = 0;
            for (std::size_t i = 0; i < buffer.buckets.size(); i++)
            {
                cum_count += buffer.buckets[i].value.load(std::memory_order_relaxed);
                append(histogram_internal.bucket_names_svs_[i], out_buffer);
                append(cum_count, out_buffer);
                append("\n", out_buffer);
            }
            BOOST_ASSERT(cum_count == atomic_count.load(std::memory_order_relaxed));
            append(histogram_internal.sum_name_sv_, out_buffer);
            append(sum, out_buffer);
            append("\n", out_buffer);
            append(histogram_internal.count_name_sv_, out_buffer);
            append(cum_count, out_buffer);
            append("\n", out_buffer);
            // Update other buffer and reset current buffer
            std::size_t other_buffer_idx = static_cast<std::size_t>((~n) >> 63);
            auto& other_buffer = histogram_internal.buffers_[other_buffer_idx];
            BOOST_ASSERT(other_buffer.buckets.size() == buffer.buckets.size());
            for (std::size_t i = 0; i < buffer.buckets.size(); i++)
            {
                std::uint64_t value = buffer.buckets[i].value.load(std::memory_order_relaxed);
                other_buffer.buckets[i].value.fetch_add(value, std::memory_order_relaxed);
                buffer.buckets[i].value.store(0, std::memory_order_relaxed);
            }
            std::atomic_ref<double> other_sum(other_buffer.sum);
            std::atomic_ref<std::uint64_t> other_count(other_buffer.count);
            other_sum.fetch_add(sum, std::memory_order_relaxed);
            atomic_sum.store(0.0, std::memory_order_relaxed);
            other_count.fetch_add(count, std::memory_order_relaxed);
            atomic_count.store(0, std::memory_order_relaxed);
        }
        else
        {
            auto& buffer = histogram_internal.buffers_[0];
            // Serialize buckets
            BOOST_ASSERT(histogram_internal.bucket_names_svs_.size() == buffer.buckets.size());
            std::uint64_t cum_count = 0;
            for (std::size_t i = 0; i < buffer.buckets.size(); i++)
            {
                cum_count += buffer.buckets[i].value;
                append(histogram_internal.bucket_names_svs_[i], out_buffer);
                append(cum_count, out_buffer);
                append("\n", out_buffer);
            }
            append(histogram_internal.sum_name_sv_, out_buffer);
            append(buffer.sum, out_buffer);
            append("\n", out_buffer);
            append(histogram_internal.count_name_sv_, out_buffer);
            append(cum_count, out_buffer);
            append("\n", out_buffer);
        }
    }

    struct prometheus_0_0_4
    {
        static constexpr std::string_view content_type =
            "text/plain; version=0.0.4; charset=utf-8";

        template <typename GaugehistogramType, typename BufferType>
        static void serialize_gaugehistogram(GaugehistogramType& histogram_internal, BufferType& out_buffer)
        {
            if constexpr (GaugehistogramType::is_atomic())
            {
                std::lock_guard lock(histogram_internal.mtx_);
                // Switch active buffer idx
                std::uint64_t n = histogram_internal.count_and_idx_.fetch_add(std::uint64_t{1} << 63, std::memory_order_acquire);
                std::uint64_t count = n & ((std::uint64_t{1} << 63) - 1);
                std::size_t buffer_idx = static_cast<std::size_t>(n >> 63);
                auto& buffer = histogram_internal.buffers_[buffer_idx];
                // Wait until buffer is ready
                while (count != buffer.count.load(std::memory_order_acquire))
                {
                    boost::core::sp_thread_pause();
                }
                double sum_value = buffer.sum.load(std::memory_order_relaxed);
                // Serialize buckets
                BOOST_ASSERT(histogram_internal.bucket_names_svs_.size() == buffer.buckets.size());
                std::uint64_t cum_count = serialize_histogram_buckets_atomic(
                    histogram_internal.bucket_names_svs_,
                    buffer.buckets,
                    out_buffer);
                // Serialize sum and count as gauges
                append(histogram_internal.sum_header, out_buffer);
                append(histogram_internal.sum_name_sv_, out_buffer);
                append(sum_value, out_buffer);
                append("\n", out_buffer);
                append(histogram_internal.count_header, out_buffer);
                append(histogram_internal.count_name_sv_, out_buffer);
                append(cum_count, out_buffer);
                append("\n", out_buffer);
                // Update other buffer and reset current buffer
                std::size_t other_buffer_idx = static_cast<std::size_t>((~n) >> 63);
                auto& other_buffer = histogram_internal.buffers_[other_buffer_idx];
                reset_histogram_buffer(buffer, other_buffer, sum_value, count);
            }
            else
            {
                auto& buffer = histogram_internal.buffers_[0];
                // Serialize buckets
                BOOST_ASSERT(histogram_internal.bucket_names_svs_.size() == buffer.buckets.size());
                std::uint64_t cum_count = serialize_histogram_buckets_non_atomic(
                    histogram_internal.bucket_names_svs_,
                    buffer.buckets,
                    out_buffer);
                // Serialize sum and count as gauges
                append(histogram_internal.sum_header, out_buffer);
                append(histogram_internal.sum_name_sv_, out_buffer);
                append(buffer.sum, out_buffer);
                append("\n", out_buffer);
                append(histogram_internal.count_header, out_buffer);
                append(histogram_internal.count_name_sv_, out_buffer);
                append(cum_count, out_buffer);
                append("\n", out_buffer);
            }
        }

        // Only for gaugehistogram_vec items
        template <typename GaugehistogramType, typename BufferType>
        static void serialize_gaugehistogram_item(
            GaugehistogramType& histogram_internal,
            BufferType& out_buffer,
            std::string& sums,
            std::string& counts)
        {
            if constexpr (GaugehistogramType::is_atomic())
            {
                std::lock_guard lock(histogram_internal.mtx_);
                // Switch active buffer idx
                std::uint64_t n = histogram_internal.count_and_idx_.fetch_add(std::uint64_t{1} << 63, std::memory_order_acquire);
                std::uint64_t count = n & ((std::uint64_t{1} << 63) - 1);
                std::size_t buffer_idx = static_cast<std::size_t>(n >> 63);
                auto& buffer = histogram_internal.buffers_[buffer_idx];
                // Wait until buffer is ready
                while (count != buffer.count.load(std::memory_order_acquire))
                {
                    boost::core::sp_thread_pause();
                }
                double sum_value = buffer.sum.load(std::memory_order_relaxed);
                // Serialize buckets
                BOOST_ASSERT(histogram_internal.bucket_names_svs_.size() == buffer.buckets.size());
                std::uint64_t cum_count = serialize_histogram_buckets_atomic(
                    histogram_internal.bucket_names_svs_,
                    buffer.buckets,
                    out_buffer);
                // Serialize sum and count to independent buffers
                append(histogram_internal.sum_name_sv_, sums);
                append(sum_value, sums);
                append("\n", sums);
                append(histogram_internal.count_name_sv_, counts);
                append(cum_count, counts);
                append("\n", counts);
                // Update other buffer and reset current buffer
                std::size_t other_buffer_idx = static_cast<std::size_t>((~n) >> 63);
                auto& other_buffer = histogram_internal.buffers_[other_buffer_idx];
                reset_histogram_buffer(buffer, other_buffer, sum_value, count);
            }
            else
            {
                auto& buffer = histogram_internal.buffers_[0];
                // Serialize buckets
                BOOST_ASSERT(histogram_internal.bucket_names_svs_.size() == buffer.buckets.size());
                std::uint64_t cum_count = serialize_histogram_buckets_non_atomic(
                    histogram_internal.bucket_names_svs_,
                    buffer.buckets,
                    out_buffer);
                // Serialize sum and count to independent buffers
                append(histogram_internal.sum_name_sv_, sums);
                append(buffer.sum, sums);
                append("\n", sums);
                append(histogram_internal.count_name_sv_, counts);
                append(cum_count, counts);
                append("\n", counts);
            }
        }

        template <typename MetricType, typename BufferType> requires (
            prometheus::detail::counter_specialization<MetricType> ||
            prometheus::detail::gauge_specialization<MetricType>)
        static void serialize(MetricType& metric, BufferType& out_buffer)
        {
            append(metric.internal_.prometheus_prefix_sv_, out_buffer);
            append(metric.get_value(), out_buffer);
            append("\n", out_buffer);
        }

        template <typename MetricType, typename BufferType> requires (
            prometheus::detail::histogram_specialization<MetricType>)
        static void serialize(MetricType& metric, BufferType& out_buffer)
        {
            append(metric.internal_.prometheus_prefix_sv_, out_buffer);
            serialize_histogram(metric.internal_, out_buffer);
        }

        template <typename MetricType, typename BufferType> requires (
            prometheus::detail::gaugehistogram_specialization<MetricType>)
        static void serialize(MetricType& metric, BufferType& out_buffer)
        {
            append(metric.internal_.prometheus_prefix_sv_, out_buffer);
            serialize_gaugehistogram(metric.internal_, out_buffer);
        }

        template <typename BufferType>
        static void serialize(info& metric, BufferType& out_buffer)
        {
            append(metric.internal_.prometheus_prefix_sv_, out_buffer);
            append(metric.series_name_, out_buffer);
            append("1\n", out_buffer);
        }

        template <typename MetricVecType, typename BufferType> requires(
            prometheus::detail::counter_vec_specialization<MetricVecType> ||
            prometheus::detail::gauge_vec_specialization<MetricVecType>)
        static void serialize(MetricVecType& metric_vec, BufferType& out_buffer)
        {
            if (metric_vec.size() == 0) return;
            // Header first, then all metrics in family
            append(metric_vec.prometheus_header_sv_, out_buffer);
            metric_vec.for_each([&](auto& item){
                // Metric name
                append(item.second.internal_.base_string_, out_buffer);
                append(item.second.get_value(), out_buffer);
                append("\n", out_buffer);
            });
        }

        template <typename MetricVecType, typename BufferType> requires(
            prometheus::detail::histogram_vec_specialization<MetricVecType>)
        static void serialize(MetricVecType& metric_vec, BufferType& out_buffer)
        {
            if (metric_vec.size() == 0) return;
            // Header first, then all metrics in family
            append(metric_vec.prometheus_header_sv_, out_buffer);
            metric_vec.for_each([&](auto& item){
                serialize_histogram(item.second.internal_, out_buffer);
            });
        }

        template <typename MetricVecType, typename BufferType> requires(
            prometheus::detail::gaugehistogram_vec_specialization<MetricVecType>)
        static void serialize(MetricVecType& metric_vec, BufferType& out_buffer)
        {
            if (metric_vec.size() == 0) return;
            // Header first, then all metrics in family
            append(metric_vec.prometheus_header_sv_, out_buffer);
            // TODO: Refactor and remove these independent buffers
            std::string counts;
            std::string sums;
            counts.reserve(1024);
            sums.reserve(1024);
            metric_vec.for_each([&](auto& item){
                if (counts.empty())
                {
                    append(item.second.internal_.sum_header, sums);
                    append(item.second.internal_.count_header, counts);
                }
                serialize_gaugehistogram_item(item.second.internal_, out_buffer, sums, counts);
            });
            append(sums, out_buffer);
            append(counts, out_buffer);
        }

        template <typename MetricVecType, typename BufferType> requires(
            prometheus::detail::info_vec_specialization<MetricVecType>)
        static void serialize(MetricVecType& metric_vec, BufferType& out_buffer)
        {
            if (metric_vec.size() == 0) return;
            // Header first, then all metrics in family
            append(metric_vec.prometheus_header_sv_, out_buffer);
            metric_vec.for_each([&](auto& item){
                serialize_info_item(item.second.internal_, out_buffer);
            });
        }

        template <typename RegistryType, typename BufferType, bool>
        static void serialize(RegistryType& registry, BufferType& out_buffer)
        {
            registry.for_each([&](auto& metric)
            {
                serialize(metric, out_buffer);
            });
        }

        template <typename StaticMetricType, typename BufferType> requires(
            prometheus::detail::static_gauge_internal_specialization<StaticMetricType>)
        static void serialize(StaticMetricType& metric, BufferType& out_buffer)
        {
            if (metric.size() == 0) return;
            append(metric.prometheus_header_, out_buffer);
            for (const auto& el : metric.entries_)
            {
                append(el.formatted_name, out_buffer);
                append(el.get_value(), out_buffer);
                append("\n", out_buffer);
            }
        }

        template <typename StaticMetricType, typename BufferType> requires(
            prometheus::detail::static_histogram_internal_specialization<StaticMetricType>)
        static void serialize(StaticMetricType& metric, BufferType& out_buffer)
        {
            if (metric.size() == 0) return;
            append(metric.prometheus_header_, out_buffer);
            for (auto& el : metric.entries_)
            {
                serialize_static_histogram(el, out_buffer);
            }
        }

        template <typename StaticMetricType, typename BufferType> requires(
            prometheus::detail::static_info_internal_specialization<StaticMetricType>)
        static void serialize(StaticMetricType& metric, BufferType& out_buffer)
        {
            if (metric.size() == 0) return;
            append(metric.prometheus_header_, out_buffer);
            for (const auto& el : metric.entries_)
            {
                append(el.formatted_name, out_buffer);
            }
        }

        template <typename StaticRegistryType, typename BufferType, bool>
        static void serialize(BufferType& out_buffer)
        {
            StaticRegistryType::for_each([&](auto& metric)
            {
                serialize(metric, out_buffer);
            });
        }
    };

    struct openmetrics_1_0
    {
        static constexpr std::string_view content_type =
            "application/openmetrics-text; version=1.0.0; charset=utf-8";

        template <typename MetricType, typename BufferType> requires (
            prometheus::detail::counter_specialization<MetricType> ||
            prometheus::detail::gauge_specialization<MetricType>)
        static void serialize(MetricType& metric, BufferType& out_buffer)
        {
            append(metric.internal_.openmetrics_prefix_sv_, out_buffer);
            append(metric.get_value(), out_buffer);
            append("\n", out_buffer);
        }

        template <typename MetricType, typename BufferType> requires (
            prometheus::detail::histogram_specialization<MetricType> ||
            prometheus::detail::gaugehistogram_specialization<MetricType>)
        static void serialize(MetricType& metric, BufferType& out_buffer)
        {
            append(metric.internal_.openmetrics_prefix_sv_, out_buffer);
            serialize_histogram(metric.internal_, out_buffer);
        }

        template <typename BufferType>
        static void serialize(info& metric, BufferType& out_buffer)
        {
            append(metric.internal_.openmetrics_prefix_sv_, out_buffer);
            append(metric.series_name_, out_buffer);
            append("1\n", out_buffer);
        }

        template <typename MetricVecType, typename BufferType> requires(
            prometheus::detail::gauge_vec_specialization<MetricVecType> ||
            prometheus::detail::counter_vec_specialization<MetricVecType>)
        static void serialize(MetricVecType& metric_vec, BufferType& out_buffer)
        {
            if (metric_vec.size() == 0) return;
            // Header first, then all metrics in family
            append(metric_vec.openmetrics_header_sv_, out_buffer);
            metric_vec.for_each([&](auto& item){
                // Metric name
                append(item.second.internal_.base_string_, out_buffer);
                append(item.second.get_value(), out_buffer);
                append("\n", out_buffer);
            });
        }

        template <typename MetricVecType, typename BufferType> requires(
            prometheus::detail::histogram_vec_specialization<MetricVecType> ||
            prometheus::detail::gaugehistogram_vec_specialization<MetricVecType>)
        static void serialize(MetricVecType& metric_vec, BufferType& out_buffer)
        {
            if (metric_vec.size() == 0) return;
            // Header first, then all metrics in family
            append(metric_vec.openmetrics_header_sv_, out_buffer);
            metric_vec.for_each([&](auto& item){
                serialize_histogram(item.second.internal_, out_buffer);
            });
        }

        template <typename MetricVecType, typename BufferType> requires(
            prometheus::detail::info_vec_specialization<MetricVecType>)
        static void serialize(MetricVecType& metric_vec, BufferType& out_buffer)
        {
            if (metric_vec.size() == 0) return;
            // Header first, then all metrics in family
            append(metric_vec.openmetrics_header_sv_, out_buffer);
            metric_vec.for_each([&](auto& item){
                serialize_info_item(item.second.internal_, out_buffer);
            });
        }

        template <typename RegistryType, typename BufferType, bool AppendEOF>
        static void serialize(RegistryType& registry, BufferType& out_buffer)
        {
            registry.for_each([&](auto& metric)
            {
                serialize(metric, out_buffer);
            });
            if constexpr (AppendEOF)
                append("# EOF\n", out_buffer);
        }

        template <typename StaticMetricType, typename BufferType> requires(
            prometheus::detail::static_gauge_internal_specialization<StaticMetricType>)
        static void serialize(StaticMetricType& metric, BufferType& out_buffer)
        {
            if (metric.size() == 0) return;
            append(metric.openmetrics_header_, out_buffer);
            for (const auto& el : metric.entries_)
            {
                append(el.formatted_name, out_buffer);
                append(el.value, out_buffer);
                append("\n", out_buffer);
            }
        }

        template <typename StaticMetricType, typename BufferType> requires(
            prometheus::detail::static_histogram_internal_specialization<StaticMetricType>)
        static void serialize(StaticMetricType& metric, BufferType& out_buffer)
        {
            if (metric.size() == 0) return;
            append(metric.openmetrics_header_, out_buffer);
            for (auto& el : metric.entries_)
            {
                serialize_static_histogram(el, out_buffer);
            }
        }

        template <typename StaticMetricType, typename BufferType> requires(
            prometheus::detail::static_info_internal_specialization<StaticMetricType>)
        static void serialize(StaticMetricType& metric, BufferType& out_buffer)
        {
            if (metric.size() == 0) return;
            append(metric.openmetrics_header_, out_buffer);
            for (const auto& el : metric.entries_)
            {
                append(el.formatted_name, out_buffer);
            }
        }
        
        template <typename StaticRegistryType, typename BufferType, bool AppendEOF>
        static void serialize(BufferType& out_buffer)
        {
            StaticRegistryType::for_each([&](auto& metric)
            {
                serialize(metric, out_buffer);
            });
            if constexpr (AppendEOF)
                append("# EOF\n", out_buffer);
        }
    };

    template <typename FormatType, typename RegistryType, typename BufferType, bool AppendEOF>
    static void serialize(RegistryType& registry, BufferType& out_buffer)
    {
        FormatType::template serialize<RegistryType, BufferType, AppendEOF>(registry, out_buffer);
    }

    template <typename FormatType, typename StaticRegistryType, typename BufferType, bool AppendEOF>
    static void serialize(BufferType& out_buffer)
    {
        FormatType::template serialize<StaticRegistryType, BufferType, AppendEOF>(out_buffer);
    }
}; // class format

} // namespace boost::prometheus

#endif
