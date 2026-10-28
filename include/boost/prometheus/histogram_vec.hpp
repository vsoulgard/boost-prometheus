//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_HISTOGRAM_VEC_HPP
#define BOOST_PROMETHEUS_HISTOGRAM_VEC_HPP

#include <boost/prometheus/histogram_item.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/basic_metric_vec.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <boost/prometheus/detail/export/serialize_histogram_names_with_labels.hpp>

namespace boost::prometheus {

/**
 * \brief Histogram metric with dynamically specified labels.
 *
 * A `histogram_vec` is a family of histograms that share one name, one set
 * of label names, and one bucket configuration. Individual series are
 * obtained via `with_labels(...)`, which returns a reference to a
 * `histogram_item` that writes to the shared storage of the vector.
 *
 * All series produced by a single `histogram_vec` share the same bucket
 * boundaries — either explicitly provided via `histogram_opts::buckets`,
 * or the default bucket set (`default_buckets`) when buckets are omitted.
 * Per-series buckets are not supported: to use different buckets, create
 * separate `histogram_vec` instances with distinct names.
 *
 * Resolved label combinations are cached: the first call to
 * `with_labels()` with a given set of values creates the series, and
 * subsequent calls with the same values return the same reference.
 *
 * \tparam AtomicPolicyType Compile-time atomicity policy. The default,
 *         `default_atomic_policy`, makes `observe()` on individual items
 *         thread-safe. Use `atomic_policy<false>` when observations are
 *         externally synchronized.
 *
 * \par Thread safety
 * With the default atomic policy, all of the following are thread-safe and
 * may be called concurrently from multiple threads:
 *
 * * `with_labels(...)` — resolving a label combination, including the
 *   first call that creates the series;
 * * `observe()` on resolved items.
 *
 * Calls to `with_labels()` and item updates may be freely mixed.
 *
 * With `atomic_policy<false>`, neither `with_labels()` nor item updates are
 * synchronized. In that mode, externally serialize all calls into the
 * vector, including the first `with_labels()` for any given label
 * combination.
 *
 * \par Example
 * \code
 * prometheus::histogram_vec req_duration(
 *     "request_duration_seconds",                     // base name
 *     {.help = "Request latency",
 *      .unit = "seconds",
 *      .buckets = {0.005, 0.01, 0.05, 0.1, 0.5}},     // shared buckets
 *     {"method"});                                    // label names
 *
 * req_duration.with_labels({"GET"}).observe(0.01);
 * req_duration.with_labels({"GET"}).observe(0.05);
 * req_duration.with_labels({"POST"}).observe(0.5);
 *
 * // Cached: same reference as the first call above.
 * auto& get_duration = req_duration.with_labels({"GET"});
 * get_duration.observe(0.2);
 *
 * // Exposed as:
 * //   request_duration_seconds_bucket{le="0.005",method="GET"} 0
 * //   request_duration_seconds_bucket{le="0.01",method="GET"} 1
 * //   request_duration_seconds_bucket{le="0.05",method="GET"} 2
 * //   request_duration_seconds_bucket{le="0.1",method="GET"} 3
 * //   request_duration_seconds_bucket{le="0.5",method="GET"} 3
 * //   request_duration_seconds_bucket{le="+Inf",method="GET"} 3
 * //   request_duration_seconds_count{method="GET"} 3
 * //   request_duration_seconds_sum{method="GET"} 0.26
 * \endcode
 */
template <typename AtomicPolicyType = default_atomic_policy>
class histogram_vec : public detail::basic_metric_vec<
    histogram_vec<AtomicPolicyType>,
    histogram_item<AtomicPolicyType>,
    histogram_opts
>
{
    using item_type = histogram_item<AtomicPolicyType>;
    using base_type = detail::basic_metric_vec<
        histogram_vec<AtomicPolicyType>,
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
    detail::histogram_item_params init_params(
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
            detail::serialize_histogram_names_with_labels(
            metric_name,
            bucket_boundaries,
            labels,
            labelvalues
        );
        return {
            bucket_boundaries,
            std::move(base_string),
            std::move(buckets_offsets),
            std::move(sum_offset),
            std::move(count_offset)
        };
    }

public:

    /**
     * \brief Constructs a histogram vector from a span of label names.
     *
     * \param name    Base metric name, e.g. `"request_duration_seconds"`.
     *                The `_bucket`, `_count`, and `_sum` suffixes are
     *                appended automatically in the exposition.
     * \param options `help`, `unit`, and shared `buckets`. See
     *                \ref histogram_opts. When `buckets` is empty,
     *                `default_buckets` is used.
     * \param labels  Label names, e.g. `{"method"}`. Must outlive the
     *                construction; only the contents are copied.
     */
    histogram_vec(std::string_view name, histogram_opts&& options, std::span<const std::string_view> labels)
    : base_type(name, std::move(options), labels)
    {
        init(std::move(options));
    }

    /**
     * \overload
     * \param labels Label names owned as `std::string`.
     */
    histogram_vec(std::string_view name, histogram_opts&& options, std::span<const std::string> labels)
    : base_type(name, std::move(options), labels)
    {
        init(std::move(options));
    }

    /**
     * \overload
     * \param labels Label names as an owned `std::vector<std::string>`,
     *               moved into the metric.
     */
    histogram_vec(std::string_view name, histogram_opts&& options, std::vector<std::string>&& labels)
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
    histogram_vec(std::string_view name, histogram_opts&& options, std::initializer_list<std::string_view> labels)
    : histogram_vec(name, std::move(options), std::span(labels)) {}

    /**
     * \brief Constructs a histogram vector with default options and default
     *        bucket set.
     *
     * \param name   Base metric name.
     * \param labels Label names, e.g. `{"method"}`.
     */
    histogram_vec(std::string_view name, std::span<const std::string_view> labels)
    : base_type(name, labels) {}

    /**
     * \overload
     * \param labels Label names owned as `std::string`.
     */
    histogram_vec(std::string_view name, std::span<const std::string> labels)
    : base_type(name, labels) {}

    /**
     * \overload
     * \param labels Label names as an owned `std::vector<std::string>`,
     *               moved into the metric.
     */
    histogram_vec(std::string_view name, std::vector<std::string>&& labels)
    : base_type(name, std::move(labels)) {}

    /**
     * \overload
     * \param labels Initializer list of label names.
     */
    histogram_vec(std::string_view name, std::initializer_list<std::string_view> labels)
    : histogram_vec(name, std::span(labels)) {}
};

} // namespace boost::prometheus

#endif
