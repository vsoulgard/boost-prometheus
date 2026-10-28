//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_COUNTER_ITEM_HPP
#define BOOST_PROMETHEUS_COUNTER_ITEM_HPP

#include <boost/prometheus/detail/counter_internal.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

namespace boost::prometheus {

/**
 * \brief Element of a \ref counter_vec family.
 *
 * A `counter_item` is a counter bound to a specific label combination
 * resolved by its parent \ref counter_vec. It writes into the shared
 * storage of the vector, so all series produced by one `counter_vec`
 * update the same underlying registry entry.
 *
 * Items are **not** constructed directly. Obtain one by calling
 * `with_labels(...)` on the parent `counter_vec`; the returned reference
 * is stable for the lifetime of the vector and may be cached:
 *
 * \code
 * prometheus::counter_vec requests("requests", {}, {"method"});
 *
 * auto& get_requests = requests.with_labels({"GET"});
 * get_requests.inc();       // cached; no map lookup after the first call
 * get_requests.inc(5.0);
 * \endcode
 *
 * The item's exposition name is serialized by the parent vector at the
 * time of the first `with_labels()` call, which is what makes repeated
 * updates cheap.
 *
 * \tparam AtomicPolicyType Compile-time atomicity policy, inherited from
 *         the parent `counter_vec`. The default, `default_atomic_policy`,
 *         makes `inc()` thread-safe.
 *
 * \par Thread safety
 * Same as `counter_vec`: with the default atomic policy, `inc()` on a
 * resolved item is thread-safe and may be called concurrently from
 * multiple threads. With `atomic_policy<false>`, updates are not
 * synchronized and must be serialized by the caller.
 *
 * \sa counter_vec, counter
 */
template <typename AtomicPolicyType>
class counter_item
{
    using internal_type = detail::counter_internal<AtomicPolicyType, false>;

    internal_type internal_;

    template <typename, typename, typename>
    friend class detail::basic_metric_vec;

    // Serialization
    friend struct boost::prometheus::format;

public:

    counter_item() = delete;

    /**
     * \brief Constructs an item from a pre-serialized exposition name.
     *
     * \warning Do not call directly. This constructor is intended for
     *          use by \ref counter_vec via `with_labels()`; the
     *          `formatted_name` argument must be the fully serialized
     *          exposition name produced by the parent vector's
     *          serialization logic.
     *
     * \param formatted_name Pre-serialized name, e.g.
     *        `requests_total{method="GET"}`. Moved into the item.
     */
    counter_item(std::string&& formatted_name)
    : internal_(std::move(formatted_name)) {}

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
