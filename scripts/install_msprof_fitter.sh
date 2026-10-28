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

# This script is used to config the libs used by profiling.

CURRENT_DIR="$(dirname "$(readlink -e "$0")")"
INSTALL_PROFILING_HIPROF="${CURRENT_DIR}/install_profiling_msprof.sh"
INI_CONFIG_DIR="$(readlink -f "${CURRENT_DIR}/../tools/profiler/profiler_tool/analysis/msconfig")"
DEFAULT_USERNAME=HwHiAiUser
DEFAULT_USERGROUP=HwHiAiUser
INSTALL_INFO="$(readlink -f "${CURRENT_DIR}/../ascend_install.info")"
INSTALL_FOR_ALL="--install-for-all"
OPTIONS="$1"
ALL_PARAMS=$@

function getInstallParam() {
    local _key=$1
    local _file=$2
    local _param
    local install_info_key_array=("Install_Type" "UserName" "UserGroup" "Install_Path_Param")

    if [ ! -f "${_file}" ];then
        exit 1
    fi

    for key_param in "${install_info_key_array[@]}"; do
        if [ "${key_param}" == "${_key}" ]; then
            _param=$(grep -r "${_key}=" "${_file}" | cut -d"=" -f2-)
            break;
        fi
    done
    echo "${_param}"
}

function refresh_permission()
{
    # refresh permission for ini file
    find "${INI_CONFIG_DIR}" -type f -exec chmod 400 {} \;
    for params in ${ALL_PARAMS}; do
      if [ "${INSTALL_FOR_ALL}" == "${params}" ];then
          # refresh permission for ini file with all user.
          find "${INI_CONFIG_DIR}" -type f -exec chmod 444 {} \;
          break;
      fi
    done
}

function exec_profiling_hiprof()
{
    "${INSTALL_PROFILING_HIPROF}" ${ALL_PARAMS}
}

# get the install user info.
username=$(getInstallParam "UserName" "${INSTALL_INFO}")
usergroup=$(getInstallParam "UserGroup" "${INSTALL_INFO}")
if [ x"${username}" = "x" ]; then
    username=${DEFAULT_USERNAME}
    usergroup=${DEFAULT_USERGROUP}
fi

case ${OPTIONS} in
'--install')
    refresh_permission
    exec_profiling_hiprof
    ;;
*)
    exec_profiling_hiprof
    ;;
esac
