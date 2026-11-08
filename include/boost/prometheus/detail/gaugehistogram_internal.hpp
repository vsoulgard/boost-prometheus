//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_GAUGEHISTOGRAM_INTERNAL_HPP
#define BOOST_PROMETHEUS_DETAIL_GAUGEHISTOGRAM_INTERNAL_HPP

#include <boost/prometheus/detail/config.hpp>
#include <boost/prometheus/detail/histogram_bucket_value.hpp>
#include <boost/prometheus/detail/histogram_internal.hpp>
#include <boost/prometheus/detail/string_buffer.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <array>
#include <atomic>
#include <mutex>
#include <vector>
#include <string>
#include <span>

namespace boost::prometheus::detail {

template <typename AtomicPolicy, bool HasBaseString = true, bool HasPrefixSVs = true>
struct gaugehistogram_internal : public histogram_internal<AtomicPolicy, HasBaseString, HasPrefixSVs>
{
    using base_type = histogram_internal<AtomicPolicy, HasBaseString, HasPrefixSVs>;

    // Falback 0.0.4 headers
    const std::string sum_header;
    const std::string count_header;

    gaugehistogram_internal(
        std::span<const double> bucket_boundaries,
        std::string&& base_histogram_string,
        const std::vector<std::size_t>& bucket_offsets,
        std::size_t sum_offset,
        std::size_t count_offset,
        std::string&& sum_header,
        std::string&& count_header
    ) requires (!(HasBaseString && HasPrefixSVs));

    gaugehistogram_internal(
        std::string&& base_string,
        std::size_t openmetrics_prefix_offset,
        std::span<const double> bucket_boundaries,
        std::string&& base_histogram_string,
        const std::vector<std::size_t>& bucket_offsets,
        std::size_t sum_offset,
        std::size_t count_offset,
        std::string&& sum_header,
        std::string&& count_header
    ) requires (HasBaseString && HasPrefixSVs);

    void sub(double value, std::uint64_t count) noexcept;
};

} // namespace boost::prometheus::detail

#ifdef BOOST_PROMETHEUS_HEADER_ONLY
#include <boost/prometheus/impl/gaugehistogram_internal.ipp>
#endif

#endif
