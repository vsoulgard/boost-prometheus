//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_EXPORT_TO_STRING_BUFFER_HPP
#define BOOST_PROMETHEUS_DETAIL_EXPORT_TO_STRING_BUFFER_HPP

#include <boost/prometheus/detail/string_buffer.hpp>

#include <array>
#include <bit>
#include <concepts>
#include <cmath>
#include <cstddef>
#include <limits>

namespace boost::prometheus::detail {

template <std::floating_point T>
[[nodiscard]] consteval bool constexpr_signbit(T value) noexcept
{
    if constexpr (sizeof(T) == sizeof(std::uint32_t))
    {
        return (std::bit_cast<std::uint32_t>(value) >> 31) != 0u;
    }
    else if constexpr (sizeof(T) == sizeof(std::uint64_t))
    {
        return (std::bit_cast<std::uint64_t>(value) >> 63) != 0u;
    }
    else
    {
        const auto bytes = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);

        constexpr std::size_t significant =
            (std::numeric_limits<T>::digits == 64) ? 10u : sizeof(T);

        constexpr std::size_t sign_byte =
            (std::endian::native == std::endian::little)
                ? significant - 1u
                : 0u;

        return (std::to_integer<unsigned char>(bytes[sign_byte]) & 0x80u) != 0u;
    }
}

struct U128
{
    std::uint32_t w[4]{};

    constexpr bool is_zero() const
    {
        return (w[0] | w[1] | w[2] | w[3]) == 0;
    }
    constexpr bool fits_u64() const
    { 
        return w[2] == 0 && w[3] == 0;
    }
    constexpr std::uint64_t to_u64() const
    {
        return std::uint64_t(w[0]) | (std::uint64_t(w[1]) << 32);
    }

    constexpr bool shl(unsigned n)
    {
        if (n == 0)
            return false;
        if (n >= 128) 
        {
            bool lost = !is_zero();
            *this = {}; return lost;
        }
        unsigned ws = n / 32, bs = n % 32;
        bool lost = false;
        if (ws > 0)
        {
            for (std::size_t i = 4 - ws; i < 4; i++)
            if (w[i]) lost = true;
            for (std::size_t i = 3; i >= ws; i--)
                w[i] = w[i - ws];
            for (std::size_t i = 0; i < ws; i++)
                w[i] = 0;
        }
        if (bs > 0)
        {
            std::uint32_t prev = 0;
            for (std::size_t i = 0; i < 4; i++)
            {
                std::uint32_t cur = w[i];
                w[i] = (cur << bs) | prev;
                prev = cur >> (32 - bs);
            }
            if (prev) lost = true;
        }
        return lost;
    }

    constexpr void shr_round(unsigned n)
    {
        if (n == 0) return;
        if (n >= 128)
        {
            *this = {};
            return;
        }
        unsigned ws = n / 32, bs = n % 32;

        unsigned round_bit = 0;
        bool sticky = false;
        if (bs == 0)
        {
            if (ws > 0)
            {
                round_bit = (w[ws - 1] >> 31) & 1u;
                if (w[ws - 1] & 0x7FFFFFFFu) sticky = true;
                for (std::size_t i = 0; i + 1 < ws; i++)
                    if (w[i]) sticky = true;
            }
        }
        else
        {
            round_bit = (w[ws] >> (bs - 1)) & 1u;
            if (bs >= 2)
            {
                unsigned mask = (1u << (bs - 1)) - 1;
                if (w[ws] & mask) sticky = true;
            }
            for (std::size_t i = 0; i < ws; i++) 
                if (w[i]) sticky = true;
        }

        if (ws > 0)
        {
            for (int i = 0; i + int(ws) < 4; i++)
                w[i] = w[i + ws];
            for (int i = 4 - int(ws); i < 4; i++)
                w[i] = 0;
        }
        if (bs > 0)
        {
            for (std::size_t i = 0; i + 1 < 4; i++)
                w[i] = (w[i] >> bs) | (w[i + 1] << (32 - bs));
            w[3] >>= bs;
        }

        bool round_up = round_bit && (sticky || (w[0] & 1u));
        if (round_up)
        {
            std::uint64_t carry = 1;
            for (std::size_t i = 0; i < 4 && carry; i++)
            {
                std::uint64_t s = std::uint64_t(w[i]) + carry;
                w[i] = std::uint32_t(s);
                carry = s >> 32;
            }
        }
    }
};

