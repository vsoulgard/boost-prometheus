//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_STATIC_GAUGE_INTERNAL_HPP
#define BOOST_PROMETHEUS_DETAIL_STATIC_GAUGE_INTERNAL_HPP

#include <boost/prometheus/detail/config.hpp>

#include <boost/container/small_vector.hpp>

#include <atomic>
#include <vector>
#include <string_view>

namespace boost::prometheus::detail {

template <typename AtomicPolicyType>
struct static_gauge_internal
{
    template <typename T>
    static constexpr std::size_t alignment = AtomicPolicyType::enabled ? cache_line_size : alignof(T);

    struct gauge_entry
    {
        // Metric name in text format
        std::string_view formatted_name;
        alignas(alignment<double>)
        double value;

        double get_value() const noexcept
        {
            if constexpr (AtomicPolicyType::enabled)
            {
                std::atomic_ref<const double> atomic_value(value);
                return atomic_value.load(std::memory_order_relaxed);
            }
            else
            {
                return value;
            }
        }

        gauge_entry(std::string_view formatted_name) : formatted_name(formatted_name), value(0.0) {}
    };

    std::string_view prometheus_header_;
    std::string_view openmetrics_header_;
    boost::container::small_vector<gauge_entry, 1> entries_;

    static_gauge_internal(std::string_view prometheus_header, std::string_view openmetrics_header) :
        prometheus_header_(prometheus_header), 
        openmetrics_header_(openmetrics_header) {}

    std::size_t size() const noexcept
    {
        return entries_.size();
    }

    // Returns index of an entry
    std::size_t add_entry(std::string_view formatted_name)
    {
        entries_.push_back(formatted_name);
        return entries_.size() - 1;
    }

    double& value_ref(std::size_t index) noexcept
    {
        return entries_[index].value;
    }
};

} // namespace boost::prometheus::detail

#endif
