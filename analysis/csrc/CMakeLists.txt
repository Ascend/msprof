cmake_minimum_required (VERSION 3.16)
project(msprof_analysis VERSION 0.1.0)

set(TOP_DIR ${CMAKE_SOURCE_DIR}/../..)

# 严格控制so中源文件范围，所有源文件精细到最末端文件夹
aux_source_directory(${CMAKE_SOURCE_DIR}/application SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/application/credential SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/application/summary SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/application/timeline SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/application/database SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/data_process SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/data_process/ai_task SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/data_process/system SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/entities/hal SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/entities/json_trace SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/entities/metric SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/entities/tree SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/adapter SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/association SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/association/calculator/ SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/association/calculator/hccl SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/association/calculator/metric SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/association/cann SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/device_context SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/environment SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/host_worker SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/init SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/modeling SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/modeling/batch_id SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/modeling/step_trace SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/parser SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/parser/freq SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/parser/host SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/parser/host/cann SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/parser/log SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/parser/parser_item SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/parser/pmu SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/parser/track SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/persistence/device SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/domain/services/persistence/host SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/infrastructure/data_inventory SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/infrastructure/db SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/infrastructure/dfx SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/infrastructure/dump_tools SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/infrastructure/dump_tools/csv_tool SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/infrastructure/dump_tools/json_tool SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/infrastructure/process SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/infrastructure/utils SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/interface/py_interface SOURCES)
aux_source_directory(${CMAKE_SOURCE_DIR}/viewer/database/finals SOURCES)

add_library(msprof_analysis SHARED ${SOURCES})

find_package(Python REQUIRED COMPONENTS Development)
find_package(SQLite3 REQUIRED)

# 从 find_package 找到的绝对路径中提取库目录，用于构建时链接器搜索
# 链接仍使用 -lsqlite3，避免将构建镜像的绝对路径嵌入 NEEDED，保证运行时可在 LD_LIBRARY_PATH 中查找
get_filename_component(SQLite3_LIB_DIR ${SQLite3_LIBRARIES} DIRECTORY)

target_include_directories(msprof_analysis PRIVATE
    ${SQLite3_INCLUDE_DIRS}
    /usr/local/include/
    ${Python_INCLUDE_DIRS}
    ${TOP_DIR}
    ${TOP_DIR}/opensource/json/include
    ${TOP_DIR}/opensource/rapidjson/include
    ${TOP_DIR}/platform/securec/include
)

target_compile_options(msprof_analysis PRIVATE
    -std=c++11
    -fPIC
    -fstack-protector-all
    -fno-strict-aliasing
    -fno-common
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    -Wfloat-equal
    -Wextra
    -Werror=uninitialized
    -Werror=return-type
    -D_GLIBCXX_USE_CXX11_ABI=0
    -D_FORTIFY_SOURCE=2
    -DASCEND_CI_LIMITED_PY37 # 保持编译和运行python版本兼容性
    $<$<NOT:$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>>:-O2>
)

target_link_options(msprof_analysis PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -Wl,-Bsymbolic
    -Wl,--exclude-libs=ALL
    $<$<NOT:$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>>:-s>
)

target_link_directories(msprof_analysis PRIVATE
    ${SQLite3_LIB_DIR}
    ${SECUREC_LIB_DIR}
)

target_link_libraries(msprof_analysis PRIVATE
    -lsqlite3
    -lpthread
    c_sec
)
# 去掉lib前缀
set_target_properties(msprof_analysis PROPERTIES PREFIX  "")
# 指定安装路径
install(TARGETS msprof_analysis OPTIONAL
    LIBRARY DESTINATION ${CMAKE_SOURCE_DIR}/../lib64
)
