//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_STATIC_METRIC_REGISTRATOR_HPP
#define BOOST_PROMETHEUS_DETAIL_STATIC_METRIC_REGISTRATOR_HPP

namespace boost::prometheus::detail {

template <typename StaticRegistryType, typename StaticMetricType>
struct static_metric_registrator
{
    inline static bool _ = []()
    {
        StaticRegistryType::metrics_.push_back(&StaticMetricType::impl_);

        return true;
    }();
};

} // namespace boost::prometheus::detail

#endif
