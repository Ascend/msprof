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

import json
import logging
import os

from common_func.common_prof_rule import CommonProfRule
from common_func.file_manager import FileOpen
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from tuning.base_tuning_view import BaseTuningView


class TuningView(BaseTuningView):
    """
    view for tuning
    """

    def __init__(self: any, result_dir: str, sample_config: dict, dev_id: any) -> None:
        super().__init__()
        self.result_dir = result_dir
        self.sample_config = sample_config
        self.dev_id = dev_id
        self.data = None

    def show_by_dev_id(self: any) -> None:
        """
        show data by device id
        :return: None
        """
        self.tuning_report()

    def get_tuning_data(self: any) -> None:
        """
        tuning report
        :return: None
        """
        self.data = self._load_result_file(self.dev_id).get("data", {})

    def _load_result_file(self: any, dev_id: any) -> dict:
        file_name = CommonProfRule.RESULT_PROF_JSON.format(dev_id)
        if dev_id == str(NumberConstant.HOST_ID):
            file_name = CommonProfRule.RESULT_PROF_JSON_HOST
        prof_rule_path = os.path.join(PathManager.get_summary_dir(self.result_dir), file_name)
        try:
            if os.path.exists(prof_rule_path):
                with FileOpen(prof_rule_path, "r") as rule_reader:
                    return json.load(rule_reader.file_reader)
            return {}
        except FileNotFoundError:
            logging.error("Read rule file failed: %s", os.path.basename(prof_rule_path))
            return {}
        finally:
            pass
