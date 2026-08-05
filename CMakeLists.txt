cmake_minimum_required(VERSION 3.14.1)
project(msprof)

set(CMAKE_SKIP_RPATH TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(TOP_DIR ${CMAKE_CURRENT_LIST_DIR})

set(CMAKE_MODULE_PATH
    ${TOP_DIR}/cmake/modules
)

# find_package(protoc)
# find_package(ascend_protobuf_shared)
# find_package(ascend_protobuf_static)

if (DEFINED HITEST AND HITEST STREQUAL "on")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE hitestwrapper)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK hitestwrapper)
endif()
