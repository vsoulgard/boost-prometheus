//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_HISTOGRAM_ITEM_HPP
#define BOOST_PROMETHEUS_HISTOGRAM_ITEM_HPP

#include <boost/prometheus/detail/histogram_internal.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

#include <vector>
#include <string>
#include <span>

namespace boost::prometheus {

namespace detail {

struct histogram_item_params
{
    std::span<const double> bucket_boundaries;
    std::string base_string;
    std::vector<std::size_t> buckets_offsets;
    std::size_t sum_offset;
    std::size_t count_offset;
};

}

// Item type for `histogram_vec`
template <typename AtomicPolicyType>
class histogram_item
{
    using internal_type = detail::histogram_internal<AtomicPolicyType, false, false>;

    internal_type internal_;

    template <typename, typename, typename>
    friend class detail::basic_metric_vec;

    // Serialization
    friend class boost::prometheus::format;

public:

    histogram_item() = delete;

    histogram_item(detail::histogram_item_params&& params)
    : internal_(
        params.bucket_boundaries,
        std::move(params.base_string),
        params.buckets_offsets,
        params.sum_offset,
        params.count_offset
    ) {}

    consteval static bool is_atomic()
    {
        return internal_type::is_atomic();
    }

    void observe(double value, std::uint64_t count = 1) noexcept
    {
        internal_.observe(value, count);
    }
};

} // namespace boost::prometheus

#endif
