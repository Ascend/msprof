#!/bin/bash
# install constant
install_path=${1}
package_arch=${2}
install_for_all_flag=${3}
pylocal=y

function get_right() {
    if [ "$install_for_all_flag" = 1 ] || [ "$UID" = "0" ]; then
        right=${root_right}
    else
        right=${user_right}
    fi
}

function install_whl_package() {
    local _pylocal=$1
    local _package=$2
    local _pythonlocalpath=$3

    print "INFO" "Start to install ${_package}."
    if [ ! -f "${_package}" ]; then
        print "ERROR" "Install whl The ${_package} does not exist."
        return 1
    fi
    if [ "-${_pylocal}" = "-y" ]; then
        pip3 install --upgrade --no-deps --force-reinstall --disable-pip-version-check "${_package}" -t "${_pythonlocalpath}" > /dev/null 2>&1
    else
        if [ "$(id -u)" -ne 0 ]; then
            pip3 install --upgrade --no-deps --force-reinstall --disable-pip-version-check "${_package}" --user > /dev/null 2>&1
        else
            pip3 install --upgrade --no-deps --force-reinstall --disable-pip-version-check "${_package}" > /dev/null 2>&1
        fi
    fi
    if [ $? -ne 0 ]; then
        print "ERROR" "Install ${_package} failed."
        return 1
    fi
    chmod -R u+rwx,go+rx,go-w "${_pythonlocalpath}"
    print "INFO" "Install ${_package} success."
    return 0
}

function chmod_dir_with_backup() {
  local _dir=${1}
  local _right_var=${2}
  if [ ! -d "${_dir}" ]; then
      return
  fi
  local _origin_right=$(stat -c '%a' "${_dir}")
  eval "${_right_var}=${_origin_right}"
  chmod 755 "${_dir}"
}

function restore_dir_right() {
  local _dir=${1}
  local _origin_right=${2}
  if [ -n "${_origin_right}" ] && [ -d "${_dir}" ]; then
      chmod "${_origin_right}" "${_dir}"
  fi
}

function implement_install() {
  create_directory ${install_path}/${MSPROF_PATH} ${right}
  create_directory ${install_path}/${arch_name}/lib64 ${right}
  create_directory ${install_path}/${arch_name}/include/acl ${right}
  create_directory ${install_path}/${arch_name}/include/ge ${right}
  create_directory ${install_path}/${ANALYSIS_PATH} ${right}
  create_directory ${install_path}/${SHARE_INFO_DIR}/${MSPROF} ${right}
  create_directory ${install_path}/${SAMPLE_PATH} ${right}
  # 1. uninstall.sh
  copy_file ${UNINSTALL_SCRIPT} ${install_path}/${SHARE_INFO_DIR}/${MSPROF}/${UNINSTALL_SCRIPT}
  # 2. utils.sh
  copy_file ${UTILS_SCRIPT} ${install_path}/${SHARE_INFO_DIR}/${MSPROF}/${UTILS_SCRIPT}
  # 3. version info
  copy_file ${VERSION_INFO} ${install_path}/${SHARE_INFO_DIR}/${MSPROF}/${VERSION_INFO}
	# 4. install collector
	# msprof bin
	copy_file ${MSPROF} ${install_path}/${MSPROF_PATH}/${MSPROF}
	# libmsprofiler.so
	copy_file ${LIB_MS_PROFILER} ${install_path}/${arch_name}/lib64/${LIB_MS_PROFILER}
	# libprofapi.so
	copy_file ${LIB_PROF_API} ${install_path}/${arch_name}/lib64/${LIB_PROF_API}
	# libprofimpl.so
	copy_file ${LIB_PROF_IMPL} ${install_path}/${arch_name}/lib64/${LIB_PROF_IMPL}
	# acl_prof.h
	copy_file ${ACL_PROF_H} ${install_path}/${arch_name}/include/acl/${ACL_PROF_H}
	# ge_prof.h
	copy_file ${GE_PROF_H} ${install_path}/${arch_name}/include/ge/${GE_PROF_H}
	# samples
	copy_file ${SAMPLE} ${install_path}/${SAMPLE_PATH}

	# 5. install analyse
 	chmod -R 755 ${install_path}/${ANALYSIS_PATH}
	copy_file ${ANALYSIS} ${install_path}/${ANALYSIS_PATH}/${ANALYSIS}
	source_whl=$(ls msprof-*-py3-none-any.whl 2>/dev/null | head -1)
  if [ -f "$source_whl" ]; then
      rm -f "${install_path}/${ANALYSIS_PATH}"/msprof-*-py3-none-any.whl
      cp "$source_whl" "${install_path}/${ANALYSIS_PATH}"
      install_whl_package $pylocal ${source_whl} ${install_path}/${ANALYSIS_PATH}
      chmod -R 755 ${install_path}/${ANALYSIS_PATH}
  fi
  if [ $? -ne 0 ]; then
      print "ERROR" "Install msprof analysis whl failed."
      return 1
  fi
  # 6. install plat_form.run
  if [[ -s "${PLATFORM_PROFILER_RUN}" ]]; then
    local lib64_dir="${install_path}/${arch_name}/lib64"
    local msprof_path_dir="${install_path}/${MSPROF_PATH}"
    local lib64_origin_right=""
    local msprof_path_origin_right=""
    chmod_dir_with_backup "${lib64_dir}" lib64_origin_right
    chmod_dir_with_backup "${msprof_path_dir}" msprof_path_origin_right
    chmod u+x "${PLATFORM_PROFILER_RUN}"
    if ./"${PLATFORM_PROFILER_RUN}" --install --install-path="${install_path}" > /dev/null 2>&1; then
        print "INFO" "${PLATFORM_PROFILER_RUN} installed successfully to ${install_path}"
    else
        print "ERROR" "${PLATFORM_PROFILER_RUN} installation failed."
    fi
    restore_dir_right "${lib64_dir}" "${lib64_origin_right}"
    restore_dir_right "${msprof_path_dir}" "${msprof_path_origin_right}"
  fi
}

