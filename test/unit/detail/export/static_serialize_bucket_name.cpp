//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/detail/export/static_serialize_bucket_name.hpp>

#include <boost/prometheus/static_histogram.hpp>

#include <boost/test/unit_test.hpp>

#include <string_view>

using namespace boost::prometheus;
using namespace boost::prometheus::detail;

BOOST_AUTO_TEST_SUITE(test_static_serialize_header)

BOOST_AUTO_TEST_CASE(basic)
{
    using h = static_histogram<"test_h">;
    static_assert(static_serialize_bucket_name<h, string_buffer("0.1")>::value ==
        std::string_view("test_h_bucket{le=\"0.1\"} "));
    static_assert(static_serialize_bucket_name<h, string_buffer("0.2")>::value ==
        std::string_view("test_h_bucket{le=\"0.2\"} "));
    static_assert(static_serialize_bucket_name<h, string_buffer("+Inf")>::value ==
        std::string_view("test_h_bucket{le=\"+Inf\"} "));
}

BOOST_AUTO_TEST_CASE(with_labels)
{
    using h = static_histogram<"test_h", labels<"first", "second">>;
    static_assert(static_serialize_bucket_name<h, string_buffer("0.1"), string_buffer("1"), string_buffer("2")>::value ==
        std::string_view("test_h_bucket{first=\"1\",le=\"0.1\",second=\"2\"} "));
    static_assert(static_serialize_bucket_name<h, string_buffer("0.2"), string_buffer("1"), string_buffer("2")>::value ==
        std::string_view("test_h_bucket{first=\"1\",le=\"0.2\",second=\"2\"} "));
    static_assert(static_serialize_bucket_name<h, string_buffer("+Inf"), string_buffer("3"), string_buffer("4")>::value ==
        std::string_view("test_h_bucket{first=\"3\",le=\"+Inf\",second=\"4\"} "));
}

BOOST_AUTO_TEST_SUITE_END()
