//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_IMPL_COUNTER_IPP
#define BOOST_PROMETHEUS_IMPL_COUNTER_IPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/counter.hpp>

namespace boost::prometheus {

template <typename AtomicPolicy>
counter<AtomicPolicy>::internal_type
counter<AtomicPolicy>::init_internal(std::string_view name, const metric_opts& options)
{
    BOOST_ASSERT_MSG(name.find("_total") == std::string_view::npos,
        "Counter name shouldn't contain \"_total\" suffix; it will be appended automatically");
    auto prefixes = detail::serialize_prefix<
        detail::metric_type_string<counter>::prometheus_type,
        detail::metric_type_string<counter>::openmetrics_type
    >(name, options);
    return internal_type(std::move(prefixes.base_string), prefixes.openmetrics_prefix_offset);
}

#ifdef BOOST_PROMETHEUS_SEPARATE_COMPILATION

template counter<atomic_policy<true>>::internal_type
counter<atomic_policy<true>>::init_internal(std::string_view, const metric_opts&);
template counter<atomic_policy<false>>::internal_type
counter<atomic_policy<false>>::init_internal(std::string_view, const metric_opts&);

#endif

} // namespace boost::prometheus

#endif
