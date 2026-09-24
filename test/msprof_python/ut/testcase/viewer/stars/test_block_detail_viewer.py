import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.trace_view_manager import TraceViewManager
from mscalculate.ascend_task.ascend_task import TopDownTask
from profiling_bean.db_dto.pmu_block_dto import PmuBlockDto
from viewer.stars.block_detail_viewer import BlockDetailViewer
from constant.info_json_construct import InfoJson


class TestBlockDetailViewer(unittest.TestCase):

    def test_get_timeline_header_should_return_list_of_length_13(self):
        InfoJson(pid=0).apply()
        block_detail_viewer = BlockDetailViewer({}, {})
        header = block_detail_viewer.get_timeline_header()
        pid = 0
        thread_name = "thread_name"
        thread_sort_index = "thread_sort_index"
        expect_header = [
            ["process_name", pid, 0, "Block Detail"],
            [thread_name, pid, 1, "AIC Earliest"],
            [thread_name, pid, 2, "AIC Latest"],
            [thread_name, pid, 3, "AIV Earliest"],
            [thread_name, pid, 4, "AIV Latest"],
            [thread_sort_index, pid, 1, 1],
            [thread_sort_index, pid, 2, 2],
            [thread_sort_index, pid, 3, 3],
            [thread_sort_index, pid, 4, 4]
        ]
        self.assertEqual(9, len(header))
        self.assertEqual(TraceViewManager.metadata_event(expect_header), header)

    def test_get_block_pmu_timeline_data_when_given_pmu_block_data(self):
        pmu_time_line_data = [
            PmuBlockDto(batch_id=0,
                        core_type=0,
                        duration=408.98817158366825,
                        start_time=43997733221071.91,
                        stream_id=3,
                        subtask_id=0,
                        task_id=0),
            PmuBlockDto(batch_id=0,
                        core_type=0,
                        duration=408.98817158366825,
                        start_time=43997733221071.91,
                        stream_id=3,
                        subtask_id=0,
                        task_id=0),
            PmuBlockDto(batch_id=0,
                        core_type=1,
                        duration=408.98817158366825,
                        start_time=43997733221071.91,
                        stream_id=3,
                        subtask_id=0,
                        task_id=0),
            PmuBlockDto(batch_id=0,
                        core_type=1,
                        duration=408.98817158366825,
                        start_time=43997733221071.91,
                        stream_id=3,
                        subtask_id=0,
                        task_id=0),
        ]
        ge_data = {
            "0-3-0-0": {
                "op_name": 'FFN',
                "task_type": 'FFN'
            }
        }
        task_data_list = [
            TopDownTask(model_id=1, index_id=-1, stream_id=3, task_id=0, context_id=0, batch_id=0,
                        start_time=43997733221060, duration=450, host_task_type='KERNEL_AIVEC',
                        device_task_type='AIV_SQE', connection_id=948)
        ]
        configs, params = {}, {"project": "", "data_type": "v6_block_pmu", "export_type": "timeline"}

        InfoConfReader()._info_json = {"pid": "0"}
        with mock.patch('msmodel.stars.ffts_pmu_model.V6PmuViewModel.get_timeline_data',
                        return_value=pmu_time_line_data), \
                mock.patch('msmodel.stars.ffts_pmu_model.V6PmuViewModel.check_db', return_value=True), \
                mock.patch('msmodel.stars.ffts_pmu_model.V6PmuViewModel.check_table', return_value=True), \
                mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.get_op_name_from_ge_by_id',
                           return_value=ge_data), \
                mock.patch('msmodel.task_time.ascend_task_model.AscendTaskModel.get_ascend_task_data_without_unknown',
                           return_value=task_data_list):
            check = BlockDetailViewer(configs, params)
            ret = check.get_timeline_data()
            self.assertEqual(len(ret), 13)

    def test_get_block_pmu_timeline_data_when_given_pmu_block_data_without_task_data(self):
        pmu_time_line_data = [
            PmuBlockDto(batch_id=0,
                        core_type=0,
                        duration=408.98817158366825,
                        start_time=43997733221071.91,
                        stream_id=3,
                        subtask_id=0,
                        task_id=0),
            PmuBlockDto(batch_id=0,
                        core_type=0,
                        duration=408.98817158366825,
                        start_time=43997733221071.91,
                        stream_id=3,
                        subtask_id=0,
                        task_id=0)
        ]
        ge_data = {
            "0-3-0-0": {
                "op_name": 'FFN',
                "task_type": 'FFN'
            }
        }
        configs, params = {}, {"project": "", "data_type": "v6_block_pmu", "export_type": "timeline"}
        InfoConfReader()._info_json = {"pid": "0"}
        with mock.patch('msmodel.stars.ffts_pmu_model.V6PmuViewModel.get_timeline_data',
                        return_value=pmu_time_line_data), \
                mock.patch('msmodel.stars.ffts_pmu_model.V6PmuViewModel.check_db', return_value=True), \
                mock.patch('msmodel.stars.ffts_pmu_model.V6PmuViewModel.check_table', return_value=True), \
                mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.get_op_name_from_ge_by_id',
                           return_value=ge_data), \
                mock.patch('msmodel.task_time.ascend_task_model.AscendTaskModel.get_ascend_task_data_without_unknown',
                           return_value=[]):
            check = BlockDetailViewer(configs, params)
            ret = check.get_timeline_data()
            self.assertEqual(len(ret), 0)
