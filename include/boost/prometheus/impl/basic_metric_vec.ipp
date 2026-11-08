//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_IMPL_BASIC_METRIC_VEC_IPP
#define BOOST_PROMETHEUS_IMPL_BASIC_METRIC_VEC_IPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/basic_metric_vec.hpp>

namespace boost::prometheus::detail {

template <typename AtomicPolicy, typename DerivedType, typename ItemType, typename OptionsType>
template <typename SpanType>
void basic_metric_vec<AtomicPolicy, DerivedType, ItemType, OptionsType>::
init(std::string_view name, OptionsType&& options, SpanType&& labels)
{
    BOOST_ASSERT_MSG(name.size() > 0, "Metric name cannot be empty");
    BOOST_ASSERT_MSG(std::unordered_set<std::string_view>(labels.begin(), labels.end()).size() == labels.size(),
        "Detected duplicate labels");
    BOOST_ASSERT_MSG(labels.size() > 0, "Labels list cannot be empty");

    map_.reserve(map_initial_size);
    labels_.reserve(labels.size());
    for (std::size_t i = 0; i < labels.size(); i++)
    {
        if constexpr (std::is_lvalue_reference_v<SpanType>)
            labels_.emplace_back(labels[i], i);
        else
            labels_.emplace_back(std::move(labels[i]), i);
    }
    // Sort labels
    std::sort(labels_.begin(), labels_.end());
    // Format prefixes
    auto prefixes = serialize_prefix<
        detail::metric_type_string<item_type>::prometheus_type,
        detail::metric_type_string<item_type>::openmetrics_type,
        true, false, true
    >(name, options);
    both_format_prefixes_ = std::move(prefixes.base_string);
    prometheus_header_sv_ = std::string_view(both_format_prefixes_).substr(
        0,
        prefixes.metric_name_offset
    );
    if (prefixes.openmetrics_prefix_offset != 0)
    {
        openmetrics_header_sv_ = std::string_view(both_format_prefixes_).substr(
            prefixes.openmetrics_prefix_offset,
            both_format_prefixes_.size() - prefixes.openmetrics_prefix_offset);
    }
    else
    {
        openmetrics_header_sv_ = prometheus_header_sv_;
    }
    metric_name_sv_ = std::string_view(both_format_prefixes_).substr(
        prefixes.metric_name_offset,
        name.size());
    BOOST_ASSERT(metric_name_sv_ == name);
}

template <typename AtomicPolicy, typename DerivedType, typename ItemType, typename OptionsType>
template <typename SpanType>
ItemType& basic_metric_vec<AtomicPolicy, DerivedType, ItemType, OptionsType>::
with_labels_impl(SpanType&& labelvalues)
{
    BOOST_ASSERT_MSG(labels_.size() == labelvalues.size(),
        "Incorrect label values count");
    if constexpr (AtomicPolicy::enabled)
        return with_labels_atomic_impl(std::forward<SpanType>(labelvalues));
    else
        return with_labels_non_atomic_impl(std::forward<SpanType>(labelvalues));
}

} // namespace boost::prometheus::detail

#ifdef BOOST_PROMETHEUS_SEPARATE_COMPILATION

#include <boost/prometheus/counter_item.hpp>
#include <boost/prometheus/counter_vec.hpp>
#include <boost/prometheus/int_counter_item.hpp>
#include <boost/prometheus/int_counter_vec.hpp>
#include <boost/prometheus/gauge_item.hpp>
#include <boost/prometheus/gauge_vec.hpp>
#include <boost/prometheus/int_gauge_item.hpp>
#include <boost/prometheus/int_gauge_vec.hpp>
#include <boost/prometheus/histogram_item.hpp>
#include <boost/prometheus/histogram_vec.hpp>
#include <boost/prometheus/gaugehistogram_item.hpp>
#include <boost/prometheus/gaugehistogram_vec.hpp>
#include <boost/prometheus/info_item.hpp>
#include <boost/prometheus/info_vec.hpp>

#define BOOST_PROMETHEUS_INSTANTIATE_METRIC_VEC_METHODS(A, D, I, O, Span) \
    template void basic_metric_vec<A, D<A>, I<A>, O>                      \
    ::init(std::string_view , O&&, Span);                                 \
    template I<A>& basic_metric_vec<A, D<A>, I<A>, O>                     \
    ::with_labels_impl(Span);

#define BOOST_PROMETHEUS_INSTANTIATE_METRIC_VEC(D, I, O)                                                              \
    template class basic_metric_vec<atomic_policy<true>, D<atomic_policy<true>>, I<atomic_policy<true>>, O>;          \
    template class basic_metric_vec<atomic_policy<false>, D<atomic_policy<false>>, I<atomic_policy<false>>, O>;       \
    BOOST_PROMETHEUS_INSTANTIATE_METRIC_VEC_METHODS(atomic_policy<true>, D, I, O, std::span<const std::string_view>&) \
    BOOST_PROMETHEUS_INSTANTIATE_METRIC_VEC_METHODS(atomic_policy<true>, D, I, O, std::span<const std::string>&)      \
    BOOST_PROMETHEUS_INSTANTIATE_METRIC_VEC_METHODS(atomic_policy<true>, D, I, O, std::vector<std::string>&&)         \
    BOOST_PROMETHEUS_INSTANTIATE_METRIC_VEC_METHODS(atomic_policy<true>, D, I, O, metric_vec::key_type&&)             \
    BOOST_PROMETHEUS_INSTANTIATE_METRIC_VEC_METHODS(atomic_policy<false>, D, I, O, std::span<const std::string_view>&)\
    BOOST_PROMETHEUS_INSTANTIATE_METRIC_VEC_METHODS(atomic_policy<false>, D, I, O, std::span<const std::string>&)     \
    BOOST_PROMETHEUS_INSTANTIATE_METRIC_VEC_METHODS(atomic_policy<false>, D, I, O, std::vector<std::string>&&)        \
    BOOST_PROMETHEUS_INSTANTIATE_METRIC_VEC_METHODS(atomic_policy<false>, D, I, O, metric_vec::key_type&&)

namespace boost::prometheus::detail {

BOOST_PROMETHEUS_INSTANTIATE_METRIC_VEC(counter_vec, counter_item, metric_opts)
BOOST_PROMETHEUS_INSTANTIATE_METRIC_VEC(int_counter_vec, int_counter_item, metric_opts)
BOOST_PROMETHEUS_INSTANTIATE_METRIC_VEC(gauge_vec, gauge_item, metric_opts)
BOOST_PROMETHEUS_INSTANTIATE_METRIC_VEC(int_gauge_vec, int_gauge_item, metric_opts)
BOOST_PROMETHEUS_INSTANTIATE_METRIC_VEC(histogram_vec, histogram_item, histogram_opts)
BOOST_PROMETHEUS_INSTANTIATE_METRIC_VEC(gaugehistogram_vec, gaugehistogram_item, histogram_opts)
BOOST_PROMETHEUS_INSTANTIATE_METRIC_VEC(info_vec, info_item, info_opts)

} // namespace boost::prometheus::detail

#endif

#endif
