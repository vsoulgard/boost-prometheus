//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/histogram_vec.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/test/unit_test.hpp>

#include <thread>

using namespace boost::prometheus;

BOOST_AUTO_TEST_SUITE(test_histogram_vec)

BOOST_AUTO_TEST_CASE(default_constructor_is_deleted)
{
    static_assert(!std::is_default_constructible_v<histogram_vec<>>);
    static_assert(!std::is_default_constructible_v<histogram_vec<atomic_policy<false>>>);
}

BOOST_AUTO_TEST_CASE(construction_default_options)
{
    histogram_vec h("test_histogram", {"first", "second"});
}

BOOST_AUTO_TEST_CASE(construction_custom_options)
{
    histogram_vec h("request_duration_seconds", {.help = "help string", .unit = "seconds", .buckets = {}}, {"first", "second"});
}

BOOST_AUTO_TEST_CASE(concurrent_update)
{
    histogram_vec h("request_duration_seconds", {"method"});

    constexpr std::size_t num_threads = 4u;
    constexpr std::size_t iterations_per_thread = 1000u;

    std::atomic<std::size_t> ready_count{0u};
    std::atomic<bool> start_flag{false};

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (std::size_t i = 0u; i < num_threads; i++)
    {
        threads.emplace_back([&]()
        {
            ready_count.fetch_add(1u, std::memory_order_release);

            while (!start_flag.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }

            for (std::size_t j = 0u; j < iterations_per_thread; j++)
            {
                h.with_labels({"POST"}).observe(0.1);
            }
        });
    }

    while (ready_count.load(std::memory_order_acquire) < num_threads)
    {
        std::this_thread::yield();
    }

    start_flag.store(true, std::memory_order_release);
    for (auto& t : threads)
    {
        t.join();
    }
    BOOST_TEST(h.size() == 1u);
}

BOOST_AUTO_TEST_SUITE_END()
