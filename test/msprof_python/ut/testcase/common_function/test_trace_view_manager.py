#!/usr/bin/env python
# coding=utf-8
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

from common_func.trace_view_manager import TraceViewManager
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.info_conf_reader import InfoConfReader

NAMESPACE = 'common_func.trace_view_manager'


class TestTraceViewManager(unittest.TestCase):

    def setUp(self):
        """初始化测试环境"""
        self.PID_OFFSET = TraceViewManager.PID_OFFSET
        self.INDEX_OFFSET = TraceViewManager.INDEX_OFFSET
        self.HOST_ID_FOR_PID = TraceViewManager.HOST_ID_FOR_PID
        InfoConfReader()._info_json = {"pid": 111, "devices": "2"}

    def tearDown(self):
        InfoConfReader()._info_json.clear()

    def _create_layer_info(self, general_layer, sort_index=0):
        """创建 LayerInfo 测试对象"""
        return TraceViewHeaderConstant.LayerInfo(
            component_layer=TraceViewHeaderConstant.COMPONENT_LAYER_CANN,
            general_layer=general_layer,
            sort_index=sort_index
        )

    def test_get_format_pid_with_dpu_layer(self):
        """测试 DPU 层的 format_pid 计算"""
        layer_info = self._create_layer_info(
            general_layer=TraceViewHeaderConstant.GENERAL_LAYER_DPU,
            sort_index=5
        )

        # pid=1 会被用作 device_id，然后 pid 被重新赋值为 11
        result = TraceViewManager.get_format_pid(pid=1, layer_info=layer_info)

        # format_pid = (111 << 22) | (5 << 5) | 1
        expected = (111 << self.PID_OFFSET) | (5 << self.INDEX_OFFSET) | 1
        self.assertEqual(result, expected)

    def test_get_format_pid_with_cpu_layer(self):
        """测试 CPU 层的 format_pid 计算"""
        layer_info = self._create_layer_info(
            general_layer=TraceViewHeaderConstant.GENERAL_LAYER_CPU,
            sort_index=10
        )

        # CPU 层使用 HOST_ID_FOR_PID 作为 device_id
        result = TraceViewManager.get_format_pid(pid=200, layer_info=layer_info)

        # format_pid = (200 << 22) | (10 << 5) | HOST_ID_FOR_PID
        expected = (200 << self.PID_OFFSET) | (10 << self.INDEX_OFFSET) | self.HOST_ID_FOR_PID
        self.assertEqual(result, expected)

    def test_get_format_pid_with_invalid_device_id(self):
        """测试无效 device_id 的情况"""
        layer_info = self._create_layer_info(
            general_layer='OTHER_LAYER',  # 非 DPU、非 CPU
            sort_index=3
        )
        InfoConfReader()._info_json = {"pid": 111, "devices": "kk"}
        result = TraceViewManager.get_format_pid(pid=150, layer_info=layer_info)

        # 无效 device_id 时使用 HOST_ID_FOR_PID
        expected = (150 << self.PID_OFFSET) | (3 << self.INDEX_OFFSET) | self.HOST_ID_FOR_PID
        self.assertEqual(result, expected)

    def test_get_format_pid_with_valid_device_id(self):
        """测试有效 device_id 的情况"""
        layer_info = self._create_layer_info(
            general_layer='OTHER_LAYER',
            sort_index=7
        )

        result = TraceViewManager.get_format_pid(pid=300, layer_info=layer_info)

        # format_pid = (300 << 22) | (7 << 5) | 2
        expected = (300 << self.PID_OFFSET) | (7 << self.INDEX_OFFSET) | 2
        self.assertEqual(result, expected)

    def test_get_format_pid_with_zero_sort_index(self):
        """测试 sort_index 为 0 的情况"""
        layer_info = self._create_layer_info(
            general_layer='OTHER_LAYER',
            sort_index=0
        )

        result = TraceViewManager.get_format_pid(pid=50, layer_info=layer_info)

        expected = (50 << self.PID_OFFSET) | (0 << self.INDEX_OFFSET) | 2
        self.assertEqual(result, expected)

    def test_get_format_pid_bit_structure(self):
        """验证位结构正确性"""
        layer_info = self._create_layer_info(
            general_layer='OTHER_LAYER',
            sort_index=15
        )

        test_pid = 1000
        result = TraceViewManager.get_format_pid(pid=test_pid, layer_info=layer_info)

        # 验证各个字段的提取
        extracted_device_id = result & 0x1F  # 低 5 位
        extracted_sort_index = (result >> self.INDEX_OFFSET) & 0x1F  # 中间 5 位
        extracted_pid = result >> self.PID_OFFSET  # 高 22 位

        self.assertEqual(extracted_device_id, 2)
        self.assertEqual(extracted_sort_index, 15)
        self.assertEqual(extracted_pid, test_pid)

    def test_get_format_pid_dpu_layer_preserves_original_pid_as_device_id(self):
        """测试 DPU 层中原始 pid 被用作 device_id"""
        layer_info = self._create_layer_info(
            general_layer=TraceViewHeaderConstant.GENERAL_LAYER_DPU,
            sort_index=1
        )

        original_pid = 7
        result = TraceViewManager.get_format_pid(pid=original_pid, layer_info=layer_info)

        # 验证 device_id 是原始的 pid
        extracted_device_id = result & 0x1F
        self.assertEqual(extracted_device_id, original_pid)

        # 验证 pid 被替换为 get_json_pid_data 的返回值
        extracted_pid = result >> self.PID_OFFSET
        self.assertEqual(extracted_pid, 111)

    def test_get_format_pid_string_device_id_conversion(self):
        """测试字符串 device_id 转换为整数"""
        layer_info = self._create_layer_info(
            general_layer='OTHER_LAYER',
            sort_index=2
        )

        result = TraceViewManager.get_format_pid(pid=100, layer_info=layer_info)

        extracted_device_id = result & 0x1F
        self.assertEqual(extracted_device_id, 2)
        self.assertIsInstance(extracted_device_id, int)

