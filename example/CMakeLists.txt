#
# Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

add_custom_target(boost_prometheus_examples)

function(add_example example_name)
    add_executable(${example_name} ${ARGN})
    target_link_libraries(${example_name}
        PRIVATE
            Boost::prometheus
    )
    string(FIND ${example_name} "ssl" found_tls)
    if(found_tls GREATER -1)
        target_link_libraries(${example_name}
            PRIVATE 
                OpenSSL::SSL)
    endif()
    add_dependencies(boost_prometheus_examples ${example_name})
endfunction()

find_package(OpenSSL REQUIRED)

file(GLOB EXAMPLE_SOURCES CONFIGURE_DEPENDS "*.cpp")

foreach(file_path ${EXAMPLE_SOURCES})
    get_filename_component(EXAMPLE_NAME ${file_path} NAME_WE)
    add_example(${EXAMPLE_NAME} ${file_path})
endforeach()
