//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_TUPLE_VECTORS_HPP
#define BOOST_PROMETHEUS_DETAIL_TUPLE_VECTORS_HPP

namespace boost::prometheus::detail {

#include <tuple>
#include <vector>

// Tuple of vectors
template <typename... Ts>
class tuple_vectors
{
    std::tuple<std::vector<Ts>...> vectors_;

public:

    template <typename T>
    void push_back(T&& value)
    {
        using clean_type = std::decay_t<T>;
        std::get<std::vector<clean_type>>(vectors_).push_back(std::forward<T>(value));
    }

    std::size_t size() const noexcept
    {
        std::size_t result = 0;
        std::apply([&](auto&... vecs)
        {
            result = (vecs.size() + ...);
        }, vectors_);
        return result;
    }

    template <typename Func>
    void for_each_vector(Func&& func)
    {
        std::apply([&](auto&... vecs)
        {
            (func(vecs), ...);
        }, vectors_);
    }

    template <typename Func>
    void for_each(Func&& func)
    {
        for_each_vector([&](auto& vec)
        {
            for (auto& item : vec)
            {
                func(item);
            }
        });
    }

    void clear() noexcept
    {
        std::apply([](auto&... vecs)
        {
            (vecs.clear(), ...);
        }, vectors_);
    }
};

} // namespace boost::prometheus::detail

#endif
