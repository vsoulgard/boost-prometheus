//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_GAUGEHISTOGRAM_ITEM_HPP
#define BOOST_PROMETHEUS_GAUGEHISTOGRAM_ITEM_HPP

#include <boost/prometheus/detail/gaugehistogram_internal.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <vector>
#include <string>
#include <span>

namespace boost::prometheus {

namespace detail {

/**
 * \brief Construction parameters for a \ref gaugehistogram_item.
 *
 * Produced by \ref gaugehistogram_vec and consumed by
 * `gaugehistogram_item`'s constructor. Users normally never construct or
 * handle this type directly.
 *
 * \warning `bucket_boundaries` is a non-owning span. It must point to
 *          storage that outlives the resulting `gaugehistogram_item` — in
 *          practice, storage owned by the parent `gaugehistogram_vec`.
 *          This struct is an implementation detail precisely because it
 *          carries this lifetime requirement; do not construct it
 *          manually.
 */
struct gaugehistogram_item_params
{
    /// Bucket boundaries shared with the parent vector.
    std::span<const double> bucket_boundaries;
    /// Pre-serialized exposition name (with labels and bucket offsets).
    std::string base_string;
    /// Offsets of each bucket name in `base_string`.
    std::vector<std::size_t> buckets_offsets;
    /// Offset of the `_gsum` name in `base_string`.
    std::size_t sum_offset;
    /// Offset of the `_gcount` name in `base_string`.
    std::size_t count_offset;
    /// Pre-serialized `_gsum` header for Prometheus 0.0.4 fallback.
    std::string sum_header;
    /// Pre-serialized `_gcount` header for Prometheus 0.0.4 fallback.
    std::string count_header;
};

} // namespace detail

/**
 * \brief Element of a \ref gaugehistogram_vec family.
 *
 * A `gaugehistogram_item` is a gauge histogram bound to a specific label
 * combination resolved by its parent \ref gaugehistogram_vec. All series
 * produced by one `gaugehistogram_vec` share the same bucket boundaries;
 * items differ only in their label values and in the pre-serialized
 * exposition strings.
 *
 * Unlike \ref histogram_item, a gauge histogram item supports removing
 * previously recorded observations via `sub()`. This makes it suitable for
 * tracking instantaneous distributions whose contents change over time.
 *
 * Items are **not** constructed directly. Obtain one by calling
 * `with_labels(...)` on the parent `gaugehistogram_vec`; the returned
 * reference is stable for the lifetime of the vector and may be cached:
 *
 * \code
 * prometheus::gaugehistogram_vec tasks(
 *     "tasks_by_duration_seconds",
 *     {.buckets = {0.5, 1.0, 5.0, 30.0}},
 *     {"worker_pool"});
 *
 * auto& alpha = tasks.with_labels({"alpha"});
 * alpha.observe(0.3);  // record a task running for 0.3 s
 * alpha.sub(0.3);      // remove it (task finished)
 * \endcode
 *
 * \note A `gaugehistogram_item` exposes the same operations as a
 *       standalone \ref gaugehistogram (`observe()`, `sub()`, plus
 *       `is_atomic()`), but is not constructible with a plain metric name.
 *       Use `gaugehistogram` if you need one outside a vector.
 *
 * \note **Bucket lifetime.** The item holds a non-owning span to the bucket
 *       boundaries owned by the parent `gaugehistogram_vec`. The vector
 *       must outlive the item. This is guaranteed in normal use, because
 *       items are only obtainable through `with_labels()` and live inside
 *       the vector's storage — but it means an item must not be copied or
 *       moved into a context that outlives its vector.
 *
 * \par Invariant
 * For each bucket, the cumulative number of `sub()` calls must not exceed
 * the cumulative number of `observe()` calls — bucket values MUST NOT
 * become negative. The `_gsum` value may become negative when buckets with
 * negative boundaries are used.
 *
 * \tparam AtomicPolicyType Compile-time atomicity policy, inherited from
 *         the parent `gaugehistogram_vec`. The default,
 *         `default_atomic_policy`, makes `observe()` and `sub()`
 *         thread-safe.
 *
 * \par Thread safety
 * With the default atomic policy, `observe()` and `sub()` on a resolved
 * item are thread-safe and may be called concurrently from multiple
 * threads, including mixing `observe()` and `sub()` on the same item from
 * different threads. With `atomic_policy<false>`, updates are not
 * synchronized and must be serialized by the caller.
 *
 * Note that item creation — the first `with_labels()` call for a given
 * label combination — is a property of the parent `gaugehistogram_vec`,
 * not of the item. Its thread-safety is documented on
 * \ref gaugehistogram_vec.
 *
 * \sa gaugehistogram_vec, gaugehistogram, gaugehistogram_item_params
 */
template <typename AtomicPolicyType>
class gaugehistogram_item
{
    using internal_type = detail::gaugehistogram_internal<AtomicPolicyType, false, false>;

    internal_type internal_;

    template <typename, typename, typename>
    friend class detail::basic_metric_vec;

    // Serialization
    friend struct boost::prometheus::format;

public:

    gaugehistogram_item() = delete;

    /**
     * \brief Constructs an item from pre-serialized parameters.
     *
     * \warning Do not call directly. This constructor is intended for
     *          use by \ref gaugehistogram_vec via `with_labels()`. The
     *          `bucket_boundaries` field of \p params is stored as a
     *          non-owning span; the referenced storage must outlive the
     *          item.
     *
     * \param params Pre-serialized name, bucket offsets, `_gsum`/`_gcount`
     *        headers (for 0.0.4 fallback), and a span to bucket boundaries
     *        owned by the parent vector. See
     *        \ref gaugehistogram_item_params.
     */
    gaugehistogram_item(detail::gaugehistogram_item_params&& params)
    : internal_(
        params.bucket_boundaries,
        std::move(params.base_string),
        params.buckets_offsets,
        params.sum_offset,
        params.count_offset,
        std::move(params.sum_header),
        std::move(params.count_header)
    ) {}

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
     * \brief Records an observation.
     *
     * Increments the matching `_bucket` counters by \p count, adds
     * `value * count` to `_gsum`, and increases `_gcount` by \p count.
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

    /**
     * \brief Removes a previously recorded observation.
     *
     * The inverse of `observe()`: decrements the matching `_bucket`
     * counters by \p count, subtracts `value * count` from `_gsum`, and
     * decreases `_gcount` by \p count.
     *
     * \param value Previously observed value.
     * \param count Multiplicity of the observation to remove; defaults to 1.
     *
     * \pre For each bucket, cumulative `sub()` calls must not exceed
     *      cumulative `observe()` calls.
     */
    void sub(double value, std::uint64_t count = 1) noexcept
    {
        internal_.sub(value, count);
    }
};

} // namespace boost::prometheus

#endif
