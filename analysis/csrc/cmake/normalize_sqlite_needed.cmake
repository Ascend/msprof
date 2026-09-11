if(NOT TARGET_SONAME)
    message(FATAL_ERROR "TARGET_SONAME is required to normalize SQLite3 DT_NEEDED")
endif()

execute_process(
    COMMAND "${PATCHELF_EXECUTABLE}" --print-needed "${TARGET_FILE}"
    RESULT_VARIABLE PATCHELF_PRINT_RESULT
    OUTPUT_VARIABLE NEEDED_LIBRARIES
    ERROR_VARIABLE PATCHELF_PRINT_ERROR
)
if(NOT PATCHELF_PRINT_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to inspect ${TARGET_FILE}: ${PATCHELF_PRINT_ERROR}")
endif()

string(REPLACE "\r\n" "\n" NEEDED_LIBRARIES "${NEEDED_LIBRARIES}")
string(REPLACE "\n" ";" NEEDED_LIBRARY_LIST "${NEEDED_LIBRARIES}")
foreach(NEEDED_LIBRARY IN LISTS NEEDED_LIBRARY_LIST)
    string(STRIP "${NEEDED_LIBRARY}" NEEDED_LIBRARY)
    if(NEEDED_LIBRARY MATCHES "^libsqlite3\\.so(\\.[0-9]+)*$" AND
       NOT NEEDED_LIBRARY STREQUAL "${TARGET_SONAME}")
        execute_process(
            COMMAND "${PATCHELF_EXECUTABLE}" --replace-needed
                "${NEEDED_LIBRARY}" "${TARGET_SONAME}" "${TARGET_FILE}"
            RESULT_VARIABLE PATCHELF_REPLACE_RESULT
            ERROR_VARIABLE PATCHELF_REPLACE_ERROR
        )
        if(NOT PATCHELF_REPLACE_RESULT EQUAL 0)
            message(FATAL_ERROR
                "Failed to normalize SQLite3 DT_NEEDED in ${TARGET_FILE}: ${PATCHELF_REPLACE_ERROR}"
            )
        endif()
    endif()
endforeach()
