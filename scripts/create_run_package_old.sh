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

# get real path of parents' dir of this file
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..

# store product
PREFIX_DIR=${TOP_DIR}/build/prefix
MSPROF_TEMP_DIR=${TOP_DIR}/build/msprof_tmp
COMMON_DIR="common_script"
mkdir -p "${MSPROF_TEMP_DIR}/${COMMON_DIR}"

# makeself is tool for compiling run package
MAKESELF_DIR=${TOP_DIR}/opensource/makeself

# footnote for creating run package
CREATE_RUN_SCRIPT=${MAKESELF_DIR}/makeself.sh

# footnote for controlling params
CONTROL_PARAM_SCRIPT=${MAKESELF_DIR}/makeself-header.sh

# store run package
OUTPUT_DIR=${TOP_DIR}/output
mkdir -p "${OUTPUT_DIR}"

RUN_SCRIPT_DIR=${TOP_DIR}/scripts/run_script
FILTER_PARAM_SCRIPT=${RUN_SCRIPT_DIR}/help.conf
MAIN_SCRIPT=main.sh
INSTALL_SCRIPT=install.sh
UTILS_SCRIPT=utils.sh
CANN_UNINSTALL=cann_uninstall.sh
UNINSTALL=uninstall.sh

# script for spc
MAIN_SPC=main_spc.sh
FILTER_PARAM_SCRIPT_SPC=${RUN_SCRIPT_DIR}/help_spc.conf
BACKUP=backup.sh
ROLLBACK_PRECHECK=rollback_precheck.sh
ROLLBACK_SPC=rollback_spc.sh
UNINSTALL_SPC=uninstall_spc.sh
COMMON_ROLLBACK=rollback.sh
COMMON_UNINSTALL=uninstall.sh

MSPROF_RUN_NAME="mindstudio-msprof"
version="unknown"
package_type="unknown"

PKG_LIMIT_SIZE=524288000 # 500M

function parse_script_args() {
    while true; do
        if [ "$1" = "" ]; then
            break
        fi
        case "$1" in
        version_dir=*)
            version=${1#version_dir=}
            shift
            continue
            ;;
        Patch)
            package_type="Patch"
            shift
            continue
            ;;
        *)
            echo "[ERROR]" "Input option is invalid"
            exit 1
            ;;
        esac
    done
}

# build python whl
function build_python_whl() {
  cd ${TOP_DIR}/build/build
  python3  ${TOP_DIR}/build/setup_mspti.py bdist_wheel --python-tag=py3 --py-limited-api=cp37
  rm -rf ${TOP_DIR}/mspti.egg-info
  cd -
}

# create temp dir for product
function create_temp_dir() {
    local temp_dir=${1}
    local collector_dir=${PREFIX_DIR}/collector
    local mspti_dir=${PREFIX_DIR}/mspti
    local profcommon_dir=${PREFIX_DIR}/profcommon

    cp ${collector_dir}/lib/libmsprofiler.so ${temp_dir}
    cp -r ${collector_dir}/stub ${temp_dir}
    cp ${collector_dir}/bin/msprof ${temp_dir}
    cp ${mspti_dir}/libmspti.so ${temp_dir}
    cp ${profcommon_dir}/libprof_common.so ${temp_dir}
    cp ${TOP_DIR}/build/build/dist/mspti-0.0.1-py3-none-any.whl ${temp_dir}
    cp ${TOP_DIR}/collector/inc/external/acl/acl_prof.h ${temp_dir}
    cp ${TOP_DIR}/collector/inc/external/ge/ge_prof.h ${temp_dir}
    cp ${TOP_DIR}/mspti/external/mspti.h ${temp_dir}
    cp ${TOP_DIR}/mspti/external/mspti_activity.h ${temp_dir}
    cp ${TOP_DIR}/mspti/external/mspti_callback.h ${temp_dir}
    cp ${TOP_DIR}/mspti/external/mspti_cbid.h ${temp_dir}
    cp ${TOP_DIR}/mspti/external/mspti_result.h ${temp_dir}
    cp -r ${TOP_DIR}/mspti/samples ${temp_dir}
    mkdir -p ${temp_dir}/mstx_samples
    cp -r ${TOP_DIR}/samples ${temp_dir}/mstx_samples
    cp -r ${TOP_DIR}/analysis ${temp_dir}
    rm -rf ${temp_dir}/analysis/csrc
    if [ "${package_type}" = "Patch" ]; then
        # if we want to change product, we also need to change rollback_precheck
        copy_script ${MAIN_SPC} ${temp_dir}
        copy_script ${BACKUP} ${temp_dir}
        copy_script ${ROLLBACK_PRECHECK} ${temp_dir}
        copy_script ${ROLLBACK_SPC} ${temp_dir}
        copy_script ${UNINSTALL_SPC} ${temp_dir}
        copy_script ${COMMON_DIR}/${COMMON_ROLLBACK} ${temp_dir}
        copy_script ${COMMON_DIR}/${COMMON_UNINSTALL} ${temp_dir}
    else
        copy_script ${MAIN_SCRIPT} ${temp_dir}
        copy_script ${COMMON_DIR}/${COMMON_UNINSTALL} ${temp_dir}
        copy_script ${UNINSTALL} ${temp_dir}
    fi
    copy_script ${INSTALL_SCRIPT} ${temp_dir}
    copy_script ${UTILS_SCRIPT} ${temp_dir}
}

