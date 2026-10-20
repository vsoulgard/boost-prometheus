//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_BENCH_HISTOGRAM_OBSERVE_BENCHMARK_HPP
#define BOOST_PROMETHEUS_BENCH_HISTOGRAM_OBSERVE_BENCHMARK_HPP

#include <boost/config.hpp>
#include <boost/version.hpp>

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <string>
#include <atomic>

inline void print_csv_header(std::size_t iterations_per_thread)
{
    std::cout << "# Compilier: " << BOOST_COMPILER
              << "\n# Boost Version: "
              << (BOOST_VERSION / 100000) << "."
              << (BOOST_VERSION / 100 % 1000) << "."
              << (BOOST_VERSION % 100)
              << "\n# iterations_per_thread=" << iterations_per_thread
              << "\nthreads,ops_per_sec,time_per_op_ns\n";
}

template <typename ObserveFunc>
void run_benchmark_impl(
    std::size_t iterations_per_thread,
    std::size_t num_threads,
    ObserveFunc&& observe_func)
{
    std::vector<std::vector<double>> thread_values(num_threads);
    for (std::size_t i = 0; i < num_threads; i++)
    {
        std::mt19937 gen(42 + i);
        std::uniform_real_distribution<> dis(0.0, 11.0);
        thread_values[i].reserve(iterations_per_thread);
        for (std::size_t j = 0; j < iterations_per_thread; j++)
        {
            thread_values[i].push_back(dis(gen));
        }
    }

    std::atomic<std::size_t> ready_count{0};
    std::atomic<bool> start_flag{false};

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (std::size_t i = 0; i < num_threads; i++)
    {
        threads.emplace_back([&, i]()
        {
            ready_count.fetch_add(1, std::memory_order_release);
            
            while (!start_flag.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }

            for (std::size_t j = 0; j < iterations_per_thread; j++)
            {
                observe_func(thread_values[i][j]);
            }
        });
    }

    while (ready_count.load(std::memory_order_acquire) < num_threads)
    {
        std::this_thread::yield();
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    start_flag.store(true, std::memory_order_release);

    for (auto& t : threads)
    {
        t.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::nano> elapsed = end_time - start_time;

    std::size_t total_ops = num_threads * iterations_per_thread;
    double ops_per_sec = (total_ops / elapsed.count()) * 1e9;
    double time_per_op_ns = elapsed.count() / total_ops;

    std::cout << num_threads << ","
              << std::fixed << std::setprecision(2) << ops_per_sec << ","
              << std::fixed << std::setprecision(2) << time_per_op_ns << "\n";
}

template <typename ObserveFunc>
void run_benchmark(
    std::size_t iterations_per_thread,
    const std::vector<std::size_t>& thread_counts,
    ObserveFunc&& observe_func)
{
    for (std::size_t num_threads : thread_counts)
    {
        run_benchmark_impl(iterations_per_thread, num_threads, std::forward<ObserveFunc>(observe_func));
    }
}

#endif
