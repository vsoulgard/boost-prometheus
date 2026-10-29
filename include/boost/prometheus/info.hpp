//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_INFO_HPP
#define BOOST_PROMETHEUS_INFO_HPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/info_internal.hpp>

#include <boost/prometheus/detail/export/serialize_header.hpp>
#include <boost/prometheus/detail/export/serialize_name_with_labels.hpp>

#include <boost/assert.hpp>

#include <span>
#include <string>
#include <vector>

namespace boost::prometheus {

/**
 * \brief Info metric with a fixed set of labels.
 *
 * An `info` is a single info metric whose name and label values are fixed
 * at construction. It is the non-vector counterpart of \ref info_vec: where
 * `info_vec` resolves label combinations dynamically via `with_labels(...)`,
 * `info` fixes them once and exposes the resulting series directly.
 *
 * Info metrics expose static, rarely-changing metadata — build information,
 * version, commit hash, configuration details — as a constant value `1`
 * with a set of labels. They are not meant to be updated frequently or used
 * as counters; the label values themselves carry the information.
 *
 * In the exposition output, the `_info` suffix is appended automatically:
 * an `info` constructed with the name `build` is exposed as `build_info`.
 *
 * \par Immutable after construction
 * Unlike \ref counter, \ref gauge, and \ref histogram, `info` has no mutating
 * API. There is no `set()`, `inc()`, or `get_value()`: the label values are
 * the payload, and the metric value is always `1`. Once constructed, the
 * object is fully immutable and safe to read from any number of threads
 * without synchronization. This is also why `info` is not templated on
 * `atomic_policy` — there is no mutable state to protect.
 *
 * \par Label order
 * Labels are sorted lexicographically in the serialized series name,
 * independently of the order in which label names and values are passed to
 * the constructor. This matches the Prometheus and OpenMetrics requirements
 * and makes the output stable.
 *
 * \par Info metrics MUST have an empty unit
 * `info_opts` intentionally has no `unit` field: OpenMetrics specifies
 * that Info metric families have no unit, and the `# UNIT` line is not
 * emitted for them.
 *
 * \par Example
 * \code
 * prometheus::info app_info(
 *     "app",                                   // base name
 *     {.help = "Application metadata"},        // options
 *     {"version", "commit"},                   // label names
 *     {"1.0", "abc123"});                      // label values
 *
 * // Exposed as:
 * //   app_info{commit="abc123",version="1.0"} 1
 * \endcode
 */
class info
{
    using internal_type = detail::info_internal<true>;

    internal_type internal_;
    
    std::string series_name_;

    // Serialization
    friend struct boost::prometheus::format;

    static internal_type init_internal(std::string_view name, const info_opts& options)
    {
        auto prefixes = detail::serialize_header<
            internal_type::prometheus_type,
            internal_type::openmetrics_type
        >(name, options);
        return internal_type(std::move(prefixes.base_string), prefixes.openmetrics_prefix_offset);
    }

    template <typename SpanType1, typename SpanType2>
    static std::string init_series_name(std::string_view name, SpanType1&& labels, SpanType2&& labelvalues)
    {
        BOOST_ASSERT_MSG(labels.size() == labelvalues.size(), "Incorrect label values count");
        std::vector<std::pair<std::string, std::size_t>> labels_with_idx;
        labels_with_idx.reserve(labels.size());
        for (std::size_t i = 0; i < labels.size(); i++)
        {
            if constexpr (std::is_lvalue_reference_v<SpanType1>)
                labels_with_idx.emplace_back(labels[i], i);
            else
                labels_with_idx.emplace_back(std::move(labels[i]), i);
        }
        // Sort labels
        std::sort(labels_with_idx.begin(), labels_with_idx.end());

        return detail::serialize_name_with_labels<"_info">(
            name,
            labels_with_idx,
            labelvalues
        );
    }

public:

    info() = delete;

    /**
     * \brief Constructs an info metric from a span of label names and
     *        values.
     *
     * \param name         Base metric name, e.g. `"build"`. The `_info`
     *                     suffix is appended automatically in the exposition.
     * \param options      `help` only. See \ref info_opts.
     * \param labels       Label names, e.g. `{"version", "commit"}`. Must
     *                     outlive the construction; only the contents are
     *                     copied.
     * \param labelvalues  Label values, in the same order as \p labels.
     *                     Must outlive the construction; only the contents
     *                     are copied.
     *
     * \pre `labels.size() == labelvalues.size()`.
     */
    info(std::string_view name,
        info_opts&& options,
        std::span<const std::string_view> labels,
        std::span<const std::string_view> labelvalues)
    : internal_(init_internal(name, options)),
    series_name_(init_series_name(name, labels, labelvalues)) {}

    /**
     * \overload
     * \param labels      Label names owned as `std::string`.
     * \param labelvalues Label values owned as `std::string`.
     */
    info(std::string_view name,
        info_opts&& options,
        std::span<const std::string> labels,
        std::span<const std::string> labelvalues)
    : internal_(init_internal(name, options)),
    series_name_(init_series_name(name, labels, labelvalues)) {}

    /**
     * \overload
     * \param labels      Label names as an owned `std::vector<std::string>`,
     *                    moved into the metric.
     * \param labelvalues Label values as an owned `std::vector<std::string>`,
     *                    moved into the metric.
     */
    info(std::string_view name,
        info_opts&& options,
        std::vector<std::string>&& labels,
        std::vector<std::string>&& labelvalues)
    : internal_(init_internal(name, options)),
    series_name_(init_series_name(name, std::move(labels), std::move(labelvalues))) {}

    /**
     * \overload
     * \param labels      Initializer list of label names. Copied into
     *                    internal storage.
     * \param labelvalues Initializer list of label values, in the same
     *                    order as \p labels.
     */
    info(std::string_view name,
        info_opts&& options,
        std::initializer_list<const std::string_view> labels,
        std::initializer_list<const std::string_view> labelvalues)
    : info(name, std::move(options), std::span(labels), std::span(labelvalues)) {}
};

} // namespace boost::prometheus

#endif
