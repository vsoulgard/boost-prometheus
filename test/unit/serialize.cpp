//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/prometheus/registry.hpp>
#include <boost/prometheus/serialize.hpp>
#include <boost/prometheus/static_registry.hpp>
#include <boost/prometheus/static_counter.hpp>
#include <boost/prometheus/static_gauge.hpp>
#include <boost/prometheus/static_histogram.hpp>
#include <boost/prometheus/static_info.hpp>

#include <boost/test/unit_test.hpp>

#include <charconv>
#include <string>
#include <thread>

using namespace boost::prometheus;

double extract_double_from_string(std::string_view text, std::string_view prefix)
{
    std::size_t pos = 0;
    while (pos < text.size())
    {
        std::size_t eol = text.find('\n', pos);
        if (eol == std::string_view::npos) eol = text.size();

        std::string_view line = text.substr(pos, eol - pos);
        if (line.starts_with(prefix))
        {
            std::string_view value = line.substr(prefix.size());
            while (!value.empty() && value.front() == ' ') value.remove_prefix(1);

            double out = 0.0;
            auto result = std::from_chars(
                value.data(), value.data() + value.size(), out);
            BOOST_ASSERT(result.ec == std::errc());
            (void)result;
            return out;
        }
        pos = eol + 1u;
    }
    return std::nan("");
}

template <typename AtomicPolicyType>
void test_counter_prometheus_0_0_4()
{
    registry<AtomicPolicyType> reg;
    counter<AtomicPolicyType> c("http_requests");
    reg.register_metric(c);

    std::string result;
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests_total counter\n"
        "http_requests_total 0\n"));
    result.clear();
    c.inc();
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests_total counter\n"
        "http_requests_total 1\n"));
    result.clear();
    c.inc();
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests_total counter\n"
        "http_requests_total 2\n"));
    result.clear();
    c.inc(0.5);
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests_total counter\n"
        "http_requests_total 2.5\n"));
    result.clear();
    c.inc(5.5);
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests_total counter\n"
        "http_requests_total 8\n"));
    // Multiple counters
    counter<AtomicPolicyType> c2("http_requests2", {.help = "help text", .unit = ""});
    counter<AtomicPolicyType> c3("test_c_seconds", {.help = "", .unit = "seconds"});
    c3.inc();
    reg.register_metric(c2);
    reg.register_metric(c3);
    result.clear();
    serialize<format::prometheus_0_0_4>(reg, result);
    // Order is not guaranteed
    BOOST_TEST(result.find(
        "# TYPE http_requests_total counter\n"
        "http_requests_total 8\n") != std::string::npos);
    BOOST_TEST(result.find(
        "# TYPE http_requests2_total counter\n"
        "# HELP http_requests2_total help text\n"
        "http_requests2_total 0\n") != std::string::npos);
    BOOST_TEST(result.find(
        "# TYPE test_c_seconds_total counter\n"
        "test_c_seconds_total 1\n") != std::string::npos);
    BOOST_TEST(result.size() == 57u + 97u + 59u);
}

template <typename AtomicPolicyType>
void test_counter_openmetrics_1_0()
{
    registry<AtomicPolicyType> reg;
    counter<AtomicPolicyType> c("http_requests");
    reg.register_metric(c);

    std::string result;
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests counter\n"
        "http_requests_total 0\n"
        "# EOF\n"));
    result.clear();
    c.inc();
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests counter\n"
        "http_requests_total 1\n"
        "# EOF\n"));
    result.clear();
    c.inc();
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests counter\n"
        "http_requests_total 2\n"
        "# EOF\n"));
    result.clear();
    c.inc(0.5);
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests counter\n"
        "http_requests_total 2.5\n"
        "# EOF\n"));
    result.clear();
    c.inc(5.5);
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests counter\n"
        "http_requests_total 8\n"
        "# EOF\n"));
    // Multiple counters
    counter<AtomicPolicyType> c2("http_requests2", {.help = "help text", .unit = ""});
    counter<AtomicPolicyType> c3("test_c_seconds", {.help = "", .unit = "seconds"});
    c3.inc();
    reg.register_metric(c2);
    reg.register_metric(c3);
    result.clear();
    serialize<format::openmetrics_1_0>(reg, result);
    // Order is not guaranteed
    BOOST_TEST(result.find(
        "# TYPE http_requests counter\n"
        "http_requests_total 8\n") != std::string::npos);
    BOOST_TEST(result.find(
        "# TYPE http_requests2 counter\n"
        "# HELP http_requests2 help text\n"
        "http_requests2_total 0\n") != std::string::npos);
    BOOST_TEST(result.find(
        "# TYPE test_c_seconds counter\n"
        "# UNIT test_c_seconds seconds\n"
        "test_c_seconds_total 1\n") != std::string::npos);
    BOOST_TEST(result.substr(result.size() - 6u, 6u) == std::string_view(
        "# EOF\n"
    ));
    BOOST_TEST(result.size() == 51u + 85u + 83u + 6u);
}

template <typename AtomicPolicyType>
void test_counter_vec_prometheus_0_0_4()
{
    registry<AtomicPolicyType> reg;
    counter_vec<AtomicPolicyType> c("http_requests", {"first", "second"});
    reg.register_metric(c);

    std::string result;
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result.empty());
    c.with_labels({"1", "2"}).inc();
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests_total counter\n"
        "http_requests_total{first=\"1\",second=\"2\"} 1\n"));
    result.clear();
    c.with_labels({"1", "2"}).inc();
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests_total counter\n"
        "http_requests_total{first=\"1\",second=\"2\"} 2\n"));
    result.clear();
    c.with_labels({"3", "4"}).inc();
    serialize<format::prometheus_0_0_4>(reg, result);
    // Order is not guaranteed
    BOOST_TEST(result.find(
        "# TYPE http_requests_total counter\n") != std::string::npos);
    BOOST_TEST(result.find(
        "http_requests_total{first=\"1\",second=\"2\"} 2\n") != std::string::npos);
    BOOST_TEST(result.find(
        "http_requests_total{first=\"3\",second=\"4\"} 1\n") != std::string::npos);
    BOOST_TEST(result.size() == 35u + 44u + 44u);
    result.clear();
    c.with_labels({"3", "4"}).inc();
    c.with_labels({"1", "3"}).inc();
    serialize<format::prometheus_0_0_4>(reg, result);
    std::size_t header_pos, metric_pos;
    header_pos = result.find("# TYPE http_requests_total counter\n");
    BOOST_TEST(header_pos != std::string::npos);
    metric_pos = result.find("http_requests_total{first=\"1\",second=\"2\"} 2\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("http_requests_total{first=\"3\",second=\"4\"} 2\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("http_requests_total{first=\"1\",second=\"3\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.size() == 35u + 44u + 44u + 44u);
    // Multiple counter_vecs
    counter_vec<AtomicPolicyType> c2("http_requests2", {.help = "help text", .unit = ""}, {"third", "fourth"});
    counter_vec<AtomicPolicyType> c3("test_c_seconds", {.help = "", .unit = "seconds"}, {"first", "fifth"});
    c2.with_labels({"3", "4"}).inc();
    c3.with_labels({"1", "5"}).inc();
    reg.register_metric(c2);
    reg.register_metric(c3);
    result.clear();
    serialize<format::prometheus_0_0_4>(reg, result);

    header_pos = result.find("# TYPE http_requests_total counter\n");
    BOOST_TEST(header_pos != std::string::npos);
    metric_pos = result.find("http_requests_total{first=\"1\",second=\"2\"} 2\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("http_requests_total{first=\"3\",second=\"4\"} 2\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("http_requests_total{first=\"1\",second=\"3\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);

    header_pos = result.find(
        "# TYPE http_requests2_total counter\n"
        "# HELP http_requests2_total help text\n");
    metric_pos = result.find("http_requests2_total{fourth=\"4\",third=\"3\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);

    header_pos = result.find("# TYPE test_c_seconds_total counter\n");
    metric_pos = result.find("test_c_seconds_total{fifth=\"5\",first=\"1\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.size() == 35u + 44u + 44u + 44u + 74u + 45u + 36u + 44u);
}

template <typename AtomicPolicyType>
void test_counter_vec_openmetrics_1_0()
{
    registry<AtomicPolicyType> reg;
    counter_vec<AtomicPolicyType> c("http_requests", {"first", "second"});
    reg.register_metric(c);

    std::string result;
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == "# EOF\n");
    result.clear();
    c.with_labels({"1", "2"}).inc();
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests counter\n"
        "http_requests_total{first=\"1\",second=\"2\"} 1\n"
        "# EOF\n"));
    result.clear();
    c.with_labels({"1", "2"}).inc();
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests counter\n"
        "http_requests_total{first=\"1\",second=\"2\"} 2\n"
        "# EOF\n"));
    result.clear();
    c.with_labels({"3", "4"}).inc();
    serialize<format::openmetrics_1_0>(reg, result);
    // Order is not guaranteed
    BOOST_TEST(result.find(
        "# TYPE http_requests counter\n") != std::string::npos);
    BOOST_TEST(result.find(
        "http_requests_total{first=\"1\",second=\"2\"} 2\n") != std::string::npos);
    BOOST_TEST(result.find(
        "http_requests_total{first=\"3\",second=\"4\"} 1\n") != std::string::npos);
    BOOST_TEST(result.substr(result.size() - 6u, 6u) == std::string_view(
        "# EOF\n"
    ));
    BOOST_TEST(result.size() == 29u + 44u + 44u + 6u);
    result.clear();
    c.with_labels({"3", "4"}).inc();
    c.with_labels({"1", "3"}).inc();
    serialize<format::openmetrics_1_0>(reg, result);
    std::size_t header_pos, metric_pos;
    header_pos = result.find("# TYPE http_requests counter\n");
    BOOST_TEST(header_pos != std::string::npos);
    metric_pos = result.find("http_requests_total{first=\"1\",second=\"2\"} 2\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("http_requests_total{first=\"3\",second=\"4\"} 2\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("http_requests_total{first=\"1\",second=\"3\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.substr(result.size() - 6u, 6u) == std::string_view(
        "# EOF\n"
    ));
    BOOST_TEST(result.size() == 29u + 44u + 44u + 44u + 6u);
    // Multiple counter_vecs
    counter_vec<AtomicPolicyType> c2("http_requests2", {.help = "help text", .unit = ""}, {"third", "fourth"});
    counter_vec<AtomicPolicyType> c3("test_c_seconds", {.help = "", .unit = "seconds"}, {"first", "fifth"});
    c2.with_labels({"3", "4"}).inc();
    c3.with_labels({"1", "5"}).inc();
    reg.register_metric(c2);
    reg.register_metric(c3);
    result.clear();
    serialize<format::openmetrics_1_0>(reg, result);

    header_pos = result.find("# TYPE http_requests counter\n");
    BOOST_TEST(header_pos != std::string::npos);
    metric_pos = result.find("http_requests_total{first=\"1\",second=\"2\"} 2\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("http_requests_total{first=\"3\",second=\"4\"} 2\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("http_requests_total{first=\"1\",second=\"3\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);

    header_pos = result.find(
        "# TYPE http_requests2 counter\n"
        "# HELP http_requests2 help text\n");
    metric_pos = result.find("http_requests2_total{fourth=\"4\",third=\"3\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);

    header_pos = result.find(
        "# TYPE test_c_seconds counter\n"
        "# UNIT test_c_seconds seconds\n");
    metric_pos = result.find("test_c_seconds_total{fifth=\"5\",first=\"1\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.substr(result.size() - 6u, 6u) == std::string_view(
        "# EOF\n"
    ));
    BOOST_TEST(result.size() == 29u + 44u + 44u + 44u + 62u + 45u + 60u + 44u + 6u);
}

template <typename AtomicPolicyType>
void test_gauge_prometheus_0_0_4()
{
    registry<AtomicPolicyType> reg;
    gauge<AtomicPolicyType> g("test_gauge");
    reg.register_metric(g);

    std::string result;
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_gauge gauge\n"
        "test_gauge 0\n"));
    result.clear();
    g.inc();
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_gauge gauge\n"
        "test_gauge 1\n"));
    result.clear();
    g.inc();
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_gauge gauge\n"
        "test_gauge 2\n"));
    result.clear();
    g.set(2.5);
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_gauge gauge\n"
        "test_gauge 2.5\n"));
    result.clear();
    g.dec(5.5);
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_gauge gauge\n"
        "test_gauge -3\n"));
    // Multiple gauges
    gauge<AtomicPolicyType> g2("test_gauge2", {.help = "help text", .unit = ""});
    gauge<AtomicPolicyType> g3("test_gauge_bytes", {.help = "", .unit = "bytes"});
    g3.inc();
    reg.register_metric(g2);
    reg.register_metric(g3);
    result.clear();
    serialize<format::prometheus_0_0_4>(reg, result);
    // Order is not guaranteed
    BOOST_TEST(result.find(
        "# TYPE test_gauge gauge\n"
        "test_gauge -3\n") != std::string::npos);
    BOOST_TEST(result.find(
        "# TYPE test_gauge2 gauge\n"
        "# HELP test_gauge2 help text\n"
        "test_gauge2 0\n") != std::string::npos);
    BOOST_TEST(result.find(
        "# TYPE test_gauge_bytes gauge\n"
        "test_gauge_bytes 1\n") != std::string::npos);
    BOOST_TEST(result.size() == 38u + 68u + 49u);
}

template <typename AtomicPolicyType>
void test_gauge_openmetrics_1_0()
{
    registry<AtomicPolicyType> reg;
    gauge<AtomicPolicyType> g("test_gauge");
    reg.register_metric(g);

    std::string result;
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_gauge gauge\n"
        "test_gauge 0\n"
        "# EOF\n"));
    result.clear();
    g.inc();
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_gauge gauge\n"
        "test_gauge 1\n"
        "# EOF\n"));
    result.clear();
    g.inc();
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_gauge gauge\n"
        "test_gauge 2\n"
        "# EOF\n"));
    result.clear();
    g.set(2.5);
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_gauge gauge\n"
        "test_gauge 2.5\n"
        "# EOF\n"));
    result.clear();
    g.dec(5.5);
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_gauge gauge\n"
        "test_gauge -3\n"
        "# EOF\n"));
    // Multiple gauges
    gauge<AtomicPolicyType> g2("test_gauge2", {.help = "help text", .unit = ""});
    gauge<AtomicPolicyType> g3("test_gauge_bytes", {.help = "", .unit = "bytes"});
    g3.inc();
    reg.register_metric(g2);
    reg.register_metric(g3);
    result.clear();
    serialize<format::openmetrics_1_0>(reg, result);
    // Order is not guaranteed
    BOOST_TEST(result.find(
        "# TYPE test_gauge gauge\n"
        "test_gauge -3\n") != std::string::npos);
    BOOST_TEST(result.find(
        "# TYPE test_gauge2 gauge\n"
        "# HELP test_gauge2 help text\n"
        "test_gauge2 0\n") != std::string::npos);
    BOOST_TEST(result.find(
        "# TYPE test_gauge_bytes gauge\n"
        "# UNIT test_gauge_bytes bytes\n"
        "test_gauge_bytes 1\n") != std::string::npos);
    BOOST_TEST(result.substr(result.size() - 6u, 6u) == std::string_view(
        "# EOF\n"
    ));
    BOOST_TEST(result.size() == 38u + 68u + 79u + 6u);
}

