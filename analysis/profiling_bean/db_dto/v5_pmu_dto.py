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
from dataclasses import dataclass


@dataclass
class V5PmuDto:
    """
    Dto for v5 pmu data
    """

    stream_id: int = None
    task_id: int = None
    total_cycle: int = None
    block_num: int = None
    pmu0: float = None
    pmu1: float = None
    pmu2: float = None
    pmu3: float = None
    pmu4: float = None
    pmu5: float = None
    pmu6: float = None
    pmu7: float = None
    pmu8: float = None
    pmu9: float = None

    @property
    def pmu_list(self: any) -> any:
        return [
            self.pmu0,
            self.pmu1,
            self.pmu2,
            self.pmu3,
            self.pmu4,
            self.pmu5,
            self.pmu6,
            self.pmu7,
            self.pmu8,
            self.pmu9,
        ]
