#!/bin/sh
# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2025 Huawei Technologies Co.,Ltd.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

CURRENT_DIR=$(dirname $(readlink -f $0)) # 脚本目录

for dir in $(ls -d ${CURRENT_DIR}/*/); do
    if [ -f "${dir}/rollback_precheck.sh" ]; then
        "${dir}/rollback_precheck.sh"
        ret=$?
        if [ ${ret} -ne 0 ]; then
            echo "[All] [$(date +"%Y-%m-%d %H:%M:%S")] [ERROR]: ${dir}/rollback_precheck.sh failed !"
            exit ${ret}
        fi
    fi
done

for dir in $(ls -d ${CURRENT_DIR}/*/); do
    if [ -f "${dir}/rollback.sh" ]; then
        "${dir}/rollback.sh"
        ret=$?
        if [ ${ret} -ne 0 ]; then
            echo "[All] [$(date +"%Y-%m-%d %H:%M:%S")] [ERROR]: ${dir}/rollback.sh failed !"
            exit ${ret}
        fi
    fi
done

"${CURRENT_DIR}/uninstall.sh"
ret=$?
if [ ${ret} -ne 0 ]; then
    echo "[All] [$(date +"%Y-%m-%d %H:%M:%S")] [WARNING]: ${CURRENT_DIR}/uninstall.sh failed !"
fi

echo "[All] [$(date +"%Y-%m-%d %H:%M:%S")] [INFO]: patch rolled back successfully !"
exit 0
