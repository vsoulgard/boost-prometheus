//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_INFO_ITEM_HPP
#define BOOST_PROMETHEUS_INFO_ITEM_HPP

#include <boost/prometheus/detail/info_internal.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

namespace boost::prometheus {

// Item type for `info_vec`
template <typename AtomicPolicyType>
class info_item
{
    using internal_type = detail::info_internal<true>;

    internal_type internal_;

    template <typename, typename, typename>
    friend class detail::basic_metric_vec;

    // Serialization
    friend class boost::prometheus::format;

public:

    info_item() = delete;

    info_item(std::string&& formatted_name)
    : internal_(std::move(formatted_name)) {}

    consteval static bool is_atomic()
    {
        return AtomicPolicyType::enabled;
    }
};

} // namespace boost::prometheus

#endif
