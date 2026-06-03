#!/usr/bin/python3
# -*- coding: utf-8 -*-
# -------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
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

import unittest
from unittest import mock

from constant.constant import CONFIG
from msparser.add_info.dpu_hccl_track_bean import DPUHcclTrackBean
from msparser.add_info.dpu_hccl_track_parser import DPUHcclInfoParser
from profiling_bean.prof_enum.data_tag import DataTag
from common_func.hash_dict_constant import HashDictData

NAMESPACE = 'msparser.add_info.dpu_hccl_track_parser'
MODEL_NAMESPACE = 'msmodel.dpu.dpu_task_model'
HCCL_COMMON_NAMESPACE = 'common_func.hccl_info_common'


class TestDPUHcclInfoParser(unittest.TestCase):
    file_list = {DataTag.DPU_HCCL_TRACK: ['aging.compact.dpu_hccl_info.slice_0',
                                         'unaging.compact.dpu_hccl_info.slice_0']}

    def test_parse_with_empty_files(self):
        """测试空文件列表"""
        check = DPUHcclInfoParser({DataTag.DPU_HCCL_TRACK: []}, CONFIG)
        check.parse()
        self.assertEqual(check._hccl_info_data, [])

    def test_parse_success(self):
        """测试解析成功"""
        with mock.patch(NAMESPACE + '.DPUHcclInfoParser.parse_bean_data', return_value=[]):
            check = DPUHcclInfoParser(self.file_list, CONFIG)
            check.parse()
            self.assertEqual(check._hccl_info_data, [])

    def test_save_with_empty_data(self):
        """测试保存空数据"""
        with mock.patch(MODEL_NAMESPACE + '.DPUTaskModel.__init__', return_value=None), \
                mock.patch(MODEL_NAMESPACE + '.DPUTaskModel.__enter__'), \
                mock.patch(MODEL_NAMESPACE + '.DPUTaskModel.__exit__', return_value=False), \
                mock.patch(MODEL_NAMESPACE + '.DPUTaskModel.flush') as mock_flush:
            check = DPUHcclInfoParser(self.file_list, CONFIG)
            check._hccl_info_data = []
            check.save()
            mock_flush.assert_not_called()

    def test_save_success(self):
        """测试保存成功"""
        with mock.patch(MODEL_NAMESPACE + '.DPUTaskModel.__init__', return_value=None), \
                mock.patch(MODEL_NAMESPACE + '.DPUTaskModel.__enter__'), \
                mock.patch(MODEL_NAMESPACE + '.DPUTaskModel.__exit__', return_value=False) as mock_flush:
            HashDictData._ge_hash_dict = {'123456789': 'dpu1', '987654321': 'dpu2'}
            check = DPUHcclInfoParser(self.file_list, CONFIG)
            bean = DPUHcclTrackBean([
                23130, 5500, 1000, 270722, 232, 75838889645892, 666666, 777777, 123456, 0, 15, 16, 0, 111111,
                75838889643892, 0, 444555, 777888, 1000, 10, 5000, 1, 1, 0, 4096, 888888, 0, 0, 0, 0, 0, 0])
            check._hccl_info_data = [bean]
            check.save()
            mock_flush.assert_called_once()

    def test_ms_run_with_no_files(self):
        """测试没有文件时直接返回"""
        with mock.patch(NAMESPACE + '.DPUHcclInfoParser.parse') as mock_parse, \
                mock.patch(NAMESPACE + '.DPUHcclInfoParser.save') as mock_save:
            check = DPUHcclInfoParser({DataTag.DPU_HCCL_TRACK: []}, CONFIG)
            check.ms_run()
            mock_parse.assert_not_called()
            mock_save.assert_not_called()

    def test_ms_run_success(self):
        """测试正常运行"""
        with mock.patch(NAMESPACE + '.DPUHcclInfoParser.parse'), \
                mock.patch(NAMESPACE + '.DPUHcclInfoParser.save'):
            check = DPUHcclInfoParser(self.file_list, CONFIG)
            check.ms_run()

    def test_ms_run_with_exception(self):
        """测试异常处理"""
        with mock.patch(NAMESPACE + '.DPUHcclInfoParser.parse', side_effect=OSError("test error")), \
                mock.patch(NAMESPACE + '.logging.error') as mock_error, \
                mock.patch(NAMESPACE + '.DPUHcclInfoParser.save') as mock_save:
            check = DPUHcclInfoParser(self.file_list, CONFIG)
            check.ms_run()
            mock_error.assert_called_once()
            mock_save.assert_not_called()

    def test_reformat_data_empty(self):
        """测试空数据重格式化"""
        check = DPUHcclInfoParser(self.file_list, CONFIG)
        check._hccl_info_data = []
        result = check.reformat_data()
        self.assertEqual(result, [])

    def test_reformat_data_success(self):
        """测试数据重格式化成功"""
        hash_dict_data = HashDictData("./")
        hash_dict_data._ge_hash_dict = {'123456789': 'dpu1', '987654321': 'dpu2', '666666': 'hccl_dpu'}
        check = DPUHcclInfoParser(self.file_list, CONFIG)

        bean = DPUHcclTrackBean([
                23130, 5500, 1000, 270722, 232, 75838889645892, 666666, 777777, 123456, 0, 15, 16, 0, 111111,
                75838889643892, 0, 444555, 777888, 1000, 10, 5000, 1, 1, 0, 4096, 0, 0, 0, 0, 0, 0, 0])
        check._hccl_info_data = [bean]

        result = check.reformat_data()
        expect_data = [[0, 0, 270722, 75838889643892, 75838889645892, "hccl_dpu", 'N/A', "123456", 0, 15, 16, 0.0,
                        "444555", "777888", 1000, 1, 10, 5000, 1, "SUM", "INT8", "ON_CHIP", "SDMA",
                        'RDMA_SEND_NOTIFY', 'DST', "777777", "111111", '0', '0']]
        self.assertEqual(len(result), 1)
        self.assertEqual(result, expect_data)

    def test_dpu_hccl_track_bean_properties(self):
        """测试 DPUHcclTrackBean 属性"""
        bean = DPUHcclTrackBean([
            23130, 5500, 1000, 270722, 232, 75838889645892, 666666, 777777, 123456, 0, 15, 16, 0, 111111,
            75838889643892, 0, 444555, 777888, 1000, 10, 5000, 1, 1, 0, 4096, 0, 0, 0, 0, 0, 0, 0])

        self.assertEqual(bean.item_id, '666666')
        self.assertEqual(bean.ccl_tag, '777777')
        self.assertEqual(bean.group_name, '123456')
        self.assertEqual(bean.local_rank, 0)
        self.assertEqual(bean.remote_rank, 15)
        self.assertEqual(bean.rank_size, 16)
        self.assertEqual(bean.stage, '0')
        self.assertEqual(bean.notify_id, '111111')
        self.assertEqual(bean.timestamp, 75838889645892)
        self.assertEqual(bean.start_time, 75838889643892)
        self.assertEqual(bean.duration_estimated, 0.0)
        self.assertEqual(bean.src_addr, '444555')
        self.assertEqual(bean.dst_addr, '777888')
        self.assertEqual(bean.data_size, 1000)
        self.assertEqual(bean.task_id, 10)
        self.assertEqual(bean.aicpu_task_id, 5000)
        self.assertEqual(bean.stream_id, 1)
        self.assertEqual(bean.plane_id, 1)
        self.assertEqual(bean.npu_device_id, 0)
        self.assertTrue(bean.is_dpu)  # dev_type = 1
        self.assertEqual(bean.dpu_device_id, 0)
        self.assertEqual(bean.op_type, '0')
        self.assertEqual(bean.data_type, '0')
        self.assertEqual(bean.link_type, '0')
        self.assertEqual(bean.transport_type, '0')
        self.assertEqual(bean.rdma_type, '0')
        self.assertEqual(bean.role, '0')
        self.assertEqual(bean.work_flow_mode, '0')
