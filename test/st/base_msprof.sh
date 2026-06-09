#!/bin/bash

cur_dir=$(dirname $(readlink -f $0))
export ASCEND_SLOG_PRINT_TO_STDOUT=1
export ASCEND_GLOBAL_LOG_LEVEL=1

source ${cur_dir}/common.sh

file_name="$1"
MSPROF_OPTS="$2"
MODEL_NAME="$3"
output_path="$(readlink -f "$4")"

if [[ -z "${4:-}" ]]; then
    echo "Error: output_path is empty" >&2
    exit 1
fi

main(){
    testcase_result_dir="${output_path}/${file_name}"
    init_testcase_dir "${testcase_result_dir}"

    start_time=$(date "+%s")
    cd ${cur_dir}/src/

    # 1. The msprof approach is used to lift the model for training
    msprof --output=${testcase_result_dir}/result_dir ${MSPROF_OPTS} \
           python3 ${cur_dir}/src/train.py --model ${MODEL_NAME} \
           > ${testcase_result_dir}/plog.txt 2>&1
    check_run_exit_code "$?" "${file_name}" "${testcase_result_dir}"

    check_plog_error "${testcase_result_dir}/plog.txt" "${file_name}" "${testcase_result_dir}"

    # 3. Verify the profiling data items
    python3 ${cur_dir}/src/${file_name}.py ${testcase_result_dir}/result_dir
    check_run_exit_code "$?" "${file_name}" "${testcase_result_dir}"

    end_time=$(date "+%s")
    print_result "${file_name}" "${start_time}" "${end_time}"
}

main
