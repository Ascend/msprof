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

# This script is used to replace msprofbin&&libmsprofiler.so&&stub/libmsprofiler.so&&analysis in CANN with those in Ascend_mindstudio_toolkit,
# and repack CANN components to .run package

set -e


CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
OUT_DIR=${TOP_DIR}/..

if [ -e "${CUR_DIR}/tmp" ]
then
    rm -rf ${CUR_DIR}/tmp
fi

# binary check
function bep_env_init() {
    source /etc/profile
    local bep_env_config=${CUR_DIR}/bep/bep_env.conf
    local bep_sh=$(which bep_env.sh)
    echo "has bep sh :${bep_sh}"
    if [ ! -d "${SECBEPKIT_HOME}" ] && [ ! -f "$bep_sh" ]; then
        echo "BepKit is not installed, Please install the tool and configure the env var \$SECBEPKIT_HOME"
    else
        source ${SECBEPKIT_HOME}/bep_env.sh -s $bep_env_config
        if [ $? -ne 0 ]; then
            echo "build bep failed!"
            exit 1
        else
            echo "build bep success."
        fi
    fi
}

function create_run_package() {
  	local cann_component_dir=${1}
	  local cann_component_name=${2}
	  local package_name_suffix=${3}

	  ${CREATE_RUN_SCRIPT} \
    --header ${CONTROL_PARAM_SCRIPT} --help-header ${cann_component_dir}/${cann_component_name}/scripts/help.info --pigz --complevel 4 --nomd5 --sha256 \
    --nooverwrite --chown --tar-format gnu --tar-extra --numeric-owner --tar-quietly \
    ${cann_component_dir} \
    ${CUR_DIR}/tmp/${package_name_suffix} \
    ${COMMENTS} \
    ./${cann_component_name}/scripts/install.sh
    mv ${CUR_DIR}/tmp/${package_name_suffix} ${RM_DIR}/${package_name_suffix}
}

bep_env_init

MINDSTUDIO_TOOLKIT_NAME=Ascend-mindstudio-toolkit*.run
CANN_TOOLKIT_NAME=CANN-toolkit-*.run
CANN_RUNTIME_NAME=CANN-runtime-*linux*.run

# makeself is tool for compiling run package
# footnote for creating run package
CREATE_RUN_SCRIPT=${TOP_DIR}/opensource/makeself/makeself.sh
# footnote for controlling params
CONTROL_PARAM_SCRIPT=${TOP_DIR}/opensource/makeself/makeself-header.sh
COMMENTS=comments

