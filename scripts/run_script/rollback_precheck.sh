#!/bin/bash
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

# script for spc
install_path="$(
    cd "$(dirname "$0")/../../../"
    pwd
)"

function file_check() {
    if [ ! -f "$1" ]; then
        print "ERROR" "the source file $2 does not exist, rollback precheck failed"
        exit 1
    fi
}

function dir_check() {
    if [ ! -d "$1" ]; then
        print "ERROR" "dir $2 is not installed, rollback precheck failed"
        exit 1
    fi
}

function get_cann_package_name() {
	local name_list=(${install_path//// })
	cann_package_name=${name_list[-2]}
	if [ "$cann_package_name" = "ascend-toolkit" ] || [ "$cann_package_name" = "nnrt" ] || [ "$cann_package_name" = "nnae" ]; then
		return
	fi
	print "ERROR" "There is no ascend-toolkit, nnrt or nnae."
	exit 1
}

function deal_precheck() {
    # check spc backup so
    local backup_dir=${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}
	if [ "$cann_package_name" = "ascend-toolkit" ]; then
		dir_check ${backup_dir}/${ANALYSIS} ${ANALYSIS}
		file_check ${backup_dir}/${MSPROF} ${MSPROF}
	fi

	if [ "$cann_package_name" != "nnae" ]; then
		file_check ${backup_dir}/${LIBMSPROFILER_STUB} ${LIBMSPROFILER_STUB}
	fi

	file_check ${backup_dir}/${LIBMSPROFILER} ${LIBMSPROFILER}
    exit 0
}

# use utils function and constant
source $(dirname "$0")/utils.sh

get_cann_package_name
deal_precheck