template <typename AtomicPolicyType>
void test_gauge_vec_prometheus_0_0_4()
{
    registry<AtomicPolicyType> reg;
    gauge_vec<AtomicPolicyType> g("test_gauge", {"first", "second"});
    reg.register_metric(g);

    std::string result;
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result.empty());
    g.with_labels({"1", "2"}).inc();
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_gauge gauge\n"
        "test_gauge{first=\"1\",second=\"2\"} 1\n"));
    result.clear();
    g.with_labels({"1", "2"}).dec(2.0);
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_gauge gauge\n"
        "test_gauge{first=\"1\",second=\"2\"} -1\n"));
    result.clear();
    g.with_labels({"3", "4"}).inc();
    serialize<format::prometheus_0_0_4>(reg, result);
    // Order is not guaranteed
    BOOST_TEST(result.find(
        "# TYPE test_gauge gauge\n") != std::string::npos);
    BOOST_TEST(result.find(
        "test_gauge{first=\"1\",second=\"2\"} -1\n") != std::string::npos);
    BOOST_TEST(result.find(
        "test_gauge{first=\"3\",second=\"4\"} 1\n") != std::string::npos);
    BOOST_TEST(result.size() == 24u + 35u + 36u);
    result.clear();
    g.with_labels({"3", "4"}).inc();
    g.with_labels({"1", "3"}).inc();
    serialize<format::prometheus_0_0_4>(reg, result);
    std::size_t header_pos, metric_pos;
    header_pos = result.find("# TYPE test_gauge gauge\n");
    BOOST_TEST(header_pos != std::string::npos);
    metric_pos = result.find("test_gauge{first=\"1\",second=\"2\"} -1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("test_gauge{first=\"3\",second=\"4\"} 2\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("test_gauge{first=\"1\",second=\"3\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.size() == 24u + 35u + 35u + 36u);
    // Multiple gauge_vecs
    gauge_vec<AtomicPolicyType> g2("test_gauge2", {.help = "help text", .unit = ""}, {"third", "fourth"});
    gauge_vec<AtomicPolicyType> g3("test_gauge_bytes", {.help = "", .unit = "bytes"}, {"first", "fifth"});
    g2.with_labels({"3", "4"}).inc();
    g3.with_labels({"1", "5"}).inc();
    reg.register_metric(g2);
    reg.register_metric(g3);
    result.clear();
    serialize<format::prometheus_0_0_4>(reg, result);

    header_pos = result.find("# TYPE test_gauge gauge\n");
    BOOST_TEST(header_pos != std::string::npos);
    metric_pos = result.find("test_gauge{first=\"1\",second=\"2\"} -1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("test_gauge{first=\"3\",second=\"4\"} 2\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("test_gauge{first=\"1\",second=\"3\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);

    header_pos = result.find(
        "# TYPE test_gauge2 gauge\n"
        "# HELP test_gauge2 help text\n");
    metric_pos = result.find("test_gauge2{fourth=\"4\",third=\"3\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);

    header_pos = result.find(
        "# TYPE test_gauge_bytes gauge\n");
    metric_pos = result.find("test_gauge_bytes{fifth=\"5\",first=\"1\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.size() == 24u + 35u + 35u + 36u + 54u + 36u + 30u + 40u);
}

template <typename AtomicPolicyType>
void test_gauge_vec_openmetrics_1_0()
{
    registry<AtomicPolicyType> reg;
    gauge_vec<AtomicPolicyType> g("test_gauge", {"first", "second"});
    reg.register_metric(g);

    std::string result;
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == "# EOF\n");
    result.clear();
    g.with_labels({"1", "2"}).inc();
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_gauge gauge\n"
        "test_gauge{first=\"1\",second=\"2\"} 1\n"
        "# EOF\n"));
    result.clear();
    g.with_labels({"1", "2"}).dec(2.0);
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_gauge gauge\n"
        "test_gauge{first=\"1\",second=\"2\"} -1\n"
        "# EOF\n"));
    result.clear();
    g.with_labels({"3", "4"}).inc();
    serialize<format::openmetrics_1_0>(reg, result);
    // Order is not guaranteed
    BOOST_TEST(result.find(
        "# TYPE test_gauge gauge\n") != std::string::npos);
    BOOST_TEST(result.find(
        "test_gauge{first=\"1\",second=\"2\"} -1\n") != std::string::npos);
    BOOST_TEST(result.find(
        "test_gauge{first=\"3\",second=\"4\"} 1\n") != std::string::npos);
    BOOST_TEST(result.substr(result.size() - 6u, 6u) == std::string_view(
        "# EOF\n"
    ));
    BOOST_TEST(result.size() == 24u + 35u + 36u + 6u);
    result.clear();
    g.with_labels({"3", "4"}).inc();
    g.with_labels({"1", "3"}).inc();
    serialize<format::openmetrics_1_0>(reg, result);
    std::size_t header_pos, metric_pos;
    header_pos = result.find("# TYPE test_gauge gauge\n");
    BOOST_TEST(header_pos != std::string::npos);
    metric_pos = result.find("test_gauge{first=\"1\",second=\"2\"} -1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("test_gauge{first=\"3\",second=\"4\"} 2\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("test_gauge{first=\"1\",second=\"3\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.substr(result.size() - 6u, 6u) == std::string_view(
        "# EOF\n"
    ));
    BOOST_TEST(result.size() == 24u + 35u + 35u + 36u + 6u);
    // Multiple gauge_vecs
    gauge_vec<AtomicPolicyType> g2("test_gauge2", {.help = "help text", .unit = ""}, {"third", "fourth"});
    gauge_vec<AtomicPolicyType> g3("test_gauge_bytes", {.help = "", .unit = "bytes"}, {"first", "fifth"});
    g2.with_labels({"3", "4"}).inc();
    g3.with_labels({"1", "5"}).inc();
    reg.register_metric(g2);
    reg.register_metric(g3);
    result.clear();
    serialize<format::openmetrics_1_0>(reg, result);

    header_pos = result.find("# TYPE test_gauge gauge\n");
    BOOST_TEST(header_pos != std::string::npos);
    metric_pos = result.find("test_gauge{first=\"1\",second=\"2\"} -1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("test_gauge{first=\"3\",second=\"4\"} 2\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("test_gauge{first=\"1\",second=\"3\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);

    header_pos = result.find(
        "# TYPE test_gauge2 gauge\n"
        "# HELP test_gauge2 help text\n");
    metric_pos = result.find("test_gauge2{fourth=\"4\",third=\"3\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);

    header_pos = result.find(
        "# TYPE test_gauge_bytes gauge\n"
        "# UNIT test_gauge_bytes bytes\n");
    metric_pos = result.find("test_gauge_bytes{fifth=\"5\",first=\"1\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.substr(result.size() - 6u, 6u) == std::string_view(
        "# EOF\n"
    ));
    BOOST_TEST(result.size() == 24u + 35u + 35u + 36u + 54u + 36u + 60u + 40u + 6u);
}

template <typename AtomicPolicyType>
void test_histogram_prometheus_0_0_4()
{
    registry<AtomicPolicyType> reg;
    // With default buckets
    // 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0
    histogram<AtomicPolicyType> h("test_histogram");
    reg.register_metric(h);

    std::string result;
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_histogram histogram\n"
        "test_histogram_bucket{le=\"0.005\"} 0\n"
        "test_histogram_bucket{le=\"0.01\"} 0\n"
        "test_histogram_bucket{le=\"0.025\"} 0\n"
        "test_histogram_bucket{le=\"0.05\"} 0\n"
        "test_histogram_bucket{le=\"0.1\"} 0\n"
        "test_histogram_bucket{le=\"0.25\"} 0\n"
        "test_histogram_bucket{le=\"0.5\"} 0\n"
        "test_histogram_bucket{le=\"1\"} 0\n"
        "test_histogram_bucket{le=\"2.5\"} 0\n"
        "test_histogram_bucket{le=\"5\"} 0\n"
        "test_histogram_bucket{le=\"10\"} 0\n"
        "test_histogram_bucket{le=\"+Inf\"} 0\n"
        "test_histogram_sum 0\n"
        "test_histogram_count 0\n"));
    result.clear();
    h.observe(0.03);
    h.observe(0.51);
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_histogram histogram\n"
        "test_histogram_bucket{le=\"0.005\"} 0\n"
        "test_histogram_bucket{le=\"0.01\"} 0\n"
        "test_histogram_bucket{le=\"0.025\"} 0\n"
        "test_histogram_bucket{le=\"0.05\"} 1\n"
        "test_histogram_bucket{le=\"0.1\"} 1\n"
        "test_histogram_bucket{le=\"0.25\"} 1\n"
        "test_histogram_bucket{le=\"0.5\"} 1\n"
        "test_histogram_bucket{le=\"1\"} 2\n"
        "test_histogram_bucket{le=\"2.5\"} 2\n"
        "test_histogram_bucket{le=\"5\"} 2\n"
        "test_histogram_bucket{le=\"10\"} 2\n"
        "test_histogram_bucket{le=\"+Inf\"} 2\n"
        "test_histogram_sum 0.54\n"
        "test_histogram_count 2\n"));
    result.clear();
    h.observe(0.02);
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_histogram histogram\n"
        "test_histogram_bucket{le=\"0.005\"} 0\n"
        "test_histogram_bucket{le=\"0.01\"} 0\n"
        "test_histogram_bucket{le=\"0.025\"} 1\n"
        "test_histogram_bucket{le=\"0.05\"} 2\n"
        "test_histogram_bucket{le=\"0.1\"} 2\n"
        "test_histogram_bucket{le=\"0.25\"} 2\n"
        "test_histogram_bucket{le=\"0.5\"} 2\n"
        "test_histogram_bucket{le=\"1\"} 3\n"
        "test_histogram_bucket{le=\"2.5\"} 3\n"
        "test_histogram_bucket{le=\"5\"} 3\n"
        "test_histogram_bucket{le=\"10\"} 3\n"
        "test_histogram_bucket{le=\"+Inf\"} 3\n"
        "test_histogram_sum 0.56\n"
        "test_histogram_count 3\n"));
    result.clear();
    // Multiple histograms
    histogram<AtomicPolicyType> h2("test_histogram2", {.help = "help text", .unit = "", .buckets = {0.1, 0.2, 0.3}});
    histogram<AtomicPolicyType> h3("request_duration_seconds", {.help = "", .unit = "seconds", .buckets = {}});
    reg.register_metric(h2);
    reg.register_metric(h3);
    h2.observe(0.02);
    serialize<format::prometheus_0_0_4>(reg, result);
    // Order is not guaranteed
    BOOST_TEST(result.find(
        "# TYPE test_histogram histogram\n"
        "test_histogram_bucket{le=\"0.005\"} 0\n"
        "test_histogram_bucket{le=\"0.01\"} 0\n"
        "test_histogram_bucket{le=\"0.025\"} 1\n"
        "test_histogram_bucket{le=\"0.05\"} 2\n"
        "test_histogram_bucket{le=\"0.1\"} 2\n"
        "test_histogram_bucket{le=\"0.25\"} 2\n"
        "test_histogram_bucket{le=\"0.5\"} 2\n"
        "test_histogram_bucket{le=\"1\"} 3\n"
        "test_histogram_bucket{le=\"2.5\"} 3\n"
        "test_histogram_bucket{le=\"5\"} 3\n"
        "test_histogram_bucket{le=\"10\"} 3\n"
        "test_histogram_bucket{le=\"+Inf\"} 3\n"
        "test_histogram_sum 0.56\n"
        "test_histogram_count 3\n") != std::string::npos);
    BOOST_TEST(result.find(
        "# TYPE test_histogram2 histogram\n"
        "# HELP test_histogram2 help text\n"
        "test_histogram2_bucket{le=\"0.1\"} 1\n"
        "test_histogram2_bucket{le=\"0.2\"} 1\n"
        "test_histogram2_bucket{le=\"0.3\"} 1\n"
        "test_histogram2_bucket{le=\"+Inf\"} 1\n"
        "test_histogram2_sum 0.02\n"
        "test_histogram2_count 1\n") != std::string::npos);
    BOOST_TEST(result.find(
        "# TYPE request_duration_seconds histogram\n"
        "request_duration_seconds_bucket{le=\"0.005\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.01\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.025\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.05\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.1\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.25\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.5\"} 0\n"
        "request_duration_seconds_bucket{le=\"1\"} 0\n"
        "request_duration_seconds_bucket{le=\"2.5\"} 0\n"
        "request_duration_seconds_bucket{le=\"5\"} 0\n"
        "request_duration_seconds_bucket{le=\"10\"} 0\n"
        "request_duration_seconds_bucket{le=\"+Inf\"} 0\n"
        "request_duration_seconds_sum 0\n"
        "request_duration_seconds_count 0\n") != std::string::npos);
    BOOST_TEST(result.size() == 490u + 256u + 637u);
    result.clear();
    reg.unregister_metric(h);
    reg.unregister_metric(h2);
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE request_duration_seconds histogram\n"
        "request_duration_seconds_bucket{le=\"0.005\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.01\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.025\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.05\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.1\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.25\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.5\"} 0\n"
        "request_duration_seconds_bucket{le=\"1\"} 0\n"
        "request_duration_seconds_bucket{le=\"2.5\"} 0\n"
        "request_duration_seconds_bucket{le=\"5\"} 0\n"
        "request_duration_seconds_bucket{le=\"10\"} 0\n"
        "request_duration_seconds_bucket{le=\"+Inf\"} 0\n"
        "request_duration_seconds_sum 0\n"
        "request_duration_seconds_count 0\n"));
}

