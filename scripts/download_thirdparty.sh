#!/bin/bash
# This script is used to download thirdpart needed by msprof.
# Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..

OPENSOURCE_DIR=${TOP_DIR}/opensource
PLATFORM_DIR=${TOP_DIR}/platform
LLT_DIR=${TOP_DIR}/test/opensource

THIRDPARTY_LIST="${OPENSOURCE_DIR}/protobuf    \\
                 ${OPENSOURCE_DIR}/json        \\
                 ${OPENSOURCE_DIR}/makeself    \\
                 ${OPENSOURCE_DIR}/rapidjson    \\
                 ${PLATFORM_DIR}/securec       \\
                 ${LLT_DIR}/googletest         \\
                 ${LLT_DIR}/mockcpp         \\
                 ${LLT_DIR}/boost"

isUT=0
for arg in "$@"; do
    if [ "${arg}" == "force" ]; then
        echo "force delete origin opensource files"
        echo ${THIRDPARTY_LIST}
        rm -rf ${THIRDPARTY_LIST}
    elif [ "${arg}" == "ut" ]; then
        isUT=1
    fi
done

function download_boost() {
    local boost_url="https://inst.obs.cn-north-4.myhuaweicloud.com/env/mirror/boost_1_81_0.tar.bz2"
    local boost_archive="boost_1_81_0.tar.bz2"
    local boost_sha256="71feeed900fbccca04a3b4f2f84a7c217186f28a940ed8b7ed4725986baf99fa"

    cd ${LLT_DIR}
    [ -d "boost" ] && return 0

    curl -Lfk --retry 5 --retry-delay 2 -o "${boost_archive}" "${boost_url}"
    echo "${boost_sha256}  ${boost_archive}" | sha256sum -c -
    tar -xjf "${boost_archive}"
    mv boost_1_81_0 boost
    rm -f "${boost_archive}"
}

function patch_makeself() {
    cd ${OPENSOURCE_DIR}
    git clone -b v2.5.0.x https://gitcode.com/cann-src-third-party/makeself.git
    cd ${OPENSOURCE_DIR}/makeself
    tar -zxf makeself-release-2.5.0.tar.gz
    cd makeself-release-2.5.0
    ulimit -n 8192
    patch -p1 < ../makeself-2.5.0.patch
    cd ${OPENSOURCE_DIR}/makeself
    cp -r makeself-release-2.5.0 ${OPENSOURCE_DIR}
    cd ${OPENSOURCE_DIR}
    rm -rf makeself
    mv makeself-release-2.5.0 makeself
}

mkdir -p ${OPENSOURCE_DIR} && cd ${OPENSOURCE_DIR}
[ ! -d "json" ] && git clone https://gitcode.com/GitHub_Trending/js/json.git json -b v3.11.3
[ ! -d "rapidjson" ] && git clone https://gitcode.com/GitHub_Trending/ra/rapidjson.git rapidjson
[ ! -d "makeself" ] && patch_makeself

if [ "${isUT}" == "1" ]; then
    mkdir -p ${LLT_DIR} && cd ${LLT_DIR}
    if [ ! -d "googletest" ]; then
        git clone https://gitcode.com/GitHub_Trending/go/googletest.git googletest -b release-1.10.0
    fi
    # 对已存在的 googletest 目录同样移除 -Werror，保证 CI 缓存和存量环境兼容。
    # 使用 \b 边界匹配移除所有 -Werror 出现（同时处理不同格式），并校验结果。
    if [ -f "googletest/googletest/cmake/internal_utils.cmake" ]; then
        sed -i 's/-Werror\b//g' googletest/googletest/cmake/internal_utils.cmake
        if grep -q -- '-Werror\b' googletest/googletest/cmake/internal_utils.cmake 2>/dev/null; then
            echo "[WARN] -Werror removal from googletest may be incomplete"
        fi
    fi
    [ ! -d "mockcpp" ] && git clone https://gitcode.com/Ascend/mockcpp.git mockcpp -b ascend_mindstudio_mockcpp_master
    download_boost
fi
mkdir -p ${PLATFORM_DIR} && cd ${PLATFORM_DIR}
[ ! -d "securec" ] && git clone https://gitcode.com/openeuler/libboundscheck.git securec || true
