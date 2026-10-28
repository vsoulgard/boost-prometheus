| Branch | GitHub Actions | Coverage | Docs |
|:---|:---|:---|:---|
| [`master`](https://github.com/vsoulgard/boost-prometheus/tree/master) | [![CI](https://github.com/vsoulgard/boost-prometheus/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/vsoulgard/boost-prometheus/actions/workflows/ci.yml?query=branch%3Amaster) | [![codecov](https://codecov.io/gh/vsoulgard/boost-prometheus/branch/master/graph/badge.svg)](https://codecov.io/gh/vsoulgard/boost-prometheus/tree/master) | [![Documentation](https://img.shields.io/badge/docs-master-brightgreen.svg)](https://vsoulgard.github.io/boost-prometheus/prometheus/index.html) |
| [`develop`](https://github.com/vsoulgard/boost-prometheus/tree/develop) | [![CI](https://github.com/vsoulgard/boost-prometheus/actions/workflows/ci.yml/badge.svg?branch=develop)](https://github.com/vsoulgard/boost-prometheus/actions/workflows/ci.yml?query=branch%3Adevelop) | [![codecov](https://codecov.io/gh/vsoulgard/boost-prometheus/branch/develop/graph/badge.svg)](https://codecov.io/gh/vsoulgard/boost-prometheus/tree/develop) | [![Documentation](https://img.shields.io/badge/docs-develop-brightgreen.svg)](https://vsoulgard.github.io/boost-prometheus/prometheus/index.html) |

# Boost.Prometheus

*(Not yet part of Boost.)*

Modern C++20 library for instrumenting applications with Prometheus and [OpenMetrics](https://prometheus.io/docs/specs/om/open_metrics_spec/) metrics.
Header-only, optimized for production, and designed to integrate naturally with Boost.Asio.

## Why Boost.Prometheus?

- **Both OpenMetrics 1.0 and Prometheus 0.0.4 text formats** — the response format is selected via the HTTP `Accept` header, so both formats can be served by the same exposer.
- **Heavily optimized** — See [benchmarks](bench/bench.ipynb).
- **Boost.Asio exposer** — HTTP exposition with gzip compression, authentication, and SSL/TLS.
- **Header-only** — just include and go.
- **Thread-Safe** — Metric updates (`inc()`, `observe()`), label lookup/creation (`with_labels()`), and metric registration (`register_metric()`) are fully thread-safe and can be called concurrently from multiple threads.
- **Static metrics** — labels are validated by the compiler, and the exposition text is serialized at compile time — minimal overhead.

## Requirements
- **Boost 1.90.0** (header-only libraries) and later.
- **C++20 compiler**: GCC 12+, Clang 18+, MSVC 14.3+ (Visual Studio 2022).
- **OpenSSL** — optional, only required for SSL/TLS support in the exposer.

## Quick example
```cpp
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
    prometheus::info_vec build_info("build", {"build_type", "version"});
    prometheus::counter_vec requests("requests",
        {.help = "Number of received requests", .unit = ""},
        {"method"});
    prometheus::histogram req_duration(
        "request_duration_seconds",
        {.help = "Duration of HTTP requests in seconds",
            .unit = "seconds", .buckets = {}}); // Default buckets

    build_info.with_labels({"simple_example", "1.0"});
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
```
## Contributing
Contributions and feedback are highly welcome! To ensure a smooth process:
- Please **open an issue first** to discuss any major improvements, architectural changes, or new features before submitting a pull request.
- This repository follows a **squash and merge** workflow. Please ensure your PR represents a single, logical unit of work.