template <typename AtomicPolicyType>
void test_histogram_openmetrics_1_0()
{
    registry<AtomicPolicyType> reg;
    // With default buckets
    // 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0
    histogram<AtomicPolicyType> h("test_histogram");
    reg.register_metric(h);

    std::string result;
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_histogram histogram\n"
        "test_histogram_bucket{le=\"0.005\"} 0\n"
        "test_histogram_bucket{le=\"0.01\"} 0\n"
        "test_histogram_bucket{le=\"0.025\"} 0\n"
        "test_histogram_bucket{le=\"0.05\"} 0\n"
        "test_histogram_bucket{le=\"0.1\"} 0\n"
        "test_histogram_bucket{le=\"0.25\"} 0\n"
        "test_histogram_bucket{le=\"0.5\"} 0\n"
        "test_histogram_bucket{le=\"1\"} 0\n"
        "test_histogram_bucket{le=\"2.5\"} 0\n"
        "test_histogram_bucket{le=\"5\"} 0\n"
        "test_histogram_bucket{le=\"10\"} 0\n"
        "test_histogram_bucket{le=\"+Inf\"} 0\n"
        "test_histogram_sum 0\n"
        "test_histogram_count 0\n"
        "# EOF\n"));
    result.clear();
    h.observe(0.03);
    h.observe(0.51);
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_histogram histogram\n"
        "test_histogram_bucket{le=\"0.005\"} 0\n"
        "test_histogram_bucket{le=\"0.01\"} 0\n"
        "test_histogram_bucket{le=\"0.025\"} 0\n"
        "test_histogram_bucket{le=\"0.05\"} 1\n"
        "test_histogram_bucket{le=\"0.1\"} 1\n"
        "test_histogram_bucket{le=\"0.25\"} 1\n"
        "test_histogram_bucket{le=\"0.5\"} 1\n"
        "test_histogram_bucket{le=\"1\"} 2\n"
        "test_histogram_bucket{le=\"2.5\"} 2\n"
        "test_histogram_bucket{le=\"5\"} 2\n"
        "test_histogram_bucket{le=\"10\"} 2\n"
        "test_histogram_bucket{le=\"+Inf\"} 2\n"
        "test_histogram_sum 0.54\n"
        "test_histogram_count 2\n"
        "# EOF\n"));
    result.clear();
    h.observe(0.02);
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_histogram histogram\n"
        "test_histogram_bucket{le=\"0.005\"} 0\n"
        "test_histogram_bucket{le=\"0.01\"} 0\n"
        "test_histogram_bucket{le=\"0.025\"} 1\n"
        "test_histogram_bucket{le=\"0.05\"} 2\n"
        "test_histogram_bucket{le=\"0.1\"} 2\n"
        "test_histogram_bucket{le=\"0.25\"} 2\n"
        "test_histogram_bucket{le=\"0.5\"} 2\n"
        "test_histogram_bucket{le=\"1\"} 3\n"
        "test_histogram_bucket{le=\"2.5\"} 3\n"
        "test_histogram_bucket{le=\"5\"} 3\n"
        "test_histogram_bucket{le=\"10\"} 3\n"
        "test_histogram_bucket{le=\"+Inf\"} 3\n"
        "test_histogram_sum 0.56\n"
        "test_histogram_count 3\n"
        "# EOF\n"));
    result.clear();
    // Multiple histograms
    histogram<AtomicPolicyType> h2("test_histogram2", {.help = "help text", .unit = "", .buckets = {0.1, 0.2, 0.3}});
    histogram<AtomicPolicyType> h3("request_duration_seconds", {.help = "", .unit = "seconds", .buckets = {}});
    reg.register_metric(h2);
    reg.register_metric(h3);
    h2.observe(0.02);
    serialize<format::openmetrics_1_0>(reg, result);
    // Order is not guaranteed
    BOOST_TEST(result.find(
        "# TYPE test_histogram histogram\n"
        "test_histogram_bucket{le=\"0.005\"} 0\n"
        "test_histogram_bucket{le=\"0.01\"} 0\n"
        "test_histogram_bucket{le=\"0.025\"} 1\n"
        "test_histogram_bucket{le=\"0.05\"} 2\n"
        "test_histogram_bucket{le=\"0.1\"} 2\n"
        "test_histogram_bucket{le=\"0.25\"} 2\n"
        "test_histogram_bucket{le=\"0.5\"} 2\n"
        "test_histogram_bucket{le=\"1\"} 3\n"
        "test_histogram_bucket{le=\"2.5\"} 3\n"
        "test_histogram_bucket{le=\"5\"} 3\n"
        "test_histogram_bucket{le=\"10\"} 3\n"
        "test_histogram_bucket{le=\"+Inf\"} 3\n"
        "test_histogram_sum 0.56\n"
        "test_histogram_count 3\n") != std::string::npos);
    BOOST_TEST(result.find(
        "# TYPE test_histogram2 histogram\n"
        "# HELP test_histogram2 help text\n"
        "test_histogram2_bucket{le=\"0.1\"} 1\n"
        "test_histogram2_bucket{le=\"0.2\"} 1\n"
        "test_histogram2_bucket{le=\"0.3\"} 1\n"
        "test_histogram2_bucket{le=\"+Inf\"} 1\n"
        "test_histogram2_sum 0.02\n"
        "test_histogram2_count 1\n") != std::string::npos);
    BOOST_TEST(result.find(
        "# TYPE request_duration_seconds histogram\n"
        "# UNIT request_duration_seconds seconds\n"
        "request_duration_seconds_bucket{le=\"0.005\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.01\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.025\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.05\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.1\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.25\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.5\"} 0\n"
        "request_duration_seconds_bucket{le=\"1\"} 0\n"
        "request_duration_seconds_bucket{le=\"2.5\"} 0\n"
        "request_duration_seconds_bucket{le=\"5\"} 0\n"
        "request_duration_seconds_bucket{le=\"10\"} 0\n"
        "request_duration_seconds_bucket{le=\"+Inf\"} 0\n"
        "request_duration_seconds_sum 0\n"
        "request_duration_seconds_count 0\n") != std::string::npos);
    BOOST_TEST(result.substr(result.size() - 6u, 6u) == std::string_view(
        "# EOF\n"
    ));
    BOOST_TEST(result.size() == 490u + 256u + 677u + 6u);
    result.clear();
    reg.unregister_metric(h);
    reg.unregister_metric(h2);
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE request_duration_seconds histogram\n"
        "# UNIT request_duration_seconds seconds\n"
        "request_duration_seconds_bucket{le=\"0.005\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.01\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.025\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.05\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.1\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.25\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.5\"} 0\n"
        "request_duration_seconds_bucket{le=\"1\"} 0\n"
        "request_duration_seconds_bucket{le=\"2.5\"} 0\n"
        "request_duration_seconds_bucket{le=\"5\"} 0\n"
        "request_duration_seconds_bucket{le=\"10\"} 0\n"
        "request_duration_seconds_bucket{le=\"+Inf\"} 0\n"
        "request_duration_seconds_sum 0\n"
        "request_duration_seconds_count 0\n"
        "# EOF\n"));
}

template <typename AtomicPolicyType>
void test_histogram_vec_prometheus_0_0_4()
{
    registry<AtomicPolicyType> reg;
    // With default buckets
    // 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0
    histogram_vec<AtomicPolicyType> h("test_histogram", {"first", "second"});
    reg.register_metric(h);

    std::string result;
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result.empty());
    h.with_labels({"1", "2"}).observe(0.8);
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_histogram histogram\n"
        "test_histogram_bucket{first=\"1\",le=\"0.005\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.01\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.025\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.05\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.1\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.25\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.5\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"1\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"2.5\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"5\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"10\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"+Inf\",second=\"2\"} 1\n"
        "test_histogram_sum{first=\"1\",second=\"2\"} 0.8\n"
        "test_histogram_count{first=\"1\",second=\"2\"} 1\n"));
    result.clear();
    h.with_labels({"3", "4"}).observe(1.0);
    serialize<format::prometheus_0_0_4>(reg, result);
    // Order is not guaranteed
    std::size_t header_pos = result.find("# TYPE test_histogram histogram\n");
    BOOST_TEST(header_pos != std::string::npos);
    std::size_t metric_pos = result.find(
        "test_histogram_bucket{first=\"1\",le=\"0.005\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.01\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.025\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.05\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.1\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.25\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.5\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"1\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"2.5\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"5\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"10\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"+Inf\",second=\"2\"} 1\n"
        "test_histogram_sum{first=\"1\",second=\"2\"} 0.8\n"
        "test_histogram_count{first=\"1\",second=\"2\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find(
        "test_histogram_bucket{first=\"3\",le=\"0.005\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.01\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.025\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.05\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.1\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.25\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.5\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"1\",second=\"4\"} 1\n"
        "test_histogram_bucket{first=\"3\",le=\"2.5\",second=\"4\"} 1\n"
        "test_histogram_bucket{first=\"3\",le=\"5\",second=\"4\"} 1\n"
        "test_histogram_bucket{first=\"3\",le=\"10\",second=\"4\"} 1\n"
        "test_histogram_bucket{first=\"3\",le=\"+Inf\",second=\"4\"} 1\n"
        "test_histogram_sum{first=\"3\",second=\"4\"} 1\n"
        "test_histogram_count{first=\"3\",second=\"4\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.size() == 32u + 753u + 751u);
    result.clear();
    // Multiple histogram_vecs
    histogram_vec<AtomicPolicyType> h2("test_histogram2",
        {.help = "help text", .unit = "", .buckets = {0.1, 0.2, 0.3}},
        {"third", "fourth"});
    histogram_vec<AtomicPolicyType> h3("request_duration_seconds", {.help = "", .unit = "seconds", .buckets = {}}, {"method"});
    h2.with_labels({"3", "4"}).observe(0.01);
    h3.with_labels({"GET"}).observe(0.5);
    reg.register_metric(h2);
    reg.register_metric(h3);
    serialize<format::prometheus_0_0_4>(reg, result);
    header_pos = result.find("# TYPE test_histogram histogram\n");
    BOOST_TEST(header_pos != std::string::npos);
    metric_pos = result.find(
        "test_histogram_bucket{first=\"1\",le=\"0.005\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.01\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.025\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.05\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.1\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.25\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.5\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"1\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"2.5\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"5\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"10\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"+Inf\",second=\"2\"} 1\n"
        "test_histogram_sum{first=\"1\",second=\"2\"} 0.8\n"
        "test_histogram_count{first=\"1\",second=\"2\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find(
        "test_histogram_bucket{first=\"3\",le=\"0.005\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.01\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.025\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.05\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.1\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.25\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.5\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"1\",second=\"4\"} 1\n"
        "test_histogram_bucket{first=\"3\",le=\"2.5\",second=\"4\"} 1\n"
        "test_histogram_bucket{first=\"3\",le=\"5\",second=\"4\"} 1\n"
        "test_histogram_bucket{first=\"3\",le=\"10\",second=\"4\"} 1\n"
        "test_histogram_bucket{first=\"3\",le=\"+Inf\",second=\"4\"} 1\n"
        "test_histogram_sum{first=\"3\",second=\"4\"} 1\n"
        "test_histogram_count{first=\"3\",second=\"4\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.find(
        "# TYPE test_histogram2 histogram\n"
        "# HELP test_histogram2 help text\n"
        "test_histogram2_bucket{fourth=\"4\",le=\"0.1\",third=\"3\"} 1\n"
        "test_histogram2_bucket{fourth=\"4\",le=\"0.2\",third=\"3\"} 1\n"
        "test_histogram2_bucket{fourth=\"4\",le=\"0.3\",third=\"3\"} 1\n"
        "test_histogram2_bucket{fourth=\"4\",le=\"+Inf\",third=\"3\"} 1\n"
        "test_histogram2_sum{fourth=\"4\",third=\"3\"} 0.01\n"
        "test_histogram2_count{fourth=\"4\",third=\"3\"} 1\n") != std::string::npos);
    BOOST_TEST(result.find(
        "# TYPE request_duration_seconds histogram\n"
        "request_duration_seconds_bucket{le=\"0.005\",method=\"GET\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.01\",method=\"GET\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.025\",method=\"GET\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.05\",method=\"GET\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.1\",method=\"GET\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.25\",method=\"GET\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.5\",method=\"GET\"} 1\n"
        "request_duration_seconds_bucket{le=\"1\",method=\"GET\"} 1\n"
        "request_duration_seconds_bucket{le=\"2.5\",method=\"GET\"} 1\n"
        "request_duration_seconds_bucket{le=\"5\",method=\"GET\"} 1\n"
        "request_duration_seconds_bucket{le=\"10\",method=\"GET\"} 1\n"
        "request_duration_seconds_bucket{le=\"+Inf\",method=\"GET\"} 1\n"
        "request_duration_seconds_sum{method=\"GET\"} 0.5\n"
        "request_duration_seconds_count{method=\"GET\"} 1\n") != std::string::npos);
    BOOST_TEST(result.size() == 32u + 753u + 751u + 384u + 823u);
}

