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
OPTIONS="$1"
CURRENT_USER=HwHiAiUser
INSTALL_INFO="$(readlink -f "${CURRENT_DIR}/../ascend_install.info")"
SUCCESS=0
ERROR=1

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

function install()
{
    profiling_info "Install profiling successfully."
}

function uninstall()
{
    profiling_info "Uninstall profiling successfully."
}

function profiling_log()
{
    local msg_level=$1
    local log_msg=$2
    local format_time=$(date "+%Y-%m-%d %H:%M:%S")
    echo "[Toolkit] [${format_time}] [${msg_level}] [Profiler]: ${log_msg}"
}

function profiling_info()
{
    profiling_log "INFO" "$1"
}

function profiling_warning()
{
    profiling_log "WARNING" "$1"
}

function profiling_error()
{
    profiling_log "ERROR" "$1"
}

function usage()
{
    echo
    echo "Usage: install_profiling_hiprof.sh <OPTIONS> "
    echo "--help                  : print help info."
    echo "--install               : install all modules."
    echo "--uninstall             : uninstall all installed modules."
    echo
    echo "Examples:"
    echo "./install_profiling_hiprof.sh --install"
    echo
}

# get the install user info.
username=$(getInstallParam "UserName" "${INSTALL_INFO}")
if [ x"${user_install_path}" = "x" ]; then
    username=${CURRENT_USER}
fi

case ${OPTIONS} in
'--install')
    install
    exit ${SUCCESS}
    ;;
'--uninstall')
    uninstall
    exit ${SUCCESS}
    ;;
*)
    usage
    exit ${ERROR}
    ;;
esac

exit ${SUCCESS}
