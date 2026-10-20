//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef PROMFAST_DETAIL_STATIC_LABELPAIR_HPP
#define PROMFAST_DETAIL_STATIC_LABELPAIR_HPP

namespace boost::prometheus::detail {

// Wrapper for labelpairs sorting,
// which support operator< for different types.
template <typename T1, typename T2>
struct static_labelpair
{
    T1 first;
    T2 second;
    constexpr static_labelpair(T1 f, T2 s) : first(f), second(s) {}
};

template <typename A1, typename A2, typename B1, typename B2>
consteval bool operator<(const static_labelpair<A1, A2>& a, const static_labelpair<B1, B2>& b)
{
    return a.first < b.first;
}

} // namespace boost::prometheus::detail

#endif
