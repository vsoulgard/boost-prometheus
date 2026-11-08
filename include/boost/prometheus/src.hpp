//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_SRC_HPP
#define BOOST_PROMETHEUS_SRC_HPP

#include <boost/prometheus/detail/config.hpp>

#ifndef BOOST_PROMETHEUS_SEPARATE_COMPILATION
    #error You need to define BOOST_PROMETHEUS_SEPARATE_COMPILATION in all TU that use the compiled version of Boost.Prometheus, \
    as well as the one where this file is included.
#endif

#include <boost/prometheus/impl/basic_metric_vec.ipp>
#include <boost/prometheus/impl/counter.ipp>
#include <boost/prometheus/impl/gauge_internal.ipp>
#include <boost/prometheus/impl/gauge.ipp>
#include <boost/prometheus/impl/gaugehistogram_internal.ipp>
#include <boost/prometheus/impl/gaugehistogram.ipp>
#include <boost/prometheus/impl/histogram_internal.ipp>
#include <boost/prometheus/impl/histogram.ipp>
#include <boost/prometheus/impl/info.ipp>
#include <boost/prometheus/impl/int_counter.ipp>
#include <boost/prometheus/impl/int_gauge.ipp>

#endif
