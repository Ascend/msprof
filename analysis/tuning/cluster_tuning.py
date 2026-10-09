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

from common_func.common import print_msg
from common_func.common_prof_rule import CommonProfRule
from mscalculate.cluster.cluster_link_calculate import ClusterLinkCalculator
from mscalculate.cluster.trailing_calculator import TrailingCalculator
from tuning.base_tuning_view import BaseTuningView


class ClusterTuning(BaseTuningView):
    """
    recommend for inference
    """

    def __init__(self: any, cluster_params: list) -> None:
        super().__init__()
        self.cluster_params = cluster_params
        self.calculate_list = {
            TrailingCalculator: 'For slow nodes, pay attention to the data preparation phase (threshold: 20%)',
            ClusterLinkCalculator: "For slow link, pay attention to the data bandwidth (threshold: 20%)",
        }
        self.data = []
        self.turing_start = "Cluster Tuning Report"

    @staticmethod
    def print_second_level(data: any) -> None:
        """
        :param data: data
        :return: None
        """
        if not data:
            print_msg("\tN/A")
            return
        for result_index, key in enumerate(data.keys()):
            if not data.get(key, ''):
                print_msg("\tN/A")
                return
            print_msg(
                "\t{0}) {1}: \n\t {2}".format(result_index + 1, key, "\n\t ".join(list(map(str, data.get(key, '')))))
            )

    def run(self: any) -> None:
        """
        run and recommend
        """
        self.tuning_report()

    def get_tuning_data(self: any):
        """
        get turing data
        :return:
        """
        for calculator, value in self.calculate_list.items():
            calculator_result = {
                CommonProfRule.RESULT_RULE_TYPE: value,
                'result': calculator(self.cluster_params).run(),
            }
            self.data.append(calculator_result)

    def tuning_report(self: any):
        self.get_tuning_data()
        if not self.data:
            return
        print_msg(f'\n{self.turing_start}:')
        for index, every_data in enumerate(self.data):
            self.print_first_level(index + 1, every_data)
            self.print_second_level(every_data.get("result"))
        print_msg("\n")
