//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_BASIC_METRIC_VEC_HPP
#define BOOST_PROMETHEUS_DETAIL_BASIC_METRIC_VEC_HPP

#include <boost/prometheus/detail/export/metric_type_string.hpp>
#include <boost/prometheus/detail/export/serialize_prefix.hpp>

#include <boost/unordered/concurrent_node_map.hpp>
#include <boost/container_hash/hash.hpp>
#include <boost/container/small_vector.hpp>

#include <limits>
#include <span>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace boost::prometheus::detail {

struct metric_vec
{
    // Number of preallocated labels in small_vector
    static constexpr std::size_t key_static_size = 4;
    using key_type = boost::container::small_vector<std::string, key_static_size>;
};

/**
 * \brief Common base for all metric vector types.
 *
 * `basic_metric_vec` implements the family/cache logic shared by
 * \ref counter_vec, \ref gauge_vec, \ref histogram_vec,
 * \ref gaugehistogram_vec, and \ref info_vec. It performs name and
 * label serialization once at construction, and resolves a label
 * combination to a stable item reference via `with_labels(...)`.
 *
 * \par CRTP structure
 * The class uses the Curiously Recurring Template Pattern: \p DerivedType
 * is the concrete vector type (`counter_vec<...>`, `gauge_vec<...>`, ...),
 * and the base calls back into it via
 * `static_cast<DerivedType*>(this)->init_params(...)` to obtain the
 * pre-serialized exposition string for a newly resolved series. This is
 * how each vector type gets its own name formatting (e.g. the `_total`
 * suffix for counters, `_bucket`/`_count`/`_sum` for histograms).
 *
 * \par Label cache
 * Resolved label combinations are cached in an unordered map keyed by the
 * array of label values. The first call to `with_labels(...)` for a given
 * combination creates the item and stores its serialized exposition name;
 * subsequent calls return a reference to the same item. This makes
 * `with_labels(...)` cheap on the hot path — no allocation, no
 * serialization, just a hash lookup.
 *
 * The key type is a `boost::container::small_vector<std::string, 4>`
 * with a small inline buffer. For vectors with up to four labels, no heap
 * allocation occurs when constructing the key — the common case for most
 * metrics.
 *
 * \par Thread safety
 * The threading model depends on \p AtomicPolicy:
 *
 * * `atomic_policy<true>` (default): the cache is a
 *   `boost::concurrent_node_map`, and both `with_labels(...)` and item
 *   mutations are safe to call concurrently from any number of threads.
 *   The first call to `with_labels(...)` for a new label combination may
 *   block other threads performing the same lookup, but the operation is
 *   internally synchronized.
 * * `atomic_policy<false>`: the cache is a `std::unordered_map`, and
 *   **no** synchronization is performed. All calls to `with_labels(...)`
 *   and all item mutations must be externally serialized.
 *
 * \par Immutable configuration
 * After construction, the metric name, label names, format prefixes, and
 * (for histograms) bucket boundaries are fixed. Only the cache grows as
 * new label combinations are resolved. The cache is never shrunk.
 *
 * \tparam AtomicPolicy Compile-time atomicity policy. Determines both the
 *         cache implementation and the thread-safety of item updates.
 * \tparam DerivedType  The concrete vector type (CRTP).
 * \tparam ItemType     The item type returned by `with_labels(...)`, e.g.
 *         `counter_item<AtomicPolicy>`.
 * \tparam OptionsType  The options struct accepted by the constructor:
 *         `metric_opts`, `histogram_opts`, or `info_opts`.
 */
template <typename AtomicPolicy, typename DerivedType, typename ItemType, typename OptionsType>
class basic_metric_vec
{
    static constexpr std::size_t map_initial_size = 50;

public:

    /**
     * \brief Item type returned by `with_labels(...)`.
     */
    using item_type = ItemType;
    /**
     * \brief Key type used by the internal label cache.
     *
     * Exposed publicly so that callers can pre-construct a key and pass
     * it by rvalue to `with_labels(...)`, avoiding the final move.
     */
    using key_type = metric_vec::key_type;

protected:

    struct hasher
    {
        using is_transparent = void;
        // Since Boost 1.81.0 boost::hash passes SMHasher (only in 64-bit).
        using is_avalanching = std::bool_constant<(std::numeric_limits<std::size_t>::digits == 64)>;

        template <typename SpanType>
        std::size_t operator()(const SpanType& s) const noexcept
        {
            return boost::hash_range(s.begin(), s.end());
        }
    };

