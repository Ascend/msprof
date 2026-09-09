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

import struct
import unittest
from dataclasses import replace

from msparser.ccu_decoder_registry import CcuDecoderRegistry
from profiling_bean.hardware.ccu_profile import CCU_V6_1_PROFILE
from profiling_bean.hardware.ccu_profile import CcuDataKind
from profiling_bean.hardware.ccu_profile import CcuFormatProfile
from profiling_bean.prof_enum.chip_model import ChipModel


class TestCcuDecoderRegistry(unittest.TestCase):
    def test_v6_mission_decoder_should_preserve_existing_layout(self):
        decoder = CcuDecoderRegistry.get_decoder(CCU_V6_1_PROFILE, CcuDataKind.MISSION)
        binary_data = struct.pack(
            "=16HQHQQ3H", *(0,) * 16, 139937251735, 693, 139937251707, 139937231292, 691, 1, 4
        )

        mission = decoder.decode(binary_data)

        self.assertEqual(64, decoder.record_size)
        self.assertEqual(1, mission.stream_id)
        self.assertEqual(262145, mission.task_id)
        self.assertEqual(16, len(mission.rel_end_time))
        self.assertEqual((15, 139937251735), mission.rel_end_time[0])
        self.assertEqual((0, 139937251735), mission.rel_end_time[-1])

    def test_unknown_format_version_should_be_rejected(self):
        with self.assertRaises(ValueError):
            CcuDecoderRegistry.get_decoder(
                CCU_V6_1_PROFILE, CcuDataKind.CHANNEL, format_version=1
            )

    def test_explicit_format_version_should_select_registered_version(self):
        version_one = CcuFormatProfile(
            version=1,
            record_size=64,
            decoder_id="ccu_v6_1_mission",
            is_default=False,
            source_pattern=CCU_V6_1_PROFILE.get_format(CcuDataKind.MISSION).source_pattern,
        )
        profile = replace(
            CCU_V6_1_PROFILE,
            formats=CCU_V6_1_PROFILE.formats + ((CcuDataKind.MISSION, version_one),),
        )

        decoder = CcuDecoderRegistry.get_decoder(profile, CcuDataKind.MISSION, format_version=1)

        self.assertEqual(1, decoder.format_profile.version)

    def test_registry_validation_should_reject_missing_decoder(self):
        invalid_format = CcuFormatProfile(
            version=0,
            record_size=4,
            decoder_id="missing_decoder",
            source_pattern=CCU_V6_1_PROFILE.get_format(CcuDataKind.MISSION).source_pattern,
        )
        profile = replace(
            CCU_V6_1_PROFILE,
            profile_id="invalid_decoder_profile",
            supported_chip_models=(ChipModel.CHIP_V5_1_0,),
            formats=((CcuDataKind.MISSION, invalid_format),),
        )

        with self.assertRaises(ValueError):
            CcuDecoderRegistry.validate_registry((profile,))
