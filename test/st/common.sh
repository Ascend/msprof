#!/bin/bash

init_testcase_dir() {
    local testcase_result_dir="$1"
    if [ ! -d ${testcase_result_dir} ]; then
        mkdir -p ${testcase_result_dir}
    else
        rm -rf ${testcase_result_dir}/*
    fi
}

check_run_exit_code() {
    local exit_code="$1"
    local file_name="$2"
    if [ 0 -ne ${exit_code} ]; then
        echo "${file_name} fail"
        exit 1
    fi
}

check_plog_error() {
    local plog_path="$1"
    local file_name="$2"
    if grep "ERROR" ${plog_path} | grep -q "PROFILING"; then
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
