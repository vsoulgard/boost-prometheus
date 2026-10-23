//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_STATIC_COUNTER_HPP
#define BOOST_PROMETHEUS_STATIC_COUNTER_HPP

#include <boost/prometheus/detail/basic_static_metric.hpp>
#include <boost/prometheus/detail/const_find_substr.hpp>
#include <boost/prometheus/detail/friend_registration.hpp>
#include <boost/prometheus/detail/static_gauge_internal.hpp>
#include <boost/prometheus/detail/static_metric_registrator.hpp>
#include <boost/prometheus/detail/string_buffer.hpp>

#include <boost/assert.hpp>

#include <atomic>

namespace boost::prometheus {

template <detail::string_buffer MetricName, typename... Options>
class static_counter : public detail::basic_static_metric<
    static_counter,
    detail::static_gauge_internal,
    MetricName,
    Options...
>
{
    static_assert(
        !detail::const_find_substr<MetricName, "_total">(),
        "Counter name shouldn't contain \"_total\" suffix; it will be appended automatically");

    using base_type = detail::basic_static_metric<
        static_counter,
        detail::static_gauge_internal,
        MetricName,
        Options...
    >;

    BOOST_PROMETHEUS_FRIEND_REGISTRATION()

public:

    template <detail::string_buffer... LabelValues>
    static double get_value()
    {
        double& value_ref = base_type::template value_ref<LabelValues...>();

        if constexpr (base_type::is_atomic())
        {
            std::atomic_ref<double> atomic_value(value_ref);
            return atomic_value.load(std::memory_order_relaxed);
        }
        else
        {
            return value_ref;
        }
    }

    template <detail::string_buffer... LabelValues>
    static void inc(double value = 1.0)
    {
        BOOST_ASSERT_MSG(value >= 0.0, "static_counter::inc() accepts only positive values");

        double& value_ref = base_type::template value_ref<LabelValues...>();

        if constexpr (base_type::is_atomic())
        {
            std::atomic_ref<double> atomic_value(value_ref);
            atomic_value.fetch_add(value);
        }
        else
        {
            value_ref += value;
        }
    }
};

} // namespace boost::prometheus

#endif
