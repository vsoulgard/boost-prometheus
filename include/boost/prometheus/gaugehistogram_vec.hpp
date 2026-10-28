//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_GAUGEHISTOGRAM_VEC_HPP
#define BOOST_PROMETHEUS_GAUGEHISTOGRAM_VEC_HPP

#include <boost/prometheus/gaugehistogram_item.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/basic_metric_vec.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <boost/prometheus/detail/export/serialize_header.hpp>
#include <boost/prometheus/detail/export/serialize_histogram_names_with_labels.hpp>

namespace boost::prometheus {

/**
 * \brief Gaugehistogram metric with dynamically specified labels.
 *
 * A `gaugehistogram_vec` is a family of gaugehistograms that share one
 * name, one set of label names, and one bucket configuration. Individual
 * series are obtained via `with_labels(...)`, which returns a reference to
 * a `gaugehistogram_item` that writes to the shared storage of the vector.
 *
 * Unlike regular histograms, gaugehistogram bucket values can decrease:
 * individual items expose both `observe()` (record an observation) and
 * `sub()` (remove a previously recorded observation). This makes the type
 * suitable for tracking instantaneous distributions whose contents change
 * over time.
 *
 * All series produced by a single `gaugehistogram_vec` share the same
 * bucket boundaries — either explicitly provided via
 * `histogram_opts::buckets`, or `default_buckets` when buckets are omitted.
 * Per-series buckets are not supported.
 *
 * Resolved label combinations are cached: the first call to
 * `with_labels()` with a given set of values creates the series, and
 * subsequent calls with the same values return the same reference.
 *
 * \tparam AtomicPolicyType Compile-time atomicity policy. The default,
 *         `default_atomic_policy`, makes `observe()` and `sub()` on
 *         individual items thread-safe. Use `atomic_policy<false>` when
 *         updates are externally synchronized.
 *
 * \par Thread safety
 * With the default atomic policy, all of the following are thread-safe and
 * may be called concurrently from multiple threads:
 *
 * * `with_labels(...)` — resolving a label combination, including the
 *   first call that creates the series;
 * * `observe()` and `sub()` on resolved items.
 *
 * Calls to `with_labels()` and item updates may be freely mixed, including
 * `observe()`/`sub()` from different threads on the same item.
 *
 * With `atomic_policy<false>`, neither `with_labels()` nor item updates are
 * synchronized. In that mode, externally serialize all calls into the
 * vector, including the first `with_labels()` for any given label
 * combination.
 *
 * \par Invariant
 * For each bucket, the cumulative number of `sub()` calls must not exceed
 * the cumulative number of `observe()` calls — bucket values MUST NOT
 * become negative.
 *
 * \par Example
 * \code
 * prometheus::gaugehistogram_vec tasks_by_duration(
 *     "tasks_by_duration_seconds",                    // base name
 *     {.help = "Currently running tasks by duration",
 *      .unit = "seconds",
 *      .buckets = {0.5, 1.0, 5.0, 30.0}},             // shared buckets
 *     {"worker_pool"});                               // label names
 *
 * // A task has been running for 0.3 s on pool "alpha".
 * tasks_by_duration.with_labels({"alpha"}).observe(0.3);
 *
 * // The task finished — remove it from the same bucket.
 * tasks_by_duration.with_labels({"alpha"}).sub(0.3);
 *
 * // Cached: same reference as the first call above.
 * auto& alpha = tasks_by_duration.with_labels({"alpha"});
 * alpha.observe(1.2);
 *
 * // Exposed as:
 * //   tasks_by_duration_seconds_bucket{le="0.5",worker_pool="alpha"} 0
 * //   tasks_by_duration_seconds_bucket{le="1",worker_pool="alpha"} 0
 * //   tasks_by_duration_seconds_bucket{le="5",worker_pool="alpha"} 1
 * //   tasks_by_duration_seconds_bucket{le="30",worker_pool="alpha"} 1
 * //   tasks_by_duration_seconds_bucket{le="+Inf",worker_pool="alpha"} 1
 * //   tasks_by_duration_seconds_gcount{worker_pool="alpha"} 1
 * //   tasks_by_duration_seconds_gsum{worker_pool="alpha"} 1.2
 * \endcode
 */
template <typename AtomicPolicyType = default_atomic_policy>
class gaugehistogram_vec : public detail::basic_metric_vec<
    gaugehistogram_vec<AtomicPolicyType>,
    gaugehistogram_item<AtomicPolicyType>,
    histogram_opts
>
{
    using item_type = gaugehistogram_item<AtomicPolicyType>;
    using base_type = detail::basic_metric_vec<
        gaugehistogram_vec<AtomicPolicyType>,
        item_type,
        histogram_opts
    >;
    friend base_type;

    std::vector<double> bucket_boundaries_;

    // Serialization
    friend struct boost::prometheus::format;

    void init(histogram_opts&& options)
    {
        bucket_boundaries_ = std::move(options.buckets);
    }

    // Called in base_type::with_labels_impl
    template <typename LabelsType, typename LabelValuesType>
    detail::gaugehistogram_item_params init_params(
        std::string_view metric_name,
        const LabelsType& labels,
        const LabelValuesType& labelvalues)
    {
        std::span<const double> bucket_boundaries;
        if (!bucket_boundaries_.empty())
            bucket_boundaries = bucket_boundaries_;
        else
            bucket_boundaries = default_buckets::values;
        auto [base_string, buckets_offsets, sum_offset, count_offset] =
            detail::serialize_gaugehistogram_names_with_labels(
            metric_name,
            bucket_boundaries,
            labels,
            labelvalues
        );
        // For Prometheus 0.0.4 fallback we serialize _gsum 
        // and _gcount as separate gauges.
        auto sum_header = detail::serialize_header<
            "gauge",
            "gauge",
            "_gsum"
        >(metric_name, histogram_opts{});
        auto count_header = detail::serialize_header<
            "gauge",
            "gauge",
            "_gcount"
        >(metric_name, histogram_opts{});
        return {
            bucket_boundaries,
            std::move(base_string),
            std::move(buckets_offsets),
            std::move(sum_offset),
            std::move(count_offset),
            std::move(sum_header.base_string),
            std::move(count_header.base_string)
        };
    }

public:

    /**
     * \brief Constructs a gauge-histogram vector from a span of label names.
     *
     * \param name    Base metric name, e.g. `"tasks_by_duration_seconds"`.
     *                The `_bucket`, `_gcount`, and `_gsum` suffixes are
     *                appended automatically in the exposition.
     * \param options `help`, `unit`, and shared `buckets`. See
     *                \ref histogram_opts. When `buckets` is empty,
     *                `default_buckets` is used.
     * \param labels  Label names, e.g. `{"worker_pool"}`. Must outlive the
     *                construction; only the contents are copied.
     */
    gaugehistogram_vec(std::string_view name, histogram_opts&& options, std::span<const std::string_view> labels)
    : base_type(name, std::move(options), labels)
    {
        init(std::move(options));
    }

    /**
     * \overload
     * \param labels Label names owned as `std::string`.
     */
    gaugehistogram_vec(std::string_view name, histogram_opts&& options, std::span<const std::string> labels)
    : base_type(name, std::move(options), labels)
    {
        init(std::move(options));
    }

    /**
     * \overload
     * \param labels Label names as an owned `std::vector<std::string>`,
     *               moved into the metric.
     */
    gaugehistogram_vec(std::string_view name, histogram_opts&& options, std::vector<std::string>&& labels)
    : base_type(name, std::move(options), std::move(labels))
    {
        init(std::move(options));
    }

    /**
     * \overload
     * \param labels Initializer list of label names. The list is copied
     *               into internal storage; it need not outlive the
     *               constructor.
     */
    gaugehistogram_vec(std::string_view name, histogram_opts&& options, std::initializer_list<std::string_view> labels)
    : gaugehistogram_vec(name, std::move(options), std::span(labels)) {}

    /**
     * \brief Constructs a gauge-histogram vector with default options and
     *        default bucket set.
     *
     * \param name   Base metric name.
     * \param labels Label names, e.g. `{"worker_pool"}`.
     */
    gaugehistogram_vec(std::string_view name, std::span<const std::string_view> labels)
    : base_type(name, labels) {}

    /**
     * \overload
     * \param labels Label names owned as `std::string`.
     */
    gaugehistogram_vec(std::string_view name, std::span<const std::string> labels)
    : base_type(name, labels) {}

    /**
     * \overload
     * \param labels Label names as an owned `std::vector<std::string>`,
     *               moved into the metric.
     */
    gaugehistogram_vec(std::string_view name, std::vector<std::string>&& labels)
    : base_type(name, std::move(labels)) {}

    /**
     * \overload
     * \param labels Initializer list of label names.
     */
    gaugehistogram_vec(std::string_view name, std::initializer_list<std::string_view> labels)
    : gaugehistogram_vec(name, std::span(labels)) {}
};

} // namespace boost::prometheus

#endif
