//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_INFO_ITEM_HPP
#define BOOST_PROMETHEUS_INFO_ITEM_HPP

#include <boost/prometheus/detail/info_internal.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

namespace boost::prometheus {

/**
 * \brief Element of an \ref info_vec family.
 *
 * An `info_item` is an info metric bound to a specific label combination
 * resolved by its parent \ref info_vec. All series produced by one
 * `info_vec` share the same name; items differ only in their label values
 * and the pre-serialized exposition strings.
 *
 * An info metric always has the value `1`; the label values are the
 * payload. Because of this, `info_item` exposes no mutating operations
 * beyond construction — there is no `set()`, `inc()`, or `get_value()`.
 * The series is fully determined when `with_labels(...)` resolves it.
 *
 * Items are **not** constructed directly. Obtain one by calling
 * `with_labels(...)` on the parent `info_vec`; the returned reference is
 * stable for the lifetime of the vector and may be cached:
 *
 * \code
 * prometheus::info_vec build_info(
 *     "build",
 *     {.help = "Build information"},
 *     {"build_type", "version", "commit"});
 *
 * auto& release = build_info.with_labels({"Release", "1.0", "abc123"});
 * // `release` is a stable reference; nothing further to update.
 * \endcode
 *
 * \note `info_item` intentionally provides no mutable API. The label
 *       values are set by the parent vector when the item is created,
 *       and the metric value is always `1`. See
 *       xref:metric_types.adoc#info[the info metric section] for the
 *       rationale.
 *
 * \note Although `info_item` is templated on \p AtomicPolicyType, the
 *       underlying storage is always atomic — info items are cheap and
 *       immutable after construction, so there is no non-atomic variant.
 *       `is_atomic()` reflects the *parent vector's* policy for
 *       consistency, but the actual storage does not vary.
 *
 * \tparam AtomicPolicyType Compile-time atomicity policy, inherited from
 *         the parent `info_vec`. Currently only reported via `is_atomic()`;
 *         it does not change the internal representation.
 *
 * \par Thread safety
 * `info_item` has no mutating operations — once obtained via
 * `with_labels()`, it can be safely read from multiple threads without
 * synchronization. Thread-safety of the first `with_labels()` call for a
 * given label combination is documented on \ref info_vec.
 *
 * \sa info_vec, info
 */
template <typename AtomicPolicyType>
class info_item
{
    using internal_type = detail::info_internal<true>;

    internal_type internal_;

    template <typename, typename, typename>
    friend class detail::basic_metric_vec;

    // Serialization
    friend struct boost::prometheus::format;

public:

    info_item() = delete;

    /**
     * \brief Constructs an item from a pre-serialized exposition name.
     *
     * \warning Do not call directly. This constructor is intended for
     *          use by \ref info_vec via `with_labels()`; the
     *          `formatted_name` argument must be the fully serialized
     *          exposition name produced by the parent vector's
     *          serialization logic.
     *
     * \param formatted_name Pre-serialized name, e.g.
     *        `build_info{build_type="Release",version="1.0"}`. Moved into
     *        the item.
     */
    info_item(std::string&& formatted_name)
    : internal_(std::move(formatted_name)) {}

    /**
     * \brief Reports the parent vector's atomicity policy.
     *
     * \note This reflects `AtomicPolicyType::enabled` verbatim. It does
     *       not indicate whether the internal storage is atomic — for
     *       `info_item`, the storage is always atomic. The method exists
     *       for uniformity with the other `*_item` types.
     */
    consteval static bool is_atomic()
    {
        return AtomicPolicyType::enabled;
    }
};

} // namespace boost::prometheus

#endif
