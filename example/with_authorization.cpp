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

inline static std::string token;
// We use this function as token validator.
// There might be any custom validation logic.
bool validate_func(std::string_view auth)
{
    return token == auth;
}

int main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 4 && argc != 5)
    {
        std::cerr
            << "Usage: with_authorization <address> <port> [exposer_route] <token>\n"
            << "  exposer_route defaults to /metrics\n"
            << "  With 3 arguments:  address port token\n"
            << "  With 4 arguments:  address port route token\n"
            << "Example:\n"
            << "    with_authorization 0.0.0.0 8080 \"Bearer secret-key\"\n"
            << "    with_authorization 0.0.0.0 8080 /metrics \"Bearer secret-key\"\n";
        return EXIT_FAILURE;
    }
    auto address = asio::ip::make_address(argv[1]);
    auto port = static_cast<unsigned short>(std::atoi(argv[2]));
    std::string route = "/metrics";
    if (argc == 4)
        token = std::string(argv[3]);
    else if (argc == 5)
    {
        route = std::string(argv[3]);
        token = std::string(argv[4]);
    }

    asio::io_context ioc;

    prometheus::registry reg;
    prometheus::info_vec build_info(
        "build",
        {"build_type", "compiler", "boost_version"});
    build_info.with_labels(
        {"example_with_authorization",
        BOOST_COMPILER,
        boost_version_str()});
    prometheus::counter_vec requests(
        "requests",
        {.help = "Number of received requests", .unit = ""},
        {"method"});
    requests.with_labels({"GET"}).inc(5.0);
    requests.with_labels({"POST"}).inc(2.0);
    prometheus::histogram req_duration(
        "request_duration_seconds",
        {.help = "", .unit = "seconds", .buckets = {}}); // Default buckets

    reg.register_metric(build_info);
    reg.register_metric(requests);
    reg.register_metric(req_duration);

    prometheus::exposer<
        decltype(ioc.get_executor()),
        prometheus::authorization_policy<"Bearer", validate_func>
    > exp(
        ioc.get_executor(),
        asio::ip::tcp::endpoint(address, port),
        std::move(route));
    exp.expose(reg);

    ioc.run();

    return EXIT_SUCCESS;
}
