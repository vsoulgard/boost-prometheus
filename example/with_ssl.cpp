//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus.hpp>
// To use SSL, this header need to be included
// and project need to be linked with OpenSSL::SSL.
#include <boost/prometheus/exposer_ssl_policy.hpp>

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

constexpr std::string_view certificate =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEizCCAnOgAwIBAgIUWaCffrczG0dFuW1wfNntQatgeJIwDQYJKoZIhvcNAQEL\n"
    "BQAwUTELMAkGA1UEBhMCVVMxDTALBgNVBAgMBFRlc3QxDTALBgNVBAcMBFRlc3Qx\n"
    "DTALBgNVBAoMBFRlc3QxFTATBgNVBAMMDFRlc3QgUm9vdCBDQTAeFw0yNjEwMTYw\n"
    "NDAyNTlaFw0zNjEwMTMwNDAyNTlaME4xCzAJBgNVBAYTAlVTMQ0wCwYDVQQIDARU\n"
    "ZXN0MQ0wCwYDVQQHDARUZXN0MQ0wCwYDVQQKDARUZXN0MRIwEAYDVQQDDAlsb2Nh\n"
    "bGhvc3QwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQD1fVbArCqPQTwT\n"
    "0C5y6ix3H7GRN0A+faGBqbS2KEdPnt+fWYauFsDVQtDcwdcAAwvIUPOB6pKLISFG\n"
    "gexizM7MP4uMDqEkEUmsDETog+Qe1/SavfomgRRs54dTRE0fph/r3fQPWlNUjLuO\n"
    "7Zy3Kk9qUjcTHV/CsRbH8AFFQ2BPZ6iWxDan4tbcXqGOMsC39quj9i/7YHE6n6Yu\n"
    "FnvJSyBEVkvBqU2JQrsjjk2lT2N1lVaPAlEs0kmTpj2szO7kS443DsQkxsm/cHBn\n"
    "qV0mzH/tVdtjzClttJ/FLkWqTaq1r+Ku2LgZHYE15BZiSXAgez9l6WZMOYlmw70x\n"
    "030saHlpAgMBAAGjXjBcMBoGA1UdEQQTMBGCCWxvY2FsaG9zdIcEfwAAATAdBgNV\n"
    "HQ4EFgQUo4tbSTZPjy81rskqJNjZL4pKiRwwHwYDVR0jBBgwFoAUEO5t67ZSBYZt\n"
    "ps8D9rAfkBxk3vowDQYJKoZIhvcNAQELBQADggIBAKkafQy+aV85zk8dNMTykaK/\n"
    "3Y/+LkaiE40EoOZ5a6TXUQLhwf8MqYdYevjtD4hiwsA29jZZGifHHvWaW0v7fW14\n"
    "d3sSFXYj8bsagULeFxs1J/Nb4pH/lef2TD/5mRW4XfYpfOzf1lgAW9Audhlfu9zR\n"
    "Gkpu6E2iQ+LntMY7U3ByM4fpN/wrlBaCkPCuM/gUgH2icF56/bbdtxEPhEONAKJd\n"
    "w7T5NAkfmvPlwBmugF6oh4hDOz8l1HMGZsYhR2Zb7l27Erc6Sherd3uF5KNlfC1s\n"
    "yLZoZKWXbZ4GB7rO9Pf5gYxV6USS9fQq9c+uOTNiZrFhJ3Mirr+rSRagQjHbRoQz\n"
    "XIaURjJFFLTttFStmDjpcpUNJb0Hl66AHWagzt0W/acLq4fOf7NFbleiRN4K0lcz\n"
    "DknSqnt4K5VqiaYQp9Hyl5Ffd7C0/tyGIV7r3L3mja95QGM8KLPLJhVlv/jbRjiF\n"
    "1UCeRXpIlFn2ombpLmdHgHJIYuqfwTzDYGj3pHO65u0YYpvkwRv1Bfeu/lxdsJt5\n"
    "DhMu+CHRFeX5ZSAcDJYfEjVWSWhtQ1moCuFzYDfHMTJHIIAfFD/slxytnSGhb4Ki\n"
    "HQcDmXvKYWSpyJGK2WY0qMi6cSseiN11gQXm98S/Eq5jxJE67ZIE0Il1xsrzLSXN\n"
    "deF2JBG3M5/zodjtraGm\n"
    "-----END CERTIFICATE-----\n";

