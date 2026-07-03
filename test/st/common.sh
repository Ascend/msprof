#!/bin/bash

resolve_output_path() {
    local output_path="${1:-}"
    local script_name="${2:-$0}"

    if [ -z "${output_path}" ]; then
        echo "Usage: ${script_name} <output_path>" >&2
        exit 1
    fi

    readlink -m "${output_path}"
}

init_testcase_dir() {
    local testcase_result_dir="$1"
    local resolved_testcase_dir

    if [ -z "${testcase_result_dir}" ] || [ "${testcase_result_dir}" = "/" ]; then
        echo "Error: invalid testcase_result_dir: '${testcase_result_dir}'" >&2
        exit 1
    fi

    resolved_testcase_dir="$(readlink -m "${testcase_result_dir}")"
    if [ -z "${resolved_testcase_dir}" ] || [ "${resolved_testcase_dir}" = "/" ] || \
       [ "$(dirname "${resolved_testcase_dir}")" = "/" ]; then
        echo "Error: unsafe testcase_result_dir: '${testcase_result_dir}'" >&2
        exit 1
    fi

    if [ ! -d "${resolved_testcase_dir}" ]; then
        mkdir -p "${resolved_testcase_dir}"
    else
        find "${resolved_testcase_dir}" -mindepth 1 -maxdepth 1 -exec rm -rf -- {} +
    fi
}

print_error_matches() {
    local file_path="$1"
    local keyword="$2"
    if [ ! -f "${file_path}" ]; then
        return
    fi

    local matches
    matches=$(grep -in "${keyword}" "${file_path}" || true)
    if [ -n "${matches}" ]; then
        echo "[ERROR-LOG] ${file_path}"
        echo "${matches}"
    fi
}

dump_error_logs() {
    local testcase_result_dir="$1"

    print_error_matches "${testcase_result_dir}/plog.txt" "ERROR"

    if [ ! -d "${testcase_result_dir}/result_dir" ]; then
        return
    fi

    while IFS= read -r -d '' log_file; do
        print_error_matches "${log_file}" "ERROR"
    done < <(find "${testcase_result_dir}/result_dir" -path "*/mindstudio_profiler_log/*.log" -type f -print0 2>/dev/null)
}

check_run_exit_code() {
    local exit_code="$1"
    local file_name="$2"
    local testcase_result_dir="$3"
    if [ 0 -ne ${exit_code} ]; then
        if [ -n "${testcase_result_dir}" ]; then
            dump_error_logs "${testcase_result_dir}"
        fi
        echo "${file_name} fail"
        exit 1
    fi
}

check_plog_error() {
    local plog_path="$1"
    local file_name="$2"
    local testcase_result_dir="$3"
    if grep "ERROR" "${plog_path}" | grep -q "PROFILING"; then
        if [ -n "${testcase_result_dir}" ]; then
            dump_error_logs "${testcase_result_dir}"
        fi
        echo "${file_name} fail"
        exit 1
    fi
}

print_result() {
    local file_name="$1"
    local start_time="$2"
    local end_time="$3"
    local duration_time=$(( ${end_time} - ${start_time} ))
    echo "${file_name} pass ${duration_time}"
}
