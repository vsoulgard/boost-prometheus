//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_GAUGEHISTOGRAM_VEC_HPP
#define BOOST_PROMETHEUS_GAUGEHISTOGRAM_VEC_HPP

#include <boost/prometheus/gaugehistogram_item.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/basic_metric_vec.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <boost/prometheus/detail/export/serialize_header.hpp>
#include <boost/prometheus/detail/export/serialize_histogram_names_with_labels.hpp>

namespace boost::prometheus {

template <typename AtomicPolicyType = default_atomic_policy>
class gaugehistogram_vec : public detail::basic_metric_vec<
    gaugehistogram_vec<AtomicPolicyType>,
    gaugehistogram_item<AtomicPolicyType>,
    histogram_opts
>
{
    using item_type = gaugehistogram_item<AtomicPolicyType>;
    using base_type = detail::basic_metric_vec<
        gaugehistogram_vec<AtomicPolicyType>,
        item_type,
        histogram_opts
    >;
    friend base_type;

    std::vector<double> bucket_boundaries_;

    // Serialization
    friend class boost::prometheus::format;

    void init(histogram_opts&& options)
    {
        bucket_boundaries_ = std::move(options.buckets);
    }

    // Called in base_type::with_labels_impl
    template <typename LabelsType, typename LabelValuesType>
    detail::gaugehistogram_item_params init_params(
        std::string_view metric_name,
        const LabelsType& labels,
        const LabelValuesType& labelvalues)
    {
        std::span<const double> bucket_boundaries;
        if (!bucket_boundaries_.empty())
            bucket_boundaries = bucket_boundaries_;
        else
            bucket_boundaries = default_buckets::values;
        auto [base_string, buckets_offsets, sum_offset, count_offset] =
            detail::serialize_gaugehistogram_names_with_labels(
            metric_name,
            bucket_boundaries,
            labels,
            labelvalues
        );
        auto sum_header = detail::serialize_header<
            "gauge",
            "gauge",
            "_gsum"
        >(metric_name, histogram_opts{});
        auto count_header = detail::serialize_header<
            "gauge",
            "gauge",
            "_gcount"
        >(metric_name, histogram_opts{});
        return {
            bucket_boundaries,
            std::move(base_string),
            std::move(buckets_offsets),
            std::move(sum_offset),
            std::move(count_offset),
            std::move(sum_header.base_string),
            std::move(count_header.base_string)
        };
    }

public:

    gaugehistogram_vec(std::string_view name, histogram_opts&& options, std::span<const std::string_view> labels)
    : base_type(name, std::move(options), labels)
    {
        init(std::move(options));
    }

    gaugehistogram_vec(std::string_view name, histogram_opts&& options, std::span<const std::string> labels)
    : base_type(name, std::move(options), labels)
    {
        init(std::move(options));
    }

    gaugehistogram_vec(std::string_view name, histogram_opts&& options, std::vector<std::string>&& labels)
    : base_type(name, std::move(options), std::move(labels))
    {
        init(std::move(options));
    }

    gaugehistogram_vec(std::string_view name, histogram_opts&& options, std::initializer_list<std::string_view> labels)
    : gaugehistogram_vec(name, std::move(options), std::span(labels)) {}

    gaugehistogram_vec(std::string_view name, std::span<const std::string_view> labels)
    : base_type(name, labels) {}

    gaugehistogram_vec(std::string_view name, std::span<const std::string> labels)
    : base_type(name, labels) {}

    gaugehistogram_vec(std::string_view name, std::vector<std::string>&& labels)
    : base_type(name, std::move(labels)) {}

    gaugehistogram_vec(std::string_view name, std::initializer_list<std::string_view> labels)
    : gaugehistogram_vec(name, std::span(labels)) {}
};

} // namespace boost::prometheus

#endif
