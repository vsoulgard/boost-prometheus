//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_COUNTER_INTERNAL_HPP
#define BOOST_PROMETHEUS_DETAIL_COUNTER_INTERNAL_HPP

#include <boost/prometheus/detail/basic_metric_internal.hpp>
#include <boost/prometheus/detail/string_buffer.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <boost/assert.hpp>

#include <atomic>
#include <cmath>

namespace boost::prometheus::detail {

template <typename AtomicPolicyType, bool HasPrefixSVs = true>
struct counter_internal : public basic_metric_internal<AtomicPolicyType, true, HasPrefixSVs>
{
    static constexpr string_buffer prometheus_type = "counter";
    static constexpr string_buffer openmetrics_type = "counter";

    using base_type = basic_metric_internal<AtomicPolicyType, true, HasPrefixSVs>;

    using value_type = std::conditional_t<
        AtomicPolicyType::enabled,
        std::atomic<double>,
        double
    >;

    alignas(base_type::template alignment<value_type>)
    value_type value_;

    counter_internal(std::string&& base_string)
    : base_type(std::move(base_string)),
    value_(0.0) {}

    counter_internal(std::string&& base_string, std::size_t openmetrics_prefix_offset)
    : base_type(std::move(base_string), openmetrics_prefix_offset),
    value_(0.0) {}

    consteval static bool is_atomic()
    {
        return AtomicPolicyType::enabled;
    }

    double get_value() const noexcept
    {
        if constexpr (AtomicPolicyType::enabled)
        {
            return value_.load(std::memory_order_relaxed);
        }
        else
        {
            return value_;
        }
    }

    void inc(double value) noexcept
    {
        BOOST_ASSERT_MSG(!std::signbit(value), "The counter increment must be a positive value");
        if constexpr (AtomicPolicyType::enabled)
        {
            value_.fetch_add(value, std::memory_order_relaxed);
        }
        else
        {
            value_ += value;
        }
    }
};

} // namespace boost::prometheus::detail

#endif
