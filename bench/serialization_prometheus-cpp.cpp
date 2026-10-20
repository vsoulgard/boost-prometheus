//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>
#include <prometheus/text_serializer.h>

#include <boost/config.hpp>
#include <boost/version.hpp>

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <list>
#include <vector>

using namespace prometheus;

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

    const Histogram::BucketBoundaries buckets{
        0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0};
    for (std::size_t metric_count : metric_counts)
    {
        prometheus::TextSerializer serializer;
        auto start_time = std::chrono::high_resolution_clock::now();
        for (std::size_t i = 0; i < iterations; i++)
        {
            auto registry = std::make_shared<Registry>();
            for (std::size_t j = 0; j < metric_count;)
            {
                // Add all different types
                auto& c = BuildCounter()
                    .Name(metric_names[j])
                    .Help(metric_names[j++])
                    .Register(*registry);
                c.Add({});
                auto& g = BuildGauge()
                    .Name(metric_names[j])
                    .Help(metric_names[j++])
                    .Register(*registry);
                g.Add({});
                auto& h = BuildHistogram()
                    .Name(metric_names[j])
                    .Help(metric_names[j++])
                    .Register(*registry);
                h.Add({}, buckets);
                c.Add({{metric_names[j], metric_names[j++]}});
                g.Add({{metric_names[j], metric_names[j++]}});
                h.Add({{metric_names[j], metric_names[j++]}}, buckets);
            }
            std::ostringstream output;
            serializer.Serialize(output, registry->Collect());
            (void)output;
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
