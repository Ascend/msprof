#!/bin/bash
# This script is used to build msprofbin&&libmsprofiler.so
# Copyright Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
VERSION="none"
WHL_VERSION="0.0.1"
BUILD_TYPE="Release"
BUILD_MODE="analysis"

# input param check
while [[ $# -gt 0 ]]; do
    case $1 in
        --build_type=*)
            BUILD_TYPE="${1#*=}"
            if [[ "$BUILD_TYPE" != "Release" && "$BUILD_TYPE" != "Debug" ]]; then
                echo "[ERROR] Invalid build type. Valid options are: Release, Debug"
                exit 1
            fi
            shift
            ;;
        --mode=*)
            MODE="${1#*=}"
            if [[ "$MODE" != "all" && "$MODE" != "collector" && "$MODE" != "analysis" ]]; then
                echo "[ERROR] Invalid mode. Valid options are: all, collector, analysis"
                exit 1
            fi
            BUILD_MODE="$MODE"
            shift
            ;;
        --version=*)
            VERSION="${1#*=}"
            shift
            ;;
        --whl_version=*)
            WHL_VERSION="${1#*=}"
            shift
            ;;
        *)
            echo "[ERROR] Unknown parameter: $1"
            exit 1
            ;;
    esac
done

# input param check
if [[ $# -gt 3 ]]; then
    echo "[ERROR] Please input valid parameters, for example:"
    echo "       ./build.sh                 # Default"
    echo "       ./build.sh --build_type=Debug --mode=all --version=none"
    exit 1
fi

function build_runtime() {
    cd ${TOP_DIR}/build/collector
    git clone https://gitcode.com/cann/runtime.git
    cd runtime
    echo "build runtime start."
    bash install_deps.sh
    python download_3rd_party.py
    bash build.sh --cann_3rd_lib_path=third_party
    cd build_out/
    chmod +x cann-npu-runtime_*.run
    ./cann-npu-runtime_*.run --noexec --extract=./runtime_decompress
    echo "build runtime end."
    mkdir -p ${TOP_DIR}/build/collector/runtime_install
    chmod 755 -R ${TOP_DIR}/build/collector/runtime_install
    yes | ./cann-npu-runtime_*.run --full --install-path=${TOP_DIR}/build/collector/runtime_install
}

function build_oam_tools() {
    cd ${TOP_DIR}/build/collector
    git clone https://gitcode.com/cann/oam-tools.git
    cd oam-tools
    export ASCEND_HOME_PATH=$TOP_DIR/build/collector/runtime_install/cann/
    echo "build oam-tools start."
    bash build.sh
    cd build_out/
    chmod +x cann-oam-tools_*.run
    ./cann-oam-tools_*.run --noexec --extract=./oam_tools_decompress
    echo "build oam-tools end."
}

function build_analysis() {
    rm -rf ${TOP_DIR}/build/analysis
    cmake -S ${TOP_DIR}/cmake/superbuild/ -B ${TOP_DIR}/build/analysis -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${TOP_DIR}/prefix -DSECUREC_LIB_DIR=${TOP_DIR}/prefix/securec_shared
    cd ${TOP_DIR}/build/analysis; make -j$(nproc)
}

function ensure_metadef_version_info() {
    local version_file="${TOP_DIR}/build/collector/runtime_install/cann/share/info/metadef/version.info"
    local version_dir=$(dirname "${version_file}")
    mkdir -p "${version_dir}"
    if [[ ! -f "${version_file}" ]]; then
        echo "Version=1.0.0" > "${version_file}"
        echo "[build] created ${version_file} for oam-tools dependency check"
    fi
}

function build_collector() {
    rm -rf ${TOP_DIR}/build/collector
    mkdir -p ${TOP_DIR}/build/collector
    build_runtime
    ensure_metadef_version_info
    build_oam_tools
}

# 1. build
case "$BUILD_MODE" in
    "all")
        build_collector
        build_analysis
        ;;
    "collector")
        build_collector
        ;;
    "analysis")
        build_analysis
        ;;
    *)
        echo "Invalid BUILD_MODE: $BUILD_MODE"
        exit 1
        ;;
esac

# 2. package
bash ${TOP_DIR}/scripts/create_run_package.sh ${VERSION} ${WHL_VERSION} ${BUILD_MODE}