constexpr std::string_view private_key =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQD1fVbArCqPQTwT\n"
    "0C5y6ix3H7GRN0A+faGBqbS2KEdPnt+fWYauFsDVQtDcwdcAAwvIUPOB6pKLISFG\n"
    "gexizM7MP4uMDqEkEUmsDETog+Qe1/SavfomgRRs54dTRE0fph/r3fQPWlNUjLuO\n"
    "7Zy3Kk9qUjcTHV/CsRbH8AFFQ2BPZ6iWxDan4tbcXqGOMsC39quj9i/7YHE6n6Yu\n"
    "FnvJSyBEVkvBqU2JQrsjjk2lT2N1lVaPAlEs0kmTpj2szO7kS443DsQkxsm/cHBn\n"
    "qV0mzH/tVdtjzClttJ/FLkWqTaq1r+Ku2LgZHYE15BZiSXAgez9l6WZMOYlmw70x\n"
    "030saHlpAgMBAAECggEABTSXhABilU+5zrsUW7rGye6HHQrXNuatEmB0hNmids1p\n"
    "9SljJ1/eI5IwzELj+eEWp98SOsgBkX0DWp5jUnkAoujEtGnpqtJ0WyHreCQL0Jzz\n"
    "AoZSvpbwGvBnOIcd54o7P7d2Ibk1mSO0JRoOv6eLBY3MEytaniM+gMDVwLqRWT2m\n"
    "fbQCwgJ+WqiobHA+OlqR45CcwEsM8c2eP7DZiy3urdaoRvXrwRx0DMUQqPAYmrzZ\n"
    "EwJiowKU/5b3XZ98bCbYiCyQ4yYteMTQEJcnfP6TWnEPZQ3V+a5mkxaS1pww9Vi0\n"
    "49RaNqu851yTDJMYK/opiqRoXXCE554wVd6Cc+hxgQKBgQD/XWCNO0fMR5dM5MHz\n"
    "wqGCmUEspo2tnjmSdQAz9c0jaEXWOO/Dx8U/AowlXOyURaG30HjEz/+1C8IMa+lQ\n"
    "cvceUAXLowmWECGMGOAMqdytVxub9/zGP/vdqIGsnDObfrGqXaSSVJ8+1+2wxY0J\n"
    "u3OoGizV2TrgNb7MpRPC32jg+QKBgQD2GaxH+EHQN781wdGTbyMhpfiP97UfcHOT\n"
    "oqfkOIocV213oX8CeS1xlmUQZTrrKFC+4r5MrGzkFLNxzhaECymyGBSA8O7DwJJ3\n"
    "+bMCebthKuAyIvDJYpWGlNFU+9vfpjLsgVzKRNcB33Cz4IBUop6NoTj4f5qwx9By\n"
    "SgeTDAzn8QKBgQD5M1Osn2QSeu6wkDChw5a/jvUmMKbj/htqzBjVnBKDchds1YVN\n"
    "hl77fW3sbEFG9Ki7a4rfbae5Mu88FfbKllaH00nEjqclNTq3r/K9COqckfwUea3I\n"
    "fdo5Vl6tZoWKQdn5Sob2cfQxbKC9OBvJmo9kRZ7pVaL5+BjpNlMRgdmkaQKBgQDB\n"
    "KQ9nAb0uX8MNv43uSYnqjCmXXlZkUc/1bfKXF6WXbyu1fSWCXHhFskSq5Lmk/2S5\n"
    "HqE87w0cy/64cYiMpXJZnbriJblSHeEYl/1gEQn4zz1fPLt/ZYh1NHNFX9WJKhrI\n"
    "NRUOkY+soaMFHE29oFIQWh2sNgPFg1/hhKeDTC8JEQKBgFDGJsJtc5Z0VELz5sYM\n"
    "gxOG+vQHphN4IbTf0EQqh1B3ZbmASSOE4tbChuyeearxsiVAnz3nOhagLZB8L/Rm\n"
    "AZ30042kOt9nATVUY/lyYbyZFw4XDbq/kFNOWKMrh1Rk+LcOtEjg8AbP/6Iv5t2P\n"
    "IblVbeXfs5v38D5oqXg7qnw8\n"
    "-----END PRIVATE KEY-----\n";

void load_server_certificate(boost::asio::ssl::context& ctx)
{
    ctx.set_options(
        boost::asio::ssl::context::default_workarounds |
        boost::asio::ssl::context::no_sslv2 |
        boost::asio::ssl::context::no_sslv3 |
        boost::asio::ssl::context::no_tlsv1 |
        boost::asio::ssl::context::no_tlsv1_1);

    using boost::asio::ssl::context;
    ctx.use_certificate_chain(
        boost::asio::buffer(certificate.data(), certificate.size()));
    ctx.use_private_key(
        boost::asio::buffer(private_key.data(), private_key.size()),
        context::pem);
}

int main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 3 && argc != 4)
    {
        std::cerr
            << "Usage: with_ssl <address> <port> [exposer_route]\n"
            << "  exposer_route defaults to /metrics\n"
            << "Example:\n"
            << "    with_ssl 0.0.0.0 8080\n"
            << "    with_ssl 0.0.0.0 8080 /metrics\n";
        return EXIT_FAILURE;
    }
    auto address = asio::ip::make_address(argv[1]);
    auto port = static_cast<unsigned short>(std::atoi(argv[2]));
    std::string route = "/metrics";
    if (argc == 4)
        route = std::string(argv[3]);

    asio::io_context ioc;

    prometheus::registry reg;
    prometheus::info_vec build_info(
        "build",
        {"build_type", "compiler", "boost_version"});
    build_info.with_labels(
        {"example_with_ssl",
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

    boost::asio::ssl::context server_ctx{boost::asio::ssl::context::tls_server};
    // Configure SSL context
    // (https://www.boost.org/doc/libs/latest/doc/html/boost_asio/overview/ssl.html).
    // Test setup: the server uses a self-signed certificate, so
    // scrapers must be configured with the matching root certificate from
    // test/include/test_common/client_certificate.hpp.
    load_server_certificate(server_ctx);

    prometheus::exposer<
        decltype(ioc.get_executor()), prometheus::ssl_policy<true>
    > exp(
        ioc.get_executor(),
        server_ctx,
        asio::ip::tcp::endpoint(address, port),
        std::move(route));
    exp.expose(reg);

    ioc.run();

    return EXIT_SUCCESS;
}
