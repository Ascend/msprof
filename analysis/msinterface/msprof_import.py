# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
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

import os
import shutil

from common_func.common import warn, print_info
from common_func.config_mgr import ConfigMgr
from common_func.constant import Constant
from common_func.data_check_manager import DataCheckManager
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import MsProfCommonConstant
from common_func.msprof_common import analyze_collect_data, prepare_and_load_info, prepare_for_parse
from common_func.msprof_common import check_path_valid
from common_func.msprof_common import get_path_dir
from common_func.msprof_common import get_valid_sub_path
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from msinterface.msprof_c_interface import dump_device_data
from msparser.cluster.cluster_info_parser import ClusterInfoParser, ClusterBasicInfo
from msparser.cluster.cluster_step_trace_parser import ClusterStepTraceParser
from msparser.parallel.cluster_parallel_collector import ClusterParallelCollector


class ImportCommand:
    """
    The class for handle import command.
    """

    FILE_NAME = os.path.basename(__file__)

    def __init__(self: any, args: any) -> None:
        self.collection_path = os.path.realpath(args.collection_path)
        self.is_cluster_scene = args.cluster_flag
        self.device_cluster_basic_info = {}
        self.device_or_rank_ids = set()
        self.have_rank_id = {}
        self.valid_data_count = 0

    @staticmethod
    def do_import(result_dir: str) -> None:
        """
        parse data from result dir
        :param result_dir: result dir
        :return: None
        """
        try:
            analyze_collect_data(result_dir, ConfigMgr.read_sample_config(result_dir))
        except ProfException:
            warn(
                MsProfCommonConstant.COMMON_FILE_NAME,
                'Analysis data in "%s" failed. Maybe the data is incomplete.' % result_dir,
            )

    def process(self: any) -> None:
        """
        command import command entry
        :return: None
        """
        check_path_valid(self.collection_path, False)
        if not self.is_cluster_scene:
            self._process_parse()
        else:
            _unresolved_dirs = self._find_unresolved_dirs()
            if _unresolved_dirs:
                self._parse_unresolved_dirs(_unresolved_dirs)
            self._prepare_for_cluster_parse()
            print_info(
                MsProfCommonConstant.COMMON_FILE_NAME, 'Start parse cluster data in "%s" ...' % self.collection_path
            )
            cluster_info_parser = ClusterInfoParser(self.collection_path, self.device_cluster_basic_info)
            cluster_info_parser.ms_run()
            cluster_step_trace_parser = ClusterStepTraceParser(self.collection_path)
            cluster_step_trace_parser.ms_run()
            cluster_parallel_collector = ClusterParallelCollector(self.collection_path)
            cluster_parallel_collector.ms_run()
            print_info(MsProfCommonConstant.COMMON_FILE_NAME, 'Cluster data parse finished!')

    def _process_parse(self: any) -> None:
        check_path_valid(self.collection_path, False)
        self._process_sub_dirs()
        if not self.valid_data_count:
            message = f'The path "{self.collection_path}" does not contain valid profiling data.'
            raise ProfException(ProfException.PROF_INVALID_PATH_ERROR, message)

    def _process_sub_dirs(self: any, subdir: str = '', is_cluster: bool = False) -> None:
        collect_path = self.collection_path
        if subdir:
            collect_path = os.path.join(self.collection_path, subdir)
        path_table = {StrConstant.HOST_PATH: "", StrConstant.DEVICE_PATH: []}
        for sub_dir, valid_sub_path, contain_info_json in DataCheckManager.iter_valid_profiling_sub_paths(
            collect_path, self.FILE_NAME, skip_invalid=is_cluster
        ):
            if contain_info_json:
                if sub_dir == StrConstant.HOST_PATH:
                    path_table[StrConstant.HOST_PATH] = valid_sub_path
                else:
                    path_table[StrConstant.DEVICE_PATH].append(valid_sub_path)
            elif collect_path and is_cluster:
                warn(
                    self.FILE_NAME,
                    'Invalid parsing dir(%s), -dir must be profiling data dir, such as PROF_XXX_XXX_XXX' % collect_path,
                )
            else:
                self._process_sub_dirs(sub_dir, is_cluster=True)
        self._process_data(path_table)

    def _process_data(self, path_table: dict):
        if not path_table.get(StrConstant.HOST_PATH) and not path_table.get(StrConstant.DEVICE_PATH):
            warn(self.FILE_NAME, 'Can not find any host or device path in dir("%s"). ' % self.collection_path)
            return
        self.valid_data_count += bool(path_table.get(StrConstant.HOST_PATH))
        self.valid_data_count += len(path_table.get(StrConstant.DEVICE_PATH))
        # start parse
        self._start_parse(path_table)

    def _start_parse(self, path_table: dict):
        # host
        host_path = path_table.get(StrConstant.HOST_PATH)
        self._parse_data(host_path)
        # device
        for device_path in path_table.get(StrConstant.DEVICE_PATH):
            self._parse_data(device_path)
        # device 执行完后执行device c化
        dump_device_data(host_path if host_path else path_table.get(StrConstant.DEVICE_PATH)[0])

    def _parse_data(self, device_path: str):
        if not device_path:
            return
        prepare_and_load_info(device_path)
        self.do_import(device_path)

    def _find_unresolved_dirs(self: any) -> dict:
        prof_dirs = self._get_prof_dirs()
        print_info(
            MsProfCommonConstant.COMMON_FILE_NAME,
            'Start verify that all data is parsed in "%s" .' % self.collection_path,
        )
        _unresolved_dirs = {}
        for prof_dir in prof_dirs:
            _unresolved_prof_sub_dirs = []
            _invalid_prof_sub_dirs = []
            prof_path = os.path.realpath(os.path.join(self.collection_path, prof_dir))
            if not os.listdir(prof_path):
                warn(
                    MsProfCommonConstant.COMMON_FILE_NAME,
                    'The path(%s) is not a PROF dir, Please check the path.' % prof_path,
                )
                continue
            prof_sub_dirs = get_path_dir(prof_path)
            for prof_sub_dir in prof_sub_dirs:
                prof_sub_path = os.path.realpath(os.path.join(prof_path, prof_sub_dir))
                prof_sub_path_check = self._check_prof_sub_path(prof_sub_path)
                if prof_sub_path_check == Constant.DEFAULT_TURE_VALUE:
                    _unresolved_prof_sub_dirs.append(prof_sub_path)
                if prof_sub_path_check == Constant.DEFAULT_INVALID_VALUE:
                    _invalid_prof_sub_dirs.append(prof_sub_dir)
            if _unresolved_prof_sub_dirs:
                _unresolved_dirs.setdefault(prof_dir, _unresolved_prof_sub_dirs)
            if _invalid_prof_sub_dirs:
                warn(
                    MsProfCommonConstant.COMMON_FILE_NAME,
                    'There are some invalid PROF dirs in the path(%s)! '
                    'please check the dirs: %s' % (prof_path, _invalid_prof_sub_dirs),
                )
        if not self.device_cluster_basic_info:
            message = f"None of device PROF dir find in the dir({self.collection_path}), please check the -dir argument"
            raise ProfException(ProfException.PROF_CLUSTER_DIR_ERROR, message)
        return _unresolved_dirs

    def _get_prof_dirs(self: any) -> list:
        self._dir_exception_check(self.collection_path)
        prof_dirs = get_path_dir(self.collection_path)
        for prof_dir in prof_dirs:
            self._dir_exception_check(os.path.join(self.collection_path, prof_dir))
        return prof_dirs

    def _dir_exception_check(self: any, path: str) -> None:
        if DataCheckManager.contain_info_json_data(path):
            message = f"Incorrect parse dir({self.collection_path}), -dir argument must be cluster data root dir."
            raise ProfException(ProfException.PROF_CLUSTER_DIR_ERROR, message)

    def _check_prof_sub_path(self: any, prof_sub_path: str) -> int:
        if not DataCheckManager.contain_info_json_data(prof_sub_path):
            return Constant.DEFAULT_INVALID_VALUE
        cluster_basic_info = ClusterBasicInfo(prof_sub_path)
        cluster_basic_info.init()
        if not cluster_basic_info.is_host_profiling:
            _have_rank_id = cluster_basic_info.rank_id != Constant.DEFAULT_INVALID_VALUE
            if self.have_rank_id.setdefault("have_rank_id", _have_rank_id) != _have_rank_id:
                message = (
                    f"The dir({self.collection_path}) contains both cluster and "
                    f"non-cluster data! please check whether all devices have rank ids."
                )
                raise ProfException(ProfException.PROF_CLUSTER_DIR_ERROR, message)
            _device_or_rank_id = int(cluster_basic_info.rank_id) if _have_rank_id else int(cluster_basic_info.device_id)
            if _device_or_rank_id in self.device_or_rank_ids:
                message = (
                    f"There are same rank_id or device_id ( {_device_or_rank_id} ) in "
                    f"the dir({self.collection_path}). Please check the PROF dirs!"
                )
                raise ProfException(ProfException.PROF_CLUSTER_DIR_ERROR, message)
            self.device_or_rank_ids.add(_device_or_rank_id)
            dir_name = os.sep.join(prof_sub_path.split(os.sep)[-2:])
            self.device_cluster_basic_info[dir_name] = cluster_basic_info
        sqlite_path = PathManager.get_sql_dir(prof_sub_path)
        if not os.path.exists(sqlite_path) or not os.listdir(sqlite_path):
            return Constant.DEFAULT_TURE_VALUE
        return Constant.DEFAULT_FALSE_VALUE

    def _parse_unresolved_dirs(self: any, unresolved_dirs: dict) -> None:
        print_info(
            MsProfCommonConstant.COMMON_FILE_NAME, 'Start parse unresolved dirs in "%s" ...' % self.collection_path
        )
        path_table = {StrConstant.HOST_PATH: "", StrConstant.DEVICE_PATH: []}
        for pro_dir, result_dir_list in unresolved_dirs.items():
            prof_path = os.path.join(self.collection_path, pro_dir)
            for result_dir in result_dir_list:
                result_path = get_valid_sub_path(prof_path, result_dir, False)
                # 统一收集合法路径 后续统一处理
                if result_path.endswith(StrConstant.HOST_PATH):
                    path_table[StrConstant.HOST_PATH] = result_path
                else:
                    path_table[StrConstant.DEVICE_PATH].append(result_path)
            self._process_data(path_table)
        print_info(MsProfCommonConstant.COMMON_FILE_NAME, 'Unresolved dirs parse finished!')

    def _prepare_for_cluster_parse(self: any) -> None:
        cluster_sqlite_path = PathManager.get_sql_dir(self.collection_path)
        if os.path.exists(cluster_sqlite_path):
            shutil.rmtree(cluster_sqlite_path)
        # The cluster root holds aggregate databases, not device info.json files.
        # Device information is loaded from individual profiling paths during parsing.
        prepare_for_parse(self.collection_path)
