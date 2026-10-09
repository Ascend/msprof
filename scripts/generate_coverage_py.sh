#!/bin/bash
# Python UT 覆盖率。全量 xml/html/文本每次都会生成；加 diff 时额外做增量门槛。不与 C++ 覆盖率合并。
#
# 用法（仓库根目录）：
#   bash scripts/generate_coverage_py.sh        # 全量
#   bash scripts/generate_coverage_py.sh diff   # 全量 + 增量
#
# 参数：
#   diff   在全量之外跑 diff-cover；其它参数忽略
#
# 环境变量：
#   DIFF_BASE     增量比较基线，默认 origin/${targetBranch:-master}
#   FAIL_UNDER    增量行覆盖率门槛（百分比），默认 80
#   targetBranch  未设置 DIFF_BASE 时使用，默认 master
#
# 依赖：coverage、pytest；diff 时还需要 diff-cover
#
# 每次运行会清掉上次 python_coverage 目录，再重新跑 pytest 并采集。
# 采集范围：analysis 下的 .py（coverage --source）。
# 增量范围：analysis 整棵树下的 .py（含多层子目录），排除 test/。
# 报告：
#   全量  test/build_llt/output/python_coverage/html
#         test/build_llt/output/python_coverage/coverage.xml
#         test/build_llt/output/python_coverage/python_coverage_report.log
#   增量  test/build_llt/output/python_coverage/inc_coverage_result.html
#         test/build_llt/output/python_coverage/incremental.json
# Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

set -e
real_path=$(readlink -f "$0")
script_dir=$(dirname "$real_path")
top_dir=$(readlink -f "${script_dir}/..")
output_dir="${top_dir}/test/build_llt/output/python_coverage"
src_code="${top_dir}/analysis"
test_code="${top_dir}/test/msprof_python/ut/testcase"
DIFF_BASE="${DIFF_BASE:-origin/${targetBranch:-master}}"
FAIL_UNDER="${FAIL_UNDER:-80}"

DO_DIFF=0
if [[ "${1:-}" == "diff" ]]; then
    DO_DIFF=1
fi

echo "cleaning previous Python coverage under ${output_dir}"
rm -rf "${output_dir}"
mkdir -p "${output_dir}"

export PYTHONPATH=${src_code}:${test_code}:${PYTHONPATH}
work_dir="${output_dir}/pytest_cwd"
mkdir -p "${work_dir}"
cd "${work_dir}"

coverage run --branch --source="${src_code}" -m pytest -s "${test_code}" --junit-xml="${output_dir}/final.xml"
coverage xml -o "${output_dir}/coverage.xml"
coverage html -d "${output_dir}/html"
coverage report > "${output_dir}/python_coverage_report.log"
echo "full report: ${output_dir}/html"
echo "full summary: ${output_dir}/python_coverage_report.log"

if [ "${DO_DIFF}" -eq 1 ]; then
    cd "${top_dir}"
    diff-cover "${output_dir}/coverage.xml" \
        --compare-branch="${DIFF_BASE}" \
        --include 'analysis/**/*.py' \
        --exclude 'test/*' \
        --html-report "${output_dir}/inc_coverage_result.html" \
        --json-report "${output_dir}/incremental.json" \
        --fail-under="${FAIL_UNDER}"
    echo "python incremental vs ${DIFF_BASE} (fail-under ${FAIL_UNDER}%): ${output_dir}/inc_coverage_result.html"
fi
echo "report: ${output_dir}"
find "${top_dir}" -name "__pycache__" | xargs --no-run-if-empty rm -r
