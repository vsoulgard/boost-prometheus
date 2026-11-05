//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_EXPORT_METRIC_TYPE_STRING_HPP
#define BOOST_PROMETHEUS_DETAIL_EXPORT_METRIC_TYPE_STRING_HPP

#include <boost/prometheus/detail/types_fwd.hpp>
#include <boost/prometheus/detail/string_buffer.hpp>

namespace boost::prometheus::detail {

template <typename T>
struct metric_type_string;

template <typename A>
struct metric_type_string<counter_item<A>>
{
    static constexpr string_buffer prometheus_type  = "counter";
    static constexpr string_buffer openmetrics_type = "counter";
};

template <typename A>
struct metric_type_string<counter<A>>
{
    static constexpr string_buffer prometheus_type  = "counter";
    static constexpr string_buffer openmetrics_type = "counter";
};

template <typename A>
struct metric_type_string<int_counter<A>>
{
    static constexpr string_buffer prometheus_type  = "counter";
    static constexpr string_buffer openmetrics_type = "counter";
};

template <typename A>
struct metric_type_string<gauge_item<A>>
{
    static constexpr string_buffer prometheus_type  = "gauge";
    static constexpr string_buffer openmetrics_type = "gauge";
};

template <typename A>
struct metric_type_string<gauge<A>>
{
    static constexpr string_buffer prometheus_type  = "gauge";
    static constexpr string_buffer openmetrics_type = "gauge";
};

template <typename A>
struct metric_type_string<int_gauge<A>>
{
    static constexpr string_buffer prometheus_type  = "gauge";
    static constexpr string_buffer openmetrics_type = "gauge";
};

template <typename A>
struct metric_type_string<histogram_item<A>>
{
    static constexpr string_buffer prometheus_type  = "histogram";
    static constexpr string_buffer openmetrics_type = "histogram";
};

template <typename A>
struct metric_type_string<histogram<A>>
{
    static constexpr string_buffer prometheus_type  = "histogram";
    static constexpr string_buffer openmetrics_type = "histogram";
};

template <typename A>
struct metric_type_string<gaugehistogram_item<A>>
{
    static constexpr string_buffer prometheus_type  = "histogram";
    static constexpr string_buffer openmetrics_type = "gaugehistogram";
};

template <typename A>
struct metric_type_string<gaugehistogram<A>>
{
    static constexpr string_buffer prometheus_type  = "histogram";
    static constexpr string_buffer openmetrics_type = "gaugehistogram";
};

template <typename A>
struct metric_type_string<info_item<A>>
{
    static constexpr string_buffer prometheus_type = "gauge";
    static constexpr string_buffer openmetrics_type = "info";
};

template <>
struct metric_type_string<info>
{
    static constexpr string_buffer prometheus_type = "gauge";
    static constexpr string_buffer openmetrics_type = "info";
};

template <string_buffer N, typename... O>
struct metric_type_string<static_counter<N, O...>>
{
    static constexpr string_buffer prometheus_type  = "counter";
    static constexpr string_buffer openmetrics_type = "counter";
};

template <string_buffer N, typename... O>
struct metric_type_string<static_int_counter<N, O...>>
{
    static constexpr string_buffer prometheus_type  = "counter";
    static constexpr string_buffer openmetrics_type = "counter";
};

template <string_buffer N, typename... O>
struct metric_type_string<static_gauge<N, O...>>
{
    static constexpr string_buffer prometheus_type  = "gauge";
    static constexpr string_buffer openmetrics_type = "gauge";
};

template <string_buffer N, typename... O>
struct metric_type_string<static_int_gauge<N, O...>>
{
    static constexpr string_buffer prometheus_type  = "gauge";
    static constexpr string_buffer openmetrics_type = "gauge";
};

template <string_buffer N, typename... O>
struct metric_type_string<static_histogram<N, O...>>
{
    static constexpr string_buffer prometheus_type  = "histogram";
    static constexpr string_buffer openmetrics_type = "histogram";
};

template <string_buffer N, typename... O>
struct metric_type_string<static_info<N, O...>>
{
    static constexpr string_buffer prometheus_type = "gauge";
    static constexpr string_buffer openmetrics_type = "info";
};

} // namespace boost::prometheus::detail

#endif
