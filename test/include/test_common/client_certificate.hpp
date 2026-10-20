//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_TEST_CLIENT_CERTIFICATE_HPP
#define BOOST_PROMETHEUS_TEST_CLIENT_CERTIFICATE_HPP

#include <boost/asio/ssl/context.hpp>

#include <string_view>

namespace boost::prometheus::test {

constexpr std::string_view root_certificate =
    "-----BEGIN CERTIFICATE-----\n"
    "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\n"
    "1ArwBXw57BXWNvDWrEabV3cQgK0oxxs=\n"
    "-----END CERTIFICATE-----\n";

void load_client_certificate(boost::asio::ssl::context& ctx)
{
    ctx.add_certificate_authority(boost::asio::buffer(root_certificate.data(), root_certificate.size()));
    ctx.set_verify_mode(boost::asio::ssl::verify_peer);
}

} // namespace boost::prometheus::test

#endif
