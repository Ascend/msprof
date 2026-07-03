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

if [ ! -z "$test_list" ]; then
    echo -e "${WHITE}========================================${NC}"
    echo -e "${GREEN}[DEBUG][$LEVEL] There are $num_of_cases test cases:${NC}"
    echo -e "${WHITE}----------------------------------------${NC}"
    echo -e "$test_list" | sed 's/^/    /'
    echo -e "${WHITE}========================================${NC}"

    for i in $test_list
    do
        echo -e "${WHITE}====================${i%.sh} Test Case ====================${NC}"
        start_time=$(date "+%s")
        bash "$i" "$output_path"
        check_exit_code=$?
        end_time=$(date "+%s")
        duration_time=$(( ${end_time} - ${start_time} ))
        if [ 0 -ne ${check_exit_code} ]; then
            echo "$i fail" >> "$result_file"
            echo -e "--------------------------------------${WHITE}${i%.sh}${NC}------${RED}FAIL${NC}"
        else
            echo "$i pass ${duration_time}" >> "$result_file"
            echo -e "--------------------------------------${WHITE}${i%.sh}${NC}------${GREEN}PASS${NC}"
        fi
    done
    echo -e "${GREEN}[DEBUG] End msprof_smoke_test${NC}"
else
    echo -e "${RED}[DEBUG] No test cases: $test_list${NC}"
fi
