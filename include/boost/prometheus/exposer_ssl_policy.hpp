//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_EXPOSER_SSL_POLICY_HPP
#define BOOST_PROMETHEUS_EXPOSER_SSL_POLICY_HPP

#include <boost/prometheus/detail/ssl_fwd.hpp>

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <boost/prometheus/exposer_options.hpp>

namespace boost::prometheus {

/**
 * \brief Enables or disables SSL/TLS mode for the exposer.
 *
 * Enabling this policy requires including
 * `<boost/prometheus/exposer_ssl_policy.hpp>` and linking against OpenSSL.
 *
 * \tparam Enabled `true` for TLS mode, `false` for plain HTTP.
 */
template <>
struct ssl_policy<true>
{
    static constexpr bool enabled = true;
}; 

} // namespace boost::prometheus

#endif
