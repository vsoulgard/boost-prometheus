//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_ALWAYS_FALSE_HPP
#define BOOST_PROMETHEUS_DETAIL_ALWAYS_FALSE_HPP

namespace boost::prometheus::detail {

template <typename T>
inline constexpr bool always_false_v = false;

} // namespace boost::prometheus::detail

#endif