template <typename AtomicPolicyType>
void test_histogram_vec_openmetrics_1_0()
{
    registry<AtomicPolicyType> reg;
    // With default buckets
    // 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0
    histogram_vec<AtomicPolicyType> h("test_histogram", {"first", "second"});
    reg.register_metric(h);

    std::string result;
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == "# EOF\n");
    result.clear();
    h.with_labels({"1", "2"}).observe(0.8);
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_histogram histogram\n"
        "test_histogram_bucket{first=\"1\",le=\"0.005\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.01\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.025\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.05\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.1\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.25\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.5\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"1\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"2.5\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"5\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"10\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"+Inf\",second=\"2\"} 1\n"
        "test_histogram_sum{first=\"1\",second=\"2\"} 0.8\n"
        "test_histogram_count{first=\"1\",second=\"2\"} 1\n"
        "# EOF\n"));
    result.clear();
    h.with_labels({"3", "4"}).observe(1.0);
    serialize<format::openmetrics_1_0>(reg, result);
    // Order is not guaranteed
    std::size_t header_pos = result.find("# TYPE test_histogram histogram\n");
    BOOST_TEST(header_pos != std::string::npos);
    std::size_t metric_pos = result.find(
        "test_histogram_bucket{first=\"1\",le=\"0.005\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.01\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.025\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.05\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.1\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.25\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.5\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"1\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"2.5\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"5\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"10\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"+Inf\",second=\"2\"} 1\n"
        "test_histogram_sum{first=\"1\",second=\"2\"} 0.8\n"
        "test_histogram_count{first=\"1\",second=\"2\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find(
        "test_histogram_bucket{first=\"3\",le=\"0.005\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.01\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.025\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.05\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.1\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.25\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.5\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"1\",second=\"4\"} 1\n"
        "test_histogram_bucket{first=\"3\",le=\"2.5\",second=\"4\"} 1\n"
        "test_histogram_bucket{first=\"3\",le=\"5\",second=\"4\"} 1\n"
        "test_histogram_bucket{first=\"3\",le=\"10\",second=\"4\"} 1\n"
        "test_histogram_bucket{first=\"3\",le=\"+Inf\",second=\"4\"} 1\n"
        "test_histogram_sum{first=\"3\",second=\"4\"} 1\n"
        "test_histogram_count{first=\"3\",second=\"4\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.substr(result.size() - 6u, 6u) == std::string_view(
        "# EOF\n"
    ));
    BOOST_TEST(result.size() == 32u + 753u + 751u + 6u);
    result.clear();
    // Multiple histogram_vecs
    histogram_vec<AtomicPolicyType> h2("test_histogram2",
        {.help = "help text", .unit = "", .buckets = {0.1, 0.2, 0.3}},
        {"third", "fourth"});
    histogram_vec<AtomicPolicyType> h3("request_duration_seconds",
        {.help = "", .unit = "seconds", .buckets = {}}, {"method"});
    h2.with_labels({"3", "4"}).observe(0.01);
    h3.with_labels({"GET"}).observe(0.5);
    reg.register_metric(h2);
    reg.register_metric(h3);
    serialize<format::openmetrics_1_0>(reg, result);
    header_pos = result.find("# TYPE test_histogram histogram\n");
    BOOST_TEST(header_pos != std::string::npos);
    metric_pos = result.find(
        "test_histogram_bucket{first=\"1\",le=\"0.005\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.01\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.025\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.05\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.1\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.25\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"0.5\",second=\"2\"} 0\n"
        "test_histogram_bucket{first=\"1\",le=\"1\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"2.5\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"5\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"10\",second=\"2\"} 1\n"
        "test_histogram_bucket{first=\"1\",le=\"+Inf\",second=\"2\"} 1\n"
        "test_histogram_sum{first=\"1\",second=\"2\"} 0.8\n"
        "test_histogram_count{first=\"1\",second=\"2\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find(
        "test_histogram_bucket{first=\"3\",le=\"0.005\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.01\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.025\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.05\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.1\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.25\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"0.5\",second=\"4\"} 0\n"
        "test_histogram_bucket{first=\"3\",le=\"1\",second=\"4\"} 1\n"
        "test_histogram_bucket{first=\"3\",le=\"2.5\",second=\"4\"} 1\n"
        "test_histogram_bucket{first=\"3\",le=\"5\",second=\"4\"} 1\n"
        "test_histogram_bucket{first=\"3\",le=\"10\",second=\"4\"} 1\n"
        "test_histogram_bucket{first=\"3\",le=\"+Inf\",second=\"4\"} 1\n"
        "test_histogram_sum{first=\"3\",second=\"4\"} 1\n"
        "test_histogram_count{first=\"3\",second=\"4\"} 1\n");
    BOOST_TEST(metric_pos != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.find(
        "# TYPE test_histogram2 histogram\n"
        "# HELP test_histogram2 help text\n"
        "test_histogram2_bucket{fourth=\"4\",le=\"0.1\",third=\"3\"} 1\n"
        "test_histogram2_bucket{fourth=\"4\",le=\"0.2\",third=\"3\"} 1\n"
        "test_histogram2_bucket{fourth=\"4\",le=\"0.3\",third=\"3\"} 1\n"
        "test_histogram2_bucket{fourth=\"4\",le=\"+Inf\",third=\"3\"} 1\n"
        "test_histogram2_sum{fourth=\"4\",third=\"3\"} 0.01\n"
        "test_histogram2_count{fourth=\"4\",third=\"3\"} 1\n") != std::string::npos);
    BOOST_TEST(result.find(
        "# TYPE request_duration_seconds histogram\n"
        "# UNIT request_duration_seconds seconds\n"
        "request_duration_seconds_bucket{le=\"0.005\",method=\"GET\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.01\",method=\"GET\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.025\",method=\"GET\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.05\",method=\"GET\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.1\",method=\"GET\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.25\",method=\"GET\"} 0\n"
        "request_duration_seconds_bucket{le=\"0.5\",method=\"GET\"} 1\n"
        "request_duration_seconds_bucket{le=\"1\",method=\"GET\"} 1\n"
        "request_duration_seconds_bucket{le=\"2.5\",method=\"GET\"} 1\n"
        "request_duration_seconds_bucket{le=\"5\",method=\"GET\"} 1\n"
        "request_duration_seconds_bucket{le=\"10\",method=\"GET\"} 1\n"
        "request_duration_seconds_bucket{le=\"+Inf\",method=\"GET\"} 1\n"
        "request_duration_seconds_sum{method=\"GET\"} 0.5\n"
        "request_duration_seconds_count{method=\"GET\"} 1\n") != std::string::npos);
    BOOST_TEST(result.substr(result.size() - 6u, 6u) == std::string_view(
        "# EOF\n"
    ));
    BOOST_TEST(result.size() == 32u + 753u + 751u + 384u + 863u + 6u);
}

template <typename AtomicPolicyType>
void test_gaugehistogram_prometheus_0_0_4()
{
    registry<AtomicPolicyType> reg;
    // With default buckets
    // 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0
    gaugehistogram<AtomicPolicyType> h("test_ghist");
    reg.register_metric(h);

    std::string result;
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_ghist histogram\n"
        "test_ghist_bucket{le=\"0.005\"} 0\n"
        "test_ghist_bucket{le=\"0.01\"} 0\n"
        "test_ghist_bucket{le=\"0.025\"} 0\n"
        "test_ghist_bucket{le=\"0.05\"} 0\n"
        "test_ghist_bucket{le=\"0.1\"} 0\n"
        "test_ghist_bucket{le=\"0.25\"} 0\n"
        "test_ghist_bucket{le=\"0.5\"} 0\n"
        "test_ghist_bucket{le=\"1\"} 0\n"
        "test_ghist_bucket{le=\"2.5\"} 0\n"
        "test_ghist_bucket{le=\"5\"} 0\n"
        "test_ghist_bucket{le=\"10\"} 0\n"
        "test_ghist_bucket{le=\"+Inf\"} 0\n"
        "# TYPE test_ghist_gsum gauge\n"
        "test_ghist_gsum 0\n"
        "# TYPE test_ghist_gcount gauge\n"
        "test_ghist_gcount 0\n"));
    result.clear();
    h.observe(0.03);
    h.observe(0.51);
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_ghist histogram\n"
        "test_ghist_bucket{le=\"0.005\"} 0\n"
        "test_ghist_bucket{le=\"0.01\"} 0\n"
        "test_ghist_bucket{le=\"0.025\"} 0\n"
        "test_ghist_bucket{le=\"0.05\"} 1\n"
        "test_ghist_bucket{le=\"0.1\"} 1\n"
        "test_ghist_bucket{le=\"0.25\"} 1\n"
        "test_ghist_bucket{le=\"0.5\"} 1\n"
        "test_ghist_bucket{le=\"1\"} 2\n"
        "test_ghist_bucket{le=\"2.5\"} 2\n"
        "test_ghist_bucket{le=\"5\"} 2\n"
        "test_ghist_bucket{le=\"10\"} 2\n"
        "test_ghist_bucket{le=\"+Inf\"} 2\n"
        "# TYPE test_ghist_gsum gauge\n"
        "test_ghist_gsum 0.54\n"
        "# TYPE test_ghist_gcount gauge\n"
        "test_ghist_gcount 2\n"));
    result.clear();
    h.sub(0.03);
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_ghist histogram\n"
        "test_ghist_bucket{le=\"0.005\"} 0\n"
        "test_ghist_bucket{le=\"0.01\"} 0\n"
        "test_ghist_bucket{le=\"0.025\"} 0\n"
        "test_ghist_bucket{le=\"0.05\"} 0\n"
        "test_ghist_bucket{le=\"0.1\"} 0\n"
        "test_ghist_bucket{le=\"0.25\"} 0\n"
        "test_ghist_bucket{le=\"0.5\"} 0\n"
        "test_ghist_bucket{le=\"1\"} 1\n"
        "test_ghist_bucket{le=\"2.5\"} 1\n"
        "test_ghist_bucket{le=\"5\"} 1\n"
        "test_ghist_bucket{le=\"10\"} 1\n"
        "test_ghist_bucket{le=\"+Inf\"} 1\n"
        "# TYPE test_ghist_gsum gauge\n"
        "test_ghist_gsum 0.51\n"
        "# TYPE test_ghist_gcount gauge\n"
        "test_ghist_gcount 1\n"));
    result.clear();
    // Multiple gaugehistograms
    gaugehistogram<AtomicPolicyType> h2("test_ghist2",
        {.help = "help text", .unit = "", .buckets = {0.1, 0.2, 0.3}});
    gaugehistogram<AtomicPolicyType> h3("temperature_dist_celsius",
        {.help = "", .unit = "celsius", .buckets = {}});
    reg.register_metric(h2);
    reg.register_metric(h3);
    h2.observe(0.02);
    serialize<format::prometheus_0_0_4>(reg, result);
    // Order is not guaranteed
    BOOST_TEST(result.find(
        "# TYPE test_ghist histogram\n"
        "test_ghist_bucket{le=\"0.005\"} 0\n"
        "test_ghist_bucket{le=\"0.01\"} 0\n"
        "test_ghist_bucket{le=\"0.025\"} 0\n"
        "test_ghist_bucket{le=\"0.05\"} 0\n"
        "test_ghist_bucket{le=\"0.1\"} 0\n"
        "test_ghist_bucket{le=\"0.25\"} 0\n"
        "test_ghist_bucket{le=\"0.5\"} 0\n"
        "test_ghist_bucket{le=\"1\"} 1\n"
        "test_ghist_bucket{le=\"2.5\"} 1\n"
        "test_ghist_bucket{le=\"5\"} 1\n"
        "test_ghist_bucket{le=\"10\"} 1\n"
        "test_ghist_bucket{le=\"+Inf\"} 1\n"
        "# TYPE test_ghist_gsum gauge\n"
        "test_ghist_gsum 0.51\n"
        "# TYPE test_ghist_gcount gauge\n"
        "test_ghist_gcount 1\n") != std::string::npos);
    BOOST_TEST(result.find(
        "# TYPE test_ghist2 histogram\n"
        "# HELP test_ghist2 help text\n"
        "test_ghist2_bucket{le=\"0.1\"} 1\n"
        "test_ghist2_bucket{le=\"0.2\"} 1\n"
        "test_ghist2_bucket{le=\"0.3\"} 1\n"
        "test_ghist2_bucket{le=\"+Inf\"} 1\n"
        "# TYPE test_ghist2_gsum gauge\n"
        "test_ghist2_gsum 0.02\n"
        "# TYPE test_ghist2_gcount gauge\n"
        "test_ghist2_gcount 1\n") != std::string::npos);
    BOOST_TEST(result.find(
        "# TYPE temperature_dist_celsius histogram\n"
        "temperature_dist_celsius_bucket{le=\"0.005\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.01\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.025\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.05\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.1\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.25\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.5\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"1\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"2.5\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"5\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"10\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"+Inf\"} 0\n"
        "# TYPE temperature_dist_celsius_gsum gauge\n"
        "temperature_dist_celsius_gsum 0\n"
        "# TYPE temperature_dist_celsius_gcount gauge\n"
        "temperature_dist_celsius_gcount 0\n") != std::string::npos);
    BOOST_TEST(result.size() == 492u + 288u + 727u);
    result.clear();
    reg.unregister_metric(h);
    reg.unregister_metric(h2);
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE temperature_dist_celsius histogram\n"
        "temperature_dist_celsius_bucket{le=\"0.005\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.01\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.025\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.05\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.1\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.25\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.5\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"1\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"2.5\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"5\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"10\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"+Inf\"} 0\n"
        "# TYPE temperature_dist_celsius_gsum gauge\n"
        "temperature_dist_celsius_gsum 0\n"
        "# TYPE temperature_dist_celsius_gcount gauge\n"
        "temperature_dist_celsius_gcount 0\n"));
}

template <typename AtomicPolicyType>
void test_gaugehistogram_openmetrics_1_0()
{
    registry<AtomicPolicyType> reg;
    // With default buckets
    // 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0
    gaugehistogram<AtomicPolicyType> h("test_ghist");
    reg.register_metric(h);

    std::string result;
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_ghist gaugehistogram\n"
        "test_ghist_bucket{le=\"0.005\"} 0\n"
        "test_ghist_bucket{le=\"0.01\"} 0\n"
        "test_ghist_bucket{le=\"0.025\"} 0\n"
        "test_ghist_bucket{le=\"0.05\"} 0\n"
        "test_ghist_bucket{le=\"0.1\"} 0\n"
        "test_ghist_bucket{le=\"0.25\"} 0\n"
        "test_ghist_bucket{le=\"0.5\"} 0\n"
        "test_ghist_bucket{le=\"1\"} 0\n"
        "test_ghist_bucket{le=\"2.5\"} 0\n"
        "test_ghist_bucket{le=\"5\"} 0\n"
        "test_ghist_bucket{le=\"10\"} 0\n"
        "test_ghist_bucket{le=\"+Inf\"} 0\n"
        "test_ghist_gsum 0\n"
        "test_ghist_gcount 0\n"
        "# EOF\n"));
    result.clear();
    h.observe(0.03);
    h.observe(0.51);
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_ghist gaugehistogram\n"
        "test_ghist_bucket{le=\"0.005\"} 0\n"
        "test_ghist_bucket{le=\"0.01\"} 0\n"
        "test_ghist_bucket{le=\"0.025\"} 0\n"
        "test_ghist_bucket{le=\"0.05\"} 1\n"
        "test_ghist_bucket{le=\"0.1\"} 1\n"
        "test_ghist_bucket{le=\"0.25\"} 1\n"
        "test_ghist_bucket{le=\"0.5\"} 1\n"
        "test_ghist_bucket{le=\"1\"} 2\n"
        "test_ghist_bucket{le=\"2.5\"} 2\n"
        "test_ghist_bucket{le=\"5\"} 2\n"
        "test_ghist_bucket{le=\"10\"} 2\n"
        "test_ghist_bucket{le=\"+Inf\"} 2\n"
        "test_ghist_gsum 0.54\n"
        "test_ghist_gcount 2\n"
        "# EOF\n"));
    result.clear();
    h.sub(0.03);
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_ghist gaugehistogram\n"
        "test_ghist_bucket{le=\"0.005\"} 0\n"
        "test_ghist_bucket{le=\"0.01\"} 0\n"
        "test_ghist_bucket{le=\"0.025\"} 0\n"
        "test_ghist_bucket{le=\"0.05\"} 0\n"
        "test_ghist_bucket{le=\"0.1\"} 0\n"
        "test_ghist_bucket{le=\"0.25\"} 0\n"
        "test_ghist_bucket{le=\"0.5\"} 0\n"
        "test_ghist_bucket{le=\"1\"} 1\n"
        "test_ghist_bucket{le=\"2.5\"} 1\n"
        "test_ghist_bucket{le=\"5\"} 1\n"
        "test_ghist_bucket{le=\"10\"} 1\n"
        "test_ghist_bucket{le=\"+Inf\"} 1\n"
        "test_ghist_gsum 0.51\n"
        "test_ghist_gcount 1\n"
        "# EOF\n"));
    result.clear();
    // Multiple gaugehistograms
    gaugehistogram<AtomicPolicyType> h2("test_ghist2",
        {.help = "help text", .unit = "", .buckets = {0.1, 0.2, 0.3}});
    gaugehistogram<AtomicPolicyType> h3("temperature_dist_celsius",
        {.help = "", .unit = "celsius", .buckets = {}});
    reg.register_metric(h2);
    reg.register_metric(h3);
    h2.observe(0.02);
    serialize<format::openmetrics_1_0>(reg, result);
    // Order is not guaranteed
    BOOST_TEST(result.find(
        "# TYPE test_ghist gaugehistogram\n"
        "test_ghist_bucket{le=\"0.005\"} 0\n"
        "test_ghist_bucket{le=\"0.01\"} 0\n"
        "test_ghist_bucket{le=\"0.025\"} 0\n"
        "test_ghist_bucket{le=\"0.05\"} 0\n"
        "test_ghist_bucket{le=\"0.1\"} 0\n"
        "test_ghist_bucket{le=\"0.25\"} 0\n"
        "test_ghist_bucket{le=\"0.5\"} 0\n"
        "test_ghist_bucket{le=\"1\"} 1\n"
        "test_ghist_bucket{le=\"2.5\"} 1\n"
        "test_ghist_bucket{le=\"5\"} 1\n"
        "test_ghist_bucket{le=\"10\"} 1\n"
        "test_ghist_bucket{le=\"+Inf\"} 1\n"
        "test_ghist_gsum 0.51\n"
        "test_ghist_gcount 1\n") != std::string::npos);
    BOOST_TEST(result.find(
        "# TYPE test_ghist2 gaugehistogram\n"
        "# HELP test_ghist2 help text\n"
        "test_ghist2_bucket{le=\"0.1\"} 1\n"
        "test_ghist2_bucket{le=\"0.2\"} 1\n"
        "test_ghist2_bucket{le=\"0.3\"} 1\n"
        "test_ghist2_bucket{le=\"+Inf\"} 1\n"
        "test_ghist2_gsum 0.02\n"
        "test_ghist2_gcount 1\n") != std::string::npos);
    BOOST_TEST(result.find(
        "# TYPE temperature_dist_celsius gaugehistogram\n"
        "# UNIT temperature_dist_celsius celsius\n"
        "temperature_dist_celsius_bucket{le=\"0.005\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.01\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.025\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.05\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.1\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.25\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.5\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"1\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"2.5\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"5\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"10\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"+Inf\"} 0\n"
        "temperature_dist_celsius_gsum 0\n"
        "temperature_dist_celsius_gcount 0\n") != std::string::npos);
    BOOST_TEST(result.substr(result.size() - 6u, 6u) == std::string_view(
        "# EOF\n"
    ));
    BOOST_TEST(result.size() == 437u + 231u + 684u + 6u);
    result.clear();
    reg.unregister_metric(h);
    reg.unregister_metric(h2);
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE temperature_dist_celsius gaugehistogram\n"
        "# UNIT temperature_dist_celsius celsius\n"
        "temperature_dist_celsius_bucket{le=\"0.005\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.01\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.025\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.05\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.1\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.25\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.5\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"1\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"2.5\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"5\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"10\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"+Inf\"} 0\n"
        "temperature_dist_celsius_gsum 0\n"
        "temperature_dist_celsius_gcount 0\n"
        "# EOF\n"));
}

