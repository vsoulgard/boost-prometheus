//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_TEST_GZIP_DECODE_HPP
#define BOOST_PROMETHEUS_TEST_GZIP_DECODE_HPP

#include <boost/beast/zlib/inflate_stream.hpp>

#include <boost/crc.hpp>

#include <cstdint>

namespace boost::prometheus::test {

template <typename BufferInType, typename BufferOutType>
bool gzip_decode(const BufferInType& gzip_data, BufferOutType& out)
{
    namespace zlib = boost::beast::zlib;

    if (gzip_data.size() < 18)
        return false;

    if (std::uint8_t(gzip_data[0]) != 0x1f || std::uint8_t(gzip_data[1]) != 0x8b)
        return false;
    if (std::uint8_t(gzip_data[2]) != 0x08) // deflate
        return false;

    std::size_t pos = 10;
    const std::uint8_t flg = gzip_data[3];
    const bool has_fextra   = (flg & 0x04) != 0;
    const bool has_fname    = (flg & 0x08) != 0;
    const bool has_fcomment = (flg & 0x10) != 0;
    const bool has_fhcrc    = (flg & 0x02) != 0;

    if (has_fextra)
    {
        if (pos + 2 > gzip_data.size() - 8)
            return false;
        const std::size_t xlen = gzip_data[pos] | (static_cast<std::size_t>(gzip_data[pos + 1]) << 8);
        pos += 2 + xlen;
    }
    if (has_fname)
    {
        while (pos < gzip_data.size() - 8 && gzip_data[pos] != 0)
            ++pos;
        ++pos;
    }
    if (has_fcomment)
    {
        while (pos < gzip_data.size() - 8 && gzip_data[pos] != 0)
            ++pos;
        ++pos;
    }
    if (has_fhcrc)
        pos += 2;

    if (pos > gzip_data.size() - 8)
        return false;

    const std::size_t deflate_start = pos;
    const std::size_t deflate_end   = gzip_data.size() - 8;
    const std::size_t deflate_size  = deflate_end - deflate_start;

    const std::uint32_t expected_crc32 =
        static_cast<std::uint32_t>(std::uint8_t(gzip_data[deflate_end])) |
        (static_cast<std::uint32_t>(std::uint8_t(gzip_data[deflate_end + 1])) << 8) |
        (static_cast<std::uint32_t>(std::uint8_t(gzip_data[deflate_end + 2])) << 16) |
        (static_cast<std::uint32_t>(std::uint8_t(gzip_data[deflate_end + 3])) << 24);

    const std::uint32_t expected_isize =
        static_cast<std::uint32_t>(std::uint8_t(gzip_data[deflate_end + 4])) |
        (static_cast<std::uint32_t>(std::uint8_t(gzip_data[deflate_end + 5])) << 8) |
        (static_cast<std::uint32_t>(std::uint8_t(gzip_data[deflate_end + 6])) << 16) |
        (static_cast<std::uint32_t>(std::uint8_t(gzip_data[deflate_end + 7])) << 24);

    zlib::inflate_stream inflater;
    inflater.reset(15);

    zlib::z_params zs{};
    zs.next_in = const_cast<void*>(static_cast<const void*>(
        gzip_data.data() + deflate_start));
    zs.avail_in = deflate_size;

    out.clear();
    out.reserve(deflate_size * 2);

    constexpr std::size_t chunk = 65536;
    boost::system::error_code ec;
    unsigned stall_counter = 0;
    constexpr unsigned max_stall = 16;

    for (;;)
    {
        const std::size_t old = out.size();
        out.resize(old + chunk);
        zs.next_out = static_cast<void*>(out.data() + old);
        zs.avail_out = chunk;
        const std::size_t before_avail_in = zs.avail_in;

        inflater.write(zs, zlib::Flush::finish, ec);

        const std::size_t produced = chunk - zs.avail_out;
        out.resize(old + produced);

        if (ec == zlib::error::end_of_stream)
            break;
        if (ec == zlib::error::need_buffers)
        {
            ec = {};
            if (produced == 0 && zs.avail_in == before_avail_in)
            {
                if (++stall_counter > max_stall)
                    return false;
            }
            else
            {
                stall_counter = 0;
            }
            continue;
        }
        if (ec)
            return false;
    }

    boost::crc_32_type crc;
    if (!out.empty())
        crc.process_bytes(out.data(), out.size());

    if (crc.checksum() != expected_crc32)
        return false;

    if ((out.size() & 0xffffffffULL) != expected_isize)
        return false;

    return true;
}

} // namespace boost::prometheus::test

#endif
