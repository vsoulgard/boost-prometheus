//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/detail/error_categories.hpp>

#include <boost/test/unit_test.hpp>

#include <boost/beast/http/status.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

#include <memory>
#include <string>
#include <string_view>

using namespace boost;
using namespace boost::prometheus;
using namespace boost::prometheus::detail;

BOOST_AUTO_TEST_SUITE(test_error_categories)

BOOST_AUTO_TEST_CASE(http_status_category_singleton)
{
    auto& c1 = error::get_http_status_category();
    auto& c2 = error::get_http_status_category();
    BOOST_TEST(std::addressof(c1) == std::addressof(c2));
}

BOOST_AUTO_TEST_CASE(http_status_category_name)
{
    BOOST_TEST(std::string_view(error::get_http_status_category().name())
        == "prometheus.http_status");
}

BOOST_AUTO_TEST_CASE(http_status_category_message_known_codes)
{
    auto const& cat = error::get_http_status_category();
    using S = beast::http::status;

    BOOST_TEST(cat.message(static_cast<int>(S::bad_request))           == "400 Bad Request");
    BOOST_TEST(cat.message(static_cast<int>(S::unauthorized))          == "401 Unauthorized");
    BOOST_TEST(cat.message(static_cast<int>(S::forbidden))             == "403 Forbidden");
    BOOST_TEST(cat.message(static_cast<int>(S::not_found))             == "404 Not Found");
    BOOST_TEST(cat.message(static_cast<int>(S::method_not_allowed))    == "405 Method Not Allowed");
    BOOST_TEST(cat.message(static_cast<int>(S::payload_too_large))     == "413 Payload Too Large");
    BOOST_TEST(cat.message(static_cast<int>(S::internal_server_error)) == "500 Internal Server Error");
    BOOST_TEST(cat.message(static_cast<int>(S::bad_gateway))           == "502 Bad Gateway");
    BOOST_TEST(cat.message(static_cast<int>(S::service_unavailable))   == "503 Service Unavailable");
}

BOOST_AUTO_TEST_CASE(http_status_category_message_unknown_codes)
{
    auto const& cat = error::get_http_status_category();

    BOOST_TEST(cat.message(418) == "HTTP 418");
    BOOST_TEST(cat.message(0)   == "HTTP 0");
    BOOST_TEST(cat.message(999) == "HTTP 999");
    BOOST_TEST(cat.message(-1)  == "HTTP -1");
}

BOOST_AUTO_TEST_CASE(make_http_status_error_value_and_category)
{
    auto ec = error::make_http_status_error(404);

    BOOST_TEST(ec.value() == 404);
    BOOST_TEST(std::addressof(ec.category())
        == std::addressof(error::get_http_status_category()));
    BOOST_TEST(ec.message() == "404 Not Found");
    BOOST_TEST(static_cast<bool>(ec));
}

BOOST_AUTO_TEST_CASE(make_http_status_error_zero_is_no_error)
{
    auto ec = error::make_http_status_error(0);

    BOOST_TEST(ec.value() == 0);
    BOOST_TEST(!ec);
    BOOST_TEST(std::addressof(ec.category())
        == std::addressof(error::get_http_status_category()));
}

BOOST_AUTO_TEST_CASE(make_http_status_error_unknown_status)
{
    auto ec = error::make_http_status_error(418);
    BOOST_TEST(ec.value() == 418);
    BOOST_TEST(ec.message() == "HTTP 418");
}

BOOST_AUTO_TEST_CASE(http_status_category_distinct_from_system_and_generic)
{
    auto const& http = error::get_http_status_category();
    BOOST_TEST(std::addressof(http) != std::addressof(boost::system::system_category()));
    BOOST_TEST(std::addressof(http) != std::addressof(boost::system::generic_category()));
}

BOOST_AUTO_TEST_CASE(http_status_error_not_equal_across_categories)
{
    auto ec_http = error::make_http_status_error(42);
    boost::system::error_code ec_sys(42, boost::system::generic_category());

    BOOST_TEST(ec_http != ec_sys);
    BOOST_TEST(ec_sys != ec_http);
    BOOST_TEST(std::addressof(ec_http.category())
        != std::addressof(ec_sys.category()));
}

