//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_IMPL_INFO_IPP
#define BOOST_PROMETHEUS_IMPL_INFO_IPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/info.hpp>

namespace boost::prometheus {

info::internal_type info::init_internal(std::string_view name, const info_opts& options)
{
    auto prefixes = detail::serialize_header<
        detail::metric_type_string<info>::prometheus_type,
        detail::metric_type_string<info>::openmetrics_type
    >(name, options);
    return internal_type(std::move(prefixes.base_string), prefixes.openmetrics_prefix_offset);
}

} // namespace boost::prometheus

#endif
