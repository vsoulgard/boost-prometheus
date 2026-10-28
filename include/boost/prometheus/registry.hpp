//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_REGISTRY_HPP
#define BOOST_PROMETHEUS_REGISTRY_HPP

#include <boost/prometheus/detail/tuple_unordered_flat_sets.hpp>

#include <boost/prometheus/counter.hpp>
#include <boost/prometheus/counter_vec.hpp>
#include <boost/prometheus/gauge.hpp>
#include <boost/prometheus/gauge_vec.hpp>
#include <boost/prometheus/gaugehistogram.hpp>
#include <boost/prometheus/gaugehistogram_vec.hpp>
#include <boost/prometheus/histogram.hpp>
#include <boost/prometheus/histogram_vec.hpp>
#include <boost/prometheus/info_vec.hpp>
#include <boost/prometheus/metric_options.hpp>

namespace boost::prometheus {

/**
 * \brief Non-owning collection of metrics exposed by an \ref exposer.
 *
 * A `registry` holds pointers to metrics and provides an iteration
 * interface used by the exposer to serialize them. It does **not** own the
 * metrics: the caller must keep each registered metric alive for as long
 * as it remains in the registry.
 *
 * All metrics held by one `registry` must share the same atomic policy as
 * the registry itself. Registering a metric with a different policy is a
 * **compile-time error** (see `register_metric()`).
 *
 * A registry is not copyable and not movable, because it contains
 * synchronization primitives. It is typically constructed once per
 * application (or per exposer) and passed by reference to
 * `exposer::expose()`.
 *
 * \tparam AtomicPolicyType Compile-time atomicity policy. The default,
 *         `default_atomic_policy` (i.e. `atomic_policy<true>`), makes the
 *         registry's storage safe for concurrent iteration and mutation.
 *         With `atomic_policy<false>`, no internal synchronization is
 *         performed — the caller must ensure that registration,
 *         unregistration, iteration, and metric updates do not overlap.
 *
 * \par Lifetime
 * Metrics are stored by pointer. Registering a metric transfers **no**
 * ownership. If a metric is destroyed while still registered, the registry
 * (and any ongoing exposition through it) holds a dangling pointer and the
 * behavior is undefined. Call `unregister_metric()` before destroying a
 * metric, or keep all metrics alive for the lifetime of the registry.
 *
 * \par Thread safety
 * - Metric updates (`inc()`, `observe()`, etc.) are thread-safe when the
 *   atomic policy is enabled, and may be called concurrently with each
 *   other and with serialization through `for_each()`.
 * - `register_metric()` and `unregister_metric()` are intended for startup
 *   and shutdown. They take an internal per-type lock; calling them
 *   concurrently with `for_each()` will briefly block, but is safe.
 *   For best performance, register all metrics before starting the
 *   `io_context`.
 * - `for_each()` locks each type's storage in turn, so it may be called
 *   concurrently from multiple threads (for example, from multiple
 *   exposer connections) without external synchronization.
 *
 * \par Example
 * \code
 * prometheus::registry reg;
 *
 * prometheus::counter requests("requests", {.help = "Number of requests"});
 * prometheus::counter_vec errors("errors",
 *     {.help = "Number of errors"}, {"kind"});
 *
 * errors.with_labels({"parse"});
 * errors.with_labels({"io"});
 *
 * reg.register_metric(requests);
 * reg.register_metric(errors);
 *
 * prometheus::exposer exp(ioc.get_executor(), endpoint);
 * exp.expose(reg);   // registry must outlive the exposer
 * ioc.run();
 * \endcode
 */
template <typename AtomicPolicyType = default_atomic_policy>
class registry
{
    using counter_ptr_type = counter<AtomicPolicyType>*;
    using counter_vec_ptr_type = counter_vec<AtomicPolicyType>*;
    using gauge_ptr_type = gauge<AtomicPolicyType>*;
    using gauge_vec_ptr_type = gauge_vec<AtomicPolicyType>*;
    using histogram_ptr_type = histogram<AtomicPolicyType>*;
    using histogram_vec_ptr_type = histogram_vec<AtomicPolicyType>*;
    using gaugehistogram_ptr_type = gaugehistogram<AtomicPolicyType>*;
    using gaugehistogram_vec_ptr_type = gaugehistogram_vec<AtomicPolicyType>*;
    using info_vec_ptr_type = info_vec<AtomicPolicyType>*;

    // Each type has independent std::mutex with atomic_policy<true>
    detail::tuple_unordered_flat_sets<AtomicPolicyType,
        counter_ptr_type,
        counter_vec_ptr_type,
        gauge_ptr_type,
        gauge_vec_ptr_type,
        histogram_ptr_type,
        histogram_vec_ptr_type,
        gaugehistogram_ptr_type,
        gaugehistogram_vec_ptr_type,
        info_vec_ptr_type> 
    metrics_;

public:

    /**
     * \brief Invokes \p func for each registered metric.
     *
     * Each metric is passed by reference, so \p func may call `inc()`,
     * `observe()`, or any other mutating member on it. The iteration
     * order is unspecified.
     *
     * Used by the exposer to serialize the registry. Users normally do not
     * call this directly.
     *
     * \param func Callable invoked with a reference to each registered
     *             metric. The callable must accept every metric type
     *             listed in `metrics_` — typically a generic lambda
     *             (`[](auto& metric) { ... }`).
     *
     * \par Thread safety
     * With the default atomic policy, `for_each()` takes per-type locks
     * and may be called concurrently from multiple threads and with
     * metric updates. With `atomic_policy<false>`, no synchronization is
     * performed — the caller must prevent concurrent mutation.
     */
    template <typename Func>
    void for_each(Func&& func)
    {
        metrics_.for_each([&](auto* metric_ptr)
        {
            func(*metric_ptr);
        });
    }

    /**
     * \brief Whether the registry uses atomic storage.
     *
     * Mirrors the `enabled` flag of \p AtomicPolicyType. This is the same
     * policy that every metric in the registry must satisfy (enforced by
     * `register_metric()`).
     */
    static consteval bool is_atomic()
    {
        return AtomicPolicyType::enabled;
    }

    /**
     * \brief Registers \p metric with this registry.
     *
     * The registry stores a **pointer** to \p metric; ownership is not
     * transferred. The caller must ensure \p metric outlives the
     * registration, or call `unregister_metric()` before destroying it.
     *
     * The call is idempotent: registering a metric that is already in the
     * registry is a no-op.
     *
     * \tparam MetricType Concrete metric type, e.g. `counter<...>`,
     *         `histogram_vec<...>`.
     * \param metric Metric to register.
     *
     * \pre `MetricType::is_atomic() == is_atomic()` — mixing atomic and
     *      non-atomic metrics in one registry is a compile-time error.
     */
    template <typename MetricType>
    void register_metric(MetricType& metric)
    {
        static_assert(is_atomic() == MetricType::is_atomic(), "Registry and metric type must share the same atomic policy");
        // Insert metric address
        metrics_.insert(&metric);
    }

    /**
     * \brief Removes \p metric from this registry.
     *
     * If \p metric is not currently registered, the call is a no-op.
     * After this call, \p metric may be safely destroyed.
     *
     * \tparam MetricType Concrete metric type.
     * \param metric Metric to unregister.
     *
     * \pre `MetricType::is_atomic() == is_atomic()`.
     */
    template <typename MetricType>
    void unregister_metric(MetricType& metric)
    {
        static_assert(is_atomic() == MetricType::is_atomic(), "Registry and metric type must share the same atomic policy");
        metrics_.erase(&metric);
    }
};

} // namespace boost::prometheus

#endif
