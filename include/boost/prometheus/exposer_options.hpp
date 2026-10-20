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

template <bool Enabled>
struct gzip_policy
{
    static constexpr bool enabled = Enabled;
};

using default_gzip_policy = gzip_policy<true>;

template <bool Enabled>
struct ssl_policy
{
    static_assert(!Enabled,
        "SSL is not enabled. Include boost/prometheus/exposer_ssl_policy.hpp "
        "and link against OpenSSL to use ssl_policy<true>.");
    static constexpr bool enabled = Enabled;
};

using default_ssl_policy = ssl_policy<false>;

template <std::size_t Value>
struct parse_buffer_size
{
    static constexpr std::size_t value = Value;
};

// 8192 is enough to parse every scraper's headers, unless
// extensive headers like large authorization tokens are used.
using default_parse_buffer_size = parse_buffer_size<8192>;

template <std::size_t Value>
struct header_limit_size
{
    static constexpr std::size_t value = Value;
};

using default_header_limit_size = header_limit_size<4096>;

template <typename StaticRegistryType>
struct expose_static_registry
{
    using type = StaticRegistryType;
};

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

    static bool validate(std::string_view authorization)
    {
        return ValidateFunc(authorization);
    }
};

} // namespace boost::prometheus

#endif
