include_directories(
    ${TOP_DIR}/analysis/csrc
    ${TOP_DIR}/test/msprof_cpp/analysis_ut/application/assemble
    ${TOP_DIR}/test/msprof_cpp/analysis_ut/application/summary
)

add_subdirectory(assemble)
add_subdirectory(summary)
