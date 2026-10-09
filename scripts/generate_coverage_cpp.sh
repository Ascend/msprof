#!/bin/bash
# C++ UT 覆盖率。全量 HTML 每次都会生成；加 diff 时额外做增量门槛。不与 Python 覆盖率合并。
#
# 用法（仓库根目录）：
#   bash scripts/generate_coverage_cpp.sh                 # 全量
#   bash scripts/generate_coverage_cpp.sh diff            # 全量 + 增量
#   bash scripts/generate_coverage_cpp.sh analysis        # 全量，cmake MODE=analysis
#   bash scripts/generate_coverage_cpp.sh analysis diff   # 上面两项组合，参数顺序不限
#
# 参数：
#   diff          在全量之外跑 diff-cover
#   analysis|all  传给 execute_cpp_test_case.sh 的 cmake MODE，默认 all
#
# 环境变量：
#   DIFF_BASE     增量比较基线，默认 origin/${targetBranch:-master}
#   FAIL_UNDER    增量行覆盖率门槛（百分比），默认 80
#   targetBranch  未设置 DIFF_BASE 时使用，默认 master
#
# 依赖：lcov >= 2.0、genhtml；diff 时还需要 lcov_cobertura、diff-cover
#
# 每次运行会清掉上次的报告和 *.gcda，再重新编译/跑 UT 并采集。
# 采集范围：各 *_utest.dir 下的 analysis/csrc（不含 UT 源码）。
# 增量范围：analysis/csrc 整棵树（含多层子目录）。
# 报告：
#   全量  test/build_llt/output/cpp_coverage/result
#   增量  test/build_llt/output/cpp_coverage/result/ut_incremental_coverage_report.html
#         test/build_llt/output/cpp_coverage/incremental.json
# Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=$(readlink -f "${CUR_DIR}/..")
# Resolve the build dir so find(1) can search it. On WSL, test/build_llt is often
# a symlink onto ext4; find starting at the 9p path does not recurse into it.
BUILD_DIR=$(readlink -f "${TOP_DIR}/test/build_llt")
COV_DIR=${BUILD_DIR}/output/cpp_coverage
DIFF_BASE="${DIFF_BASE:-origin/${targetBranch:-master}}"
FAIL_UNDER="${FAIL_UNDER:-80}"
LCOV_MIN_MAJOR=2

DO_DIFF=0
UT_MODE=all
for arg in "$@"; do
    case "$arg" in
        diff) DO_DIFF=1 ;;
        analysis|all) UT_MODE=$arg ;;
    esac
done

check_lcov_version() {
    if ! command -v lcov >/dev/null 2>&1; then
        echo "ERROR: 未找到 lcov。本脚本需要 lcov >= ${LCOV_MIN_MAJOR}.0（使用 --omit-lines 与 --ignore-errors）。" >&2
        exit 1
    fi
    local raw major
    raw=$(lcov --version 2>&1 | head -1)
    major=$(echo "${raw}" | grep -oE '[0-9]+' | head -1)
    if [ -z "${major}" ] || [ "${major}" -lt "${LCOV_MIN_MAJOR}" ]; then
        echo "ERROR: lcov 版本不对：${raw}" >&2
        echo "       本脚本需要 lcov >= ${LCOV_MIN_MAJOR}.0，请升级后再跑。" >&2
        exit 1
    fi
    echo "lcov: ${raw}"
}

# These are the pre-2.0 rc spellings. lcov 2.x only warns and then ignores them,
# which is why branch data currently collapses to almost nothing. Incremental
# gate uses line coverage only.
LCOV_RC="--rc lcov_branch_coverage=1 --rc geninfo_no_exception_branch=1"

# lcov 2.x turns several things that used to be warnings into hard errors:
#   unused      - an -r pattern that matches nothing aborts the script
#   negative    - multithreaded gcov counters can report -1
#   range       - gcda/gcno line past current source (CRLF or a newer edit)
#   category    - genhtml UNK category from merged inconsistent records
# The rest is toolchain noise (system headers, mismatched/source revisions).
LCOV_IGN="--ignore-errors unused,negative,mismatch,deprecated,version,empty,inconsistent,source,count,range,category"

