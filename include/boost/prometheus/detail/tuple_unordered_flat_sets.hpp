//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_TUPLE_UNORDERED_FLAT_SETS_HPP
#define BOOST_PROMETHEUS_DETAIL_TUPLE_UNORDERED_FLAT_SETS_HPP

#include <boost/prometheus/detail/config.hpp>

#include <boost/unordered/unordered_flat_set.hpp>

#include <mutex>
#include <tuple>
#include <type_traits>

namespace boost::prometheus::detail {

template <typename T, typename... Ts>
struct types_element_index
{
    static constexpr std::size_t value = []() consteval {
        constexpr bool matches[] = { std::is_same_v<T, Ts>... };
        for (std::size_t i = 0; i < sizeof...(Ts); i++)
        {
            if (matches[i]) return i;
        }
        return static_cast<std::size_t>(-1);
    }();

    static_assert(value != static_cast<std::size_t>(-1), 
        "Type T is not present in the template parameter pack Ts...");
};

template <typename AtomicPolicyType, typename... Ts>
class tuple_unordered_flat_sets
{
    struct alignas(cache_line_size) aligned_mutex
    {
        std::mutex mtx;
    };

    using mutex_array_type = std::conditional_t<
        AtomicPolicyType::enabled,
        std::array<aligned_mutex, sizeof...(Ts)>,
        std::tuple<>
    >;

    std::tuple<boost::unordered_flat_set<Ts>...> sets_;
    [[no_unique_address]]
    mutable mutex_array_type mutexes_;

public:

    template <typename T>
    void insert(T&& value)
    {   
        using clean_type = std::decay_t<T>;

        if constexpr (AtomicPolicyType::enabled)
        {
            std::lock_guard lock(mutexes_[types_element_index<clean_type, Ts...>::value].mtx);
            std::get<boost::unordered_flat_set<clean_type>>(sets_).insert(std::forward<T>(value));
        }
        else
        {
            std::get<boost::unordered_flat_set<clean_type>>(sets_).insert(std::forward<T>(value));
        }
    }

    template <typename T>
    bool erase(T&& value)
    {
        using clean_type = std::decay_t<T>;

        if constexpr (AtomicPolicyType::enabled)
        {
            std::lock_guard lock(mutexes_[types_element_index<clean_type, Ts...>::value].mtx);
            return std::get<boost::unordered_flat_set<clean_type>>(sets_).erase(std::forward<T>(value));
        }
        else
        {
            return std::get<boost::unordered_flat_set<clean_type>>(sets_).erase(std::forward<T>(value));
        }
    }

    template <typename T>
    std::size_t size() const noexcept
    {
        return std::get<boost::unordered_flat_set<T>>(sets_).size();
    }

    template <typename Func>
    void for_each_set(Func&& func)
    {
        std::apply([&](auto&... sets)
        {
            auto process_one = [&](auto& set)
            {
                if constexpr (AtomicPolicyType::enabled)
                {   
                    using set_type = std::remove_cvref_t<decltype(set)>;
                    std::lock_guard lock(mutexes_[
                        types_element_index<
                        typename set_type::key_type,
                        Ts...>::value].mtx);

                    func(set);
                }
                else
                {
                    func(set);
                }
            };

            (process_one(sets), ...);
        }, sets_);
    }

    template <typename Func>
    void for_each(Func&& func)
    {
        for_each_set([&](auto& set)
        {
            for (auto& item : set)
            {
                func(item);
            }
        });
    }

    void clear()
    {
        for_each_set([&](auto& set)
        {
            set.clear();
        });
    }
};

} // namespace boost::prometheus::detail

#endif
