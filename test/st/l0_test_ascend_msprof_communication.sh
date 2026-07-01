#!/bin/bash
set -e

file_name=$(basename $0 .sh)

cur_dir="$(cd "$(dirname "$0")"; pwd)"
ouput_path=$1
MSPROF_OPTS="--ascendcl=on --model-execution=on --runtime-api=on --task-time=on --ai-core=on --l2=on --aic-freq=100 --aicpu=on --sys-hardware-mem=on --sys-hardware-mem-freq=50 --sys-cpu-profiling=on --sys-cpu-freq=50 --sys-profiling=on --sys-sampling-freq=10 --sys-pid-profiling=on --sys-pid-sampling-freq=10 --sys-io-profiling=on --sys-io-sampling-freq=100 --sys-interconnection-profiling=on --sys-interconnection-freq=50 --dvpp-profiling=on --dvpp-freq=50"
MODEL_NAME="communication"

chmod 755 ${cur_dir}/base_msprof.sh
"$cur_dir/base_msprof.sh" "$file_name" "${MSPROF_OPTS}" "${MODEL_NAME}" "${ouput_path}"
