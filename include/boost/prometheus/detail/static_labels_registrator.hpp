//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_LABELS_REGISTRATOR_HPP
#define BOOST_PROMETHEUS_DETAIL_LABELS_REGISTRATOR_HPP

#include <boost/prometheus/detail/export/static_serialize_bucket_name.hpp>
#include <boost/prometheus/detail/export/static_serialize_name.hpp>
#include <boost/prometheus/detail/export/to_string_buffer.hpp>

#include <boost/prometheus/detail/always_false.hpp>
#include <boost/prometheus/detail/metric_traits.hpp>
#include <boost/prometheus/detail/string_buffer.hpp>

namespace boost::prometheus::detail {

// Registrates static metric with passed labelvalues and returns it's vector index.
template <typename MetricType, string_buffer... LabelValues>
struct static_labels_registrator
{
    inline static std::size_t index = []()
    {
        std::size_t index;
        if constexpr (static_counter_specialization<MetricType> ||
            static_gauge_specialization<MetricType>)
        {
            static constexpr string_buffer formatted_name = static_serialize_name<MetricType, LabelValues...>::value;
            index = MetricType::impl_.add_entry(formatted_name);
        }
        else if constexpr (static_histogram_specialization<MetricType>)
        {
            constexpr std::size_t bucket_count = MetricType::buckets_type::values.size() + 1;
            index = MetricType::impl_.add_entry(bucket_count);
            auto& value_ref = MetricType::impl_.value_ref(index);

            [&]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                ([&]()
                {
                    constexpr double val = MetricType::buckets_type::values[Is];

                    constexpr auto le_value = to_string_buffer<val>();

                    static constexpr string_buffer formatted_bucket = static_serialize_bucket_name<
                        MetricType,
                        le_value,
                        LabelValues...
                    >::value;

                    value_ref.bucket_names_svs_.push_back(formatted_bucket);
                }(), ...);
            }(std::make_index_sequence<MetricType::buckets_type::values.size()>{});

            static constexpr string_buffer formatted_inf_bucket = static_serialize_bucket_name<
                MetricType,
                "+Inf",
                LabelValues...
            >::value;

            value_ref.bucket_names_svs_.push_back(formatted_inf_bucket);

            static constexpr string_buffer formatted_sum = static_serialize_name_impl<
                MetricType::metric_name + string_buffer("_sum"),
                typename MetricType::labels_type,
                LabelValues...
            >::value;

            static constexpr string_buffer formatted_count = static_serialize_name_impl<
                MetricType::metric_name + string_buffer("_count"),
                typename MetricType::labels_type,
                LabelValues...
            >::value;

            value_ref.sum_name_sv_ = formatted_sum;
            value_ref.count_name_sv_ = formatted_count;
        }
        else if constexpr (static_info_specialization<MetricType>)
        {
            static constexpr string_buffer formatted_name =
                static_serialize_name<MetricType, LabelValues...>::value + string_buffer("1\n");
            index = MetricType::impl_.add_entry(formatted_name);
        }
        else
        {
            static_assert(always_false_v<MetricType>);
        }
        return index;
    }();
};

} // namespace boost::prometheus::detail

#endif
