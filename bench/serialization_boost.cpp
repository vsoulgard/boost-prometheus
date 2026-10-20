//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/registry.hpp>
#include <boost/prometheus/serialize.hpp>

#include <boost/config.hpp>
#include <boost/version.hpp>

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <random>
#include <list>
#include <vector>

using namespace boost::prometheus;

inline void print_csv_header(std::size_t iterations)
{
    std::cout << "# Compilier: " << BOOST_COMPILER
              << "\n# Boost Version: "
              << (BOOST_VERSION / 100000) << "."
              << (BOOST_VERSION / 100 % 1000) << "."
              << (BOOST_VERSION % 100)
              << "\n# iterations=" << iterations
              << "\nmetric_count,ops_per_sec,time_per_op_ns\n";
}

void run_benchmark(
    std::size_t iterations,
    const std::vector<std::size_t>& metric_counts)
{
    std::vector<std::string> metric_names;
    std::size_t names_count = *std::max_element(metric_counts.begin(), metric_counts.end());
    metric_names.resize(names_count);

    for (std::size_t i = 0; i < names_count; i++)
    {
        metric_names[i] = "metric_" + std::to_string(i);
    }

    for (std::size_t metric_count : metric_counts)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        for (std::size_t i = 0; i < iterations; i++)
        {
            // A little overhead because of metric creation, but it's 
            // the only way to measure pre-serialization upon metric creation.
            std::list<counter<>> counters;
            std::list<gauge<>> gauges;
            std::list<histogram<>> histograms;
            std::list<counter_vec<>> counter_vecs;
            std::list<gauge_vec<>> gauge_vecs;
            std::list<histogram_vec<>> histogram_vecs;
            registry reg;
            for (std::size_t j = 0; j < metric_count;)
            {
                // Add all different types
                counters.emplace_back(metric_names[j], metric_opts{.help = metric_names[j++]});
                gauges.emplace_back(metric_names[j], metric_opts{.help = metric_names[j++]});
                histograms.emplace_back(metric_names[j], histogram_opts{.help = metric_names[j++]});
                counter_vecs.emplace_back(metric_names[j],
                    metric_opts{.help = metric_names[j]}, std::span<const std::string>{{metric_names[j]}});
                counter_vecs.back().with_labels({metric_names[j++]});
                gauge_vecs.emplace_back(metric_names[j],
                    metric_opts{.help = metric_names[j]}, std::span<const std::string>{{metric_names[j]}});
                gauge_vecs.back().with_labels({metric_names[j++]});
                histogram_vecs.emplace_back(metric_names[j],
                    histogram_opts{.help = metric_names[j]}, std::span<const std::string>{{metric_names[j]}});
                histogram_vecs.back().with_labels({metric_names[j++]});
                reg.register_metric(counters.back());
                reg.register_metric(gauges.back());
                reg.register_metric(histograms.back());
                reg.register_metric(counter_vecs.back());
                reg.register_metric(gauge_vecs.back());
                reg.register_metric(histogram_vecs.back());
            }
            std::string result;
            serialize<format::prometheus_0_0_4>(reg, result);
            (void)result;
        }
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::nano> elapsed = end_time - start_time;

        std::size_t total_ops = iterations;
        double ops_per_sec = (total_ops / elapsed.count()) * 1e9;
        double time_per_op_ns = elapsed.count() / total_ops;

        std::cout << metric_count << ","
                << std::fixed << std::setprecision(2) << ops_per_sec << ","
                << std::fixed << std::setprecision(2) << time_per_op_ns << "\n";
    }
}

int main()
{
    const std::size_t iterations = 10000;
    const std::vector<std::size_t> metric_counts = {60, 120, 600, 1200};

    print_csv_header(iterations);
    run_benchmark(iterations, metric_counts);

    return 0;
}
