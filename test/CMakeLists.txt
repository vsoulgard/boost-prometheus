#
# Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

if(NOT (BOOST_SUPERPROJECT_VERSION OR DEFINED BOOST_SUPERPROJECT_SOURCE_DIR))
    find_package(Boost REQUIRED COMPONENTS unit_test_framework CONFIG)
endif()

function(add_boost_test test_name)
    add_executable(${PROJECT_NAME}_${test_name} src/entry_point.cpp ${ARGN})

    target_include_directories(${PROJECT_NAME}_${test_name} PRIVATE include)
    target_link_libraries(${PROJECT_NAME}_${test_name}
        PRIVATE
            Boost::prometheus
            Boost::unit_test_framework
    )

    # Follow the Boost convention: don't build test targets by default,
    # and only when explicitly requested by building target tests
    set_target_properties(${PROJECT_NAME}_${test_name} PROPERTIES EXCLUDE_FROM_ALL ON)
    add_dependencies(tests ${PROJECT_NAME}_${test_name})
    add_test(
        NAME ${PROJECT_NAME}_${test_name}
        COMMAND ${PROJECT_NAME}_${test_name}
    )
endfunction()

file(GLOB_RECURSE UNIT_TEST_SOURCES CONFIGURE_DEPENDS "unit/*.cpp")
add_boost_test(unittests ${UNIT_TEST_SOURCES})

if(BOOST_PROMETHEUS_INTEGRATION_TESTS)
    # OpenSSL required for SSL integration tests
    find_package(OpenSSL REQUIRED)
    file(GLOB_RECURSE INTEGRATION_TEST_SOURCES CONFIGURE_DEPENDS "integration/*.cpp")
    add_boost_test(integrationtests ${INTEGRATION_TEST_SOURCES})
    target_link_libraries(${PROJECT_NAME}_integrationtests
        PRIVATE
            OpenSSL::SSL
    )
endif()
