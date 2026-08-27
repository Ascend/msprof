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
import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.add_info.graph_add_info_model import GraphAddInfoModel
from msmodel.v5.v5_exeom_model import V5ExeomModel
from msmodel.v5.v5_exeom_model import V5ExeomViewModel
from msmodel.v5.v5_exeom_model import V5GraphAddInfoViewModel

NAMESPACE = 'msmodel.v5.v5_exeom_model'

ADD_INFO_DATA_LIST = [['model', 'model_exeom', 844, 187587417144680, 'quantized', 2]]
GE_TASK_INFO_DATA_LIST = [
    [2, 'trans_TransData_0', 0, 0, 1, 0, '0', 'KERNEL', 'TransData', -1, 16, -1, 0, -1,
     'FLOAT16;', '32,1,512,36;', 'NHWC;', 'FLOAT16;', '32,5,1,512,8;', 'FORMAT_1;', 0, 4294967295, "NO", "N/A",
     "N/A", "N/A"]
]


class TestV5ExeomModel(TestDirCRBaseModel):
    def test_flush_when_get_data_list_then_return_none(self):
        with mock.patch(NAMESPACE + '.V5ExeomModel.insert_data_to_db'):
            self.assertEqual(V5ExeomModel('test').flush(GE_TASK_INFO_DATA_LIST,
                                                        table_name=DBNameConstant.TABLE_GE_TASK), None)

    def test_get_v5_model_filename_with_model_id_data_then_get_name_with_id(self):
        with GraphAddInfoModel(self.PROF_DEVICE_DIR) as model:
            model.flush(ADD_INFO_DATA_LIST)
            with V5GraphAddInfoViewModel(self.PROF_DEVICE_DIR) as view_model:
                name_dict = {'quantized': 2}
                res = view_model.get_v5_model_filename_with_model_id_data()
                self.assertEqual(res, name_dict)

    def test_get_v5_thread_id_with_model_id_data_then_get_model_id_with_thread_id(self):
        with GraphAddInfoModel(self.PROF_DEVICE_DIR) as model:
            model.flush(ADD_INFO_DATA_LIST)
            with V5GraphAddInfoViewModel(self.PROF_DEVICE_DIR) as view_model:
                thread_id_dict = {2: 844}
                res = view_model.get_v5_thread_id_with_model_id_data()
                self.assertEqual(res, thread_id_dict)

    def test_get_v5_host_data_then_get_host_info(self):
        with V5ExeomModel(self.PROF_DEVICE_DIR) as model:
            model.flush(GE_TASK_INFO_DATA_LIST, DBNameConstant.TABLE_GE_TASK)
            with V5ExeomViewModel(self.PROF_DEVICE_DIR) as view_model:
                host_info_dict = {(2, 0): [(0, 1)]}
                res = view_model.get_v5_host_data()
                self.assertEqual(res, host_info_dict)


if __name__ == '__main__':
    unittest.main()
