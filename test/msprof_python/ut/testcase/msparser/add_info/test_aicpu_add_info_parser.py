#!/usr/bin/env python
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
import os
import struct
import shutil
import unittest
from unittest import mock
 
from common_func.info_conf_reader import InfoConfReader
from msparser.add_info.aicpu_add_info_bean import AicpuAddInfoBean
from msparser.add_info.aicpu_add_info_parser import AicpuAddInfoParser
from msparser.data_struct_size_constant import StructFmt
from msparser.step_trace.ts_binary_data_reader.task_flip_bean import TaskFlip
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.add_info.aicpu_add_info_parser'


class TestAicpuAddInfoParser(unittest.TestCase):
    file_list = {
        DataTag.AICPU_ADD_INFO: [
            'aicpu.data.0.slice_0'
        ]
    }
    DIR_PATH = os.path.join(os.path.dirname(__file__), "aicpu_add_info")
    SQLITE_PATH = os.path.join(DIR_PATH, "sqlite")
    CONFIG = {
        'result_dir': DIR_PATH, 'device_id': '0', 'iter_id': IterationRange(0, 1, 1),
        'job_id': 'job_default', 'model_id': -1
    }

    def setUp(self) -> None:
        InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 100}], "devices": "0"}

    def setup_class(self):
        if not os.path.exists(self.DIR_PATH):
            os.mkdir(self.DIR_PATH)
        if not os.path.exists(self.SQLITE_PATH):
            os.mkdir(self.SQLITE_PATH)

    def teardown_class(self):
        if os.path.exists(self.DIR_PATH):
            shutil.rmtree(self.DIR_PATH)

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse'), \
                mock.patch(NAMESPACE + '.AicpuAddInfoParser.save'):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.ms_run()

    def test_save(self):
        check = AicpuAddInfoParser(self.file_list, self.CONFIG)
        check.save()
        check = AicpuAddInfoParser(self.file_list, self.CONFIG)
        check._ai_cpu_datas = [
            [0, '0', 1e-05, 2e-05, '', 0.0, 0.0, 1e-05, 0.0, 0.0]
        ]
        check.save()

    def test_parse_should_return_aicpu_node_data_when_type_0_and_start_time_not_0(self):
        aicpu_data = (
            23130, 6000, 0, 1, 128, 2000,
            1, 2, 0, 0, 1000, 10000, 10000, 15000,
            20000, 30000, 0, 0, 40000, 40000, 40000, 40000, 2, 123, 456, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        )
        struct_data = struct.pack(StructFmt.AI_CPU_NODE_ADD_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.AICPU_NODE, [])
        self.assertEqual(1, len(data))
        self.assertEqual(0.123, data[0].data.dispatch_time)
        InfoConfReader()._info_json = {}

    def test_parse_should_return_aicpu_node_data_when_type_0_and_start_time_0(self):
        aicpu_data = (
            23130, 6000, 0, 1, 128, 2000,
            1, 2, 0, 0, 0, 10000, 10000, 15000,
            20000, 30000, 0, 0, 40000, 40000, 40000, 40000, 2, 123, 456, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        )
        struct_data = struct.pack(StructFmt.AI_CPU_NODE_ADD_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.AICPU_NODE, [])
        self.assertEqual(0, len(data))
        InfoConfReader()._info_json = {}

    def test_parse_should_return_aicpu_dp_data_when_type_1(self):
        aicpu_data = [23130, 6000, 1, 1, 128, 2000,
                      b"Last dequeue", b"mark_name_VZDO11n4aPy", 1, 56] + [0] * 17
        struct_data = struct.pack(StructFmt.AI_CPU_DP_ADD_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.AICPU_DP, [])
        self.assertEqual(1, len(data))
        self.assertEqual("Last dequeue", data[0].data.action)

    def test_parse_should_return_aicpu_model_data_when_type_2(self):
        aicpu_data = [23130, 6000, 2, 1, 128, 2000,
                      0, 1, 2, 0, 4] + [0] * 26
        struct_data = struct.pack(StructFmt.AI_CPU_MODEL_ADD_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.AICPU_MODEL, [])
        self.assertEqual(1, len(data))
        self.assertEqual(2, data[0].data.tag_id)

    def test_parse_should_return_aicpu_mi_data_when_type_3(self):
        aicpu_data = [23130, 6000, 3, 1, 128, 2000,
                      1, 0, 10, 1000, 2000] + [0] * 25
        struct_data = struct.pack(StructFmt.AI_CPU_MI_ADD_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.AICPU_MI, [])
        self.assertEqual(1, len(data))
        self.assertEqual(2000, data[0].data.end_time)

    def test_parse_should_return_comm_turn_data_when_type_4(self):
        aicpu_data = [23130, 6000, 4, 1, 128, 20000,
                      1000, 2000, 3000, 4000, 5000, 6000, 7000, 128, 0, 1, 2, 0, 2, 0] + [0] * 43
        struct_data = struct.pack(StructFmt.KFC_COMM_TURN_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.KFC_COMM_TURN, [])
        self.assertEqual(1, len(data))
        self.assertEqual(1000, data[0].data.server_start_time)

    def test_parse_should_return_compute_turn_data_when_type_5(self):
        aicpu_data = [23130, 6000, 5, 1, 128, 20000,
                      1000, 2000, 3000, 128, 0, 1, 2, 0, 2, 0] + [0] * 51
        struct_data = struct.pack(StructFmt.KFC_COMPUTE_TURN_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.KFC_COMPUTE_TURN, [])
        self.assertEqual(1, len(data))
        self.assertEqual(1000, data[0].data.wait_compute_start_time)

    def test_parse_should_return_kfc_hccl_info_data_when_type_13(self):
        aicpu_data = [23130, 6000, 13, 1, 128, 20000] + \
                     [12345, 0, 123, 0, 8, 8, 0, 4294967295, 255143588, 0.1, 0, 0, 8, 2, 0, 1, 1, 1, 1, 1, 1, 1, 1,
                      255, 0, 0, 0, 0, 0, 0, 0, 0, 0] + \
                     [12345, 0, 0, 0, 8, 8, 0, 4294967295, 255144588, 0.1, 0, 0, 8, 3, 0, 1, 1, 1, 1, 1, 1, 1, 1,
                      255, 0, 0, 0, 0, 0, 0, 0, 0, 0]
        struct_data = struct.pack(StructFmt.BYTE_ORDER_CHAR + StructFmt.KFC_HCCL_INFO_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.KFC_HCCL_INFO, [])
        self.assertEqual(1, len(data))  # 拆分成了2条数据,但是第二条因为groupname是"0",被过滤
        self.assertEqual("1", data[0].op_type)  # op type
        self.assertEqual("1", data[0].rdma_type)  # rdma type
        InfoConfReader()._info_json = {}

    def test_parse_should_return_kfc_hccl_info_data_when_type_13_and_data_more_than_int64max(self):
        aicpu_data = [23130, 6000, 13, 1, 128, 20000] + \
                     [12345, 0, 123, 0, 8, 8, 0, 4294967295, 255143588, 0.1, 0, 0, 18446744073709551111,
                      2, 0, 1, 1, 1, 1, 1, 1, 1, 1, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0] + \
                     [12345, 0, 444444, 0, 8, 8, 0, 4294967295, 255144588, 0.1, 0, 0, 18446744073709551111,
                      3, 0, 1, 1, 1, 1, 1, 1, 1, 1, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0]
        struct_data = struct.pack(StructFmt.BYTE_ORDER_CHAR + StructFmt.KFC_HCCL_INFO_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.KFC_HCCL_INFO, [])
        self.assertEqual(2, len(data))
        self.assertEqual((2 ** 63 -1), data[0].data_size)  # data_size被重置为2 ** 63 -1
        InfoConfReader()._info_json = {}

    def test_parse_should_return_device_hccl_op_info_data_when_type_10(self):
        aicpu_data = [23130, 6000, 10, 1, 128, 20000,
                      0, 0, 0, 1, 12345, 8, 6, 1] + [0] * 196
        struct_data = struct.pack(StructFmt.DEVICE_HCCL_OP_INFO_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.HCCL_OP_INFO, [])
        self.assertEqual(1, len(data))
        self.assertEqual("0", data[0].data.data_type)  # data type
        InfoConfReader()._info_json = {}


