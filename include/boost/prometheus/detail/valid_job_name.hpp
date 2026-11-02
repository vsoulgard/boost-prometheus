//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_VALID_JOB_NAME_HPP
#define BOOST_PROMETHEUS_DETAIL_VALID_JOB_NAME_HPP

#include <string_view>

namespace boost::prometheus::detail {

inline bool valid_job_name(std::string_view name) noexcept
{
    if (name.empty()) return false;
    for (char c : name)
        if (c == '/' || c == '\n' || c == '\r' || c == ' ') return false;
    return true;
}

} // namespace boost::prometheus::detail

#endif
