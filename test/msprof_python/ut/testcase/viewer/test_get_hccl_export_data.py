#!/usr/bin/python3
# coding=utf-8
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
import json
import unittest
from collections import OrderedDict
from collections import namedtuple
from unittest import mock

from common_func.msprof_object import CustomizedNamedtupleFactory
from mscalculate.hccl.hccl_task import HcclOps
from mscalculate.hccl.hccl_task import HcclTask
from common_func.info_conf_reader import InfoConfReader
from constant.constant import ITER_RANGE
from sqlite.db_manager import DBManager
from viewer.get_hccl_export_data import HCCLExport

NAMESPACE = 'viewer.get_hccl_export_data'
PARAMS = {'data_type': 'hccl', 'project': '',
          'device_id': '1', 'job_id': 'job_default', 'export_type': 'timeline',
          'iter_id': ITER_RANGE, 'export_format': None, 'model_id': -1}
HcclTaskTuple = CustomizedNamedtupleFactory.generate_named_tuple_from_dto(HcclTask, [])


class TestHCCLExport(unittest.TestCase):
    def setUp(self) -> None:
        InfoConfReader()._info_json = {"pid": 1}

    def tearDown(self) -> None:
        InfoConfReader()._info_json = {}

    def test_get_hccl_timeline_data(self):
        db_manager = DBManager()
        table_name = 'HCCLTaskSingleDevice'
        create_sql = "CREATE TABLE IF NOT EXISTS {0} (model_id int, index_id int, iteration int)".format(table_name)

        data = ((1, 1, 1),)
        insert_sql = "insert into {0} values ({1})".format(table_name, "?," * (len(data[0]) - 1) + "?")
        conn, curs = db_manager.create_table('hccl.db', create_sql, insert_sql, data)
        res = HCCLExport(PARAMS).get_hccl_timeline_data()
        self.assertEqual(res, [])
        with mock.patch('msmodel.hccl.hccl_model.HcclViewModel.get_all_data', return_value=[]), \
                mock.patch('msmodel.interface.view_model.DBManager.create_connect_db', return_value=(conn, curs)), \
                mock.patch('msmodel.hccl.hccl_model.HcclViewModel.check_table', return_value=True), \
                mock.patch('os.path.exists', return_value=True):
            res = HCCLExport(PARAMS).get_hccl_timeline_data()
        self.assertEqual(res, [])
        conn, curs = db_manager.create_table('hccl.db')
        with mock.patch(NAMESPACE + '.HCCLExport._format_hccl_data'), \
                mock.patch('msmodel.interface.view_model.DBManager.create_connect_db', return_value=(conn, curs)), \
                mock.patch('msmodel.hccl.hccl_model.HcclViewModel.check_table', return_value=True), \
                mock.patch('os.path.exists', return_value=True):
            res = HCCLExport(PARAMS).get_hccl_timeline_data()
        self.assertEqual(len(res), 5)
        conn, curs = db_manager.create_table('hccl.db')
        db_manager.destroy((conn, curs))

    def test_format_hccl_data(self):
        hccl_data = [
            HcclTask(plane_id=1, task_id=2, stream_id=3, hccl_name="0", duration=0, timestamp=0,
                     group_name="1"),
            HcclTask(plane_id=2, task_id=3, stream_id=4, hccl_name="0", duration=0, timestamp=0,
                     group_name="1")
        ]

        op_data = namedtuple('model',
                             ['op_name', 'timestamp', 'args', 'duration',
                              "group_name", "connection_id", "model_id"])
        op_data = [op_data('test_1', 0, [1, 2, 3], 0, "1", 0, 10)]
        op_info_data = {0: HcclOps(op_name='test', data_type='INT64', connection_id=0, model_id=10, alg_type='HD-NB')}
        InfoConfReader()._info_json = {"pid": 1}
        with mock.patch('msmodel.hccl.hccl_model.HcclViewModel.get_hccl_op_data_by_group', return_value=op_data), \
                mock.patch('msmodel.hccl.hccl_model.HcclViewModel.get_hccl_op_info_from_table',
                           return_value=op_info_data), \
                mock.patch('os.path.exists', return_value=False):
            hccl = HCCLExport(PARAMS)
            hccl._get_meta_data(hccl_data, [])
            hccl._format_hccl_data(hccl_data)
            self.assertEqual(len(hccl.result), 9)
        with mock.patch('msmodel.hccl.hccl_model.HcclViewModel.get_hccl_op_data_by_group', return_value=op_data), \
                mock.patch('msmodel.hccl.hccl_model.HcclViewModel.get_hccl_op_info_from_table',
                           return_value=op_info_data), \
                mock.patch('os.path.exists', return_value=True):
            hccl = HCCLExport(PARAMS)
            hccl._get_meta_data(hccl_data, [])
            hccl._format_hccl_data(hccl_data)
            # 1 + 3 * 2 + 3
            self.assertEqual(len(hccl.result), 10)
            hccl_op_args = hccl.result[-1]['args']
            self.assertEqual(hccl_op_args['data_type'], 'INT64')
            self.assertEqual(hccl_op_args['alg_type'], 'HD-NB')

    def test__add_hccl_bar_should_return_correct_hccl_bar(self):
        hccl = HCCLExport(PARAMS)
        hccl._add_hccl_bar()
        expect_res = [
            OrderedDict(
                [
                    ('name', 'process_name'),
                    ('pid', 1),
                    ('tid', 0),
                    ('args', OrderedDict([('name', 'Communication')])),
                    ('ph', 'M')
                ]
            )
        ]
        self.assertEqual(hccl.result, expect_res)

    def test__init_hccl_group_should_return_3_group_when_5_hccl_op_in_three_comm_domain(self):
        hccl_data = [
            HcclTaskTuple(group_name="1", plane_id=0), HcclTaskTuple(group_name="1", plane_id=1),
            HcclTaskTuple(group_name="2", plane_id=1), HcclTaskTuple(group_name="3", plane_id=0),
            HcclTaskTuple(group_name="2", plane_id=0), HcclTaskTuple(group_name="3", plane_id=-1)
        ]

        hccl = HCCLExport(PARAMS)
        groups = hccl._init_hccl_group(hccl_data, [])
        self.assertEqual(groups['1'][0].group_name, "1")
        self.assertEqual(groups['1'][0].planes, {0, 1})
        self.assertEqual(groups['1'][0].id, 0)
        self.assertEqual(groups['2'][0].group_name, "2")
        self.assertEqual(groups['2'][0].planes, {0, 1})
        self.assertEqual(groups['2'][0].id, 1)
        self.assertEqual(groups['3'][0].group_name, "3")
        self.assertEqual(groups['3'][0].planes, {0})
        self.assertEqual(groups['3'][0].id, 2)

    def test__add_group_threads_should_return_comm_op_group_and_index_3_when_comm_op_in_2_plane(self):
        hccl = HCCLExport(PARAMS)
        group = hccl.HcclGroup("hccl_word", {0, 1}, 0, -1, False)
        index = hccl._add_group_threads(group, 0, True)
        self.assertEqual(index, 3)
        self.assertEqual(len(hccl.result), 6)
        self.assertEqual(hccl.result[0].get('args').get('name'), 'Group hccl_word Communication')

    def test__add_group_threads_should_return_comm_and_index_3_when_comm_op_in_2_plane_invalid_group(self):
        hccl = HCCLExport(PARAMS)
        group = hccl.HcclGroup("1", {0, 1}, 0, -1, False)
        index = hccl._add_group_threads(group, 0, False)
        self.assertEqual(index, 3)
        self.assertEqual(len(hccl.result), 6)
        self.assertEqual(hccl.result[0].get('args').get('name'), 'Communication')

    def test__get_meta_data_should_return_1_hccl_bar_3_group_threads_and_5_plane_threads_when_8_comm_op_in_2_g(self):
        hccl_data = [
            HcclTask(group_name="1", plane_id=0), HcclTask(group_name="1", plane_id=1),
            HcclTask(group_name="2", plane_id=1), HcclTask(group_name="3", plane_id=0),
            HcclTask(group_name="2", plane_id=0), HcclTask(group_name="1", plane_id=1),
            HcclTask(group_name="2", plane_id=1), HcclTask(group_name="3", plane_id=0)
        ]
        hccl = HCCLExport(PARAMS)
        hccl._get_meta_data(hccl_data, [])
        # 1 + (3+5)*2=17
        self.assertEqual(len(hccl.result), 17)

    def test_format_hccl_communication_data(self):
        hccl_data = []
        hccl = HCCLExport(PARAMS)
        result = hccl._format_hccl_communication_data(hccl_data)
        self.assertEqual(len(result), 0)

    def test_get_hccl_arg_should_show_normal_size_when_size_valid(self):
        task = HcclTaskTuple(size=1024, notify_id=1, local_rank=0,
                            remote_rank=1, transport_type="RDMA", data_type="FP16",
                            link_type="PIX", bandwidth=100.0,
                            duration_estimated=10, stream_id=1,
                             task_id=1, task_type="HCCL")
        result = HCCLExport.get_hccl_arg(task)
        self.assertEqual(result['size(Byte)'], 1024)


if __name__ == '__main__':
    unittest.main()
