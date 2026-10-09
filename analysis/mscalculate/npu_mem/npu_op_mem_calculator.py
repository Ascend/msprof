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
from collections import namedtuple

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.hash_dict_constant import HashDictData
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from mscalculate.interface.icalculator import ICalculator
from msmodel.npu_mem.npu_ai_stack_mem_model import NpuAiStackMemModel


class NpuOpMemCalculator(ICalculator, MsMultiProcess):
    """
    calculate npu op memory data
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

        self._conn = None
        self._curs = None

        self._op_data = []
        self._model = NpuAiStackMemModel(
            self._project_path,
            DBNameConstant.DB_MEMORY_OP,
            [DBNameConstant.TABLE_NPU_OP_MEM_RAW, DBNameConstant.TABLE_NPU_OP_MEM, DBNameConstant.TABLE_NPU_OP_MEM_REC],
        )
        self._memory_record = []
        self._operator_memory = []

    def calculate(self: any) -> None:
        with self._model as _model:
            self._op_data = _model.get_table_data(DBNameConstant.TABLE_NPU_OP_MEM_RAW)
        self._calc_memory_record()
        self._calc_operator_memory()

    def save(self: any) -> None:
        """
        save data
        """
        if not self._memory_record or not self._operator_memory:
            return
        with self._model as _model:
            _model.flush(DBNameConstant.TABLE_NPU_OP_MEM_REC, self._memory_record)
            _model.flush(DBNameConstant.TABLE_NPU_OP_MEM, self._operator_memory)
            return

    def ms_run(self: any) -> None:
        """
        calculate for task scheduler
        :return:
        """
        if not self._judge_should_calculate():
            return
        self.calculate()
        self.save()

    def _judge_should_calculate(self):
        npu_op_mem_db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_MEMORY_OP)
        if not os.path.exists(npu_op_mem_db_path):
            return False
        if DBManager.check_tables_in_db(
            npu_op_mem_db_path, DBNameConstant.TABLE_NPU_OP_MEM, DBNameConstant.TABLE_NPU_OP_MEM_REC
        ):
            logging.info(
                "Found table %s and %s, no need to generate again",
                DBNameConstant.TABLE_NPU_OP_MEM,
                DBNameConstant.TABLE_NPU_OP_MEM_REC,
            )
            return False
        return True

    def _calc_operator_memory(self: any) -> None:
        allocated_data = {}
        OperatorKey = namedtuple('OperatorKey', ['operator', 'addr', 'device_type'])
        OperatorValue = namedtuple(
            'OperatorValue', ['size', 'timestamp', 'total_allocate_memory', 'total_reserve_memory']
        )
        for item in self._op_data:
            if item.size > 0:
                item_key = OperatorKey(operator=item.operator, addr=item.addr, device_type=item.device_type)
                item_value = OperatorValue(
                    size=item.size,
                    timestamp=item.timestamp,
                    total_allocate_memory=item.total_allocate_memory,
                    total_reserve_memory=item.total_reserve_memory,
                )
                allocated_data[item_key] = item_value
            elif item.size < 0:
                item_key = OperatorKey(operator=item.operator, addr=item.addr, device_type=item.device_type)
                item_value = OperatorValue(
                    size=item.size,
                    timestamp=item.timestamp,
                    total_allocate_memory=item.total_allocate_memory,
                    total_reserve_memory=item.total_reserve_memory,
                )
                if item_key in allocated_data:
                    allocated_value = allocated_data[item_key]
                    op_mem = [
                        item.operator,
                        allocated_value.size,
                        allocated_value.timestamp,
                        item_value.timestamp,
                        item_value.timestamp - allocated_value.timestamp,
                        allocated_value.total_allocate_memory,
                        allocated_value.total_reserve_memory,
                        item_value.total_allocate_memory,
                        item_value.total_reserve_memory,
                        item_key.device_type,
                    ]
                    self._operator_memory.append(op_mem)
                    allocated_data.pop(item_key)
        if len(allocated_data) > 0:
            for key, value in allocated_data.items():
                self._operator_memory.append(
                    [
                        key.operator,
                        value.size,
                        value.timestamp,
                        NumberConstant.NULL_NUMBER,
                        NumberConstant.NULL_NUMBER,
                        value.total_allocate_memory,
                        value.total_reserve_memory,
                        NumberConstant.NULL_NUMBER,
                        NumberConstant.NULL_NUMBER,
                        key.device_type,
                    ]
                )
        self._reformat_data()

    def _calc_memory_record(self: any) -> None:
        self._memory_record = [
            ['GE', item.timestamp, item.total_reserve_memory, item.total_allocate_memory, item.device_type]
            for item in self._op_data
        ]

    def _reformat_data(self: any) -> list:
        hash_dict_data = HashDictData(self._project_path)
        ge_dict = hash_dict_data.get_ge_hash_dict()
        for item in self._operator_memory:
            name = ''
            if item[0] in ge_dict:
                name = ge_dict[item[0]]
            item.append(name)
