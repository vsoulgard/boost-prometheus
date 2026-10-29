//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/info.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_info)

BOOST_AUTO_TEST_CASE(default_constructor_is_deleted)
{
    static_assert(!std::is_default_constructible_v<info>);
}

BOOST_AUTO_TEST_CASE(construction_default_options)
{
    info in("build", {}, {}, {});
}

BOOST_AUTO_TEST_CASE(construction_custom_options)
{
    info in("build", {.help = "help text"}, {"version"}, {"1.0"});
}

BOOST_AUTO_TEST_CASE(construction_lvalue)
{
    {
        std::vector<std::string> labels = {"version"};
        std::vector<std::string> labelvalues = {"1.0"};
        info in("build", {.help = "help text"}, labels, labelvalues);
        BOOST_TEST(labels == std::vector<std::string>{"version"});
        BOOST_TEST(labelvalues == std::vector<std::string>{"1.0"});
    }
    {
        std::vector<std::string> labels = {"version", "build_type"};
        std::vector<std::string> labelvalues = {"1.0", "Debug"};
        info in("build", {.help = "help text"}, labels, labelvalues);
        BOOST_TEST(labels == std::vector<std::string>({"version", "build_type"}));
        BOOST_TEST(labelvalues == std::vector<std::string>({"1.0", "Debug"}));
    }
}

BOOST_AUTO_TEST_CASE(construction_rvalue)
{
    {
        std::vector<std::string> labels = {"version"};
        std::vector<std::string> labelvalues = {"1.0"};
        info in("build", {.help = "help text"}, std::move(labels), std::move(labelvalues));
        // All labels are moved.
        BOOST_TEST(labels == std::vector<std::string>(1));
        BOOST_TEST(labelvalues == std::vector<std::string>{"1.0"});
    }
    {
        std::vector<std::string> labels = {"version", "build_type"};
        std::vector<std::string> labelvalues = {"1.0", "Debug"};
        info in("build", {.help = "help text"}, std::move(labels), std::move(labelvalues));
        BOOST_TEST(labels == std::vector<std::string>(2));
        BOOST_TEST(labelvalues == std::vector<std::string>({"1.0", "Debug"}));
    }
}

BOOST_AUTO_TEST_SUITE_END()
