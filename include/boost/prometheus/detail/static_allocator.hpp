//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_DETAIL_STATIC_ALLOCATOR_HPP
#define BOOST_PROMETHEUS_DETAIL_STATIC_ALLOCATOR_HPP

#include <boost/prometheus/detail/string_buffer.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

namespace boost::prometheus::detail {

template <std::size_t Capacity>
struct allocator_buffer
{
    alignas(std::max_align_t) std::byte buffer[Capacity];
    std::size_t offset = 0;

    void reset() noexcept
    {
        offset = 0;
    }
};

// Used in `exposer_session` for request headers parsing
template <typename T, std::size_t Capacity>
class static_allocator
{
public:

    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using is_always_equal = std::false_type;

    allocator_buffer<Capacity>& pool;

    explicit static_allocator(allocator_buffer<Capacity>& p) noexcept : pool(p) {}

    template <typename U>
    static_allocator(const static_allocator<U, Capacity>& other) noexcept : pool(other.pool) {}

    T* allocate(std::size_t n)
    {
        std::size_t bytes_needed = n * sizeof(T);

        std::size_t current_alignment = alignof(T);
        std::uintptr_t ptr_val = reinterpret_cast<std::uintptr_t>(pool.buffer + pool.offset);
        std::size_t padding = (current_alignment - (ptr_val % current_alignment)) % current_alignment;

        if (pool.offset + padding + bytes_needed > Capacity) 
        {
            throw std::bad_alloc();
        }

        pool.offset += padding;
        void* result = pool.buffer + pool.offset;
        pool.offset += bytes_needed;

        return static_cast<T*>(result);
    }

    void deallocate(T*, std::size_t) noexcept {}

    template <typename U>
    struct rebind
    {
        using other = static_allocator<U, Capacity>;
    };

    template <typename U>
    bool operator==(const static_allocator<U, Capacity>& other) const noexcept 
    { 
        return &pool == &other.pool; 
    }
};

} // namespace boost::prometheus::detail

#endif
