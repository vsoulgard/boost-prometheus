//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_COUNTER_VEC_HPP
#define BOOST_PROMETHEUS_COUNTER_VEC_HPP

#include <boost/prometheus/counter_item.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/basic_metric_vec.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <boost/prometheus/detail/export/serialize_name_with_labels.hpp>

namespace boost::prometheus {

template <typename AtomicPolicyType = default_atomic_policy>
class counter_vec : public detail::basic_metric_vec<
    counter_vec<AtomicPolicyType>,
    counter_item<AtomicPolicyType>,
    metric_opts
>
{
    using item_type = counter_item<AtomicPolicyType>;
    using base_type = detail::basic_metric_vec<
        counter_vec<AtomicPolicyType>,
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
        return detail::serialize_counter_name_with_labels(
            metric_name,
            labels,
            labelvalues
        );
    }

public:

    counter_vec(std::string_view name, metric_opts&& options, std::span<const std::string_view> labels)
    : base_type(name, std::move(options), labels)
    {
       BOOST_ASSERT_MSG(name.find("_total") == std::string_view::npos,
            "Counter name shouldn't contain \"_total\" suffix; it will be appended automatically");
    }

    counter_vec(std::string_view name, metric_opts&& options, std::span<const std::string> labels)
    : base_type(name, std::move(options), labels)
    {
       BOOST_ASSERT_MSG(name.find("_total") == std::string_view::npos,
            "Counter name shouldn't contain \"_total\" suffix; it will be appended automatically");
    }

    counter_vec(std::string_view name, metric_opts&& options, std::vector<std::string>&& labels)
    : base_type(name, std::move(options), std::move(labels))
    {
       BOOST_ASSERT_MSG(name.find("_total") == std::string_view::npos,
            "Counter name shouldn't contain \"_total\" suffix; it will be appended automatically");
    }

    counter_vec(std::string_view name, metric_opts&& options, std::initializer_list<std::string_view> labels)
    : counter_vec(name, std::move(options), std::span(labels)) {}

    counter_vec(std::string_view name, std::span<const std::string_view> labels)
    : base_type(name, labels)
    {
       BOOST_ASSERT_MSG(name.find("_total") == std::string_view::npos,
            "Counter name shouldn't contain \"_total\" suffix; it will be appended automatically");
    }

    counter_vec(std::string_view name, std::span<const std::string> labels)
    : base_type(name, labels)
    {
       BOOST_ASSERT_MSG(name.find("_total") == std::string_view::npos,
            "Counter name shouldn't contain \"_total\" suffix; it will be appended automatically");
    }

    counter_vec(std::string_view name, std::vector<std::string>&& labels)
    : base_type(name, std::move(labels))
    {
       BOOST_ASSERT_MSG(name.find("_total") == std::string_view::npos,
            "Counter name shouldn't contain \"_total\" suffix; it will be appended automatically");
    }

    counter_vec(std::string_view name, std::initializer_list<std::string_view> labels)
    : counter_vec(name, std::span(labels)) {}
};

} // namespace boost::prometheus

#endif
