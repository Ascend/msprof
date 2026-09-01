#!/usr/bin/python3
# -*- coding: utf-8 -*-
# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

import os
from unittest import mock

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode
from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.hccl.hccl_model import HCCLModel
from msmodel.hccl.hccl_model import HcclViewModel
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msmodel.hccl.hccl_model'


class TestHCCLModel(TestDirCRBaseModel):
    sample_config = {'result_dir': '/tmp/result',
                     'tag_id': 'JOBEJGBAHABDEEIJEDFHHFAAAAAAAAAA',
                     'device_id': '127.0.0.1'
    }
    file_list = {DataTag.HCCL: ['HCCL.hcom_allReduce_1_1_1.1.slice_0']}

    DIR_PATH = os.path.join(os.path.dirname(__file__), "DT_HCCL_MODEL")
    PROF_DIR = os.path.join(DIR_PATH, 'PROF1')
    PROF_DEVICE_DIR = os.path.join(PROF_DIR, 'device')
    PROF_HOST_DIR = os.path.join(PROF_DIR, 'host')

    def test_flush(self):
        with mock.patch(NAMESPACE + '.HCCLModel.insert_data_to_db'):
            HCCLModel("", [" "]).flush([])

    # def test_get_hccl_data(self):
    #     data = [1, 2, 3, 4,
    #             "{'notify id': 4294967840, 'duration estimated': 0.8800048828125, 'stage': 4294967295, "
    #             "'step': 4294967385, 'bandwidth': 'NULL', 'stream id': 8, 'task id': 34, 'task type': 'Notify Record',"
    #             " 'src rank': 2, 'dst rank': 1, 'transport type': 'SDMA', 'size': None, 'tag': 'all2allvc_1_5'}"]
    #     col = ["hccl_name", "plane_id", "timestamp", "duration", "args"]
    #     create_sql = "create table IF NOT EXISTS {0} " \
    #                  "(name TEXT, " \
    #                  "plane_id INTEGER, " \
    #                  "timestamp REAL, " \
    #                  "duration REAL, " \
    #                  "args TEXT)".format(DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE)
    #     test = HcclTask()
    #     for index, i in enumerate(data):
    #         if hasattr(test, col[index]):
    #             setattr(test, col[index], i)
    #     with DBOpen(DBNameConstant.DB_HCCL) as db_open:
    #         db_open.create_table(create_sql)
    #         with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[test]):
    #             check = HCCLModel("", [DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE])
    #             check.cur = db_open.db_curs
    #             check.get_hccl_data()

    def test_get_hccl_task_data_should_return_empty_when_attach_db_failed(self):
        with mock.patch(NAMESPACE + '.HcclViewModel.attach_to_db', return_value=False):
            model = HcclViewModel("", DBNameConstant.DB_HCCL, [DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE])
            ret = model.get_hccl_task_data()
            self.assertEqual([], ret)

    def test_get_hccl_task_data_should_return_empty_when_device_id_na(self):
        with mock.patch(NAMESPACE + '.HcclViewModel.attach_to_db', return_value=True):
            InfoConfReader()._info_json = {}
            model = HcclViewModel("", DBNameConstant.DB_HCCL, [DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE])
            ret = model.get_hccl_task_data()
            self.assertEqual([], ret)

    def test_get_hccl_task_data_should_return_diff_result_when_query_diff_deviceid(self):
        # model_id, index_id, name, group_name, plane_id, timestamp,
        # stream_id, task_id, context_id, batch_id, device_id, is_master,
        # local_rank, remote_rank, transport_type, size, data_type, link_type,
        # notify_id, rdma_type, thread_id, rank_size, op_id
        hccl_task_data = [
            (1, -1, "Memcpy", "1", 1, 1, 100, 200, 300, 0, 0, 0, 1, 1, "1", 20, "a", "b", "2", "RDMA_SEND", 123, 8, -1),
            (1, -1, "Notify_Wait", "1", 1, 1, 102, 202, 302, 0, 0, 0, 1, 1, "1", 20, "a", "b", "2", "INVALID", 124, 8, -1),
            (1, -1, "Notify_Record", "1", 1, 1, 103, 203, 303, 0, 0, 0, 1, 1, "1", 20, "a", "b", "2",
             "INVALID", 123, 8, -1),
            (1, -1, "Memcpy", "1", 1, 1, 999, 204, 304, 0, 0, 0, 1, 1, "1", 20, "a", "b", "2", "INVALID", 125, 8, -1),
            (1, -1, "Memcpy", "1", 1, 1, 105, 205, 305, 0, 1, 0, 1, 1, "1", 20, "a", "b", "2", "INVALID", 123, 8, -1),
            (1, -1, "Memcpy", "1", 1, 1, 999, 206, 306, 0, 2, 0, 1, 1, "1", 20, "a", "b", "2", "INVALID", 132, 8, -1),
        ]

        # model_id, index_id, stream_id, task_id, context_id, batch_id, start_time,
        # duration, host_task_type, device_task_type, connection_id
        ascend_task_data = [
            (1, -1, 100, 200, 300, 0, 1, 1, "PROFILING_ENABLE", "PLACE_HOLDER_SQE", 52212),
            (1, -1, 102, 202, 302, 0, 1, 1, "PROFILING_ENABLE", "PLACE_HOLDER_SQE", 52212),
            (1, -1, 103, 203, 303, 0, 1, 1, "PROFILING_ENABLE", "PLACE_HOLDER_SQE", 52212),
            (1, -1, 104, 204, 304, 0, 1, 1, "PROFILING_ENABLE", "PLACE_HOLDER_SQE", 52212),
            (1, -1, 105, 205, 305, 0, 1, 1, "PROFILING_ENABLE", "PLACE_HOLDER_SQE", 52212),
            (1, -1, 106, 206, 306, 0, 1, 1, "PROFILING_ENABLE", "PLACE_HOLDER_SQE", 52212),
        ]

        model = HcclViewModel(self.PROF_DEVICE_DIR, DBNameConstant.DB_HCCL,
                              [DBNameConstant.TABLE_HCCL_TASK, DBNameConstant.TABLE_ASCEND_TASK])

        model.init()
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_HCCL_TASK, hccl_task_data)
        model.insert_data_to_db(DBNameConstant.TABLE_ASCEND_TASK, ascend_task_data)

        with mock.patch(NAMESPACE + '.HcclViewModel.attach_to_db', return_value=True):
            try:
            # test on device_0 matched case
                InfoConfReader()._info_json = {"devices": "0"}
                ret = model.get_hccl_task_data()
                self.assertEqual(len(ret), 3)

                # test on device_1 matched case
                InfoConfReader()._info_json = {"devices": "1"}
                ret = model.get_hccl_task_data()
                self.assertEqual(len(ret), 1)

                # test on device_2 matched case
                InfoConfReader()._info_json = {"devices": "2"}
                ret = model.get_hccl_task_data()
                self.assertEqual(len(ret), 0)
                InfoConfReader()._info_json = {}
            finally:
                model.finalize()


    def test_get_hccl_ops_should_return_empty_when_device_id_na(self):
        with mock.patch(NAMESPACE + '.HcclViewModel.attach_to_db', return_value=True):
            InfoConfReader()._info_json = {}
            model = HcclViewModel("", DBNameConstant.DB_HCCL, [DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE])
            ret = model.get_hccl_ops(1, 1)
            self.assertEqual([], ret)

    def test_get_hccl_ops_should_return_diff_result_when_query_diff_device_id_in_op_scene(self):
        broadcast_op_name = "hcom_broadcast_"
        task_type = "HCCL"
        group_name = "728400854065026987"
        alg_type = "HD-MESH"
        # device_id, model_id, index_id, thread_id, op_name, task_type, op_type, connection_id, kfc_connection_ids,
        # relay, retry, data_type, alg_type, count, group_name
        hccl_op_data = [
            (0, 1, 1, 1, broadcast_op_name, task_type, broadcast_op_name, 1, 1, 1, 0, "INT8", alg_type, 123, group_name),
            (0, 1, 1, 1, broadcast_op_name, task_type, broadcast_op_name, 1, 1, 1, 0, "INT16", alg_type, 489, group_name),
            (0, 1, 1, 1, broadcast_op_name, task_type, broadcast_op_name, 1, 1, 1, 0, "INT32", alg_type, 984, group_name),
            (1, 1, 1, 1, broadcast_op_name, task_type, broadcast_op_name, 1, 1, 1, 0, "INT64", alg_type, 892, group_name),
            (2, 1, 1, 1, broadcast_op_name, task_type, broadcast_op_name, 1, 1, 1, 0, "FP16", alg_type, 369, group_name),
        ]

        model = HcclViewModel(self.PROF_DEVICE_DIR, DBNameConstant.DB_HCCL, [DBNameConstant.TABLE_HCCL_OP])
        model.init()
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_HCCL_OP, hccl_op_data)

        with mock.patch(NAMESPACE + '.HcclViewModel.attach_to_db', return_value=True):
            scene = ProfilingScene()
            scene._scene = Constant.SINGLE_OP

            # test on device_0 matched case
            InfoConfReader()._info_json = {"devices": "0"}
            ret = model.get_hccl_ops(1, 1)
            self.assertEqual(len(ret), 3)

            # test on device_1 matched case
            InfoConfReader()._info_json = {"devices": "1"}
            ret = model.get_hccl_ops(1, 1)
            self.assertEqual(len(ret), 1)

            InfoConfReader()._info_json = {}
            scene._scene = None

        model.finalize()

    def test_get_hccl_ops_should_return_diff_result_when_query_diff_device_id_in_graph_scene(self):
        broadcast_op_name = "hcom_broadcast_"
        task_type = "HCCL"
        group_name = "728400854065026987"
        alg_type = "HD-MESH"
        # device_id, model_id, index_id, thread_id, op_name, task_type, op_type, connection_id, kfc_connection_ids,
        # relay, retry, data_type, alg_type, count, group_name
        hccl_op_data = [
            (0, 1, 1, 1, broadcast_op_name, task_type, broadcast_op_name, 1, 1, 1, 0, "INT8", alg_type, 123, group_name),
            (0, 2, 2, 1, broadcast_op_name, task_type, broadcast_op_name, 1, 1, 1, 0, "INT16", alg_type, 146, group_name),
            (0, 2, 2, 1, broadcast_op_name, task_type, broadcast_op_name, 1, 1, 1, 0, "INT32", alg_type, 692, group_name),
            (1, 1, 1, 1, broadcast_op_name, task_type, broadcast_op_name, 1, 1, 1, 0, "INT64", alg_type, 437, group_name),
            (2, 3, 3, 1, broadcast_op_name, task_type, broadcast_op_name, 1, 1, 1, 0, "FP16", alg_type, 831, group_name),
        ]

        model = HcclViewModel(self.PROF_DEVICE_DIR, DBNameConstant.DB_HCCL, [DBNameConstant.TABLE_HCCL_OP])
        model.init()
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_HCCL_OP, hccl_op_data)

        with mock.patch(NAMESPACE + '.HcclViewModel.attach_to_db', return_value=True):
            scene = ProfilingScene()
            scene._scene = Constant.STEP_INFO
            ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)

            # test on device_0 matched case
            InfoConfReader()._info_json = {"devices": "0"}
            ret = model.get_hccl_ops(2, 2)
            self.assertEqual(len(ret), 2)

            # test on device_1 matched case
            InfoConfReader()._info_json = {"devices": "1"}
            ret = model.get_hccl_ops(1, 1)
            self.assertEqual(len(ret), 1)

            InfoConfReader()._info_json = {}
            scene._scene = None

        model.finalize()

    def test_get_hccl_op_data_by_group_sql(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data'):
            check = HcclViewModel("", DBNameConstant.DB_HCCL_SINGLE_DEVICE,
                                  [DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE])
            check.get_hccl_op_data_by_group()

    def test_get_hccl_op_data_by_group_sql_should_return_empty_when_no_master(self):
        # model_id, index_id, op_name, iteration, hccl_name,
        # group_name, first_timestamp, plane_id, timestamp, duration,
        # is_dynamic, task_type, op_type, connection_id, is_master
        hccl_op_data = [
            (4294967295, -1, "hcom_allReduce__111_5939", 0, "Notify_Wait",
             9402293354310575111, 642053765422, 1, 6286049445495.44, 1324686.46679688,
             1, "HCCL", "hcom_allReduce_", 733589, 0),
            (4294967295, -1, "hcom_allReduce__111_5939", 0, "Notify_Wait",
             9402293354310575111, 642070508282, 2, 6286050773361.97, 1409648.16503906,
             1, "HCCL", "hcom_allReduce_", 733589, 0),
        ]
        model = HcclViewModel("", DBNameConstant.DB_HCCL_SINGLE_DEVICE,
                              [DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE])
        model.init()
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE, hccl_op_data)
        with mock.patch(NAMESPACE + '.HcclViewModel.attach_to_db', return_value=True):
            hccl_result = model.get_hccl_op_data_by_group()
            self.assertEqual(len(hccl_result), 0)
        model.finalize()

    def test_get_hccl_op_data_by_group_should_return_all_hccl_op_rows(self):
        hccl_op_data = [
            (1, 0, "hcom_allReduce__1", "HCCL", "hcom_allReduce_", 100, 110,
             0, 0, "FP16", "NA", 1, "group", 1, 8, 0),
            (1, 0, "hcom_allReduce__1", "HCCL", "hcom_allReduce_", 100, 110,
             0, 0, "FP16", "NA", 1, "group", 1, 8, 0),
        ]
        model = HcclViewModel(
            self.PROF_DEVICE_DIR, DBNameConstant.DB_HCCL_SINGLE_DEVICE,
            [DBNameConstant.TABLE_HCCL_OP_SINGLE_DEVICE]
        )
        model.init()
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_HCCL_OP_SINGLE_DEVICE, hccl_op_data)
        result = model.get_hccl_op_data_by_group()
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0].op_name, "hcom_allReduce__1")
        self.assertEqual(result[0].start, 100)
        self.assertEqual(result[0].end, 110)
        model.finalize()

    def test_get_hccl_op_data_by_group_should_return_kfc_op(self):
        kfc_op_data = [
            (1, 0, "mc2_matmul_allreduce", 200, 250, "group", 2, "MatmulAllReduce",
             0, 0, "FP16", "NA", 1, 8, 0, 1),
        ]
        model = HcclViewModel(
            self.PROF_DEVICE_DIR, DBNameConstant.DB_HCCL_SINGLE_DEVICE,
            [DBNameConstant.TABLE_KFC_OP]
        )
        model.init()
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_KFC_OP, kfc_op_data)
        result = model.get_hccl_op_data_by_group(DBNameConstant.TABLE_KFC_OP)
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0].op_name, "mc2_matmul_allreduce")
        self.assertEqual(result[0].op_type, "MatmulAllReduce")
        self.assertEqual(result[0].start, 200)
        self.assertEqual(result[0].end, 250)
        model.finalize()

    def test_get_hccl_op_data_by_group_should_return_empty_when_kfc_table_missing(self):
        model = HcclViewModel(self.PROF_DEVICE_DIR, DBNameConstant.DB_HCCL_SINGLE_DEVICE, [])
        model.init()
        result = model.get_hccl_op_data_by_group(DBNameConstant.TABLE_KFC_OP)
        self.assertEqual(result, [])
        model.finalize()

    def test_get_hccl_op_info_from_table_sql(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data'):
            check = HcclViewModel("", DBNameConstant.DB_HCCL_SINGLE_DEVICE,
                                  [DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE])
            check.get_hccl_op_info_from_table()

    def test_get_hccl_op_time_section_sql(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data'):
            check = HcclViewModel("", DBNameConstant.DB_HCCL_SINGLE_DEVICE,
                                  [DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE])
            check.get_hccl_op_time_section()

    def test_create_table_by_name_should_drop_table_when_tabel_exist(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            check = HcclViewModel("", DBNameConstant.DB_HCCL_SINGLE_DEVICE,
                                  [DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE])
            check.create_table_by_name(table_name='test_name')

    def test_create_table_by_name_should_not_drop_table_when_tabel_not_exist(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'):
            check = HcclViewModel("", DBNameConstant.DB_HCCL_SINGLE_DEVICE,
                                  [DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE])
            check.create_table_by_name(table_name='test_name')

    def test_rebuild_hccl_task_table_should_delegate_to_create_table_by_name(self):
        """rebuild_hccl_task_table 委托调用 create_table_by_name(HCCL_TASK_SINGLE_DEVICE)"""
        with mock.patch.object(check := HcclViewModel("", DBNameConstant.DB_HCCL_SINGLE_DEVICE,
                                                       [DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE]),
                               'create_table_by_name') as mock_create:
            check.rebuild_hccl_task_table()
            mock_create.assert_called_once_with(DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE)

    def test_rebuild_hccl_op_table_should_delegate_to_create_table_by_name(self):
        """rebuild_hccl_op_table 委托调用 create_table_by_name(HCCL_OP_SINGLE_DEVICE)"""
        with mock.patch.object(check := HcclViewModel("", DBNameConstant.DB_HCCL_SINGLE_DEVICE,
                                                       [DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE]),
                               'create_table_by_name') as mock_create:
            check.rebuild_hccl_op_table()
            mock_create.assert_called_once_with(DBNameConstant.TABLE_HCCL_OP_SINGLE_DEVICE)

    def test_rebuild_hccl_op_report_table_should_delegate_to_create_table_by_name(self):
        """rebuild_hccl_op_report_table 委托调用 create_table_by_name(HCCL_OP_REPORT)"""
        with mock.patch.object(check := HcclViewModel("", DBNameConstant.DB_HCCL_SINGLE_DEVICE,
                                                       [DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE]),
                               'create_table_by_name') as mock_create:
            check.rebuild_hccl_op_report_table()
            mock_create.assert_called_once_with(DBNameConstant.TABLE_HCCL_OP_REPORT)
