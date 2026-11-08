//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_GAUGE_INTERNAL_HPP
#define BOOST_PROMETHEUS_DETAIL_GAUGE_INTERNAL_HPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/basic_metric_internal.hpp>
#include <boost/prometheus/detail/config.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <atomic>

namespace boost::prometheus::detail {

BOOST_PROMETHEUS_MSVC_DISABLE_ALIGN_WARNING
template <typename T, typename AtomicPolicyType, bool HasPrefixSVs>
struct gauge_internal : public basic_metric_internal<AtomicPolicyType, true, HasPrefixSVs>
{
    using base_type = basic_metric_internal<AtomicPolicyType, true, HasPrefixSVs>;

    using value_type = std::conditional_t<
        AtomicPolicyType::enabled,
        std::atomic<T>,
        T>;

    alignas(base_type::template alignment<value_type>)
    value_type value_;

    gauge_internal(std::string&& base_string)
    requires (!HasPrefixSVs);

    gauge_internal(std::string&& base_string, std::size_t openmetrics_prefix_offset)
    requires (HasPrefixSVs);

    consteval static bool is_atomic() { return AtomicPolicyType::enabled; }

    T get_value() const noexcept;

    void inc(T value) noexcept;

    void dec(T value) noexcept;

    void set(T value) noexcept;
};
BOOST_PROMETHEUS_MSVC_RESTORE_WARNING

} // namespace boost::prometheus::detail

#ifdef BOOST_PROMETHEUS_HEADER_ONLY
#include <boost/prometheus/impl/gauge_internal.ipp>
#endif

#endif
