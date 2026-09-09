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

from common_func.cpp_enable_scene import DeviceParseScene, ExportTimelineScene, ExportSummaryScene, \
    CannCalculatorScene, ExportDBScene, CppEnableSceneConstant
from profiling_bean.prof_enum.chip_model import ChipModel


class TestCppEnableScene(unittest.TestCase):
    """
    Test cpp enable scene
    """

    def setUp(self):
        """
        Set up test environment
        """
        pass

    def tearDown(self):
        """
        Tear down test environment
        """
        pass

    def test_parse_scene_should_return_false_when_chip_v5_1_0(self):
        """
        Test DeviceParseScene should return false when chip is v5.1.0
        """
        with mock.patch('common_func.cpp_enable_scene.check_so_valid') as mock_check_so, \
                mock.patch('common_func.cpp_enable_scene.ChipManager') as mock_chip_manager:
            mock_check_so.return_value = True
            mock_chip_manager_instance = mock_chip_manager.return_value
            mock_chip_manager_instance.chip_id = ChipModel.CHIP_V5_1_0

            scene = DeviceParseScene()
            result = scene.is_cpp_enable()
            self.assertFalse(result)

    def test_parse_scene_should_return_false_when_chip_v6_1(self):
        """
        Test DeviceParseScene should return false when chip is v6.1
        """
        with mock.patch('common_func.cpp_enable_scene.check_so_valid') as mock_check_so, \
                mock.patch('common_func.cpp_enable_scene.ChipManager') as mock_chip_manager:
            mock_check_so.return_value = True
            mock_chip_manager_instance = mock_chip_manager.return_value
            mock_chip_manager_instance.chip_id = ChipModel.CHIP_V6_1_0

            scene = DeviceParseScene()
            result = scene.is_cpp_enable()
            self.assertFalse(result)

    def test_parse_scene_should_return_true_when_chip_v4(self):
        """
        Test DeviceParseScene should return true when chip is v4
        """
        with mock.patch('common_func.cpp_enable_scene.check_so_valid') as mock_check_so, \
                mock.patch('common_func.cpp_enable_scene.ChipManager') as mock_chip_manager:
            mock_check_so.return_value = True
            mock_chip_manager_instance = mock_chip_manager.return_value
            mock_chip_manager_instance.chip_id = ChipModel.CHIP_V4_1_0

            scene = DeviceParseScene()
            result = scene.is_cpp_enable()
            self.assertTrue(result)

    def test_parse_scene_should_return_false_when_chip_v6_2(self):
        with mock.patch('common_func.cpp_enable_scene.check_so_valid') as mock_check_so, \
                mock.patch('common_func.cpp_enable_scene.ChipManager') as mock_chip_manager:
            mock_check_so.return_value = True
            mock_chip_manager_instance = mock_chip_manager.return_value
            mock_chip_manager_instance.chip_id = ChipModel.CHIP_V6_2_0
            self.assertFalse(DeviceParseScene().is_cpp_enable())

    def test_export_timeline_scene_should_return_true_when_valid(self):
        """
        Test ExportTimelineScene should return true when valid
        """
        with mock.patch('common_func.cpp_enable_scene.check_so_valid') as mock_check_so, \
                mock.patch('common_func.cpp_enable_scene.ChipManager') as mock_chip_manager:
            mock_check_so.return_value = True
            mock_chip_manager_instance = mock_chip_manager.return_value
            mock_chip_manager_instance.chip_id = ChipModel.CHIP_V4_1_0

            scene = ExportTimelineScene(CppEnableSceneConstant.TIMELINE, False)
            result = scene.is_cpp_enable()
            self.assertTrue(result)

    def test_export_timeline_scene_should_return_false_when_wrong_command(self):
        """
        Test ExportTimelineScene should return false when command type is wrong
        """
        with mock.patch('common_func.cpp_enable_scene.check_so_valid') as mock_check_so, \
                mock.patch('common_func.cpp_enable_scene.ChipManager') as mock_chip_manager:
            mock_check_so.return_value = True
            mock_chip_manager_instance = mock_chip_manager.return_value
            mock_chip_manager_instance.chip_id = ChipModel.CHIP_V4_1_0

            scene = ExportTimelineScene(CppEnableSceneConstant.SUMMARY, False)
            result = scene.is_cpp_enable()
            self.assertFalse(result)

    def test_export_timeline_scene_should_return_false_when_sample_based(self):
        """
        Test ExportTimelineScene should return false when sample based
        """
        with mock.patch('common_func.cpp_enable_scene.check_so_valid') as mock_check_so, \
                mock.patch('common_func.cpp_enable_scene.ChipManager') as mock_chip_manager:
            mock_check_so.return_value = True
            mock_chip_manager_instance = mock_chip_manager.return_value
            mock_chip_manager_instance.chip_id = ChipModel.CHIP_V4_1_0

            scene = ExportTimelineScene(CppEnableSceneConstant.TIMELINE, True)
            result = scene.is_cpp_enable()
            self.assertFalse(result)

    def test_export_summary_scene_should_return_true_when_valid(self):
        """
        Test ExportSummaryScene should return true when valid
        """
        with mock.patch('common_func.cpp_enable_scene.check_so_valid') as mock_check_so, \
                mock.patch('common_func.cpp_enable_scene.ChipManager') as mock_chip_manager:
            mock_check_so.return_value = True
            mock_chip_manager_instance = mock_chip_manager.return_value
            mock_chip_manager_instance.chip_id = ChipModel.CHIP_V4_1_0

            scene = ExportSummaryScene(CppEnableSceneConstant.SUMMARY, True)
            result = scene.is_cpp_enable()
            self.assertTrue(result)

    def test_export_summary_scene_should_return_false_when_not_all_export(self):
        """
        Test ExportSummaryScene should return false when not all export
        """
        with mock.patch('common_func.cpp_enable_scene.check_so_valid') as mock_check_so, \
                mock.patch('common_func.cpp_enable_scene.ChipManager') as mock_chip_manager:
            mock_check_so.return_value = True
            mock_chip_manager_instance = mock_chip_manager.return_value
            mock_chip_manager_instance.chip_id = ChipModel.CHIP_V4_1_0

            scene = ExportSummaryScene(CppEnableSceneConstant.SUMMARY, False)
            result = scene.is_cpp_enable()
            self.assertFalse(result)

    def test_cann_calculator_scene_should_return_false_when_chip_v5_1_0(self):
        """
        Test CannCalculatorScene should return false when chip is v5.1.0
        """
        with mock.patch('common_func.cpp_enable_scene.check_so_valid') as mock_check_so, \
                mock.patch('common_func.cpp_enable_scene.ChipManager') as mock_chip_manager:
            mock_check_so.return_value = True
            mock_chip_manager_instance = mock_chip_manager.return_value
            mock_chip_manager_instance.chip_id = ChipModel.CHIP_V5_1_0

            scene = CannCalculatorScene()
            result = scene.is_cpp_enable()
            self.assertFalse(result)

    def test_cann_calculator_scene_should_return_true_when_chip_v4(self):
        """
        Test CannCalculatorScene should return true when chip is v4
        """
        with mock.patch('common_func.cpp_enable_scene.check_so_valid') as mock_check_so, \
                mock.patch('common_func.cpp_enable_scene.ChipManager') as mock_chip_manager:
            mock_check_so.return_value = True
            mock_chip_manager_instance = mock_chip_manager.return_value
            mock_chip_manager_instance.chip_id = ChipModel.CHIP_V4_1_0

            scene = CannCalculatorScene()
            result = scene.is_cpp_enable()
            self.assertTrue(result)

    def test_export_db_scene_should_return_false_when_chip_v5_1_0(self):
        """
        Test ExportDBScene should return false when chip is v5.1.0
        """
        with mock.patch('common_func.cpp_enable_scene.check_so_valid') as mock_check_so, \
                mock.patch('common_func.cpp_enable_scene.ChipManager') as mock_chip_manager:
            mock_check_so.return_value = True
            mock_chip_manager_instance = mock_chip_manager.return_value
            mock_chip_manager_instance.chip_id = ChipModel.CHIP_V5_1_0

            scene = ExportDBScene()
            result = scene.is_cpp_enable()
            self.assertFalse(result)

    def test_export_db_scene_should_return_true_when_chip_v4(self):
        """
        Test ExportDBScene should return true when chip is v4
        """
        with mock.patch('common_func.cpp_enable_scene.check_so_valid') as mock_check_so, \
                mock.patch('common_func.cpp_enable_scene.ChipManager') as mock_chip_manager:
            mock_check_so.return_value = True
            mock_chip_manager_instance = mock_chip_manager.return_value
            mock_chip_manager_instance.chip_id = ChipModel.CHIP_V4_1_0

            scene = ExportDBScene()
            result = scene.is_cpp_enable()
            self.assertTrue(result)

    def test_export_db_scene_should_return_true_when_chip_in_whitelist(self):
        """
        Test ExportDBScene should return true when chip is in whitelist
        """
        whitelist = [
            ChipModel.CHIP_V2_1_0,
            ChipModel.CHIP_V3_1_0,
            ChipModel.CHIP_V3_2_0,
            ChipModel.CHIP_V3_3_0,
            ChipModel.CHIP_V4_1_0,
            ChipModel.CHIP_V1_1_1,
            ChipModel.CHIP_V6_1_0,
            ChipModel.CHIP_V6_2_0
        ]
        with mock.patch('common_func.cpp_enable_scene.check_so_valid') as mock_check_so, \
                mock.patch('common_func.cpp_enable_scene.ChipManager') as mock_chip_manager:
            mock_check_so.return_value = True
            mock_chip_manager_instance = mock_chip_manager.return_value
            scene = ExportDBScene()

            for chip in whitelist:
                with self.subTest(chip=chip):
                    mock_chip_manager_instance.chip_id = chip
                    result = scene.is_cpp_enable()
                    self.assertTrue(result)

    def test_all_scenes_should_return_false_when_so_not_valid(self):
        """
        Test all scenes should return false when so file is not valid
        """
        with mock.patch('common_func.cpp_enable_scene.check_so_valid') as mock_check_so:
            mock_check_so.return_value = False

            scenes = [
                DeviceParseScene(),
                ExportTimelineScene(CppEnableSceneConstant.TIMELINE, False),
                ExportSummaryScene(CppEnableSceneConstant.SUMMARY, True),
                CannCalculatorScene(),
                ExportDBScene()
            ]

            for scene in scenes:
                result = scene.is_cpp_enable()
                self.assertFalse(result)


if __name__ == '__main__':
    unittest.main()
