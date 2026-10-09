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
import os
import unittest
from unittest import mock

from constant.constant import CONFIG
from msparser.aicpu.data_preparation_parser import DataPreparationParser
from profiling_bean.prof_enum.data_tag import DataTag
from subclass.data_preparation_parser_subclass import DataPreparationParserSubclass

NAMESPACE = 'msparser.aicpu.data_preparation_parser'


class TestDataPreparationParser(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_DataPreparationParser')
    FILE_LIST = {DataTag.DATA_QUEUE: ['DATA_PREPROCESS.AICPUMI.3.slice_0'],
                 DataTag.HOST_QUEUE: ['Framework.device_queue.3.slice_0']}
    RAW_DATA = b'Node:GetNext_dequeue_wait, queue size:108, Run start:65347643713, ' \
               b'Run end:65347643732\n\x00Node:GetNext_dequeue_wait, queue size:107, ' \
               b'Run start:65347648468, Run end:65347648517\n\x00Node:GetNext_dequeue_wait, ' \
               b'queue size:106, Run start:65347649639, Run end:65347649655'
    READED_RAW_DATA = ['Node:GetNext_dequeue_wait, queue size:108, Run start:65347643713, Run end:65347643732',
                       'Node:GetNext_dequeue_wait, queue size:107, Run start:65347648468, Run end:65347648517',
                       'Node:GetNext_dequeue_wait, queue size:106, Run start:65347649639, Run end:65347649655']
    PARSED_DATA_QUEUE = [["Node:GetNext_dequeue_wait", 108, 65347643713, 65347643732, 19],
                         ["Node:GetNext_dequeue_wait", 107, 65347648468, 65347648517, 49]]
    PARSED_HOST_QUEUE = [[0, 1, 1, 0, 1], [0, 2, 1, 1020, 1], [0, 0, 1, 1020, 1], [1, 16, 1, 0, 1]]
    DATA_LINES = ["0 1 1 0 1402709056", "0 2 1 1020 1402709056", "0 0 1 1020 1402709056", "1 16 1 0 1402709056"]
    HOST_QUEUE_NON_SINK_MODE_FILE = ['Framework.dataset_iterator.3.slice_0']

    def test_format_host_queue_raw_data(self):
        self.assertEqual(len(DataPreparationParserSubclass(
            self.FILE_LIST, CONFIG).format_host_queue_raw_data(1, self.DATA_LINES)), 4)

    def test__read_data_queue(self):
        with mock.patch(NAMESPACE + '.check_file_readable'), \
                mock.patch("builtins.open", mock.mock_open(read_data=self.RAW_DATA)), \
                mock.patch('common_func.file_manager.check_path_valid', return_value=True):
            self.assertEqual(len(
                DataPreparationParserSubclass(self.FILE_LIST, CONFIG).read_data_queue(self.DIR_PATH)), 3)

    def test__parse_data_queue(self):
        with mock.patch(NAMESPACE + '.DataPreparationParser._parse_data_queue_file',
                        return_value=self.PARSED_DATA_QUEUE), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path'):
            parser = DataPreparationParserSubclass(
                {DataTag.DATA_QUEUE: self.FILE_LIST.get(DataTag.DATA_QUEUE)}, CONFIG)
            self.assertEqual(len(parser.parse_data_queue(['DATA_PREPROCESS.AICPUMI.3.slice_0'])), 2)

    def test__parse_host_queue(self):
        with mock.patch(NAMESPACE + '.DataParser.parse_plaintext_data', return_value=self.PARSED_HOST_QUEUE):
            parser = DataPreparationParserSubclass(
                {DataTag.HOST_QUEUE: self.FILE_LIST.get(DataTag.HOST_QUEUE)}, CONFIG)
            self.assertEqual(len(parser.parse_host_queue(['Framework.device_queue.3.slice_0'])), 4)

    def test__check_host_queue_mode(self):
        parser = DataPreparationParserSubclass({}, {})
        parser.check_host_queue_mode(self.HOST_QUEUE_NON_SINK_MODE_FILE)
        self.assertEqual(parser.get_host_queue_mode(), DataPreparationParser.HOST_DATASET_NOT_SINK_MODE)

    def test__parse_data_queue_file(self):
        with mock.patch(NAMESPACE + '.DataPreparationParser._read_data_queue', return_value=self.READED_RAW_DATA):
            parser = DataPreparationParserSubclass(self.FILE_LIST, CONFIG)
            self.assertEqual(len(parser.parse_data_queue_file('')), 3)

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.DataParser.parse_plaintext_data', return_value=self.PARSED_HOST_QUEUE), \
                mock.patch(NAMESPACE + '.DataPreparationParser._parse_host_queue',
                           return_value=self.PARSED_HOST_QUEUE), \
                mock.patch(NAMESPACE + '.DataPreparationParser._parse_data_queue',
                           return_value=self.PARSED_DATA_QUEUE), \
                mock.patch('msmodel.interface.base_model.BaseModel.insert_data_to_db'):
            parser = DataPreparationParserSubclass(self.FILE_LIST, CONFIG)
            parser.ms_run()


if __name__ == '__main__':
    unittest.main()
