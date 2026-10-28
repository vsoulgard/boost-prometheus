//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_STATIC_COUNTER_HPP
#define BOOST_PROMETHEUS_STATIC_COUNTER_HPP

#include <boost/prometheus/detail/basic_static_metric.hpp>
#include <boost/prometheus/detail/const_find_substr.hpp>
#include <boost/prometheus/detail/friend_registration.hpp>
#include <boost/prometheus/detail/static_gauge_internal.hpp>
#include <boost/prometheus/detail/static_metric_registrator.hpp>
#include <boost/prometheus/detail/string_buffer.hpp>

#include <boost/assert.hpp>

#include <atomic>

namespace boost::prometheus {

/**
 * \brief Compile-time counter with label values validated by the compiler.
 *
 * A `static_counter` is a counter whose name and label names are fixed at
 * compile time, and whose label values are checked by the compiler at each
 * call site. There is no runtime lookup of labels, no hashing, and no
 * allocation on the hot path — each `inc<...>()` call compiles down to a
 * direct atomic (or plain) `double` update on pre-resolved storage.
 *
 * The series is registered with a `static_registry` at first use. The
 * exposition text (name, `# HELP`, `# TYPE`, label formatting) is
 * serialized at compile time.
 *
 * As with dynamic \ref counter, the `_total` suffix is appended
 * automatically in the exposition. Passing a `MetricName` that contains
 * `_total` is a **compile-time error**.
 *
 * \tparam MetricName Base metric name as an NTTP, e.g. `"requests"`.
 *         The `_total` suffix is appended automatically.
 * \tparam Options    Zero or more configuration types:
 *                   \ref help, \ref unit, \ref labels, \ref atomic_policy.
 *                   Order does not matter; when `atomic_policy` is omitted,
 *                   `default_atomic_policy` is used.
 *
 * \par Thread safety
 * With the default atomic policy, `inc()` is thread-safe and may be called
 * concurrently from multiple threads. With `atomic_policy<false>`, updates
 * are not synchronized and must be serialized by the caller.
 *
 * \par Example
 * \code
 * using namespace boost::prometheus;
 *
 * using requests_total = static_counter<
 *     "requests",
 *     help<"Number of received requests">,
 *     labels<"method", "status">>;
 *
 * requests_total::inc<"GET", "200">();
 * requests_total::inc<"GET", "200">(5.0);
 * requests_total::inc<"POST", "500">();
 *
 * // Compile-time error:
 * // requests_total::inc<"GET">(); // wrong number of labels
 *
 * double v = requests_total::get_value<"GET", "200">();  // 6.0
 * \endcode
 *
 * \note All members are static: a `static_counter` is never instantiated.
 *       It serves purely as a compile-time handle for a specific metric.
 *
 * \sa static_gauge, static_histogram, static_info, static_registry
 */
template <detail::string_buffer MetricName, typename... Options>
class static_counter : public detail::basic_static_metric<
    static_counter,
    detail::static_gauge_internal,
    MetricName,
    Options...
>
{
    static_assert(
        !detail::const_find_substr<MetricName, "_total">(),
        "Counter name shouldn't contain \"_total\" suffix; it will be appended automatically");

    using base_type = detail::basic_static_metric<
        static_counter,
        detail::static_gauge_internal,
        MetricName,
        Options...
    >;

    BOOST_PROMETHEUS_FRIEND_REGISTRATION()

public:

    /**
     * \brief Returns the current value of the counter.
     *
     * \tparam LabelValues Label values, matching the `labels<...>` option
     *         in the same order. The number and values are checked at
     *         compile time.
     */
    template <detail::string_buffer... LabelValues>
    static double get_value()
    {
        double& value_ref = base_type::template value_ref<LabelValues...>();

        if constexpr (base_type::is_atomic())
        {
            std::atomic_ref<double> atomic_value(value_ref);
            return atomic_value.load(std::memory_order_relaxed);
        }
        else
        {
            return value_ref;
        }
    }

    /**
     * \brief Adds \p value to the counter.
     *
     * \tparam LabelValues Label values, matching the `labels<...>` option
     *         in the same order. The number and values are checked at
     *         compile time.
     * \param value Amount to add. Must be non-negative; a negative value
     *        trips a `BOOST_ASSERT_MSG` in debug builds.
     */
    template <detail::string_buffer... LabelValues>
    static void inc(double value = 1.0)
    {
        BOOST_ASSERT_MSG(value >= 0.0, "static_counter::inc() accepts only positive values");

        double& value_ref = base_type::template value_ref<LabelValues...>();

        if constexpr (base_type::is_atomic())
        {
            std::atomic_ref<double> atomic_value(value_ref);
            atomic_value.fetch_add(value, std::memory_order_relaxed);
        }
        else
        {
            value_ref += value;
        }
    }
};

} // namespace boost::prometheus

#endif
