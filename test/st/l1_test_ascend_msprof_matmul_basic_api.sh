#!/bin/bash
set -e

file_name=$(basename "$0" .sh)
cur_dir="$(cd "$(dirname "$0")"; pwd)"
source "${cur_dir}/common.sh"
output_path="$(resolve_output_path "${1:-}" "$0")"

sample_dir="${cur_dir}/matmul_basic_api"
build_dir="${sample_dir}/build"
demo_path="${build_dir}/demo"

testcase_result_dir="${output_path}/${file_name}"
init_testcase_dir "${testcase_result_dir}"
start_time=$(date "+%s")
build_log="${testcase_result_dir}/build.log"

rm -rf "${build_dir}"
mkdir -p "${build_dir}"
cd "${build_dir}"
cmake -DCMAKE_ASC_ARCHITECTURES=dav-2201 .. > "${build_log}" 2>&1
check_run_exit_code "$?" "${file_name}" "${testcase_result_dir}"
make -j >> "${build_log}" 2>&1
check_run_exit_code "$?" "${file_name}" "${testcase_result_dir}"

python3 ../scripts/gen_data.py >> "${build_log}" 2>&1
check_run_exit_code "$?" "${file_name}" "${testcase_result_dir}"

if [ ! -f "${demo_path}" ]; then
    echo "Error: demo not found at ${demo_path}" >&2
    echo "Build finished but demo is still missing." >&2
    exit 1
fi

MSPROF_OPTS="--ascendcl=on --model-execution=on --runtime-api=on --task-time=on --ai-core=on --l2=on --aic-freq=100 --aicpu=on --sys-hardware-mem=on --sys-hardware-mem-freq=50 --sys-cpu-profiling=on --sys-cpu-freq=50 --sys-profiling=on --sys-sampling-freq=10 --sys-pid-profiling=on --sys-pid-sampling-freq=10 --sys-io-profiling=on --sys-io-sampling-freq=100 --sys-interconnection-profiling=on --sys-interconnection-freq=50 --dvpp-profiling=on --dvpp-freq=50"
msprof --output="${testcase_result_dir}/result_dir" ${MSPROF_OPTS} \
    "${demo_path}" \
    > "${testcase_result_dir}/plog.txt" 2>&1
check_run_exit_code "$?" "${file_name}" "${testcase_result_dir}"
check_plog_error "${testcase_result_dir}/plog.txt" "${file_name}" "${testcase_result_dir}"

python3 "${cur_dir}/src/${file_name}.py" "${testcase_result_dir}/result_dir"
check_run_exit_code "$?" "${file_name}" "${testcase_result_dir}"

end_time=$(date "+%s")
print_result "${file_name}" "${start_time}" "${end_time}"
