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

import logging
import os
from collections import namedtuple
from decimal import Decimal

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import is_number
from common_func.msvp_constant import MsvpConstant
from common_func.utils import Utils
from common_func.file_manager import FileOpen
from host_prof.host_network_usage.model.host_network_usage import HostNetworkUsage
from host_prof.host_prof_base.host_prof_presenter_base import HostProfPresenterBase


def parse_net_stats(line: str) -> any:
    """
    parse net status by line
    """
    NetStats = namedtuple('netstats', ['intf', 'rxbytes', 'rxpackets', 'rxerrs', 'txbytes', 'txpackets', 'txerrs'])
    try:
        intf, rxbytes, rxpackets, rxerrs, txbytes, txpackets, txerrs = init_params(line)
        return NetStats(intf, rxbytes, rxpackets, rxerrs, txbytes, txpackets, txerrs)
    except (TypeError, ValueError, RuntimeError) as parse_file_except:
        logging.error("Error in parsing network data:%s", str(parse_file_except), exc_info=Constant.TRACE_BACK_SWITCH)
        return NetStats(0, 0, 0, 0, 0, 0, 0)
    finally:
        pass


def init_params(line: str) -> tuple:
    """
    init net status by line
    :param line:
    :return:
    """
    fields = line.split()
    if len(fields) >= 12:
        intf = fields[0][0:-1]
        rxbytes = int(fields[1])
        rxpackets = int(fields[2])
        rxerrs = int(fields[3])
        txbytes = int(fields[9])
        txpackets = int(fields[10])
        txerrs = int(fields[11])
        stat_info = (intf, rxbytes, rxpackets, rxerrs, txbytes, txpackets, txerrs)
    else:
        stat_info = (0, 0, 0, 0, 0, 0, 0)
    return stat_info


class HostNetworkUsagePresenter(HostProfPresenterBase):
    """
    class for parsing host mem usage data
    """

    NS_TO_S = 10**9

    def __init__(self: any, result_dir: str, file_name: str = "") -> None:
        super().__init__(result_dir, file_name)
        self.network_usage_info = []
        self.speeds = {}

    @staticmethod
    def compute_netcard_speed() -> float:
        """
        load speed and sum all network cards
        return network speed (KB/s) for all network cards
        """
        _, net_card = InfoConfReader().get_net_info()
        all_netcard_speed = 0
        for net in net_card:
            # Mbits/sec to KByte/sec
            speed = net.get("speed")
            if is_number(speed) and speed != -1:
                all_netcard_speed += speed * Constant.MBPS_TO_BYTES / Constant.KILOBYTE
        return all_netcard_speed

    @classmethod
    def get_timeline_header(cls: any) -> list:
        """
        get timeline header
        """
        return [
            [
                "process_name",
                InfoConfReader().get_json_pid_data(),
                InfoConfReader().get_json_tid_data(),
                "Network Usage",
            ]
        ]

    def init(self: any) -> None:
        """
        init model
        """
        self.set_model(HostNetworkUsage(self.result_dir))

    def parse_prof_data(self: any) -> None:
        """
        implement interface
        """
        try:
            with FileOpen(self.file_name, "r") as file:
                self._parse_network_usage(file.file_reader)
                logging.info("Finish parsing network usage data file: %s", os.path.basename(self.file_name))
        except (FileNotFoundError, ValueError, IOError) as parse_file_except:
            logging.error(
                "Error in parsing network data:%s", str(parse_file_except), exc_info=Constant.TRACE_BACK_SWITCH
            )
        finally:
            pass

    def load_interface_speed(self: any) -> None:
        """
        load speed
        """
        _, net_card = InfoConfReader().get_net_info()
        for net in net_card:
            # Mbits/sec to Byte/sec
            speed = net.get("speed")
            if is_number(speed) and speed != -1:
                self.speeds[net.get("netCardName")] = net.get("speed") * Constant.MBPS_TO_BYTES

    def write_per_usage(
        self: any, curr_timestamp: float, curr_data: dict, last_timestamp: float, last_data: dict
    ) -> None:
        """
        calculate usage then insert to db
        """
        total_bytes = 0
        intf_num = 0
        intf = ""
        for intf, stat in curr_data.items():
            if intf not in last_data:
                continue
            rx_delta = stat.rxbytes - last_data[intf].rxbytes
            tx_delta = stat.txbytes - last_data[intf].txbytes
            if rx_delta < 0 or tx_delta < 0:
                continue
            total_bytes += rx_delta + tx_delta
            intf_num += 1

        if intf_num == 0:
            return

        avg_bytes = Decimal(total_bytes) / intf_num
        time_duration = float(curr_timestamp) - float(last_timestamp)
        speed = avg_bytes / Decimal(time_duration) if time_duration else 0
        usage = speed * NumberConstant.PERCENTAGE / self.speeds.get(intf)
        usage = usage.quantize(NumberConstant.USAGE_PLACES) if intf else 0
        transit_speed = float(speed) * intf_num / NumberConstant.KILOBYTE
        self.cur_model.insert_single_data(
            [
                last_timestamp * HostNetworkUsagePresenter.NS_TO_S,
                curr_timestamp * HostNetworkUsagePresenter.NS_TO_S,
                str(usage),
                transit_speed,
            ]
        )

    def write_usage_items(self: any, file: any) -> None:
        """
        write every usage
        """
        curr_timestamp = None
        last_timestamp = None
        last_data = {}
        curr_data = {}

        for line in file:
            # timestamp
            if line.startswith('time'):
                # compute with last sample point
                if last_timestamp is not None:
                    self.write_per_usage(curr_timestamp, curr_data, last_timestamp, last_data)
                # set curr -> last
                last_data = Utils.dict_copy(curr_data)
                last_timestamp = curr_timestamp
                curr_data.clear()
                # timestamp ns to sec
                tmp_time = line.split()
                if len(tmp_time) > 1 and is_number(tmp_time[1]):
                    curr_timestamp = float(tmp_time[1]) / HostNetworkUsagePresenter.NS_TO_S
            else:
                self._update_cur_data(line, curr_data)

        if curr_data:
            self.write_per_usage(curr_timestamp, curr_data, last_timestamp, last_data)

    def get_network_usage_data(self: any) -> dict:
        """
        get network usage data
        """
        with self.cur_model as model:
            if model.has_network_usage_data():
                res = model.get_network_usage_data()
                return res
        return MsvpConstant.EMPTY_DICT

    def get_timeline_data(self: any) -> list:
        """
        get network timeline data
        """
        mem_usage_data = self.get_network_usage_data()
        result = []
        if mem_usage_data is MsvpConstant.EMPTY_DICT:
            return result

        for data in mem_usage_data.get("data"):
            temp_data = ["Network Usage", float(data["start"]), {"Usage(%)": data["usage"]}]
            result.append(temp_data)
        return result

    def get_summary_data(self: any) -> list:
        """
        get summary data
        """
        with self.cur_model as model:
            if model.has_network_usage_data():
                res = model.get_recommend_value('speed', DBNameConstant.TABLE_HOST_NETWORK_USAGE)
                if res:
                    netcard_speed = self.compute_netcard_speed()
                    return [tuple([netcard_speed, *res])]
        return MsvpConstant.EMPTY_LIST

    def _parse_network_usage(self: any, file: any) -> None:
        # network interface info
        self.load_interface_speed()

        # loop all net stat sample
        self.write_usage_items(file)

    def _update_cur_data(self: any, line: str, curr_data: dict) -> None:
        stat = parse_net_stats(line)
        if stat and stat.intf in self.speeds:
            curr_data[stat.intf] = stat
