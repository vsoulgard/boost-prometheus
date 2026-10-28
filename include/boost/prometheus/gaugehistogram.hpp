//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_GAUGEHISTOGRAM_HPP
#define BOOST_PROMETHEUS_GAUGEHISTOGRAM_HPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/gaugehistogram_internal.hpp>

#include <boost/prometheus/detail/export/serialize_header.hpp>
#include <boost/prometheus/detail/export/serialize_histogram_names.hpp>

namespace boost::prometheus {

/**
 * \brief Histogram metric whose bucket values can decrease.
 *
 * A gauge histogram behaves like \ref histogram, but its bucket values can
 * go down as well as up. `observe()` records an observation (incrementing
 * the matching bucket), while `sub()` removes it (decrementing). It is
 * suitable for tracking instantaneous distributions whose contents change
 * over time, such as currently-running tasks grouped by duration.
 *
 * In the exposition output, the count and sum are exposed as `_gcount` and
 * `_gsum` in the Prometheus 0.0.4 format (as separate gauges), and as
 * `_gcount` and `_gsum` in OpenMetrics 1.0. The `_g` prefix distinguishes
 * them from a regular histogram's `_count` and `_sum`.
 *
 * When `histogram_opts::buckets` is empty, `default_buckets` is used.
 *
 * \tparam AtomicPolicyType Compile-time atomicity policy. The default,
 *         `default_atomic_policy` (i.e. `atomic_policy<true>`), makes
 *         `observe()` and `sub()` thread-safe. Use `atomic_policy<false>`
 *         to skip atomic operations when updates are externally
 *         synchronized.
 *
 * \par Thread safety
 * With the default atomic policy, `observe()` and `sub()` may be called
 * concurrently from multiple threads. With a non-atomic policy, concurrent
 * calls are not synchronized and must be guarded by the caller.
 *
 * \par Invariant
 * For each bucket, the number of `sub()` calls must not exceed the number
 * of `observe()` calls — bucket values MUST NOT become negative. The
 * `_gsum` value may become negative when buckets with negative boundaries
 * are used.
 *
 * \par Example
 * \code
 * prometheus::gaugehistogram temperature(
 *     "cpu_temperature_celsius",
 *     {.help = "CPU core temperature",
 *      .unit = "celsius",
 *      .buckets = {25.0, 50.0, 75.0, 90.0}});
 *
 * temperature.observe(33.0);  // record a core at ~33 °C
 * temperature.sub(33.0);      // core has cooled down or exited
 * temperature.observe(55.0);
 * \endcode
 */
template <typename AtomicPolicyType = default_atomic_policy>
class gaugehistogram
{
    using internal_type = detail::gaugehistogram_internal<AtomicPolicyType>;

    internal_type internal_;

    // Empty if default buckets provided.
    std::vector<double> bucket_boundaries_;

    // Serialization
    friend struct boost::prometheus::format;

    static internal_type init_internal(std::string_view name, const histogram_opts& options)
    {
        auto headers = detail::serialize_header<
            internal_type::prometheus_type,
            internal_type::openmetrics_type
        >(name, options);
        // Fallback 0.0.4 format headers (serialized as gauges)
        auto sum_header = detail::serialize_header<
            "gauge",
            "gauge",
            "_gsum"
        >(name, histogram_opts{});
        auto count_header = detail::serialize_header<
            "gauge",
            "gauge",
            "_gcount"
        >(name, histogram_opts{});

        std::span<const double> bucket_boundaries;
        if (!options.buckets.empty())
        {
            bucket_boundaries = options.buckets;
        }
        else
        {
            bucket_boundaries = default_buckets::values;
        }
        auto histogram_names = detail::serialize_gaugehistogram_names(name, bucket_boundaries);
        return internal_type(
            std::move(headers.base_string),
            headers.openmetrics_prefix_offset,
            bucket_boundaries,
            std::move(histogram_names.base_string),
            histogram_names.buckets_offsets,
            histogram_names.sum_offset,
            histogram_names.count_offset,
            std::move(sum_header.base_string),
            std::move(count_header.base_string)
        );
    }

public:

    gaugehistogram() = delete;

    /**
     * \brief Constructs a gauge histogram.
     *
     * If `options.buckets` is non-empty, the bucket boundaries are moved
     * into the gauge histogram; the gauge histogram owns its bucket data,
     * so \p options need not outlive the gauge histogram. If
     * `options.buckets` is empty, `default_buckets` is used.
     *
     * \param name    Base metric name. The `_bucket`, `_gcount`, and `_gsum`
     *                suffixes are appended automatically in the exposition.
     * \param options `help`, `unit`, and `buckets`. See \ref histogram_opts.
     */
    gaugehistogram(std::string_view name, histogram_opts&& options = {})
    : internal_(init_internal(name, options))
    {
        if (!options.buckets.empty())
        {
            bucket_boundaries_ = std::move(options.buckets);
            // Re-assign std::span
            internal_.bucket_boundaries_ = bucket_boundaries_;
        }
    }

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
     *              recording when the same value is observed multiple times;
     *              defaults to 1.
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