def _pack_aicpu_bean(struct_type, fmt, inner_fields, timestamp=0):
    """构造 aicpu bean 二进制数据：
       header: magic(23130) level(6000) struct_type thread_id(1) data_len(128) timestamp
       inner_fields 填在 header 之后，不足的字段用 0 补齐"""
    import re
    # 计算 fmt 中的总字段数（单个字母 = 1 字段，N+字母 = N 字段）
    total_fields = sum(int(m.group(1) or 1) for m in re.finditer(r'(\d*)([HIQBd])', fmt))
    header = [23130, 6000, struct_type, 1, 128, timestamp]
    full = header + list(inner_fields)
    if len(full) < total_fields:
        full += [0] * (total_fields - len(full))
    return struct.pack(fmt, *full)


def _make_flip_bean(timestamp, stream_id, task_id, flip_num):
    """通过二进制构造 AICPU_FLIP_TASK (type=11) 真 bean"""
    inner = [stream_id, task_id, flip_num]  # data[6]=streamId, data[7]=taskId, data[8]=flipNum
    raw = _pack_aicpu_bean(11, StructFmt.AICPU_FLIP_TASK_FMT, inner, timestamp=timestamp)
    return AicpuAddInfoBean.decode(raw)


def _make_main_stream_bean(timestamp, aicpu_stream_id, aicpu_task_id, stream_id, task_id, task_type):
    """通过二进制构造 AICPU_MASTER_STREAM_HCCL_TASK (type=12) 真 bean"""
    inner = [aicpu_stream_id, aicpu_task_id, stream_id, task_id, task_type]
    raw = _pack_aicpu_bean(12, StructFmt.AICPU_MASTER_STREAM_HCCL_TASK_FMT, inner, timestamp=timestamp)
    return AicpuAddInfoBean.decode(raw)


