#
# Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

find_package(prometheus-cpp REQUIRED CONFIG) 
find_package(Threads REQUIRED)

add_custom_target(boost_prometheus_bench)

file(GLOB BENCH_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(SOURCE_FILE ${BENCH_SOURCES})
    get_filename_component(BENCH_NAME ${SOURCE_FILE} NAME_WE)
    
    set(TARGET_NAME "boost_prometheus_bench_${BENCH_NAME}")

    add_executable(${TARGET_NAME} ${SOURCE_FILE})

    target_link_libraries(${TARGET_NAME} PRIVATE
        Boost::prometheus
        prometheus-cpp::core
        Threads::Threads
    )

    add_dependencies(boost_prometheus_bench ${TARGET_NAME})

endforeach()
