//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_STATIC_GAUGE_HPP
#define BOOST_PROMETHEUS_STATIC_GAUGE_HPP

#include <boost/prometheus/detail/basic_static_metric.hpp>
#include <boost/prometheus/detail/friend_registration.hpp>
#include <boost/prometheus/detail/static_gauge_internal.hpp>
#include <boost/prometheus/detail/static_metric_registrator.hpp>
#include <boost/prometheus/detail/string_buffer.hpp>

#include <atomic>

namespace boost::prometheus {

/**
 * \brief Compile-time gauge with label values validated by the compiler.
 *
 * A `static_gauge` is a gauge whose name and label names are fixed at
 * compile time, and whose label values are checked by the compiler at each
 * call site. There is no runtime lookup of labels, no hashing, and no
 * allocation on the hot path — each `inc<...>()`, `dec<...>()`, or
 * `set<...>()` call compiles down to a direct atomic (or plain) `double`
 * update on pre-resolved storage.
 *
 * The series is registered with a `static_registry` at first use. The
 * exposition text (name, `# HELP`, `# TYPE`, label formatting) is
 * serialized at compile time.
 *
 * Unlike \ref static_counter, the exposition name is emitted as-is — no
 * suffix is appended automatically.
 *
 * \tparam MetricName Metric name as an NTTP, e.g. `"applications_active"`.
 * \tparam Options    Zero or more configuration types:
 *                   \ref help, \ref unit, \ref labels, \ref atomic_policy.
 *                   Order does not matter; when `atomic_policy` is omitted,
 *                   `default_atomic_policy` is used.
 *
 * \par Thread safety
 * With the default atomic policy, `inc()`, `dec()`, and `set()` are
 * thread-safe and may be called concurrently from multiple threads,
 * including mixing different operations on the same series. With
 * `atomic_policy<false>`, updates are not synchronized and must be
 * serialized by the caller.
 *
 * \par Example
 * \code
 * using namespace boost::prometheus;
 *
 * using active = static_gauge<
 *     "applications_active",
 *     help<"Number of active applications">,
 *     labels<"state">>;
 *
 * active::set<"running">(42.0);
 * active::inc<"idle">();
 * active::inc<"idle">();
 * active::dec<"running">(2.0);
 *
 * double v = active::get_value<"running">();  // 40.0
 * \endcode
 *
 * \note All members are static: a `static_gauge` is never instantiated.
 *       It serves purely as a compile-time handle for a specific metric.
 *
 * \sa static_counter, static_histogram, static_info, static_registry
 */
template <detail::string_buffer MetricName, typename... Options>
class static_gauge : public detail::basic_static_metric<
    static_gauge,
    detail::static_gauge_internal,
    MetricName,
    Options...
>
{
    using base_type = detail::basic_static_metric<
        static_gauge,
        detail::static_gauge_internal,
        MetricName,
        Options...
    >;

    BOOST_PROMETHEUS_FRIEND_REGISTRATION()

public:

    /**
     * \brief Returns the current value of the gauge.
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
     * \brief Adds \p value to the gauge.
     *
     * \tparam LabelValues Label values, matching the `labels<...>` option
     *         in the same order. Checked at compile time.
     * \param value Amount to add. Negative values decrease the gauge.
     */
    template <detail::string_buffer... LabelValues>
    static void inc(double value = 1.0)
    {
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

    /**
     * \brief Subtracts \p value from the gauge.
     *
     * \tparam LabelValues Label values, matching the `labels<...>` option
     *         in the same order. Checked at compile time.
     * \param value Amount to subtract. Negative values increase the gauge.
     */
    template <detail::string_buffer... LabelValues>
    static void dec(double value = 1.0)
    {
        double& value_ref = base_type::template value_ref<LabelValues...>();

        if constexpr (base_type::is_atomic())
        {
            std::atomic_ref<double> atomic_value(value_ref);
            atomic_value.fetch_sub(value, std::memory_order_relaxed);
        }
        else
        {
            value_ref -= value;
        }
    }

    /**
     * \brief Sets the gauge to \p value.
     *
     * \tparam LabelValues Label values, matching the `labels<...>` option
     *         in the same order. Checked at compile time.
     * \param value New value, replacing the current one.
     */
    template <detail::string_buffer... LabelValues>
    static void set(double value)
    {
        double& value_ref = base_type::template value_ref<LabelValues...>();

        if constexpr (base_type::is_atomic())
        {
            std::atomic_ref<double> atomic_value(value_ref);
            atomic_value.store(value, std::memory_order_relaxed);
        }
        else
        {
            value_ref = value;
        }
    }
};

} // namespace boost::prometheus

#endif
