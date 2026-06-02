#!/bin/bash
set -e

file_name=$(basename $0 .sh)
cur_dir="$(cd "$(dirname "$0")"; pwd)"
output_path=$1
source ${cur_dir}/common.sh

testcase_result_dir="${output_path}/${file_name}"
init_testcase_dir "${testcase_result_dir}"
start_time=$(date "+%s")
cd ${testcase_result_dir}
msprof --output=${testcase_result_dir}/result_dir \
    python3 ${cur_dir}/src/acl_graph_and_triton_inductor.py > ${testcase_result_dir}/plog.txt 2>&1
check_run_exit_code "$?" "${file_name}"
check_plog_error "${testcase_result_dir}/plog.txt" "${file_name}"
python3 ${cur_dir}/src/${file_name}.py ${testcase_result_dir}/result_dir
check_run_exit_code "$?" "${file_name}"
end_time=$(date "+%s")
print_result "${file_name}" "${start_time}" "${end_time}"
