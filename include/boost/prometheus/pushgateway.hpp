//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_PUSHGATEWAY_HPP
#define BOOST_PROMETHEUS_PUSHGATEWAY_HPP

#include <boost/prometheus/detail/clear_op.hpp>
#include <boost/prometheus/detail/extract_option.hpp>
#include <boost/prometheus/detail/push_op.hpp>
#include <boost/prometheus/detail/ssl_fwd.hpp>
#include <boost/prometheus/detail/static_allocator.hpp>

#include <boost/prometheus/exposer_options.hpp>
#include <boost/prometheus/registry.hpp>
#include <boost/prometheus/metric_options.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <string>
#include <tuple>

namespace boost::prometheus {

/**
 * \brief Asynchronous client for the Prometheus Pushgateway.
 *
 * A `pushgateway` connects to a Pushgateway instance over HTTP (or HTTPS)
 * and pushes metric snapshots, or clears the metrics of a given job. It is
 * the write-side counterpart to \ref exposer: where `exposer` serves metrics
 * on demand (pull model), `pushgateway` uploads them (push model) for short-
 * lived jobs, batch processes, and services that cannot be scraped directly.
 *
 * The HTTP layer is built on Boost.Asio and Boost.Beast. Each call to
 * `async_push()` or `async_clear()` opens a fresh connection, performs one
 * request, reads the response header, and completes. The connection is not
 * reused across calls; a new request implies a new TCP (or TLS) handshake.
 *
 * \par Semantics of `async_push`
 * A push **replaces** the values of every metric present in the request
 * body. Metrics that already exist in the job group but are **not** present
 * in the current request are **left untouched** — they are neither deleted
 * nor reset. To remove all metrics of a job group, use `async_clear()`.
 * This matches the behavior of the Pushgateway's `POST /metrics/job/<job>`
 * endpoint.
 *
 * \par Thread safety
 * A `pushgateway` instance is **not** thread-safe: Concurrent calls
 * to `async_push()` or `async_clear()` on the same instance are a data
 * race. Use one instance per thread, or serialize access externally. It is
 * safe to have multiple instances operating on the same Pushgateway.
 *
 * \par Endpoint
 * Each operation takes a `boost::asio::ip::tcp::endpoint` identifying the
 * Pushgateway's address. HTTPS is selected at compile time via
 * `ssl_policy<true>` (see below); when enabled, the constructor also takes
 * an `asio::ssl::context&` that must outlive the `pushgateway` instance.
 *
 * \par Template parameters (policies)
 * The template parameters mirror those of \ref exposer.
 *
 * The accepted policy types are:
 *
 * * `atomic_policy<bool>` — selects the atomicity of `registry_type`.
 *   Defaults to `default_atomic_policy` (`atomic_policy<true>`). Note that
 *   `pushgateway` itself is never thread-safe (see above); this policy
 *   affects the **registry** it serializes, not the client's own state.
 * * `gzip_policy<bool>` — when enabled, the request body is gzip-compressed
 *   before being sent (with `Content-Encoding: gzip`). Defaults to
 *   `default_gzip_policy` (`gzip_policy<true>`).
 * * `parse_buffer_size<N>` — size of the static buffer used by the HTTP
 *   field parser. Defaults to `parse_buffer_size<2048>`. Increase only if
 *   the Pushgateway returns unusually large response headers.
 * * `header_limit_size<N>` — maximum size of the response header block.
 *   Defaults to `header_limit_size<2048>`.
 * * `ssl_policy<bool>` — when `true`, connects over TLS. Defaults to
 *   `default_ssl_policy` (`ssl_policy<false>`). Requires the SSL-enabled
 *   `pushgateway` constructor and OpenSSL.
 *
 * \tparam Executor The Asio executor type, typically obtained via
 *         `decltype(ioc.get_executor())`.
 * \tparam Options  Zero or more policy types (see above). Order does not
 *         matter; when a policy is omitted, its default is used.
 *
 * \par Lifetime
 * The `pushgateway` owns its socket and internal buffers, but does **not**
 * own the registry passed to `async_push()` nor the `ssl::context` passed
 * to the TLS constructor. Both must outlive the `pushgateway` and every
 * outstanding operation that references them.
 *
 * \par Example
 * \code
 * asio::io_context ioc;
 * prometheus::registry reg;
 * // ... register metrics in `reg` ...
 *
 * prometheus::pushgateway pg(ioc.get_executor());
 * auto endpoint = asio::ip::tcp::endpoint(
 *     asio::ip::make_address("127.0.0.1"), 9091);
 *
 * pg.async_push(endpoint, "batch_job", reg,
 *     [](boost::system::error_code ec)
 *     {
 *         if (ec)
 *             std::cerr << "push failed: " << ec.message() << "\n";
 *     });
 *
 * ioc.run();
 * \endcode
 *
 * \sa exposer, registry, static_registry
 */
template <typename Executor, typename... Options>
class pushgateway
{
    using atomic_policy_type = detail::fallback_if_void_t<
        typename detail::extract_option<atomic_policy, Options...>::type,
        default_atomic_policy>;

