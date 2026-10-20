//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_REGISTRY_HPP
#define BOOST_PROMETHEUS_REGISTRY_HPP

#include <boost/prometheus/detail/tuple_unordered_flat_sets.hpp>

#include <boost/prometheus/counter.hpp>
#include <boost/prometheus/counter_vec.hpp>
#include <boost/prometheus/gauge.hpp>
#include <boost/prometheus/gauge_vec.hpp>
#include <boost/prometheus/gaugehistogram.hpp>
#include <boost/prometheus/gaugehistogram_vec.hpp>
#include <boost/prometheus/histogram.hpp>
#include <boost/prometheus/histogram_vec.hpp>
#include <boost/prometheus/info_vec.hpp>
#include <boost/prometheus/metric_options.hpp>

namespace boost::prometheus {

template <typename AtomicPolicyType = default_atomic_policy>
class registry
{
    using counter_ptr_type = counter<AtomicPolicyType>*;
    using counter_vec_ptr_type = counter_vec<AtomicPolicyType>*;
    using gauge_ptr_type = gauge<AtomicPolicyType>*;
    using gauge_vec_ptr_type = gauge_vec<AtomicPolicyType>*;
    using histogram_ptr_type = histogram<AtomicPolicyType>*;
    using histogram_vec_ptr_type = histogram_vec<AtomicPolicyType>*;
    using gaugehistogram_ptr_type = gaugehistogram<AtomicPolicyType>*;
    using gaugehistogram_vec_ptr_type = gaugehistogram_vec<AtomicPolicyType>*;
    using info_vec_ptr_type = info_vec<AtomicPolicyType>*;

    // Each type has independent std::mutex with atomic_policy<true>
    detail::tuple_unordered_flat_sets<AtomicPolicyType,
        counter_ptr_type,
        counter_vec_ptr_type,
        gauge_ptr_type,
        gauge_vec_ptr_type,
        histogram_ptr_type,
        histogram_vec_ptr_type,
        gaugehistogram_ptr_type,
        gaugehistogram_vec_ptr_type,
        info_vec_ptr_type> 
    metrics_;

public:

    // Used in serialization
    template <typename Func>
    void for_each(Func&& func)
    {
        metrics_.for_each([&](auto* metric_ptr)
        {
            func(*metric_ptr);
        });
    }

    static consteval bool is_atomic()
    {
        return AtomicPolicyType::enabled;
    }

    template <typename MetricType>
    void register_metric(MetricType& metric)
    {
        static_assert(is_atomic() == MetricType::is_atomic(), "Registry and metric type must share the same atomic policy.");
        // Insert metric address
        metrics_.insert(&metric);
    }

    template <typename MetricType>
    void unregister_metric(MetricType& metric)
    {
        static_assert(is_atomic() == MetricType::is_atomic(), "Registry and metric type must share the same atomic policy.");
        metrics_.erase(&metric);
    }
};

} // namespace boost::prometheus

#endif
