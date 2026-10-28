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
#include <array>
#include <functional>

namespace boost::prometheus {

/**
 * \brief Compile-time metric help string.
 *
 * Sets the `# HELP` line for a static metric at compile time.
 *
 * \tparam Value Human-readable description of the metric.
 */
template <detail::string_buffer Value>
struct help
{
    static constexpr detail::string_buffer value = Value;
};

/**
 * \brief Compile-time metric unit string.
 *
 * Sets the `# UNIT` line for a static metric at compile time. Must be
 * empty for `static_info`. When non-empty, should be a suffix of the
 * metric name (e.g. `_seconds`, `_celsius`).
 *
 * \tparam Value Unit of measurement, e.g. `"seconds"` or `"bytes"`.
 */
template <detail::string_buffer Value>
struct unit
{
    static constexpr detail::string_buffer value = Value;
};

/**
 * \brief Compile-time label names for a static metric.
 *
 * \tparam Labels Label names, e.g. `"method"`, `"status"`.
 */
template <detail::string_buffer... Labels>
struct labels
{
    // std::tuple rather than std::array because
    // each label name is a distinct type.
    static constexpr auto values = std::make_tuple(Labels...);

    /**
     * \brief Number of labels in this set.
     */
    static consteval std::size_t size()
    {
        return sizeof...(Labels);
    }
};

/**
 * \brief Compile-time bucket boundaries for a static histogram or
 *        gaugehistogram.
 *
 * The `+Inf` bucket is appended automatically during serialization;
 * do not include it explicitly.
 *
 * \tparam Values Upper bounds of the histogram buckets, in ascending order.
 */
template <double... Values>
struct buckets
{
    static constexpr std::array<double, sizeof...(Values)> values = {Values...};

    static_assert(std::is_sorted(values.begin(), values.end(), std::less<>{}),
        "Bucket boundaries must be strictly increasing!");

    static constexpr std::size_t get_bucket_index(double x)
    {
        auto it = std::lower_bound(values.begin(), values.end(), x);
        return std::distance(values.begin(), it);
    }
};

/**
 * \brief Default bucket set used when no explicit `buckets` option is given.
 *
 * Matches the classic Prometheus default histogram buckets, from 5 ms
 * to 10 s.
 */
using default_buckets = buckets<0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0>;

/**
 * \brief Compile-time atomicity policy.
 *
 * Controls whether metric updates use atomic operations. `atomic_policy<true>`
 * is thread-safe but slightly slower per update; `atomic_policy<false>` is
 * faster but only safe from a single thread.
 *
 * \tparam Enabled `true` for thread-safe (atomic) updates, `false` otherwise.
 */
template <bool Enabled>
struct atomic_policy
{
    static constexpr bool enabled = Enabled;
};

/**
 * \brief Default atomicity policy: thread-safe updates.
 *
 * Equivalent to `atomic_policy<true>`. Used when a static metric does not
 * specify a policy explicitly.
 */
using default_atomic_policy = atomic_policy<true>;

/**
 * \brief Default registry name for static metrics.
 */
constexpr detail::string_buffer default_static_registry_name = "";

/**
 * \brief Options shared by all metric types.
 */
struct metric_opts
{
    /**
     * \brief Human-readable description of the metric.
     *
     * Emitted in the `# HELP` line of the exposition format.
     */
    std::string_view help;

    /**
     * \brief Unit of measurement, e.g. "seconds" or "bytes".
     *
     * Optional. When non-empty, emitted in the `# UNIT` line and, for
     * OpenMetrics, appended to the metric name as a suffix.
     */
    std::string_view unit;
};

/**
 * \brief Options specific to histogram metrics.
 */
struct histogram_opts
{
    /**
     * \brief Human-readable description of the metric.
     *
     * Emitted in the `# HELP` line of the exposition format.
     */
    std::string_view help;

    /**
     * \brief Unit of measurement, e.g. "seconds" or "bytes".
     *
     * See \ref metric_opts::unit for details.
     */
    std::string_view unit;

    /**
     * \brief Upper bounds of the histogram buckets, in ascending order.
     *
     * A `+Inf` bucket is appended automatically. When empty, the
     * library's default bucket set is used.
     */
    std::vector<double> buckets;
};

/**
 * \brief Options specific to info metrics.
 *
 * \note Info metric families MUST have an empty unit string. The
 *       `# UNIT` line is not emitted for Info.
 */
struct info_opts
{
    /**
     * \brief Human-readable description of the metric.
     *
     * Emitted in the `# HELP` line of the exposition format.
     */
    std::string_view help;
};

} // namespace boost::prometheus

#endif
