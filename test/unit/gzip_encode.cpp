//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/gzip_encode.hpp>

#include <boost/beast/zlib/inflate_stream.hpp>

#include <boost/test/unit_test.hpp>

#include <random>

#include "test_common/gzip_decode.hpp"

using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_gzip_encode)

BOOST_AUTO_TEST_CASE(empty_input)
{
    std::vector<std::uint8_t> in;
    std::vector<std::uint8_t> out;

    BOOST_CHECK_NO_THROW(gzip_encode(in, out));

    BOOST_REQUIRE_GE(out.size(), 18u);

    std::vector<std::uint8_t> decoded;
    BOOST_REQUIRE(test::gzip_decode(out, decoded));
    BOOST_CHECK_EQUAL(decoded.size(), 0u);
}

BOOST_AUTO_TEST_CASE(small_string)
{
    const std::string in = "hello world";
    std::vector<std::uint8_t> out;

    BOOST_CHECK_NO_THROW(gzip_encode(in, out));
    BOOST_REQUIRE_GE(out.size(), in.size() + 18u);

    std::vector<std::uint8_t> decoded;
    BOOST_REQUIRE(test::gzip_decode(out, decoded));

    BOOST_REQUIRE_EQUAL(decoded.size(), in.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(
        decoded.begin(), decoded.end(),
        reinterpret_cast<const std::uint8_t*>(in.data()),
        reinterpret_cast<const std::uint8_t*>(in.data()) + in.size());
}

BOOST_AUTO_TEST_CASE(large_data_roundtrip)
{
    std::vector<std::uint8_t> in(1024 * 1024); // 1 MB
    std::mt19937_64 rng(42);
    for (auto& b : in)
        b = static_cast<std::uint8_t>(rng() & 0xff);

    std::vector<std::uint8_t> out;
    BOOST_CHECK_NO_THROW(gzip_encode(in, out));

    BOOST_REQUIRE_GE(out.size(), in.size() * 90 / 100);

    std::vector<std::uint8_t> decoded;
    BOOST_REQUIRE(test::gzip_decode(out, decoded));

    BOOST_REQUIRE_EQUAL(decoded.size(), in.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(decoded.begin(), decoded.end(),
                                  in.begin(), in.end());
}

BOOST_AUTO_TEST_CASE(repetitive_data_compresses_well)
{
    std::string in;
    for (int i = 0; i < 100000; ++i)
        in += "AAAA";

    std::vector<std::uint8_t> out;
    BOOST_CHECK_NO_THROW(gzip_encode(in, out));

    BOOST_CHECK_LT(out.size(), in.size() / 10);

    std::vector<std::uint8_t> decoded;
    BOOST_REQUIRE(test::gzip_decode(out, decoded));

    BOOST_REQUIRE_EQUAL(decoded.size(), in.size());
    BOOST_CHECK(std::equal(decoded.begin(), decoded.end(),
                           reinterpret_cast<const std::uint8_t*>(in.data())));
}

BOOST_AUTO_TEST_CASE(reusable_buffer_preserves_capacity)
{
    std::vector<std::uint8_t> in(100000, 'X');
    std::vector<std::uint8_t> out;

    gzip_encode(in, out);
    const std::size_t cap_after_first = out.capacity();
    BOOST_CHECK_GT(cap_after_first, 0u);
    out.clear();
    gzip_encode(in, out);
    const std::size_t cap_after_second = out.capacity();

    BOOST_CHECK_GE(cap_after_second, cap_after_first);

    BOOST_CHECK_EQUAL(cap_after_second, cap_after_first);

    std::vector<std::uint8_t> out2;
    gzip_encode(in, out2);
    BOOST_CHECK_EQUAL(out.size(), out2.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(),
                                  out2.begin(), out2.end());
}

BOOST_AUTO_TEST_CASE(reusable_buffer_with_larger_second_input)
{
    std::vector<std::uint8_t> small_in(1000, 'A');
    std::vector<std::uint8_t> big_in(100000, 'B');

    std::vector<std::uint8_t> out;
    gzip_encode(small_in, out);
    const std::size_t cap_after_small = out.capacity();
    out.clear();
    gzip_encode(big_in, out);
    const std::size_t cap_after_big = out.capacity();

    BOOST_CHECK_GE(cap_after_big, cap_after_small);

    std::vector<std::uint8_t> decoded;
    BOOST_REQUIRE(test::gzip_decode(out, decoded));
    BOOST_CHECK_EQUAL(decoded.size(), big_in.size());
    BOOST_CHECK(std::all_of(decoded.begin(), decoded.end(),
                            [](std::uint8_t b) { return b == 'B'; }));
}

BOOST_AUTO_TEST_CASE(gzip_header_magic_and_method)
{
    const std::string in = "test data";
    std::vector<std::uint8_t> out;
    gzip_encode(in, out);

    BOOST_REQUIRE_GE(out.size(), 10u);

    // Magic: 0x1f 0x8b
    BOOST_CHECK_EQUAL(out[0], 0x1f);
    BOOST_CHECK_EQUAL(out[1], 0x8b);

    // Method: 8 (deflate)
    BOOST_CHECK_EQUAL(out[2], 0x08);

    // FLG: 0
    BOOST_CHECK_EQUAL(out[3], 0x00);

    // MTIME: 0
    BOOST_CHECK_EQUAL(out[4], 0x00);
    BOOST_CHECK_EQUAL(out[5], 0x00);
    BOOST_CHECK_EQUAL(out[6], 0x00);
    BOOST_CHECK_EQUAL(out[7], 0x00);

    // XFL: 0
    BOOST_CHECK_EQUAL(out[8], 0x00);

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    BOOST_CHECK_EQUAL(out[9], 3u);
#elif defined(_WIN32)
    BOOST_CHECK_EQUAL(out[9], 0u);
#else
    BOOST_CHECK_EQUAL(out[9], 255u);
#endif
}

BOOST_AUTO_TEST_CASE(gzip_trailer_crc_and_isize)
{
    const std::string in = "hello, gzip world!";
    std::vector<std::uint8_t> out;
    gzip_encode(in, out);

    BOOST_REQUIRE_GE(out.size(), 18u);

    boost::crc_32_type crc;
    crc.process_bytes(in.data(), in.size());
    const std::uint32_t expected_crc = crc.checksum();

    const std::uint32_t expected_isize =
        static_cast<std::uint32_t>(in.size() & 0xffffffffULL);

    const std::size_t trailer_start = out.size() - 8;

    // Little-endian
    std::uint32_t actual_crc = 0;
    actual_crc |= static_cast<std::uint32_t>(out[trailer_start + 0]);
    actual_crc |= static_cast<std::uint32_t>(out[trailer_start + 1]) << 8;
    actual_crc |= static_cast<std::uint32_t>(out[trailer_start + 2]) << 16;
    actual_crc |= static_cast<std::uint32_t>(out[trailer_start + 3]) << 24;

    BOOST_CHECK_EQUAL(actual_crc, expected_crc);

    std::uint32_t actual_isize = 0;
    actual_isize |= static_cast<std::uint32_t>(out[trailer_start + 4]);
    actual_isize |= static_cast<std::uint32_t>(out[trailer_start + 5]) << 8;
    actual_isize |= static_cast<std::uint32_t>(out[trailer_start + 6]) << 16;
    actual_isize |= static_cast<std::uint32_t>(out[trailer_start + 7]) << 24;

    BOOST_CHECK_EQUAL(actual_isize, expected_isize);
}

BOOST_AUTO_TEST_CASE(vector_char_works)
{
    std::vector<char> in = {'f', 'o', 'o', ' ', 'b', 'a', 'r'};
    std::vector<std::uint8_t> out;

    BOOST_CHECK_NO_THROW(gzip_encode(in, out));

    std::vector<std::uint8_t> decoded;
    BOOST_REQUIRE(test::gzip_decode(out, decoded));

    BOOST_REQUIRE_EQUAL(decoded.size(), in.size());
    for (std::size_t i = 0; i < in.size(); ++i)
        BOOST_CHECK_EQUAL(decoded[i], static_cast<std::uint8_t>(in[i]));
}

BOOST_AUTO_TEST_CASE(string_output_works)
{
    const std::string in = "output can be string too";
    std::string out;

    BOOST_CHECK_NO_THROW(gzip_encode(in, out));

    std::vector<std::uint8_t> out_vec(out.begin(), out.end());

    std::vector<std::uint8_t> decoded;
    BOOST_REQUIRE(test::gzip_decode(out_vec, decoded));

    BOOST_CHECK_EQUAL(decoded.size(), in.size());
}

BOOST_AUTO_TEST_CASE(string_view_input_works)
{
    const std::string storage = "string_view input test data";
    std::string_view in = storage;

    std::vector<std::uint8_t> out;
    BOOST_CHECK_NO_THROW(gzip_encode(in, out));

    std::vector<std::uint8_t> decoded;
    BOOST_REQUIRE(test::gzip_decode(out, decoded));

    BOOST_REQUIRE_EQUAL(decoded.size(), storage.size());
    BOOST_CHECK(std::equal(decoded.begin(), decoded.end(),
                           reinterpret_cast<const std::uint8_t*>(storage.data())));
}

BOOST_AUTO_TEST_CASE(std_byte_vector_works)
{
    std::vector<std::byte> in = {
        std::byte{'h'}, std::byte{'i'}, std::byte{'!'}
    };
    std::vector<std::byte> out;

    BOOST_CHECK_NO_THROW(gzip_encode(in, out));

    std::vector<std::uint8_t> out_u8;
    for (auto b : out)
        out_u8.push_back(static_cast<std::uint8_t>(b));

    std::vector<std::uint8_t> decoded;
    BOOST_REQUIRE(test::gzip_decode(out_u8, decoded));

    BOOST_REQUIRE_EQUAL(decoded.size(), in.size());
}

BOOST_AUTO_TEST_CASE(very_small_input_1_byte)
{
    std::vector<std::uint8_t> in = {42};
    std::vector<std::uint8_t> out;

    BOOST_CHECK_NO_THROW(gzip_encode(in, out));

    std::vector<std::uint8_t> decoded;
    BOOST_REQUIRE(test::gzip_decode(out, decoded));

    BOOST_REQUIRE_EQUAL(decoded.size(), 1u);
    BOOST_CHECK_EQUAL(decoded[0], 42u);
}

BOOST_AUTO_TEST_CASE(medium_input_roundtrip)
{
    std::vector<std::uint8_t> in(64 * 1024);
    for (std::size_t i = 0; i < in.size(); ++i)
        in[i] = static_cast<std::uint8_t>(i & 0xff);

    std::vector<std::uint8_t> out;
    BOOST_CHECK_NO_THROW(gzip_encode(in, out));

    std::vector<std::uint8_t> decoded;
    BOOST_REQUIRE(test::gzip_decode(out, decoded));

    BOOST_REQUIRE_EQUAL(decoded.size(), in.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(decoded.begin(), decoded.end(),
                                  in.begin(), in.end());
}

BOOST_AUTO_TEST_CASE(large_input_multiple_chunks)
{
    std::vector<std::uint8_t> in(1024 * 1024);
    std::mt19937_64 rng(123);
    for (auto& b : in)
        b = static_cast<std::uint8_t>(rng() & 0xff);

    std::vector<std::uint8_t> out;
    BOOST_CHECK_NO_THROW(gzip_encode(in, out));

    std::vector<std::uint8_t> decoded;
    BOOST_REQUIRE(test::gzip_decode(out, decoded));

    BOOST_REQUIRE_EQUAL(decoded.size(), in.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(decoded.begin(), decoded.end(),
                                  in.begin(), in.end());
}

BOOST_AUTO_TEST_SUITE_END()
