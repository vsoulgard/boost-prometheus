//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_IS_SPECIALIZATION_OF_HPP
#define BOOST_PROMETHEUS_DETAIL_IS_SPECIALIZATION_OF_HPP

#include <boost/prometheus/detail/string_buffer.hpp>

namespace boost::prometheus::detail {

template <typename T, template <auto...> typename Template>
struct is_specialization_of : std::false_type {};

template <template <auto...> typename Template, auto... Args>
struct is_specialization_of<Template<Args...>, Template> : std::true_type {};

template <typename T, template <typename> typename Template>
struct is_type_specialization_of : std::false_type {};

template <template <typename> typename Template, typename Arg>
struct is_type_specialization_of<Template<Arg>, Template> : std::true_type {};

} // namespace boost::prometheus::detail

#endif
