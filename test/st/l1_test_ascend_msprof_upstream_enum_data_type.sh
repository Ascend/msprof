#!/bin/bash

file_name=$(basename "$0" .sh)
cur_dir="$(cd "$(dirname "$0")"; pwd)"
source "${cur_dir}/common.sh"
output_path="$(resolve_output_path "${1:-}" "$0")"

# 校验脚本以 src.xxx / utils.xxx 方式导入，需保证 test/st 在 PYTHONPATH 中。
# run_all.sh 已经设置过，这里再兜底一次以支持单用例手动执行。
if [ -z "${PYTHONPATH}" ]; then
    export PYTHONPATH="${cur_dir}"
else
    export PYTHONPATH="${cur_dir}:${PYTHONPATH}"
fi

testcase_result_dir="${output_path}/${file_name}"
init_testcase_dir "${testcase_result_dir}"
start_time=$(date "+%s")

# 本用例只比对上游枚举定义与仓库内枚举，不采集性能数据，因此不需要 msprof 拉起。
# 输出同时打到终端和日志，便于在冒烟日志中看到新增了哪个枚举值、上游文件在哪。
python3 "${cur_dir}/src/${file_name}.py" 2>&1 | tee "${testcase_result_dir}/enum_check.log"
check_run_exit_code "${PIPESTATUS[0]}" "${file_name}" "${testcase_result_dir}"

end_time=$(date "+%s")
print_result "${file_name}" "${start_time}" "${end_time}"
