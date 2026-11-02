//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_ERROR_CATEGORIES_HPP
#define BOOST_PROMETHEUS_ERROR_CATEGORIES_HPP

#include <boost/prometheus/pushgateway_errc.hpp>

#include <boost/beast/http/status.hpp>

#include <boost/system/error_category.hpp>

namespace boost::prometheus {

namespace detail {

class http_status_category : public boost::system::error_category
{
public:
    const char* name() const noexcept override { return "prometheus.http_status"; }

    std::string message(int status) const override
    {
        using S = beast::http::status;
        switch (static_cast<S>(status)) {
        case S::bad_request:           return "400 Bad Request";
        case S::unauthorized:          return "401 Unauthorized";
        case S::forbidden:             return "403 Forbidden";
        case S::not_found:             return "404 Not Found";
        case S::method_not_allowed:    return "405 Method Not Allowed";
        case S::payload_too_large:     return "413 Payload Too Large";
        case S::internal_server_error: return "500 Internal Server Error";
        case S::bad_gateway:           return "502 Bad Gateway";
        case S::service_unavailable:   return "503 Service Unavailable";
        default:                       return "HTTP " + std::to_string(status);
        }
    }
};

class pushgateway_category : public boost::system::error_category
{
public:
    const char* name() const noexcept override { return "prometheus.pushgateway"; }

    std::string message(int ev) const override
    {
        switch (static_cast<pushgateway_errc>(ev))
        {
            case pushgateway_errc::incorrect_job_name: return "incorrect job name";
        }
        return "unknown Prometheus pushgateway error";
    }
};

}

namespace error {

inline const boost::system::error_category& get_http_status_category() noexcept
{
    static detail::http_status_category instance;
    return instance;
}

inline boost::system::error_code make_http_status_error(int status)
{
    return {status, error::get_http_status_category()};
}

inline const boost::system::error_category& get_pushgateway_category() noexcept
{
    static detail::pushgateway_category instance;
    return instance;
}

} // namespace error

} // namespace boost::prometheus

#endif
