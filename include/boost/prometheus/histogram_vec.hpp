//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_HISTOGRAM_VEC_HPP
#define BOOST_PROMETHEUS_HISTOGRAM_VEC_HPP

#include <boost/prometheus/histogram_item.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/basic_metric_vec.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <boost/prometheus/detail/export/serialize_histogram_names_with_labels.hpp>

namespace boost::prometheus {

template <typename AtomicPolicyType = default_atomic_policy>
class histogram_vec : public detail::basic_metric_vec<
    histogram_vec<AtomicPolicyType>,
    histogram_item<AtomicPolicyType>,
    histogram_opts
>
{
    using item_type = histogram_item<AtomicPolicyType>;
    using base_type = detail::basic_metric_vec<
        histogram_vec<AtomicPolicyType>,
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
    detail::histogram_item_params init_params(
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
            detail::serialize_histogram_names_with_labels(
            metric_name,
            bucket_boundaries,
            labels,
            labelvalues
        );
        return {
            bucket_boundaries,
            std::move(base_string),
            std::move(buckets_offsets),
            std::move(sum_offset),
            std::move(count_offset)
        };
    }

public:

    histogram_vec(std::string_view name, histogram_opts&& options, std::span<const std::string_view> labels)
    : base_type(name, std::move(options), labels)
    {
        init(std::move(options));
    }

    histogram_vec(std::string_view name, histogram_opts&& options, std::span<const std::string> labels)
    : base_type(name, std::move(options), labels)
    {
        init(std::move(options));
    }

    histogram_vec(std::string_view name, histogram_opts&& options, std::vector<std::string>&& labels)
    : base_type(name, std::move(options), std::move(labels))
    {
        init(std::move(options));
    }

    histogram_vec(std::string_view name, histogram_opts&& options, std::initializer_list<std::string_view> labels)
    : histogram_vec(name, std::move(options), std::span(labels)) {}

    histogram_vec(std::string_view name, std::span<const std::string_view> labels)
    : base_type(name, labels) {}

    histogram_vec(std::string_view name, std::span<const std::string> labels)
    : base_type(name, labels) {}

    histogram_vec(std::string_view name, std::vector<std::string>&& labels)
    : base_type(name, std::move(labels)) {}

    histogram_vec(std::string_view name, std::initializer_list<std::string_view> labels)
    : histogram_vec(name, std::span(labels)) {}
};

} // namespace boost::prometheus

#endif
