//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_SSL_TRAITS_HPP
#define BOOST_PROMETHEUS_DETAIL_SSL_TRAITS_HPP

#include <boost/prometheus/detail/ssl_fwd.hpp>

#include <type_traits>

namespace boost::prometheus::detail {

template <typename T> struct is_ssl_stream : std::false_type {};
template <typename NextLayer>
struct is_ssl_stream<boost::asio::ssl::stream<NextLayer>> : std::true_type {};

template <typename T>
inline constexpr bool is_ssl_stream_v =
    is_ssl_stream<std::remove_cvref_t<T>>::value;

} // namespace boost::prometheus::detail

#endif
