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

# uninstall constant
CUR_DIR=$(dirname $(readlink -f $0))
CANN_INSTALL_PATH=$(readlink -f "${CUR_DIR}/../../..")
if [ -z "${1}" ]; then
 	  install_path=${CANN_INSTALL_PATH}
else
 	  install_path=${1}
fi

function delete_register_uninstall() {
 	if [ ! -f "${install_path}/${CANN_UNINSTALL_SCRIPT}" ]; then
 	  print "ERROR" "Failed to delete_register_uninstall, no such file: ${install_path}/${CANN_UNINSTALL_SCRIPT}"
 	  return 1
 	fi
 	  local script_right=$(stat -c '%a' "${install_path}/${CANN_UNINSTALL_SCRIPT}")
 	  chmod u+w "${install_path}/${CANN_UNINSTALL_SCRIPT}"
 	  sed -i "/uninstall_package \"share\/info\/msprof\"/d" "${install_path}/${CANN_UNINSTALL_SCRIPT}"
 	  chmod ${script_right} "${install_path}/${CANN_UNINSTALL_SCRIPT}"
}

source utils.sh
delete_register_uninstall
remove ${install_path}/${ANALYSIS_PATH}
remove ${install_path}/${SHARE_INFO_DIR}/${MSPROF}
