//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/detail/static_allocator.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::prometheus;
using namespace boost::prometheus::detail;

BOOST_AUTO_TEST_SUITE(test_static_allocator)

BOOST_AUTO_TEST_CASE(allocates_within_capacity)
{
    allocator_buffer<64> pool;
    static_allocator<int, 64> alloc(pool);

    int* p = alloc.allocate(4);
    BOOST_REQUIRE(p != nullptr);
    BOOST_TEST(pool.offset >= 16u);
}

BOOST_AUTO_TEST_CASE(throws_bad_alloc_when_capacity_exceeded)
{
    allocator_buffer<8> pool;
    static_allocator<int, 8> alloc(pool);

    BOOST_CHECK_THROW(alloc.allocate(3), std::bad_alloc);
}

BOOST_AUTO_TEST_CASE(throws_after_pool_is_exhausted)
{
    allocator_buffer<16> pool;
    static_allocator<int, 16> alloc(pool);

    BOOST_REQUIRE_NO_THROW(alloc.allocate(4));
    BOOST_CHECK_THROW(alloc.allocate(1), std::bad_alloc);
}

BOOST_AUTO_TEST_CASE(succeeds_at_exact_boundary)
{
    allocator_buffer<16> pool;
    static_allocator<int, 16> alloc(pool);

    BOOST_CHECK_NO_THROW(alloc.allocate(4));
}

BOOST_AUTO_TEST_CASE(deallocate_does_not_reclaim_space)
{
    allocator_buffer<16> pool;
    static_allocator<int, 16> alloc(pool);

    int* p = alloc.allocate(4);
    alloc.deallocate(p, 4);
    BOOST_CHECK_THROW(alloc.allocate(1), std::bad_alloc);
}

BOOST_AUTO_TEST_CASE(reset_restores_capacity)
{
    allocator_buffer<16> pool;
    static_allocator<int, 16> alloc(pool);

    BOOST_CHECK_THROW(alloc.allocate(5), std::bad_alloc);
    pool.reset();
    BOOST_CHECK_NO_THROW(alloc.allocate(4));
}

BOOST_AUTO_TEST_SUITE_END()
