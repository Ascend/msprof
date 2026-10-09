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
from common_func.info_conf_reader import InfoConfReader
from profiling_bean.basic_info.base_info import BaseInfo


class VersionInfo(BaseInfo):
    def __init__(self: any) -> None:
        super().__init__()
        self.collection_version = ""
        self.analysis_version = ""
        self.drv_version = 0

    def run(self, _):
        self.merge_data()

    def merge_data(self: any) -> any:
        self.collection_version = InfoConfReader().get_collection_version()
        self.analysis_version = InfoConfReader().ANALYSIS_VERSION
        self.drv_version = InfoConfReader().get_drv_version()
