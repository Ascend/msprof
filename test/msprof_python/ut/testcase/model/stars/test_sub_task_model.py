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

from common_func.db_name_constant import DBNameConstant
from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.stars.sub_task_model import SubTaskTimeModel


class TestSubTaskModel(TestDirCRBaseModel):

    def test_flush_should_no_return(self: any) -> None:
        data = [
            [0, 23, 2, "AIV", "FFTS+", 1000, 2000, 1000, 0],
            [1, 23, 2, "AIC", "FFTS+", 3000, 4000, 1000, 0],
            [2, 23, 2, "AIC", "FFTS+", 5000, 6000, 1000, 0],
            [0, 24, 2, "MIX_AIV", "FFTS+", 7000, 8000, 1000, 0],
        ]
        with SubTaskTimeModel(self.PROF_DEVICE_DIR) as model:
            model.flush(data)
            self.assertEqual(len(model.get_all_data(DBNameConstant.TABLE_SUBTASK_TIME)), 4)
