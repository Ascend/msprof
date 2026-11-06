aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/data_inventory DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/process DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/utils DEVICE_TASK_SOURCE)

aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/infrastructure/process_control/test DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/stubs/dfx DEVICE_TASK_SOURCE)

add_executable(device_task_utest
    ${DEVICE_TASK_SOURCE}
)

target_include_directories(device_task_utest BEFORE PRIVATE
    ${TOP_DIR}/
    ${TOP_DIR}/analysis/csrc
    ${TOP_DIR}/test/msprof_cpp/analysis_ut/infrastructure/process_control/
)

run_test(device_task_utest)
