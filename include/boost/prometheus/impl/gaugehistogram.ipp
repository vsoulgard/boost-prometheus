//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_IMPL_GAUGEHISTOGRAM_IPP
#define BOOST_PROMETHEUS_IMPL_GAUGEHISTOGRAM_IPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/gaugehistogram.hpp>

namespace boost::prometheus {

template <typename AtomicPolicy>
gaugehistogram<AtomicPolicy>::internal_type
gaugehistogram<AtomicPolicy>::init_internal(std::string_view name, const histogram_opts& options)
{
    auto headers = detail::serialize_header<
        detail::metric_type_string<gaugehistogram>::prometheus_type,
        detail::metric_type_string<gaugehistogram>::openmetrics_type
    >(name, options);
    // Fallback 0.0.4 format headers (serialized as gauges)
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

#ifdef BOOST_PROMETHEUS_SEPARATE_COMPILATION

template gaugehistogram<atomic_policy<true>>::internal_type
gaugehistogram<atomic_policy<true>>::init_internal(std::string_view, const histogram_opts&);
template gaugehistogram<atomic_policy<false>>::internal_type
gaugehistogram<atomic_policy<false>>::init_internal(std::string_view, const histogram_opts&);

#endif

} // namespace boost::prometheus

#endif