BOOST_AUTO_TEST_CASE(http_status_error_equality_within_same_category)
{
    auto a = error::make_http_status_error(404);
    auto b = error::make_http_status_error(404);
    auto c = error::make_http_status_error(500);

    BOOST_TEST(a == b);
    BOOST_TEST(a != c);
}

BOOST_AUTO_TEST_CASE(pushgateway_category_singleton)
{
    auto& c1 = error::get_pushgateway_category();
    auto& c2 = error::get_pushgateway_category();
    BOOST_TEST(std::addressof(c1) == std::addressof(c2));
}

BOOST_AUTO_TEST_CASE(pushgateway_category_name)
{
    BOOST_TEST(std::string_view(error::get_pushgateway_category().name())
        == "prometheus.pushgateway");
}

BOOST_AUTO_TEST_CASE(pushgateway_category_message_known_codes)
{
    auto const& cat = error::get_pushgateway_category();

    BOOST_TEST(cat.message(static_cast<int>(pushgateway_errc::incorrect_job_name))
        == "incorrect job name");
}

BOOST_AUTO_TEST_CASE(pushgateway_category_message_unknown_codes)
{
    auto const& cat = error::get_pushgateway_category();

    BOOST_TEST(cat.message(12345) == "unknown Prometheus pushgateway error");
    BOOST_TEST(cat.message(-1)    == "unknown Prometheus pushgateway error");
}

BOOST_AUTO_TEST_CASE(make_error_code_incorrect_job_name)
{
    auto ec = make_error_code(pushgateway_errc::incorrect_job_name);

    BOOST_TEST(static_cast<bool>(ec));
    BOOST_TEST(ec.value()
        == static_cast<int>(pushgateway_errc::incorrect_job_name));
    BOOST_TEST(std::addressof(ec.category())
        == std::addressof(error::get_pushgateway_category()));
    BOOST_TEST(ec.message() == "incorrect job name");
}

BOOST_AUTO_TEST_CASE(pushgateway_error_code_compares_with_enum)
{
    namespace sys = boost::system;

    sys::error_code err = make_error_code(pushgateway_errc::incorrect_job_name);

    BOOST_TEST(err == pushgateway_errc::incorrect_job_name);
}

BOOST_AUTO_TEST_CASE(pushgateway_category_distinct_from_others)
{
    auto const& pgw  = error::get_pushgateway_category();
    auto const& http = error::get_http_status_category();

    BOOST_TEST(std::addressof(pgw) != std::addressof(http));
    BOOST_TEST(std::addressof(pgw) != std::addressof(boost::system::system_category()));
    BOOST_TEST(std::addressof(pgw) != std::addressof(boost::system::generic_category()));
}

BOOST_AUTO_TEST_CASE(categories_are_not_equal_to_each_other)
{
    auto const& http = error::get_http_status_category();
    auto const& pgw  = error::get_pushgateway_category();

    BOOST_TEST(std::addressof(http) != std::addressof(pgw));
    BOOST_TEST(!(std::addressof(http) == std::addressof(pgw)));
}

BOOST_AUTO_TEST_CASE(error_codes_with_same_value_different_categories_are_distinct)
{
    namespace sys = boost::system;

    sys::error_code http_1(1, error::get_http_status_category());
    sys::error_code pgw_1 (1, error::get_pushgateway_category());

    BOOST_TEST(http_1 != pgw_1);
    BOOST_TEST(http_1.value() == pgw_1.value());
}

BOOST_AUTO_TEST_CASE(error_codes_from_same_category_are_equal)
{
    namespace sys = boost::system;

    sys::error_code a(404, error::get_http_status_category());
    sys::error_code b(404, error::get_http_status_category());
    sys::error_code c(500, error::get_http_status_category());

    BOOST_TEST(a == b);
    BOOST_TEST(a != c);
}

BOOST_AUTO_TEST_CASE(system_error_reports_category_and_message)
{
    namespace sys = boost::system;

    sys::error_code ec = error::make_http_status_error(404);
    sys::system_error se(ec);

    BOOST_TEST(std::string_view(se.code().category().name())
        == "prometheus.http_status");
    BOOST_TEST(std::string_view(se.what()).find("404 Not Found")
        != std::string_view::npos);
}

BOOST_AUTO_TEST_SUITE_END()
