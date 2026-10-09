#!/bin/bash
# 编译并执行 C++ UT。不采集覆盖率，不修改 analysis/csrc。
#
# 用法（仓库根目录）：
#   bash scripts/execute_cpp_test_case.sh            # cmake MODE=all（默认）
#   bash scripts/execute_cpp_test_case.sh all
#   bash scripts/execute_cpp_test_case.sh analysis   # 只编/跑 analysis 相关 UT
#
# 覆盖率请用 scripts/generate_coverage_cpp.sh。
# 该脚本会先清上次的 *.gcda / 报告，再调用本脚本重新跑 UT，并用 lcov --omit-lines 排除日志行。
# Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..

# Ensure libsqlite3.so can be found at runtime when sqlite-devel is not installed
# The system may only have libsqlite3.so.0 (runtime) without the .so symlink (dev)
LOCAL_LIB_DIR=${TOP_DIR}/test/output/lib
mkdir -p ${LOCAL_LIB_DIR}
if [ ! -f "${LOCAL_LIB_DIR}/libsqlite3.so" ] && [ -f /usr/lib64/libsqlite3.so.0 ]; then
    ln -sf /usr/lib64/libsqlite3.so.0 ${LOCAL_LIB_DIR}/libsqlite3.so
fi
export LD_LIBRARY_PATH=${LOCAL_LIB_DIR}:${LD_LIBRARY_PATH}

mkdir -p ${TOP_DIR}/test/build_llt
cd ${TOP_DIR}/test/build_llt
UT_MODE=all
if [[ -n "$1" && "$1" == "analysis" ]]; then
    UT_MODE=analysis
fi
cmake ../ -DPACKAGE=ut -DMODE=${UT_MODE}
# PACKAGE=ut registers run_llt_test custom targets, so make builds and runs *_utest.
make -j$(nproc)
