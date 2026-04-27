#!/bin/bash
# the params for checking
install_args_num=0
install_path_num=0

uninstall_flag=0
check_flag=0
install_for_all_flag=0

function parse_script_args() {
    while true; do
        if [ "$3" = "" ]; then
            break
        fi
        case "$3" in
        --install-path=*)
            let "install_path_num+=1"
            install_path="${3#--install-path=}"
            install_path=$(readlink -f ${install_path})
            check_path ${install_path}
            check_cann_path ${install_path}
            shift
            continue
            ;;
        --quiet)
            shift
            continue
            ;;
        --install)
            let "install_args_num+=1"
            shift
            continue
            ;;
        --uninstall)
            uninstall_flag=1
            shift
            continue
            ;;
        --install-for-all)
            install_for_all_flag=1
            shift
            continue
            ;;
        --check)
            check_flag=1
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
    if [ ${check_flag} -eq 1 ]; then
        return 0
    fi
    if [ ${install_args_num} -ne 0 ] && [ ${uninstall_flag} -eq 1 ]; then
        print "ERROR" "Input option is invalid. Please try --help."
        exit 1
    fi
    if [ ${install_args_num} -eq 0 ] && [ ${uninstall_flag} -eq 0 ]; then
 	      print "ERROR" "Input option is invalid. Please try --help."
 	      exit 1
 	  fi
    if [ ${install_path_num} -gt 1 ]; then
        print "ERROR" "Do not input --install-path many times. Please try --help."
        exit 1
    fi
}

function execute_run() {
 	  if [ ${uninstall_flag} -eq 1 ]; then
 	      bash uninstall.sh ${install_path}
 	      if [ $? -ne 0 ]; then
 	          print "ERROR" "${MSPROF_RUN_NAME} package uninstall failed."
 	          exit 1
 	      fi
 	      print "INFO" "${MSPROF_RUN_NAME} package uninstall success."
 	  elif [ ${install_args_num} -gt 0 ]; then
 	      bash install.sh ${install_path} ${package_arch} ${install_for_all_flag}
 	      if [ $? -ne 0 ]; then
 	          print "ERROR" "${MSPROF_RUN_NAME} package install failed."
 	          exit 1
 	      fi
 	      print "INFO" "${MSPROF_RUN_NAME} package install success, the path is: '${install_path}'."
 	  fi
}

function get_default_install_path() {
    if [ "$UID" = "0" ]; then
        echo "/usr/local/Ascend/cann"
    else
        echo "${HOME}/Ascend/cann"
    fi
}

# use utils function and constant
source utils.sh
install_path=$(get_default_install_path)
#0, this footnote path;1, path for executing run;2, parents' dir for run package;3, run params
parse_script_args $*
check_args
execute_run