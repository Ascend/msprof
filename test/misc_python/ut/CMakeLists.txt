set(MISC_CODE ${TOP_DIR}/abl/msprof/misc)
set(MISC_DB ${TOP_DIR}/llt/abl/msprof/misc_python/ut/testcase)

run_python_llt_test(
    TARGET msprof_python_utest
    SRC_FILES_DIR ${TOP_DIR}/abl/msprof/misc/
    TEST_FILES_DIR ${TOP_DIR}/llt/abl/msprof/misc_python/ut/testcase
    EXPORT_PYTHONPATH "${MISC_CODE}:${MISC_DB}"
)