    struct equal
    {
        using is_transparent = void;

        bool operator()(const key_type& a, const key_type& b) const noexcept
        {
            return a == b;
        }

        template <typename SpanType1, typename SpanType2>
        bool operator()(const SpanType1& a, const SpanType2& b) const noexcept
        {
            return std::equal(a.begin(), a.end(), b.begin(), b.end());
        }
    };

    using map_type = std::conditional_t<
        AtomicPolicy::enabled,
        boost::concurrent_node_map<key_type, item_type, hasher, equal>,
        std::unordered_map<key_type, item_type, hasher, equal>>;

    map_type map_;

    // Sorted labels and original index for serialization.
    boost::container::small_vector<std::pair<std::string, std::size_t>, metric_vec::key_static_size> labels_;
    // Contains both format prefixes.
    std::string both_format_prefixes_;
    // Headers point to header part of prefix string.
    std::string_view prometheus_header_sv_;
    std::string_view openmetrics_header_sv_;
    // Metric name points to name part of prefix string,
    // instead of creating another string for name.
    std::string_view metric_name_sv_;

    /**
     * \brief Initializes the vector: validates the name and labels,
     *        sorts labels for serialization, and pre-serializes the
     *        exposition prefixes for both supported formats.
     *
     * \pre `name` is non-empty.
     * \pre `labels` contains no duplicates.
     * \pre `labels` is non-empty.
     *
     * All preconditions are checked with `BOOST_ASSERT_MSG` in debug
     * builds.
     */
    template <typename SpanType>
    void init(std::string_view name, OptionsType&& options, SpanType&& labels);

    /**
     * Called by `with_labels(...)` only on a cache miss, i.e. when a new
     * label combination needs to be registered. On a cache hit, the
     * serialization step is skipped entirely.
     */
    template <typename SpanType>
    auto init_key_and_params(key_type& key, SpanType&& labelvalues)
    {
        if constexpr (std::is_same_v<std::remove_cvref_t<SpanType>, key_type>)
        {
            static_assert(!std::is_lvalue_reference_v<SpanType>);
            key = std::move(labelvalues);
        }
        else if constexpr (std::is_lvalue_reference_v<SpanType>)
            key.assign(labelvalues.begin(), labelvalues.end());
        else
            // We can't move a container (such as std::vector) into
            // boost::small_vector directly, thus we just move all its elements.
            key.assign(
                std::make_move_iterator(labelvalues.begin()),
                std::make_move_iterator(labelvalues.end())
            );
        return static_cast<DerivedType*>(this)->init_params(metric_name_sv_, labels_, key);
    }

    template <typename SpanType>
    ItemType& with_labels_atomic_impl(SpanType&& labelvalues)
    {
        ItemType* result;
        auto get_result = [&result](auto& element)
        {
            result = &element.second;
        };
        // visit (no copy)
        bool found = map_.visit(labelvalues, get_result);
        if (found)
        {
            return *result;
        }

        key_type new_key;
        auto params = init_key_and_params(new_key, std::forward<SpanType>(labelvalues));

        map_.emplace_and_visit(
            std::move(new_key),
            std::move(params),
            get_result,
            get_result
        );

        return *result;
    }

    template <typename SpanType>
    ItemType& with_labels_non_atomic_impl(SpanType&& labelvalues)
    {
        // find (no copy)
        auto it = map_.find(labelvalues);
        if (it != map_.end())
            return it->second;

        key_type new_key;
        auto params = init_key_and_params(new_key, std::forward<SpanType>(labelvalues));

        auto result = map_.emplace(
            std::move(new_key),
            std::move(params));
        return result.first->second;
    }

    /**
     * \brief Dispatches to the atomic or non-atomic implementation based
     *        on \p AtomicPolicy.
     *
     * \pre `labels_.size() == labelvalues.size()`.
     */
    template <typename SpanType>
    ItemType& with_labels_impl(SpanType&& labelvalues);

public:

    basic_metric_vec() = delete;

    basic_metric_vec(std::string_view name, OptionsType&& options, std::span<const std::string_view> labels)
    {
        init(name, std::move(options), labels);
    }

    basic_metric_vec(std::string_view name, OptionsType&& options, std::span<const std::string> labels)
    {
        init(name, std::move(options), labels);
    }

    basic_metric_vec(std::string_view name, OptionsType&& options, std::vector<std::string>&& labels)
    {
        init(name, std::move(options), std::move(labels));
    }

