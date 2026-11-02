//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/pushgateway_errc.hpp>

#include <boost/test/unit_test.hpp>

#include <boost/system/error_code.hpp>

#include <sstream>
#include <string>
#include <string_view>

using namespace boost;
using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_pushgateway_errc)

BOOST_AUTO_TEST_CASE(enum_values_are_stable)
{
    BOOST_TEST(static_cast<int>(pushgateway_errc::incorrect_job_name) == 1);
}

BOOST_AUTO_TEST_CASE(is_error_code_enum_trait)
{
    BOOST_TEST((boost::system::is_error_code_enum<pushgateway_errc>::value));
}

BOOST_AUTO_TEST_CASE(make_error_code_incorrect_job_name)
{
    auto ec = make_error_code(pushgateway_errc::incorrect_job_name);

    BOOST_TEST(ec.value() == 1);
    BOOST_TEST(static_cast<bool>(ec));
    BOOST_TEST(std::addressof(ec.category())
        == std::addressof(error::get_pushgateway_category()));
}

BOOST_AUTO_TEST_CASE(make_error_code_is_deterministic)
{
    auto a = make_error_code(pushgateway_errc::incorrect_job_name);
    auto b = make_error_code(pushgateway_errc::incorrect_job_name);

    BOOST_TEST(a == b);
    BOOST_TEST(a.value() == b.value());
    BOOST_TEST(std::addressof(a.category()) == std::addressof(b.category()));
}

BOOST_AUTO_TEST_CASE(implicit_conversion_to_error_code)
{
    boost::system::error_code ec = pushgateway_errc::incorrect_job_name;
    BOOST_TEST(ec.value() == 1);
    BOOST_TEST(std::addressof(ec.category())
        == std::addressof(error::get_pushgateway_category()));
}

BOOST_AUTO_TEST_CASE(implicit_conversion_matches_make_error_code)
{
    boost::system::error_code implicit = pushgateway_errc::incorrect_job_name;
    boost::system::error_code explicit_ = make_error_code(pushgateway_errc::incorrect_job_name);

    BOOST_TEST(implicit == explicit_);
    BOOST_TEST(implicit.value() == explicit_.value());
    BOOST_TEST(std::addressof(implicit.category())
        == std::addressof(explicit_.category()));
}

BOOST_AUTO_TEST_CASE(comparison_with_enum_both_orders)
{
    boost::system::error_code ec = pushgateway_errc::incorrect_job_name;

    BOOST_TEST(ec == pushgateway_errc::incorrect_job_name);
    BOOST_TEST(pushgateway_errc::incorrect_job_name == ec);
}

BOOST_AUTO_TEST_CASE(ostream_operator_incorrect_job_name)
{
    std::ostringstream oss;
    oss << pushgateway_errc::incorrect_job_name;

    BOOST_TEST(oss.str().find("prometheus.pushgateway:1") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(category_is_singleton)
{
    BOOST_TEST(std::addressof(error::get_pushgateway_category())
        == std::addressof(error::get_pushgateway_category()));
}

BOOST_AUTO_TEST_CASE(category_name)
{
    BOOST_TEST(std::string_view(error::get_pushgateway_category().name())
        == "prometheus.pushgateway");
}

BOOST_AUTO_TEST_CASE(category_message_known_codes)
{
    auto const& cat = error::get_pushgateway_category();

    BOOST_TEST(cat.message(static_cast<int>(pushgateway_errc::incorrect_job_name))
        == "incorrect job name");
}

BOOST_AUTO_TEST_CASE(category_message_unknown_code)
{
    BOOST_TEST(error::get_pushgateway_category().message(42)
        == "unknown Prometheus pushgateway error");
}

BOOST_AUTO_TEST_CASE(not_equal_to_system_or_generic_category)
{
    auto const& pgw = error::get_pushgateway_category();

    BOOST_TEST(std::addressof(pgw)
        != std::addressof(boost::system::system_category()));
    BOOST_TEST(std::addressof(pgw)
        != std::addressof(boost::system::generic_category()));
}

BOOST_AUTO_TEST_CASE(error_code_with_same_value_but_other_category_is_distinct)
{
    boost::system::error_code pgw_1 =
        pushgateway_errc::incorrect_job_name;
    boost::system::error_code sys_1(1, boost::system::generic_category());

    BOOST_TEST(pgw_1.value() == sys_1.value());
    BOOST_TEST(pgw_1 != sys_1);
    BOOST_TEST(sys_1 != pgw_1);
}

BOOST_AUTO_TEST_SUITE_END()
