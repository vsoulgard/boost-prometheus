//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_EXPOSER_OPTIONS_HPP
#define BOOST_PROMETHEUS_EXPOSER_OPTIONS_HPP

#include <boost/prometheus/detail/string_buffer.hpp>

namespace boost::prometheus {

/**
 * \brief Enables or disables gzip compression of responses.
 *
 * When enabled, responses larger than 1024 bytes are gzip-compressed if
 * the client sends `Accept-Encoding: gzip`.
 *
 * \tparam Enabled `true` to enable compression, `false` to disable.
 */
template <bool Enabled>
struct gzip_policy
{
    static constexpr bool enabled = Enabled;
};

/**
 * \brief Default compression policy: gzip enabled.
 */
using default_gzip_policy = gzip_policy<true>;

/**
 * \brief Enables or disables SSL/TLS mode for the exposer.
 *
 * Enabling this policy requires including
 * `<boost/prometheus/exposer_ssl_policy.hpp>` and linking against OpenSSL.
 * This primary template enforces that requirement with a `static_assert`.
 *
 * \tparam Enabled `true` for TLS mode, `false` for plain HTTP.
 */
template <bool Enabled>
struct ssl_policy
{
    static_assert(!Enabled,
        "SSL is not enabled. Include boost/prometheus/exposer_ssl_policy.hpp "
        "and link against OpenSSL to use ssl_policy<true>.");
    static constexpr bool enabled = Enabled;
};

/**
 * \brief Default SSL policy: plain HTTP.
 */
using default_ssl_policy = ssl_policy<false>;

/**
 * \brief Size in bytes of the buffer used to parse incoming HTTP requests.
 *
 * Increase this if clients send unusually large headers, such as long
 * bearer tokens.
 *
 * \tparam Value Buffer size in bytes.
 */
template <std::size_t Value>
struct parse_buffer_size
{
    static constexpr std::size_t value = Value;
};

/**
 * \brief Default request-parse buffer size: 8192 bytes.
 */
using default_parse_buffer_size = parse_buffer_size<8192>;

/**
 * \brief Maximum allowed size in bytes of a single HTTP header block.
 *
 * Requests whose headers exceed this limit are rejected by
 * connection reset.
 *
 * \tparam Value Header size limit in bytes.
 */
template <std::size_t Value>
struct header_limit_size
{
    static constexpr std::size_t value = Value;
};

/**
 * \brief Default header size limit: 4096 bytes.
 */
using default_header_limit_size = header_limit_size<4096>;

/**
 * \brief Exposes a `static_registry` alongside the dynamic registry.
 *
 * When this policy is present, the exposer serves metrics from both the
 * runtime `prometheus::registry` and the given compile-time static registry.
 *
 * \tparam StaticRegistryType The static registry type to expose.
 */
template <typename StaticRegistryType>
struct expose_static_registry
{
    using type = StaticRegistryType;
};

/**
 * \brief Enables `Authorization` header validation.
 *
 * On every request, the value of the `Authorization` header is passed to
 * `ValidateFunc`. If it returns `false`, the exposer replies with
 * `401 Unauthorized` (or `404 Not Found` when `ReturnNotFound` is `true`).
 *
 * \tparam AuthType       Scheme name placed in the `WWW-Authenticate` header,
 *                       e.g. `"Bearer"` or `"Basic"`.
 * \tparam ValidateFunc   `constexpr` or `static` callable with signature
 *                       `bool(std::string_view)`. Must be usable as an NTTP.
 * \tparam Realm          Realm placed in the `WWW-Authenticate` header.
 *                       Empty by default.
 * \tparam ReturnNotFound When `true`, unauthorized requests receive
 *                       `404 Not Found` instead of `401 Unauthorized`.
 *                       Useful for hiding the existence of the endpoint.
 */
template <
    detail::string_buffer AuthType,
    auto ValidateFunc,
    detail::string_buffer Realm = "",
    bool ReturnNotFound = false>
struct authorization_policy
{
    static_assert(AuthType.size() > 0, "AuthType cannot be empty");
    static constexpr detail::string_buffer auth_type = AuthType;
    static constexpr detail::string_buffer realm = Realm;
    static constexpr bool return_not_found = ReturnNotFound;

    /**
     * \brief Runs `ValidateFunc` on the provided `Authorization` value.
     */
    static bool validate(std::string_view authorization)
    {
        return ValidateFunc(authorization);
    }
};

} // namespace boost::prometheus

#endif
