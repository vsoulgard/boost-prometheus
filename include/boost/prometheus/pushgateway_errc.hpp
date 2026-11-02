//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_PUSHGATEWAY_ERRC_HPP
#define BOOST_PROMETHEUS_PUSHGATEWAY_ERRC_HPP

#include <boost/system/error_code.hpp>

namespace boost {
namespace prometheus {

enum class pushgateway_errc : int
{
    incorrect_job_name = 1
};

namespace error {
    
inline const boost::system::error_category& get_pushgateway_category() noexcept;

} // namespace error

inline boost::system::error_code make_error_code(pushgateway_errc error)
{
    return boost::system::error_code(static_cast<int>(error), error::get_pushgateway_category());
}

inline std::ostream& operator<<(std::ostream& os, pushgateway_errc e)
{
    return os << make_error_code(e);
}

} // namespace prometheus

namespace system {

template <>
struct is_error_code_enum<::boost::prometheus::pushgateway_errc>
{
    static constexpr bool value = true;
};

} // namespace system

} // namespace boost

#include <boost/prometheus/detail/error_categories.hpp>

#endif