template <typename AtomicPolicyType>
void test_gaugehistogram_vec_prometheus_0_0_4()
{
    registry<AtomicPolicyType> reg;
    // With default buckets
    // 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0
    gaugehistogram_vec<AtomicPolicyType> h("test_ghist", {"first", "second"});
    reg.register_metric(h);

    std::string result;
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result.empty());
    h.with_labels({"1", "2"}).observe(0.8);
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_ghist histogram\n"
        "test_ghist_bucket{first=\"1\",le=\"0.005\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.01\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.025\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.05\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.1\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.25\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.5\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"1\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"2.5\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"5\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"10\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"+Inf\",second=\"2\"} 1\n"
        "# TYPE test_ghist_gsum gauge\n"
        "test_ghist_gsum{first=\"1\",second=\"2\"} 0.8\n"
        "# TYPE test_ghist_gcount gauge\n"
        "test_ghist_gcount{first=\"1\",second=\"2\"} 1\n"));
    result.clear();
    h.with_labels({"3", "4"}).observe(1.0);
    serialize<format::prometheus_0_0_4>(reg, result);
    // Order is not guaranteed
    std::size_t header_pos = result.find("# TYPE test_ghist histogram\n");
    BOOST_TEST(header_pos != std::string::npos);
    std::size_t metric_pos = result.find(
        "test_ghist_bucket{first=\"1\",le=\"0.005\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.01\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.025\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.05\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.1\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.25\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.5\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"1\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"2.5\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"5\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"10\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"+Inf\",second=\"2\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find(
        "test_ghist_bucket{first=\"3\",le=\"0.005\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.01\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.025\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.05\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.1\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.25\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.5\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"1\",second=\"4\"} 1\n"
        "test_ghist_bucket{first=\"3\",le=\"2.5\",second=\"4\"} 1\n"
        "test_ghist_bucket{first=\"3\",le=\"5\",second=\"4\"} 1\n"
        "test_ghist_bucket{first=\"3\",le=\"10\",second=\"4\"} 1\n"
        "test_ghist_bucket{first=\"3\",le=\"+Inf\",second=\"4\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    header_pos = result.find("# TYPE test_ghist_gsum gauge\n");
    BOOST_TEST(header_pos != std::string::npos);
    metric_pos = result.find("test_ghist_gsum{first=\"1\",second=\"2\"} 0.8\n");
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("test_ghist_gsum{first=\"3\",second=\"4\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    header_pos = result.find("# TYPE test_ghist_gcount gauge\n");
    BOOST_TEST(header_pos != std::string::npos);
    metric_pos = result.find("test_ghist_gcount{first=\"1\",second=\"2\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("test_ghist_gcount{first=\"3\",second=\"4\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.size() == 28u + 615u + 615u + 29u + 42u + 40u + 31u + 42u + 42u);
    result.clear();
    // Multiple gaugehistogram_vecs
    gaugehistogram_vec<AtomicPolicyType> h2("test_ghist2",
        {.help = "help text", .unit = "", .buckets = {0.1, 0.2, 0.3}},
        {"third", "fourth"});
    gaugehistogram_vec<AtomicPolicyType> h3("temperature_dist_celsius",
        {.help = "", .unit = "celsius", .buckets = {}},
        {"region"});
    h2.with_labels({"3", "4"}).observe(0.01);
    h3.with_labels({"42"}).observe(0.5);
    reg.register_metric(h2);
    reg.register_metric(h3);
    serialize<format::prometheus_0_0_4>(reg, result);
    header_pos = result.find("# TYPE test_ghist histogram\n");
    BOOST_TEST(header_pos != std::string::npos);
    metric_pos = result.find(
        "test_ghist_bucket{first=\"1\",le=\"0.005\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.01\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.025\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.05\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.1\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.25\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.5\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"1\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"2.5\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"5\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"10\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"+Inf\",second=\"2\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find(
        "test_ghist_bucket{first=\"3\",le=\"0.005\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.01\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.025\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.05\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.1\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.25\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.5\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"1\",second=\"4\"} 1\n"
        "test_ghist_bucket{first=\"3\",le=\"2.5\",second=\"4\"} 1\n"
        "test_ghist_bucket{first=\"3\",le=\"5\",second=\"4\"} 1\n"
        "test_ghist_bucket{first=\"3\",le=\"10\",second=\"4\"} 1\n"
        "test_ghist_bucket{first=\"3\",le=\"+Inf\",second=\"4\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    header_pos = result.find("# TYPE test_ghist_gsum gauge\n");
    BOOST_TEST(header_pos != std::string::npos);
    metric_pos = result.find("test_ghist_gsum{first=\"1\",second=\"2\"} 0.8\n");
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("test_ghist_gsum{first=\"3\",second=\"4\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    header_pos = result.find("# TYPE test_ghist_gcount gauge\n");
    BOOST_TEST(header_pos != std::string::npos);
    metric_pos = result.find("test_ghist_gcount{first=\"1\",second=\"2\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("test_ghist_gcount{first=\"3\",second=\"4\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.find(
        "# TYPE test_ghist2 histogram\n"
        "# HELP test_ghist2 help text\n"
        "test_ghist2_bucket{fourth=\"4\",le=\"0.1\",third=\"3\"} 1\n"
        "test_ghist2_bucket{fourth=\"4\",le=\"0.2\",third=\"3\"} 1\n"
        "test_ghist2_bucket{fourth=\"4\",le=\"0.3\",third=\"3\"} 1\n"
        "test_ghist2_bucket{fourth=\"4\",le=\"+Inf\",third=\"3\"} 1\n"
        "# TYPE test_ghist2_gsum gauge\n"
        "test_ghist2_gsum{fourth=\"4\",third=\"3\"} 0.01\n"
        "# TYPE test_ghist2_gcount gauge\n"
        "test_ghist2_gcount{fourth=\"4\",third=\"3\"} 1\n") != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.find(
        "# TYPE temperature_dist_celsius histogram\n"
        "temperature_dist_celsius_bucket{le=\"0.005\",region=\"42\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.01\",region=\"42\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.025\",region=\"42\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.05\",region=\"42\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.1\",region=\"42\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.25\",region=\"42\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.5\",region=\"42\"} 1\n"
        "temperature_dist_celsius_bucket{le=\"1\",region=\"42\"} 1\n"
        "temperature_dist_celsius_bucket{le=\"2.5\",region=\"42\"} 1\n"
        "temperature_dist_celsius_bucket{le=\"5\",region=\"42\"} 1\n"
        "temperature_dist_celsius_bucket{le=\"10\",region=\"42\"} 1\n"
        "temperature_dist_celsius_bucket{le=\"+Inf\",region=\"42\"} 1\n"
        "# TYPE temperature_dist_celsius_gsum gauge\n"
        "temperature_dist_celsius_gsum{region=\"42\"} 0.5\n"
        "# TYPE temperature_dist_celsius_gcount gauge\n"
        "temperature_dist_celsius_gcount{region=\"42\"} 1\n") != std::string::npos);
    BOOST_TEST(result.size() == 28u + 615u + 615u + 29u + 42u + 40u + 31u + 42u + 42u + 416u + 899u);
}

template <typename AtomicPolicyType>
void test_gaugehistogram_vec_openmetrics_1_0()
{
    registry<AtomicPolicyType> reg;
    // With default buckets
    // 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0
    gaugehistogram_vec<AtomicPolicyType> h("test_ghist", {"first", "second"});
    reg.register_metric(h);

    std::string result;
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == "# EOF\n");
    result.clear();
    h.with_labels({"1", "2"}).observe(0.8);
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_ghist gaugehistogram\n"
        "test_ghist_bucket{first=\"1\",le=\"0.005\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.01\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.025\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.05\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.1\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.25\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.5\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"1\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"2.5\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"5\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"10\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"+Inf\",second=\"2\"} 1\n"
        "test_ghist_gsum{first=\"1\",second=\"2\"} 0.8\n"
        "test_ghist_gcount{first=\"1\",second=\"2\"} 1\n"
        "# EOF\n"));
    result.clear();
    h.with_labels({"3", "4"}).observe(1.0);
    serialize<format::openmetrics_1_0>(reg, result);
    // Order is not guaranteed
    std::size_t header_pos = result.find("# TYPE test_ghist gaugehistogram\n");
    BOOST_TEST(header_pos != std::string::npos);
    std::size_t metric_pos = result.find(
        "test_ghist_bucket{first=\"1\",le=\"0.005\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.01\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.025\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.05\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.1\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.25\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.5\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"1\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"2.5\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"5\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"10\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"+Inf\",second=\"2\"} 1\n"
        "test_ghist_gsum{first=\"1\",second=\"2\"} 0.8\n"
        "test_ghist_gcount{first=\"1\",second=\"2\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find(
        "test_ghist_bucket{first=\"3\",le=\"0.005\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.01\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.025\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.05\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.1\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.25\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.5\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"1\",second=\"4\"} 1\n"
        "test_ghist_bucket{first=\"3\",le=\"2.5\",second=\"4\"} 1\n"
        "test_ghist_bucket{first=\"3\",le=\"5\",second=\"4\"} 1\n"
        "test_ghist_bucket{first=\"3\",le=\"10\",second=\"4\"} 1\n"
        "test_ghist_bucket{first=\"3\",le=\"+Inf\",second=\"4\"} 1\n"
        "test_ghist_gsum{first=\"3\",second=\"4\"} 1\n"
        "test_ghist_gcount{first=\"3\",second=\"4\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.substr(result.size() - 6u, 6u) == std::string_view(
        "# EOF\n"
    ));
    BOOST_TEST(result.size() == 33u + 699u + 697u + 6u);
    result.clear();
    // Multiple gaugehistogram_vecs
    gaugehistogram_vec<AtomicPolicyType> h2("test_ghist2",
        {.help = "help text", .unit = "", .buckets = {0.1, 0.2, 0.3}},
        {"third", "fourth"});
    gaugehistogram_vec<AtomicPolicyType> h3("temperature_dist_celsius",
        {.help = "", .unit = "celsius", .buckets = {}},
        {"region"});
    h2.with_labels({"3", "4"}).observe(0.01);
    h3.with_labels({"42"}).observe(0.5);
    reg.register_metric(h2);
    reg.register_metric(h3);
    serialize<format::openmetrics_1_0>(reg, result);
    header_pos = result.find("# TYPE test_ghist gaugehistogram\n");
    BOOST_TEST(header_pos != std::string::npos);
    metric_pos = result.find(
        "test_ghist_bucket{first=\"1\",le=\"0.005\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.01\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.025\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.05\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.1\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.25\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"0.5\",second=\"2\"} 0\n"
        "test_ghist_bucket{first=\"1\",le=\"1\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"2.5\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"5\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"10\",second=\"2\"} 1\n"
        "test_ghist_bucket{first=\"1\",le=\"+Inf\",second=\"2\"} 1\n"
        "test_ghist_gsum{first=\"1\",second=\"2\"} 0.8\n"
        "test_ghist_gcount{first=\"1\",second=\"2\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find(
        "test_ghist_bucket{first=\"3\",le=\"0.005\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.01\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.025\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.05\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.1\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.25\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"0.5\",second=\"4\"} 0\n"
        "test_ghist_bucket{first=\"3\",le=\"1\",second=\"4\"} 1\n"
        "test_ghist_bucket{first=\"3\",le=\"2.5\",second=\"4\"} 1\n"
        "test_ghist_bucket{first=\"3\",le=\"5\",second=\"4\"} 1\n"
        "test_ghist_bucket{first=\"3\",le=\"10\",second=\"4\"} 1\n"
        "test_ghist_bucket{first=\"3\",le=\"+Inf\",second=\"4\"} 1\n"
        "test_ghist_gsum{first=\"3\",second=\"4\"} 1\n"
        "test_ghist_gcount{first=\"3\",second=\"4\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.find(
        "# TYPE test_ghist2 gaugehistogram\n"
        "# HELP test_ghist2 help text\n"
        "test_ghist2_bucket{fourth=\"4\",le=\"0.1\",third=\"3\"} 1\n"
        "test_ghist2_bucket{fourth=\"4\",le=\"0.2\",third=\"3\"} 1\n"
        "test_ghist2_bucket{fourth=\"4\",le=\"0.3\",third=\"3\"} 1\n"
        "test_ghist2_bucket{fourth=\"4\",le=\"+Inf\",third=\"3\"} 1\n"
        "test_ghist2_gsum{fourth=\"4\",third=\"3\"} 0.01\n"
        "test_ghist2_gcount{fourth=\"4\",third=\"3\"} 1\n") != std::string::npos);
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.find(
        "# TYPE temperature_dist_celsius gaugehistogram\n"
        "# UNIT temperature_dist_celsius celsius\n"
        "temperature_dist_celsius_bucket{le=\"0.005\",region=\"42\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.01\",region=\"42\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.025\",region=\"42\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.05\",region=\"42\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.1\",region=\"42\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.25\",region=\"42\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.5\",region=\"42\"} 1\n"
        "temperature_dist_celsius_bucket{le=\"1\",region=\"42\"} 1\n"
        "temperature_dist_celsius_bucket{le=\"2.5\",region=\"42\"} 1\n"
        "temperature_dist_celsius_bucket{le=\"5\",region=\"42\"} 1\n"
        "temperature_dist_celsius_bucket{le=\"10\",region=\"42\"} 1\n"
        "temperature_dist_celsius_bucket{le=\"+Inf\",region=\"42\"} 1\n"
        "temperature_dist_celsius_gsum{region=\"42\"} 0.5\n"
        "temperature_dist_celsius_gcount{region=\"42\"} 1\n") != std::string::npos);
    BOOST_TEST(result.substr(result.size() - 6u, 6u) == std::string_view(
        "# EOF\n"
    ));
    BOOST_TEST(result.size() == 33u + 699u + 697u + 359u + 856u + 6u);
}

