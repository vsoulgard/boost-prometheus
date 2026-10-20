//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_FRIEND_REGISTRATION_HPP
#define BOOST_PROMETHEUS_DETAIL_FRIEND_REGISTRATION_HPP

// Friend all registration types
#define BOOST_PROMETHEUS_FRIEND_REGISTRATION()                              \
    template <typename, ::boost::prometheus::detail::string_buffer...>      \
    friend struct ::boost::prometheus::detail::static_labels_registrator;          \
    template <typename, typename>                                           \
    friend struct ::boost::prometheus::detail::static_metric_registrator;   \
    template <typename, ::boost::prometheus::detail::string_buffer...>      \
    friend struct ::boost::prometheus::detail::static_serialize_name;       \
    template <typename,                                                     \
        ::boost::prometheus::detail::string_buffer,                         \
        ::boost::prometheus::detail::string_buffer...>                      \
    friend struct ::boost::prometheus::detail::static_serialize_bucket_name;\
    template <typename,                                                     \
        ::boost::prometheus::detail::string_buffer,                         \
        typename...>                                                        \
    friend class ::boost::prometheus::detail::static_serialize_header;

#endif
