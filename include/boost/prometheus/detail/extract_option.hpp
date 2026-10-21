//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_EXTRACT_OPTION_HPP
#define BOOST_PROMETHEUS_DETAIL_EXTRACT_OPTION_HPP

#include <boost/prometheus/detail/is_specialization_of.hpp>

namespace boost::prometheus::detail {

template <template <typename> class Pred, typename... Ts>
struct find_first;

template <template <typename> class Pred, typename T, typename... Rest>
struct find_first<Pred, T, Rest...>
{
    using type = std::conditional_t<Pred<T>::value, T, typename find_first<Pred, Rest...>::type>;
};

template <template <typename> class Pred>
struct find_first<Pred>
{
    using type = void;
};

template <template <auto...> typename Option, typename... Options>
struct extract_option
{
    template <typename T>
    struct is_option : is_specialization_of<T, Option> {};

    // extracted type specialization or void_t
    using type = find_first<is_option, Options...>::type;
};

template <template <typename> typename Option, typename... Options>
struct extract_type_option
{
    template <typename T>
    struct is_option : is_type_specialization_of<T, Option> {};

    using type = find_first<is_option, Options...>::type;
};

template <typename T, typename FallbackType>
using fallback_if_void_t = std::conditional_t<std::is_void_v<T>, FallbackType, T>;

} // namespace boost::prometheus::detail

#endif
