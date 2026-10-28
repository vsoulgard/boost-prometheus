//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_INFO_VEC_HPP
#define BOOST_PROMETHEUS_INFO_VEC_HPP

#include <boost/prometheus/info_item.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/basic_metric_vec.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <boost/prometheus/detail/export/serialize_name_with_labels.hpp>

namespace boost::prometheus {

/**
 * \brief Info metric with dynamically specified labels.
 *
 * An `info_vec` is a family of info metrics that share one name and one
 * set of label names. Individual series are obtained via `with_labels(...)`,
 * which returns a reference to an `info_item` that writes to the shared
 * storage of the vector.
 *
 * Info metrics expose static, rarely-changing metadata — build information,
 * version, commit hash, configuration details — as a constant value `1` with
 * a set of labels. They are not meant to be updated frequently or used as
 * counters; the label values themselves carry the information.
 *
 * In the exposition output, the `_info` suffix is appended automatically:
 * an `info_vec` constructed with the name `build` is exposed as `build_info`.
 *
 * Resolved label combinations are cached: the first call to
 * `with_labels()` with a given set of values creates the series, and
 * subsequent calls with the same values return the same reference.
 *
 * \tparam AtomicPolicyType Compile-time atomicity policy. The default,
 *         `default_atomic_policy`, makes updates to individual items
 *         thread-safe. Use `atomic_policy<false>` when updates are
 *         externally synchronized. In practice, info metrics are usually
 *         set once at startup, so atomicity rarely matters.
 *
 * \par Thread safety
 * With the default atomic policy, all of the following are thread-safe and
 * may be called concurrently from multiple threads:
 *
 * * `with_labels(...)` — resolving a label combination, including the
 *   first call that creates the series;
 * * updating the label values of a resolved item.
 *
 * Calls to `with_labels()` and item updates may be freely mixed.
 *
 * With `atomic_policy<false>`, neither `with_labels()` nor item updates are
 * synchronized. In that mode, externally serialize all calls into the
 * vector, including the first `with_labels()` for any given label
 * combination.
 *
 * In practice, info metrics are usually populated once at startup, so
 * thread-safety of updates rarely matters — but `with_labels()` itself is
 * commonly called from request handlers, and benefits from the default
 * atomic policy.
 *
 * \par Info metrics MUST have an empty unit
 * `info_opts` intentionally has no `unit` field: OpenMetrics specifies
 * that Info metric families have no unit, and the `# UNIT` line is not
 * emitted for them.
 *
 * \par Example
 * \code
 * prometheus::info_vec build_info(
 *     "build",                            // base name
 *     {.help = "Build information"},      // options
 *     {"build_type", "version", "commit"}); // label names
 *
 * build_info.with_labels({"Release", "1.0", "abc123"});
 * build_info.with_labels({"Debug", "1.0", "abc123"});
 *
 * // Exposed as:
 * //   build_info{build_type="Release",commit="abc123",version="1.0"} 1
 * //   build_info{build_type="Debug",commit="abc123",version="1.0"}   1
 * \endcode
 */
template <typename AtomicPolicyType = default_atomic_policy>
class info_vec : public detail::basic_metric_vec<
    info_vec<AtomicPolicyType>,
    info_item<AtomicPolicyType>,
    info_opts
>
{
    using item_type = info_item<AtomicPolicyType>;
    using base_type = detail::basic_metric_vec<
        info_vec<AtomicPolicyType>,
        item_type,
        info_opts
    >;
    friend base_type;

    // Serialization
    friend struct boost::prometheus::format;

    // Called in base_type::with_labels_impl
    template <typename LabelsType, typename LabelValuesType>
    std::string init_params(std::string_view metric_name, const LabelsType& labels, const LabelValuesType& labelvalues)
    {
        return detail::serialize_name_with_labels<"_info">(
            metric_name,
            labels,
            labelvalues
        );
    }

public:

    /**
     * \brief Constructs an info vector from a span of label names.
     *
     * \param name    Base metric name, e.g. `"build"`. The `_info` suffix
     *                is appended automatically in the exposition.
     * \param options `help` only. See \ref info_opts.
     * \param labels  Label names, e.g. `{"build_type", "version"}`. Must
     *                outlive the construction; only the contents are copied.
     */
    info_vec(std::string_view name, info_opts&& options, std::span<const std::string_view> labels)
    : base_type(name, std::move(options), labels) {}

    /**
     * \overload
     * \param labels Label names owned as `std::string`.
     */
    info_vec(std::string_view name, info_opts&& options, std::span<const std::string> labels)
    : base_type(name, std::move(options), labels) {}

    /**
     * \overload
     * \param labels Label names as an owned `std::vector<std::string>`,
     *               moved into the metric.
     */
    info_vec(std::string_view name, info_opts&& options, std::vector<std::string>&& labels)
    : base_type(name, std::move(options), std::move(labels)) {}

    /**
     * \overload
     * \param labels Initializer list of label names. The list is copied
     *               into internal storage; it need not outlive the
     *               constructor.
     */
    info_vec(std::string_view name, info_opts&& options, std::initializer_list<std::string_view> labels)
    : info_vec(name, std::move(options), std::span(labels)) {}

    /**
     * \brief Constructs an info vector with default options.
     *
     * Equivalent to calling the options-taking overload with `info_opts{}`.
     *
     * \param name   Base metric name. The `_info` suffix is appended
     *               automatically in the exposition.
     * \param labels Label names, e.g. `{"build_type", "version"}`.
     */
    info_vec(std::string_view name, std::span<const std::string_view> labels)
    : base_type(name, labels) {}

    /**
     * \overload
     * \param labels Label names owned as `std::string`.
     */
    info_vec(std::string_view name, std::span<const std::string> labels)
    : base_type(name, labels) {}

    /**
     * \overload
     * \param labels Label names as an owned `std::vector<std::string>`,
     *               moved into the metric.
     */
    info_vec(std::string_view name, std::vector<std::string>&& labels)
    : base_type(name, std::move(labels)) {}

    /**
     * \overload
     * \param labels Initializer list of label names.
     */
    info_vec(std::string_view name, std::initializer_list<std::string_view> labels)
    : info_vec(name, std::span(labels)) {}
};

} // namespace boost::prometheus

#endif
