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

# the params for checking
install_args_num=0
install_path_num=0
install_for_all_flag=0

function parse_script_args() {
    while true; do
		if [ "$3" = "" ]; then
			break
		fi
        case "$3" in
        --install-path=*)
			let "install_path_num+=1"
			install_path=${3#--install-path=}/${VERSION}
            check_path ${install_path}
			install_path=$(readlink -f ${install_path})
			check_path ${install_path}
			shift
			continue
            ;;
        --full)
            let "install_args_num+=1"
            shift
			continue
            ;;
        --quiet)
            shift
			continue
            ;;
        --nox11)
            shift
			continue
            ;;
        *)
			print "ERROR" "Input option is invalid. Please try --help."
			exit 1
            ;;
        esac
    done
}

function check_args() {
	if [ "$install_path_num" = "1" ] && [ "$install_args_num" = "1" ]; then
        return
	fi

    print "ERROR" "Input option is invalid. Please try --help."
    exit 1
}

function execute_backup() {
    bash backup.sh ${install_path}
}

function execute_install() {
    bash install.sh ${install_path} ${package_arch} ${install_for_all_flag}
}

function get_install_flag() {
    local libmsprofiler_path=$(readlink -f ${install_path}/${LIBMSPROFILER_PATH}/${LIBMSPROFILER})
    local libmsprofiler_right=$(stat -c '%a' ${libmsprofiler_path})
    if [ ${libmsprofiler_right} = ${root_right} ]; then
        install_for_all_flag=1
    fi
}

# use utils function and constant
source utils.sh
parse_script_args $*
get_install_flag
check_args
execute_backup
execute_install
print "INFO" "${MSPROF_RUN_NAME} package install success."