def _make_kfc_bean(timestamp, stream_id, task_id):
    """通过二进制构造 KFC_HCCL_INFO (type=13) 真 bean，走 _pre_process_kfc_info 得到 KfcHcclInfoBean"""
    inner = (
        # first info (33 fields): 3Q4I2Qd3Q2I2H16B
        # KfcHcclInfoBean: data[0]=item_id(3Q[0]), data[1]=ccl_tag(3Q[1]), data[2]=group_name(3Q[2]),
        # data[3..6]=4I, data[7]=notify_id(2Q[0]), data[8]=timestamp(2Q[1])
        0, 0, 1,      # 3Q: item_id=0, ccl_tag=0, group_name=1
        0, 0, 0, 0,   # 4I: local_rank, remote_rank, rank_size, stage
        0, timestamp, # 2Q: notify_id=0, timestamp
        0.0,          # d: duration_estimated
        0, 0, 0,      # 3Q: src_addr, dst_addr, data_size
        task_id, 0,    # 2I: data[13]=task_id(用于set_task_id), data[14]=unused
        stream_id, 0,  # 2H: data[15]=stream_id(用于set_stream_id), data[16]=plane_id
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  # 16B
        # second info (33 fields): group_name=0 会被 _pre_process_kfc_info 过滤
        0, 0, 0,
        0, 0, 0, 0,
        0, 0,
        0.0,
        0, 0, 0,
        0, 0,
        0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    )
    raw = _pack_aicpu_bean(13, StructFmt.KFC_HCCL_INFO_FMT, inner, timestamp=timestamp)
    return AicpuAddInfoBean.decode(raw)


