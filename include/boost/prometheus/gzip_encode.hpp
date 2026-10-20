//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_GZIP_ENCODE_HPP
#define BOOST_PROMETHEUS_GZIP_ENCODE_HPP

#include <boost/beast/zlib/deflate_stream.hpp>
#include <boost/crc.hpp>

#include <boost/predef/os.h>

#include <bit>
#include <span>
#include <string>

namespace boost::prometheus {

namespace detail {

// RFC 1952 OS codes:
//   0 = FAT filesystem (MS-DOS, OS/2, NT/Win32)
//   1 = Amiga
//   2 = VMS / OpenVMS
//   3 = Unix
// 255 = unknown
consteval std::uint8_t gzip_os_id() noexcept
{
#if defined(BOOST_OS_CYGWIN) && BOOST_OS_CYGWIN
    return 3;
#elif defined(BOOST_OS_WINDOWS) && BOOST_OS_WINDOWS
    return 0;
#elif defined(BOOST_OS_VMS) && BOOST_OS_VMS
    return 2;
#elif defined(BOOST_OS_AMIGAOS) && BOOST_OS_AMIGAOS
    return 1;
#elif (defined(BOOST_OS_UNIX) && BOOST_OS_UNIX) || \
      (defined(BOOST_OS_LINUX) && BOOST_OS_LINUX) || \
      (defined(BOOST_OS_BSD) && BOOST_OS_BSD) || \
      (defined(BOOST_OS_MACOS) && BOOST_OS_MACOS) || \
      (defined(BOOST_OS_IOS) && BOOST_OS_IOS) || \
      (defined(BOOST_OS_SOLARIS) && BOOST_OS_SOLARIS) || \
      (defined(BOOST_OS_AIX) && BOOST_OS_AIX) || \
      (defined(BOOST_OS_HPUX) && BOOST_OS_HPUX) || \
      (defined(BOOST_OS_QNX) && BOOST_OS_QNX) || \
      (defined(BOOST_OS_HAIKU) && BOOST_OS_HAIKU) || \
      (defined(BOOST_OS_BEOS) && BOOST_OS_BEOS)
    return 3;
#else
    return 255;
#endif
}

constexpr std::array<std::byte, 10> gzip_header{{
    std::byte{0x1f}, std::byte{0x8b},                   // Gzip ID
    std::byte{0x08},                                    // 0x08 = DEFLATE
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    static_cast<std::byte>(detail::gzip_os_id())        // OS ID
}};

template <class Buffer>
bool append_bytes(Buffer& out, const std::byte* data, std::size_t n)
{
    static_assert(sizeof(typename Buffer::value_type) == 1);
    static_assert(std::is_trivially_copyable_v<typename Buffer::value_type>);

    if (n == 0)
        return true;

    const std::size_t old_size = out.size();
    constexpr std::size_t max_value = std::numeric_limits<std::size_t>::max();

    BOOST_ASSERT(old_size <= max_value - n);
    if (old_size > max_value - n)
        return false;

    const std::size_t new_size = old_size + n;

    if (out.capacity() < new_size)
        out.reserve(new_size);

    out.resize(new_size);
    std::memcpy(out.data() + old_size, data, n);
    return true;
}

template <class Buffer>
bool ensure_spare(Buffer& out, std::size_t extra)
{
    static_assert(sizeof(typename Buffer::value_type) == 1);
    static_assert(std::is_trivially_copyable_v<typename Buffer::value_type>);

    const std::size_t size = out.size();
    constexpr std::size_t max_val = std::numeric_limits<std::size_t>::max();

    BOOST_ASSERT(size <= max_val - extra);
    if (size > max_val - extra)
        return false;

    const std::size_t needed = size + extra;

    if (out.capacity() >= needed)
        return true;

    const std::size_t cap = out.capacity();
    const std::size_t step = (cap > extra) ? cap : extra;
    const std::size_t doubled = (cap > max_val - step) ? max_val : (cap + step);

    out.reserve((doubled > needed) ? doubled : needed);
    return true;
}

template <typename BufferInType, typename BufferOutType>
bool gzip_encode_impl(const BufferInType& in_buffer, BufferOutType& out_buffer)
{
    namespace zlib = boost::beast::zlib;

    static_assert(sizeof(typename BufferOutType::value_type) == 1);
    static_assert(std::is_trivially_copyable_v<typename BufferOutType::value_type>);

    const auto* in_data = in_buffer.data();
    const std::size_t in_count = in_buffer.size();

    BOOST_ASSERT(in_count == 0 || in_data != nullptr);
    if (in_count != 0 && in_data == nullptr)
        return false;

    std::span<const std::byte> input_bytes;
    if (in_count != 0)
        input_bytes = std::as_bytes(std::span(in_data, in_count));

    const std::size_t input_size = input_bytes.size();

    boost::crc_32_type crc;
    if (input_size != 0)
        crc.process_bytes(input_bytes.data(), input_size);

    const std::uint32_t crc32 = crc.checksum();
    const std::uint32_t isize = static_cast<std::uint32_t>(input_size & 0xffffffffULL);

    constexpr std::size_t chunk_size = 65536;

    if (!ensure_spare(out_buffer, gzip_header.size() + chunk_size))
        return false;

    if (!append_bytes(out_buffer, gzip_header.data(), gzip_header.size()))
        return false;

    zlib::deflate_stream compressor;
    compressor.reset(6, 15, 8, zlib::Strategy::normal);

    zlib::z_params zs{};

    zs.next_in = input_bytes.data();
    zs.avail_in = input_size;

    boost::system::error_code ec;
    unsigned stall_counter = 0;
    constexpr unsigned max_stall_iterations = 16;

    while (zs.avail_in > 0)
    {
        const std::size_t old_size = out_buffer.size();

        if (!ensure_spare(out_buffer, chunk_size))
            return false;

        out_buffer.resize(old_size + chunk_size);

        zs.next_out = out_buffer.data() + old_size;
        zs.avail_out = chunk_size;

        const std::size_t before_avail_in = zs.avail_in;

        compressor.write(zs, zlib::Flush::none, ec);

        if (ec && ec != zlib::error::need_buffers)
            return false;

        BOOST_ASSERT(zs.avail_out <= chunk_size);
        if (zs.avail_out > chunk_size)
            return false;

        const std::size_t produced = chunk_size - zs.avail_out;
        out_buffer.resize(old_size + produced);

        if (produced == 0 && zs.avail_in == before_avail_in)
        {
            if (++stall_counter > max_stall_iterations)
                return false;
        }
        else
        {
            stall_counter = 0;
        }
    }

    stall_counter = 0;

    while (ec != zlib::error::end_of_stream)
    {
        const std::size_t old_size = out_buffer.size();

        if (!ensure_spare(out_buffer, chunk_size))
            return false;

        out_buffer.resize(old_size + chunk_size);

        zs.next_out = out_buffer.data() + old_size;
        zs.avail_out = chunk_size;

        compressor.write(zs, zlib::Flush::finish, ec);

        if (ec && ec != zlib::error::end_of_stream)
            return false;

        BOOST_ASSERT(zs.avail_out <= chunk_size);
        if (zs.avail_out > chunk_size)
            return false;

        const std::size_t produced = chunk_size - zs.avail_out;
        out_buffer.resize(old_size + produced);

        if (produced == 0)
        {
            if (++stall_counter > max_stall_iterations)
                return false;
        }
        else
        {
            stall_counter = 0;
        }
    }

    // Gzip trailer: CRC32 (4 bytes) + ISIZE (4 bytes)
    // in little-endian
    std::array<std::byte, 8> trailer{};
    if constexpr (std::endian::native == std::endian::little)
    {
        std::memcpy(trailer.data(), &crc32, 4);
        std::memcpy(trailer.data() + 4, &isize, 4);
    }
    else
    {
        for (std::size_t i = 0; i < 4; i++)
        {
            trailer[i] = static_cast<std::byte>((crc32 >> (8 * i)) & 0xffU);
            trailer[i + 4] = static_cast<std::byte>((isize >> (8 * i)) & 0xffU);
        }
    }

    return append_bytes(out_buffer, trailer.data(), trailer.size());
}

} // namespace detail

template <typename BufferInType, typename BufferOutType>
bool gzip_encode(const BufferInType& in_buffer, BufferOutType& out_buffer)
{
    return detail::gzip_encode_impl(in_buffer, out_buffer);
}

} // namespace boost::prometheus

#endif
