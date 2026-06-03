import unittest
from unittest.mock import patch

from host_analyzer.cpu_binder.cpu_binder import CpuAlloc, DeviceInfo, CustomBind, expand_cpu_list


class TestDeviceInfo(unittest.TestCase):

    @patch('misc.host_analyzer.cpu_binder.cpu_binder.execute_command')
    def setUp(self, mock_execute_command):
        mock_execute_command.side_effect = [
            ("NPU ID  Chip ID  Chip Logic ID  Chip Name\n0 0 0 Ascend\n0 1 - Mcu\n1 0 1 Ascend", 0),
            ("| NPU Chip | Process id |\n| 0 0 | 1234 | vllm | 56000 |\n| 1 0 | 1235 | vllm | 56000 |", 0),
            ("", 0)
        ]
        self.device_info = DeviceInfo()

    @patch('misc.host_analyzer.cpu_binder.cpu_binder.execute_command')
    def test_get_npu_map_info(self, mock_execute_command):
        execute_result_list = [
            ("NPU ID  Chip ID  Chip Logic ID  Chip Phy-ID Chip Name\n0 0 0 0 Ascend\n0 1 1 1 Ascend\n0 2 - - Mcu", 0),
            ("NPU ID  Chip ID  Chip Logic ID  Chip Name\n8 0 0 Ascend\n8 1 - Mcu\n9 0 1 Ascend", 0)
        ]
        result_list = [{'0': {'0': '0', '1': '1'}}, {'8': {'0': '0'}, '9': {'0': '1'}}]
        for result in execute_result_list:
            mock_execute_command.return_value = result
            npu_map_info = self.device_info.get_npu_map_info()
            expected = result_list.pop(0)
            self.assertEqual(npu_map_info, expected)

    @patch('misc.host_analyzer.cpu_binder.cpu_binder.execute_command')
    def test_get_running_npus(self, mock_execute_command):
        mock_execute_command.side_effect = [
            ("| NPU Chip | Process id |\n| 0 1 | 1236 | vllm | 56000 |", 0),
            ("", 0),
            ("| NPU Chip | Process id |\n| 1 0 | 1236 | vllm | 56000 |", 0)
        ]
        expected_result_list = [[], [], [1]]
        for expected_result in expected_result_list:
            running_npu_list = self.device_info.get_running_npus()
            self.assertEqual(running_npu_list, expected_result)

    @patch('misc.host_analyzer.cpu_binder.cpu_binder.execute_command')
    def test_parse_topo_affinity(self, mock_execute_command):
        mock_execute_command.side_effect = [
            ("NPU0 X HCCS HCCS HCCS HCCS HCCS HCCS HCCS 0-3", 0),
            ("GPU0 X HCCS HCCS HCCS HCCS HCCS HCCS HCCS 0-3", 0)
        ]
        expected_result_list = [{0: [0, 1, 2, 3]}, {}]
        for expected_result in expected_result_list:
            affinity = self.device_info.parse_topo_affinity()
            self.assertEqual(affinity, expected_result)

    def test_expand_cpu_list(self):
        result = expand_cpu_list("0-2, 4, 6-8")
        self.assertEqual(result, [0, 1, 2, 4, 6, 7, 8])
        with self.assertRaises(RuntimeError):
            expand_cpu_list("0/1")


class TestCpuAlloc(unittest.TestCase):

    @patch('misc.host_analyzer.cpu_binder.cpu_binder.execute_command')
    def setUp(self, mock_execute_command):
        mock_execute_command.side_effect = [
            ("NPU ID  Chip ID  Chip Logic ID  Chip Name\n0 0 0 Ascend\n0 1 - Mcu\n1 0 1 Ascend", 0),
            ("| NPU Chip | Process id |\n| 0 0 | 1234 | vllm | 56000 |\n| 1 0 | 1235 | vllm | 56000 |", 0),
            ("", 0)
        ]
        self.cpu_alloc = CpuAlloc()

    def test_average_distribute(self):
        npu_cpu_pool = {
            0: [10, 11, 12, 13],
            1: [10, 11, 12, 13]
        }
        groups = {"[10, 11, 12, 13]": [0, 1]}
        result = self.cpu_alloc.average_distribute(groups, npu_cpu_pool)
        self.assertEqual(result, {0: [10, 11], 1: [12, 13]})
        npu_cpu_pool = {
            0: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13],
            1: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13],
            2: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13]
        }
        groups = {"[0, 1, 2, 3, 4, 5]": [0, 1, 2]}
        result = self.cpu_alloc.average_distribute(groups, npu_cpu_pool)
        self.assertEqual(result, {
            0: [0, 1, 2, 3],
            1: [4, 5, 6, 7],
            2: [8, 9, 10, 11, 12, 13]
        })

    def test_extend_numa(self):
        result = self.cpu_alloc.extend_numa([])
        self.assertEqual(result, [])
        self.cpu_alloc.cpu_node = {0: 0, 1: 0, 2: 1, 3: 1}
        self.cpu_alloc.numa_to_cpu_map = {0: [0, 1], 1: [2, 3]}
        self.cpu_alloc.device_info.allowed_cpus = [0, 1, 2, 3]
        result = self.cpu_alloc.extend_numa([0, 1])
        self.assertEqual(result, [0, 1, 2, 3])
        self.cpu_alloc.device_info.allowed_cpus = [0, 1, 3]
        result = self.cpu_alloc.extend_numa([0, 1])
        self.assertEqual(result, [0, 1, 3])

    @patch('misc.host_analyzer.cpu_binder.cpu_binder.execute_command')
    def test_build_cpu_node_map(self, mock_execute_command):
        mock_execute_command.return_value = ("", 0)
        with self.assertRaises(RuntimeError):
            self.cpu_alloc.build_cpu_node_map()
        mock_execute_command.return_value = ("0 0\n1 1\n2 0\n3 1", 0)
        self.cpu_alloc.build_cpu_node_map()
        expected_cpu_node = {0: 0, 1: 1, 2: 0, 3: 1}
        expected_numa_to_cpu_map = {0: [0, 2], 1: [1, 3]}
        self.assertEqual(self.cpu_alloc.cpu_node, expected_cpu_node)
        self.assertEqual(self.cpu_alloc.numa_to_cpu_map,
                         expected_numa_to_cpu_map)

    @patch('misc.host_analyzer.cpu_binder.cpu_binder.execute_command')
    def test_handle_no_affinity(self, mock_execute_command):
        mock_execute_command.side_effect = [("0 0\n1 1", 0), ("0 0\n1 1", 0)]
        self.cpu_alloc.device_info.running_npu_list = [0, 1]
        self.cpu_alloc.device_info.allowed_cpus = [0, 1, 2, 3]
        self.cpu_alloc.numa_to_cpu_map = {0: [4, 5], 1: [6, 7]}
        self.cpu_alloc.handle_no_affinity()
        self.assertEqual(self.cpu_alloc.npu_cpu_pool, {})
        self.cpu_alloc.numa_to_cpu_map = {0: [0, 1, 2], 1: [3, 4, 5]}
        self.cpu_alloc.handle_no_affinity()
        self.assertEqual(self.cpu_alloc.npu_cpu_pool, {0: [0, 1, 2], 1: [3]})

    def test_allocate(self):
        self.cpu_alloc.device_info.running_npu_list = [0]
        self.cpu_alloc.npu_cpu_pool = {0: [0, 1, 2, 3, 4, 5, 6]}
        self.cpu_alloc.allocate(2, 1, 1)
        self.assertEqual(self.cpu_alloc.assign_main[0], [3, 4])
        self.assertEqual(self.cpu_alloc.assign_acl[0], [5])
        self.assertEqual(self.cpu_alloc.assign_rel[0], [6])
        self.cpu_alloc.npu_cpu_pool = {0: [0, 1]}
        with self.assertRaises(RuntimeError):
            self.cpu_alloc.allocate(6, 1, 1)


