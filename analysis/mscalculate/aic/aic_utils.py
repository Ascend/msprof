# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2025 Huawei Technologies Co.,Ltd.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

from common_func.msvp_common import read_cpu_cfg, MsvpCommonConst
from common_func.utils import Utils


class AicPmuUtils:
    """
    class used to help parse aic pmu
    """

    HEX = 16

    @staticmethod
    def get_pmu_event_name(pmu_event: str, cfg_name: str = MsvpCommonConst.AI_CORE) -> str:
        """
        return pmu event name by pmu_event
        :param pmu_event:
        :param cfg_name: decide use which config
        :return: pmu name
        """
        aicore_events_map = read_cpu_cfg(cfg_name, "event2metric")
        pmu_name = aicore_events_map.get(int(pmu_event, AicPmuUtils.HEX), "")
        if not pmu_name:
            pmu_name = str(pmu_event)
        return pmu_name

    @staticmethod
    def get_pmu_events(aic_pmu_events: str, cfg_name: str = MsvpCommonConst.AI_CORE) -> list:
        """
        get pmu events
        :param aic_pmu_events:
        :param cfg_name: decide use which config
        :return:
        """
        if not aic_pmu_events:
            return []
        return Utils.generator_to_list(
            AicPmuUtils.get_pmu_event_name(pmu_event, cfg_name) for pmu_event in aic_pmu_events.split(",")
        )

    @staticmethod
    def get_custom_pmu_events(aic_pmu_events: str) -> list:
        """
        get pmu events
        :param aic_pmu_events:
        :return:
        """
        events_list = aic_pmu_events.split(",")
        return list(map(lambda x: x.replace('0x', 'r'), events_list))

    @classmethod
    def remove_unused_column(cls: any, ai_core_profiling_events: list) -> list:
        """
        :param ai_core_profiling_events:
        :return:
        """
        remove_list = cls._remove_list(ai_core_profiling_events)
        result_list = list(filter(lambda events: events not in remove_list, ai_core_profiling_events))
        return result_list

    @classmethod
    def remove_redundant(cls: any, ai_core_profiling_events: dict) -> None:
        """
        :param ai_core_profiling_events:
        :return:
        """
        key_list = list(ai_core_profiling_events.keys())
        remove_list = cls._remove_list(key_list)
        for _key in key_list:
            if _key in remove_list:
                del ai_core_profiling_events[_key]

    @classmethod
    def _remove_list(cls: any, key_list: list) -> list:
        """
        :param key_list:
        :return:
        """
        remove_list = ["icache_req_ratio", "vec_fp16_128lane_ratio", "vec_fp16_64lane_ratio"]
        unused_list = [
            ["ub_read_bw_mte(GB/s)", "ub_write_bw_mte(GB/s)", "l2_write_bw(GB/s)", "main_mem_write_bw(GB/s)"],
            ["ub_read_bw_mte(GB/s)", "ub_write_bw_mte(GB/s)"],
            ["l2_read_bw(GB/s)", "l2_write_bw(GB/s)"],
        ]
        for _unused_list in unused_list:
            if set(key_list) >= set(_unused_list):
                remove_list.extend(_unused_list)
        return remove_list
