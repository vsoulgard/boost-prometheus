//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_CONST_FIND_SUBSTR_HPP
#define BOOST_PROMETHEUS_DETAIL_CONST_FIND_SUBSTR_HPP

#include <boost/prometheus/detail/string_buffer.hpp>

#include <string_view>

namespace boost::prometheus::detail {

// Custom find(), because
// std::string_view().find()
// is not constexpr in some workarounds.
template <string_buffer Str, string_buffer Substr>
consteval bool const_find_substr()
{
    constexpr std::string_view haystack(Str);
    constexpr std::string_view needle(Substr);

    if (needle.empty()) return true;
    if (needle.size() > haystack.size()) return false;

    bool found = false;
    for (std::size_t i = 0; i <= haystack.size() - needle.size(); i++)
    {
        bool match = true;
        for (std::size_t j = 0; j < needle.size(); j++)
        {
            if (haystack[i + j] != needle[j])
            {
                match = false;
                break;
            }
        }
        if (match) { found = true; break; }
    }
    return found;
}

} // namespace boost::prometheus::detail

#endif
