#!/bin/bash
# This script is used to execute llt testcase.
# Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..

function add_gcov_excl_line_for_analysis() {
    find ${TOP_DIR}/analysis/csrc -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*INFO.*;/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*INFO.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
    find ${TOP_DIR}/analysis/csrc -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*ERROR.*;/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*ERROR.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
    find ${TOP_DIR}/analysis/csrc -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*WARN.*;/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*WARN.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
    find ${TOP_DIR}/analysis/csrc -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*DEBUG.*;/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*DEBUG.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
    find ${TOP_DIR}/analysis/csrc -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*PRINT_.*;/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*PRINT_.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
    find ${TOP_DIR}/analysis/csrc -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*MAKE_SHARED.*;$/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*MAKE_SHARED.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
}

function add_gcov_excl_line() {
    add_gcov_excl_line_for_analysis
}

function change_file_to_unix_format()
{
    find ${TOP_DIR}/analysis/csrc -type f -exec sed -i 's/\r$//' {} +
}

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
if [[ -n "$1" && "$1" == "analysis" ]]; then
    cmake ../ -DPACKAGE=ut -DMODE=analysis
elif [[ -n "$1" && "$1" == "all" ]]; then
    cmake ../ -DPACKAGE=ut -DMODE=all
else
    # gcov 覆盖率统计需要给日志宏加 LCOV_EXCL_LINE 注释，且需要 Unix 换行。
    # 为避免污染源文件，先备份 analysis/csrc，修改后编译测试，最后恢复。
    BACKUP_DIR=$(mktemp -d)
    cp -a ${TOP_DIR}/analysis/csrc ${BACKUP_DIR}/csrc_bak || { echo "[execute_cpp_test_case] WARN: failed to backup csrc, aborting"; exit 1; }

    # trap 必须在 csrc 被修改之前注册，包含 EXIT/INT/TERM 信号
    cleanup_and_restore() {
        echo "[execute_cpp_test_case] Restoring analysis/csrc from backup..."
        rm -rf ${TOP_DIR}/analysis/csrc
        if cp -a ${BACKUP_DIR}/csrc_bak ${TOP_DIR}/analysis/csrc; then
            rm -rf ${BACKUP_DIR}
        else
            echo "[execute_cpp_test_case] ERROR: restore failed, backup kept at ${BACKUP_DIR}"
        fi
    }
    trap cleanup_and_restore EXIT INT TERM

    change_file_to_unix_format
    add_gcov_excl_line

    cmake ../ -DPACKAGE=ut -DMODE=all
fi
make -j$(nproc)
