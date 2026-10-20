//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_TUPLE_SORTING_HPP
#define BOOST_PROMETHEUS_DETAIL_TUPLE_SORTING_HPP

#include <tuple>

namespace boost::prometheus::detail {

template <auto... Vs>
struct value_tuple
{
    static constexpr std::size_t size = sizeof...(Vs);
};

template <std::size_t I, auto... Vs>
static constexpr auto element = std::get<I>(std::tuple{Vs...});

template <typename VT, std::size_t N, typename = std::make_index_sequence<N>>
struct value_tuple_take;

template <auto... Vs, std::size_t N, std::size_t... Is>
struct value_tuple_take<value_tuple<Vs...>, N, std::index_sequence<Is...>>
{
    using type = value_tuple<element<Is, Vs...>...>;
};

template <typename VT, std::size_t N, typename = std::make_index_sequence<VT::size - N>>
struct value_tuple_drop;

template <auto... Vs, std::size_t N, std::size_t... Is>
struct value_tuple_drop<value_tuple<Vs...>, N, std::index_sequence<Is...>>
{
    using type = value_tuple<element<N + Is, Vs...>...>;
};

template <auto V, typename VT>
struct value_tuple_prepend;

template <auto V, auto... Vs>
struct value_tuple_prepend<V, value_tuple<Vs...>>
{
    using type = value_tuple<V, Vs...>;
};

template <typename L, typename R>
struct merge_values;

template <auto... Rs>
struct merge_values<value_tuple<>, value_tuple<Rs...>>
{
    using type = value_tuple<Rs...>;
};

template <auto... Ls>
struct merge_values<value_tuple<Ls...>, value_tuple<>>
{
    using type = value_tuple<Ls...>;
};

template <auto L, auto... Ls, auto R, auto... Rs>
struct merge_values<value_tuple<L, Ls...>, value_tuple<R, Rs...>>
{
    using type = std::conditional_t<(L < R),
        typename value_tuple_prepend<L, 
            typename merge_values<value_tuple<Ls...>, value_tuple<R, Rs...>>::type>::type,
        typename value_tuple_prepend<R, 
            typename merge_values<value_tuple<L, Ls...>, value_tuple<Rs...>>::type>::type
    >;
};

template <typename VT>
struct sort_values_impl;

template <auto V>
struct sort_values_impl<value_tuple<V>>
{
    using type = value_tuple<V>;
};

// Compile-time sorting for tuple of different types.
// operator< should be overloaded for corresponding types.
template <auto... Vs>
struct sort_values_impl<value_tuple<Vs...>> 
{
    static constexpr std::size_t N = sizeof...(Vs), Mid = N / 2;

    using left  = typename value_tuple_take<value_tuple<Vs...>, Mid>::type;
    using right = typename value_tuple_drop<value_tuple<Vs...>, Mid>::type;
    using type = typename merge_values<
        typename sort_values_impl<left>::type, 
        typename sort_values_impl<right>::type>::type;
};

template <typename VT>
struct to_std_tuple;

template <auto... Vs>
struct to_std_tuple<value_tuple<Vs...>>
{
    using type = std::tuple<decltype(Vs)...>;

    static consteval auto make() { return type{Vs...}; }
};

} // namespace boost::prometheus::detail

#endif