function create_directory() {
  local _dir=${1}
 	local _right=${2}
 	if [ ! -d "${_dir}" ]; then
 	    mkdir -p ${_dir}
 	    chmod ${_right} ${_dir}
 	fi
}

function copy_file() {
	local filename=${1}
	local target_file=$(readlink -f ${2})

	if [ ! -f "$filename" ] && [ ! -d "$filename" ]; then
		return
	fi

	if [ -f "$target_file" ] || [ -d "$target_file" ]; then
		local parent_dir=$(dirname ${target_file})
		local parent_right=$(stat -c '%a' ${parent_dir})

		chmod u+w ${parent_dir}
		chmod -R u+w ${target_file}
		rm -r ${target_file}

		cp -r ${filename} ${target_file}
		chmod -R ${parent_right} ${target_file}
		chmod ${parent_right} ${parent_dir}
	else
 	  cp -r ${filename} ${target_file}
 	  chmod -R ${right} ${target_file}
 	fi
	print "INFO" "$filename is replaced."
}

function chmod_ini_file() {
	local ini_config_dir=${install_path}${ANALYSIS_PATH}${ANALYSIS}"/config"
	if [ -d "$ini_config_dir" ]; then
		if [ "$install_for_all_flag" = "1" ] || [ "$UID" = "0" ]; then
			find "${ini_config_dir}" -type f -exec chmod ${root_ini_right} {} \;
		else
			find "${ini_config_dir}" -type f -exec chmod ${user_ini_right} {} \;
		fi
	fi
}

function set_libmsprofiler_right() {
	libmsprofiler_right=${user_libmsprofiler_right}
	if [ "$install_for_all_flag" = "1" ] || [ "$UID" = "0" ]; then
		libmsprofiler_right=${root_libmsprofiler_right}
	fi
}

function chmod_libmsprofiler() {

	if [ -f "${install_path}/${arch_name}/lib64/${LIB_MS_PROFILER}" ]; then
		chmod ${libmsprofiler_right} "${install_path}/${arch_name}/lib64/${LIB_MS_PROFILER}"
	fi

	if [ -f "${install_path}/${arch_name}/lib64/${LIB_PROF_API}" ]; then
		chmod ${libmsprofiler_right} "${install_path}/${arch_name}/lib64/${LIB_PROF_API}"
	fi

	if [ -f "${install_path}/${arch_name}/lib64/${LIB_PROF_IMPL}" ]; then
		chmod ${libmsprofiler_right} "${install_path}/${arch_name}/lib64/${LIB_PROF_IMPL}"
	fi
}

function register_uninstall() {
  local target_line='uninstall_package "share/info/msprof"'
  if [ ! -f "${install_path}/${SHARE_INFO_DIR}/${MSPROF}/${UNINSTALL_SCRIPT}" ]; then
      print "ERROR" "No such file: ${install_path}/${SHARE_INFO_DIR}/${MSPROF}/${UNINSTALL_SCRIPT}"
  fi
  if [ ! -x "${install_path}/${SHARE_INFO_DIR}/${MSPROF}/${UNINSTALL_SCRIPT}" ]; then
      print "ERROR" "The file ${install_path}/${SHARE_INFO_DIR}/${MSPROF}/${UNINSTALL_SCRIPT} is not executable."
      return 1
  fi
  if [ ! -f "${install_path}/${CANN_UNINSTALL_SCRIPT}" ]; then
      print "ERROR" "Failed to register uninstall script, no such file: ${install_path}/${CANN_UNINSTALL_SCRIPT}"
      return 1
  fi
  if grep -qxF "${target_line}" "${install_path}/${CANN_UNINSTALL_SCRIPT}"; then
        return 0
  fi
  local script_right=$(stat -c '%a' "${install_path}/${CANN_UNINSTALL_SCRIPT}")
  chmod u+w "${install_path}/${CANN_UNINSTALL_SCRIPT}"
  sed -i "/^exit /i uninstall_package \"share\/info\/msprof\"" "${install_path}/${CANN_UNINSTALL_SCRIPT}"
  chmod ${script_right} "${install_path}/${CANN_UNINSTALL_SCRIPT}"
}

source utils.sh

right=${user_right}
arch_name="${package_arch}-linux"
get_right
implement_install
if [ $? -eq 0 ]; then
 	register_uninstall
fi
chmod_ini_file
set_libmsprofiler_right
chmod_libmsprofiler