constexpr U128 umul64(std::uint64_t a, std::uint64_t b)
{
    std::uint32_t aw[2]{std::uint32_t(a), std::uint32_t(a >> 32)};
    std::uint32_t bw[2]{std::uint32_t(b), std::uint32_t(b >> 32)};
    std::uint32_t r[4]{};
    for (std::size_t i = 0; i < 2; i++)
    {
        std::uint64_t carry = 0;
        for (std::size_t j = 0; j < 2; j++)
        {
            std::uint64_t p = std::uint64_t(aw[i]) * bw[j] + r[i + j] + carry;
            r[i + j] = std::uint32_t(p);
            carry = p >> 32;
        }
        std::size_t k = i + 2;
        while (carry && k < 4)
        {
            std::uint64_t p = r[k] + carry;
            r[k] = std::uint32_t(p);
            carry = p >> 32;
            k++;
        }
    }
    U128 out;
    for (std::size_t i = 0; i < 4; i++)
        out.w[i] = r[i];
    return out;
}

// `std::to_chars(..., double)` isn't constexpr.
// We use this function for bucket values serialization.
// Unlike `std::to_chars(..., double, std::chars_format::fixed, precision)`,
// this function removes trailing zeros in the fractional part.
template <double Value, std::size_t Precision = 8>
consteval auto to_string_buffer()
{
    static_assert(Precision <= 18, "Precision too high");
    static_assert(Value == Value, "Value is NaN");
    static_assert(Value != std::numeric_limits<double>::infinity() &&
                  Value != -std::numeric_limits<double>::infinity(), "Value is inf");

    constexpr auto fmt = []() consteval
    {
        constexpr std::size_t buffer_size = 32 + Precision;
        std::array<char, buffer_size> buffer{};
        std::size_t idx = 0;

        std::uint64_t raw_bits = std::bit_cast<std::uint64_t>(Value);
        bool negative = constexpr_signbit(Value);
        std::uint64_t bits = raw_bits & ~(std::uint64_t(1) << 63);

        std::uint32_t raw_exp = std::uint32_t((bits >> 52) & 0x7FFu);
        std::uint64_t mant = bits & ((std::uint64_t(1) << 52) - 1);

        std::uint64_t m;
        std::int32_t e;
        if (raw_exp == 0)
        {
            m = mant;
            e = -1074;
        }
        else
        {
            m = mant | (std::uint64_t(1) << 52);
            e = std::int32_t(raw_exp) - 1075;
        }

        std::uint64_t pow5 = 1;
        for (std::size_t i = 0; i < Precision; i++)
        {
            pow5 *= 5;
        }

        U128 A = umul64(m, pow5);

        std::int32_t shift = e + std::int32_t(Precision);
        if (shift >= 0)
        {
            if (A.shl(unsigned(shift)))
                throw "Scaled value overflows 64 bits";
        }
        else
        {
            A.shr_round(unsigned(-shift));
        }
        if (!A.fits_u64())
        {
            throw "Scaled value overflows 64 bits";
        }

        std::uint64_t scaled = A.to_u64();

        std::array<char, 32> temp{};
        std::size_t t_idx = 0;
        if (scaled == 0)
        {
            temp[t_idx++] = '0';
        }
        while (scaled > 0)
        {
            temp[t_idx++] = char('0' + (scaled % 10));
            scaled /= 10;
        }
        while (t_idx < Precision + 1)
        {
            temp[t_idx++] = '0';
        }

        if (negative)
        {
            buffer[idx++] = '-';
        }

        for (std::size_t i = t_idx; i > Precision; i--)
        {
            buffer[idx++] = temp[i - 1];
        }

        if constexpr (Precision > 0)
        {
            std::size_t frac_start = Precision;
            for (std::size_t i = 0; i < Precision; i++)
            {
                if (temp[i] != '0') 
                {
                    frac_start = i;
                    break;
                }
            }
            std::size_t frac_count = Precision - frac_start;
            if (frac_count > 0)
            {
                buffer[idx++] = '.';
                for (std::size_t i = 0; i < frac_count; i++)
                {
                    buffer[idx++] = temp[Precision - 1 - i];
                }
            }
        }

        buffer[idx] = '\0';
        return std::pair{buffer, idx + 1};
    }();

    string_buffer<fmt.second> result{};
    for (std::size_t i = 0; i < fmt.second; i++)
    {
        result.value[i] = fmt.first.data()[i];
    }
    return result;
}

} // namespace boost::prometheus::detail

#endif
