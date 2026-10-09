#!/usr/bin/python3
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

import unittest

from mscalculate.cluster.slow_link_calculator import SlowLinkCalculator

NAMESPACE = 'mscalculate.cluster.slow_link_calculator'


class TestSlowLinkCalculator(unittest.TestCase):

        op_rank_list = [('AllReduce_1_1', 0)]
        op_info = {t[0]: {t[1]: {}} for t in op_rank_list}

        def test_run_small_packet(self):
            data = [
                {
                    "RDMA": {
                        "Transit Size(MB)": 26.791015625,
                        "Transit Time(ms)": 2.30471,
                        "Bandwidth(GB/s)": 11.6245,
                        "Bandwidth(Utilization)": 0.73,
                        "Large Packet Ratio": 0.0,
                        "Size Distribution": {
                            "13.3955078125": 2.0
                        }
                    },
                    "HCCS": {
                        "Transit Size(MB)": 281.3056640625,
                        "Transit Time(ms)": 20.709440000000004,
                        "Bandwidth(GB/s)": 13.5835,
                        "Bandwidth(Utilization)": 0.7546,
                        "Large Packet Ratio": 0.0,
                        "Size Distribution": {
                            "6.69775390625": 42.0
                        }
                    },
                    "PCIE": {
                        "Transit Size(MB)": 93.7685546875,
                        "Transit Time(ms)": 6.71993,
                        "Bandwidth(GB/s)": 13.9538,
                        "Bandwidth(Utilization)": 0.6977,
                        "Large Packet Ratio": 0.0,
                        "Size Distribution": {
                            "6.69775390625": 14.0
                        }
                    },
                    "SDMA": {
                        "Transit Size(MB)": 375.07421875,
                        "Transit Time(ms)": 2.429370000000006,
                        "Bandwidth(GB/s)": 13.6742,
                        "Bandwidth(Utilization)": 0,
                        "Large Packet Ratio": 0,
                        "Size Distribution": {}
                    }
                }
            ]
            cal = SlowLinkCalculator(data, self.op_rank_list)
            cal.run()
            cal.add_suggestions(self.op_info)

        def test_run_bandwidth_config(self):
            data = [
                {
                    "RDMA": {
                        "Transit Size(MB)": 26.791015625,
                        "Transit Time(ms)": 2.30471,
                        "Bandwidth(GB/s)": 11.6245,
                        "Bandwidth(Utilization)": 0.63,
                        "Large Packet Ratio": 1.0,
                        "Size Distribution": {
                            "13.3955078125": 2.0
                        }
                    },
                    "HCCS": {
                        "Transit Size(MB)": 281.3056640625,
                        "Transit Time(ms)": 20.709440000000004,
                        "Bandwidth(GB/s)": 13.5835,
                        "Bandwidth(Utilization)": 0.7546,
                        "Large Packet Ratio": 1.0,
                        "Size Distribution": {
                            "66.69775390625": 42.0
                        }
                    },
                    "PCIE": {
                        "Transit Size(MB)": 93.7685546875,
                        "Transit Time(ms)": 6.71993,
                        "Bandwidth(GB/s)": 13.9538,
                        "Bandwidth(Utilization)": 0.6977,
                        "Large Packet Ratio": 1.0,
                        "Size Distribution": {
                            "66.69775390625": 14.0
                        }
                    },
                    "SDMA": {
                        "Transit Size(MB)": 375.07421875,
                        "Transit Time(ms)": 2.429370000000006,
                        "Bandwidth(GB/s)": 13.6742,
                        "Bandwidth(Utilization)": 0,
                        "Large Packet Ratio": 0,
                        "Size Distribution": {}
                    }
                }
            ]
            cal = SlowLinkCalculator(data, self.op_rank_list)
            cal.run()
            cal.add_suggestions(self.op_info)

        def test_run_no_slow_link(self):
            data = [
                {
                    "RDMA": {
                        "Transit Size(MB)": 26.791015625,
                        "Transit Time(ms)": 2.30471,
                        "Bandwidth(GB/s)": 11.6245,
                        "Bandwidth(Utilization)": 0.93,
                        "Large Packet Ratio": 1.0,
                        "Size Distribution": {
                            "13.3955078125": 2.0
                        }
                    },
                    "HCCS": {
                        "Transit Size(MB)": 281.3056640625,
                        "Transit Time(ms)": 20.709440000000004,
                        "Bandwidth(GB/s)": 13.5835,
                        "Bandwidth(Utilization)": 0.8546,
                        "Large Packet Ratio": 1.0,
                        "Size Distribution": {
                            "66.69775390625": 42.0
                        }
                    },
                    "PCIE": {
                        "Transit Size(MB)": 93.7685546875,
                        "Transit Time(ms)": 6.71993,
                        "Bandwidth(GB/s)": 13.9538,
                        "Bandwidth(Utilization)": 0.8977,
                        "Large Packet Ratio": 1.0,
                        "Size Distribution": {
                            "66.69775390625": 14.0
                        }
                    },
                    "SDMA": {
                        "Transit Size(MB)": 375.07421875,
                        "Transit Time(ms)": 2.429370000000006,
                        "Bandwidth(GB/s)": 13.6742,
                        "Bandwidth(Utilization)": 0,
                        "Large Packet Ratio": 0,
                        "Size Distribution": {}
                    }
                }
            ]
            cal = SlowLinkCalculator(data, self.op_rank_list)
            cal.run()
            cal.add_suggestions(self.op_info)
