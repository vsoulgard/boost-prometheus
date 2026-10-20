//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_BASIC_METRIC_VEC_HPP
#define BOOST_PROMETHEUS_DETAIL_BASIC_METRIC_VEC_HPP

#include <boost/prometheus/detail/export/serialize_prefix.hpp>

#include <boost/unordered/concurrent_node_map.hpp>
#include <boost/container_hash/hash.hpp>
#include <boost/container/small_vector.hpp>

#include <limits>
#include <unordered_set>
#include <span>
#include <string>

namespace boost::prometheus::detail {

// TODO: Add overload for non_atomic_policy with non concurrent map
template <typename DerivedType, typename ItemType, typename OptionsType>
class basic_metric_vec
{
    static constexpr std::size_t map_initial_size = 50;
    // Number of preallocated labels in small_vector
    static constexpr std::size_t key_static_size = 4;

public:

    using item_type = ItemType;
    using key_type = boost::container::small_vector<std::string, key_static_size>;

protected:

    struct hasher
    {
        using is_transparent = void;
        // Since Boost 1.81.0 boost::hash passes SMHasher (only in 64-bit).
        using is_avalanching = std::bool_constant<(std::numeric_limits<std::size_t>::digits == 64)>;

        template <typename SpanType>
        std::size_t operator()(const SpanType& s) const noexcept
        {
            return boost::hash_range(s.begin(), s.end());
        }
    };

    struct equal
    {
        using is_transparent = void;

        bool operator()(const key_type& a, const key_type& b) const noexcept
        {
            return a == b;
        }

        template <typename SpanType1, typename SpanType2>
        bool operator()(const SpanType1& a, const SpanType2& b) const noexcept
        {
            return std::equal(a.begin(), a.end(), b.begin(), b.end());
        }
    };

    boost::concurrent_node_map<key_type, item_type, hasher, equal> map_;

    // Sorted labels and original index for serialization.
    boost::container::small_vector<std::pair<std::string, std::size_t>, key_static_size> labels_;
    // Contains both format prefixes.
    std::string both_format_prefixes_;
    // Headers point to header part of prefix string.
    std::string_view prometheus_header_sv_;
    std::string_view openmetrics_header_sv_;
    // Metric name points to name part of prefix string,
    // instead of creating another string for name.
    std::string_view metric_name_sv_;

    template <typename SpanType>
    void init(std::string_view name, OptionsType&& options, SpanType&& labels)
    {
        BOOST_ASSERT_MSG(name.size() > 0, "Metric name cannot be empty");
        BOOST_ASSERT_MSG(std::unordered_set<std::string_view>(labels.begin(), labels.end()).size() == labels.size(),
            "Detected duplicate labels");
        BOOST_ASSERT_MSG(labels.size() > 0, "Labels list cannot be empty");

        map_.reserve(map_initial_size);
        labels_.reserve(labels.size());
        for (std::size_t i = 0; i < labels.size(); i++)
        {
            if constexpr (std::is_lvalue_reference_v<SpanType>)
                labels_.emplace_back(labels[i], i);
            else
                labels_.emplace_back(std::move(labels[i]), i);
        }
        // Sort labels
        std::sort(labels_.begin(), labels_.end());
        // Format prefixes
        auto prefixes = serialize_prefix<
            item_type::internal_type::prometheus_type,
            item_type::internal_type::openmetrics_type,
            true, false, true
        >(name, options);
        both_format_prefixes_ = std::move(prefixes.base_string);
        prometheus_header_sv_ = std::string_view(both_format_prefixes_).substr(
            0,
            prefixes.metric_name_offset
        );
        if (prefixes.openmetrics_prefix_offset != 0)
        {
            openmetrics_header_sv_ = std::string_view(both_format_prefixes_).substr(
                prefixes.openmetrics_prefix_offset,
                both_format_prefixes_.size() - prefixes.openmetrics_prefix_offset);
        }
        else
        {
            openmetrics_header_sv_ = prometheus_header_sv_;
        }
        metric_name_sv_ = std::string_view(both_format_prefixes_).substr(
            prefixes.metric_name_offset,
            name.size());
        BOOST_ASSERT(metric_name_sv_ == name);
    }

    template <typename SpanType>
    item_type& with_labels_impl(SpanType&& labelvalues)
    {
        BOOST_ASSERT_MSG(labels_.size() == labelvalues.size(),
            "Incorrect label values count");

        item_type* result;
        auto get_result = [&result](auto& element)
        {
            result = &element.second;
        };
        // visit (no copy)
        bool found = map_.visit(labelvalues, get_result);
        if (found)
        {
            return *result;
        }

        key_type new_key;
        if constexpr (std::is_same_v<std::remove_cvref_t<SpanType>, key_type>)
        {
            static_assert(!std::is_lvalue_reference_v<SpanType>);
            new_key = std::move(labelvalues);
        }
        else if constexpr (std::is_lvalue_reference_v<SpanType>)
            new_key.assign(labelvalues.begin(), labelvalues.end());
        else
            // We can't move a container (such as std::vector) into
            // boost::small_vector directly, thus we just move all its elements.
            new_key.assign(
                std::make_move_iterator(labelvalues.begin()),
                std::make_move_iterator(labelvalues.end())
            );
        auto params = static_cast<DerivedType*>(this)->init_params(metric_name_sv_, labels_, new_key);

        map_.emplace_and_visit(
            std::move(new_key),
            std::move(params),
            get_result,
            get_result
        );

        return *result;
    }

public:

    basic_metric_vec() = delete;

    basic_metric_vec(std::string_view name, OptionsType&& options, std::span<const std::string_view> labels)
    {
        init(name, std::move(options), labels);
    }

    basic_metric_vec(std::string_view name, OptionsType&& options, std::span<const std::string> labels)
    {
        init(name, std::move(options), labels);
    }

    basic_metric_vec(std::string_view name, OptionsType&& options, std::vector<std::string>&& labels)
    {
        init(name, std::move(options), std::move(labels));
    }

    basic_metric_vec(std::string_view name, OptionsType&& options, std::initializer_list<std::string_view> labels)
    : basic_metric_vec(name, std::move(options), std::span(labels)) {}

    basic_metric_vec(std::string_view name, std::span<const std::string_view> labels)
    {
        init(name, {}, labels);
    }

    basic_metric_vec(std::string_view name, std::span<const std::string> labels)
    {
        init(name, {}, labels);
    }

    basic_metric_vec(std::string_view name, std::vector<std::string>&& labels)
    {
        init(name, {}, std::move(labels));
    }

    basic_metric_vec(std::string_view name, std::initializer_list<std::string_view> labels)
    : basic_metric_vec(name, std::span(labels)) {}

    consteval static bool is_atomic()
    {
        return item_type::is_atomic();
    }

    std::size_t size() const noexcept
    {
        return map_.size();
    }

    item_type& with_labels(std::span<const std::string_view> labelvalues)
    {
        return with_labels_impl(labelvalues);
    }

    item_type& with_labels(std::span<const std::string> labelvalues)
    {
        return with_labels_impl(labelvalues);
    }

    item_type& with_labels(std::vector<std::string>&& labelvalues)
    {
        return with_labels_impl(std::move(labelvalues));
    }

    item_type& with_labels(key_type&& labelvalues)
    {
        return with_labels_impl(std::move(labelvalues));
    }

    item_type& with_labels(std::initializer_list<std::string_view> labelvalues)
    {
        return with_labels(std::span(labelvalues));
    }

    // Used in serialization
    template <typename Func>
    void for_each(Func&& func)
    {
        map_.visit_all(func);
    }
};

} // namespace boost::prometheus::detail

#endif
