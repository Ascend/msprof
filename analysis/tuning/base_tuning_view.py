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
from tuning.data_manager import DataManager, BaseTuningDataHandle


class BaseTuningView:
    """
    view for tuning
    """

    def __init__(self: any) -> None:
        self.data = {}
        self.turing_start = "Performance Summary Report"

    @staticmethod
    def print_first_level(index: any, data: dict) -> None:
        """
        print title of first level
        :param index: index
        :param data: data
        :return: None
        """
        if data and data.get(CommonProfRule.RESULT_RULE_TYPE):
            print_msg("{0}. {1}:".format(index, data.get(CommonProfRule.RESULT_RULE_TYPE)))

    @staticmethod
    def print_second_level(data: any, handle_class: BaseTuningDataHandle) -> None:
        """
        :param data: data
        :param handle_class: print information
        :return: None
        """
        if not data:
            print_msg("\tN/A")
            return
        sub_rule_dict = {}
        for result_index, result in enumerate(data):
            rule_sub_type = result.get(CommonProfRule.RESULT_RULE_SUBTYPE, "")
            if rule_sub_type:
                sub_rule_dict.setdefault(rule_sub_type, []).append(result)
            else:
                message = handle_class.print_format(result.get(CommonProfRule.RESULT_TUNING_DATA, []))
                print_msg(
                    "\t{0}){1}: {2}".format(
                        result_index + 1, result.get(CommonProfRule.RESULT_RULE_SUGGESTION, ""), message
                    )
                )
        if sub_rule_dict:
            for sub_key_index, sub_key in enumerate(sub_rule_dict.keys()):
                print_msg("\t{0}){1}:".format(sub_key_index + 1, sub_key))
                for value_index, value in enumerate(sub_rule_dict.get(sub_key)):
                    # pylint: disable=undefined-loop-variable
                    message = handle_class.print_format(result.get(CommonProfRule.RESULT_TUNING_DATA, []))
                    print_msg(
                        "\t\t{0}){1}: {2}".format(
                            value_index + 1, value.get(CommonProfRule.RESULT_RULE_SUGGESTION, ""), message
                        )
                    )

    def get_tuning_data(self: any) -> None:
        """
        get turing data
        :return:
        """
        self.data = {}

    def tuning_report(self: any) -> None:
        """
        tuning report
        :return: None
        """
        self.get_tuning_data()
        if not self.data:
            return
        print_msg("\n{0}:".format(self.turing_start))
        global_index = 0
        for tuning_type, tuning_data_handle_class in DataManager.HANDLE_MAP.items():
            for every_data in self.data.get(tuning_type, []):
                global_index += 1
                self.print_first_level(global_index, every_data)
                self.print_second_level(every_data.get(CommonProfRule.RESULT_KEY), tuning_data_handle_class)
        print_msg("\n")
