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
import struct
import unittest
from unittest import mock

from common_func.hash_dict_constant import HashDictData
from msmodel.api.api_data_model import ApiDataModel
from profiling_bean.struct_info.api_data_bean import ApiDataBean

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'msmodel.api.api_data_model'


class TestApiDataModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.ApiDataModel.reformat_data'), \
                mock.patch(NAMESPACE + '.ApiDataModel.insert_data_to_db'):
            self.assertEqual(ApiDataModel('test').flush([]), None)

        with mock.patch(NAMESPACE + '.ApiDataModel.reformat_data', side_effect=IndexError), \
                mock.patch(NAMESPACE + '.ApiDataModel.insert_data_to_db'):
            self.assertEqual(ApiDataModel('test').flush([]), None)

    def test_reformat_data(self):
        bin_data = struct.pack('=HHIIIQQQHHIIIQQQHHIIIQQQ',
                               23130, 5000, 1113, 94827, 0, 3095879082697020, 3095879082697020, 0,
                               23130, 20000, 458759, 94815, 0, 3095879086911896, 3095879086912038, 0,
                               23130, 5000, 1002, 94815, 0, 3095879519081144, 3095879519087172, 0)
        connection_ids = [0, 1, 2]
        data_list = [
            (connection_ids[0], ApiDataBean.decode(bin_data[:40])),
            (connection_ids[1], ApiDataBean.decode(bin_data[40:80])),
            (connection_ids[2], ApiDataBean.decode(bin_data[80:])),
        ]
        HashDictData('test')._type_hash_dict = {'runtime': {}, 'acl': {}}
        HashDictData('test')._ge_hash_dict = {}
        with mock.patch(NAMESPACE + '.ApiDataModel.insert_data_to_db'):
            self.assertEqual(len(ApiDataModel('test').reformat_data(data_list)), 3)

        with mock.patch('common_func.singleton.singleton', return_value={}), \
                mock.patch(NAMESPACE + '.ApiDataModel.insert_data_to_db'):
            self.assertEqual(len(ApiDataModel('test').reformat_data(data_list)), 3)

    def test_update_type_hash_value(self):
        bin_data = struct.pack('=HHIIIQQQ', 23130, 20000, 458759, 94815, 0, 0, 3095879086912038, 0)
        bin_data_1 = struct.pack('=HHIIIQQQ', 23130, 5000, 458759, 94815, 0, 0, 3095879086912038, 0)
        data = ApiDataBean.decode(bin_data)
        data_1 = ApiDataBean.decode(bin_data_1)
        res = ApiDataModel('test').update_type_hash_value(data, {'acl': {'917518': 'opexecute'}})
        res_1 = ApiDataModel('test').update_type_hash_value(data, {'hccl': {'917518': 'opexecute'}})
        res_2 = ApiDataModel('test').update_type_hash_value(data_1, {'hccl': {'917518': 'opexecute'}})
        self.assertEqual(res, ('HOST_HCCL', '458759'))
        self.assertEqual(res_1, ('458759', 0))
        self.assertEqual(res_2, ('458759', 0))


if __name__ == '__main__':
    unittest.main()
