//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_EXPORT_STATIC_SERIALIZE_HEADER_HPP
#define BOOST_PROMETHEUS_DETAIL_EXPORT_STATIC_SERIALIZE_HEADER_HPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/extract_option.hpp>
#include <boost/prometheus/detail/metric_traits.hpp>
#include <boost/prometheus/detail/string_buffer.hpp>

#include <string_view>

namespace boost::prometheus::detail {

// Compile-time header text format serialization for both OpenMetrics 1.0 and Prometheus 0.0.4
template <typename MetricType, detail::string_buffer MetricName, typename... Options>
class static_serialize_header
{
    using help_type = extract_option<help, Options...>::type;
    using unit_type = extract_option<unit, Options...>::type;

    static constexpr auto series_metric_name = []() consteval
    {
        if constexpr (static_counter_specialization<MetricType>)
            return MetricName + string_buffer("_total");
        else if constexpr (static_info_specialization<MetricType>)
            return MetricName + string_buffer("_info");
        else
            return MetricName;
    }();

    static constexpr auto prom_help_part = []() consteval
    {
        if constexpr (!std::is_void_v<help_type>)
        {
            return string_buffer("# HELP ") + series_metric_name + string_buffer(" ") + help_type::value + string_buffer("\n");
        }
        else
        {
            return string_buffer(""); 
        }
    }();

    static constexpr auto om_help_part = []() consteval
    {
        if constexpr (!std::is_void_v<help_type>)
        {
            return string_buffer("# HELP ") + MetricName + string_buffer(" ") + help_type::value + string_buffer("\n");
        }
        else
        {
            return string_buffer(""); 
        }
    }();

    static constexpr auto prom_type_part = []() consteval
    {
        return string_buffer("# TYPE ") + series_metric_name + string_buffer(" ") + MetricType::prometheus_type + string_buffer("\n");
    }();

    static constexpr auto om_type_part = []() consteval
    {
        return string_buffer("# TYPE ") + MetricName + string_buffer(" ") + MetricType::openmetrics_type + string_buffer("\n");
    }();

    // OpenMetrics # UNIT part
    static constexpr auto om_unit_part = []() consteval
    {
        if constexpr (!std::is_void_v<unit_type>)
        {
            return string_buffer("# UNIT ") + MetricName + string_buffer(" ") + unit_type::value + string_buffer("\n");
        }
        else
        {
            return string_buffer("");
        }
    }();

    template <string_buffer help, string_buffer type, string_buffer unit>
    struct concat
    {
        inline static constexpr auto value = type + unit + help;
    };

public:

    // Using std::string_view instead of detail::string_buffer to optimize value duplication
    inline static constexpr std::string_view prometheus_format = std::string_view(concat<prom_help_part, prom_type_part, "">::value);
    inline static constexpr std::string_view openmetrics_format = std::string_view(concat<om_help_part, om_type_part, om_unit_part>::value);
};

} // namespace boost::prometheus::detail

#endif
