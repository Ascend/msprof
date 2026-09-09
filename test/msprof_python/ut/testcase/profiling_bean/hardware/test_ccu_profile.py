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
from dataclasses import replace

from common_func.file_name_manager import FileNameManagerConstant
from common_func.file_name_manager import get_ccu_channel_compiles
from common_func.file_name_manager import get_ccu_mission_compiles
from msconfig.data_parsers_config import DataParsersConfig
from profiling_bean.hardware.ccu_profile import CCU_V6_1_PROFILE
from profiling_bean.hardware.ccu_profile import CcuDataKind
from profiling_bean.hardware.ccu_profile import CcuFormatProfile
from profiling_bean.hardware.ccu_profile import CcuHardwareProfileRegistry
from profiling_bean.hardware.ccu_profile import CcuMetricKind
from profiling_bean.hardware.ccu_profile import CcuMetricProfile
from profiling_bean.hardware.ccu_profile import CcuMetricUnit
from profiling_bean.prof_enum.chip_model import ChipModel


class TestCcuHardwareProfile(unittest.TestCase):
    def test_v6_1_chip_model_should_resolve_to_implemented_profile(self):
        profile = CcuHardwareProfileRegistry.require_profile(ChipModel.CHIP_V6_1_0)

        self.assertIs(profile, CCU_V6_1_PROFILE)
        self.assertEqual("ccu_v6_1", profile.profile_id)
        self.assertEqual(128, profile.channel_count)
        self.assertTrue(profile.supports_metric_kind(CcuMetricKind.DELAY))
        self.assertEqual(CcuMetricKind.DELAY, profile.get_metric("channel_delay").kind)

    def test_v6_2_should_remain_unregistered_until_its_decoder_lands(self):
        self.assertIsNone(CcuHardwareProfileRegistry.get_profile(ChipModel.CHIP_V6_2_0))
        with self.assertRaises(ValueError):
            CcuHardwareProfileRegistry.require_profile(ChipModel.CHIP_V6_2_0)

    def test_numeric_chip_model_should_resolve_without_global_state(self):
        self.assertIs(CCU_V6_1_PROFILE, CcuHardwareProfileRegistry.require_profile("15"))
        with self.assertRaises(ValueError):
            CcuHardwareProfileRegistry.require_profile(16)

    def test_unknown_chip_model_should_be_rejected(self):
        with self.assertRaises(ValueError):
            CcuHardwareProfileRegistry.require_profile(ChipModel.CHIP_V5_1_0)

    def test_v6_source_scope_should_validate_die_id(self):
        self.assertEqual((0, 0), CCU_V6_1_PROFILE.get_source_scope(
            CcuDataKind.MISSION, "ccu0.instr.0.slice_0"
        ))
        self.assertEqual((1, 7), CCU_V6_1_PROFILE.get_source_scope(
            CcuDataKind.CHANNEL, "ccu1.stat.7.slice_0"
        ))
        with self.assertRaises(ValueError):
            CCU_V6_1_PROFILE.get_source_scope(CcuDataKind.MISSION, "ccu2.instr.0.slice_0")

    def test_file_name_patterns_should_remain_unchanged(self):
        self.assertEqual(
            r"^ccu(0|1)\.instr\.(\d+)\.slice_\d+", FileNameManagerConstant.CCU_MISSION_PATTERN
        )
        self.assertEqual(
            r"^ccu(0|1)\.stat\.(\d+)\.slice_\d+", FileNameManagerConstant.CCU_CHANNEL_PATTERN
        )
        self.assertIsNotNone(get_ccu_mission_compiles()[0].match("ccu0.instr.0.slice_0"))
        self.assertIsNotNone(get_ccu_channel_compiles()[0].match("ccu1.stat.0.slice_0"))
        self.assertIsNone(get_ccu_mission_compiles()[0].match("ccu2.instr.0.slice_0"))
        self.assertIsNone(get_ccu_channel_compiles()[0].match("ccu2.stat.0.slice_0"))

    def test_parser_config_should_use_profile_supported_chips(self):
        supported_chips = CcuHardwareProfileRegistry.supported_chip_models_csv()
        self.assertEqual("15", supported_chips)
        for parser_name in ("CCUMissionParser", "CCUChannelParser"):
            parser_config = dict(DataParsersConfig.DATA[parser_name])
            self.assertEqual(supported_chips, parser_config["chip_model"])
        host_parser_config = dict(DataParsersConfig.DATA["CCUAddInfoParser"])
        self.assertEqual("15,16", host_parser_config["chip_model"])

    def test_profile_should_support_multiple_versions_for_one_data_kind(self):
        version_one = CcuFormatProfile(
            version=1,
            record_size=128,
            decoder_id="ccu_v6_1_mission",
            is_default=False,
            source_pattern=CCU_V6_1_PROFILE.get_format(CcuDataKind.MISSION).source_pattern,
        )
        profile = replace(
            CCU_V6_1_PROFILE,
            formats=CCU_V6_1_PROFILE.formats + ((CcuDataKind.MISSION, version_one),),
        )

        CcuHardwareProfileRegistry.validate((profile,))

        self.assertEqual(0, profile.get_format(CcuDataKind.MISSION).version)
        self.assertEqual(1, profile.get_format(CcuDataKind.MISSION, 1).version)

    def test_profile_should_support_multiple_metric_capabilities(self):
        bandwidth_metric = CcuMetricProfile(
            metric_id="channel_bandwidth",
            data_kind=CcuDataKind.CHANNEL,
            kind=CcuMetricKind.BANDWIDTH,
            unit=CcuMetricUnit.BYTE_PER_SECOND,
        )
        profile = replace(CCU_V6_1_PROFILE, metrics=CCU_V6_1_PROFILE.metrics + (bandwidth_metric,))

        CcuHardwareProfileRegistry.validate((profile,))

        self.assertTrue(profile.supports_metric_kind(CcuMetricKind.DELAY))
        self.assertTrue(profile.supports_metric_kind(CcuMetricKind.BANDWIDTH))

    def test_metric_should_refer_to_a_supported_data_kind(self):
        profile = replace(
            CCU_V6_1_PROFILE,
            formats=((
                CcuDataKind.MISSION,
                CCU_V6_1_PROFILE.get_format(CcuDataKind.MISSION),
            ),),
        )

        with self.assertRaises(ValueError):
            CcuHardwareProfileRegistry.validate((profile,))

    def test_profile_source_pattern_should_capture_die_and_source_ids(self):
        invalid_mission_format = replace(
            CCU_V6_1_PROFILE.get_format(CcuDataKind.MISSION),
            source_pattern=r"^ccu[01]\.instr\.\d+\.slice_\d+",
        )
        profile = replace(
            CCU_V6_1_PROFILE,
            formats=(
                (CcuDataKind.MISSION, invalid_mission_format),
                (CcuDataKind.CHANNEL, CCU_V6_1_PROFILE.get_format(CcuDataKind.CHANNEL)),
            ),
        )

        with self.assertRaises(ValueError):
            CcuHardwareProfileRegistry.validate((profile,))

    def test_profile_validation_should_reject_two_default_versions(self):
        second_default = CcuFormatProfile(
            version=1,
            record_size=64,
            decoder_id="ccu_v6_1_mission",
            source_pattern=CCU_V6_1_PROFILE.get_format(CcuDataKind.MISSION).source_pattern,
        )
        profile = replace(
            CCU_V6_1_PROFILE,
            formats=CCU_V6_1_PROFILE.formats + ((CcuDataKind.MISSION, second_default),),
        )

        with self.assertRaises(ValueError):
            CcuHardwareProfileRegistry.validate((profile,))
