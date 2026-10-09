#!/bin/bash
#----------------------------------------------------------------------------
# Purpose:
# Copyright Huawei Technologies Co., Ltd. 2010-2018. All rights reserved.
#----------------------------------------------------------------------------
set -e

CUR_DIR=$(dirname $(readlink -f "$0"))
TOP_DIR=$(readlink -f "$CUR_DIR/../../..")

log()
{
    info=$1

	timestr=`date '+%Y-%m-%d %H:%M:%S.%N '|cut -b 1-23`
        echo "[${timestr}]""${info}"

    return 0
}

log_error()
{
	info="$1"
	log "[ERROR] ${info}"
	return 0
}


log_warning()
{
        info="$1"
        log "[WARNING] ${info}"
        return 0
}

log_info()
{
        info="$1"
        log "[INFO] ${info}"
        return 0
}

LLT_REPORT_OUT="$1"
LLT_NAME="$2"
LLT_MPI_TASK_NUM="$3"
LLT_KILL_TIME="$4"
LLT_RUN_MOD="$5"
RUN_ENV_FILE="$6"
LLT_TARGET_BASENAME=$(basename $LLT_NAME)

log_info "begin run cpp llt target:${LLT_TARGET_BASENAME}"

if [ "${RUN_ENV_FILE}" != "" ]; then
    if [ -f "${RUN_ENV_FILE}" ]; then
        log_info "source ${RUN_ENV_FILE}"
        # ${RUN_ENV_PARAMS}不可以加引号，因为它可能有多个参数
        source "${RUN_ENV_FILE}"
    else
        log_info "WARNING: RUN_ENV_FILE ${RUN_ENV_FILE} does not exist, skip source."
    fi
fi

if [ "$LLT_RUN_MOD"x = "x" ]; then
    LLT_RUN_MOD="single"
fi

if [ ! -n "$LLT_KILL_TIME" ]; then
    LLT_KILL_TIME=1200
fi

#RUN_OPTION=$3
log_info "LLT_REPORT_OUT is ${LLT_REPORT_OUT}"
log_info "LLT_NAME is ${LLT_NAME}"
log_info "LLT_MPI_TASK_NUM is ${LLT_MPI_TASK_NUM}"
log_info "LLT_KILL_TIME is ${LLT_KILL_TIME}"
log_info "LLT_TARGET_BASENAME is ${LLT_TARGET_BASENAME}"
log_info "LLT_RUN_MOD is ${LLT_RUN_MOD}"


if [ "${use_hi_sudo}" = "true" ]; then
    hisudo="${HI_SUDO}"
fi

FAIL=1
SUCC=0

if [ "${close_mem_leak_detect}" = "true" ]; then
    run_time_flags=detect_leaks=0
else
    run_time_flags=detect_leaks=1
fi

if [ "${LLT_MPI_TASK_NUM}" -gt 1 ] 2>/dev/null ; then
    log_info "run ${LLT_MPI_TASK_NUM} llt tasks with mpi"
    ASAN_OPTIONS=$run_time_flags timeout -s SIGKILL ${LLT_KILL_TIME}s ${hisudo} ./llt/third_party/mpi/bin/mpirun -n ${LLT_MPI_TASK_NUM} ${LLT_NAME} \
    --gtest_output=xml:${LLT_REPORT_OUT}/${LLT_TARGET_BASENAME}.xml 2>&1 | tee ${LLT_REPORT_OUT}/run_${LLT_TARGET_BASENAME}.log
elif [ "${LLT_RUN_MOD}" = "single" ]; then
    log_info "run single llt task"
    ASAN_OPTIONS=$run_time_flags timeout -s SIGKILL ${LLT_KILL_TIME}s ${hisudo} ${LLT_NAME} \
        --gtest_output=xml:${LLT_REPORT_OUT}/${LLT_TARGET_BASENAME}.xml 2>&1 | tee ${LLT_REPORT_OUT}/run_${LLT_TARGET_BASENAME}.log
else
    log_info "run llt with LLT_RUN_MOD=${LLT_RUN_MOD}"
    # The executor sits next to this script. This path used to read
    # ${TOP_DIR}/cmake/function/tools/llt_executor.py, which resolves to nothing:
    # TOP_DIR is the repo root, whose cmake/ contains only superbuild/, and there
    # is no function/ directory anywhere in the tree. The file is at
    # test/cmake/tools/llt_executor.py. This branch is only taken when
    # LLT_RUN_MOD is set to something other than single with a task count of 1,
    # and depend.cmake defaults LLT_RUN_MOD to single, which is why it went
    # unnoticed.
    ASAN_OPTIONS=$run_time_flags timeout -s SIGKILL ${LLT_KILL_TIME}s ${hisudo} python3 ${TOP_DIR}/test/cmake/tools/llt_executor.py ${LLT_NAME} \
        --run-mod=testsuite \
        --tmp=${LLT_REPORT_OUT} \
        --output=${LLT_REPORT_OUT}/${LLT_TARGET_BASENAME}.xml  \
        --thread=8 \
        --print-llt-log 2>&1 | tee ${LLT_REPORT_OUT}/run_${LLT_TARGET_BASENAME}.log
fi

run_ret=${PIPESTATUS[0]}
log_info "exit code: ${run_ret}"
if [ "${run_ret}" -eq 137 ]
then
    log_error "timeout: execute ${LLT_NAME} more than ${LLT_KILL_TIME}s killed"
    exit "${FAIL}"
fi
#check exit code of run result
if [ "${run_ret}" -ne 0 ]
then
    log_error "return run_res value not 0"
    exit "${FAIL}"
fi

#check report.xml.if not exist,return fail.
if [ ! -f "${LLT_REPORT_OUT}/${LLT_TARGET_BASENAME}.xml" ]
then
    log_error "can not find ${LLT_REPORT_OUT}/${LLT_TARGET_BASENAME}.xml"
    exit "${FAIL}"
fi

#check testcase result.
if ! grep 'testsuites .* failures="0"' "${LLT_REPORT_OUT}/${LLT_TARGET_BASENAME}.xml" >/dev/null
then
    log_error "failures case is not 0 in ${LLT_REPORT_OUT}/${LLT_TARGET_BASENAME}.xml"
    exit "${FAIL}"
fi

#check testcase result.
if ! grep 'testsuites .* errors="0"' "${LLT_REPORT_OUT}/${LLT_TARGET_BASENAME}.xml" >/dev/null
then
    log_error "errors case is not 0 in ${LLT_REPORT_OUT}/${LLT_TARGET_BASENAME}.xml"
    exit "${FAIL}"
fi

#ubsan check. check code problem that is runtime error.
if grep -nw 'runtime error:' "${LLT_REPORT_OUT}/run_${LLT_TARGET_BASENAME}.log" >/dev/null
then
    log_error "testcase runtime error in ${LLT_REPORT_OUT}/run_${LLT_TARGET_BASENAME}.log"
    exit "${FAIL}"
fi

#check LSAN and ASAN  problem.
if grep -n '==ERROR: .*Sanitizer' "${LLT_REPORT_OUT}/run_${LLT_TARGET_BASENAME}.log" >/dev/null
then
    log_error "Address Sanitizer error in ${LLT_REPORT_OUT}/run_${LLT_TARGET_BASENAME}.log"
    exit "${FAIL}"
fi

log_info "llt run task succ."
log_info "llt run time consuming (module=${LLT_TARGET_BASENAME}):$SECONDS seconds"
exit "${SUCC}"