class TestCustomBind(unittest.TestCase):

    def setUp(self):
        self.binder = CustomBind(process_name="test_process", cpu_list=["0-3"], bind_sub_process=True)

    def test_cpu_to_mask(self):
        cpu_list = [0, 1, 30, 39]
        mask_str = self.binder.cpu_to_mask(cpu_list)
        self.assertEqual(mask_str, "00000080,40000003")
        cpu_list = [0]
        mask_str = self.binder.cpu_to_mask(cpu_list)
        self.assertEqual(mask_str, "00000001")

    @patch('os.path.exists', return_value=True)
    @patch('misc.host_analyzer.cpu_binder.cpu_binder.execute_command')
    def test_get_main_pid_from_docker(self, mock_execute_command, mock_exists):
        mock_execute_command.return_value = ("Ngid:\t123", 0)
        pid = self.binder.get_main_pid_from_docker(1000)
        self.assertEqual(pid, 123)
        mock_execute_command.return_value = ("", 1)
        pid = self.binder.get_main_pid_from_docker(1000)
        self.assertEqual(pid, 0)

    @patch('misc.host_analyzer.cpu_binder.cpu_binder.CustomBind.get_main_pid_from_docker')
    def test_get_real_main_pid_list(self, mock_get_main_pid_from_docker):
        mock_get_main_pid_from_docker.return_value = 0
        pid_list = [(1111, 2111), (2112, 3112), (8888, 9999)]
        main_pid_list = [[2111], [2112]]
        real_main_pid_list = self.binder.get_real_main_pid_list(pid_list, main_pid_list)
        self.assertEqual(real_main_pid_list, main_pid_list)
        pid_list = [(8888, 9999)]
        real_main_pid_list = self.binder.get_real_main_pid_list(pid_list, main_pid_list)
        self.assertEqual(real_main_pid_list, [])
        mock_get_main_pid_from_docker.return_value = 2111
        pid_list = [(8888, 9999)]
        real_main_pid_list = self.binder.get_real_main_pid_list(pid_list, main_pid_list)
        self.assertEqual(real_main_pid_list, [[8888]])

    @patch('subprocess.check_output', return_value="1112")
    @patch('misc.host_analyzer.cpu_binder.cpu_binder.execute_command')
    def test_find_threads(self, mock_execute_command, mock_check_output):
        mock_execute_command.return_value = ("", 0)
        with self.assertRaises(RuntimeError):
            self.binder.find_threads()
        mock_execute_command.return_value = ("1113 1114 test_process\n", 0)
        pid_list = self.binder.find_threads()
        self.assertEqual(pid_list, [(1113, 1114)])
        self.binder.pid = [1111]
        pid_list = self.binder.find_threads()
        self.assertEqual(pid_list, [(1111, 1112)])

    @patch('shutil.which', return_value=True)
    @patch('misc.host_analyzer.cpu_binder.cpu_binder.execute_command')
    def test_execute_bind(self, mock_execute_command, mock_which):
        mock_execute_command.return_value = ("", 1)
        with self.assertRaises(RuntimeError):
            self.binder.execute_bind(123, "0, 1", "pid", "0", {0: 0})
        mock_execute_command.return_value = ("", 0)
        self.binder.execute_bind(123, "0, 1", "pid", "0", {0: 0})


if __name__ == '__main__':
    unittest.main()
