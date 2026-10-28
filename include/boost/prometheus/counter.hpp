//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_COUNTER_HPP
#define BOOST_PROMETHEUS_COUNTER_HPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/counter_internal.hpp>

#include <boost/prometheus/detail/export/serialize_prefix.hpp>

namespace boost::prometheus {

/**
 * \brief Monotonically increasing metric.
 *
 * A counter can only go up (or be reset to zero on restart). It is the
 * standard metric type for values such as requests served, tasks completed,
 * or errors total.
 *
 * In the exposition output, the `_total` suffix is appended automatically:
 * a counter constructed with the name `requests` is exposed as
 * `requests_total`. Passing a name that already contains `_total` is a
 * precondition violation (checked in debug builds via `BOOST_ASSERT_MSG`).
 *
 * \tparam AtomicPolicyType Compile-time atomicity policy. The default,
 *         `default_atomic_policy` (i.e. `atomic_policy<true>`), makes
 *         `inc()` thread-safe. Use `atomic_policy<false>` to skip the atomic
 *         RMW when updates are externally synchronized.
 *
 * \par Thread safety
 * With the default atomic policy, `inc()` may be called concurrently from
 * multiple threads. With a non-atomic policy, concurrent calls are not
 * synchronized and must be guarded by the caller.
 *
 * \par Example
 * \code
 * prometheus::counter requests("requests", {.help = "Number of received requests"});
 * requests.inc();     // +1
 * requests.inc(5.0);  // +5
 * // Exposed as: requests_total 6
 * \endcode
 */
template <typename AtomicPolicyType = default_atomic_policy>
class counter
{
    using internal_type = detail::counter_internal<AtomicPolicyType>;

    internal_type internal_;

    // Serialization
    friend struct boost::prometheus::format;

    static internal_type init_internal(std::string_view name, const metric_opts& options)
    {
        BOOST_ASSERT_MSG(name.find("_total") == std::string_view::npos,
            "Counter name shouldn't contain \"_total\" suffix; it will be appended automatically");
        auto prefixes = detail::serialize_prefix<
            internal_type::prometheus_type,
            internal_type::openmetrics_type
        >(name, options);
        return internal_type(std::move(prefixes.base_string), prefixes.openmetrics_prefix_offset);
    }

public:

    counter() = delete;

    /**
     * \brief Constructs a counter.
     *
     * The name must not contain the `_total` suffix — it is appended
     * automatically during exposition.
     *
     * \param name    Base metric name, e.g. `"requests"`.
     * \param options Optional `help` and `unit`. See \ref metric_opts.
     *
     * \pre `name` does not contain `_total`.
     */
    counter(std::string_view name, metric_opts&& options = {})
    : internal_(init_internal(name, options)) {}

    /**
     * \brief Whether increments use atomic operations.
     *
     * Mirrors the `enabled` flag of \p AtomicPolicyType.
     */
    consteval static bool is_atomic()
    {
        return internal_type::is_atomic();
    }

    /**
     * \brief Returns the current value of the counter.
     */
    double get_value() const noexcept
    {
        return internal_.get_value();
    }

    /**
     * \brief Adds \p value to the counter.
     *
     * \param value Amount to add. Must be non-negative; passing a negative
     *              value is a precondition violation.
     */
    void inc(double value = 1.0) noexcept
    {
        internal_.inc(value);
    }
};

} // namespace boost::prometheus

#endif
