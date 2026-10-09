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
from typing import List

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from common_func.msprof_object import CustomizedNamedtupleFactory
from mscalculate.ascend_task.ascend_task import HostTask
from msmodel.runtime.runtime_host_task_model import RuntimeHostTaskModel


class HostTaskCollector:
    CONTEXT_ID_INDEX = 4
    HOST_TASK_TUPLE_TYPE = CustomizedNamedtupleFactory.generate_named_tuple_from_dto(HostTask, [])

    def __init__(self: any, result_dir: str):
        self.result_dir = result_dir

    @classmethod
    def _generate_host_task_objs(cls: any, raw_data: list):
        objs = []
        for data in raw_data:
            if data[cls.CONTEXT_ID_INDEX] == str(NumberConstant.DEFAULT_GE_CONTEXT_ID):
                objs.append(
                    cls.HOST_TASK_TUPLE_TYPE(
                        *data[: cls.CONTEXT_ID_INDEX],
                        NumberConstant.DEFAULT_GE_CONTEXT_ID,
                        *data[cls.CONTEXT_ID_INDEX + 1 :],
                    )
                )
            else:
                context_ids = data[cls.CONTEXT_ID_INDEX].split(",")
                for _id in context_ids:
                    objs.append(
                        cls.HOST_TASK_TUPLE_TYPE(
                            *data[: cls.CONTEXT_ID_INDEX], int(_id), *data[cls.CONTEXT_ID_INDEX + 1 :]
                        )
                    )
        return objs

    def get_host_tasks_by_model_and_iter(self: any, model_id: int, iter_id: int, device_id: int) -> List[HostTask]:
        """
        This function will get host tasks with model within iter.
        """
        if not self._check_host_tasks_exists():
            return []

        dev_visible_host_tasks = self._get_host_tasks(
            is_all=False, model_id=model_id, iter_id=iter_id, device_id=device_id
        )

        if not dev_visible_host_tasks:
            logging.error("Get dev visible hosts for model_id: %d, iter_id: %d error.", model_id, iter_id)

        return dev_visible_host_tasks

    def get_host_tasks(self, device_id: int) -> List[HostTask]:
        """
        This function will get host tasks.
        """
        if not self._check_host_tasks_exists():
            return []

        dev_visible_host_tasks = self._get_host_tasks(
            is_all=True,
            model_id=NumberConstant.INVALID_MODEL_ID,
            iter_id=NumberConstant.INVALID_ITER_ID,
            device_id=device_id,
        )

        if not dev_visible_host_tasks:
            logging.error("Get dev visible hosts error.")

        return dev_visible_host_tasks

    def get_host_task_stream_table(self: any, device_id: int) -> dict:
        host_task_stream_table = {}
        host_tasks = self.get_host_tasks(device_id)
        for host_task in host_tasks:
            if host_task.task_id in host_task_stream_table:
                logging.error("Duplicate task_id found: %s", host_task.task_id)
                continue
            host_task_stream_table[host_task.task_id] = host_task.stream_id
        return host_task_stream_table

    def _get_host_tasks(self: any, is_all: bool, model_id: int, iter_id: int, device_id: int) -> List[HostTask]:
        """
        get host tasks
        filter_: host tasks unique id range
        :return:
            is_all Ture: all host tasks
            is_all False: host tasks with model_id within iter
        """
        if not self._check_host_tasks_exists():
            return []
        with RuntimeHostTaskModel(self.result_dir) as model:
            host_tasks = model.get_host_tasks(is_all, model_id, iter_id, device_id)
            return self._generate_host_task_objs(host_tasks)

    def _check_host_tasks_exists(self):
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_RUNTIME)
        if not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_HOST_TASK):
            logging.warning("No table %s.%s found", DBNameConstant.DB_RUNTIME, DBNameConstant.TABLE_HOST_TASK)
            return False
        return True
