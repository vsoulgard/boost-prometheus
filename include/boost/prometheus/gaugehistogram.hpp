//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_GAUGEHISTOGRAM_HPP
#define BOOST_PROMETHEUS_GAUGEHISTOGRAM_HPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/gaugehistogram_internal.hpp>

#include <boost/prometheus/detail/export/serialize_header.hpp>
#include <boost/prometheus/detail/export/serialize_histogram_names.hpp>

namespace boost::prometheus {

template <typename AtomicPolicyType = default_atomic_policy>
class gaugehistogram
{
    using internal_type = detail::gaugehistogram_internal<AtomicPolicyType>;

    internal_type internal_;

    // Empty if default buckets provided.
    std::vector<double> bucket_boundaries_;

    // Serialization
    friend struct boost::prometheus::format;

    static internal_type init_internal(std::string_view name, const histogram_opts& options)
    {
        auto headers = detail::serialize_header<
            internal_type::prometheus_type,
            internal_type::openmetrics_type
        >(name, options);
        // Fallback 0.0.4 format headers (serialized as gauges)
        histogram_opts header_options;
        auto sum_header = detail::serialize_header<
            "gauge",
            "gauge",
            "_gsum"
        >(name, histogram_opts{});
        auto count_header = detail::serialize_header<
            "gauge",
            "gauge",
            "_gcount"
        >(name, histogram_opts{});

        std::span<const double> bucket_boundaries;
        if (!options.buckets.empty())
        {
            bucket_boundaries = options.buckets;
        }
        else
        {
            bucket_boundaries = default_buckets::values;
        }
        auto histogram_names = detail::serialize_gaugehistogram_names(name, bucket_boundaries);
        return internal_type(
            std::move(headers.base_string),
            headers.openmetrics_prefix_offset,
            bucket_boundaries,
            std::move(histogram_names.base_string),
            histogram_names.buckets_offsets,
            histogram_names.sum_offset,
            histogram_names.count_offset,
            std::move(sum_header.base_string),
            std::move(count_header.base_string)
        );
    }

public:

    gaugehistogram() = delete;

    gaugehistogram(std::string_view name, histogram_opts&& options = {})
    : internal_(init_internal(name, options))
    {
        if (!options.buckets.empty())
        {
            bucket_boundaries_ = std::move(options.buckets);
            // Re-assign std::span
            internal_.bucket_boundaries_ = bucket_boundaries_;
        }
    }

    consteval static bool is_atomic()
    {
        return internal_type::is_atomic();
    }

    void observe(double value, std::uint64_t count = 1) noexcept
    {
        internal_.observe(value, count);
    }

    void sub(double value, std::uint64_t count = 1) noexcept
    {
        internal_.sub(value, count);
    }
};

} // namespace boost::prometheus

#endif
