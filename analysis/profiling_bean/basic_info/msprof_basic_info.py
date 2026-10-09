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

import json

from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_exception import ProfException
from framework.load_info_manager import LoadInfoManager
from profiling_bean.basic_info.collect_info import CollectInfo
from profiling_bean.basic_info.device_info import DeviceInfo
from profiling_bean.basic_info.host_info import HostInfo
from profiling_bean.basic_info.model_info import ModelInfo
from profiling_bean.basic_info.version_info import VersionInfo


class BasicInfo:
    """
    basic info contain collect, device and host info
    """

    def __init__(self: any) -> None:
        self.collection_info = CollectInfo()
        self.device_info = DeviceInfo()
        self.host_info = HostInfo()
        self.model_info = ModelInfo()
        self.version_info = VersionInfo()

    def fresh_data(self: any, project_path: str) -> None:
        """
        run to fresh basic data
        :param project_path: project path
        :return: None
        """
        self.collection_info.run(project_path)
        self.device_info.run(project_path)
        self.host_info.run(project_path)
        self.model_info.run(project_path)
        self.version_info.run(project_path)

    def run(self: any, project_path: str) -> None:
        """
        run for collect, device and host info
        :param project_path: project path
        :return: None
        """
        self.fresh_data(project_path)


class MsProfBasicInfo:
    """
    msprof basic info class
    """

    # transform the json to be show for ide.
    TRANSFORM_MAP = {
        "collection_start_time": "Collection start time",
        "collection_end_time": "Collection end time",
        "result_size": "Result Size",
        "device_id": "Device Id",
        "ai_cpu_num": "AI CPU Number",
        "ai_core_num": "AI Core Number",
        "control_cpu_num": "Control CPU Number",
        "control_cpu_type": "Control CPU Type",
        "ts_cpu_num": "TS CPU Number",
        "host_computer_name": "Host Computer Name",
        "host_operating_system": "Host Operating System",
        "_cpu_id": "CPU ID",
        "_frequency": "Frequency",
        "_logical_cpu_count": "Logical_CPU_Count",
        "_cpu_name": "Name",
        "_cpu_type": "Type",
        "_device_id": "Device Id",
        "_model_id": "Model Id",
        "_iteration_num": "Iteration Number",
    }

    def __init__(self: any, project_path: str) -> None:
        self.project_path = project_path
        self.basic_info = None

    def init(self: any) -> None:
        """
        init the component for the msprof basic info
        :return: None
        """
        LoadInfoManager().load_info(self.project_path)
        self.basic_info = BasicInfo()

    def run(self: any) -> str:
        """
        run to merge data and format the json data
        :return: the result to get basic info
        """
        try:
            json_result = self._run_and_deal_basic_info()
            return json.dumps({'status': NumberConstant.SUCCESS, 'info': '', 'data': json_result})
        except (OSError, SystemError, ValueError, TypeError, RuntimeError):
            return json.dumps(
                {
                    'status': NumberConstant.ERROR,
                    'info': "Get the basic info failed, "
                    "maybe no config files generated, "
                    "please check the data directory: "
                    "{}".format(self.project_path),
                    'data': "",
                }
            )

    def _run_and_deal_basic_info(self: any) -> dict:
        self.basic_info.run(self.project_path)
        json_data = json.dumps(self.basic_info, default=lambda json_info: json_info.__dict__, sort_keys=True)
        json_result = json.loads(json_data)
        self.__deal_with_json_keys(json_result)
        return json_result

    def __deal_with_json_keys(self: any, json_data: any, depth: int = 0) -> None:
        depth_limit = 5  # 最大递归深度
        if depth > depth_limit:
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)
        depth += 1
        if isinstance(json_data, dict):
            for _key in list(json_data.keys()):
                if _key in self.TRANSFORM_MAP:
                    json_data.setdefault(self.TRANSFORM_MAP.get(_key), json_data.get(_key))
                    json_data.pop(_key)
                    self.__deal_with_json_keys(json_data.get(self.TRANSFORM_MAP.get(_key)), depth)
                self.__deal_with_json_keys(json_data.get(_key), depth)
        elif isinstance(json_data, list):
            for per_json_data in json_data:
                self.__deal_with_json_keys(per_json_data, depth)
