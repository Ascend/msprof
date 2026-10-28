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

function unregist_uninstall() {
    if [ -f "${install_path}/cann_uninstall.sh" ]; then
        chmod u+w ${install_path}/cann_uninstall.sh
        remove_uninstall_package "${install_path}/cann_uninstall.sh"
        if [ -f "${install_path}/cann_uninstall.sh" ]; then
            chmod u-w ${install_path}/cann_uninstall.sh
        fi
    fi
}

function remove_uninstall_package() {
    local uninstall_file=$1
    if [ -f "${uninstall_file}" ]; then
        sed -i "/uninstall_package \"${MSPROF_RUN_NAME}\/script\"/d" "${uninstall_file}"
        if [ $? -ne 0 ]; then
            print "ERROR" "remove ${uninstall_file} uninstall_package command failed!"
            exit 1
        fi
    fi
    num=$(grep "^uninstall_package " ${uninstall_file} | wc -l)
    if [ ${num} -eq 0 ]; then
        rm -f "${uninstall_file}" > /dev/null 2>&1
        if [ $? -ne 0 ]; then
            print "ERROR" "delete file: ${uninstall_file}failed, please delete it by yourself."
        fi
    fi
}

function uninstall_product() {
	delete_product ${install_path}/${ANALYSIS_PATH}/${ANALYSIS}
    delete_mindstudio_msprof
    delete_parent_dir
}

function delete_product() {
	local target_path=${1}

	if [ ! -f "$target_path" ] && [ ! -d "$target_path" ]; then
		return
	fi

    local parent_dir=$(dirname ${target_path})
    local right=$(stat -c '%a' ${parent_dir})

	chmod u+w ${parent_dir}

    chmod -R u+w ${target_path}
    rm -rf ${target_path}

	chmod ${right} ${parent_dir}
}

function delete_mindstudio_msprof() {
	if [ ! -d "${install_path}/${MSPROF_RUN_NAME}" ]; then
		return
	fi

    chmod -R u+w ${install_path}/${MSPROF_RUN_NAME}
    rm -rf ${install_path}/${MSPROF_RUN_NAME}
}

function delete_parent_dir(){
    # must from in to out
    remove_dir_by_order ${install_path}/tools/profiler/profiler_tool
    remove_dir_by_order ${install_path}/tools/profiler
    remove_dir_by_order ${install_path}/tools
    remove_dir_by_order ${install_path}
}

function remove_dir_by_order() {
    local dir_name=${1}

	if [ ! -d "${dir_name}" ]; then
        return
	fi

    local parent_dir=$(dirname ${dir_name})
    local parent_dir_right=$(stat -c '%a' ${parent_dir})

    chmod u+w ${parent_dir}
    remove_empty_dir ${dir_name}
    chmod ${parent_dir_right} ${parent_dir}
}

function uninstall_latest() {
    local arch_name="${package_arch}-linux"
    if [ -L "${latest_path}/${arch_name}/bin/${MSPROF}" ]; then
        chmod u+w "${latest_path}/${arch_name}"
        chmod u+w "${latest_path}/${arch_name}/bin"
        rm_file_safe "${latest_path}/${arch_name}/bin/${MSPROF}"
        remove_empty_dir "${latest_path}/${arch_name}/bin"
        if [ -d "${latest_path}/${arch_name}/bin" ]; then
            chmod u-w "${latest_path}/${arch_name}/bin"
        fi
        chmod u-w "${latest_path}/${arch_name}"
    fi

    # runtime has been uninstall
    local leader_package="runtime"

    if [ -L "${latest_path}/${MSPROF_RUN_NAME}" ]; then
        rm_file_safe ${latest_path}/${MSPROF_RUN_NAME}
    fi

    # single version or multi version old
    if [ ! -L "${latest_path}/${leader_package}" ] || [ ! -d "${latest_path}/${leader_package}/../${MSPROF_RUN_NAME}" ]; then
        return
    fi

    # multi version new
    local mindstudio_msprof_abs_path=$(dirname $(readlink -f ${latest_path}/${leader_package}/../${MSPROF_RUN_NAME}))
    local version_name=$(basename ${mindstudio_msprof_abs_path})
    ln -sf ../${version_name}/${MSPROF_RUN_NAME} ${latest_path}/${MSPROF_RUN_NAME}
}

# must use readlink, or can not bash by latest
uninstall_location=$(readlink -f ${0})

source $(dirname ${uninstall_location})/utils.sh

# get path
install_path=$(dirname ${uninstall_location})/../../
if [ ! -d "${install_path}" ]; then
    print "ERROR" "Can not find install path."
    exit 1
fi
install_path="$(
    cd ${install_path}
    pwd
)"

latest_path=$(dirname ${uninstall_location})/../../../latest
if [ ! -d "${latest_path}" ]; then
    print "ERROR" "Can not find latest path."
    exit 1
fi
latest_path="$(
    cd ${latest_path}
    pwd
)"

unregist_uninstall
uninstall_product
uninstall_latest
