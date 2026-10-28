//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_STATIC_INFO_HPP
#define BOOST_PROMETHEUS_STATIC_INFO_HPP

#include <boost/prometheus/detail/basic_static_metric.hpp>
#include <boost/prometheus/detail/const_find_substr.hpp>
#include <boost/prometheus/detail/friend_registration.hpp>
#include <boost/prometheus/detail/static_info_internal.hpp>
#include <boost/prometheus/detail/static_metric_registrator.hpp>
#include <boost/prometheus/detail/string_buffer.hpp>

#include <boost/assert.hpp>

namespace boost::prometheus {

/**
 * \brief Compile-time info metric with label values validated by the
 *        compiler.
 *
 * A `static_info` is an info metric whose name and label names are fixed
 * at compile time, and whose label values are checked by the compiler at
 * the call site. An info metric always has the value `1`; the label values
 * are the payload — typically build information, version, commit, or
 * configuration details. There is no mutable state and no observation API.
 *
 * The series is registered with a `static_registry` when `create<...>()` is
 * first called for a given label combination. The exposition text (name,
 * `# HELP`, `# TYPE`, label formatting) is serialized at compile time.
 *
 * In the exposition output, the `_info` suffix is appended automatically:
 * an info metric constructed with the name `build` is exposed as
 * `build_info`.
 *
 * Info metrics **must not** carry a `unit`. Specifying a `unit<...>` option
 * is a **compile-time error**.
 *
 * \tparam MetricName Base metric name as an NTTP, e.g. `"build"`. The
 *         `_info` suffix is appended automatically.
 * \tparam Options    Zero or more configuration types:
 *                   \ref help, \ref labels, \ref atomic_policy.
 *                   Order does not matter; when `atomic_policy` is omitted,
 *                   `default_atomic_policy` is used.
 *
 * \par Thread safety
 * `create<...>()` is thread-safe with the default atomic policy: calling
 * it concurrently from multiple threads for the same label combination
 * results in a single series being created. With `atomic_policy<false>`,
 * concurrent calls are not synchronized and must be serialized by the
 * caller.
 *
 * \par Example
 * \code
 * using namespace boost::prometheus;
 *
 * using build_info = static_info<
 *     "build",
 *     help<"Build information">,
 *     labels<"build_type", "version", "commit">>;
 *
 * build_info::create<"Release", "1.0", "abc123">();
 *
 * // Exposed as:
 * //   build_info{build_type="Release",commit="abc123",version="1.0"} 1
 *
 * // Compile-time errors:
 * // using bad = static_info<"build", unit<"bytes">>;  // info has no unit
 * // build_info::create<"Release">();                 // wrong number of labels
 * \endcode
 *
 * \note All members are static: a `static_info` is never instantiated.
 *       It serves purely as a compile-time handle for a specific metric.
 *
 * \note `static_info` provides no `get_value()` or mutating API. The value
 *       is always `1`, and the label values are fully determined when the
 *       series is created.
 *
 * \sa static_counter, static_gauge, static_histogram, static_registry
 */
template <detail::string_buffer MetricName, typename... Options>
class static_info : public detail::basic_static_metric<
    static_info,
    detail::static_info_internal,
    MetricName,
    Options...
>
{
    static_assert(!detail::const_find_substr<MetricName, "_info">(),
        "Info name shouldn't contain \"_info\" suffix; it will be appended automatically");

    using base_type = detail::basic_static_metric<
        static_info,
        detail::static_info_internal,
        MetricName,
        Options...
    >;

    using unit_type = detail::extract_option<unit, Options...>::type;
    static_assert(std::is_void_v<unit_type>, "Info metric must not have a unit");

    BOOST_PROMETHEUS_FRIEND_REGISTRATION()

public:

    /**
     * \brief Registers the info series for the given label values.
     *
     * Idempotent: calling `create<...>()` again with the same label values
     * is a no-op. The first call for a given label combination registers
     * the series with the `static_registry`.
     *
     * \tparam LabelValues Label values, matching the `labels<...>` option
     *         in the same order. The number and values are checked at
     *         compile time.
     */
    template <detail::string_buffer... LabelValues>
    static void create()
    {
        (void)base_type::template value_ref<LabelValues...>();
    }
};

} // namespace boost::prometheus

#endif
