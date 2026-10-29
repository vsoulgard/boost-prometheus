//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus.hpp>

int main()
{
    namespace prometheus = boost::prometheus;
    namespace asio = boost::asio;

    asio::io_context ioc;
    // Start exposer on http://localhost:8080/metrics.
    prometheus::exposer exp(ioc.get_executor(),
        asio::ip::tcp::endpoint(asio::ip::address_v4::loopback(), 8080));

    // Create registry and some metrics.
    prometheus::registry reg;
    prometheus::info build_info(
        "build",
        {.help = "Build information"},
        {"build_type", "version"},
        {"Debug", "1.0"});
    prometheus::counter_vec requests("requests",
        {.help = "Number of received requests", .unit = ""},
        {"method"});
    prometheus::histogram req_duration(
        "request_duration_seconds",
        {.help = "Duration of HTTP requests in seconds",
            .unit = "seconds", .buckets = {}}); // Default buckets

    requests.with_labels({"GET"}).inc(5.0);
    // Caching is also supported for metric vectors.
    auto& post_requests = requests.with_labels({"POST"});
    post_requests.inc(2.0);
    req_duration.observe(0.5);
    // Add metrics to the registry.
    reg.register_metric(build_info);
    reg.register_metric(requests);
    reg.register_metric(req_duration);
    // Expose the registry (can only expose one registry at a time).
    exp.expose(reg);
    // Run the io_context to start exposition.
    ioc.run();
}
