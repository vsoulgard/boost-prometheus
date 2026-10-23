//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_EXPORT_SERIALIZE_PREFIX_HPP
#define BOOST_PROMETHEUS_DETAIL_EXPORT_SERIALIZE_PREFIX_HPP

#include <boost/prometheus/detail/string_buffer.hpp>

#include <boost/assert.hpp>

#include <cstring>
#include <string>
#include <utility>

namespace boost::prometheus::detail {

struct serialize_prefix_result
{
    std::string base_string;
    // Points to Prometheus 0.0.4 prefix inside of base_string
    static constexpr std::size_t prometheus_prefix_offset = 0;
    // Points to OpenMetrics 1.0 prefix inside of base_string
    std::size_t openmetrics_prefix_offset;
};

struct serialize_prefix_with_name_result : public serialize_prefix_result
{
    // Points to metric_name inside of base_string
    std::size_t metric_name_offset;
};

template <typename OptionsType>
consteval bool has_unit_member()
{
    if constexpr (requires { std::declval<OptionsType>().unit.empty(); })
        return true;
    else
        return false;
}

// Serialize header + name (optional)
template <
    string_buffer PrometheusTypeStr,
    string_buffer OpenMetricsTypeStr,
    bool IncludePromName = true,
    bool IncludeOMName = true,
    bool AddNameOffset = false,
    string_buffer Suffix = "",
    typename OptionsType
>
auto serialize_prefix(std::string_view name, const OptionsType& options)
{
    BOOST_ASSERT_MSG(name.size() > 0, "Metric name cannot be empty");

    static_assert(!AddNameOffset || IncludePromName);
    using result_type = std::conditional_t<AddNameOffset,
        serialize_prefix_with_name_result,
        serialize_prefix_result>;

    result_type result;

    constexpr std::string_view total_suffix = "_total";
    constexpr std::string_view info_suffix = "_info";
    constexpr bool has_om_type =
        (std::string_view(PrometheusTypeStr) != std::string_view(OpenMetricsTypeStr));
    // A counter is identified by the OpenMetrics type string. The _total suffix
    // applies to the series name in both formats and to the metadata name in
    // Prometheus only (OpenMetrics keeps "# TYPE <name> counter").
    constexpr bool is_counter =
        (std::string_view(OpenMetricsTypeStr) == std::string_view("counter"));
    constexpr bool is_info =
        (std::string_view(OpenMetricsTypeStr) == std::string_view("info"));

    constexpr bool has_unit_m = has_unit_member<OptionsType>();
    const bool has_unit = [&](){
        if constexpr (has_unit_m)
            return !options.unit.empty();
        else
            return false;
    }();
    const bool has_help = !options.help.empty();

    const std::size_t name_size = name.size();
    constexpr std::size_t name_suffix_size = Suffix.size();
    const std::size_t help_size = options.help.size();
    const std::size_t unit_size = [&](){
        if constexpr (has_unit_m)
            return options.unit.size();
        else
            return 0u;
    }();
    const std::string_view prom_type_view(PrometheusTypeStr);
    const std::string_view om_type_view(OpenMetricsTypeStr);
    const std::size_t prom_type_size = prom_type_view.size();
    const std::size_t om_type_size = om_type_view.size();

    // Series name carries "_total" for counters and "_info" for info in both formats.
    const std::size_t series_suffix_size =
        is_counter ? total_suffix.size() :
        is_info ? info_suffix.size() : 0;
    const std::size_t series_name_size = name_size + name_suffix_size + series_suffix_size;

    // Metadata name: Prometheus appends the suffix (both "_total" and "_info"),
    // OpenMetrics keeps the bare name.
    const std::size_t prom_meta_name_size = name_size + name_suffix_size + series_suffix_size;
    const std::size_t om_meta_name_size = name_size + name_suffix_size;

    // Prometheus metadata: TYPE, HELP (UNIT is OpenMetrics-only)
    const std::size_t prom_type_sec = 7 + prom_meta_name_size + 1 + prom_type_size + 1;
    const std::size_t prom_help_sec = has_help ? (7 + prom_meta_name_size + 1 + help_size + 1) : 0;
    const std::size_t prom_base_size = prom_type_sec + prom_help_sec;

    const std::size_t final_name_size = IncludePromName ? (series_name_size + 1) : 0;
    const std::size_t prom_total_size = prom_base_size + final_name_size;
    std::size_t base_string_total_size = prom_total_size;
    // OpenMetrics prefix (only if it actually differs from Prometheus)
    // Differs when the type strings differ, when this is a counter (Prometheus
    // appends "_total" to the metadata name, OpenMetrics does not), or when
    // UNIT is present (OpenMetrics-only section).
    const bool need_separate_om = has_om_type || is_counter || is_info || has_unit;
    if (need_separate_om)
    {
        // OpenMetrics metadata: TYPE, UNIT, HELP
        const std::size_t om_type_sec = 7 + om_meta_name_size + 1 + om_type_size + 1;
        const std::size_t om_unit_sec = has_unit ? (7 + om_meta_name_size + 1 + unit_size + 1) : 0;
        const std::size_t om_help_sec = has_help ? (7 + om_meta_name_size + 1 + help_size + 1) : 0;
        const std::size_t om_base_size = om_type_sec + om_unit_sec + om_help_sec;
        const std::size_t om_final_name_size = IncludeOMName ? (series_name_size + 1) : 0;
        const std::size_t om_total_size = om_base_size + om_final_name_size;
        base_string_total_size += om_total_size;
        result.openmetrics_prefix_offset = prom_total_size;
    }
    else
    {
        result.openmetrics_prefix_offset = 0;
    }

    result.base_string.resize(base_string_total_size);
    char* prom_data = result.base_string.data() + result.prometheus_prefix_offset;
    std::size_t off = 0;

    // TYPE
    std::memcpy(prom_data + off, "# TYPE ", 7);
    off += 7;
    std::memcpy(prom_data + off, name.data(), name_size);
    off += name_size;
    if constexpr (name_suffix_size > 0)
    {
        std::memcpy(prom_data + off, std::string_view(Suffix).data(), name_suffix_size);
        off += name_suffix_size;
    }
    if constexpr (is_counter)
    {
        std::memcpy(prom_data + off, total_suffix.data(), total_suffix.size());
        off += total_suffix.size();
    }
    else if constexpr (is_info)
    {
        std::memcpy(prom_data + off, info_suffix.data(), info_suffix.size());
        off += info_suffix.size();
    }
    std::memcpy(prom_data + off, " ", 1);
    off += 1;
    std::memcpy(prom_data + off, prom_type_view.data(), prom_type_size);
    off += prom_type_size;
    std::memcpy(prom_data + off, "\n", 1);
    off += 1;

    // HELP
    if (has_help)
    {
        std::memcpy(prom_data + off, "# HELP ", 7);
        off += 7;
        std::memcpy(prom_data + off, name.data(), name_size);
        off += name_size;
        if constexpr (name_suffix_size > 0)
        {
            std::memcpy(prom_data + off, std::string_view(Suffix).data(), name_suffix_size);
            off += name_suffix_size;
        }
        if constexpr (is_counter)
        {
            std::memcpy(prom_data + off, total_suffix.data(), total_suffix.size());
            off += total_suffix.size();
        }
        else if constexpr (is_info)
        {
            std::memcpy(prom_data + off, info_suffix.data(), info_suffix.size());
            off += info_suffix.size();
        }
        std::memcpy(prom_data + off, " ", 1);
        off += 1;
        std::memcpy(prom_data + off, options.help.data(), help_size);
        off += help_size;
        std::memcpy(prom_data + off, "\n", 1);
        off += 1;
    }

    BOOST_ASSERT(off == prom_base_size);

    if (need_separate_om)
    {
        char* om_data = result.base_string.data() + result.openmetrics_prefix_offset;
        std::size_t om_off = 0;

        // TYPE (metadata name without "_total")
        std::memcpy(om_data + om_off, "# TYPE ", 7);
        om_off += 7;
        std::memcpy(om_data + om_off, name.data(), name_size);
        om_off += name_size;
        if constexpr (name_suffix_size > 0)
        {
            std::memcpy(prom_data + off, std::string_view(Suffix).data(), name_suffix_size);
            om_off += name_suffix_size;
        }
        std::memcpy(om_data + om_off, " ", 1);
        om_off += 1;
        std::memcpy(om_data + om_off, om_type_view.data(), om_type_size);
        om_off += om_type_size;
        std::memcpy(om_data + om_off, "\n", 1);
        om_off += 1;

        // UNIT
        if constexpr (has_unit_m)
        {
            if (has_unit)
            {
                BOOST_ASSERT_MSG(name.find(options.unit) != std::string_view::npos,
                    "Metric name must include unit");
                std::memcpy(om_data + om_off, "# UNIT ", 7);
                om_off += 7;
                std::memcpy(om_data + om_off, name.data(), name_size);
                om_off += name_size;
                if constexpr (name_suffix_size > 0)
                {
                    std::memcpy(prom_data + off, std::string_view(Suffix).data(), name_suffix_size);
                    om_off += name_suffix_size;
                }
                std::memcpy(om_data + om_off, " ", 1);
                om_off += 1;
                std::memcpy(om_data + om_off, options.unit.data(), unit_size);
                om_off += unit_size;
                std::memcpy(om_data + om_off, "\n", 1);
                om_off += 1;
            }
        }

        // HELP
        if (has_help)
        {
            std::memcpy(om_data + om_off, "# HELP ", 7);
            om_off += 7;
            std::memcpy(om_data + om_off, name.data(), name_size);
            om_off += name_size;
            if constexpr (name_suffix_size > 0)
            {
                std::memcpy(prom_data + off, std::string_view(Suffix).data(), name_suffix_size);
                om_off += name_suffix_size;
            }
            std::memcpy(om_data + om_off, " ", 1);
            om_off += 1;
            std::memcpy(om_data + om_off, options.help.data(), help_size);
            om_off += help_size;
            std::memcpy(om_data + om_off, "\n", 1);
            om_off += 1;
        }

        // Series name (carries "_total" for counters).
        if constexpr (IncludeOMName)
        {
            std::memcpy(om_data + om_off, name.data(), name_size);
            om_off += name_size;
            if constexpr (name_suffix_size > 0)
            {
                std::memcpy(prom_data + off, std::string_view(Suffix).data(), name_suffix_size);
                om_off += name_suffix_size;
            }
            if constexpr (is_counter)
            {
                std::memcpy(om_data + om_off, total_suffix.data(), total_suffix.size());
                om_off += total_suffix.size();
            }
            else if constexpr (is_info)
            {
                std::memcpy(om_data + om_off, info_suffix.data(), info_suffix.size());
                om_off += info_suffix.size();
            }
            std::memcpy(om_data + om_off, " ", 1);
            om_off += 1;
        }

        BOOST_ASSERT(om_off + result.openmetrics_prefix_offset == result.base_string.size());
    }

    // Prometheus series name (carries "_total" for counters and "_info" for info)
    if constexpr (IncludePromName)
    {
        std::memcpy(prom_data + off, name.data(), name_size);
        off += name_size;
        if constexpr (name_suffix_size > 0)
        {
            std::memcpy(prom_data + off, std::string_view(Suffix).data(), name_suffix_size);
            off += name_suffix_size;
        }
        if constexpr (is_counter)
        {
            std::memcpy(prom_data + off, total_suffix.data(), total_suffix.size());
            off += total_suffix.size();
        }
        else if constexpr (is_info)
        {
            std::memcpy(prom_data + off, info_suffix.data(), info_suffix.size());
            off += info_suffix.size();
        }
        std::memcpy(prom_data + off, " ", 1);
        off += 1;
    }

    BOOST_ASSERT(off == prom_total_size);

    if constexpr (AddNameOffset)
    {
        result.metric_name_offset = prom_base_size;
    }
    return result;
}

} // namespace boost::prometheus::detail

#endif
