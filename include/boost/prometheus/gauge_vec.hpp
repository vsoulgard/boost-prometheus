//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_GAUGE_VEC_HPP
#define BOOST_PROMETHEUS_GAUGE_VEC_HPP

#include <boost/prometheus/gauge_item.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/basic_metric_vec.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <boost/prometheus/detail/export/serialize_name_with_labels.hpp>

namespace boost::prometheus {

/**
 * \brief Gauge metric with dynamically specified labels.
 *
 * A `gauge_vec` is a family of gauges that share one name and one set of
 * label names. Individual series are obtained via `with_labels(...)`, which
 * returns a reference to a `gauge_item` — a gauge that writes to the shared
 * storage of the vector.
 *
 * Resolved label combinations are cached: the first call to
 * `with_labels()` with a given set of values creates the series, and
 * subsequent calls with the same values return the same reference. This
 * makes `with_labels()` cheap enough to call from a hot path.
 *
 * Unlike \ref counter_vec, the exposition name is emitted as-is — no
 * suffix is appended automatically.
 *
 * \tparam AtomicPolicyType Compile-time atomicity policy. The default,
 *         `default_atomic_policy`, makes `inc()`, `dec()`, and `set()` on
 *         individual items thread-safe. Use `atomic_policy<false>` when
 *         updates are externally synchronized.
 *
 * \par Thread safety
 * With the default atomic policy, all of the following are thread-safe and
 * may be called concurrently from multiple threads:
 *
 * * `with_labels(...)` — resolving a label combination, including the
 *   first call that creates the series;
 * * `inc()`, `dec()`, and `set()` on resolved items.
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
 * prometheus::gauge_vec active(
 *     "applications_active",                    // base name
 *     {.help = "Number of active applications"},// options
 *     {"state"});                               // label names
 *
 * active.with_labels({"running"}).set(42);
 * active.with_labels({"idle"}).set(7);
 *
 * // Cached: same reference as the first call above.
 * auto& running = active.with_labels({"running"});
 * running.inc();    // 43
 * running.dec(3);   // 40
 *
 * // Exposed as:
 * //   applications_active{state="running"} 40
 * //   applications_active{state="idle"} 7
 * \endcode
 */
template <typename AtomicPolicyType = default_atomic_policy>
class gauge_vec : public detail::basic_metric_vec<
    gauge_vec<AtomicPolicyType>,
    gauge_item<AtomicPolicyType>,
    metric_opts
>
{
    using item_type = gauge_item<AtomicPolicyType>;
    using base_type = detail::basic_metric_vec<
        gauge_vec<AtomicPolicyType>,
        item_type,
        metric_opts
    >;
    friend base_type;

    // Serialization
    friend struct boost::prometheus::format;

    // Called in base_type::with_labels_impl
    template <typename LabelsType, typename LabelValuesType>
    std::string init_params(std::string_view metric_name, const LabelsType& labels, const LabelValuesType& labelvalues)
    {
        return detail::serialize_name_with_labels(
            metric_name,
            labels,
            labelvalues
        );
    }

public:

    /**
     * \brief Constructs a gauge vector from a span of label names.
     *
     * \param name    Metric name, e.g. `"applications_active"`.
     * \param options `help` and `unit`. See \ref metric_opts.
     * \param labels  Label names, e.g. `{"state"}`. Must outlive the
     *                construction; only the contents are copied.
     */
    gauge_vec(std::string_view name, metric_opts&& options, std::span<const std::string_view> labels)
    : base_type(name, std::move(options), labels) {}

    /**
     * \overload
     * \param labels Label names owned as `std::string`.
     */
    gauge_vec(std::string_view name, metric_opts&& options, std::span<const std::string> labels)
    : base_type(name, std::move(options), labels) {}

    /**
     * \overload
     * \param labels Label names as an owned `std::vector<std::string>`,
     *               moved into the metric.
     */
    gauge_vec(std::string_view name, metric_opts&& options, std::vector<std::string>&& labels)
    : base_type(name, std::move(options), std::move(labels)) {}

    /**
     * \overload
     * \param labels Initializer list of label names. The list is copied
     *               into internal storage; it need not outlive the
     *               constructor.
     */
    gauge_vec(std::string_view name, metric_opts&& options, std::initializer_list<std::string_view> labels)
    : gauge_vec(name, std::move(options), std::span(labels)) {}

    /**
     * \brief Constructs a gauge vector with default options.
     *
     * Equivalent to calling the options-taking overload with `metric_opts{}`.
     *
     * \param name   Metric name, e.g. `"applications_active"`.
     * \param labels Label names, e.g. `{"state"}`.
     */
    gauge_vec(std::string_view name, std::span<const std::string_view> labels)
    : base_type(name, labels) {}

    /**
     * \overload
     * \param labels Label names owned as `std::string`.
     */
    gauge_vec(std::string_view name, std::span<const std::string> labels)
    : base_type(name, labels) {}

    /**
     * \overload
     * \param labels Label names as an owned `std::vector<std::string>`,
     *               moved into the metric.
     */
    gauge_vec(std::string_view name, std::vector<std::string>&& labels)
    : base_type(name, std::move(labels)) {}

    /**
     * \overload
     * \param labels Initializer list of label names.
     */
    gauge_vec(std::string_view name, std::initializer_list<std::string_view> labels)
    : gauge_vec(name, std::span(labels)) {}
};

} // namespace boost::prometheus

#endif
