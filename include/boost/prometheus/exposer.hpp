//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_EXPOSER_HPP
#define BOOST_PROMETHEUS_EXPOSER_HPP

#include <boost/prometheus/detail/exposer_session.hpp>
#include <boost/prometheus/detail/extract_option.hpp>
#include <boost/prometheus/detail/ssl_fwd.hpp>

#include <boost/prometheus/exposer_options.hpp>
#include <boost/prometheus/registry.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/basic_stream_socket.hpp>

#include <atomic>
#include <stop_token>
#include <string>
#include <tuple>

namespace boost::prometheus {

/**
 * \brief HTTP exposition endpoint for Prometheus and OpenMetrics metrics.
 *
 * The exposer listens on a TCP endpoint and serves a `registry` in either
 * the Prometheus 0.0.4 or OpenMetrics 1.0 text format, depending on the
 * request's `Accept` header. Behaviour is configured through policy types
 * in `Options...`; all policies are resolved at compile time and add no
 * runtime overhead when unused.
 *
 * Responses larger than 1024 bytes are transparently gzip-compressed when
 * the client sends a compatible `Accept-Encoding` header; smaller responses
 * are sent uncompressed.
 *
 * The exposer starts accepting connections immediately after construction,
 * using coroutines spawned on the provided executor. It does not own the
 * executor: the caller is responsible for running the associated
 * `io_context` (via `run()`, `poll()`, etc.).
 *
 * \tparam Executor The Asio executor type, typically obtained via
 *         `decltype(ioc.get_executor())`.
 * \tparam Options  Zero or more policy types: `atomic_policy`,
 *         `gzip_policy`, `ssl_policy`, `parse_buffer_size`,
 *         `header_limit_size`, `expose_static_registry`,
 *         `authorization_policy`. Policies may appear in any order; when a
 *         policy is omitted, its default is used.
 *
 * \note **Lifetime contract.** The caller must guarantee that both the
 *       exposed registry and the exposer itself outlive every connection
 *       created through them. Typical usage: construct the exposer and
 *       registry before `io_context.run()`, and destroy them after
 *       `io_context.run()` returns with all connections closed.
 *
 * \note **Non-copyable, non-movable.** The exposer owns an acceptor and
 *       background coroutines; it cannot be copied or moved. Construct it
 *       in place.
 *
 * \note **Registry ownership.** The exposer does not own the registry
 *       passed to `expose()`. The registry must outlive the exposer, or at
 *       least every connection served by it.
 *
 * \par Example
 * \code
 * asio::io_context ioc;
 * prometheus::registry reg;
 * reg.register_metric(requests);
 *
 * prometheus::exposer exp(
 *     ioc.get_executor(),
 *     asio::ip::tcp::endpoint(asio::ip::address_v4::loopback(), 8080));
 * exp.expose(reg);
 *
 * ioc.run();
 * \endcode
 */
template <typename Executor, typename... Options>
class exposer
{
    using atomic_policy_type = detail::fallback_if_void_t<
        typename detail::extract_option<atomic_policy, Options...>::type,
        default_atomic_policy>;

    using gzip_policy_type = detail::fallback_if_void_t<
        typename detail::extract_option<gzip_policy, Options...>::type,
        default_gzip_policy>;

    using ssl_policy_type = detail::fallback_if_void_t<
        typename detail::extract_option<ssl_policy, Options...>::type,
        default_ssl_policy>;

    using parse_buffer_size_type = detail::fallback_if_void_t<
        typename detail::extract_option<parse_buffer_size, Options...>::type,
        default_parse_buffer_size>;

    using header_limit_size_type = detail::fallback_if_void_t<
        typename detail::extract_option<header_limit_size, Options...>::type,
        default_header_limit_size>;

    using static_registry_type =
        detail::extract_type_option<expose_static_registry, Options...>::type;

    using authorization_policy_type =
        detail::extract_option<authorization_policy, Options...>::type;