    using gzip_policy_type = detail::fallback_if_void_t<
        typename detail::extract_option<gzip_policy, Options...>::type,
        default_gzip_policy>;

    // Fields parser buffer size, 2048 is default.
    using parse_buffer_size_type = detail::fallback_if_void_t<
        typename detail::extract_option<parse_buffer_size, Options...>::type,
        parse_buffer_size<2048>>;

    // Header limit, 2048 is default.
    using header_limit_size_type = detail::fallback_if_void_t<
        typename detail::extract_option<header_limit_size, Options...>::type,
        header_limit_size<2048>>;

    using ssl_policy_type = detail::fallback_if_void_t<
        typename detail::extract_option<ssl_policy, Options...>::type,
        default_ssl_policy>;

    using ssl_ctx_type = std::conditional_t<
        ssl_policy_type::enabled,
        typename boost::asio::ssl::context&,
        std::tuple<>>;

    using plain_socket = boost::asio::basic_stream_socket<boost::asio::ip::tcp, Executor>;

public:

    /**
     * \brief Socket type used by this `pushgateway`.
     *
     * Plain `tcp::socket` when `ssl_policy<false>`, or
     * `asio::ssl::stream<tcp::socket>` when `ssl_policy<true>`.
     */
    using socket_type = std::conditional_t<
        ssl_policy_type::enabled,
        typename boost::asio::ssl::stream<plain_socket>,
        plain_socket>;

    /**
     * \brief Registry type accepted by this `pushgateway`.
     *
     * Depends on `atomic_policy` in `Options...`; the registry passed to
     * `async_push()` must be of this type. Registries with a different
     * atomic policy are rejected at compile time.
     */
    using registry_type = registry<atomic_policy_type>;

private:

    static constexpr std::size_t static_buffer_size = parse_buffer_size_type::value;
    using alloc_t = detail::static_allocator<char, static_buffer_size>;

    using request_type = beast::http::request<beast::http::vector_body<char>,
        beast::http::basic_fields<alloc_t>>;

    using gzip_buffer_type = std::conditional_t<
        gzip_policy_type::enabled,
        typename request_type::body_type::value_type,
        std::tuple<>>;

    detail::allocator_buffer<static_buffer_size> alloc_buffer_;
    alloc_t alloc_;
    Executor executor_;
    request_type request_;
    // Parse response header only.
    beast::http::response_parser<beast::http::empty_body, alloc_t>
    response_parser_;
    // Buffer to read pushgateway response header.
    boost::beast::flat_static_buffer<header_limit_size_type::value> buffer_;
    // Buffer to store the gzip result to.
    [[no_unique_address]]
    gzip_buffer_type gzip_buffer_;
    [[no_unique_address]]
    ssl_ctx_type ssl_ctx_;
    socket_type socket_;

    // We need to reset ssl::stream on every connection.
    void reset_ssl_stream()
    {
        if constexpr (ssl_policy_type::enabled)
            socket_ = socket_type(executor_, ssl_ctx_);
    }

public:

    /**
     * \brief Constructs a plain-HTTP pushgateway.
     *
     * Available only when `ssl_policy<false>` (the default).
     *
     * \param ex Executor used for all I/O. Typically
     *        `ioc.get_executor()`.
     */
    explicit pushgateway(Executor ex)
    requires (!ssl_policy_type::enabled)
    : executor_(std::move(ex)),
    alloc_(alloc_buffer_),
    request_(
        std::piecewise_construct,
        std::make_tuple(),
        std::make_tuple(alloc_)
    ),
    response_parser_(
        std::piecewise_construct,
        std::make_tuple(),
        std::make_tuple(alloc_)
    ),
    socket_(executor_)
    {
        request_.body().reserve(4096);
    }

