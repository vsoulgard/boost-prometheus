//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_METRIC_OPTIONS_HPP
#define BOOST_PROMETHEUS_METRIC_OPTIONS_HPP

#include <boost/prometheus/detail/string_buffer.hpp>

#include <algorithm>
#include <functional>

namespace boost::prometheus {

template <detail::string_buffer Value>
struct help
{
    static constexpr detail::string_buffer value = Value;
};

template <detail::string_buffer Value>
struct unit
{
    static constexpr detail::string_buffer value = Value;
};

template <detail::string_buffer... Labels>
struct labels
{
    // Cannot use std::array instead of std::tuple here, because all Labels are different types.
    static constexpr auto values = std::make_tuple(Labels...);

    static consteval std::size_t size()
    {
        return sizeof...(Labels);
    }
};

template <double... Values>
struct buckets
{
    static constexpr std::array<double, sizeof...(Values)> values = {Values...};

    static_assert(std::is_sorted(values.begin(), values.end(), std::less_equal<>{}),
        "Bucket boundaries must be strictly increasing!");

    static constexpr std::size_t get_bucket_index(double x)
    {
        auto it = std::lower_bound(values.begin(), values.end(), x);
        return std::distance(values.begin(), it);
    }
};

using default_buckets = buckets<0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0>;

template <bool Enabled>
struct atomic_policy
{
    static constexpr bool enabled = Enabled;
};

using default_atomic_policy = atomic_policy<true>;

constexpr detail::string_buffer default_static_registry_name = "";

struct metric_opts
{
    std::string_view help;
    std::string_view unit;
};

struct histogram_opts
{
    std::string_view help;
    std::string_view unit;
    std::vector<double> buckets;
};

/// \note MetricFamilies of type Info MUST have an empty Unit string.
struct info_opts
{
    std::string_view help;
};

} // namespace boost::prometheus

#endif
