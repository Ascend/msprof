aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/dump_tools DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/dump_tools/json_tool DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/dump_tools/csv_tool DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/utils/ DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/dfx/ DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/db DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/json_trace DEVICE_TASK_SOURCE)

aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/infrastructure/dump_tools/test DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/infrastructure/dump_tools/sample DEVICE_TASK_SOURCE)

add_executable(dump_tools_utest
    ${DEVICE_TASK_SOURCE}
)

target_link_libraries(dump_tools_utest PRIVATE
    -lsqlite3
)

target_include_directories(dump_tools_utest PRIVATE
    ${TOP_DIR}/analysis/csrc
    ${TOP_DIR}/opensource/rapidjson/include
    ${TOP_DIR}/test/msprof_cpp/analysis_ut/infrastructure/dump_tools/
    ${TOP_DIR}/test/msprof_cpp/analysis_ut/infrastructure/dump_tools/csv_tool/
)

run_test(dump_tools_utest)