    using ssl_ctx_ptr_type = std::conditional_t<
        ssl_policy_type::enabled,
        typename boost::asio::ssl::context*,
        std::tuple<>>;

public:

    /**
     * \brief Socket type accepted by the exposer.
     */
    using socket_type = boost::asio::basic_stream_socket<boost::asio::ip::tcp, Executor>;
    /**
     * \brief Acceptor type used by the exposer.
     */
    using acceptor_type = boost::asio::basic_socket_acceptor<boost::asio::ip::tcp, Executor>;
    /**
     * \brief Registry type accepted by this exposer.
     */
    using registry_type = registry<atomic_policy_type>;

private:

    using registry_ptr_type = std::conditional_t<
        atomic_policy_type::enabled,
        std::atomic<registry<atomic_policy_type>*>,
        registry<atomic_policy_type>*>;

    Executor executor_;
    [[no_unique_address]]
    ssl_ctx_ptr_type ssl_ctx_;
    acceptor_type acceptor_;
    std::stop_source stop_source_;
    std::string route_;

    // The user is responsible for keeping the registry alive
    registry_ptr_type registry_;

    using session_type = detail::exposer_session<
        socket_type,
        exposer<Executor, Options...>,
        gzip_policy_type,
        ssl_policy_type,
        parse_buffer_size_type,
        header_limit_size_type,
        authorization_policy_type,
        static_registry_type>;

    friend session_type;

    registry<atomic_policy_type>* get_registry()
    {
        if constexpr (atomic_policy_type::enabled)
        {
            return registry_.load(std::memory_order_acquire);
        }
        else
        {
            return registry_;
        }
    }

    boost::asio::awaitable<void> ssl_session_runner(socket_type socket)
    {
        try
        {
            session_type session(std::move(socket), *this, *ssl_ctx_);
            co_await session.run();
        }
        catch (const std::exception& e) {}
    }

    boost::asio::awaitable<void> session_runner(socket_type socket)
    {
        try
        {
            session_type session(std::move(socket), *this);
            co_await session.run();
        }
        catch (const std::exception& e) {}
    }

    boost::asio::awaitable<void> listen()
    {
        while (!stop_source_.stop_requested())
        {
            beast::error_code ec;
            socket_type socket = co_await acceptor_.async_accept(
                boost::asio::redirect_error(boost::asio::use_awaitable, ec));

            if (ec)
            {
                // operation_aborted - acceptor stop()
                if (ec == boost::asio::error::operation_aborted) break;
                continue;
            }

            if (stop_source_.stop_requested()) break;

            if constexpr (ssl_policy_type::enabled)
            {
                boost::asio::co_spawn(
                    executor_,
                    ssl_session_runner(std::move(socket)),
                    boost::asio::detached);
            }
            else
            {
                boost::asio::co_spawn(
                    executor_,
                    session_runner(std::move(socket)),
                    boost::asio::detached);
            }
        }
    }

    void init(boost::asio::ip::tcp::endpoint& endpoint)
    {
        BOOST_ASSERT_MSG(route_.starts_with('/'), "Route must start with \"/\"");

        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen(boost::asio::socket_base::max_listen_connections);

        // Start listening
        boost::asio::co_spawn(executor_, listen(), boost::asio::detached);
    }

public:

    /**
     * \brief Constructs a plain-HTTP exposer.
     *
     * Available only when SSL is disabled (the default, i.e. when no
     * `ssl_policy<true>` is present in `Options...`).
     *
     * The exposer binds and listens on \p endpoint immediately; it does not
     * wait for `io_context.run()` to be called.
     *
     * \param ex       Executor used for all I/O and for spawning sessions.
     * \param endpoint Local endpoint to bind.
     * \param route    URL path served by the exposer. Must start with `/`.
     *                 Defaults to `/metrics`.
     *
     * \throws boost::system::system_error if the socket cannot be opened,
     *         bound, or put into listening state.
     */
    explicit exposer(
        Executor ex,
        boost::asio::ip::tcp::endpoint endpoint,
        std::string route = "/metrics")
    requires (!ssl_policy_type::enabled)
    : executor_(std::move(ex)),
    acceptor_(executor_),
    route_(std::move(route)),
    registry_(nullptr)
    {
        init(endpoint);
    }