class TestAicpuComputeBatchId(unittest.TestCase):
    """测试 _compute_batch_id 逻辑"""

    FILE_LIST = {DataTag.AICPU_ADD_INFO: []}
    CONFIG = {'result_dir': '/tmp', 'device_id': '0'}

    def setUp(self) -> None:
        # get_freq 内部会 hwts_frequency * 1e6，设 1000 使 time_from_syscnt 近似恒等，
        # 保证测试中 flip 和 task 的 timestamp 处于同一量级
        InfoConfReader()._info_json = {
            "DeviceInfo": [{'hwts_frequency': 1000}], "devices": "0"}

    def _make_parser(self):
        return AicpuAddInfoParser(self.FILE_LIST, self.CONFIG)

    def test_compute_batch_id_should_return_when_flip_data_empty(self):
        parser = self._make_parser()
        parser._aicpu_data = {
            AicpuAddInfoBean.AICPU_FLIP_TASK: [],
            AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK: [
                _make_main_stream_bean(100, 0, 0, 1, 100, 0),
            ],
            AicpuAddInfoBean.KFC_HCCL_INFO: [],
        }
        parser._compute_batch_id()
        ms = parser._aicpu_data[AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK][0]
        self.assertEqual(0, ms.data.batch_id)

    def test_compute_batch_id_should_set_batch_id_on_kfc_info(self):
        parser = self._make_parser()
        parser._aicpu_data[AicpuAddInfoBean.AICPU_FLIP_TASK] = [
            _make_flip_bean(200, 1, 0, 0),
        ]
        parser._aicpu_data[AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK] = []
        kfc_bean = _make_kfc_bean(100, 1, 10)
        parser._aicpu_data[AicpuAddInfoBean.KFC_HCCL_INFO] = \
            parser._pre_process_kfc_info(kfc_bean)
        parser._compute_batch_id()
        kfc = parser._aicpu_data[AicpuAddInfoBean.KFC_HCCL_INFO][0]
        self.assertEqual(0, kfc.batch_id)

    def test_compute_batch_id_should_set_batch_id_on_main_stream(self):
        parser = self._make_parser()
        parser._aicpu_data = {
            AicpuAddInfoBean.AICPU_FLIP_TASK: [
                _make_flip_bean(200, 1, 0, 0),
            ],
            AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK: [
                _make_main_stream_bean(100, 0, 0, 1, 100, 0),
            ],
            AicpuAddInfoBean.KFC_HCCL_INFO: [],
        }
        parser._compute_batch_id()
        ms = parser._aicpu_data[AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK][0]
        self.assertEqual(0, ms.data.batch_id)

    def test_compute_batch_id_should_use_sequential_batch_id(self):
        parser = self._make_parser()
        parser._aicpu_data = {
            AicpuAddInfoBean.AICPU_FLIP_TASK: [
                _make_flip_bean(100, 1, 0, 0),
                _make_flip_bean(300, 1, 0, 5),
            ],
            AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK: [
                _make_main_stream_bean(50, 0, 0, 1, 10, 0),
                _make_main_stream_bean(200, 0, 0, 1, 20, 0),
                _make_main_stream_bean(400, 0, 0, 1, 30, 0),
            ],
            AicpuAddInfoBean.KFC_HCCL_INFO: [],
        }
        parser._compute_batch_id()
        tasks = parser._aicpu_data[AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK]
        self.assertEqual(0, tasks[0].data.batch_id)
        self.assertEqual(1, tasks[1].data.batch_id)
        self.assertEqual(2, tasks[2].data.batch_id)

    def test_compute_batch_id_should_skip_stream_without_flip(self):
        parser = self._make_parser()
        parser._aicpu_data = {
            AicpuAddInfoBean.AICPU_FLIP_TASK: [
                _make_flip_bean(100, 1, 0, 0),
            ],
            AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK: [
                _make_main_stream_bean(50, 0, 0, 2, 10, 0),
            ],
            AicpuAddInfoBean.KFC_HCCL_INFO: [],
        }
        parser._compute_batch_id()
        ms = parser._aicpu_data[AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK][0]
        self.assertEqual(0, ms.data.batch_id)

    def test_compute_batch_id_should_work_with_both_kfc_and_main_stream(self):
        parser = self._make_parser()
        parser._aicpu_data[AicpuAddInfoBean.AICPU_FLIP_TASK] = [
            _make_flip_bean(200, 1, 0, 1),
        ]
        parser._aicpu_data[AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK] = [
            _make_main_stream_bean(100, 0, 0, 1, 10, 0),
        ]
        kfc_bean = _make_kfc_bean(300, 1, 20)
        parser._aicpu_data[AicpuAddInfoBean.KFC_HCCL_INFO] = \
            parser._pre_process_kfc_info(kfc_bean)
        parser._compute_batch_id()
        ms = parser._aicpu_data[AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK][0]
        self.assertEqual(0, ms.data.batch_id)
        kfc = parser._aicpu_data[AicpuAddInfoBean.KFC_HCCL_INFO][0]
        self.assertEqual(1, kfc.batch_id)

    def test_compute_batch_id_should_be_independent_per_stream(self):
        parser = self._make_parser()
        parser._aicpu_data = {
            AicpuAddInfoBean.AICPU_FLIP_TASK: [
                _make_flip_bean(100, 1, 0, 1),
                _make_flip_bean(300, 2, 0, 5),
            ],
            AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK: [
                _make_main_stream_bean(50, 0, 0, 1, 10, 0),
                _make_main_stream_bean(150, 0, 0, 1, 11, 0),
                _make_main_stream_bean(200, 0, 0, 2, 20, 0),
                _make_main_stream_bean(500, 0, 0, 2, 21, 0),
            ],
            AicpuAddInfoBean.KFC_HCCL_INFO: [],
        }
        parser._compute_batch_id()
        tasks = parser._aicpu_data[AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK]
        self.assertEqual(0, tasks[0].data.batch_id)
        self.assertEqual(1, tasks[1].data.batch_id)
        self.assertEqual(0, tasks[2].data.batch_id)
        self.assertEqual(1, tasks[3].data.batch_id)