# only find x86 or ARM
for DIR in ${OUT_DIR}/platform/Tuscany/*$(lsb_release -i | awk '{print $3}' | tr 'A-Z' 'a-z')*;
  do
    RM_DIR=${DIR}
    mkdir -p ${CUR_DIR}/tmp

    cp ${RM_DIR}/${MINDSTUDIO_TOOLKIT_NAME} ${CUR_DIR}/tmp
    PACKAGE_NAME_MINDSTUDIO_SUFFIX=$(ls ${CUR_DIR}/tmp)
    mkdir -p ${CUR_DIR}/tmp/cann_toolkit
    mkdir -p ${CUR_DIR}/tmp/cann_runtime
    cp ${RM_DIR}/${CANN_TOOLKIT_NAME} ${CUR_DIR}/tmp/cann_toolkit
    cp ${RM_DIR}/${CANN_RUNTIME_NAME} ${CUR_DIR}/tmp/cann_runtime
    chmod -R +x ${CUR_DIR}/tmp
    package_name_toolkit_suffix=$(ls ${CUR_DIR}/tmp/cann_toolkit)
    package_name_runtime_suffix=$(ls ${CUR_DIR}/tmp/cann_runtime)
    PACKAGE_NAME_TOOLKIT=$(basename $package_name_toolkit_suffix .run)
    PACKAGE_NAME_RUNTIME=$(basename $package_name_runtime_suffix .run)

    ${CUR_DIR}/tmp/$MINDSTUDIO_TOOLKIT_NAME --noexec --extract=${CUR_DIR}/tmp/mindstudio
    ${CUR_DIR}/tmp/cann_toolkit/$CANN_TOOLKIT_NAME --noexec --extract=${CUR_DIR}/tmp/cann_toolkit/${PACKAGE_NAME_TOOLKIT}
    ${CUR_DIR}/tmp/cann_runtime/$CANN_RUNTIME_NAME --noexec --extract=${CUR_DIR}/tmp/cann_runtime/${PACKAGE_NAME_RUNTIME}

    cp -f ${CUR_DIR}/tmp/mindstudio/mindstudio-toolkit/tools/msprof/stub/libmsprofiler.so ${CUR_DIR}/tmp/cann_runtime/${PACKAGE_NAME_RUNTIME}/runtime/lib64/stub/libmsprofiler.so
    cp -f ${CUR_DIR}/tmp/mindstudio/mindstudio-toolkit/tools/msprof/libmsprofiler.so ${CUR_DIR}/tmp/cann_runtime/${PACKAGE_NAME_RUNTIME}/runtime/lib64/libmsprofiler.so
    cp -f ${CUR_DIR}/tmp/mindstudio/mindstudio-toolkit/tools/mspti/lib64/libmspti.so ${CUR_DIR}/tmp/cann_runtime/${PACKAGE_NAME_RUNTIME}/runtime/lib64/libmspti.so
    cp -f ${CUR_DIR}/tmp/mindstudio/mindstudio-toolkit/tools/msprof/msprof ${CUR_DIR}/tmp/cann_toolkit/${PACKAGE_NAME_TOOLKIT}/toolkit/tools/profiler/bin/msprof
    rm -rf ${CUR_DIR}/tmp/cann_toolkit/${PACKAGE_NAME_TOOLKIT}/toolkit/tools/profiler/profiler_tool/msprof-0.0.1-py3-none-any.whl
    cp -f ${CUR_DIR}/tmp/mindstudio/mindstudio-toolkit/tools/msprof/msprof-0.0.1-py3-none-any.whl \
    ${CUR_DIR}/tmp/cann_toolkit/${PACKAGE_NAME_TOOLKIT}/toolkit/tools/profiler/profiler_tool/msprof-0.0.1-py3-none-any.whl
    cp -f ${CUR_DIR}/tmp/mindstudio/mindstudio-toolkit/tools/msprof/acl_prof.h ${CUR_DIR}/tmp/cann_runtime/${PACKAGE_NAME_RUNTIME}/runtime/include/acl/acl_prof.h

    mkdir -p ${CUR_DIR}/tmp/cann_runtime/${PACKAGE_NAME_RUNTIME}/runtime/include/mspti
    cp -f ${CUR_DIR}/tmp/mindstudio/mindstudio-toolkit/tools/mspti/include/mspti.h ${CUR_DIR}/tmp/cann_runtime/${PACKAGE_NAME_RUNTIME}/runtime/include/mspti/mspti.h
    cp -f ${CUR_DIR}/tmp/mindstudio/mindstudio-toolkit/tools/mspti/include/mspti_activity.h ${CUR_DIR}/tmp/cann_runtime/${PACKAGE_NAME_RUNTIME}/runtime/include/mspti/mspti_activity.h
    cp -f ${CUR_DIR}/tmp/mindstudio/mindstudio-toolkit/tools/mspti/include/mspti_callback.h ${CUR_DIR}/tmp/cann_runtime/${PACKAGE_NAME_RUNTIME}/runtime/include/mspti/mspti_callback.h
    cp -f ${CUR_DIR}/tmp/mindstudio/mindstudio-toolkit/tools/mspti/include/mspti_cbid.h ${CUR_DIR}/tmp/cann_runtime/${PACKAGE_NAME_RUNTIME}/runtime/include/mspti/mspti_cbid.h
    cp -f ${CUR_DIR}/tmp/mindstudio/mindstudio-toolkit/tools/mspti/include/mspti_result.h ${CUR_DIR}/tmp/cann_runtime/${PACKAGE_NAME_RUNTIME}/runtime/include/mspti/mspti_result.h

    create_run_package ${CUR_DIR}/tmp/cann_toolkit/${PACKAGE_NAME_TOOLKIT} toolkit ${package_name_toolkit_suffix}
    create_run_package ${CUR_DIR}/tmp/cann_runtime/${PACKAGE_NAME_RUNTIME} runtime ${package_name_runtime_suffix}

    MINDSTUDIO_TOOLKIT=${CUR_DIR}/tmp/mindstudio/mindstudio-toolkit

    filelist_csv=${MINDSTUDIO_TOOLKIT}/script/filelist.csv
    file_interval=${MINDSTUDIO_TOOLKIT}/script/file_interval.csv
    awk -F, '$4 !~ /msprof/ || $2 !~ /mkdir/' ${filelist_csv} > ${file_interval}
    awk -F, '$1 !~ /msprof/' ${file_interval} > ${filelist_csv}
    rm -f $file_interval

    ${CREATE_RUN_SCRIPT} \
    --header ${CONTROL_PARAM_SCRIPT}\
    --help-header ${MINDSTUDIO_TOOLKIT}/script/help.info \
    --complevel 4 \
    --nomd5 \
    --sha256 \
    --chown \
    --tar-quietly \
    ${CUR_DIR}/tmp/mindstudio \
    ${CUR_DIR}/tmp/${PACKAGE_NAME_MINDSTUDIO_SUFFIX} \
    ${COMMENTS} \
    ./mindstudio-toolkit/script/install.sh
    mv ${CUR_DIR}/tmp/${PACKAGE_NAME_MINDSTUDIO_SUFFIX} ${RM_DIR}/${PACKAGE_NAME_MINDSTUDIO_SUFFIX}

    rm -rf ${CUR_DIR}/tmp
  done