# Log statements are dropped from line coverage at collection time, so
# analysis/csrc is never rewritten.
OMIT_LINES='^[[:blank:]]*(INFO|ERROR|WARN|DEBUG|PRINT_|MAKE_SHARED)'

# Trees that are not project code. Collection is scoped to analysis/csrc under
# each *_utest.dir, so UT sources (test/, fake_process/, ...) are not in scope.
EXCLUDE_PATTERNS=(
    '*/usr/include/*'  # system headers (libstdc++, python3.x, ...)
    '*opensource*'     # vendored json / rapidjson
)

generate_coverage(){
    echo "********************** Generate $2 Coverage Start.************************"
    local info=${COV_DIR}/lcov_$2.info
    lcov -c -d $1 -o ${info} $LCOV_IGN $LCOV_RC --omit-lines "${OMIT_LINES}"
    for pattern in "${EXCLUDE_PATTERNS[@]}" ; do
        lcov -r ${info} "${pattern}" -o ${info} $LCOV_IGN $LCOV_RC
    done
    echo "********************** Generate $2 Coverage Stop.*************************"
}

# CMake records product coverage under
#   <name>_utest.dir/<absolute-source-path>/analysis/csrc/*.gcda
discover_targets(){
    test_obj=()
    test_root=()
    for dir in $(find "${BUILD_DIR}" -name "*_utest.dir" -type d | sort) ; do
        csrc=$(find "${dir}" -type d -path "*/analysis/csrc" -print -quit)
        [ -n "${csrc}" ] || continue
        find "${csrc}" -name '*.gcda' -print -quit | grep -q . || continue
        test_obj+=("$(basename ${dir} .dir)")
        test_root+=("${csrc}")
    done
}

clean_previous_coverage() {
    echo "cleaning previous C++ coverage under ${BUILD_DIR}"
    rm -rf "${COV_DIR}"
    mkdir -p "${COV_DIR}"
    if [ -d "${BUILD_DIR}" ]; then
        # Drop last-run counters. *.gcno stays so make does not have to recompile.
        find "${BUILD_DIR}" -name '*.gcda' -type f -delete || true
        # run_llt_test is gated by *.timestamp; delete so make actually re-runs UTs.
        find "${BUILD_DIR}" -name '*.timestamp' -type f -delete || true
    fi
}

check_lcov_version
clean_previous_coverage

echo "building and running the UTs (MODE=${UT_MODE})"
bash ${CUR_DIR}/execute_cpp_test_case.sh "${UT_MODE}"
discover_targets

if [ ${#test_obj[@]} -eq 0 ] ; then
    echo "ERROR: no coverage data found under ${BUILD_DIR} after running the UTs" >&2
    exit 1
fi
echo "targets with coverage data (${#test_obj[@]}): ${test_obj[@]}"

str_test=""
for i in "${!test_obj[@]}" ; do
    str_test=${str_test}"-a ${COV_DIR}/lcov_${test_obj[$i]}.info "
    generate_coverage "${test_root[$i]}" "${test_obj[$i]}"
done

echo "${str_test}"
lcov ${str_test} -o ${COV_DIR}/ut_report.info $LCOV_IGN $LCOV_RC
genhtml ${COV_DIR}/ut_report.info -o ${COV_DIR}/result --branch-coverage $LCOV_IGN
echo "full report: ${COV_DIR}/result"

if [ "${DO_DIFF}" -eq 1 ]; then
    lcov_cobertura ${COV_DIR}/ut_report.info -o ${COV_DIR}/coverage.xml
    (
        cd "${TOP_DIR}"
        diff-cover ${COV_DIR}/coverage.xml \
            --compare-branch="${DIFF_BASE}" \
            --include 'analysis/csrc/**' \
            --html-report ${COV_DIR}/result/ut_incremental_coverage_report.html \
            --json-report ${COV_DIR}/incremental.json \
            --fail-under="${FAIL_UNDER}"
    )
    echo "cpp incremental vs ${DIFF_BASE} (fail-under ${FAIL_UNDER}%): ${COV_DIR}/result/ut_incremental_coverage_report.html"
fi
echo "report: ${COV_DIR}"