# copy script
function copy_script() {
    local script_name=${1}
    local temp_dir=${2}

    if [ -f "${temp_dir}/${script_name}" ]; then
        rm -f "${temp_dir}/${script_name}"
    fi

    cp ${RUN_SCRIPT_DIR}/${script_name} ${temp_dir}/${script_name}
    chmod 500 "${temp_dir}/${script_name}"
}

function version() {
    local path="${TOP_DIR}/../manifest/dependency/config.ini"
    local version=$(grep "^version=" "${path}" | cut -d"=" -f2)
    echo "${version}"
}

function get_package_name() {
    local product="Ascend"
    local name=${MSPROF_RUN_NAME}

    local version=$(echo $(version) | cut -d '.' -f 1,2,3)
    local os_arch=$(arch)
    echo "${product}-${name}_${version}_linux-${os_arch}.run"
}

function create_run_package() {
    local run_name=${1}
    local temp_dir=${2}
    local main_script=${3}
    local filer_param=${4}
    local package_name=$(get_package_name)

    ${CREATE_RUN_SCRIPT} \
    --header ${CONTROL_PARAM_SCRIPT} \
    --help-header ${filer_param} \
    --pigz \
    --tar-quietly \
    --complevel 4 \
    --nomd5 \
    --sha256 \
    --chown \
    ${temp_dir} \
    ${OUTPUT_DIR}/${package_name} \
    ${run_name} \
    ./${main_script}
}

function sed_param() {
    local main_script=${1}
    sed -i "2i VERSION=$version" "${RUN_SCRIPT_DIR}/${main_script}"
    sed -i "2i package_arch=$(arch)" "${RUN_SCRIPT_DIR}/${main_script}"
    build_python_whl
    if [ "${main_script}" = "${MAIN_SCRIPT}" ]; then
        sed -i "2i package_arch=$(arch)" "${RUN_SCRIPT_DIR}/${UNINSTALL}"
    fi
}

function delete_sed_param() {
    local main_script=${1}
    sed -i "2d" "${RUN_SCRIPT_DIR}/${main_script}"
    sed -i "2d" "${RUN_SCRIPT_DIR}/${main_script}"

    if [ "${main_script}" = "${MAIN_SCRIPT}" ]; then
        sed -i "2d" "${RUN_SCRIPT_DIR}/${UNINSTALL}"
    fi
}

function sed_main_param() {
	local main_script=${1}
	local filer=${2}
	sed_param ${main_script}
	create_temp_dir ${MSPROF_TEMP_DIR}
	check_file_exist ${MSPROF_TEMP_DIR}
	create_run_package ${MSPROF_RUN_NAME} ${MSPROF_TEMP_DIR} ${main_script} ${filer}
	check_package ${OUTPUT_DIR}/$(get_package_name) ${PKG_LIMIT_SIZE}
	delete_sed_param ${main_script}
}

check_file_exist() {
  local temp_dir=${1}
  check_package ${temp_dir}/acl_prof.h ${PKG_LIMIT_SIZE}
  check_package ${temp_dir}/ge_prof.h ${PKG_LIMIT_SIZE}
  check_package ${temp_dir}/msprof ${PKG_LIMIT_SIZE}
  check_package ${temp_dir}/libmsprofiler.so ${PKG_LIMIT_SIZE}
  check_package ${temp_dir}/libmsprofiler.so ${PKG_LIMIT_SIZE}
  check_package ${temp_dir}/mspti-0.0.1-py3-none-any.whl ${PKG_LIMIT_SIZE}
  check_package ${temp_dir}/analysis
  check_package ${temp_dir}/analysis/lib64/msprof_analysis.so
  check_package ${temp_dir}/${MAIN_SCRIPT}
  check_package ${temp_dir}/${COMMON_DIR}/${COMMON_UNINSTALL}
  check_package ${temp_dir}/${UNINSTALL}
  check_package ${temp_dir}/${UTILS_SCRIPT} ${PKG_LIMIT_SIZE}
  check_package ${temp_dir}/libmspti.so ${PKG_LIMIT_SIZE}
  check_package ${temp_dir}/mspti.h ${PKG_LIMIT_SIZE}
  check_package ${temp_dir}/mspti_activity.h ${PKG_LIMIT_SIZE}
  check_package ${temp_dir}/mspti_callback.h ${PKG_LIMIT_SIZE}
  check_package ${temp_dir}/mspti_cbid.h ${PKG_LIMIT_SIZE}
  check_package ${temp_dir}/mspti_result.h ${PKG_LIMIT_SIZE}
  check_package ${temp_dir}/samples ${PKG_LIMIT_SIZE}
  check_package ${temp_dir}/mstx_samples/samples ${PKG_LIMIT_SIZE}
  check_package ${temp_dir}/libprof_common.so ${PKG_LIMIT_SIZE}
}

function check_package() {
    local _path="$1"
    local _limit_size=$2
    echo "check ${_path} exists"
    # 检查路径是否存在
    if [ ! -e "${_path}" ]; then
        echo "${_path} does not exist."
        exit 1
    fi

    # 检查路径是否为文件
    if [ -f "${_path}" ]; then
        local _file_size=$(stat -c%s "${_path}")
        # 检查文件大小是否超过限制
        if [ "${_file_size}" -gt "${_limit_size}" ] || [ "${_file_size}" -eq 0 ]; then
            echo "package size exceeds limit:${_limit_size}"
            exit 1
        fi
    fi
}

parse_script_args $*

if [ "${package_type}" = "Patch" ];
    then
        sed_main_param ${MAIN_SPC} ${FILTER_PARAM_SCRIPT_SPC}
    else
        sed_main_param ${MAIN_SCRIPT} ${FILTER_PARAM_SCRIPT}
fi
