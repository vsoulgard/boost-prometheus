//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_BASIC_METRIC_INTERNAL_HPP
#define BOOST_PROMETHEUS_DETAIL_BASIC_METRIC_INTERNAL_HPP

#include <boost/prometheus/detail/config.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <string>
#include <string_view>
#include <type_traits>
#include <tuple>

namespace boost::prometheus::detail {

template <typename AtomicPolicyType, bool HasBaseString, bool HasPrefixSVs>
struct basic_metric_internal
{
    template <typename T>
    static constexpr std::size_t alignment = AtomicPolicyType::enabled ? cache_line_size : alignof(T);

    using base_string_type = std::conditional_t<HasBaseString, std::string, std::tuple<>>;
    using prometheus_prefix_type = std::conditional_t<
        HasBaseString && HasPrefixSVs, std::string_view, std::tuple<>>;
    using openmetrics_prefix_type = std::conditional_t<
        HasBaseString && HasPrefixSVs, std::string_view, std::tuple<>>;

    // For some metric vectors such as histogram_vec we don't need that
    [[no_unique_address]]
    const base_string_type base_string_;
    // Points to Prometheus 0.0.4 prefix inside of base_string.
    [[no_unique_address]]
    const prometheus_prefix_type prometheus_prefix_sv_;
    // Points to OpenMetrics 1.0 prefix if exists, otherwise
    // points to prometheus prefix.
    [[no_unique_address]]
    const openmetrics_prefix_type openmetrics_prefix_sv_;

    basic_metric_internal() = delete;
    basic_metric_internal(const basic_metric_internal&) = delete;
    basic_metric_internal& operator=(const basic_metric_internal&) = delete;
    basic_metric_internal(basic_metric_internal&&) = delete;
    basic_metric_internal& operator=(basic_metric_internal&&) = delete;

    basic_metric_internal(std::string&& base_string)
    requires (HasBaseString && !HasPrefixSVs)
    : base_string_(std::move(base_string)) {}

    basic_metric_internal(std::string&& base_string, std::size_t openmetrics_prefix_offset)
    requires (HasBaseString && HasPrefixSVs)
    : base_string_(std::move(base_string)),
    prometheus_prefix_sv_(std::string_view(base_string_).substr(0,
        openmetrics_prefix_offset ?
        openmetrics_prefix_offset
        : std::string_view::npos)),
    openmetrics_prefix_sv_(std::string_view(base_string_).substr(
        openmetrics_prefix_offset, base_string_.size() - openmetrics_prefix_offset)) {}

    basic_metric_internal()
    requires (!(HasBaseString && HasPrefixSVs)) {}

    consteval static bool is_atomic()
    {
        return AtomicPolicyType::enabled;
    }
};

} // namespace boost::prometheus::detail

#endif
