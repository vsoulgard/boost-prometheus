//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_COUNTER_HPP
#define BOOST_PROMETHEUS_COUNTER_HPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/counter_internal.hpp>

#include <boost/prometheus/detail/export/serialize_prefix.hpp>

namespace boost::prometheus {

template <typename AtomicPolicyType = default_atomic_policy>
class counter
{
    using internal_type = detail::counter_internal<AtomicPolicyType>;

    internal_type internal_;

    // Serialization
    friend class boost::prometheus::format;

    static internal_type init_internal(std::string_view name, const metric_opts& options)
    {
        BOOST_ASSERT_MSG(name.find("_total") == std::string_view::npos,
            "Counter name shouldn't contain \"_total\" suffix; it will be appended automatically");
        auto prefixes = detail::serialize_prefix<
            internal_type::prometheus_type,
            internal_type::openmetrics_type
        >(name, options);
        return internal_type(std::move(prefixes.base_string), prefixes.openmetrics_prefix_offset);
    }

public:

    counter() = delete;

    counter(std::string_view name, metric_opts&& options = {})
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
};

} // namespace boost::prometheus

#endif
