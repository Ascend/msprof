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
import unittest
from unittest import mock

from profiling_bean.stars.fusion_task_bean import FusionTaskBean

NAMESPACE = 'profiling_bean.stars.fusion_task_bean'


class TestFusionTaskBean(unittest.TestCase):

    def test_init_basic_fields(self):
        """bit0=1 (cpu), magic=0x6BD3, task_id=100, syscnt=56, acc_id=5"""
        args = [2134, 0x6BD3, 100, 56, 1, 5, 0, 0, 0]
        with mock.patch(NAMESPACE + '.InfoConfReader') as mock_reader, \
                mock.patch(NAMESPACE + '.Utils.get_func_type', return_value='010110'):
            mock_reader.return_value.time_from_syscnt.return_value = 1120.0
            bean = FusionTaskBean(args)
            self.assertEqual(bean.func_type, '010110')
            self.assertEqual(bean.cnt, 1)
            self.assertEqual(bean.task_type, 2)
            self.assertEqual(bean.magic, 0x6BD3)
            self.assertEqual(bean.stream_id, 0)
            self.assertEqual(bean.task_id, 100)
            self.assertEqual(bean.sys_cnt, 1120.0)
            self.assertEqual(bean.acc_id, 5)

    def test_fusion_task_type_cpu(self):
        """bit0=1 → 'CPU'"""
        args = [0, 0x6BD3, 100, 56, 1, 0, 0, 0, 0]
        with mock.patch(NAMESPACE + '.InfoConfReader'), \
                mock.patch(NAMESPACE + '.Utils.get_func_type', return_value='010110'):
            bean = FusionTaskBean(args)
            self.assertEqual(bean.fusion_task_type, 'CPU')
            self.assertEqual(bean.ccu_die_id, None)

    def test_fusion_task_type_aicpu(self):
        """bit1=1 (value=2) → 'AICPU'"""
        args = [0, 0x6BD3, 100, 56, 2, 0, 0, 0, 0]
        with mock.patch(NAMESPACE + '.InfoConfReader'), \
                mock.patch(NAMESPACE + '.Utils.get_func_type', return_value='010110'):
            bean = FusionTaskBean(args)
            self.assertEqual(bean.fusion_task_type, 'AICPU')
            self.assertEqual(bean.ccu_die_id, None)

    def test_fusion_task_type_aicore(self):
        """bit2=1 (value=4) → 'AICORE'"""
        args = [0, 0x6BD3, 100, 56, 4, 0, 0, 0, 0]
        with mock.patch(NAMESPACE + '.InfoConfReader'), \
                mock.patch(NAMESPACE + '.Utils.get_func_type', return_value='010110'):
            bean = FusionTaskBean(args)
            self.assertEqual(bean.fusion_task_type, 'AICORE')
            self.assertEqual(bean.ccu_die_id, None)

    def test_fusion_task_type_ccu_die0(self):
        """bit3=1 (value=8) → 'CCU', die=0"""
        args = [0, 0x6BD3, 100, 56, 8, 0, 0, 0, 0]
        with mock.patch(NAMESPACE + '.InfoConfReader'), \
                mock.patch(NAMESPACE + '.Utils.get_func_type', return_value='010110'):
            bean = FusionTaskBean(args)
            self.assertEqual(bean.fusion_task_type, 'CCU')
            self.assertEqual(bean.ccu_die_id, 0)

    def test_fusion_task_type_ccu_die1(self):
        """bit4=1 (value=16) → 'CCU', die=1"""
        args = [0, 0x6BD3, 100, 56, 16, 0, 0, 0, 0]
        with mock.patch(NAMESPACE + '.InfoConfReader'), \
                mock.patch(NAMESPACE + '.Utils.get_func_type', return_value='010110'):
            bean = FusionTaskBean(args)
            self.assertEqual(bean.fusion_task_type, 'CCU')
            self.assertEqual(bean.ccu_die_id, 1)

    def test_fusion_task_type_unknown(self):
        """value=0 → 'unknown'"""
        args = [0, 0x6BD3, 100, 56, 0, 0, 0, 0, 0]
        with mock.patch(NAMESPACE + '.InfoConfReader'), \
                mock.patch(NAMESPACE + '.Utils.get_func_type', return_value='010110'):
            bean = FusionTaskBean(args)
            self.assertEqual(bean.fusion_task_type, 'UNKNOWN')
            self.assertEqual(bean.ccu_die_id, None)

    def test_mission_id_extraction(self):
        """mission_id from bits 8:5 of args[4] — e.g., (0x60 >> 5) & 0xF = 3"""
        args = [0, 0x6BD3, 100, 56, 0x61, 0, 0, 0, 0]
        with mock.patch(NAMESPACE + '.InfoConfReader'), \
                mock.patch(NAMESPACE + '.Utils.get_func_type', return_value='010110'):
            bean = FusionTaskBean(args)
            self.assertEqual(bean.fusion_task_type, 'CPU')      # bits[4:0]=1
            self.assertEqual(bean.mission_id, 3)                 # bits[8:5]=0x3

    def test_get_fusion_type_display(self):
        self.assertEqual(FusionTaskBean.get_fusion_type_display(1), ('CPU', None))
        self.assertEqual(FusionTaskBean.get_fusion_type_display(2), ('AICPU', None))
        self.assertEqual(FusionTaskBean.get_fusion_type_display(4), ('AICORE', None))
        self.assertEqual(FusionTaskBean.get_fusion_type_display(8), ('CCU', 0))
        self.assertEqual(FusionTaskBean.get_fusion_type_display(16), ('CCU', 1))
        self.assertEqual(FusionTaskBean.get_fusion_type_display(0), ('UNKNOWN', None))


if __name__ == '__main__':
    unittest.main()