    /**
     * \brief Constructs a TLS pushgateway.
     *
     * Available only when `ssl_policy<true>`.
     *
     * \param ex      Executor used for all I/O.
     * \param ssl_ctx SSL context with the CA / client certificate. Must
     *                outlive the `pushgateway` and every operation that
     *                references it.
     */
    explicit pushgateway(
        Executor ex,
        boost::asio::ssl::context& ssl_ctx)
    requires (ssl_policy_type::enabled)
    : executor_(std::move(ex)),
    alloc_(alloc_buffer_),
    request_(
        std::piecewise_construct,
        std::make_tuple(),
        std::make_tuple(alloc_)
    ),
    response_parser_(
        std::piecewise_construct,
        std::make_tuple(),
        std::make_tuple(alloc_)
    ),
    socket_(executor_, ssl_ctx),
    ssl_ctx_(ssl_ctx)
    {
        request_.body().reserve(4096);
    }

    pushgateway(const pushgateway&)            = delete;
    pushgateway& operator=(const pushgateway&) = delete;
    pushgateway(pushgateway&&)                 = delete;
    pushgateway& operator=(pushgateway&&)      = delete;

    /**
     * \brief Pushes a dynamic registry to a job.
     *
     * Serializes \p registry in the Prometheus 0.0.4 text format, sends it
     * to the Pushgateway at \p endpoint under \p job_name, and reads the
     * response. On completion, `ec` is empty on success or contains the
     * transport / HTTP error otherwise.
     *
     * Metrics already present in the job group but absent from
     * \p registry are **left untouched** — only metrics in the request are
     * overwritten. Use `async_clear()` to remove the entire job group.
     *
     * The registry must outlive the operation.
     *
     * \param endpoint Pushgateway address.
     * \param job_name Job label value, e.g. `"batch_job"`.
     * \param registry Registry to serialize and push. Must have the same
     *        `atomic_policy` as this `pushgateway`.
     * \param token    Completion token with signature
     *        `void(boost::system::error_code)`.
     */
    template
        <boost::asio::completion_token_for<void(boost::system::error_code)>
        CompletionToken>
    auto async_push(
        boost::asio::ip::tcp::endpoint endpoint,
        std::string job_name,
        registry_type& registry,
        CompletionToken&& token)
    {
        reset_ssl_stream();
        return boost::asio::async_compose<CompletionToken,
            void(boost::system::error_code)>(
            detail::push_op<false, gzip_policy_type, decltype(response_parser_), socket_type, request_type,
            decltype(buffer_), gzip_buffer_type, decltype(alloc_buffer_), registry_type>{
            response_parser_, socket_, request_, buffer_, gzip_buffer_, alloc_buffer_, registry,
            std::move(endpoint), std::move(job_name)},
            token,
            socket_
        );
    }

    /**
     * \overload
     * \brief Pushes a static registry to a job.
     *
     * \tparam StaticRegistry A `static_registry<...>` type whose
     *         `atomic_policy` matches this `pushgateway`. Passed as a
     *         template parameter because static registries have no
     *         instances — only the type carries the data.
     *
     * \param endpoint Pushgateway address.
     * \param job_name Job label value.
     * \param token    Completion token with signature
     *        `void(boost::system::error_code)`.
     */
    template <typename StaticRegistry,
        boost::asio::completion_token_for<void(boost::system::error_code)>
        CompletionToken>
    auto async_push(
        boost::asio::ip::tcp::endpoint endpoint,
        std::string job_name,
        CompletionToken&& token)
    {
        reset_ssl_stream();
        return boost::asio::async_compose<CompletionToken,
            void(boost::system::error_code)>(
            detail::push_op<false, gzip_policy_type, decltype(response_parser_), socket_type, request_type,
            decltype(buffer_), gzip_buffer_type, decltype(alloc_buffer_), StaticRegistry>{
            response_parser_, socket_, request_, buffer_, gzip_buffer_, alloc_buffer_,
            std::move(endpoint), std::move(job_name)},
            token,
            socket_
        );
    }

