#
# Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.8...3.22)

project(boost_prometheus VERSION "${BOOST_SUPERPROJECT_VERSION}" LANGUAGES CXX)

add_library(boost_prometheus INTERFACE)
add_library(Boost::prometheus ALIAS boost_prometheus)

target_include_directories(boost_prometheus INTERFACE include)

target_compile_features(boost_prometheus INTERFACE cxx_std_20)

if(BOOST_SUPERPROJECT_VERSION OR DEFINED BOOST_SUPERPROJECT_SOURCE_DIR)
    target_link_libraries(
        boost_prometheus
        INTERFACE
        Boost::asio
        Boost::assert
        Boost::beast
        Boost::config
        Boost::container
        Boost::container_hash
        Boost::core
        Boost::crc
        Boost::predef
        Boost::system
        Boost::unordered
    )
else()
    find_package(Boost 1.90 REQUIRED CONFIG)
    target_link_libraries(
        boost_prometheus
        INTERFACE Boost::headers
    )
endif()

if(BUILD_TESTING)
    # Custom target tests; required by the Boost superproject
    if(NOT TARGET tests)
        add_custom_target(tests)
    endif()
    add_subdirectory(test)
endif()

option(BOOST_PROMETHEUS_BENCH "Build performance benchmarks" OFF)

if (BOOST_PROMETHEUS_BENCH)
    add_subdirectory(bench)
endif()

option(BOOST_PROMETHEUS_EXAMPLES "Build examples" OFF)

if (BOOST_PROMETHEUS_EXAMPLES)
    add_subdirectory(example)
endif()
