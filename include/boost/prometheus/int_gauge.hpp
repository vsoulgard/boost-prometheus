//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_INT_GAUGE_HPP
#define BOOST_PROMETHEUS_INT_GAUGE_HPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/gauge_internal.hpp>

#include <boost/prometheus/detail/export/metric_type_string.hpp>
#include <boost/prometheus/detail/export/serialize_prefix.hpp>

namespace boost::prometheus {

/**
 * \brief Metric that can increase or decrease for integer values.
 *
 * A gauge represents a single value that can go up and down over time.
 * Typical uses: current temperature, memory usage, number of in-flight
 * requests, queue size, and any other value where both directions matter.
 *
 * Unlike \ref counter, the exposition name is emitted as-is — no suffix
 * is appended automatically.
 *
 * \tparam AtomicPolicy Compile-time atomicity policy. The default,
 *         `default_atomic_policy` (i.e. `atomic_policy<true>`), makes
 *         `inc()`, `dec()`, and `set()` thread-safe. Use `atomic_policy<false>`
 *         to skip atomic operations when updates are externally synchronized.
 *
 * \par Thread safety
 * With the default atomic policy, `inc()`, `dec()`, and `set()` may be
 * called concurrently from multiple threads. With a non-atomic policy,
 * concurrent calls are not synchronized and must be guarded by the caller.
 *
 * \par Example
 * \code
 * prometheus::int_gauge active("applications_active",
 *     {.help = "Number of active applications"});
 * active.inc();     // +1
 * active.dec();     // -1
 * active.set(100);  // set to 100
 * // Exposed as: applications_active 100
 * \endcode
 */
template <typename AtomicPolicy = default_atomic_policy>
class int_gauge
{
    using internal_type = detail::gauge_internal<std::int64_t, AtomicPolicy, true>;

    internal_type internal_;

    // Serialization
    friend struct boost::prometheus::format;

    static internal_type init_internal(std::string_view name, const metric_opts& options);

public:

    int_gauge() = delete;

    /**
     * \brief Constructs a gauge.
     *
     * \param name    Metric name, e.g. `"applications_active"`.
     * \param options Optional `help` and `unit`. See \ref metric_opts.
     */
    int_gauge(std::string_view name, metric_opts&& options = {})
    : internal_(init_internal(name, options)) {}

    /**
     * \brief Whether updates use atomic operations.
     *
     * Mirrors the `enabled` flag of \p AtomicPolicyType.
     */
    consteval static bool is_atomic()
    {
        return internal_type::is_atomic();
    }

    /**
     * \brief Returns the current value of the gauge.
     */
    std::int64_t get_value() const noexcept
    {
        return internal_.get_value();
    }

    /**
     * \brief Adds \p value to the gauge.
     *
     * \param value Amount to add.
     */
    void inc(std::int64_t value = 1) noexcept
    {
        internal_.inc(value);
    }

    /**
     * \brief Subtracts \p value from the gauge.
     *
     * \param value Amount to subtract.
     */
    void dec(std::int64_t value = 1) noexcept
    {
        internal_.dec(value);
    }

    /**
     * \brief Sets the gauge to \p value.
     *
     * \param value New value, replacing the current one.
     */
    void set(std::int64_t value) noexcept
    {
        internal_.set(value);
    }
};

} // namespace boost::prometheus

#ifdef BOOST_PROMETHEUS_HEADER_ONLY
#include <boost/prometheus/impl/int_gauge.ipp>
#endif

#endif
