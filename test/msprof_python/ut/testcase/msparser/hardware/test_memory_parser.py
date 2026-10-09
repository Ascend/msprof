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
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from msparser.hardware.sys_mem_parser import ParsingMemoryData
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hardware.sys_mem_parser'


class TestParsingMemoryData(unittest.TestCase):
    file_list = {DataTag.SYS_MEM: ['Memory.data.0.slice_0'],
                 DataTag.PID_MEM: ['0-Memory.data.0.slice_0']}

    def test_get_sys_data(self):
        data = 'TimeStamp:586320603077\n' + \
               'Index:0\n' + \
               'DataLen:1308\n' + \
               'MemTotal:       21777756 kB\n' + \
               'MemFree:        19276752 kB\n' + \
               'MemAvailable:   19216840 kB\n' + \
               'Buffers:           12396 kB\n' + \
               'Cached:           272816 kB\n' + \
               'SwapCached:            0 kB\n' + \
               'Active:           182036 kB\n' + \
               'Inactive:         155360 kB\n' + \
               'Active(anon):      86104 kB\n' + \
               'Inactive(anon):    94892 kB\n' + \
               'Active(file):      95932 kB\n' + \
               'Inactive(file):    60468 kB\n' + \
               'Unevictable:       33924 kB\n' + \
               'Mlocked:           33924 kB\n' + \
               'SwapTotal:             0 kB\n' + \
               'SwapFree:              0 kB\n' + \
               'Dirty:               580 kB\n' + \
               'Writeback:             0 kB\n' + \
               'AnonPages:         86268 kB\n' + \
               'Mapped:            98720 kB\n' + \
               'Shmem:            128784 kB\n' + \
               'Slab:             111860 kB\n' + \
               'SReclaimable:      36748 kB\n' + \
               'SUnreclaim:        75112 kB\n' + \
               'KernelStack:        8316 kB\n' + \
               'PageTables:         4448 kB\n' + \
               'NFS_Unstable:          0 kB\n' + \
               'Bounce:                0 kB\n' + \
               'WritebackTmp:          0 kB\n' + \
               'CommitLimit:    10882732 kB\n' + \
               'Committed_AS:    1917316 kB\n' + \
               'VmallocTotal:   135290290112 kB\n' + \
               'VmallocUsed:           0 kB\n' + \
               'VmallocChunk:          0 kB\n' + \
               'Percpu:             7232 kB\n' + \
               'HardwareCorrupted:     0 kB\n' + \
               'AnonHugePages:         0 kB\n' + \
               'ShmemHugePages:        0 kB\n' + \
               'ShmemPmdMapped:        0 kB\n' + \
               'CmaTotal:         262144 kB\n' + \
               'CmaFree:           94252 kB\n' + \
               'HugePages_Total:       6\n' + \
               'HugePages_Free:        0\n' + \
               'HugePages_Rsvd:        0\n' + \
               'HugePages_Surp:        0\n' + \
               'Hugepagesize:       2048 kB\n' + \
               'Hugetlb:           12288 kB\n' + \
               '\n'
        with mock.patch('os.path.join', return_value=TypeError), \
             mock.patch('common_func.file_manager.check_path_valid'), \
             mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingMemoryData(self.file_list, CONFIG)
            check.get_sys_data('Memory.data.0.slice_0')
        with mock.patch('os.path.join', return_value='test\\data'), \
             mock.patch('common_func.file_manager.check_path_valid'), \
             mock.patch('builtins.open', mock.mock_open(read_data=data)):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingMemoryData(self.file_list, CONFIG)
            check.get_sys_data('Memory.data.0.slice_0')

    def test_get_pid_data(self):
        data = 'TimeStamp:100000\n' + \
               'Index:8\n' + \
               'DataLen:34\n' + \
               '0 0 0 0 0 0 0\n' + \
               'ProcessName:cpuhp/1\n' + \
               '\n'
        with mock.patch('os.path.join', return_value=TypeError), \
             mock.patch('common_func.file_manager.check_path_valid'), \
             mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingMemoryData(self.file_list, CONFIG)
            check.get_pid_data('0-Memory.data.0.slice_0')
        with mock.patch('os.path.join', return_value='test\\data'), \
             mock.patch('common_func.file_manager.check_path_valid'), \
             mock.patch('builtins.open', mock.mock_open(read_data=data)):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingMemoryData(self.file_list, CONFIG)
            check.get_pid_data('0-Memory.data.0.slice_0')

    def test_main(self):
        with mock.patch('os.path.join', side_effect=RuntimeError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingMemoryData(self.file_list, CONFIG)
            check.main()
        with mock.patch('os.path.join', return_value='test\\data'), \
                mock.patch(NAMESPACE + '.is_valid_original_data'), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.ParsingMemoryData.get_sys_data'), \
                mock.patch(NAMESPACE + '.ParsingMemoryData.get_pid_data'), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingMemoryData(self.file_list, CONFIG)
            check.main()

    def test_save(self):
        with mock.patch('msmodel.hardware.sys_mem_model.SysMemModel.init'), \
                mock.patch('msmodel.hardware.sys_mem_model.SysMemModel.flush'), \
                mock.patch('msmodel.hardware.sys_mem_model.SysMemModel.create_table'), \
                mock.patch('msmodel.hardware.sys_mem_model.SysMemModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingMemoryData(self.file_list, CONFIG)
            check.data_dict = {'sys_data_list': [123]}
            check.save()

    def test_ms_run(self):
        with mock.patch('os.path.join', return_value='test\\data'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.listdir', return_value=['test', 'test1']), \
                mock.patch(NAMESPACE + '.ParsingMemoryData.main'):
            InfoConfReader()._info_json = {'devices': '0'}
            ParsingMemoryData(self.file_list, CONFIG).ms_run()
        with mock.patch('os.path.join', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            ParsingMemoryData(self.file_list, CONFIG).ms_run()
