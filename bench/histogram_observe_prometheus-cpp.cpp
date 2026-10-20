//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <prometheus/registry.h>
#include <prometheus/histogram.h>
#include <prometheus/family.h>

#include "histogram_observe_benchmark.hpp"

int main()
{
    const std::size_t iterations_per_thread = 1000000;
    const std::vector<std::size_t> thread_counts = {1, 2, 4, 8, 12, 16};

    print_csv_header(iterations_per_thread);

    prometheus::Registry registry;
    auto& family = prometheus::BuildHistogram()
        .Name("benchmark_histogram")
        .Register(registry);

    std::vector<double> boundaries;
    for (std::size_t i = 0; i <= 10; i++)
    {
        boundaries.push_back(static_cast<double>(i));
    }

    auto& bench_histogram = family.Add({}, boundaries);

    run_benchmark(
        iterations_per_thread,
        thread_counts,
        [&](double val) {
            bench_histogram.Observe(val);
        }
    );

    return 0;
}