template <typename AtomicPolicyType>
void test_info_vec_prometheus_0_0_4()
{
    registry<AtomicPolicyType> reg;
    info_vec<AtomicPolicyType> in("build", {"version", "component"});
    reg.register_metric(in);

    std::string result;
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result.empty());
    result.clear();
    in.with_labels({"0.1", "auth"});
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE build_info gauge\n"
        "build_info{component=\"auth\",version=\"0.1\"} 1\n"));
    result.clear();
    in.with_labels({"0.1", "session"});
    in.with_labels({"0.3", "profile"});
    serialize<format::prometheus_0_0_4>(reg, result);
    // Order is not guaranteed
    std::size_t header_pos = result.find("# TYPE build_info gauge\n");
    BOOST_TEST(header_pos != std::string::npos);
    std::size_t metric_pos = result.find("build_info{component=\"auth\",version=\"0.1\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("build_info{component=\"session\",version=\"0.1\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("build_info{component=\"profile\",version=\"0.3\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.size() == 24u + 45u + 48u + 48u);
}

template <typename AtomicPolicyType>
void test_info_vec_openmetrics_1_0()
{
    registry<AtomicPolicyType> reg;
    info_vec<AtomicPolicyType> in("build", {"version", "component"});
    reg.register_metric(in);

    std::string result;
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == "# EOF\n");
    result.clear();
    in.with_labels({"0.1", "auth"});
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# TYPE build info\n"
        "build_info{component=\"auth\",version=\"0.1\"} 1\n"
        "# EOF\n"));
    result.clear();
    in.with_labels({"0.1", "session"});
    in.with_labels({"0.3", "profile"});
    serialize<format::openmetrics_1_0>(reg, result);
    // Order is not guaranteed
    std::size_t header_pos = result.find("# TYPE build info\n");
    BOOST_TEST(header_pos != std::string::npos);
    std::size_t metric_pos = result.find("build_info{component=\"auth\",version=\"0.1\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("build_info{component=\"session\",version=\"0.1\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    metric_pos = result.find("build_info{component=\"profile\",version=\"0.3\"} 1\n");
    BOOST_TEST(metric_pos > header_pos);
    BOOST_TEST(result.substr(result.size() - 6u, 6u) == std::string_view(
        "# EOF\n"
    ));
    BOOST_TEST(result.size() == 18u + 45u + 48u + 48u + 6u);
}

template <typename AtomicPolicyType, detail::string_buffer Suffix>
void test_static_counter_prometheus_0_0_4()
{
    using static_reg = static_registry<"counters_reg" + Suffix, AtomicPolicyType>;

    using c = static_counter<"http_requests" + Suffix, AtomicPolicyType>;
    static_reg::template register_metric<c>();

    std::string result;
    serialize<format::prometheus_0_0_4, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests"+ Suffix +"_total counter\n"
        "http_requests"+ Suffix +"_total 0\n"));
    result.clear();
    c::inc();
    serialize<format::prometheus_0_0_4, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests"+ Suffix +"_total counter\n"
        "http_requests"+ Suffix +"_total 1\n"));
    result.clear();
    c::inc();
    serialize<format::prometheus_0_0_4, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests"+ Suffix +"_total counter\n"
        "http_requests"+ Suffix +"_total 2\n"));
    result.clear();
    c::inc(0.5);
    serialize<format::prometheus_0_0_4, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests"+ Suffix +"_total counter\n"
        "http_requests"+ Suffix +"_total 2.5\n"));
    result.clear();
    c::inc(5.5);
    serialize<format::prometheus_0_0_4, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests"+ Suffix +"_total counter\n"
        "http_requests"+ Suffix +"_total 8\n"));
    // Multiple counters
    using static_reg_mul = static_registry<"multiple_counters_reg" + Suffix, AtomicPolicyType>;
    using c2 = static_counter<"http_requests2" + Suffix, help<"help text">, AtomicPolicyType>;
    using c3 = static_counter<"test_c"+ Suffix +"_seconds", unit<"seconds">, AtomicPolicyType>;
    c3::inc();
    static_reg_mul::template register_metric<c>();
    static_reg_mul::template register_metric<c2>();
    static_reg_mul::template register_metric<c3>();
    result.clear();
    serialize<format::prometheus_0_0_4, static_reg_mul>(result);
    // Order is guaranteed
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests"+ Suffix +"_total counter\n"
        "http_requests"+ Suffix +"_total 8\n"
        "# TYPE http_requests2"+ Suffix +"_total counter\n"
        "# HELP http_requests2"+ Suffix +"_total help text\n"
        "http_requests2"+ Suffix +"_total 0\n"
        "# TYPE test_c"+ Suffix +"_seconds_total counter\n"
        "test_c"+ Suffix +"_seconds_total 1\n"));
    c2::inc();
}

template <typename AtomicPolicyType, detail::string_buffer Suffix>
void test_static_counter_openmetrics_1_0()
{
    using static_reg = static_registry<"counters_reg_om" + Suffix, AtomicPolicyType>;

    using c = static_counter<"http_requests_om" + Suffix, AtomicPolicyType>;
    static_reg::template register_metric<c>();

    std::string result;
    serialize<format::openmetrics_1_0, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests_om"+ Suffix +" counter\n"
        "http_requests_om"+ Suffix +"_total 0\n"
        "# EOF\n"));
    result.clear();
    c::inc();
    serialize<format::openmetrics_1_0, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests_om"+ Suffix +" counter\n"
        "http_requests_om"+ Suffix +"_total 1\n"
        "# EOF\n"));
    result.clear();
    c::inc();
    serialize<format::openmetrics_1_0, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests_om"+ Suffix +" counter\n"
        "http_requests_om"+ Suffix +"_total 2\n"
        "# EOF\n"));
    result.clear();
    c::inc(0.5);
    serialize<format::openmetrics_1_0, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests_om"+ Suffix +" counter\n"
        "http_requests_om"+ Suffix +"_total 2.5\n"
        "# EOF\n"));
    result.clear();
    c::inc(5.5);
    serialize<format::openmetrics_1_0, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests_om"+ Suffix +" counter\n"
        "http_requests_om"+ Suffix +"_total 8\n"
        "# EOF\n"));
    // Multiple counters
    using static_reg_mul = static_registry<"multiple_counters_reg_om" + Suffix, AtomicPolicyType>;
    using c2 = static_counter<"http_requests2_om" + Suffix, help<"help text">, AtomicPolicyType>;
    using c3 = static_counter<"test_c_om"+ Suffix +"_seconds", unit<"seconds">, AtomicPolicyType>;
    c3::inc();
    static_reg_mul::template register_metric<c>();
    static_reg_mul::template register_metric<c2>();
    static_reg_mul::template register_metric<c3>();
    result.clear();
    serialize<format::openmetrics_1_0, static_reg_mul>(result);
    // Order is guaranteed
    BOOST_TEST(result == std::string_view(
        "# TYPE http_requests_om"+ Suffix +" counter\n"
        "http_requests_om"+ Suffix +"_total 8\n"
        "# TYPE http_requests2_om"+ Suffix +" counter\n"
        "# HELP http_requests2_om"+ Suffix +" help text\n"
        "http_requests2_om"+ Suffix +"_total 0\n"
        "# TYPE test_c_om"+ Suffix +"_seconds counter\n"
        "# UNIT test_c_om"+ Suffix +"_seconds seconds\n"
        "test_c_om"+ Suffix +"_seconds_total 1\n"
        "# EOF\n"));
    c2::inc();
}

template <typename AtomicPolicyType, detail::string_buffer Suffix>
void test_static_gauge_prometheus_0_0_4()
{
    using static_reg = static_registry<"gauges_reg" + Suffix, AtomicPolicyType>;

    using g = static_gauge<"test_g_prom" + Suffix, AtomicPolicyType>;
    static_reg::template register_metric<g>();

    std::string result;
    serialize<format::prometheus_0_0_4, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_g_prom"+ Suffix +" gauge\n"
        "test_g_prom"+ Suffix +" 0\n"));
    result.clear();
    g::inc();
    serialize<format::prometheus_0_0_4, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_g_prom"+ Suffix +" gauge\n"
        "test_g_prom"+ Suffix +" 1\n"));
    result.clear();
    g::set(5);
    serialize<format::prometheus_0_0_4, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_g_prom"+ Suffix +" gauge\n"
        "test_g_prom"+ Suffix +" 5\n"));
    result.clear();
    g::inc(0.5);
    serialize<format::prometheus_0_0_4, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_g_prom"+ Suffix +" gauge\n"
        "test_g_prom"+ Suffix +" 5.5\n"));
    result.clear();
    g::inc(5.5);
    serialize<format::prometheus_0_0_4, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_g_prom"+ Suffix +" gauge\n"
        "test_g_prom"+ Suffix +" 11\n"));
    // Multiple gauges
    using static_reg_mul = static_registry<"multiple_gauges_reg" + Suffix, AtomicPolicyType>;
    using g2 = static_gauge<"test_g_prom2" + Suffix, help<"help text">, AtomicPolicyType>;
    using g3 = static_gauge<"test_g"+ Suffix +"_seconds", unit<"seconds">, AtomicPolicyType>;
    g3::inc();
    static_reg_mul::template register_metric<g>();
    static_reg_mul::template register_metric<g2>();
    static_reg_mul::template register_metric<g3>();
    result.clear();
    serialize<format::prometheus_0_0_4, static_reg_mul>(result);
    // Order is guaranteed
    BOOST_TEST(result == std::string_view(
        "# TYPE test_g_prom"+ Suffix +" gauge\n"
        "test_g_prom"+ Suffix +" 11\n"
        "# TYPE test_g_prom2"+ Suffix +" gauge\n"
        "# HELP test_g_prom2"+ Suffix +" help text\n"
        "test_g_prom2"+ Suffix +" 0\n"
        "# TYPE test_g"+ Suffix +"_seconds gauge\n"
        "test_g"+ Suffix +"_seconds 1\n"));
    g2::inc();
}

template <typename AtomicPolicyType, detail::string_buffer Suffix>
void test_static_gauge_openmetrics_1_0()
{
    using static_reg = static_registry<"gauges_reg_om" + Suffix, AtomicPolicyType>;

    using g = static_gauge<"test_g_om" + Suffix, AtomicPolicyType>;
    static_reg::template register_metric<g>();

    std::string result;
    serialize<format::openmetrics_1_0, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_g_om"+ Suffix +" gauge\n"
        "test_g_om"+ Suffix +" 0\n"
        "# EOF\n"));
    result.clear();
    g::inc();
    serialize<format::openmetrics_1_0, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_g_om"+ Suffix +" gauge\n"
        "test_g_om"+ Suffix +" 1\n"
        "# EOF\n"));
    result.clear();
    g::set(5);
    serialize<format::openmetrics_1_0, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_g_om"+ Suffix +" gauge\n"
        "test_g_om"+ Suffix +" 5\n"
        "# EOF\n"));
    result.clear();
    g::inc(0.5);
    serialize<format::openmetrics_1_0, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_g_om"+ Suffix +" gauge\n"
        "test_g_om"+ Suffix +" 5.5\n"
        "# EOF\n"));
    result.clear();
    g::inc(5.5);
    serialize<format::openmetrics_1_0, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_g_om"+ Suffix +" gauge\n"
        "test_g_om"+ Suffix +" 11\n"
        "# EOF\n"));
    // Multiple gauges
    using static_reg_mul = static_registry<"multiple_gauges_reg_om" + Suffix, AtomicPolicyType>;
    using g2 = static_gauge<"test_g_om2" + Suffix, help<"help text">, AtomicPolicyType>;
    using g3 = static_gauge<"test_g_om"+ Suffix +"_seconds", unit<"seconds">, labels<"first">, AtomicPolicyType>;
    g3::template inc<"1">();
    static_reg_mul::template register_metric<g>();
    static_reg_mul::template register_metric<g2>();
    static_reg_mul::template register_metric<g3>();
    result.clear();
    serialize<format::openmetrics_1_0, static_reg_mul>(result);
    // Order is guaranteed
    BOOST_TEST(result == std::string_view(
        "# TYPE test_g_om"+ Suffix +" gauge\n"
        "test_g_om"+ Suffix +" 11\n"
        "# TYPE test_g_om2"+ Suffix +" gauge\n"
        "# HELP test_g_om2"+ Suffix +" help text\n"
        "test_g_om2"+ Suffix +" 0\n"
        "# TYPE test_g_om"+ Suffix +"_seconds gauge\n"
        "# UNIT test_g_om"+ Suffix +"_seconds seconds\n"
        "test_g_om"+ Suffix +"_seconds{first=\"1\"} 1\n"
        "# EOF\n"));
    g2::inc();
}

