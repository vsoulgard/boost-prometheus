//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_EXPORT_SERIALIZE_NAME_WITH_LABELS_HPP
#define BOOST_PROMETHEUS_DETAIL_EXPORT_SERIALIZE_NAME_WITH_LABELS_HPP

#include <boost/prometheus/detail/string_buffer.hpp>

#include <boost/assert.hpp>

#include <algorithm>
#include <cstring>
#include <string>
#include <string_view>

namespace boost::prometheus::detail {

template <string_buffer Suffix = "", typename LabelsWithIndexType, typename LabelValuesType>
std::string serialize_name_with_labels(
    std::string_view name,
    const LabelsWithIndexType& labels,
    const LabelValuesType& labelvalues
)
{
    BOOST_ASSERT_MSG(name.size() > 0, "Metric name cannot be empty");
    BOOST_ASSERT_MSG(std::is_sorted(labels.cbegin(), labels.cend()), "Label names must be sorted");

    std::string result;
    constexpr std::string_view suffix(Suffix);
    std::size_t required_size = name.size() + 2 + suffix.size();
    for (std::size_t i = 0; i < labels.size(); i++)
    {
        required_size += labels[i].first.size() + labelvalues[i].size() + 4;
    }
    result.resize(required_size);
    char* data = result.data();
    std::size_t off = 0;
    std::memcpy(data + off, name.data(), name.size());
    off += name.size();
    if constexpr (suffix.size())
    {
        std::memcpy(data + off, suffix.data(), suffix.size());
        off += suffix.size();
    }
    std::memcpy(data + off, "{", 1);
    off += 1;
    for (std::size_t i = 0; i < labels.size(); i++)
    {
        // Get original index for the label.
        std::size_t label_index = labels[i].second;
        std::memcpy(
            data + off,
            labels[i].first.data(),
            labels[i].first.size());
        off += labels[i].first.size();
        std::memcpy(data + off, "=\"", 2);
        off += 2;
        std::memcpy(
            data + off,
            labelvalues[label_index].data(),
            labelvalues[label_index].size());
            off += labelvalues[label_index].size();
        std::memcpy(data + off, "\",", 2);
        off += 2;
    }
    std::memcpy(data + off - 1, "}", 1);
    std::memcpy(data + off, " ", 1);
    BOOST_ASSERT(result.size() == required_size);
    return result;
}

template <typename LabelsWithIndexType, typename LabelValuesType>
std::string serialize_counter_name_with_labels(
    std::string_view name,
    const LabelsWithIndexType& labels,
    const LabelValuesType& labelvalues
)
{
    return serialize_name_with_labels<"_total">(name, labels, labelvalues);
}

} // namespace boost::prometheus::detail

#endif
