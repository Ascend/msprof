aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/utils DEVICE_PERSISTENCE_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/data_inventory DEVICE_PERSISTENCE_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/hal DEVICE_PERSISTENCE_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/step_trace DEVICE_PERSISTENCE_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/modeling/step_trace DEVICE_PERSISTENCE_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/modeling/batch_id DEVICE_PERSISTENCE_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/metric DEVICE_PERSISTENCE_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/process DEVICE_PERSISTENCE_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/db DEVICE_PERSISTENCE_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/constant DEVICE_PERSISTENCE_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/persistence/device DEVICE_PERSISTENCE_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/association/calculator/metric DEVICE_PERSISTENCE_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/association/calculator/ DEVICE_PERSISTENCE_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group
        DEVICE_PERSISTENCE_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser/track DEVICE_PERSISTENCE_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser DEVICE_PERSISTENCE_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser/freq DEVICE_PERSISTENCE_SOURCE)

aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/stubs/dfx DEVICE_PERSISTENCE_SOURCE)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/persistence/test DEVICE_PERSISTENCE_SOURCE)

add_executable(device_persistence_utest
    ${DEVICE_PERSISTENCE_SOURCE}
    ${TOP_DIR}/analysis/csrc/domain/services/persistence/host/number_mapping.cpp
)
target_link_libraries(device_persistence_utest PRIVATE
    c_sec_static
    -lsqlite3
)

run_test(device_persistence_utest)