template <typename AtomicPolicyType, detail::string_buffer Suffix>
void test_static_histogram_prometheus_0_0_4()
{
    using static_reg = static_registry<"histograms_reg" + Suffix, AtomicPolicyType>;

    using h = static_histogram<"test_h_prom" + Suffix, AtomicPolicyType>;
    static_reg::template register_metric<h>();

    std::string result;
    serialize<format::prometheus_0_0_4, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_h_prom"+ Suffix +" histogram\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.005\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.01\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.025\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.05\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.1\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.25\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.5\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"1\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"2.5\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"5\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"10\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"+Inf\"} 0\n"
        "test_h_prom"+ Suffix +"_sum 0\n"
        "test_h_prom"+ Suffix +"_count 0\n"));
    result.clear();
    h::observe(0.1);
    serialize<format::prometheus_0_0_4, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_h_prom"+ Suffix +" histogram\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.005\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.01\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.025\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.05\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.1\"} 1\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.25\"} 1\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.5\"} 1\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"1\"} 1\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"2.5\"} 1\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"5\"} 1\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"10\"} 1\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"+Inf\"} 1\n"
        "test_h_prom"+ Suffix +"_sum 0.1\n"
        "test_h_prom"+ Suffix +"_count 1\n"));
    result.clear();
    h::observe(0.2);
    h::observe(0.2);
    serialize<format::prometheus_0_0_4, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_h_prom"+ Suffix +" histogram\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.005\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.01\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.025\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.05\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.1\"} 1\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.25\"} 3\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.5\"} 3\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"1\"} 3\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"2.5\"} 3\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"5\"} 3\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"10\"} 3\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"+Inf\"} 3\n"
        "test_h_prom"+ Suffix +"_sum 0.5\n"
        "test_h_prom"+ Suffix +"_count 3\n"));
    // Multiple histograms
    using static_reg_mul = static_registry<"multiple_histograms_reg" + Suffix, AtomicPolicyType>;
    using h2 = static_histogram<"test_h_prom2" + Suffix, help<"help text">, AtomicPolicyType>;
    using h3 = static_histogram<"test_h"+ Suffix +"_seconds",
        unit<"seconds">, buckets<0.1, 0.2, 0.3, 0.4>, labels<"second">, AtomicPolicyType>;
    h3::template observe<"2">(0.1);
    static_reg_mul::template register_metric<h>();
    static_reg_mul::template register_metric<h2>();
    static_reg_mul::template register_metric<h3>();
    result.clear();
    serialize<format::prometheus_0_0_4, static_reg_mul>(result);
    // Order is guaranteed
    BOOST_TEST(result == std::string_view(
        "# TYPE test_h_prom"+ Suffix +" histogram\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.005\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.01\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.025\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.05\"} 0\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.1\"} 1\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.25\"} 3\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"0.5\"} 3\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"1\"} 3\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"2.5\"} 3\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"5\"} 3\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"10\"} 3\n"
        "test_h_prom"+ Suffix +"_bucket{le=\"+Inf\"} 3\n"
        "test_h_prom"+ Suffix +"_sum 0.5\n"
        "test_h_prom"+ Suffix +"_count 3\n"
        "# TYPE test_h_prom2"+ Suffix +" histogram\n"
        "# HELP test_h_prom2"+ Suffix +" help text\n"
        "test_h_prom2"+ Suffix +"_bucket{le=\"0.005\"} 0\n"
        "test_h_prom2"+ Suffix +"_bucket{le=\"0.01\"} 0\n"
        "test_h_prom2"+ Suffix +"_bucket{le=\"0.025\"} 0\n"
        "test_h_prom2"+ Suffix +"_bucket{le=\"0.05\"} 0\n"
        "test_h_prom2"+ Suffix +"_bucket{le=\"0.1\"} 0\n"
        "test_h_prom2"+ Suffix +"_bucket{le=\"0.25\"} 0\n"
        "test_h_prom2"+ Suffix +"_bucket{le=\"0.5\"} 0\n"
        "test_h_prom2"+ Suffix +"_bucket{le=\"1\"} 0\n"
        "test_h_prom2"+ Suffix +"_bucket{le=\"2.5\"} 0\n"
        "test_h_prom2"+ Suffix +"_bucket{le=\"5\"} 0\n"
        "test_h_prom2"+ Suffix +"_bucket{le=\"10\"} 0\n"
        "test_h_prom2"+ Suffix +"_bucket{le=\"+Inf\"} 0\n"
        "test_h_prom2"+ Suffix +"_sum 0\n"
        "test_h_prom2"+ Suffix +"_count 0\n"
        "# TYPE test_h"+ Suffix +"_seconds histogram\n"
        "test_h"+ Suffix +"_seconds_bucket{le=\"0.1\",second=\"2\"} 1\n"
        "test_h"+ Suffix +"_seconds_bucket{le=\"0.2\",second=\"2\"} 1\n"
        "test_h"+ Suffix +"_seconds_bucket{le=\"0.3\",second=\"2\"} 1\n"
        "test_h"+ Suffix +"_seconds_bucket{le=\"0.4\",second=\"2\"} 1\n"
        "test_h"+ Suffix +"_seconds_bucket{le=\"+Inf\",second=\"2\"} 1\n"
        "test_h"+ Suffix +"_seconds_sum{second=\"2\"} 0.1\n"
        "test_h"+ Suffix +"_seconds_count{second=\"2\"} 1\n"));
    h2::observe(0.1);
}

template <typename AtomicPolicyType, detail::string_buffer Suffix>
void test_static_histogram_openmetrics_1_0()
{
    using static_reg = static_registry<"histograms_reg_om" + Suffix, AtomicPolicyType>;

    using h = static_histogram<"test_h_om" + Suffix, AtomicPolicyType>;
    static_reg::template register_metric<h>();

    std::string result;
    serialize<format::openmetrics_1_0, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_h_om"+ Suffix +" histogram\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.005\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.01\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.025\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.05\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.1\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.25\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.5\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"1\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"2.5\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"5\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"10\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"+Inf\"} 0\n"
        "test_h_om"+ Suffix +"_sum 0\n"
        "test_h_om"+ Suffix +"_count 0\n"
        "# EOF\n"));
    result.clear();
    h::observe(0.1);
    serialize<format::openmetrics_1_0, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_h_om"+ Suffix +" histogram\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.005\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.01\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.025\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.05\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.1\"} 1\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.25\"} 1\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.5\"} 1\n"
        "test_h_om"+ Suffix +"_bucket{le=\"1\"} 1\n"
        "test_h_om"+ Suffix +"_bucket{le=\"2.5\"} 1\n"
        "test_h_om"+ Suffix +"_bucket{le=\"5\"} 1\n"
        "test_h_om"+ Suffix +"_bucket{le=\"10\"} 1\n"
        "test_h_om"+ Suffix +"_bucket{le=\"+Inf\"} 1\n"
        "test_h_om"+ Suffix +"_sum 0.1\n"
        "test_h_om"+ Suffix +"_count 1\n"
        "# EOF\n"));
    result.clear();
    h::observe(0.2);
    h::observe(0.2);
    serialize<format::openmetrics_1_0, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_h_om"+ Suffix +" histogram\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.005\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.01\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.025\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.05\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.1\"} 1\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.25\"} 3\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.5\"} 3\n"
        "test_h_om"+ Suffix +"_bucket{le=\"1\"} 3\n"
        "test_h_om"+ Suffix +"_bucket{le=\"2.5\"} 3\n"
        "test_h_om"+ Suffix +"_bucket{le=\"5\"} 3\n"
        "test_h_om"+ Suffix +"_bucket{le=\"10\"} 3\n"
        "test_h_om"+ Suffix +"_bucket{le=\"+Inf\"} 3\n"
        "test_h_om"+ Suffix +"_sum 0.5\n"
        "test_h_om"+ Suffix +"_count 3\n"
        "# EOF\n"));
    // Multiple histograms
    using static_reg_mul = static_registry<"multiple_histograms_reg_om" + Suffix, AtomicPolicyType>;
    using h2 = static_histogram<"test_h_om2" + Suffix, help<"help text">, AtomicPolicyType>;
    using h3 = static_histogram<"test_h_om"+ Suffix +"_seconds",
        unit<"seconds">, buckets<0.1, 0.2, 0.3, 0.4>, labels<"second">, AtomicPolicyType>;
    h3::template observe<"2">(0.1);
    static_reg_mul::template register_metric<h>();
    static_reg_mul::template register_metric<h2>();
    static_reg_mul::template register_metric<h3>();
    result.clear();
    serialize<format::openmetrics_1_0, static_reg_mul>(result);
    // Order is guaranteed
    BOOST_TEST(result == std::string_view(
        "# TYPE test_h_om"+ Suffix +" histogram\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.005\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.01\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.025\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.05\"} 0\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.1\"} 1\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.25\"} 3\n"
        "test_h_om"+ Suffix +"_bucket{le=\"0.5\"} 3\n"
        "test_h_om"+ Suffix +"_bucket{le=\"1\"} 3\n"
        "test_h_om"+ Suffix +"_bucket{le=\"2.5\"} 3\n"
        "test_h_om"+ Suffix +"_bucket{le=\"5\"} 3\n"
        "test_h_om"+ Suffix +"_bucket{le=\"10\"} 3\n"
        "test_h_om"+ Suffix +"_bucket{le=\"+Inf\"} 3\n"
        "test_h_om"+ Suffix +"_sum 0.5\n"
        "test_h_om"+ Suffix +"_count 3\n"
        "# TYPE test_h_om2"+ Suffix +" histogram\n"
        "# HELP test_h_om2"+ Suffix +" help text\n"
        "test_h_om2"+ Suffix +"_bucket{le=\"0.005\"} 0\n"
        "test_h_om2"+ Suffix +"_bucket{le=\"0.01\"} 0\n"
        "test_h_om2"+ Suffix +"_bucket{le=\"0.025\"} 0\n"
        "test_h_om2"+ Suffix +"_bucket{le=\"0.05\"} 0\n"
        "test_h_om2"+ Suffix +"_bucket{le=\"0.1\"} 0\n"
        "test_h_om2"+ Suffix +"_bucket{le=\"0.25\"} 0\n"
        "test_h_om2"+ Suffix +"_bucket{le=\"0.5\"} 0\n"
        "test_h_om2"+ Suffix +"_bucket{le=\"1\"} 0\n"
        "test_h_om2"+ Suffix +"_bucket{le=\"2.5\"} 0\n"
        "test_h_om2"+ Suffix +"_bucket{le=\"5\"} 0\n"
        "test_h_om2"+ Suffix +"_bucket{le=\"10\"} 0\n"
        "test_h_om2"+ Suffix +"_bucket{le=\"+Inf\"} 0\n"
        "test_h_om2"+ Suffix +"_sum 0\n"
        "test_h_om2"+ Suffix +"_count 0\n"
        "# TYPE test_h_om"+ Suffix +"_seconds histogram\n"
        "# UNIT test_h_om"+ Suffix +"_seconds seconds\n"
        "test_h_om"+ Suffix +"_seconds_bucket{le=\"0.1\",second=\"2\"} 1\n"
        "test_h_om"+ Suffix +"_seconds_bucket{le=\"0.2\",second=\"2\"} 1\n"
        "test_h_om"+ Suffix +"_seconds_bucket{le=\"0.3\",second=\"2\"} 1\n"
        "test_h_om"+ Suffix +"_seconds_bucket{le=\"0.4\",second=\"2\"} 1\n"
        "test_h_om"+ Suffix +"_seconds_bucket{le=\"+Inf\",second=\"2\"} 1\n"
        "test_h_om"+ Suffix +"_seconds_sum{second=\"2\"} 0.1\n"
        "test_h_om"+ Suffix +"_seconds_count{second=\"2\"} 1\n"
        "# EOF\n"));
    h2::observe(0.1);
}

template <typename AtomicPolicyType, detail::string_buffer Suffix>
void test_static_info_prometheus_0_0_4()
{
    using static_reg = static_registry<"info_reg" + Suffix, AtomicPolicyType>;

    using in = static_info<"test_in_prom" + Suffix, AtomicPolicyType>;
    in::create();
    static_reg::template register_metric<in>();

    std::string result;
    serialize<format::prometheus_0_0_4, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# TYPE test_in_prom"+ Suffix +"_info gauge\n"
        "test_in_prom"+ Suffix +"_info 1\n"));
    result.clear();
    // Multiple infos
    using static_reg_mul = static_registry<"multiple_infos_reg" + Suffix, AtomicPolicyType>;
    using in2 = static_info<"test_in_prom2" + Suffix, help<"help text">, labels<"version", "compiler">, AtomicPolicyType>;
    using in3 = static_info<"build"+ Suffix, labels<"version">, AtomicPolicyType>;
    in2::template create<"0.1", "g++">();
    in3::template create<"0.2">();
    static_reg_mul::template register_metric<in>();
    static_reg_mul::template register_metric<in2>();
    static_reg_mul::template register_metric<in3>();
    result.clear();
    serialize<format::prometheus_0_0_4, static_reg_mul>(result);
    // Order is guaranteed
    BOOST_TEST(result == std::string_view(
        "# TYPE test_in_prom"+ Suffix +"_info gauge\n"
        "test_in_prom"+ Suffix +"_info 1\n"
        "# TYPE test_in_prom2"+ Suffix +"_info gauge\n"
        "# HELP test_in_prom2"+ Suffix +"_info help text\n"
        "test_in_prom2"+ Suffix +"_info{compiler=\"g++\",version=\"0.1\"} 1\n"
        "# TYPE build"+ Suffix +"_info gauge\n"
        "build"+ Suffix +"_info{version=\"0.2\"} 1\n"));
}

BOOST_AUTO_TEST_SUITE(test_serialize)

BOOST_AUTO_TEST_CASE(empty_prometheus_0_0_4)
{
    registry reg;
    std::string result;
    serialize<format::prometheus_0_0_4>(reg, result);
    BOOST_TEST(result.empty());

    using static_reg = static_registry<"empty_reg">;
    serialize<format::prometheus_0_0_4, static_reg>(result);
    BOOST_TEST(result.empty());
}

BOOST_AUTO_TEST_CASE(empty_openmetrics_1_0)
{
    registry reg;
    std::string result;
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result == std::string_view(
        "# EOF\n"
    ));
    result.clear();
    using static_reg = static_registry<"empty_reg">;
    serialize<format::openmetrics_1_0, static_reg>(result);
    BOOST_TEST(result == std::string_view(
        "# EOF\n"
    ));
    result.clear();
    serialize<format::openmetrics_1_0, static_reg, false>(result);
    BOOST_TEST(result.empty());
}

BOOST_AUTO_TEST_CASE(counter_prometheus_0_0_4)
{
    test_counter_prometheus_0_0_4<atomic_policy<true>>();
}

BOOST_AUTO_TEST_CASE(counter_non_atomic_prometheus_0_0_4)
{
    test_counter_prometheus_0_0_4<atomic_policy<false>>();
}

BOOST_AUTO_TEST_CASE(counter_openmetrics_1_0)
{
    test_counter_openmetrics_1_0<atomic_policy<true>>();
}

BOOST_AUTO_TEST_CASE(counter_non_atomic_openmetrics_1_0)
{
    test_counter_openmetrics_1_0<atomic_policy<false>>();
}

BOOST_AUTO_TEST_CASE(counter_vec_prometheus_0_0_4)
{
    test_counter_vec_prometheus_0_0_4<atomic_policy<true>>();
}

BOOST_AUTO_TEST_CASE(counter_vec_non_atomic_prometheus_0_0_4)
{
    test_counter_vec_prometheus_0_0_4<atomic_policy<false>>();
}

BOOST_AUTO_TEST_CASE(counter_vec_openmetrics_1_0)
{
    test_counter_vec_openmetrics_1_0<atomic_policy<true>>();
}

BOOST_AUTO_TEST_CASE(counter_vec_non_atomic_openmetrics_1_0)
{
    test_counter_vec_openmetrics_1_0<atomic_policy<false>>();
}

BOOST_AUTO_TEST_CASE(gauge_prometheus_0_0_4)
{
    test_gauge_prometheus_0_0_4<atomic_policy<true>>();
}

BOOST_AUTO_TEST_CASE(gauge_non_atomic_prometheus_0_0_4)
{
    test_gauge_prometheus_0_0_4<atomic_policy<false>>();
}

BOOST_AUTO_TEST_CASE(gauge_openmetrics_1_0)
{
    test_gauge_openmetrics_1_0<atomic_policy<true>>();
}