    basic_metric_vec(std::string_view name, OptionsType&& options, std::initializer_list<std::string_view> labels)
    : basic_metric_vec(name, std::move(options), std::span(labels)) {}

    basic_metric_vec(std::string_view name, std::span<const std::string_view> labels)
    {
        init(name, {}, labels);
    }

    basic_metric_vec(std::string_view name, std::span<const std::string> labels)
    {
        init(name, {}, labels);
    }

    basic_metric_vec(std::string_view name, std::vector<std::string>&& labels)
    {
        init(name, {}, std::move(labels));
    }

    basic_metric_vec(std::string_view name, std::initializer_list<std::string_view> labels)
    : basic_metric_vec(name, std::span(labels)) {}

    /**
     * \brief Reports the atomicity policy of the vector.
     *
     * Mirrors `ItemType::is_atomic()`. When `true`, `with_labels(...)`
     * and item mutations are thread-safe; when `false`, they are not.
     */
    consteval static bool is_atomic() { return item_type::is_atomic(); }

    /**
     * \brief Returns the number of resolved label combinations.
     *
     * Counts distinct series, not updates. A vector that has never had
     * `with_labels(...)` called returns `0`. The count only grows; it is
     * never decremented.
     *
     * \par Thread safety
     * Safe to call concurrently with `with_labels(...)` and item updates
     * when the atomic policy is enabled.
     */
    std::size_t size() const noexcept { return map_.size(); }

    /**
     * \brief Returns a reference to the counter for the given label values.
     *
     * On the first call with a given label combination, the counter is
     * created and its exposition string is serialized. On subsequent
     * calls, the cached counter is returned directly — no allocation, no
     * serialization, just a hash lookup.
     *
     * The returned reference is stable for the lifetime of the vector and
     * may be cached and reused, e.g. to avoid repeated lookups in a hot
     * path:
     *
     * \code
     * auto& get_200 = requests.with_labels({"GET", "200"});
     * for (std::size_t i = 0; i < 1000u; i++)
     *     get_200.inc();
     * \endcode
     *
     * \param labelvalues Label values, in the same order as the label
     *        names passed to the constructor.
     *
     * \return Reference to the counter for the given label combination.
     *
     * \pre `labelvalues.size() == label_names.size()`.
     */
    item_type& with_labels(std::span<const std::string_view> labelvalues)
    {
        return with_labels_impl(labelvalues);
    }

    /**
     * \overload
     * \param labelvalues Label values as `std::string`.
     */
    item_type& with_labels(std::span<const std::string> labelvalues)
    {
        return with_labels_impl(labelvalues);
    }

    /**
     * \overload
     * \param labelvalues Label values as an owned
     *        `std::vector<std::string>`, moved into the vector. Use this
     *        overload to avoid copying when the values are already owned.
     */
    item_type& with_labels(std::vector<std::string>&& labelvalues)
    {
        return with_labels_impl(std::move(labelvalues));
    }

    /**
     * \overload
     *
     * Accepts a pre-built `key_type` directly, bypassing the per-element
     * conversion. Exposed for callers that already hold a key
     * and wish to avoid the intermediate `small_vector` construction.
     *
     * \param labelvalues Pre-built key. Moved into the vector on a cache
     *        miss; on a cache hit, the key is discarded.
     */
    item_type& with_labels(key_type&& labelvalues)
    {
        return with_labels_impl(std::move(labelvalues));
    }

    /**
     * \overload
     * \param labelvalues Initializer list of label values. The list is
     *        copied into the cache.
     */
    item_type& with_labels(std::initializer_list<std::string_view> labelvalues)
    {
        return with_labels(std::span(labelvalues));
    }

    /**
     * \brief Invokes \p func for each resolved item.
     *
     * Used by the serialization layer of the exposer and of
     * \ref serialize to enumerate all series of the vector.
     *
     * \param func Callable invoked with a reference to each item. The
     *             iteration order is unspecified.
     *
     * \par Thread safety
     * With the default atomic policy, safe to call concurrently with
     * `with_labels(...)` and item mutations: the underlying container is
     * a concurrent map. With `atomic_policy<false>`, the caller must
     * serialize all access.
     */
    template <typename Func>
    void for_each(Func&& func)
    {
        if constexpr (AtomicPolicy::enabled)
            map_.visit_all(func);
        else
            std::for_each(map_.begin(), map_.end(), func);
    }
};

} // namespace boost::prometheus::detail

#ifdef BOOST_PROMETHEUS_HEADER_ONLY
#include <boost/prometheus/impl/basic_metric_vec.ipp>
#endif

#endif
