#!/bin/bash
# right constant
root_right=755
user_right=750

root_ini_right=444
user_ini_right=400

script_right=500

mindstudio_msprof_spc_right=500

root_libmsprofiler_right=444
user_libmsprofiler_right=440
package_arch=$(uname -m)
PATH_LENGTH=4096

MSPROF_RUN_NAME="mindstudio-profiler"
# product constant
LIB_MS_PROFILER="libmsprofiler.so"
LIB_PROF_API="libprofapi.so"
LIB_PROF_IMPL="libprofimpl.so"
ACL_PROF_H="acl_prof.h"
GE_PROF_H="ge_prof.h"

# never use analysis/, or remove important file by softlink
ANALYSIS="analysis"
MSPROF="msprof"
SAMPLE="samples"

ANALYSIS_PATH="tools/profiler/profiler_tool"
SAMPLE_PATH="tools/mstx/samples"
MSPROF_PATH="tools/profiler/bin"
SHARE_INFO_DIR="share/info"
UNINSTALL_SCRIPT="uninstall.sh"
UTILS_SCRIPT="utils.sh"
CANN_UNINSTALL_SCRIPT="cann_uninstall.sh"
VERSION_INFO="version.info"
PLATFORM_PROFILER_RUN="platform-profiler.run"

function print() {
    if [ ! -f "$log_file" ]; then
        echo "[${MSPROF_RUN_NAME}] [$(date +"%Y-%m-%d %H:%M:%S")] [$1]: $2"
    else
        echo "[${MSPROF_RUN_NAME}] [$(date +"%Y-%m-%d %H:%M:%S")] [$1]: $2" | tee -a $log_file
    fi
}

function remove() {
 	  local target_path=${1}
 	  if [ ! -d "${target_path}" ] && [ ! -f "${target_path}" ]; then
 	      return
 	  fi
 	  local parent_dir=$(dirname ${target_path})
 	  local parent_right=$(stat -c '%a' ${parent_dir})
 	  chmod u+wx ${parent_dir}
 	  chmod -R u+wx ${target_path}
 	  rm -rf ${target_path}
 	  chmod ${parent_right} ${parent_dir}
}

function get_log_file() {
    local log_dir
    if [ "$UID" = "0" ]; then
		    log_dir="/var/log/ascend_seclog"
	  else
		    log_dir="${HOME}/var/log/ascend_seclog"
	  fi
	  mkdir -p "${log_dir}"
	  echo "${log_dir}/ascend_install.log"
}

function log_init() {
    if [ ! -f "$log_file" ]; then
        touch $log_file
        if [ $? -ne 0 ]; then
            print "ERROR" "touch $log_file permission denied"
            exit 1
        fi
    fi
    chmod 640 $log_file
}

function check_path() {
    local path_str=${1}
    # check the existence of the path
    if [ ! -e "${path_str}" ]; then
        print "ERROR" "The path ${path_str} does not exist, please check."
        exit 1
    fi
    # check the length of path
    if [ ${#path_str} -gt ${PATH_LENGTH} ]; then
        print "ERROR" "parameter error $path_str, the length exceeds ${PATH_LENGTH}."
        exit 1
    fi
    # check absolute path
    if [[ ! "${path_str}" =~ ^/.* ]]; then
        print "ERROR" "parameter error $path_str, must be an absolute path."
        exit 1
    fi
    # black list
    if echo "${path_str}" | grep -Eq '/{2,}|\.{3,}'; then
        print "ERROR" "The path ${path_str} is invalid, cannot contain the following characters: // ...!"
        exit 1
    fi
    # white list
    if echo "${path_str}" | grep -Eq '^~?[a-zA-Z0-9./_-]*$'; then
        return
    else
        print "ERROR" "The path ${path_str} is invalid, only [a-z,A-Z,0-9,-,_] is support!"
        exit 1
    fi
}

function check_valid_cann_path() {
    local cann_path=${1}
    local cann_dir_name=$(basename -- "${cann_path}")

    if [[ ! "${cann_dir_name}" =~ ^cann(-[a-zA-Z0-9._-]+)?$ ]]; then
        return 1
    fi

    if [ ! -d "${cann_path}/tools" ] ||
       [ ! -d "${cann_path}/runtime" ] ||
       [ ! -d "${cann_path}/opp" ]; then
        return 1
    fi
    return 0
}

function check_cann_path() {
    local cann_path=${1}
    # Check the installation path is a valid CANN path.
    if ! check_valid_cann_path "${cann_path}"; then
        print "ERROR" "Invalid CANN installation path: ${cann_path}. The directory name must be 'cann' or start with 'cann-', for example, '/usr/local/Ascend/cann-9.2.0'."
        exit 1
    fi

    local current_user=$(whoami)
    local cann_path_owner=$(stat -c '%U' "$cann_path")

    if [ "$current_user" != "root" ]; then
        # 1. Check the current executing user and the installation user of cann_path.
        if [ "$current_user" != "$cann_path_owner" ]; then
            print "ERROR" "Current user ($current_user) is not the same as the owner of cann_path ($cann_path_owner)."
            exit 1
        fi

        # 2. Check whether the current cann_path is writable (except for the root user).
        if [ ! -w "$cann_path" ]; then
            print "ERROR" "cann_path ($cann_path) is not writable by the current user ($current_user)."
            exit 1
        fi
    fi
}

# init log file
log_file=$(get_log_file)
log_init
