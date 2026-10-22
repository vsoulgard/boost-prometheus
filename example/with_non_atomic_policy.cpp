//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus.hpp>

#include <boost/config.hpp>
#include <boost/version.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

using namespace boost;

std::string boost_version_str()
{
    return std::to_string(BOOST_VERSION / 100000) + "." +
        std::to_string(BOOST_VERSION / 100 % 1000) + "." +
        std::to_string(BOOST_VERSION % 100);
}

int main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 3 && argc != 4)
    {
        std::cerr
            << "Usage: with_non_atomic_policy <address> <port> [exposer_route]\n"
            << "  exposer_route defaults to /metrics\n"
            << "Example:\n"
            << "    with_non_atomic_policy 0.0.0.0 8080\n"
            << "    with_non_atomic_policy 0.0.0.0 8080 /metrics\n";
        return EXIT_FAILURE;
    }
    auto address = asio::ip::make_address(argv[1]);
    auto port = static_cast<unsigned short>(std::atoi(argv[2]));
    std::string route = "/metrics";
    if (argc == 4)
        route = std::string(argv[3]);

    asio::io_context ioc;
    // Disable thread-safety for all objects for extra optimization.
    using non_atomic_policy = prometheus::atomic_policy<false>;
    prometheus::registry<non_atomic_policy> reg;
    prometheus::info_vec<non_atomic_policy> build_info(
        "build",
        {"build_type", "compiler", "boost_version"});
    build_info.with_labels(
        {"example_with_non_atomic_policy",
        BOOST_COMPILER,
        boost_version_str()});
    prometheus::counter_vec<non_atomic_policy> requests(
        "requests",
        {.help = "Number of received requests", .unit = ""},
        {"method"});
    requests.with_labels({"GET"}).inc(5.0);
    requests.with_labels({"POST"}).inc(2.0);
    prometheus::histogram<non_atomic_policy> req_duration(
        "request_duration_seconds",
        {.help = "", .unit = "seconds", .buckets = {}}); // Default buckets

    reg.register_metric(build_info);
    reg.register_metric(requests);
    reg.register_metric(req_duration);

    prometheus::exposer<
        decltype(ioc.get_executor()), non_atomic_policy
    > exp(
        ioc.get_executor(),
        asio::ip::tcp::endpoint(address, port),
        std::move(route));
    exp.expose(reg);

    ioc.run();

    return EXIT_SUCCESS;
}
