//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_COUNTER_ITEM_HPP
#define BOOST_PROMETHEUS_COUNTER_ITEM_HPP

#include <boost/prometheus/detail/counter_internal.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

namespace boost::prometheus {

// Item type for `counter_vec`
template <typename AtomicPolicyType>
class counter_item
{
    using internal_type = detail::counter_internal<AtomicPolicyType, false>;

    internal_type internal_;

    template <typename, typename, typename>
    friend class detail::basic_metric_vec;

    // Serialization
    friend struct boost::prometheus::format;

public:

    counter_item() = delete;

    counter_item(std::string&& formatted_name)
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
};

} // namespace boost::prometheus

#endif
