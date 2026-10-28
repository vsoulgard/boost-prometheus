//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_STATIC_REGISTRY_HPP
#define BOOST_PROMETHEUS_STATIC_REGISTRY_HPP

#include <boost/prometheus/metric_options.hpp>

#include <boost/prometheus/detail/friend_registration.hpp>
#include <boost/prometheus/detail/static_gauge_internal.hpp>
#include <boost/prometheus/detail/static_histogram_internal.hpp>
#include <boost/prometheus/detail/static_info_internal.hpp>
#include <boost/prometheus/detail/static_metric_registrator.hpp>
#include <boost/prometheus/detail/string_buffer.hpp>
#include <boost/prometheus/detail/tuple_vectors.hpp>
#include <boost/prometheus/detail/types_fwd.hpp>

namespace boost::prometheus {

/**
 * \brief Compile-time registry for static metrics.
 *
 * A `static_registry` is the compile-time counterpart of \ref registry. It
 * holds pointers to the internal storage of \ref static_counter,
 * \ref static_gauge, \ref static_histogram, and \ref static_info instances,
 * and is exposed by the exposer via
 * `expose_static_registry<...>`.
 *
 * Unlike \ref registry, a `static_registry` has **no instances**: all of its
 * storage and methods are static. Each instantiation of the class template
 * — a distinct `(RegistryName, AtomicPolicyType)` pair — is a separate,
 * global registry. This makes it possible to have several independent static
 * registries in one program, for example to group metrics by subsystem.
 *
 * Registration happens during static initialization: calling
 * `register_metric<MetricType>()` instantiates a registrator that
 * appends the metric to the registry before `main()` runs. By the time
 * `main()` starts, the registry is **constant by design** — its storage is
 * a plain `std::vector` with no locks, so `for_each()` and `size()` are safe
 * to call concurrently from any number of threads without synchronization.
 *
 * \tparam RegistryName     Registry name as an NTTP. Defaults to
 *                          `default_static_registry_name` (an empty string),
 *                          which designates the unnamed default registry.
 *                          The name is reported by `name()` and used to
 *                          distinguish registries in diagnostics.
 * \tparam AtomicPolicyType Compile-time atomicity policy. The default,
 *                          `default_atomic_policy` (i.e.
 *                          `atomic_policy<true>`), makes updates to the
 *                          registered metrics thread-safe. All metrics
 *                          registered in this registry must share the same
 *                          policy.
 *
 * \par Thread safety
 * Registration (`register_metric<...>()`) is intended for static
 * initialization, before any other thread can observe the registry.
 * After `main()` begins, the registry is read-only: `for_each()` and
 * `size()` may be called concurrently without synchronization.
 *
 * Individual metric updates (`static_counter::inc()`, etc.) follow the
 * atomic policy of each metric. With the default atomic policy, updates are
 * thread-safe and may proceed concurrently with `for_each()`.
 *
 * \par Example
 * \code
 * using namespace boost::prometheus;
 *
 * // A named registry for metrics of the "http" subsystem.
 * using http_registry = static_registry<"http">;
 *
 * using requests_total = static_counter<
 *     "requests",
 *     help<"Number of received requests">,
 *     labels<"method">>;
 *
 * requests_total::inc<"GET">();
 *
 * // Exposed alongside the dynamic registry via:
 * //   exposer<Executor, expose_static_registry<http_registry>> exp(...);
 *
 * // The registry reports its name and size:
 * static_assert(http_registry::name()[0] == 'h');
 * // http_registry::size() == 1 after requests_total is registered.
 * \endcode
 *
 * \note All members are static: a `static_registry` is never instantiated.
 *       It serves purely as a compile-time handle for a group of static
 *       metrics.
 *
 * \sa static_counter, static_gauge, static_histogram, static_info,
 *     registry, expose_static_registry
 */
template <detail::string_buffer RegistryName = default_static_registry_name,
    typename AtomicPolicyType = default_atomic_policy>
class static_registry
{
    using gauge_internal_ptr_type = detail::static_gauge_internal<AtomicPolicyType>*;
    using histogram_internal_ptr_type = detail::static_histogram_internal<AtomicPolicyType>*;
    using info_internal_ptr_type = detail::static_info_internal<AtomicPolicyType>*;

    // Plain vectors, without any locks: the whole registry
    // is constant by design once the program enters main().
    inline static detail::tuple_vectors<
        gauge_internal_ptr_type,
        histogram_internal_ptr_type,
        info_internal_ptr_type>
    metrics_;

    template <typename RegistryType, typename MetricType>
    friend struct detail::static_metric_registrator;

    // Serialization
    friend struct boost::prometheus::format;

public:

 /**
     * \brief Invokes \p func for each registered metric.
     *
     * Each metric is passed by reference, so \p func may call `get_value()`
     * or other read-only members on it. The iteration order is unspecified.
     *
     * Used by the exposer to serialize the registry. Users normally do not
     * call this directly.
     *
     * \param func Callable invoked with a reference to each registered
     *             metric. The callable must accept every internal metric
     *             type — typically a generic lambda
     *             (`[](auto& metric) { ... }`).
     *
     * \par Thread safety
     * Safe to call concurrently from multiple threads: the registry is
     * immutable once `main()` begins.
     */
    template <typename Func>
    static void for_each(Func&& func)
    {
        metrics_.for_each([&](auto* metric_ptr)
        {
            func(*metric_ptr);
        });
    }

    /**
     * \brief Returns the registry name.
     *
     * The returned pointer points to the NTTP storage of \p RegistryName
     * and is valid for the entire program lifetime.
     */
    static consteval const char* name()
    {
        return RegistryName.value;
    }

    /**
     * \brief Whether metrics in this registry use atomic operations.
     *
     * Mirrors the `enabled` flag of \p AtomicPolicyType. Every metric
     * registered in this registry must satisfy this policy (enforced by
     * `register_metric()`).
     */
    static consteval bool is_atomic()
    {
        return AtomicPolicyType::enabled;
    }

    /**
     * \brief Returns the number of distinct metric types registered.
     *
     * Counts **types**, not series: a single \ref static_counter with two
     * label combinations contributes `1`, not `2`. The count is final once
     * `main()` begins.
     *
     * \par Thread safety
     * Safe to call concurrently from multiple threads.
     */
    static std::size_t size() noexcept
    {
        return metrics_.size();
    }

    /**
     * \brief Registers a static metric with this registry.
     *
     * Triggers the static registrator for \p MetricType, which appends the
     * metric's storage to the registry during static initialization.
     *
     * The call is idempotent: registering the same \p MetricType twice is a
     * no-op (the registrator itself is a static object, initialized once).
     *
     * \tparam MetricType A static metric type, e.g. `static_counter<...>`,
     *         `static_gauge<...>`, `static_histogram<...>`, or
     *         `static_info<...>`.
     *
     * \pre `MetricType::is_atomic() == is_atomic()` — mixing atomic and
     *      non-atomic metrics in one registry is a compile-time error.
     */
    template <typename MetricType>
    static void register_metric()
    {
        static_assert(is_atomic() == MetricType::is_atomic(), "Registry and metric type must share the same atomic policy.");
        // Initialize static metric
        (void)detail::static_metric_registrator<static_registry, MetricType>::_;
    }
};

} // namespace boost::prometheus

#endif