class TestAicpuSetAicpuData(unittest.TestCase):
    """测试 set_aicpu_data 中的零时间戳过滤，复用已有 struct 格式构造真 bean"""

    FILE_LIST = {DataTag.AICPU_ADD_INFO: []}
    CONFIG = {'result_dir': '/tmp', 'device_id': '0'}

    def _make_node_bean(self, start_time, end_time):
        """用 AICPU_NODE (type=0) 格式构造真 bean，start_time/end_time 填在 data[10]/data[15] 位置"""
        inner = [0] * 36  # AI_CPU_NODE_ADD_FMT(42) - header(6) = 36
        inner[4] = start_time   # data[10] = ai_cpu_task_start_time
        inner[9] = end_time     # data[15] = ai_cpu_task_end_time
        raw = _pack_aicpu_bean(0, StructFmt.AI_CPU_NODE_ADD_FMT, inner)
        return AicpuAddInfoBean.decode(raw)

    def test_set_aicpu_data_should_skip_node_with_zero_start_time(self):
        parser = AicpuAddInfoParser(self.FILE_LIST, self.CONFIG)
        parser._aicpu_data = {k: [] for k in parser._aicpu_data}
        parser.set_aicpu_data([self._make_node_bean(start_time=0, end_time=100)])
        self.assertEqual(0, len(parser._aicpu_data[AicpuAddInfoBean.AICPU_NODE]))

    def test_set_aicpu_data_should_skip_node_with_zero_end_time(self):
        parser = AicpuAddInfoParser(self.FILE_LIST, self.CONFIG)
        parser._aicpu_data = {k: [] for k in parser._aicpu_data}
        parser.set_aicpu_data([self._make_node_bean(start_time=100, end_time=0)])
        self.assertEqual(0, len(parser._aicpu_data[AicpuAddInfoBean.AICPU_NODE]))

    def test_set_aicpu_data_should_keep_node_with_nonzero_times(self):
        parser = AicpuAddInfoParser(self.FILE_LIST, self.CONFIG)
        parser._aicpu_data = {k: [] for k in parser._aicpu_data}
        parser.set_aicpu_data([self._make_node_bean(start_time=100, end_time=200)])
        self.assertEqual(1, len(parser._aicpu_data[AicpuAddInfoBean.AICPU_NODE]))


    def test_kfc_batch_id_should_be_set_after_compute(self):
        """kfc_info 对象上 batch_id 应在 _compute_batch_id 后被正确设置"""
        parser = AicpuAddInfoParser(self.FILE_LIST, self.CONFIG)
        parser._aicpu_data = {k: [] for k in parser._aicpu_data}
        parser._aicpu_data[AicpuAddInfoBean.AICPU_FLIP_TASK] = [
            _make_flip_bean(200, 1, 0, 5),
        ]
        kfc_raw = _make_kfc_bean(300, 1, 20)
        parser._aicpu_data[AicpuAddInfoBean.KFC_HCCL_INFO] = \
            parser._pre_process_kfc_info(kfc_raw)

        # 检查 compute 前 batch_id 初始值
        kfc_before = parser._aicpu_data[AicpuAddInfoBean.KFC_HCCL_INFO][0]
        self.assertEqual(0, kfc_before.batch_id)

        parser._compute_batch_id()

        # 检查 compute 后 batch_id 应被刷新
        kfc_after = parser._aicpu_data[AicpuAddInfoBean.KFC_HCCL_INFO][0]
        self.assertEqual(1, kfc_after.batch_id,
                         "kfc@300 >= flip@200(flip_num=5) → batch_id should be 5")

    def test_main_stream_batch_id_should_be_set_after_compute(self):
        """mainStream data 上 batch_id 应在 _compute_batch_id 后被正确设置"""
        parser = AicpuAddInfoParser(self.FILE_LIST, self.CONFIG)
        parser._aicpu_data = {k: [] for k in parser._aicpu_data}
        parser._aicpu_data[AicpuAddInfoBean.AICPU_FLIP_TASK] = [
            _make_flip_bean(200, 1, 0, 5),
        ]
        parser._aicpu_data[AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK] = [
            _make_main_stream_bean(300, 0, 0, 1, 10, 0),
        ]

        # 检查 compute 前 batch_id 初始值
        ms_before = parser._aicpu_data[AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK][0]
        self.assertEqual(0, ms_before.data.batch_id)

        parser._compute_batch_id()

        # 检查 compute 后 batch_id 应被刷新
        ms_after = parser._aicpu_data[AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK][0]
        self.assertEqual(1, ms_after.data.batch_id,
                         "mainStream@300 >= flip@200(flip_num=5) → batch_id should be 5")

    def test_kfc_batch_id_should_be_zero_before_flip(self):
        """kfc task timestamp 小于 flip timestamp 时 batch_id 应为 0"""
        parser = AicpuAddInfoParser(self.FILE_LIST, self.CONFIG)
        parser._aicpu_data = {k: [] for k in parser._aicpu_data}
        parser._aicpu_data[AicpuAddInfoBean.AICPU_FLIP_TASK] = [
            _make_flip_bean(200, 1, 0, 5),
        ]
        kfc_raw = _make_kfc_bean(100, 1, 10)
        parser._aicpu_data[AicpuAddInfoBean.KFC_HCCL_INFO] = \
            parser._pre_process_kfc_info(kfc_raw)

        parser._compute_batch_id()

        kfc = parser._aicpu_data[AicpuAddInfoBean.KFC_HCCL_INFO][0]
        self.assertEqual(0, kfc.batch_id,
                         "kfc@100 < flip@200 → batch_id should be 0")


if __name__ == '__main__':
    unittest.main()
