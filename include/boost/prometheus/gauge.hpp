//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_GAUGE_HPP
#define BOOST_PROMETHEUS_GAUGE_HPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/gauge_internal.hpp>

#include <boost/prometheus/detail/export/serialize_prefix.hpp>

namespace boost::prometheus {

template <typename AtomicPolicyType = default_atomic_policy>
class gauge
{
    using internal_type = detail::gauge_internal<AtomicPolicyType>;

    internal_type internal_;

    // Serialization
    friend class boost::prometheus::format;

    static internal_type init_internal(std::string_view name, const metric_opts& options)
    {
        auto prefixes = detail::serialize_prefix<
            internal_type::prometheus_type,
            internal_type::openmetrics_type
        >(name, options);
        return internal_type(std::move(prefixes.base_string), prefixes.openmetrics_prefix_offset);
    }

public:

    gauge() = delete;

    gauge(std::string_view name, metric_opts&& options = {})
    : internal_(init_internal(name, options)) {}

    consteval static bool is_atomic()
    {
        return internal_type::is_atomic();
    }

    double get_value() const noexcept
    {
        return internal_.get_value();
    }

    void inc(double value = 1.0) noexcept
    {
        internal_.inc(value);
    }

    void dec(double value = 1.0) noexcept
    {
        internal_.dec(value);
    }

    void set(double value) noexcept
    {
        internal_.set(value);
    }
};

} // namespace boost::prometheus

#endif
