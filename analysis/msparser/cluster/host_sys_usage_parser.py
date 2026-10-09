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

import logging
import os
from collections import OrderedDict

from common_func.config_mgr import ConfigMgr
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from msmodel.cluster_info.cluster_info_model import ClusterInfoViewModel
from msmodel.hardware.sys_mem_model import SysMemModel
from msmodel.hardware.sys_usage_model import SysUsageModel


class HostSysUsageParser:
    """
    host sys usage data parser
    """

    NS_TO_S = 1000000000

    def __init__(self: any, params: dict) -> None:
        self.collection_path = params.get("collection_path")
        self.npu_id = params.get("npu_id")
        self.model_id = params.get("model_id")
        self.cluster_info_model = ClusterInfoViewModel(self.collection_path)
        self.cpu_usage_model = None
        self.mem_usage_model = None

    @staticmethod
    def _get_host_common_info(host_path: str) -> dict:
        try:
            sample_config = ConfigMgr.read_sample_config(host_path)
        except ProfException as err:
            logging.error("Get common info fail. %s", str(err))
            return {}
        cpu_sampling_interval = sample_config.get(StrConstant.HOST_CPU_SAMPLING_INTV, 0)
        mem_sampling_interval = sample_config.get(StrConstant.HOST_MEM_SAMPLING_INTV, 0)
        if cpu_sampling_interval == 0 or mem_sampling_interval == 0:
            logging.error("Get sampling interval fail.")
            return {}

        InfoConfReader().load_info(host_path)
        pid = InfoConfReader().get_json_pid_data()
        cpu_nums, _ = InfoConfReader().get_cpu_info()
        common_info = {
            'pid': pid,
            'cpu_nums': cpu_nums,
            'cpu_sampling_interval': cpu_sampling_interval,
            'mem_sampling_interval': mem_sampling_interval,
        }
        return common_info

    def process(self: any) -> None:
        # get host dir path from cluster_rank.db
        host_path = self._get_host_dir_path()
        if not host_path:
            logging.warning("Get prof host directory path fail.")
            return
        # get host basic info from info.json/sample.json
        common_info = self._get_host_common_info(host_path)
        if not common_info:
            logging.warning("The common info cannot be obtained.")
            return

        self.cpu_usage_model = SysUsageModel(
            host_path,
            DBNameConstant.DB_HOST_SYS_USAGE_CPU,
            [DBNameConstant.TABLE_SYS_USAGE, DBNameConstant.TABLE_PID_USAGE],
        )
        self.mem_usage_model = SysMemModel(
            host_path,
            DBNameConstant.DB_HOST_SYS_USAGE_MEM,
            [DBNameConstant.TABLE_SYS_MEM, DBNameConstant.TABLE_PID_MEM],
        )
        if not (self.cpu_usage_model.init() and self.mem_usage_model.init()):
            logging.error("SysUsageModel or SysMemModel init() failed.")
            return

        self._process_data(common_info)

    def _get_host_dir_path(self: any) -> str:
        if not os.path.exists(PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_RANK)):
            logging.warning("Can not find the %s or Permission denied!", DBNameConstant.DB_CLUSTER_RANK)
            return ""

        with self.cluster_info_model as model:
            if not model.check_table():
                return ""

            rank_or_device_ids = model.get_rank_or_device_ids()
            if not rank_or_device_ids:
                logging.warning("From %s get rank id or device id fail.", DBNameConstant.DB_CLUSTER_RANK)
                return ""
            if self.npu_id not in rank_or_device_ids:
                logging.error("Rank id or device id %d error, valid id is %s", self.npu_id, rank_or_device_ids)
                return ""

            device_dir = model.get_dir_name(self.npu_id)
            if not device_dir:
                logging.error("From %s get id %d dir name fail.", DBNameConstant.DB_CLUSTER_RANK, self.npu_id)
                return ""

            prof_dir = os.path.dirname(device_dir[0][0])
            host_dir_path = os.path.join(self.collection_path, prof_dir, "host")
            if not os.path.exists(host_dir_path):
                logging.error("%s not exist.", host_dir_path)
                return ""

            return host_dir_path

    def _host_cpu_common_proc(self, datas: list, tags: list, column_sum: list) -> list:
        if not datas or not datas[0]:
            logging.error("Host cpu datas error.")
            return []

        detail_data = []
        start_time = datas[0][-1]
        for data in datas:
            row_sum = sum(data[0:-1])
            if row_sum == 0:
                return []
            percent_data = [
                round(d / row_sum * NumberConstant.PERCENTAGE, NumberConstant.ROUND_TWO_DECIMAL) for d in data[0:-1]
            ]
            relative_time = round((data[-1] - start_time) / self.NS_TO_S, NumberConstant.ROUND_TWO_DECIMAL)
            percent_data.append(relative_time)
            detail_data.append(OrderedDict(zip(tags, percent_data)))
            for j, _ in enumerate(column_sum):
                column_sum[j] += data[j]

        return detail_data

    def _host_sys_cpu_proc(self: any, datas: list, tags: list, base_info: dict) -> dict:
        column_sum = [0, 0, 0, 0]
        detail_data = self._host_cpu_common_proc(datas, tags, column_sum)
        if not detail_data:
            logging.error("The sys cpu data have some all zeros.")
            return {}

        total = sum(column_sum)
        if total == 0:
            logging.error("Total cpu data is all zero.")
            return {}
        base_info["average_user_usage"] = round(
            column_sum[0] / total * NumberConstant.PERCENTAGE, NumberConstant.ROUND_TWO_DECIMAL
        )
        base_info["average_sys_usage"] = round(
            column_sum[1] / total * NumberConstant.PERCENTAGE, NumberConstant.ROUND_TWO_DECIMAL
        )
        base_info["average_io_usage"] = round(
            column_sum[2] / total * NumberConstant.PERCENTAGE, NumberConstant.ROUND_TWO_DECIMAL
        )
        base_info["average_idle_usage"] = round(
            column_sum[3] / total * NumberConstant.PERCENTAGE, NumberConstant.ROUND_TWO_DECIMAL
        )

        return {"info": base_info, "data": detail_data}

    def _host_pid_cpu_proc(self: any, datas: list, tags: list, base_info: dict) -> dict:
        column_sum = [0, 0]
        detail_data = self._host_cpu_common_proc(datas, tags, column_sum)
        if not detail_data:
            logging.error("The pid cpu data have some all zeros.")
            return {}

        total = sum(column_sum)
        if total == 0:
            logging.error("Total cpu data is all zero.")
            return {}
        base_info["average_user_usage"] = round(
            column_sum[0] / total * NumberConstant.PERCENTAGE, NumberConstant.ROUND_TWO_DECIMAL
        )
        base_info["average_sys_usage"] = round(
            column_sum[1] / total * NumberConstant.PERCENTAGE, NumberConstant.ROUND_TWO_DECIMAL
        )

        return {"info": base_info, "data": detail_data}

    def _host_sys_mem_proc(self, datas: list, tags: list, base_info: dict) -> dict:
        if not datas or not datas[0]:
            logging.error("Host sys mem datas error.")
            return {}
        detail_data = []
        mem_usage_list = []
        start_time = datas[0][-1]
        for data in datas:
            if data[0] == 0:
                logging.error("The mem data is all zero.")
                return {}
            mem_usage = round(
                (data[0] - data[1]) / data[0] * NumberConstant.PERCENTAGE, NumberConstant.ROUND_TWO_DECIMAL
            )
            relative_time = round((data[-1] - start_time) / self.NS_TO_S, NumberConstant.ROUND_TWO_DECIMAL)
            detail_data.append(OrderedDict(zip(tags, [mem_usage, relative_time])))
            mem_usage_list.append(mem_usage)

        base_info["total_mem"] = datas[0][0]
        base_info["average_mem_usage"] = round(
            sum(mem_usage_list) / len(mem_usage_list), NumberConstant.ROUND_TWO_DECIMAL
        )
        return {"info": base_info, "data": detail_data}

    def _host_pid_mem_proc(self, datas: list, tags: list, base_info: dict) -> dict:
        if not datas or not datas[0]:
            logging.error("Host pid mem datas error.")
            return {}
        detail_data = []
        start_time = datas[0][-1]
        for data in datas:
            _data = [int(d / 256) for d in data[:-1]]  # 1page = 4KB = 1/256MB
            relative_time = round((data[-1] - start_time) / self.NS_TO_S, NumberConstant.ROUND_TWO_DECIMAL)
            _data.append(relative_time)
            detail_data.append(OrderedDict(zip(tags, _data)))
        return {"info": base_info, "data": detail_data}

    def _get_all_pid(self: any) -> tuple:
        cpu_pids = self.cpu_usage_model.get_all_pid()
        cpu_pids = list(set({d[0] for d in cpu_pids}))
        mem_pids = self.mem_usage_model.get_all_pid()
        mem_pids = list({d[0] for d in mem_pids})
        return cpu_pids, mem_pids

    def _construct_data_proc_params(self, common_info):
        pid = common_info.get('pid')
        cpu_nums = common_info.get('cpu_nums')
        cpu_sampling_interval = common_info.get('cpu_sampling_interval')
        mem_sampling_interval = common_info.get('mem_sampling_interval')
        cpu_pids, mem_pids = self._get_all_pid()

        data_proc_params = [
            # host sys cpu proc params
            {
                "original_data": self.cpu_usage_model.get_sys_cpu_data(),
                "save_file": "host_sys_cpu_usage_{}_{}.json".format(self.npu_id, self.model_id),
                "data_tags": ["user_usage", "sys_usage", "io_usage", "idle_usage", "timestamp"],
                "base_info": {"cpu_nums": cpu_nums, "sampling_interval": cpu_sampling_interval},
                "proc_data_func": self._host_sys_cpu_proc,
            },
            # host pid cpu proc params
            {
                "original_data": self.cpu_usage_model.get_pid_cpu_data(pid),
                "save_file": "host_pid_cpu_usage_{}_{}.json".format(self.npu_id, self.model_id),
                "data_tags": ["user_usage", "sys_usage", "timestamp"],
                "base_info": {"sampling_interval": cpu_sampling_interval, "cur_pid": pid, "all_pids": cpu_pids},
                "proc_data_func": self._host_pid_cpu_proc,
            },
            # host sys mem proc params
            {
                "original_data": self.mem_usage_model.get_sys_mem_data(),
                "save_file": "host_sys_mem_usage_{}_{}.json".format(self.npu_id, self.model_id),
                "data_tags": ["mem_usage", "timestamp"],
                "base_info": {"sampling_interval": mem_sampling_interval},
                "proc_data_func": self._host_sys_mem_proc,
            },
            # host pid mem proc params
            {
                "original_data": self.mem_usage_model.get_pid_mem_data(pid),
                "save_file": "host_pid_mem_usage_{}_{}.json".format(self.npu_id, self.model_id),
                "data_tags": ["size", "resident", "shared", "timestamp"],
                "base_info": {"sampling_interval": mem_sampling_interval, "cur_pid": pid, "all_pids": mem_pids},
                "proc_data_func": self._host_pid_mem_proc,
            },
        ]
        return data_proc_params

    def _process_data(self: any, common_info: tuple) -> None:
        # construct params for various data processing
        data_proc_params = self._construct_data_proc_params(common_info)

        # start handle data and save data
        for params in data_proc_params:
            if not params.get("original_data"):
                logging.warning("Get data from database fail.")
                continue
            handled_data = params.get("proc_data_func")(
                params.get("original_data"), params.get("data_tags"), params.get("base_info")
            )
            if not handled_data:
                logging.warning("Process data fail, not save %s.", params.get("save_file"))
                continue
            FileManager.storage_query_result_json_file(self.collection_path, handled_data, params.get("save_file"))
