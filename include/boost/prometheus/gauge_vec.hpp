//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_GAUGE_VEC_HPP
#define BOOST_PROMETHEUS_GAUGE_VEC_HPP

#include <boost/prometheus/gauge_item.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/basic_metric_vec.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <boost/prometheus/detail/export/serialize_name_with_labels.hpp>

namespace boost::prometheus {

template <typename AtomicPolicyType = default_atomic_policy>
class gauge_vec : public detail::basic_metric_vec<
    gauge_vec<AtomicPolicyType>,
    gauge_item<AtomicPolicyType>,
    metric_opts
>
{
    using item_type = gauge_item<AtomicPolicyType>;
    using base_type = detail::basic_metric_vec<
        gauge_vec<AtomicPolicyType>,
        item_type,
        metric_opts
    >;
    friend base_type;

    // Serialization
    friend class boost::prometheus::format;

    // Called in base_type::with_labels_impl
    template <typename LabelsType, typename LabelValuesType>
    std::string init_params(std::string_view metric_name, const LabelsType& labels, const LabelValuesType& labelvalues)
    {
        return detail::serialize_name_with_labels(
            metric_name,
            labels,
            labelvalues
        );
    }

public:

    gauge_vec(std::string_view name, metric_opts&& options, std::span<const std::string_view> labels)
    : base_type(name, std::move(options), labels) {}

    gauge_vec(std::string_view name, metric_opts&& options, std::span<const std::string> labels)
    : base_type(name, std::move(options), labels) {}

    gauge_vec(std::string_view name, metric_opts&& options, std::vector<std::string>&& labels)
    : base_type(name, std::move(options), std::move(labels)) {}

    gauge_vec(std::string_view name, metric_opts&& options, std::initializer_list<std::string_view> labels)
    : gauge_vec(name, std::move(options), std::span(labels)) {}

    gauge_vec(std::string_view name, std::span<const std::string_view> labels)
    : base_type(name, labels) {}

    gauge_vec(std::string_view name, std::span<const std::string> labels)
    : base_type(name, labels) {}

    gauge_vec(std::string_view name, std::vector<std::string>&& labels)
    : base_type(name, std::move(labels)) {}

    gauge_vec(std::string_view name, std::initializer_list<std::string_view> labels)
    : gauge_vec(name, std::span(labels)) {}
};

} // namespace boost::prometheus

#endif
