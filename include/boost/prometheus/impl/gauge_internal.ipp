//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_IMPL_GAUGE_INTERNAL_IPP
#define BOOST_PROMETHEUS_IMPL_GAUGE_INTERNAL_IPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/gauge_internal.hpp>

namespace boost::prometheus::detail {

template <typename T, typename AtomicPolicyType, bool HasPrefixSVs>
gauge_internal<T, AtomicPolicyType, HasPrefixSVs>::gauge_internal(std::string&& base_string)
    requires (!HasPrefixSVs)
: base_type(std::move(base_string)), value_() {}

template <typename T, typename AtomicPolicyType, bool HasPrefixSVs>
gauge_internal<T, AtomicPolicyType, HasPrefixSVs>::gauge_internal(
    std::string&& base_string, std::size_t offset)
    requires (HasPrefixSVs)
: base_type(std::move(base_string), offset), value_() {}

template <typename T, typename AtomicPolicyType, bool HasPrefixSVs>
T gauge_internal<T, AtomicPolicyType, HasPrefixSVs>::get_value() const noexcept
{
    if constexpr (AtomicPolicyType::enabled)
        return value_.load(std::memory_order_relaxed);
    else
        return value_;
}

template <typename T, typename AtomicPolicyType, bool HasPrefixSVs>
void gauge_internal<T, AtomicPolicyType, HasPrefixSVs>::inc(T v) noexcept
{
    if constexpr (AtomicPolicyType::enabled) value_.fetch_add(v, std::memory_order_relaxed);
    else                                     value_ += v;
}

template <typename T, typename AtomicPolicyType, bool HasPrefixSVs>
void gauge_internal<T, AtomicPolicyType, HasPrefixSVs>::dec(T v) noexcept
{
    if constexpr (AtomicPolicyType::enabled) value_.fetch_sub(v, std::memory_order_relaxed);
    else                                     value_ -= v;
}

template <typename T, typename AtomicPolicyType, bool HasPrefixSVs>
void gauge_internal<T, AtomicPolicyType, HasPrefixSVs>::set(T v) noexcept
{
    if constexpr (AtomicPolicyType::enabled) value_.store(v, std::memory_order_relaxed);
    else                                     value_ = v;
}

#ifdef BOOST_PROMETHEUS_SEPARATE_COMPILATION

#define BOOST_PROMETHEUS_INSTANTIATE_GAUGE(T)                       \
    template struct gauge_internal<T, atomic_policy<true>, false>;  \
    template struct gauge_internal<T, atomic_policy<false>, false>; \
    template struct gauge_internal<T, atomic_policy<true>, true>;   \
    template struct gauge_internal<T, atomic_policy<false>, true>;

BOOST_PROMETHEUS_INSTANTIATE_GAUGE(double)
BOOST_PROMETHEUS_INSTANTIATE_GAUGE(std::uint64_t)
BOOST_PROMETHEUS_INSTANTIATE_GAUGE(std::int64_t)

#endif

} // namespace boost::prometheus::detail

#endif
