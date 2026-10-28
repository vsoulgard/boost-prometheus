//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_GAUGE_ITEM_HPP
#define BOOST_PROMETHEUS_GAUGE_ITEM_HPP

#include <boost/prometheus/detail/gauge_internal.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

namespace boost::prometheus {

/**
 * \brief Element of a \ref gauge_vec family.
 *
 * A `gauge_item` is a gauge bound to a specific label combination resolved
 * by its parent \ref gauge_vec. It writes into the shared storage of the
 * vector, so all series produced by one `gauge_vec` update the same
 * underlying registry entry.
 *
 * Items are **not** constructed directly. Obtain one by calling
 * `with_labels(...)` on the parent `gauge_vec`; the returned reference is
 * stable for the lifetime of the vector and may be cached:
 *
 * \code
 * prometheus::gauge_vec active("applications_active", {}, {"state"});
 *
 * auto& running = active.with_labels({"running"});
 * running.set(42);
 * running.inc();      // 43
 * running.dec(3);     // 40
 * \endcode
 *
 * The item's exposition name is serialized by the parent vector at the
 * time of the first `with_labels()` call, which is what makes repeated
 * updates cheap.
 *
 * \note A `gauge_item` exposes the same operations as a standalone
 *       \ref gauge (`inc()`, `dec()`, `set()`, `get_value()`,
 *       `is_atomic()`), but is not constructible with a plain metric name.
 *       Use `gauge` if you need a gauge outside a vector.
 *
 * \tparam AtomicPolicyType Compile-time atomicity policy, inherited from
 *         the parent `gauge_vec`. The default, `default_atomic_policy`,
 *         makes `inc()`, `dec()`, and `set()` thread-safe.
 *
 * \par Thread safety
 * With the default atomic policy, `inc()`, `dec()`, and `set()` on a
 * resolved item are thread-safe and may be called concurrently from
 * multiple threads, including mixing different operations on the same
 * item. With `atomic_policy<false>`, updates are not synchronized and
 * must be serialized by the caller.
 *
 * Note that item creation — the first `with_labels()` call for a given
 * label combination — is a property of the parent `gauge_vec`, not of the
 * item. Its thread-safety is documented on \ref gauge_vec.
 *
 * \sa gauge_vec, gauge
 */
template <typename AtomicPolicyType>
class gauge_item
{
    using internal_type = detail::gauge_internal<AtomicPolicyType, false>;

    internal_type internal_;

    template <typename, typename, typename>
    friend class detail::basic_metric_vec;

    // Serialization
    friend struct boost::prometheus::format;

public:

    gauge_item() = delete;

    /**
     * \brief Constructs an item from a pre-serialized exposition name.
     *
     * \warning Do not call directly. This constructor is intended for
     *          use by \ref gauge_vec via `with_labels()`; the
     *          `formatted_name` argument must be the fully serialized
     *          exposition name produced by the parent vector's
     *          serialization logic.
     *
     * \param formatted_name Pre-serialized name, e.g.
     *        `applications_active{state="running"}`. Moved into the item.
     */
    gauge_item(std::string&& formatted_name)
    : internal_(std::move(formatted_name)) {}

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
    double get_value() const noexcept
    {
        return internal_.get_value();
    }

    /**
     * \brief Adds \p value to the gauge.
     *
     * \param value Amount to add. Negative values decrease the gauge.
     */
    void inc(double value = 1.0) noexcept
    {
        internal_.inc(value);
    }

    /**
     * \brief Subtracts \p value from the gauge.
     *
     * \param value Amount to subtract. Negative values increase the gauge.
     */
    void dec(double value = 1.0) noexcept
    {
        internal_.dec(value);
    }

    /**
     * \brief Sets the gauge to \p value.
     *
     * \param value New value, replacing the current one.
     */
    void set(double value) noexcept
    {
        internal_.set(value);
    }
};

} // namespace boost::prometheus

#endif
