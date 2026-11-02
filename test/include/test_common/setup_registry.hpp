//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_TEST_SETUP_REGISTRY_HPP
#define BOOST_PROMETHEUS_TEST_SETUP_REGISTRY_HPP

#include <boost/prometheus/registry.hpp>
#include <boost/prometheus/static_counter.hpp>
#include <boost/prometheus/static_gauge.hpp>
#include <boost/prometheus/static_histogram.hpp>
#include <boost/prometheus/static_registry.hpp>

#include <memory>

namespace boost::prometheus::test {

using premade_static_registry = static_registry<"integration_tests_premade">;
using static_c = static_counter<
    "requests", help<"Number of HTTP requests">, labels<"method">>;
using static_g = static_gauge<
    "applications_active", help<"Current number of applications">>;
using static_h = static_histogram<
    "request_duration_seconds", help<"Duration of HTTP requests in seconds">, unit<"seconds">>;

inline void setup_static_registry()
{
    static bool init = [&](){
        static_c::inc<"POST">(15.0);
        static_g::set(50.0);
        static_h::observe(0.1);
        premade_static_registry::register_metric<static_c>();
        premade_static_registry::register_metric<static_g>();
        premade_static_registry::register_metric<static_h>();
        return true;
    }();
    (void)init;
}

// Premade registry is big enough to trigger gzip compression.
inline std::unique_ptr<registry<>> setup_registry()
{
    std::unique_ptr<registry<>> reg = std::make_unique<registry<>>();
    static counter_vec c(
        "requests",
        {.help = "Number of HTTP requests", .unit = ""},
        {"method"});
    static gauge g("applications_active",
        {.help = "Current number of applications", .unit = ""});
    c.with_labels({"GET"}).inc(5.0);
    g.set(100.0);
    static histogram h(
        "request_duration_seconds",
        {.help = "Duration of HTTP requests in seconds", .unit = "seconds", .buckets = {}});
    h.observe(0.5);
    static gaugehistogram gh(
        "cpu_temperature_celsius",
        {.help = "CPU core temperature", .unit = "celsius", .buckets = {25.0, 50.0, 75.0, 90.0}});
    gh.observe(33.0);

    reg->register_metric(c);
    reg->register_metric(g);
    reg->register_metric(h);
    reg->register_metric(gh);
    return reg;
}

inline registry<>& get_registry()
{
    static std::unique_ptr<registry<>> reg = setup_registry();
    return *reg;
}

} // namespace boost::prometheus::test

#endif
