//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_STATIC_HISTOGRAM_HPP
#define BOOST_PROMETHEUS_STATIC_HISTOGRAM_HPP

#include <boost/prometheus/detail/basic_static_metric.hpp>
#include <boost/prometheus/detail/friend_registration.hpp>
#include <boost/prometheus/detail/static_histogram_internal.hpp>
#include <boost/prometheus/detail/static_metric_registrator.hpp>

#include <boost/prometheus/metric_options.hpp>

#include <atomic>

namespace boost::prometheus {

/**
 * \brief Compile-time histogram with label values validated by the compiler.
 *
 * A `static_histogram` is a histogram whose name, label names, and bucket
 * boundaries are fixed at compile time, and whose label values are checked
 * by the compiler at each call site. There is no runtime lookup of labels,
 * no hashing, and no allocation on the hot path — each `observe<...>()`
 * call compiles down to direct atomic (or plain) updates on pre-resolved
 * storage.
 *
 * The series is registered with a `static_registry` at first use. The
 * exposition text (name, `# HELP`, `# TYPE`, bucket names, label
 * formatting) is serialized at compile time.
 *
 * The label name `"le"` is reserved for histogram bucket boundaries and
 * cannot be used as a user-defined label. Using it is a **compile-time
 * error**.
 *
 * When no `buckets<...>` option is provided, `default_buckets` is used:
 * 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0.
 *
 * \tparam MetricName Base metric name as an NTTP, e.g.
 *         `"request_duration_seconds"`. The `_bucket`, `_count`, and
 *         `_sum` suffixes are appended automatically in the exposition.
 * \tparam Options    Zero or more configuration types:
 *                   \ref help, \ref unit, \ref labels, \ref buckets,
 *                   \ref atomic_policy. Order does not matter; when
 *                   `atomic_policy` is omitted, `default_atomic_policy`
 *                   is used; when `buckets` is omitted, `default_buckets`
 *                   is used.
 *
 * \par Thread safety
 * With the default atomic policy, `observe()` is thread-safe and may be
 * called concurrently from multiple threads. With `atomic_policy<false>`,
 * updates are not synchronized and must be serialized by the caller.
 *
 * \par Example
 * \code
 * using namespace boost::prometheus;
 *
 * using req_duration = static_histogram<
 *     "request_duration_seconds",
 *     help<"Request latency">,
 *     unit<"seconds">,
 *     labels<"method">,
 *     buckets<0.005, 0.01, 0.05, 0.1, 0.5>>;
 *
 * req_duration::observe<"GET">(0.01);
 * req_duration::observe<"GET">(0.05);
 * req_duration::observe<"POST">(0.5, 3);  // count = 3
 *
 * // Compile-time errors:
 * // using bad = static_histogram<
 * //     "h", labels<"le">>;              // "le" is reserved
 * \endcode
 *
 * \note All members are static: a `static_histogram` is never instantiated.
 *       It serves purely as a compile-time handle for a specific metric.
 *
 * \sa static_counter, static_gauge, static_info, static_registry,
 *     default_buckets
 */
template <detail::string_buffer MetricName, typename... Options>
class static_histogram : public detail::basic_static_metric<
    static_histogram,
    detail::static_histogram_internal,
    MetricName,
    Options...
>
{
    using base_type = detail::basic_static_metric<
        static_histogram,
        detail::static_histogram_internal,
        MetricName,
        Options...
    >;

    static_assert([]() consteval
    {
        if constexpr (std::is_void_v<typename base_type::labels_type>)
            return true;
        else
            return ![]<std::size_t... Is>(std::index_sequence<Is...>) consteval
            {
                return ((std::string_view(std::get<Is>(base_type::labels_type::values)) == "le") || ...);
            }(std::make_index_sequence<base_type::labels_type::size()>{});
    }(), "Label \"le\" is reserved for histogram buckets");

    using buckets_type = detail::fallback_if_void_t<
        typename detail::extract_option<buckets, Options...>::type,
        default_buckets
    >;

    BOOST_PROMETHEUS_FRIEND_REGISTRATION()

public:

    /**
     * \brief Records a single observation.
     *
     * Increments the matching `_bucket` counters by \p count, adds
     * `value * count` to `_sum`, and increases `_count` by \p count.
     *
     * \tparam LabelValues Label values, matching the `labels<...>` option
     *         in the same order. Checked at compile time.
     * \param value Observed value.
     * \param count Multiplicity of the observation. Useful for batch
     *              recording when the same value is observed multiple
     *              times; defaults to 1.
     */
    template <detail::string_buffer... LabelValues>
    static void observe(double value, std::size_t count = 1)
    {
        auto& value_ref = base_type::template value_ref<LabelValues...>();
        // Find bucket index
        std::size_t bucket_idx = buckets_type::get_bucket_index(value);
        value_ref.observe(value, count, bucket_idx);
    }

    /**
     * \brief Resets the histogram for the given label values.
     *
     * \tparam LabelValues Label values, matching the `labels<...>` option
     *         in the same order. Checked at compile time.
     */
    // template <detail::string_buffer... LabelValues>
    // static void reset()
    // {
    //     auto& value_ref = base_type::template value_ref<LabelValues...>();
        
    // }
};

} // namespace boost::prometheus

#endif
