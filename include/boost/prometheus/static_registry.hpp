//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_STATIC_REGISTRY_HPP
#define BOOST_PROMETHEUS_STATIC_REGISTRY_HPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/friend_registration.hpp>
#include <boost/prometheus/detail/static_gauge_internal.hpp>
#include <boost/prometheus/detail/static_histogram_internal.hpp>
#include <boost/prometheus/detail/static_info_internal.hpp>
#include <boost/prometheus/detail/static_metric_registrator.hpp>
#include <boost/prometheus/detail/string_buffer.hpp>
#include <boost/prometheus/detail/tuple_vectors.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

namespace boost::prometheus {

template <detail::string_buffer RegistryName = default_static_registry_name,
    typename AtomicPolicyType = default_atomic_policy>
class static_registry
{
    using gauge_internal_ptr_type = detail::static_gauge_internal<AtomicPolicyType>*;
    using histogram_internal_ptr_type = detail::static_histogram_internal<AtomicPolicyType>*;
    using info_internal_ptr_type = detail::static_info_internal<AtomicPolicyType>*;

    // Plain vectors, without any locks: the whole registry
    // is constant by design once the program enters main().
    inline static detail::tuple_vectors<
        gauge_internal_ptr_type,
        histogram_internal_ptr_type,
        info_internal_ptr_type>
    metrics_;

    template <typename RegistryType, typename MetricType>
    friend struct detail::static_metric_registrator;

    // Serialization
    friend class boost::prometheus::format;

public:

    // Used in serialization
    template <typename Func>
    static void for_each(Func&& func)
    {
        metrics_.for_each([&](auto* metric_ptr)
        {
            func(*metric_ptr);
        });
    }

    static consteval const char* name()
    {
        return RegistryName.value;
    }

    static consteval bool is_atomic()
    {
        return AtomicPolicyType::enabled;
    }

    static std::size_t size() noexcept
    {
        return metrics_.size();
    }

    template <typename MetricType>
    static void register_metric()
    {
        static_assert(is_atomic() == MetricType::is_atomic(), "Registry and metric type must share the same atomic policy.");
        // Initialize static metric
        (void)detail::static_metric_registrator<static_registry, MetricType>::_;
    }
};

} // namespace boost::prometheus

#endif
