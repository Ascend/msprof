aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/utils DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/data_inventory DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/hal DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/metric DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/tree DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/process DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/db DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/db/include DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/association DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/constant DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/association/cann DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser/host/cann DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser/host DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/adapter DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/environment DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/association/calculator/metric DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/association/calculator/ DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/association/calculator/hccl DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/association/calculator/hccl/include
        DEVICE_ASSOCIATION_SOURCE)

aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/stubs/dfx DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/association/test DEVICE_ASSOCIATION_SOURCE)

list(APPEND DEVICE_ASSOCIATION_SOURCE ${TOP_DIR}/analysis/csrc/domain/services/device_context/load_host_data.h)

add_executable(association_utest
    ${DEVICE_ASSOCIATION_SOURCE}
    ${TOP_DIR}/analysis/csrc/domain/services/persistence/host/number_mapping.cpp
)

target_link_libraries(association_utest PRIVATE
    c_sec_static
    -lsqlite3
)

run_test(association_utest)
