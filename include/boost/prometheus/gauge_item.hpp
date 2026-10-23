//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_GAUGE_ITEM_HPP
#define BOOST_PROMETHEUS_GAUGE_ITEM_HPP

#include <boost/prometheus/detail/gauge_internal.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

namespace boost::prometheus {

// Item type for `gauge_vec`
template <typename AtomicPolicyType>
class gauge_item
{
    using internal_type = detail::gauge_internal<AtomicPolicyType, false>;

    internal_type internal_;

    template <typename, typename, typename>
    friend class detail::basic_metric_vec;

    // Serialization
    friend struct boost::prometheus::format;

public:

    gauge_item() = delete;

    gauge_item(std::string&& formatted_name)
    : internal_(std::move(formatted_name)) {}

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
