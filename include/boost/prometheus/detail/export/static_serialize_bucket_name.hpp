//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_EXPORT_STATIC_SERIALIZE_BUCKET_NAME_HPP
#define BOOST_PROMETHEUS_DETAIL_EXPORT_STATIC_SERIALIZE_BUCKET_NAME_HPP

#include <boost/prometheus/detail/export/static_serialize_name.hpp>

#include <boost/prometheus/detail/string_buffer.hpp>

namespace boost::prometheus::detail {

template <typename HistogramType, string_buffer LeValue, string_buffer... LabelValues>
struct static_serialize_bucket_name
{
    static constexpr string_buffer value = []() consteval
    {
        using labels_type = HistogramType::base_type::labels_type;
        constexpr string_buffer metric_name = HistogramType::metric_name + string_buffer("_bucket");

        if constexpr (sizeof...(LabelValues) > 0)
        {
            constexpr std::size_t N = labels_type::size() + 1;

            constexpr auto labels_tuple = std::tuple_cat(labels_type::values,
                std::make_tuple(string_buffer("le")));
            constexpr auto label_values_tuple = std::tuple_cat(std::make_tuple(LabelValues...),
                std::make_tuple(LeValue));

            // Make Label:LabelValue pairs
            using pairs_vt = decltype([&]<std::size_t... Is>(std::index_sequence<Is...>) consteval
            {
                return value_tuple<static_labelpair(
                    std::get<Is>(labels_tuple),
                    std::get<Is>(label_values_tuple)
                )...>{};
            }(std::make_index_sequence<N>{}));

            return format_labels_impl<metric_name, pairs_vt>::value;
        }
        else
        {
            // only "le" label
            return metric_name + string_buffer("{le=\"") + LeValue + string_buffer("\"} ");
        }
    }();
};

} // namespace boost::prometheus::detail

#endif
