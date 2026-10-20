//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/counter_vec.hpp>

#include "metric_vec_benchmark.hpp"

int main()
{
    constexpr std::size_t iterations_per_thread = 1000000;
    const std::vector<std::size_t> thread_counts = {1, 2, 4, 8, 12, 16};
    constexpr std::size_t pool_size = 20;
    constexpr std::size_t indices_size = 2 << 16;

    print_csv_header(iterations_per_thread, pool_size, indices_size);
    {
        constexpr std::size_t num_labels = 2;
        std::vector<std::string> label_names;
        for (std::size_t i = 0; i < num_labels; i++)
        {
            label_names.push_back("label_" + std::to_string(i));
        }

        boost::prometheus::counter_vec metric_vec("benchmark_counter", label_names);

        run_benchmark<num_labels>(
            iterations_per_thread,
            thread_counts,
            pool_size,
            indices_size,
            [&](const auto& label_values) {
                metric_vec.with_labels(label_values);
            }
        );
    }
    {
        constexpr std::size_t num_labels = 4;
        std::vector<std::string> label_names;
        for (std::size_t i = 0; i < num_labels; i++)
        {
            label_names.push_back("label_" + std::to_string(i));
        }

        boost::prometheus::counter_vec metric_vec("benchmark_counter", label_names);

        run_benchmark<num_labels>(
            iterations_per_thread,
            thread_counts,
            pool_size,
            indices_size,
            [&](const auto& label_values) {
                (void)metric_vec.with_labels(label_values);
            }
        );
    }

    return 0;
}
