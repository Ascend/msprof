cmake_minimum_required(VERSION 3.14.1)
project(analysis_llt)
add_definitions(-DMSPORF_LLT)
set(CMAKE_SKIP_RPATH TRUE)
set(TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../)

set(CMAKE_MODULE_PATH
    ${TOP_DIR}/cmake/modules
    ${TOP_DIR}/test/cmake/modules
)
include(${TOP_DIR}/test/cmake/depend.cmake)

find_package(gtest)
find_package(mockcpp)

file(GLOB_RECURSE SECUREC_SOURCES
    ${TOP_DIR}/platform/securec/src/*.c
)

add_library(securec STATIC ${SECUREC_SOURCES})

# common include
include_directories(
    ${TOP_DIR}/test/msprof_cpp/analysis_ut/infrastructure/common_stub/ # 写最前面
    ${TOP_DIR}/
    ${TOP_DIR}/opensource/json/include
    ${TOP_DIR}/opensource/rapidjson/include
    ${TOP_DIR}/platform/securec/include
    ${GTEST_INCLUDE_DIR}
    ${MOCKCPP_INCLUDE_DIR}
)

# common compile option
add_compile_options(
    -g
    -pipe
    -Wall
    -std=c++11
    -fno-access-control
    -Wfloat-equal
    -Wextra
    -D_GLIBCXX_USE_CXX11_ABI=0
    -fprofile-arcs
    -ftest-coverage
)

# common link options
add_link_options(
    -lgcov
    --coverage
)

# common link library
link_libraries(
    ${GTEST_STATIC_LIBRARY}
    ${MOCKCPP_STATIC_LIBRARY}
)

function(run_test test_case)
    include_directories(${TOP_DIR}/)

    target_compile_options(${test_case} PRIVATE
        -g
        -pipe
        -Wall
        -std=c++11
        -fno-access-control
        -Wfloat-equal
        -Wextra
        -D_GLIBCXX_USE_CXX11_ABI=0
        -fprofile-arcs
        -ftest-coverage
    )

    target_link_options(${test_case} PRIVATE
        -lgcov
        --coverage
    )

    target_link_libraries(${test_case} PRIVATE
        ${GTEST_STATIC_LIBRARY}
        ${MOCKCPP_STATIC_LIBRARY}
        pthread
        dl
        securec
    )

    set_target_properties(${test_case}
        PROPERTIES
        OUTPUT_NAME ${test_case}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    run_llt_test(
        TARGET ${test_case}
        TASK_NUM 1
    )
endfunction()

add_subdirectory(viewer)
add_subdirectory(infrastructure)
add_subdirectory(domain)
add_subdirectory(application)
