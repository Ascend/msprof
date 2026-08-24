aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/data_inventory SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/dump_tools SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/dump_tools/csv_tool SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/dump_tools/json_tool SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/utils SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/dfx SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/process SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/environment SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/json_trace SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/data_process SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/data_process/ai_task SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/data_process/system SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/viewer_data/ai_task SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/environment SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/db SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/viewer/database/finals SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/application/timeline SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/application/summary SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/application/database SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/application/credential SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/application SOURCES)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/application/summary/test SOURCES)

list(APPEND SOURCES "${TOP_DIR}/analysis/csrc/domain/services/device_context/load_host_data.cpp")

add_executable(export_summary_utest
        ${SOURCES}
)

target_link_libraries(export_summary_utest PRIVATE
    -lsqlite3
)

run_test(export_summary_utest)
