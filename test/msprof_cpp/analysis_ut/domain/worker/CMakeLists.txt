aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/association/cann SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/host_worker SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/dfx SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/utils SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser/host/cann SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser/host/ SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/db SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/adapter SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/tree SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/environment SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/persistence/host SOURCES)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/worker/test SOURCES)
list(APPEND SOURCES ${TOP_DIR}/analysis/csrc/domain/services/modeling/npu_op_mem_calculator.cpp)

add_executable(host_worker_utest
    ${SOURCES}
)

target_link_libraries(host_worker_utest PRIVATE
    -lsqlite3
)

run_test(host_worker_utest)