     /**
     * \brief Constructs an SSL/TLS exposer.
     *
     * Available only when `ssl_policy<true>` is present in `Options...`.
     * The caller must keep \p ssl_ctx alive for the lifetime of the exposer
     * and all connections it serves.
     *
     * \param ex       Executor used for all I/O and for spawning sessions.
     * \param ssl_ctx  SSL context with the certificate and private key.
     * \param endpoint Local endpoint to bind.
     * \param route    URL path served by the exposer. Must start with `/`.
     *                 Defaults to `/metrics`.
     *
     * \throws boost::system::system_error if the socket cannot be opened,
     *         bound, or put into listening state.
     */
    explicit exposer(
        Executor ex,
        boost::asio::ssl::context& ssl_ctx,
        boost::asio::ip::tcp::endpoint endpoint,
        std::string route = "/metrics")
    requires (ssl_policy_type::enabled)
    : executor_(std::move(ex)),
    ssl_ctx_(&ssl_ctx),
    acceptor_(executor_),
    route_(std::move(route)),
    registry_(nullptr)
    {
        init(endpoint);
    }

    exposer(const exposer&)            = delete;
    exposer& operator=(const exposer&) = delete;
    exposer(exposer&&)                 = delete;
    exposer& operator=(exposer&&)      = delete;

    /**
     * \brief Whether this exposer was instantiated with an atomic policy.
     *
     * `true` when updates to the exposed registry use atomic operations
     * (the default), `false` when a non-atomic policy was requested.
     */
    static consteval bool is_atomic()
    {
        return atomic_policy_type::enabled;
    }

    /**
     * \brief Sets the registry served by the exposer.
     *
     * Replaces any previously exposed registry. The caller must ensure
     * \p registry outlives every subsequent connection. Calling `expose()`
     * while requests are in flight is safe in atomic mode (the pointer is
     * swapped with release semantics); in non-atomic mode, concurrent
     * `expose()` calls with request processing are not synchronized.
     *
     * \param registry Registry to serve. Must match `registry_type` of this
     *                 exposer (i.e. use the same `atomic_policy`).
     */
    void expose(registry_type& registry)
    {
        if constexpr (atomic_policy_type::enabled)
        {
            registry_.store(&registry, std::memory_order_release);
        }
        else
        {
            registry_ = &registry;
        }
    }

    /**
     * \brief Returns the URL path served by this exposer.
     */
    std::string_view route() const noexcept
    {
        return route_;
    }

    /**
     * \brief Stops accepting new connections and closes the acceptor.
     *
     * Idempotent and thread-safe with respect to repeated calls. Existing
     * connections are not forcibly closed; they finish independently and
     * are torn down by the executor when their coroutines complete.
     *
     * The destructor calls `stop()` automatically.
     */
    void stop() noexcept
    {
        stop_source_.request_stop();

        boost::beast::error_code ec;
        acceptor_.close(ec);
        acceptor_.cancel(ec);
    }

    /**
     * \brief Stops the exposer.
     *
     * \sa stop()
     */
    ~exposer()
    {
        stop();
    }

    /**
     * \brief Returns the local port the exposer is bound to.
     *
     * Useful when the endpoint was constructed with port `0` and the
     * operating system assigned a free port. Returns `0` if the acceptor is
     * not bound or the query fails.
     */
    unsigned short port() const noexcept
    {
        boost::beast::error_code ec;
        auto ep = acceptor_.local_endpoint(ec);
        return ec ? 0 : ep.port();
    }
};

} // namespace boost::prometheus

#endif
