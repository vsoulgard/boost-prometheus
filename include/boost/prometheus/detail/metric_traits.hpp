//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_IS_METRIC_TRAITS_HPP
#define BOOST_PROMETHEUS_DETAIL_IS_METRIC_TRAITS_HPP

#include <boost/prometheus/detail/string_buffer.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

namespace boost::prometheus::detail {

template <typename T, template <auto, typename...> class Template>
struct static_metric_specialization_impl : std::false_type {};

template <template <auto, typename...> class Template, auto V, typename... Opts>
struct static_metric_specialization_impl <Template<V, Opts...>, Template> : std::true_type {};

template <typename T, template <auto, typename...> class Template>
concept static_metric_specialization = static_metric_specialization_impl<T, Template>::value;

template <typename T, template <typename> class Template>
struct metric_specialization_impl : std::false_type {};

template <template <typename> class Template, typename U>
struct metric_specialization_impl<Template<U>, Template> : std::true_type {};

template <typename T, template <typename> class Template>
concept metric_specialization = metric_specialization_impl<T, Template>::value;

template <typename T>
concept static_counter_specialization = static_metric_specialization<T, static_counter>;

template <typename T>
concept static_gauge_specialization = static_metric_specialization<T, static_gauge>;

template <typename T>
concept static_histogram_specialization = static_metric_specialization<T, static_histogram>;

template <typename T>
concept static_info_specialization = static_metric_specialization<T, static_info>;

template <typename T>
concept static_gauge_internal_specialization = metric_specialization<T, static_gauge_internal>;

template <typename T>
concept static_histogram_internal_specialization = metric_specialization<T, static_histogram_internal>;

template <typename T>
concept static_info_internal_specialization = metric_specialization<T, static_info_internal>;

template <typename T>
concept counter_specialization = metric_specialization<T, counter>;

template <typename T>
concept counter_vec_specialization = metric_specialization<T, counter_vec>;

template <typename T>
concept gauge_specialization = metric_specialization<T, gauge>;

template <typename T>
concept gauge_vec_specialization = metric_specialization<T, gauge_vec>;

template <typename T>
concept histogram_specialization = metric_specialization<T, histogram>;

template <typename T>
concept histogram_vec_specialization = metric_specialization<T, histogram_vec>;

template <typename T>
concept gaugehistogram_specialization = metric_specialization<T, gaugehistogram>;

template <typename T>
concept gaugehistogram_vec_specialization = metric_specialization<T, gaugehistogram_vec>;

template <typename T>
concept info_vec_specialization = metric_specialization<T, info_vec>;

} // namespace boost::prometheus::detail

#endif
