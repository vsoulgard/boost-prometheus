//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_EXPORT_SERIALIZE_HEADER_HPP
#define BOOST_PROMETHEUS_DETAIL_EXPORT_SERIALIZE_HEADER_HPP

#include <boost/prometheus/detail/export/serialize_prefix.hpp>

#include <cstring>
#include <string>

namespace boost::prometheus::detail {

// Serialize header only
template <string_buffer PrometheusTypeStr, string_buffer OpenMetricsTypeStr, string_buffer Suffix = "", typename OptionsType>
serialize_prefix_result serialize_header(std::string_view name, const OptionsType& options)
{
    return serialize_prefix<PrometheusTypeStr, OpenMetricsTypeStr, false, false, false, Suffix>(name, options);
}

} // namespace boost::prometheus::detail

#endif