BOOST_AUTO_TEST_CASE(gauge_non_atomic_openmetrics_1_0)
{
    test_gauge_openmetrics_1_0<atomic_policy<false>>();
}

BOOST_AUTO_TEST_CASE(gauge_vec_prometheus_0_0_4)
{
    test_gauge_vec_prometheus_0_0_4<atomic_policy<true>>();
}

BOOST_AUTO_TEST_CASE(gauge_vec_non_atomic_prometheus_0_0_4)
{
    test_gauge_vec_prometheus_0_0_4<atomic_policy<false>>();
}

BOOST_AUTO_TEST_CASE(gauge_vec_openmetrics_1_0)
{
    test_gauge_vec_openmetrics_1_0<atomic_policy<true>>();
}

BOOST_AUTO_TEST_CASE(gauge_vec_non_atomic_openmetrics_1_0)
{
    test_gauge_vec_openmetrics_1_0<atomic_policy<false>>();
}

BOOST_AUTO_TEST_CASE(histogram_prometheus_0_0_4)
{
    test_histogram_prometheus_0_0_4<atomic_policy<true>>();
}

BOOST_AUTO_TEST_CASE(histogram_non_atomic_prometheus_0_0_4)
{
    test_histogram_prometheus_0_0_4<atomic_policy<false>>();
}

BOOST_AUTO_TEST_CASE(histogram_openmetrics_1_0)
{
    test_histogram_openmetrics_1_0<atomic_policy<true>>();
}

BOOST_AUTO_TEST_CASE(histogram_non_atomic_openmetrics_1_0)
{
    test_histogram_openmetrics_1_0<atomic_policy<false>>();
}

BOOST_AUTO_TEST_CASE(histogram_vec_prometheus_0_0_4)
{
    test_histogram_vec_prometheus_0_0_4<atomic_policy<true>>();
}

BOOST_AUTO_TEST_CASE(histogram_vec_non_atomic_prometheus_0_0_4)
{
    test_histogram_vec_prometheus_0_0_4<atomic_policy<false>>();
}

BOOST_AUTO_TEST_CASE(histogram_vec_openmetrics_1_0)
{
    test_histogram_vec_openmetrics_1_0<atomic_policy<true>>();
}

BOOST_AUTO_TEST_CASE(histogram_vec_non_atomic_openmetrics_1_0)
{
    test_histogram_vec_openmetrics_1_0<atomic_policy<false>>();
}

BOOST_AUTO_TEST_CASE(concurrent_histogram_openmetrics_1_0, * boost::unit_test::tolerance(1e-9))
{
    // Test histogram serialization under concurrent observation
    registry reg;
    histogram h("test_histogram");
    reg.register_metric(h);

    constexpr std::size_t num_threads = 4u;
    constexpr std::size_t iterations_per_thread = 1000u;

    std::atomic<std::size_t> ready_count{0u};
    std::atomic<bool> start_flag{false};

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (std::size_t i = 0u; i < num_threads; i++)
    {
        threads.emplace_back([&]()
        {
            ready_count.fetch_add(1u, std::memory_order_release);

            while (!start_flag.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }

            for (std::size_t j = 0u; j < iterations_per_thread; j++)
            {
                h.observe(0.1);
            }
        });
    }

    while (ready_count.load(std::memory_order_acquire) < num_threads)
    {
        std::this_thread::yield();
    }

    start_flag.store(true, std::memory_order_release);
    // Serialize while other threads are observing
    for (std::size_t i = 0u; i < iterations_per_thread; i++)
    {
        std::string result;
        serialize<format::openmetrics_1_0>(reg, result);
    }

    for (auto& t : threads)
    {
        t.join();
    }

    std::string result;
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result.starts_with(
        "# TYPE test_histogram histogram\n"
        "test_histogram_bucket{le=\"0.005\"} 0\n"
        "test_histogram_bucket{le=\"0.01\"} 0\n"
        "test_histogram_bucket{le=\"0.025\"} 0\n"
        "test_histogram_bucket{le=\"0.05\"} 0\n"
        "test_histogram_bucket{le=\"0.1\"} 4000\n"
        "test_histogram_bucket{le=\"0.25\"} 4000\n"
        "test_histogram_bucket{le=\"0.5\"} 4000\n"
        "test_histogram_bucket{le=\"1\"} 4000\n"
        "test_histogram_bucket{le=\"2.5\"} 4000\n"
        "test_histogram_bucket{le=\"5\"} 4000\n"
        "test_histogram_bucket{le=\"10\"} 4000\n"
        "test_histogram_bucket{le=\"+Inf\"} 4000\n"));
    BOOST_TEST(result.ends_with(
        "test_histogram_count 4000\n"
         "# EOF\n"));
    double sum = extract_double_from_string(result, "test_histogram_sum ");
    BOOST_TEST(!std::isnan(sum));
    BOOST_TEST(sum == 400.0);
}

BOOST_AUTO_TEST_CASE(gaugehistogram_prometheus_0_0_4)
{
    test_gaugehistogram_prometheus_0_0_4<atomic_policy<true>>();
}

BOOST_AUTO_TEST_CASE(gaugehistogram_non_atomic_prometheus_0_0_4)
{
    test_gaugehistogram_prometheus_0_0_4<atomic_policy<false>>();
}

BOOST_AUTO_TEST_CASE(gaugehistogram_openmetrics_1_0)
{
    test_gaugehistogram_openmetrics_1_0<atomic_policy<true>>();
}

BOOST_AUTO_TEST_CASE(gaugehistogram_non_atomic_openmetrics_1_0)
{
    test_gaugehistogram_openmetrics_1_0<atomic_policy<false>>();
}

BOOST_AUTO_TEST_CASE(gaugehistogram_vec_prometheus_0_0_4)
{
    test_gaugehistogram_vec_prometheus_0_0_4<atomic_policy<true>>();
}

BOOST_AUTO_TEST_CASE(gaugehistogram_vec_non_atomic_prometheus_0_0_4)
{
    test_gaugehistogram_vec_prometheus_0_0_4<atomic_policy<false>>();
}

BOOST_AUTO_TEST_CASE(gaugehistogram_vec_openmetrics_1_0)
{
    test_gaugehistogram_vec_openmetrics_1_0<atomic_policy<true>>();
}

BOOST_AUTO_TEST_CASE(gaugehistogram_vec_non_atomic_openmetrics_1_0)
{
    test_gaugehistogram_vec_openmetrics_1_0<atomic_policy<false>>();
}

BOOST_AUTO_TEST_CASE(concurrent_gaugehistogram_openmetrics_1_0, * boost::unit_test::tolerance(1e-9))
{
    // Test histogram serialization under concurrent observation
    registry reg;
    gaugehistogram h("temperature_dist_celsius");
    reg.register_metric(h);

    constexpr std::size_t num_observe_threads = 2u;
    constexpr std::size_t num_sub_threads = 2u;
    constexpr std::size_t iterations_per_thread = 1000u;

    std::atomic<std::size_t> ready_count{0u};
    std::atomic<bool> start_flag{false};

    std::vector<std::thread> threads;
    threads.reserve(num_observe_threads + num_sub_threads);

    for (std::size_t i = 0u; i < num_observe_threads; i++)
    {
        threads.emplace_back([&]()
        {
            ready_count.fetch_add(1u, std::memory_order_release);

            while (!start_flag.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }

            for (std::size_t j = 0u; j < iterations_per_thread; j++)
            {
                h.observe(0.2);
            }
        });
    }
    for (std::size_t i = 0u; i < num_sub_threads; i++)
    {
        threads.emplace_back([&]()
        {
            ready_count.fetch_add(1, std::memory_order_release);

            while (!start_flag.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }

            for (std::size_t j = 0u; j < iterations_per_thread; j++)
            {
                h.sub(0.2);
            }
        });
    }

    while (ready_count.load(std::memory_order_acquire) < num_observe_threads + num_sub_threads)
    {
        std::this_thread::yield();
    }

    start_flag.store(true, std::memory_order_release);
    // Serialize while other threads are observing
    for (std::size_t i = 0u; i < iterations_per_thread; i++)
    {
        std::string result;
        serialize<format::openmetrics_1_0>(reg, result);
    }

    for (auto& t : threads)
    {
        t.join();
    }

    std::string result;
    serialize<format::openmetrics_1_0>(reg, result);
    BOOST_TEST(result.starts_with(
        "# TYPE temperature_dist_celsius gaugehistogram\n"
        "temperature_dist_celsius_bucket{le=\"0.005\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.01\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.025\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.05\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.1\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.25\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"0.5\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"1\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"2.5\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"5\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"10\"} 0\n"
        "temperature_dist_celsius_bucket{le=\"+Inf\"} 0\n"));
    BOOST_TEST(result.ends_with(
        "temperature_dist_celsius_gcount 0\n"
         "# EOF\n"));
    double sum = extract_double_from_string(result, "temperature_dist_celsius_gsum ");
    BOOST_TEST(!std::isnan(sum));
    BOOST_TEST(sum == 0.0);
}

BOOST_AUTO_TEST_CASE(info_vec_prometheus_0_0_4)
{
    test_info_vec_prometheus_0_0_4<atomic_policy<true>>();
}

BOOST_AUTO_TEST_CASE(info_vec_non_atomic_prometheus_0_0_4)
{
    test_info_vec_prometheus_0_0_4<atomic_policy<false>>();
}

BOOST_AUTO_TEST_CASE(info_vec_openmetrics_1_0)
{
    test_info_vec_openmetrics_1_0<atomic_policy<true>>();
}

BOOST_AUTO_TEST_CASE(info_vec_non_atomic_openmetrics_1_0)
{
    test_info_vec_openmetrics_1_0<atomic_policy<false>>();
}

BOOST_AUTO_TEST_CASE(static_counter_prometheus_0_0_4)
{
    test_static_counter_prometheus_0_0_4<atomic_policy<true>, "atom">();
}

BOOST_AUTO_TEST_CASE(static_counter_non_atomic_prometheus_0_0_4)
{
    test_static_counter_prometheus_0_0_4<atomic_policy<false>, "non_atom">();
}

BOOST_AUTO_TEST_CASE(static_counter_openmetrics_1_0)
{
    test_static_counter_openmetrics_1_0<atomic_policy<true>, "atom">();
}

BOOST_AUTO_TEST_CASE(static_counter_non_atomic_openmetrics_1_0)
{
    test_static_counter_openmetrics_1_0<atomic_policy<false>, "non_atom">();
}

BOOST_AUTO_TEST_CASE(static_gauge_prometheus_0_0_4)
{
    test_static_gauge_prometheus_0_0_4<atomic_policy<true>, "atom">();
}

BOOST_AUTO_TEST_CASE(static_gauge_non_atomic_prometheus_0_0_4)
{
    test_static_gauge_prometheus_0_0_4<atomic_policy<false>, "non_atom">();
}

BOOST_AUTO_TEST_CASE(static_gauge_openmetrics_1_0)
{
    test_static_gauge_openmetrics_1_0<atomic_policy<true>, "atom">();
}

BOOST_AUTO_TEST_CASE(static_gauge_non_atomic_openmetrics_1_0)
{
    test_static_gauge_openmetrics_1_0<atomic_policy<false>, "non_atom">();
}

BOOST_AUTO_TEST_CASE(static_histogram_prometheus_0_0_4)
{
    test_static_histogram_prometheus_0_0_4<atomic_policy<true>, "atom">();
}

BOOST_AUTO_TEST_CASE(static_histogram_non_atomic_prometheus_0_0_4)
{
    test_static_histogram_prometheus_0_0_4<atomic_policy<false>, "non_atom">();
}

BOOST_AUTO_TEST_CASE(static_histogram_openmetrics_1_0)
{
    test_static_histogram_openmetrics_1_0<atomic_policy<true>, "atom">();
}

BOOST_AUTO_TEST_CASE(static_histogram_non_atomic_openmetrics_1_0)
{
    test_static_histogram_openmetrics_1_0<atomic_policy<false>, "non_atom">();
}

BOOST_AUTO_TEST_CASE(concurrent_static_histogram_openmetrics_1_0, * boost::unit_test::tolerance(1e-9))
{
    // Test histogram serialization under concurrent observation
    using static_reg = static_registry<"histograms_reg_concurrent">;
    using h = static_histogram<"test_h_concurrent">;
    static_reg::template register_metric<h>();

    constexpr std::size_t num_threads = 4u;
    constexpr std::size_t iterations_per_thread = 1000u;

    std::atomic<std::size_t> ready_count{0u};
    std::atomic<bool> start_flag{false};

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (std::size_t i = 0u; i < num_threads; i++)
    {
        threads.emplace_back([&]()
        {
            ready_count.fetch_add(1u, std::memory_order_release);

            while (!start_flag.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }

            for (std::size_t j = 0u; j < iterations_per_thread; j++)
            {
                h::observe(0.1);
            }
        });
    }

    while (ready_count.load(std::memory_order_acquire) < num_threads)
    {
        std::this_thread::yield();
    }

    start_flag.store(true, std::memory_order_release);
    // Serialize while other threads are observing
    for (std::size_t i = 0u; i < iterations_per_thread; i++)
    {
        std::string result;
        serialize<format::openmetrics_1_0, static_reg>(result);
    }

    for (auto& t : threads)
    {
        t.join();
    }

    std::string result;
    serialize<format::openmetrics_1_0, static_reg>(result);
    BOOST_TEST(result.starts_with(
        "# TYPE test_h_concurrent histogram\n"
        "test_h_concurrent_bucket{le=\"0.005\"} 0\n"
        "test_h_concurrent_bucket{le=\"0.01\"} 0\n"
        "test_h_concurrent_bucket{le=\"0.025\"} 0\n"
        "test_h_concurrent_bucket{le=\"0.05\"} 0\n"
        "test_h_concurrent_bucket{le=\"0.1\"} 4000\n"
        "test_h_concurrent_bucket{le=\"0.25\"} 4000\n"
        "test_h_concurrent_bucket{le=\"0.5\"} 4000\n"
        "test_h_concurrent_bucket{le=\"1\"} 4000\n"
        "test_h_concurrent_bucket{le=\"2.5\"} 4000\n"
        "test_h_concurrent_bucket{le=\"5\"} 4000\n"
        "test_h_concurrent_bucket{le=\"10\"} 4000\n"
        "test_h_concurrent_bucket{le=\"+Inf\"} 4000\n"));
    BOOST_TEST(result.ends_with(
        "test_h_concurrent_count 4000\n"
         "# EOF\n"));
    double sum = extract_double_from_string(result, "test_h_concurrent_sum ");
    BOOST_TEST(!std::isnan(sum));
    BOOST_TEST(sum == 400.0);
}


BOOST_AUTO_TEST_CASE(static_info_prometheus_0_0_4)
{
    test_static_info_prometheus_0_0_4<atomic_policy<true>, "atom">();
}

BOOST_AUTO_TEST_CASE(static_info_non_atomic_prometheus_0_0_4)
{
    test_static_info_prometheus_0_0_4<atomic_policy<false>, "non_atom">();
}

BOOST_AUTO_TEST_SUITE_END()
