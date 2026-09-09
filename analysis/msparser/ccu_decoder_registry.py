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

from dataclasses import dataclass
from typing import Any
from typing import Dict
from typing import Iterable
from typing import Tuple
from typing import Type

from profiling_bean.hardware.ccu_bean import CCUChannelBean
from profiling_bean.hardware.ccu_bean import CCUMissionBean
from profiling_bean.hardware.ccu_profile import CcuDataKind
from profiling_bean.hardware.ccu_profile import CcuFormatProfile
from profiling_bean.hardware.ccu_profile import CcuHardwareProfile
from profiling_bean.hardware.ccu_profile import CcuHardwareProfileRegistry


@dataclass(frozen=True)
class CcuRecordDecoder:
    format_profile: CcuFormatProfile
    decoder_class: Type[Any]
    decoder_options: Tuple[Tuple[str, Any], ...] = ()

    @property
    def record_size(self) -> int:
        return self.format_profile.record_size

    def decode(self, binary_data: bytes) -> Any:
        if len(binary_data) != self.record_size:
            raise ValueError("Invalid CCU record size {}, expected {}.".format(len(binary_data), self.record_size))
        return self.decoder_class(**dict(self.decoder_options)).decode(binary_data)


class CcuDecoderRegistry:
    DECODER_CLASSES: Dict[str, Type[Any]] = {
        "ccu_v6_1_mission": CCUMissionBean,
        "ccu_v6_1_channel": CCUChannelBean,
    }

    @classmethod
    def get_decoder(
        cls, profile: CcuHardwareProfile, data_kind: CcuDataKind, format_version: int = None
    ) -> CcuRecordDecoder:
        format_profile = profile.get_format(data_kind, format_version)
        if format_profile is None:
            version_message = "default" if format_version is None else format_version
            raise ValueError(
                "CCU profile {} does not support {} format version {}".format(
                    profile.profile_id, data_kind.value, version_message
                )
            )
        decoder_class = cls.DECODER_CLASSES.get(format_profile.decoder_id)
        if decoder_class is None:
            raise ValueError("CCU decoder is not registered: {}".format(format_profile.decoder_id))
        decoder_options = ()
        if data_kind == CcuDataKind.MISSION:
            if not isinstance(profile, CcuHardwareProfile):
                raise ValueError("CCU mission decoder requires a hardware profile")
            decoder_options = (("relative_time_scale", profile.relative_time_scale),)
        return CcuRecordDecoder(format_profile, decoder_class, decoder_options)

    @classmethod
    def validate_registry(cls, profiles: Iterable[CcuHardwareProfile] = None) -> None:
        registered_profiles = CcuHardwareProfileRegistry.PROFILES if profiles is None else tuple(profiles)
        if any(not isinstance(profile, CcuHardwareProfile) for profile in registered_profiles):
            raise ValueError("Unsupported CCU format profile type")
        CcuHardwareProfileRegistry.validate(registered_profiles)
        for profile in registered_profiles:
            for _, format_profile in profile.formats:
                if format_profile.decoder_id not in cls.DECODER_CLASSES:
                    raise ValueError("CCU decoder is not registered: {}".format(format_profile.decoder_id))


CcuDecoderRegistry.validate_registry()
