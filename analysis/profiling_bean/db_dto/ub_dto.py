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
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class UBDto(metaclass=InstanceCheckMeta):
    """
    Dto for UB data
    """

    device_id: int = None
    port_id: int = None
    time_stamp: float = None
    udma_rx_bind: float = None
    udma_tx_bind: float = None
    rx_port_band_width: float = None
    rx_packet_rate: float = None
    rx_bytes: float = None
    rx_packets: float = None
    rx_errors: float = None
    rx_dropped: float = None
    tx_port_band_width: float = None
    tx_packet_rate: float = None
    tx_bytes: float = None
    tx_packets: float = None
    tx_errors: float = None
    tx_dropped: float = None
