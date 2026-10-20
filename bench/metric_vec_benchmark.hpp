//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_BENCH_METRIC_VEC_BENCHMARK_HPP
#define BOOST_PROMETHEUS_BENCH_METRIC_VEC_BENCHMARK_HPP

#include <boost/config.hpp>
#include <boost/version.hpp>

#include <iostream>
#include <iomanip>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <string>
#include <array>
#include <string_view>
#include <span>
#include <atomic>

inline void print_csv_header(
    std::size_t iterations_per_thread,
    std::size_t pool_size,
    std::size_t indices_size)
{
    std::cout << "# Compilier: " << BOOST_COMPILER
              << "\n# Boost Version: "
              << (BOOST_VERSION / 100000) << "."
              << (BOOST_VERSION / 100 % 1000) << "."
              << (BOOST_VERSION % 100)
              << "\n# iterations_per_thread=" << iterations_per_thread
              << ", pool_size=" << pool_size
              << ", indices_size=" << indices_size
              << "\nlabels,threads,ops_per_sec,time_per_op_ns\n";
}

template <std::size_t NumLabels, typename WithLabelsFunc>
void run_benchmark(
    std::size_t iterations_per_thread,
    const std::vector<std::size_t>& thread_counts,
    std::size_t pool_size,
    std::size_t indices_size,
    WithLabelsFunc&& with_labels_func)
{
    std::mt19937 gen(42);
    std::uniform_int_distribution<std::size_t> dist(0, pool_size - 1);
    
    std::vector<std::size_t> random_indices(indices_size);
    for (std::size_t i = 0; i < indices_size; i++)
    {
        random_indices[i] = dist(gen);
    }

    std::vector<std::array<std::string, NumLabels>> label_pool;
    label_pool.reserve(pool_size);

    for (std::size_t i = 0; i < pool_size; i++)
    {
        std::array<std::string, NumLabels> labels;
        
        for (std::size_t j = 0; j < NumLabels; j++)
        {
            labels[j] = "val_" + std::to_string(i) + "_" + std::to_string(j);
        }

        label_pool.push_back(std::move(labels));
    }

    for (std::size_t num_threads : thread_counts)
    {
        std::vector<std::thread> threads;
        std::atomic<std::size_t> ready_threads{0};
        std::atomic<bool> start_flag{false};

        for (std::size_t i = 0; i < num_threads; i++)
        {
            threads.emplace_back([&, i]()
            {
                ready_threads.fetch_add(1);
                while (!start_flag.load())
                {
                    std::this_thread::yield();
                }

                std::size_t idx_offset = i * iterations_per_thread;
                for (std::size_t j = 0; j < iterations_per_thread; j++)
                {
                    std::size_t idx = random_indices[(idx_offset + j) % random_indices.size()];
                    with_labels_func(label_pool[idx]);
                }
            });
        }

        while (ready_threads.load() < num_threads)
        {
            std::this_thread::yield();
        }

        auto start_time = std::chrono::high_resolution_clock::now();
        start_flag.store(true);

        for (auto& t : threads)
        {
            t.join();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::nano> elapsed = end_time - start_time;

        std::size_t total_ops = num_threads * iterations_per_thread;
        double ops_per_sec = (total_ops / elapsed.count()) * 1e9;
        double time_per_op_ns = elapsed.count() / total_ops;

        std::cout << NumLabels << "," << num_threads << ","
                  << std::fixed << std::setprecision(2) << ops_per_sec << ","
                  << std::fixed << std::setprecision(2) << time_per_op_ns << "\n";
    }
}

#endif