    /**
     * \overload
     * \brief Pushes a dynamic registry with an `Authorization` header.
     *
     * Identical to the 4-argument overload, but sends the given
     * \p authorization string verbatim as the value of the
     * `Authorization` header. The header value is used as-is: any scheme
     * prefix (e.g. `"Bearer "`, `"Basic "`) must be included by the
     * caller. The Pushgateway only needs to accept or reject the request;
     * `pushgateway` performs no validation of its own.
     *
     * \param endpoint      Pushgateway address.
     * \param job_name      Job label value.
     * \param authorization Value of the `Authorization` header.
     * \param registry      Registry to serialize and push.
     * \param token         Completion token with signature
     *        `void(boost::system::error_code)`.
     */
    template
        <boost::asio::completion_token_for<void(boost::system::error_code)>
        CompletionToken>
    auto async_push(
        boost::asio::ip::tcp::endpoint endpoint,
        std::string job_name,
        std::string authorization,
        registry_type& registry,
        CompletionToken&& token)
    {
        reset_ssl_stream();
        return boost::asio::async_compose<CompletionToken,
            void(boost::system::error_code)>(
            detail::push_op<true, gzip_policy_type, decltype(response_parser_), socket_type, request_type,
            decltype(buffer_), gzip_buffer_type, decltype(alloc_buffer_), registry_type>{
            response_parser_, socket_, request_, buffer_, gzip_buffer_, alloc_buffer_, registry,
            std::move(endpoint), std::move(job_name), std::move(authorization)},
            token,
            socket_
        );
    }

    /**
     * \overload
     * \brief Pushes a static registry with an `Authorization` header.
     *
     * \tparam StaticRegistry A `static_registry<...>` type.
     *
     * \param endpoint      Pushgateway address.
     * \param job_name      Job label value.
     * \param authorization Value of the `Authorization` header.
     * \param token         Completion token with signature
     *        `void(boost::system::error_code)`.
     */
    template <typename StaticRegistry,
        boost::asio::completion_token_for<void(boost::system::error_code)>
        CompletionToken>
    auto async_push(
        boost::asio::ip::tcp::endpoint endpoint,
        std::string job_name,
        std::string authorization,
        CompletionToken&& token)
    {
        reset_ssl_stream();
        return boost::asio::async_compose<CompletionToken,
            void(boost::system::error_code)>(
            detail::push_op<true, gzip_policy_type, decltype(response_parser_), socket_type, request_type,
            decltype(buffer_), gzip_buffer_type, decltype(alloc_buffer_), StaticRegistry>{
            response_parser_, socket_, request_, buffer_, gzip_buffer_, alloc_buffer_,
            std::move(endpoint), std::move(job_name), std::move(authorization)},
            token,
            socket_
        );
    }

    /**
     * \brief Clears all metrics of a job group.
     *
     * Sends a `DELETE /metrics/job/<job>` request to the Pushgateway,
     * removing every metric previously pushed under \p job_name. The
     * entire job group is deleted; individual metrics cannot be removed
     * selectively.
     *
     * On completion, `ec` is empty on success or contains the transport /
     * HTTP error otherwise.
     *
     * \param endpoint Pushgateway address.
     * \param job_name Job label value whose group should be removed.
     * \param token    Completion token with signature
     *        `void(boost::system::error_code)`.
     */
    template
        <boost::asio::completion_token_for<void(boost::system::error_code)>
        CompletionToken>
    auto async_clear(
        boost::asio::ip::tcp::endpoint endpoint,
        std::string job_name,
        CompletionToken&& token)
    {
        reset_ssl_stream();
        return boost::asio::async_compose<CompletionToken,
            void(boost::system::error_code)>(
            detail::clear_op<false, decltype(response_parser_), socket_type, request_type,
            decltype(buffer_), decltype(alloc_buffer_)>{
            response_parser_, socket_, request_, buffer_, alloc_buffer_,
            std::move(endpoint), std::move(job_name)},
            token,
            socket_
        );
    }

    /**
     * \overload
     * \brief Clears all metrics of a job group, with an `Authorization`
     *        header.
     *
     * \param endpoint      Pushgateway address.
     * \param job_name      Job label value whose group should be removed.
     * \param authorization Value of the `Authorization` header, used as-is.
     * \param token         Completion token with signature
     *        `void(boost::system::error_code)`.
     */
    template
        <boost::asio::completion_token_for<void(boost::system::error_code)>
        CompletionToken>
    auto async_clear(
        boost::asio::ip::tcp::endpoint endpoint,
        std::string job_name,
        std::string authorization,
        CompletionToken&& token)
    {
        reset_ssl_stream();
        return boost::asio::async_compose<CompletionToken,
            void(boost::system::error_code)>(
            detail::clear_op<true, decltype(response_parser_), socket_type, request_type,
            decltype(buffer_), decltype(alloc_buffer_)>{
            response_parser_, socket_, request_, buffer_, alloc_buffer_,
            std::move(endpoint), std::move(job_name), std::move(authorization)},
            token,
            socket_
        );
    }
};

} // namespace boost::prometheus

#endif
