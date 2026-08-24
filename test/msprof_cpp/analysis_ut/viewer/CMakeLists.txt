aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/utils SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/association/cann SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/application/credential SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/db SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/viewer/database/finals SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/persistence/host SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/dfx SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/tree SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser/host/cann SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser/host/ SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/environment SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/adapter SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/data_inventory SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/data_process SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/data_process/ai_task SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/data_process/system SOURCES)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/viewer/test SOURCES)

list(APPEND SOURCES
    "${TOP_DIR}/analysis/csrc/infrastructure/process/topo_graph.cpp"
    "${TOP_DIR}/analysis/csrc/domain/services/device_context/load_host_data.cpp"
)

add_executable(viewer_utest
    ${SOURCES}
)

target_link_libraries(viewer_utest PRIVATE
    -lsqlite3
)

run_test(viewer_utest)
