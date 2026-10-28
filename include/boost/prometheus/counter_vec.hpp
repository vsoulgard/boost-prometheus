//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_COUNTER_VEC_HPP
#define BOOST_PROMETHEUS_COUNTER_VEC_HPP

#include <boost/prometheus/counter_item.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/basic_metric_vec.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <boost/prometheus/detail/export/serialize_name_with_labels.hpp>

namespace boost::prometheus {

/**
 * \brief Counter metric with dynamically specified labels.
 *
 * A `counter_vec` is a family of counters that share one name and one set
 * of label names. Individual series are obtained via `with_labels(...)`,
 * which returns a reference to a `counter_item` — a counter that writes to
 * the shared storage of the vector.
 *
 * Resolved label combinations are cached: the first call to
 * `with_labels()` with a given set of values creates the series, and
 * subsequent calls with the same values return the same reference. This
 * makes `with_labels()` cheap enough to call from a hot path.
 *
 * As with \ref counter, the `_total` suffix is appended automatically in
 * the exposition. Passing a name that already contains `_total` is a
 * precondition violation (checked via `BOOST_ASSERT_MSG` in debug builds).
 *
 * \tparam AtomicPolicyType Compile-time atomicity policy. The default,
 *         `default_atomic_policy`, makes `inc()` on individual items
 *         thread-safe. Use `atomic_policy<false>` when updates are
 *         externally synchronized.
 *
 * \par Thread safety
 * With the default atomic policy, all of the following are thread-safe and
 * may be called concurrently from multiple threads:
 *
 * * `with_labels(...)` — resolving a label combination, including the
 *   first call that creates the series;
 * * `inc()` on resolved items (`with_labels(...).inc()`).
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
 * prometheus::counter_vec requests(
 *     "requests",                               // base name
 *     {.help = "Number of received requests"},  // options
 *     {"method", "status"});                    // label names
 *
 * requests.with_labels({"GET", "200"}).inc();
 * requests.with_labels({"GET", "500"}).inc();
 * requests.with_labels({"POST", "200"}).inc();
 *
 * // Cached: same reference as the first call above.
 * auto& get_requests = requests.with_labels({"GET", "200"});
 * get_requests.inc(5);
 *
 * // Exposed as:
 * //   requests_total{method="GET",status="200"} 6
 * //   requests_total{method="GET",status="500"} 1
 * //   requests_total{method="POST",status="200"} 1
 * \endcode
 */
template <typename AtomicPolicyType = default_atomic_policy>
class counter_vec : public detail::basic_metric_vec<
    counter_vec<AtomicPolicyType>,
    counter_item<AtomicPolicyType>,
    metric_opts
>
{
    using item_type = counter_item<AtomicPolicyType>;
    using base_type = detail::basic_metric_vec<
        counter_vec<AtomicPolicyType>,
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
        return detail::serialize_counter_name_with_labels(
            metric_name,
            labels,
            labelvalues
        );
    }

public:

    /**
     * \brief Constructs a counter vector from a span of label names.
     *
     * \param name    Base metric name. The `_total` suffix is appended
     *                automatically in the exposition.
     * \param options `help` and `unit`. See \ref metric_opts.
     * \param labels  Label names, e.g. `{"method", "status"}`. Must
     *                outlive the construction; only the contents are copied.
     *
     * \pre `name` does not contain `_total`.
     */
    counter_vec(std::string_view name, metric_opts&& options, std::span<const std::string_view> labels)
    : base_type(name, std::move(options), labels)
    {
       BOOST_ASSERT_MSG(name.find("_total") == std::string_view::npos,
            "Counter name shouldn't contain \"_total\" suffix; it will be appended automatically");
    }

    /**
     * \overload
     * \param labels Label names owned as `std::string`.
     */
    counter_vec(std::string_view name, metric_opts&& options, std::span<const std::string> labels)
    : base_type(name, std::move(options), labels)
    {
       BOOST_ASSERT_MSG(name.find("_total") == std::string_view::npos,
            "Counter name shouldn't contain \"_total\" suffix; it will be appended automatically");
    }

    /**
     * \overload
     * \param labels Label names as an owned `std::vector<std::string>`,
     *               moved into the metric.
     */
    counter_vec(std::string_view name, metric_opts&& options, std::vector<std::string>&& labels)
    : base_type(name, std::move(options), std::move(labels))
    {
       BOOST_ASSERT_MSG(name.find("_total") == std::string_view::npos,
            "Counter name shouldn't contain \"_total\" suffix; it will be appended automatically");
    }

    /**
     * \overload
     * \param labels Initializer list of label names. The list is copied
     *               into internal storage; it need not outlive the
     *               constructor.
     */
    counter_vec(std::string_view name, metric_opts&& options, std::initializer_list<std::string_view> labels)
    : counter_vec(name, std::move(options), std::span(labels)) {}

    /**
     * \brief Constructs a counter vector with default options.
     *
     * Equivalent to calling the options-taking overload with `metric_opts{}`.
     *
     * \param name   Base metric name. The `_total` suffix is appended
     *               automatically in the exposition.
     * \param labels Label names, e.g. `{"method", "status"}`.
     *
     * \pre `name` does not contain `_total`.
     */
    counter_vec(std::string_view name, std::span<const std::string_view> labels)
    : base_type(name, labels)
    {
       BOOST_ASSERT_MSG(name.find("_total") == std::string_view::npos,
            "Counter name shouldn't contain \"_total\" suffix; it will be appended automatically");
    }

    /**
     * \overload
     * \param labels Label names owned as `std::string`.
     */
    counter_vec(std::string_view name, std::span<const std::string> labels)
    : base_type(name, labels)
    {
       BOOST_ASSERT_MSG(name.find("_total") == std::string_view::npos,
            "Counter name shouldn't contain \"_total\" suffix; it will be appended automatically");
    }

    /**
     * \overload
     * \param labels Label names as an owned `std::vector<std::string>`,
     *               moved into the metric.
     */
    counter_vec(std::string_view name, std::vector<std::string>&& labels)
    : base_type(name, std::move(labels))
    {
       BOOST_ASSERT_MSG(name.find("_total") == std::string_view::npos,
            "Counter name shouldn't contain \"_total\" suffix; it will be appended automatically");
    }

    /**
     * \overload
     * \param labels Initializer list of label names.
     */
    counter_vec(std::string_view name, std::initializer_list<std::string_view> labels)
    : counter_vec(name, std::span(labels)) {}
};

} // namespace boost::prometheus

#endif
