//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_STATIC_INFO_HPP
#define BOOST_PROMETHEUS_STATIC_INFO_HPP

#include <boost/prometheus/detail/basic_static_metric.hpp>
#include <boost/prometheus/detail/friend_registration.hpp>
#include <boost/prometheus/detail/static_info_internal.hpp>
#include <boost/prometheus/detail/static_metric_registrator.hpp>
#include <boost/prometheus/detail/string_buffer.hpp>

#include <boost/assert.hpp>

namespace boost::prometheus {

template <detail::string_buffer MetricName, typename... Options>
class static_info : public detail::basic_static_metric<
    static_info,
    detail::static_info_internal,
    MetricName,
    Options...
>
{
    using base_type = detail::basic_static_metric<
        static_info,
        detail::static_info_internal,
        MetricName,
        Options...
    >;

    using unit_type = detail::extract_option<unit, Options...>::type;
    static_assert([]() consteval
    {
        if constexpr (std::is_void_v<unit_type>)
        {
            return true;
        }
        else
        {
            return false;
        }
    }(), "Info MUST not have an Unit");

    BOOST_PROMETHEUS_FRIEND_REGISTRATION()

public:

    template <detail::string_buffer... LabelValues>
    static void create()
    {
        (void)base_type::template value_ref<LabelValues...>();
    }
};

} // namespace boost::prometheus

#endif
