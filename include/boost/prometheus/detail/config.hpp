//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_CONFIG_HPP
#define BOOST_PROMETHEUS_DETAIL_CONFIG_HPP

#include <cstddef>

#ifndef BOOST_PROMETHEUS_CACHE_LINE_SIZE
#define BOOST_PROMETHEUS_CACHE_LINE_SIZE 64
#endif

#if defined(_MSC_VER)
    #define BOOST_PROMETHEUS_MSVC_DISABLE_ALIGN_WARNING \
        __pragma(warning(push)) __pragma(warning(disable: 4324))
    #define BOOST_PROMETHEUS_MSVC_RESTORE_WARNING \
        __pragma(warning(pop))
#else
    #define BOOST_PROMETHEUS_MSVC_DISABLE_ALIGN_WARNING
    #define BOOST_PROMETHEUS_MSVC_RESTORE_WARNING
#endif

#if defined(__GNUC__) && !defined(__clang__)
    #define BOOST_PROMETHEUS_GCC_DISABLE_STRINGOP_OVERFLOW_WARNING \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wstringop-overflow\"")
    #define BOOST_PROMETHEUS_GCC_RESTORE_STRINGOP_OVERFLOW_WARNING \
        _Pragma("GCC diagnostic pop")
    #define BOOST_PROMETHEUS_GCC_DISABLE_RESTRICT_WARNING \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wrestrict\"")
    #define BOOST_PROMETHEUS_GCC_RESTORE_RESTRICT_WARNING \
        _Pragma("GCC diagnostic pop")
#else
    #define BOOST_PROMETHEUS_GCC_DISABLE_STRINGOP_OVERFLOW_WARNING
    #define BOOST_PROMETHEUS_GCC_RESTORE_STRINGOP_OVERFLOW_WARNING
    #define BOOST_PROMETHEUS_GCC_DISABLE_RESTRICT_WARNING
    #define BOOST_PROMETHEUS_GCC_RESTORE_RESTRICT_WARNING
#endif

namespace boost::prometheus::detail {

constexpr std::size_t cache_line_size = BOOST_PROMETHEUS_CACHE_LINE_SIZE;

} // namespace boost::prometheus::detail

#endif
