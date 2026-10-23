//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_EXPORT_STATIC_SERIALIZE_NAME_HPP
#define BOOST_PROMETHEUS_DETAIL_EXPORT_STATIC_SERIALIZE_NAME_HPP

#include <boost/prometheus/detail/metric_traits.hpp>
#include <boost/prometheus/detail/static_labelpair.hpp>
#include <boost/prometheus/detail/string_buffer.hpp>
#include <boost/prometheus/detail/tuple_sorting.hpp>

namespace boost::prometheus::detail {

template <auto MetricName, typename PairsValueTuple>
struct format_labels_impl
{
    using sorted_pairs_vt = typename sort_values_impl<PairsValueTuple>::type;
    static constexpr auto sorted_pairs = to_std_tuple<sorted_pairs_vt>::make();

    static constexpr string_buffer value = []() consteval
    {
        // Serialize metric string in compile-time
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) consteval
        {
            auto result = MetricName + string_buffer("{") + ((
                std::get<Is>(sorted_pairs).first +
                string_buffer("=\"") +
                std::get<Is>(sorted_pairs).second +
                string_buffer("\",")) + ...) + string_buffer(" ");
            result.value[result.size() - 2] = '}';
            return result;
        }(std::make_index_sequence<std::tuple_size_v<decltype(sorted_pairs)>>{});
    }();
};

template <auto MetricName, typename LabelsType, auto... LabelValues>
struct static_serialize_name_impl
{
    static constexpr string_buffer value = []() consteval
    {
        if constexpr (sizeof...(LabelValues) > 0)
        {
            constexpr std::size_t N = LabelsType::size();
            static_assert(sizeof...(LabelValues) == N, "The number of label names and values must be equal");
            constexpr auto label_values_tuple = std::make_tuple(LabelValues...);

            // Make Label:LabelValue pairs
            using pairs_vt = decltype([&]<std::size_t... Is>(std::index_sequence<Is...>) consteval
            {
                return value_tuple<static_labelpair(
                    std::get<Is>(LabelsType::values),
                    std::get<Is>(label_values_tuple)
                )...>{};
            }(std::make_index_sequence<N>{}));

            return format_labels_impl<MetricName, pairs_vt>::value;
        }
        else
        {
            return MetricName + string_buffer(" ");
        }
    }();
};

// Formats metric name with all labels
template <typename MetricType, auto... LabelValues>
struct static_serialize_name
{
    static constexpr string_buffer value = []() consteval
    {
        using labels_type = MetricType::base_type::labels_type;
        constexpr string_buffer metric_name = []() consteval
        {
            if constexpr (static_counter_specialization<MetricType>)
                return MetricType::metric_name + string_buffer("_total");
            else if constexpr (static_info_specialization<MetricType>)
                return MetricType::metric_name + string_buffer("_info");
            else
                return MetricType::metric_name;
        }();

        return static_serialize_name_impl<metric_name, labels_type, LabelValues...>::value;
    }();
};

} // namespace boost::prometheus::detail

#endif
