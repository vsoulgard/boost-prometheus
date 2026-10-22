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

// Build constexpr string
consteval auto boost_version_str()
{
    using prometheus::detail::to_string_buffer;
    using prometheus::detail::string_buffer;
    return 
        to_string_buffer<static_cast<double>(BOOST_VERSION / 100000)>() +
        string_buffer(".") + to_string_buffer<static_cast<double>(BOOST_VERSION / 100 % 1000)>() +
        string_buffer(".") + to_string_buffer<static_cast<double>(BOOST_VERSION % 100)>();
}

int main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 3 && argc != 4)
    {
        std::cerr
            << "Usage: with_static_registry <address> <port> [exposer_route]\n"
            << "  exposer_route defaults to /metrics\n"
            << "Example:\n"
            << "    with_static_registry 0.0.0.0 8080\n"
            << "    with_static_registry 0.0.0.0 8080 /metrics\n";
        return EXIT_FAILURE;
    }
    auto address = asio::ip::make_address(argv[1]);
    auto port = static_cast<unsigned short>(std::atoi(argv[2]));
    std::string route = "/metrics";
    if (argc == 4)
        route = std::string(argv[3]);

    asio::io_context ioc;

    // Default static registry
    using static_reg = prometheus::static_registry<>;
    using build_info = prometheus::static_info<"build",
        prometheus::labels<"build_type", "compiler", "boost_version">>;
    build_info::create<
        "example_with_static_registry", BOOST_COMPILER, boost_version_str()>();
    using requests = prometheus::static_counter<
        "requests", prometheus::help<"Number of received requests">,
        prometheus::labels<"method">>;
    requests::inc<"GET">(5.0);
    requests::inc<"POST">(2.0);
    using req_duration = prometheus::static_histogram<
        "request_duration_seconds", prometheus::unit<"seconds">>; // Default buckets
    req_duration::observe(0.1);

    static_reg::register_metric<build_info>();
    static_reg::register_metric<requests>();
    static_reg::register_metric<req_duration>();

    prometheus::registry reg;
    prometheus::gaugehistogram temperature(
        "cpu_temperature_celsius",
        {.help = "", .unit = "celsius", .buckets = {25.0, 50.0, 75.0, 90.0}});
    temperature.observe(33.0);

    reg.register_metric(temperature);

    // Expose static registry via expose_static_registry<> policy
    prometheus::exposer<
        decltype(ioc.get_executor()), prometheus::expose_static_registry<static_reg>
    > exp(
        ioc.get_executor(),
        asio::ip::tcp::endpoint(address, port),
        std::move(route));
    exp.expose(reg);

    ioc.run();

    return EXIT_SUCCESS;
}
