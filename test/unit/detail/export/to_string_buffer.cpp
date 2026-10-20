//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/detail/export/to_string_buffer.hpp>

#include <boost/test/unit_test.hpp>

#include <charconv>

using namespace boost::prometheus::detail;

BOOST_AUTO_TEST_SUITE(test_to_string_buffer)

BOOST_AUTO_TEST_CASE(with_double)
{
    // Compare to the to_chars behavior
    auto test_value = []<double Value>() {
        std::array<char, 1024> buf;
        auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), Value, std::chars_format::fixed);
        BOOST_ASSERT(ec == std::errc());
        BOOST_TEST(std::string_view(to_string_buffer<Value>()) == std::string_view(buf.data(), ptr - buf.data()));
    };
    auto test_value_with_prec = []<double Value, std::size_t Precision>() {
        std::array<char, 1024> buf;
        auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), Value, std::chars_format::fixed, Precision);
        BOOST_ASSERT(ec == std::errc());
        BOOST_TEST(std::string_view(to_string_buffer<Value, Precision>()) == std::string_view(buf.data(), ptr - buf.data()));
    };

    test_value.operator()<-2.345678>();
    test_value.operator()<-1.0>();
    test_value.operator()<-0.0>();
    test_value.operator()<0.0>();
    test_value.operator()<0.000001>();
    test_value.operator()<0.99999999>();
    test_value.operator()<1.00000001>();
    test_value.operator()<10.0>();
    test_value.operator()<1234.56789>();
    test_value.operator()<98998765.56789>();
    test_value.operator()<8998998898.0>();
    test_value_with_prec.operator()<1234.56789, 0>();
    test_value_with_prec.operator()<1234.56789, 1>();
    test_value_with_prec.operator()<1234.56789, 2>();
    test_value_with_prec.operator()<1234.56789, 3>();
    test_value_with_prec.operator()<1234.56789, 4>();
    test_value_with_prec.operator()<1234.56789, 5>();
    test_value_with_prec.operator()<989865.99999999994, 11>();
    test_value_with_prec.operator()<0.999999999999998, 15>();
    test_value_with_prec.operator()<998998898.8998998898, 10>();
    test_value_with_prec.operator()<1.123456789123456789, 18>();
}

BOOST_AUTO_TEST_SUITE_END()
