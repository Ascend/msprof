#!/usr/bin/env bash
set -euo pipefail

real_path=$(readlink -f "$0")
script_dir=$(dirname "$real_path")
src_code="${script_dir}/../analysis"
test_code="${script_dir}/../test/msprof_python/ut/testcase"

# Check if the test directory exists
if [[ ! -d "${test_code}" ]]; then
    echo "ERROR: Test directory does not exist: ${test_code}" >&2
    exit 1
fi

export PYTHONPATH=${src_code:-.}:${test_code}:${PYTHONPATH:-}

python3 -m pytest -sv "${test_code}"
