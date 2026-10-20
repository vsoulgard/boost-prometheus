//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_INFO_INTERNAL_HPP
#define BOOST_PROMETHEUS_DETAIL_INFO_INTERNAL_HPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/basic_metric_internal.hpp>
#include <boost/prometheus/detail/string_buffer.hpp>

namespace boost::prometheus::detail {

template <bool HasPrefixSVs = true>
struct info_internal : public basic_metric_internal<atomic_policy<false>, true, HasPrefixSVs>
{
    static constexpr string_buffer prometheus_type = "gauge";
    static constexpr string_buffer openmetrics_type = "info";

    using base_type = basic_metric_internal<atomic_policy<false>, true, HasPrefixSVs>;

    info_internal(std::string&& base_string)
    : base_type(std::move(base_string)) {}

    info_internal(std::string&& base_string, std::size_t openmetrics_prefix_offset)
    : base_type(std::move(base_string), openmetrics_prefix_offset) {}
};

} // namespace boost::prometheus::detail

#endif
