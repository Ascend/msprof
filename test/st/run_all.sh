#!/bin/bash
set -e

# Define Color Codes
RED='\e[0;31m'
GREEN='\e[0;32m'
WHITE='\e[0;37m'
NC='\e[0m'

RUN_ALL_PATH="$(readlink -f "$0")"
PARENT_RUN_ALL_PATH="$(dirname "$RUN_ALL_PATH")"
if [ -z "$PYTHONPATH" ]; then
    export PYTHONPATH="${PARENT_RUN_ALL_PATH}"
else
    export PYTHONPATH="${PARENT_RUN_ALL_PATH}:$PYTHONPATH"
fi
result_file="${PARENT_RUN_ALL_PATH}/run_all_result.txt"
output_path="${PARENT_RUN_ALL_PATH}/result"
rm -rf -- "$result_file"
rm -rf -- "$output_path"
touch "$result_file"
mkdir -p "$output_path"

LEVEL="l0"

for arg in "$@"; do
    case $arg in
        --level=l0|--level=l1|--level=l0,l1)
            LEVEL="${arg#*=}"
            ;;
        --devices=*)
            if [ "${arg#*=}" != "0,1" ]; then
                echo "Error: this test suite is fixed to devices 0,1"
                exit 1
            fi
            ;;
        --level=*)
            echo "Error: --level can only be l0, l1 or l0,l1"
            exit 1
            ;;
    esac
done

test_list=""

if [ "$LEVEL" == "l0" ]; then
    test_list=$(ls | grep ".sh" | grep -E "l0_test_ascend_msprof")
elif [ "$LEVEL" == "l1" ]; then
    test_list=$(ls | grep ".sh" | grep -E "l1_test_ascend_msprof")
elif [ "$LEVEL" == "l0,l1" ]; then
    test_list=$(ls | grep ".sh" | grep -E "l0_test_ascend_msprof|l1_test_ascend_msprof")
fi

num_of_cases=$(echo "$test_list" | wc -l)
failed_cases=0

if [ ! -z "$test_list" ]; then
    echo -e "${WHITE}========================================${NC}"
    echo -e "${GREEN}[DEBUG][$LEVEL] There are $num_of_cases test cases:${NC}"
    echo -e "${WHITE}----------------------------------------${NC}"
    echo -e "$test_list" | sed 's/^/    /'
    echo -e "${WHITE}========================================${NC}"

    device0=0
    device1=1
    runtime_device0=0
    runtime_device1=1
    parallel_enabled=1

    run_cases() {
        local visibility="$1"
        shift
        local failed=0
        local test_case start_time end_time duration_time check_exit_code

        for test_case in "$@"; do
            echo -e "${WHITE}====================${test_case%.sh} Test Case ====================${NC}"
            start_time=$(date "+%s")
            if [ "$visibility" = "all-visible" ]; then
                if env -u ASCEND_RT_VISIBLE_DEVICES bash "$test_case" "$output_path"; then
                    check_exit_code=0
                else
                    check_exit_code=$?
                fi
            elif [ "$visibility" = "inherited" ]; then
                if bash "$test_case" "$output_path"; then
                    check_exit_code=0
                else
                    check_exit_code=$?
                fi
            elif ASCEND_RT_VISIBLE_DEVICES="$visibility" bash "$test_case" "$output_path"; then
                check_exit_code=0
            else
                check_exit_code=$?
            fi
            end_time=$(date "+%s")
            duration_time=$((end_time - start_time))
            if [ "$check_exit_code" -ne 0 ]; then
                echo "$test_case fail ${duration_time}" >> "$result_file"
                echo -e "--------------------------------------${WHITE}${test_case%.sh}${NC}------${RED}FAIL${NC}"
                failed=1
            else
                echo "$test_case pass ${duration_time}" >> "$result_file"
                echo -e "--------------------------------------${WHITE}${test_case%.sh}${NC}------${GREEN}PASS${NC}"
            fi
        done
        return "$failed"
    }

    if [ "$parallel_enabled" -eq 0 ]; then
        for i in $test_list; do
            if [ "$i" = "l0_test_ascend_msprof_all_sys_switch.sh" ]; then
                if ! run_cases "all-visible" "$i"; then
                    failed_cases=$((failed_cases + 1))
                fi
            elif ! run_cases "inherited" "$i"; then
                failed_cases=$((failed_cases + 1))
            fi
        done
    else
        communication_case=""
        pending_cases=()
        for i in $test_list; do
            if [[ "$i" == *communication.sh ]]; then
                communication_case="$i"
            else
                pending_cases+=("$i")
            fi
        done

        if [ -n "$communication_case" ]; then
            if ! run_cases "${runtime_device0},${runtime_device1}" "$communication_case"; then
                failed_cases=$((failed_cases + 1))
            fi
        fi

        declare -A running_pids=()
        remaining=${#pending_cases[@]}
        next_case=0

        while [ "$next_case" -lt "$remaining" ] || [ "${#running_pids[@]}" -gt 0 ]; do
            for requested_device in "${device0}" "${device1}"; do
                if [ "$next_case" -ge "$remaining" ] || [ -n "${running_pids[$requested_device]:-}" ]; then
                    continue
                fi
                test_case="${pending_cases[$next_case]}"
                next_case=$((next_case + 1))
                if [ "$requested_device" = "$device0" ]; then
                    visible_device="$runtime_device0"
                else
                    visible_device="$runtime_device1"
                fi
                run_cases "$visible_device" "$test_case" &
                running_pids[$requested_device]=$!
            done

            for requested_device in "${device0}" "${device1}"; do
                pid="${running_pids[$requested_device]:-}"
                if [ -n "$pid" ] && ! kill -0 "$pid" 2>/dev/null; then
                    if ! wait "$pid"; then
                        failed_cases=$((failed_cases + 1))
                    fi
                    unset 'running_pids[$requested_device]'
                fi
            done
            if [ "$next_case" -lt "$remaining" ] || [ "${#running_pids[@]}" -gt 0 ]; then
                sleep 1
            fi
        done
    fi
    echo -e "${GREEN}[DEBUG] End msprof_smoke_test${NC}"
    if [ ${failed_cases} -ne 0 ]; then
        exit 1
    fi
else
    echo -e "${RED}[DEBUG] No test cases: $test_list${NC}"
fi
