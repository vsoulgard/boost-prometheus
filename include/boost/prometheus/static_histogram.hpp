//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_STATIC_HISTOGRAM_HPP
#define BOOST_PROMETHEUS_STATIC_HISTOGRAM_HPP

#include <boost/prometheus/detail/basic_static_metric.hpp>
#include <boost/prometheus/detail/friend_registration.hpp>
#include <boost/prometheus/detail/static_histogram_internal.hpp>
#include <boost/prometheus/detail/static_metric_registrator.hpp>

#include <boost/prometheus/metric_options.hpp>

#include <atomic>

namespace boost::prometheus {

template <detail::string_buffer MetricName, typename... Options>
class static_histogram : public detail::basic_static_metric<
    static_histogram,
    detail::static_histogram_internal,
    MetricName,
    Options...
>
{
    using base_type = detail::basic_static_metric<
        static_histogram,
        detail::static_histogram_internal,
        MetricName,
        Options...
    >;

    static_assert([]() consteval
    {
        if constexpr (std::is_void_v<typename base_type::labels_type>)
            return true;
        else
            return ![]<std::size_t... Is>(std::index_sequence<Is...>) consteval
            {
                return ((std::string_view(std::get<Is>(base_type::labels_type::values)) == "le") || ...);
            }(std::make_index_sequence<base_type::labels_type::size()>{});
    }(), "Label \"le\" is reserved for histogram buckets");

    using buckets_type = detail::fallback_if_void_t<
        typename detail::extract_option<buckets, Options...>::type,
        default_buckets
    >;

    BOOST_PROMETHEUS_FRIEND_REGISTRATION()

public:

    template <detail::string_buffer... LabelValues>
    static void observe(double value, std::size_t count = 1)
    {
        auto& value_ref = base_type::template value_ref<LabelValues...>();
        // Find bucket index
        std::size_t bucket_idx = buckets_type::get_bucket_index(value);
        value_ref.observe(value, count, bucket_idx);
    }

    template <detail::string_buffer... LabelValues>
    static void reset()
    {
        auto& value_ref = base_type::template value_ref<LabelValues...>();
        
    }
};

} // namespace boost::prometheus

#endif
