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
from collections import defaultdict

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ge_logic_stream_singleton import GeLogicStreamSingleton
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.stars.ffts_pmu_model import V6PmuViewModel
from msmodel.stars.op_summary_model import OpSummaryModel
from msmodel.task_time.ascend_task_model import AscendTaskModel
from viewer.interface.base_viewer import BaseViewer


class BlockDetailViewer(BaseViewer):
    """
    class for get block detail data
    """

    AIC_EARLIEST_INDEX = 1
    AIC_LATEST_INDEX = 2
    AIV_EARLIEST_INDEX = 3
    AIV_LATEST_INDEX = 4

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.project_dir = self.params.get(StrConstant.PARAM_RESULT_DIR)

    @staticmethod
    def get_timeline_header() -> list:
        pid = InfoConfReader().get_json_pid_data()
        thread_name = "thread_name"
        thread_sort_index = "thread_sort_index"
        header = [
            ["process_name", pid, InfoConfReader().get_json_tid_data(), TraceViewHeaderConstant.PROCESS_BLOCK_DETAIL],
            [thread_name, pid, BlockDetailViewer.AIC_EARLIEST_INDEX, "AIC Earliest"],
            [thread_name, pid, BlockDetailViewer.AIC_LATEST_INDEX, "AIC Latest"],
            [thread_name, pid, BlockDetailViewer.AIV_EARLIEST_INDEX, "AIV Earliest"],
            [thread_name, pid, BlockDetailViewer.AIV_LATEST_INDEX, "AIV Latest"],
            [thread_sort_index, pid, BlockDetailViewer.AIC_EARLIEST_INDEX, 1],
            [thread_sort_index, pid, BlockDetailViewer.AIC_LATEST_INDEX, 2],
            [thread_sort_index, pid, BlockDetailViewer.AIV_EARLIEST_INDEX, 3],
            [thread_sort_index, pid, BlockDetailViewer.AIV_LATEST_INDEX, 4],
        ]
        return TraceViewManager.metadata_event(header)

    @staticmethod
    def set_log_result_list(data, pid, name, tid):
        result_list = []
        start_time = InfoConfReader().time_from_syscnt(data.start_time)
        logic_stream_id = GeLogicStreamSingleton().get_logic_stream_id(data.stream_id)
        result_list.append(
            [
                f"Stream {logic_stream_id} {name}",
                pid,
                tid,
                InfoConfReader().trans_into_local_time(start_time),
                data.duration / DBManager.NSTOUS if data.duration > 0 else 0,
                {
                    "Task Type": data.device_task_type,
                    "Physic Stream Id": data.stream_id,
                    "Task Id": data.task_id,
                    "Subtask Id": data.context_id,
                    "Block Id": data.block_id,
                    "Core Type": "AIC" if data.core_type == 0 else "AIV",
                    "Core Id": data.core_id,
                },
            ]
        )
        return result_list

    @staticmethod
    def set_pmu_result_list(datas, pid, block_pmu_timeline_data, name):
        datas = list(datas)
        # 过滤和排序
        aic_datas = sorted([data for data in datas if data.core_type == 0], key=lambda x: x.start_time)
        aiv_datas = sorted([data for data in datas if data.core_type == 1], key=lambda x: x.start_time)
        # 获取最早和最晚的数据，如果为空则返回空列表
        aic_earliest, aic_latest = (aic_datas[0], aic_datas[-1]) if aic_datas else (None, None)
        aiv_earliest, aiv_latest = (aiv_datas[0], aiv_datas[-1]) if aiv_datas else (None, None)
        for i, data in enumerate([aic_earliest, aic_latest, aiv_earliest, aiv_latest]):
            if not data:
                continue
            block_pmu_timeline_data.append(
                [
                    f"Stream {data.stream_id} {name}",
                    pid,
                    i + 1,  # thread_index
                    InfoConfReader().trans_into_local_time(data.start_time),
                    data.duration,
                    {
                        "Physic Stream Id": data.stream_id,
                        "Task Id": data.task_id,
                        "Batch Id": data.batch_id,
                        "Subtask Id": data.subtask_id,
                        "Core Id": data.core_id,
                    },
                ]
            )

    def get_data_from_db(self: any) -> list:
        with V6PmuViewModel(self.project_dir) as _model:
            block_pmu_data = _model.get_timeline_data()
        return block_pmu_data

    def get_trace_timeline(self: any, data_list: list) -> list:
        """
        to format data to chrome trace json
        :return: timeline_trace list
        """
        block_pmu_data = data_list
        block_log_timeline_data = []
        pid = InfoConfReader().get_json_pid_data()
        with OpSummaryModel({"result_dir": self.params.get("project")}) as _model:
            node_name_dict = _model.get_op_name_from_ge_by_id()
        with AscendTaskModel(self.project_dir, [DBNameConstant.TABLE_ASCEND_TASK]) as model:
            task_data_list = model.get_ascend_task_data_without_unknown()
        # Block PMU data
        if block_pmu_data:
            block_pmu_timeline_data = []
            pmu_grouped = defaultdict(list)
            for data in block_pmu_data:
                pmu_grouped[data.task_id].append(data)
            for task_data in task_data_list:
                # 检查是否存在对应的block_pmu_data组
                if task_data.task_id in pmu_grouped:
                    pmu_data_list = pmu_grouped[task_data.task_id]
                    pmu_group = self.get_block_pmu_group(pmu_data_list, task_data)
                    name = node_name_dict.get(
                        f"{task_data.task_id}-{task_data.stream_id}-{task_data.context_id}-{task_data.batch_id}", {}
                    ).get('op_name')
                    self.set_pmu_result_list(pmu_group, pid, block_pmu_timeline_data, name)
            # 调用time_graph_trace方法
            block_pmu_timeline_data = TraceViewManager.time_graph_trace(
                TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, block_pmu_timeline_data
            )
        else:
            block_pmu_timeline_data = []

        result_list = block_log_timeline_data + block_pmu_timeline_data
        if not result_list:
            return []
        return self.get_timeline_header() + result_list

    def get_block_pmu_group(self, pmu_data_list, task_data):
        task_start = task_data.start_time
        task_end = task_data.start_time + task_data.duration
        # 判断pmu时间是否和task时间有交集 返回有交集的pmu list
        return (
            pmu_data
            for pmu_data in pmu_data_list
            if not (task_end < pmu_data.start_time or pmu_data.start_time + pmu_data.duration < task_start)
        )
