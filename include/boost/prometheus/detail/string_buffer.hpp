//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_STRING_BUFFER_HPP
#define BOOST_PROMETHEUS_DETAIL_STRING_BUFFER_HPP

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace boost::prometheus::detail {

/**
 * \brief Fixed-size compile-time string buffer.
 *
 * A `string_buffer<N>` stores up to `N - 1` characters plus a null
 * terminator. The size parameter `N` includes the terminator: to hold
 * `"hello"`, use `string_buffer<6>`.
 *
 * The type is designed to be used as a non-type template parameter
 * (NTTP). When passed as an NTTP to a template like
 * `template <string_buffer Name>`, the compiler deduces `N` from the
 * string literal's length, so users write:
 *
 * \code
 * template <string_buffer Name> struct metric_name { ... };
 * metric_name<"requests"> m;
 * \endcode
 *
 * and `Name` is a `string_buffer<9>` containing `"requests\0"`.
 *
 * All non-trivial operations — construction from a literal,
 * concatenation, comparison — are `consteval`: they can only be
 * evaluated at compile time. Two operations, `size()` and
 * `operator std::string_view()`, are `constexpr` and available in both
 * contexts.
 *
 * \tparam N Total buffer size, **including** the null terminator.
 *
 * \par Example
 * \code
 * constexpr string_buffer name = "requests";   // N = 9
 * static_assert(name.size() == 8);
 * static_assert(name + "_total" == "requests_total");
 *
 * constexpr string_buffer prefix = "http_";
 * constexpr string_buffer full = prefix + name;  // "http_requests"
 * \endcode
 *
 * \note This is a `detail::` type. User code should not instantiate or
 *       manipulate `string_buffer` directly; it is exposed only because
 *       NTTP parameters of public API (e.g. `static_counter<...>`,
 *       `static_registry<...>`) are of this type.
 */
template <std::size_t N>
struct string_buffer
{
    char value[N];

    /**
     * \brief Constructs an uninitialized buffer.
     *
     * Intended for internal use in contexts where every byte is written
     * immediately afterwards (e.g. by `operator+`). Reading `value`
     * before writing it is undefined behavior, and in a `consteval`
     * context it is a compile-time error.
     *
     * \warning Do not default-construct a `string_buffer` and read from
     *          it without first writing all `N` bytes.
     */
    consteval string_buffer() = default;

    /**
     * \brief Constructs a buffer by copying a string literal.
     *
     * The literal must have exactly `N` characters including the null
     * terminator. For a literal `"hello"` (6 characters including the
     * terminator), `N` deduces to 6.
     *
     * \param str Source literal. Its `N` bytes (including the
     *            terminator) are copied verbatim.
     */
    consteval string_buffer(const char (&str)[N])
    {
        std::copy_n(str, N, value);
    }

    /**
     * \brief Returns the number of characters, excluding the null
     *        terminator.
     *
     * For `string_buffer<6>{"hello"}`, returns `5`.
     */
    consteval std::size_t size() const { return N - 1; }

    /**
     * \brief Implicit conversion to `std::string_view` over the stored
     *        characters, excluding the null terminator.
     */
    constexpr operator std::string_view() const
    {
        return std::string_view(value, N - 1);
    }

    /**
     * \brief Lexicographic comparison, ignoring the null terminator.
     *
     * Two buffers compare by their `std::string_view` views, so
     * `string_buffer<4>{"hell"}` orders before
     * `string_buffer<6>{"hello"}`. This enables compile-time sorting of
     * label names before serialization.
     *
     * \param lhs First buffer.
     * \param rhs Second buffer, possibly with a different size.
     * \return `true` if \p lhs is lexicographically less than \p rhs.
     */
    template <std::size_t M>
    friend consteval bool operator<(const string_buffer<N> lhs, const string_buffer<M>& rhs)
    {
        return std::string_view(lhs.value, lhs.size()) < std::string_view(rhs.value, rhs.size());
    }

    /**
     * \brief Compile-time concatenation of two buffers.
     *
     * The result has size `N + M - 1`: both operands contribute their
     * `N - 1` and `M - 1` characters, plus one null terminator for the
     * combined buffer.
     *
     * \param lhs Left operand.
     * \param rhs Right operand.
     * \return A `string_buffer<N + M - 1>` containing
     *         `lhs.view() + rhs.view()`.
     */
    template <std::size_t M>
    friend consteval auto operator+(string_buffer<N> lhs, string_buffer<M> rhs)
    {
        string_buffer<N + M - 1> result;

        std::copy_n(lhs.value, N - 1, result.value);
        std::copy_n(rhs.value, M, result.value + N - 1);

        return result;
    }

    /**
     * \overload
     *
     * \param lhs Left buffer.
     * \param rhs Right string literal of length `M - 1` (`M` includes
     *            the terminator).
     */
    template <std::size_t M>
    friend consteval auto operator+(string_buffer<N> lhs, const char (&rhs)[M])
    {
        string_buffer<N + M - 1> result;

        std::copy_n(lhs.value, N - 1, result.value);
        std::copy_n(rhs, M, result.value + N - 1);

        return result;
    }

    /**
     * \overload
     *
     * \param lhs Left string literal of length `M - 1` (`M` includes
     *            the terminator).
     * \param rhs Right buffer.
     */
    template <std::size_t M>
    friend consteval auto operator+(const char (&lhs)[M], string_buffer<N> rhs)
    {
        string_buffer<N + M - 1> result;

        std::copy_n(lhs, M - 1, result.value);
        std::copy_n(rhs.value, N, result.value + M - 1);

        return result;
    }
};

} // namespace boost::prometheus::detail

#endif
