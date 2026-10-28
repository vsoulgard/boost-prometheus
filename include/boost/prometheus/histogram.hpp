//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_HISTOGRAM_HPP
#define BOOST_PROMETHEUS_HISTOGRAM_HPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/histogram_internal.hpp>

#include <boost/prometheus/detail/export/serialize_header.hpp>
#include <boost/prometheus/detail/export/serialize_histogram_names.hpp>

namespace boost::prometheus {

/**
 * \brief Histogram metric that samples observations into buckets.
 *
 * A histogram records numerical observations (typically durations or sizes)
 * and counts them in configurable buckets. Each observation increments the
 * matching `_bucket` counters, and the total count and sum are exposed as
 * `_count` and `_sum`. An implicit `+Inf` bucket captures all observations
 * larger than the last configured boundary.
 *
 * When `histogram_opts::buckets` is empty, the default bucket set
 * (`default_buckets`) is used: 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5,
 * 1.0, 2.5, 5.0, 10.0.
 *
 * \tparam AtomicPolicyType Compile-time atomicity policy. The default,
 *         `default_atomic_policy` (i.e. `atomic_policy<true>`), makes
 *         `observe()` thread-safe. Use `atomic_policy<false>` to skip atomic
 *         operations when observations are externally synchronized.
 *
 * \par Thread safety
 * With the default atomic policy, `observe()` may be called concurrently
 * from multiple threads. With a non-atomic policy, concurrent calls are not
 * synchronized and must be guarded by the caller.
 *
 * \par Example
 * \code
 * prometheus::histogram req_duration(
 *     "request_duration_seconds",
 *     {.help = "Request latency",
 *      .unit = "seconds",
 *      .buckets = {0.005, 0.01, 0.05, 0.1, 0.5}});
 *
 * req_duration.observe(0.01);
 * req_duration.observe(0.5);
 *
 * // Exposed as:
 * //   request_duration_seconds_bucket{le="0.005"} 0
 * //   request_duration_seconds_bucket{le="0.01"} 1
 * //   request_duration_seconds_bucket{le="0.05"} 1
 * //   request_duration_seconds_bucket{le="0.1"} 1
 * //   request_duration_seconds_bucket{le="0.5"} 2
 * //   request_duration_seconds_bucket{le="+Inf"} 2
 * //   request_duration_seconds_count 2
 * //   request_duration_seconds_sum 0.51
 * \endcode
 */
template <typename AtomicPolicyType = default_atomic_policy>
class histogram
{
    using internal_type = detail::histogram_internal<AtomicPolicyType>;

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
        
        std::span<const double> bucket_boundaries;
        if (!options.buckets.empty())
        {
            bucket_boundaries = options.buckets;
        }
        else
        {
            bucket_boundaries = default_buckets::values;
        }
        auto histogram_names = detail::serialize_histogram_names(name, bucket_boundaries);
        return internal_type(
            std::move(headers.base_string),
            headers.openmetrics_prefix_offset,
            bucket_boundaries,
            std::move(histogram_names.base_string),
            histogram_names.buckets_offsets,
            histogram_names.sum_offset,
            histogram_names.count_offset
        );
    }

public:

    histogram() = delete;

    /**
     * \brief Constructs a histogram.
     *
     * If `options.buckets` is non-empty, the bucket boundaries are moved
     * into the histogram; the histogram owns its bucket data, so
     * \p options need not outlive the histogram. If `options.buckets` is
     * empty, `default_buckets` is used.
     *
     * \param name    Base metric name, e.g. `"request_duration_seconds"`.
     *                The `_bucket`, `_count`, and `_sum` suffixes are
     *                appended automatically in the exposition.
     * \param options `help`, `unit`, and `buckets`. See \ref histogram_opts.
     */
    histogram(std::string_view name, histogram_opts&& options = {})
    : internal_(init_internal(name, options))
    {
        if (!options.buckets.empty())
        {
            bucket_boundaries_ = std::move(options.buckets);
            // Re-assign std::span to point at the moved-into storage.
            internal_.bucket_boundaries_ = bucket_boundaries_;
        }
    }

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
     *              recording when the same value is observed multiple times;
     *              defaults to 1.
     */
    void observe(double value, std::uint64_t count = 1) noexcept
    {
        internal_.observe(value, count);
    }
};

} // namespace boost::prometheus

#endif
