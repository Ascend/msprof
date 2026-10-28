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

# get install path
install_path="$(
    cd "$(dirname "$0")/../../../"
    pwd
)"

# script for spc
UNINSTALL_SCRIPT="uninstall.sh"

function deal_rollback() {
	local backup_dir=${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}
    copy_file ${backup_dir}/${LIBMSPROFILER} ${install_path}/${LIBMSPROFILER_PATH}/${LIBMSPROFILER}
    copy_file ${backup_dir}/${ANALYSIS} ${install_path}/${ANALYSIS_PATH}/${ANALYSIS}
	copy_file ${backup_dir}/${MSPROF} ${install_path}/${MSPROF_PATH}/${MSPROF}
	copy_file ${backup_dir}/${LIBMSPROFILER_STUB} ${install_path}/${LIBMSPROFILER_PATH}/${LIBMSPROFILER_STUB}

	# uninstall backup
	uninstall_backup
	print "INFO" "${MSPROF_RUN_NAME} rollback successfully !"
}

function copy_file() {
	local filename=${1}
	local target_file=$(readlink -f ${2})
	if [ ! -f "$filename" ] && [ ! -d "$filename" ]; then
		return
	fi

	if [ -f "$target_file" ] || [ -d "$target_file" ]; then
		local target_parent_dir=$(dirname ${target_file})
		local parent_right=$(stat -c '%a' ${target_parent_dir})

		chmod u+w ${target_parent_dir}
		chmod -R u+w ${target_file}
		rm -rf ${target_file}

		cp -rp ${filename} ${target_file}

		chmod ${parent_right} ${target_parent_dir}

		print "INFO" "$filename is replaced."
		return
	fi
}

# use utils function and constant
source $(dirname "$0")/utils.sh

deal_rollback
