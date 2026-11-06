aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/data_inventory DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/hal DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/init DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/process DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/utils DEVICE_TASK_SOURCE)

aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/init/test DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/stubs/dfx DEVICE_TASK_SOURCE)

add_executable(device_init_utest
    ${DEVICE_TASK_SOURCE}
    ${TOP_DIR}/analysis/csrc/domain/services/device_context/cpu_info.cpp
    ${TOP_DIR}/analysis/csrc/domain/services/device_context/device_context.cpp
    ${TOP_DIR}/analysis/csrc/domain/services/device_context/device_info.cpp
    ${TOP_DIR}/analysis/csrc/domain/services/device_context/load_start_log.cpp
    ${TOP_DIR}/analysis/csrc/domain/services/device_context/sample_info.cpp
    ${TOP_DIR}/analysis/csrc/domain/services/device_context/single_device.cpp
    ${TOP_DIR}/analysis/csrc/domain/services/device_context/start_info.cpp
)

run_test(device_init_utest)
