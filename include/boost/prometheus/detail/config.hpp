//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_CONFIG_HPP
#define BOOST_PROMETHEUS_DETAIL_CONFIG_HPP

#include <cstdint>

#ifndef BOOST_PROMETHEUS_CACHE_LINE_SIZE
#define BOOST_PROMETHEUS_CACHE_LINE_SIZE 64
#endif

namespace boost::prometheus::detail {

constexpr std::size_t cache_line_size = BOOST_PROMETHEUS_CACHE_LINE_SIZE;

} // namespace boost::prometheus::detail

#endif
