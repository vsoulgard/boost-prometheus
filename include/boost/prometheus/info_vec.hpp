//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_INFO_VEC_HPP
#define BOOST_PROMETHEUS_INFO_VEC_HPP

#include <boost/prometheus/info_item.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/basic_metric_vec.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <boost/prometheus/detail/export/serialize_name_with_labels.hpp>

namespace boost::prometheus {

template <typename AtomicPolicyType = default_atomic_policy>
class info_vec : public detail::basic_metric_vec<
    info_vec<AtomicPolicyType>,
    info_item<AtomicPolicyType>,
    info_opts
>
{
    using item_type = info_item<AtomicPolicyType>;
    using base_type = detail::basic_metric_vec<
        info_vec<AtomicPolicyType>,
        item_type,
        info_opts
    >;
    friend base_type;

    // Serialization
    friend class boost::prometheus::format;

    // Called in base_type::with_labels_impl
    template <typename LabelsType, typename LabelValuesType>
    std::string init_params(std::string_view metric_name, const LabelsType& labels, const LabelValuesType& labelvalues)
    {
        return detail::serialize_name_with_labels<"_info">(
            metric_name,
            labels,
            labelvalues
        );
    }

public:

    info_vec(std::string_view name, info_opts&& options, std::span<const std::string_view> labels)
    : base_type(name, std::move(options), labels) {}

    info_vec(std::string_view name, info_opts&& options, std::span<const std::string> labels)
    : base_type(name, std::move(options), labels) {}

    info_vec(std::string_view name, info_opts&& options, std::vector<std::string>&& labels)
    : base_type(name, std::move(options), std::move(labels)) {}

    info_vec(std::string_view name, info_opts&& options, std::initializer_list<std::string_view> labels)
    : info_vec(name, std::move(options), std::span(labels)) {}

    info_vec(std::string_view name, std::span<const std::string_view> labels)
    : base_type(name, labels) {}

    info_vec(std::string_view name, std::span<const std::string> labels)
    : base_type(name, labels) {}

    info_vec(std::string_view name, std::vector<std::string>&& labels)
    : base_type(name, std::move(labels)) {}

    info_vec(std::string_view name, std::initializer_list<std::string_view> labels)
    : info_vec(name, std::span(labels)) {}
};

} // namespace boost::prometheus

#endif
