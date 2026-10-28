//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_HISTOGRAM_ITEM_HPP
#define BOOST_PROMETHEUS_HISTOGRAM_ITEM_HPP

#include <boost/prometheus/detail/histogram_internal.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <vector>
#include <string>
#include <span>

namespace boost::prometheus {

namespace detail {

/**
 * \brief Construction parameters for a \ref histogram_item.
 *
 * Produced by \ref histogram_vec and consumed by `histogram_item`'s
 * constructor. Users normally never construct or handle this type directly.
 *
 * \warning `bucket_boundaries` is a non-owning span. It must point to
 *          storage that outlives the resulting `histogram_item` — in
 *          practice, storage owned by the parent `histogram_vec`.
 *          This struct is an implementation detail precisely because it
 *          carries this lifetime requirement; do not construct it
 *          manually.
 */
struct histogram_item_params
{
    /// Bucket boundaries shared with the parent vector.
    std::span<const double> bucket_boundaries;
    /// Pre-serialized exposition name (with labels and bucket offsets).
    std::string base_string;
    /// Offsets of each bucket name in `base_string`.
    std::vector<std::size_t> buckets_offsets;
    /// Offset of the `_sum` name in `base_string`.
    std::size_t sum_offset;
    /// Offset of the `_count` name in `base_string`.
    std::size_t count_offset;
};

} // namespace detail

/**
 * \brief Element of a \ref histogram_vec family.
 *
 * A `histogram_item` is a histogram bound to a specific label combination
 * resolved by its parent \ref histogram_vec. All series produced by one
 * `histogram_vec` share the same bucket boundaries; items differ only in
 * their label values and in the pre-serialized exposition strings.
 *
 * Items are **not** constructed directly. Obtain one by calling
 * `with_labels(...)` on the parent `histogram_vec`; the returned reference
 * is stable for the lifetime of the vector and may be cached:
 *
 * \code
 * prometheus::histogram_vec req_duration(
 *     "request_duration_seconds",
 *     {.buckets = {0.005, 0.01, 0.05, 0.1, 0.5}},
 *     {"method"});
 *
 * auto& get_duration = req_duration.with_labels({"GET"});
 * get_duration.observe(0.01);
 * get_duration.observe(0.05);
 * \endcode
 *
 * \note A `histogram_item` exposes the same observation operation as a
 *       standalone \ref histogram (`observe()`, plus `is_atomic()`), but is
 *       not constructible with a plain metric name. Use `histogram` if you
 *       need a histogram outside a vector.
 *
 * \note **Bucket lifetime.** The item holds a non-owning span to the bucket
 *       boundaries owned by the parent `histogram_vec`. The vector must
 *       outlive the item. This is guaranteed in normal use, because items
 *       are only obtainable through `with_labels()` and live inside the
 *       vector's storage — but it means an item must not be copied or
 *       moved into a context that outlives its vector.
 *
 * \tparam AtomicPolicyType Compile-time atomicity policy, inherited from
 *         the parent `histogram_vec`. The default, `default_atomic_policy`,
 *         makes `observe()` thread-safe.
 *
 * \par Thread safety
 * With the default atomic policy, `observe()` on a resolved item is
 * thread-safe and may be called concurrently from multiple threads. With
 * `atomic_policy<false>`, updates are not synchronized and must be
 * serialized by the caller.
 *
 * Note that item creation — the first `with_labels()` call for a given
 * label combination — is a property of the parent `histogram_vec`, not of
 * the item. Its thread-safety is documented on \ref histogram_vec.
 *
 * \sa histogram_vec, histogram, histogram_item_params
 */
template <typename AtomicPolicyType>
class histogram_item
{
    using internal_type = detail::histogram_internal<AtomicPolicyType, false, false>;

    internal_type internal_;

    template <typename, typename, typename>
    friend class detail::basic_metric_vec;

    // Serialization
    friend struct boost::prometheus::format;

public:

    histogram_item() = delete;

    /**
     * \brief Constructs an item from pre-serialized parameters.
     *
     * \warning Do not call directly. This constructor is intended for
     *          use by \ref histogram_vec via `with_labels()`. The
     *          `bucket_boundaries` field of \p params is stored as a
     *          non-owning span; the referenced storage must outlive the
     *          item.
     *
     * \param params Pre-serialized name, bucket offsets, and a span to
     *        bucket boundaries owned by the parent vector. See
     *        \ref histogram_item_params.
     */
    histogram_item(detail::histogram_item_params&& params)
    : internal_(
        params.bucket_boundaries,
        std::move(params.base_string),
        params.buckets_offsets,
        params.sum_offset,
        params.count_offset
    ) {}

    /**
     * \brief Whether observations use atomic operations.
     *
     * Mirrors the `enabled` flag of \p AtomicPolicyType.
     */
    consteval static bool is_atomic()
    {
        return internal_type::is_atomic();
    }

    /**
     * \brief Records a single observation.
     *
     * Increments the matching `_bucket` counters by \p count, adds
     * `value * count` to `_sum`, and increases `_count` by \p count.
     *
     * \param value Observed value.
     * \param count Multiplicity of the observation. Useful for batch
     *              recording when the same value is observed multiple
     *              times; defaults to 1.
     */
    void observe(double value, std::uint64_t count = 1) noexcept
    {
        internal_.observe(value, count);
    }
};

} // namespace boost::prometheus

#endif
