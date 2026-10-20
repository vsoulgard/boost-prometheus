//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_HISTOGRAM_BUCKET_VALUE_HPP
#define BOOST_PROMETHEUS_DETAIL_HISTOGRAM_BUCKET_VALUE_HPP

#include <atomic>
#include <cstdint>
#include <type_traits>

namespace boost::prometheus::detail {

template <typename AtomicPolicyType, std::size_t Alignment, typename IntType = std::uint64_t>
struct histogram_bucket_value
{
    using count_type = std::conditional_t<
        AtomicPolicyType::enabled,
        std::atomic<IntType>,
        IntType
    >;
    alignas(Alignment) count_type value;
};

} // namespace boost::prometheus::detail

#endif
