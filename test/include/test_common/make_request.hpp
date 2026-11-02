//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_TEST_MAKE_REQUEST_HPP
#define BOOST_PROMETHEUS_TEST_MAKE_REQUEST_HPP

#include <boost/test/unit_test.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>

#include <string>
#include <string_view>

namespace boost::prometheus::test {

namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace asio = boost::asio;
namespace prometheus = boost::prometheus;

// For testing prometheus::exposer and Pushgateway.
inline std::string make_request(
    beast::tcp_stream& stream,
    std::string_view host,
    std::string_view route,
    std::string_view accept)
{
    http::request<http::empty_body> req{http::verb::get, route, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, "Prometheus/2.51.0");
    req.set(http::field::accept, accept);
    req.keep_alive(true);

    beast::error_code ec;
    http::write(stream, req, ec);
    if (ec)
    {
        BOOST_FAIL("Failed to write request: " << ec.message());
    }

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    
    http::read(stream, buffer, res, ec);
    if (ec)
    {
        BOOST_FAIL("Failed to read response: " << ec.message());
    }

    // Typical for exposers.
    BOOST_TEST(res.keep_alive() == true);
    BOOST_TEST(res.result() == http::status::ok);

    return std::move(res.body());
}

} // namespace boost::prometheus::test

#endif
