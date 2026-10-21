//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_STATIC_BASIC_METRIC_HPP
#define BOOST_PROMETHEUS_DETAIL_STATIC_BASIC_METRIC_HPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/config.hpp>
#include <boost/prometheus/detail/const_find_substr.hpp>
#include <boost/prometheus/detail/extract_option.hpp>
#include <boost/prometheus/detail/string_buffer.hpp>
#include <boost/prometheus/detail/static_labels_registrator.hpp>

#include <boost/prometheus/detail/export/static_serialize_header.hpp>

#include <boost/assert.hpp>

#include <tuple>

namespace boost::prometheus::detail {

template <
    template <string_buffer, typename...> typename DerivedTemplate,
    template <typename> typename ImplTemplate,
    string_buffer MetricName,
    typename... Options
>
class basic_static_metric
{
public:

    using labels_type = extract_option<labels, Options...>::type;
    using help_type = extract_option<help, Options...>::type;
    using unit_type = extract_option<unit, Options...>::type;
    using atomic_policy_type = fallback_if_void_t<
        typename extract_option<atomic_policy, Options...>::type,
        default_atomic_policy
    >;
    using derived_type = DerivedTemplate<MetricName, Options...>;
    using impl_type = ImplTemplate<atomic_policy_type>;
    
    // Metric name as detail::string_buffer
    static constexpr string_buffer metric_name = MetricName;
    
    static_assert(metric_name.size() > 0);

    static_assert([]() consteval
    {
        if constexpr (std::is_void_v<unit_type>)
        {
            return true;
        }
        else
        {
            return const_find_substr<metric_name, unit_type::value>();
        }
    }(), "Metric name must include Unit");

    using serialized_header = static_serialize_header<derived_type, MetricName, Options...>;

    inline static impl_type impl_ = impl_type(
        serialized_header::prometheus_format,
        serialized_header::openmetrics_format
    );

    // Returns reference to the static internal object
    template <string_buffer... LabelValues>
    static auto& value_ref()
    {
        if constexpr (!std::is_void_v<labels_type>)
        {
            static_assert(sizeof...(LabelValues) == labels_type::size(), "Incorrect label values count");

            std::size_t index = static_labels_registrator<derived_type, LabelValues...>::index;
            return impl_.value_ref(index);
        }
        else
        {
            static_assert(sizeof...(LabelValues) == 0, "Metric has no labels");

            (void)static_labels_registrator<derived_type>::index;
            BOOST_ASSERT((static_labels_registrator<derived_type>::index == 0));
            return impl_.value_ref(0);
        }
    }

public:

    static consteval bool is_atomic()
    {
        return atomic_policy_type::enabled;
    }

    static std::size_t size() noexcept
    {
        return impl_.size();
    }
};

} // namespace boost::prometheus::detail

#endif
