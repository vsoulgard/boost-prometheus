//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_TYPES_FWD_HPP
#define BOOST_PROMETHEUS_DETAIL_TYPES_FWD_HPP

#include <boost/prometheus/detail/string_buffer.hpp>

namespace boost::prometheus {

namespace detail {

template <typename, typename, typename>
class basic_metric_vec;

template <typename>
class static_gauge_internal;

template <typename>
class static_histogram_internal;

template <typename>
class static_info_internal;

}

class format;

template <typename>
class counter;

template <typename>
class counter_vec;

template <typename>
class gauge;

template <typename>
class gauge_vec;

template <typename>
class histogram;

template <typename>
class histogram_vec;

template <typename>
class gaugehistogram;

template <typename>
class gaugehistogram_vec;

template <typename>
class info_vec;

template <detail::string_buffer, typename...>
class static_counter;

template <detail::string_buffer, typename...>
class static_gauge;

template <detail::string_buffer, typename...>
class static_histogram;

template <detail::string_buffer, typename...>
class static_info;

} // namespace boost::prometheus

#endif
