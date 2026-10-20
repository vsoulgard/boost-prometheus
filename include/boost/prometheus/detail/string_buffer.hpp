//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_STRING_BUFFER_HPP
#define BOOST_PROMETHEUS_DETAIL_STRING_BUFFER_HPP

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace boost::prometheus::detail {

// String buffer class for compile-time calculations
template <std::size_t N>
struct string_buffer
{
    char value[N];

    consteval string_buffer() = default;

    consteval string_buffer(const char (&str)[N])
    {
        std::copy_n(str, N, value);
    }

    consteval std::size_t size() const { return N - 1; }

    constexpr operator std::string_view() const
    {
        return std::string_view(value, N - 1);
    }

    // For compile-time sorting
    template <std::size_t M>
    friend consteval bool operator<(const string_buffer<N> lhs, const string_buffer<M>& rhs)
    {
        return std::string_view(lhs.value, lhs.size()) < std::string_view(rhs.value, rhs.size());
    }

    // Compile-time concat for compile-time text format serialization
    template <std::size_t M>
    friend consteval auto operator+(string_buffer<N> lhs, string_buffer<M> rhs)
    {
        string_buffer<N + M - 1> result;

        std::copy_n(lhs.value, N - 1, result.value);
        std::copy_n(rhs.value, M, result.value + N - 1);

        return result;
    }

    template <std::size_t M>
    friend consteval auto operator+(string_buffer<N> lhs, const char (&rhs)[M])
    {
        string_buffer<N + M - 1> result;

        std::copy_n(lhs.value, N - 1, result.value);
        std::copy_n(rhs, M, result.value + N - 1);

        return result;
    }

    template <std::size_t M>
    friend consteval auto operator+(const char (&lhs)[M], string_buffer<N> rhs)
    {
        string_buffer<N + M - 1> result;

        std::copy_n(lhs, M - 1, result.value);
        std::copy_n(rhs.value, N, result.value + M - 1);

        return result;
    }
};

} // namespace boost::prometheus::detail

#endif
