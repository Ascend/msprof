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

import argparse
import os
import sys

from common_func.common import call_sys_exit
from common_func.common import error, warn
from common_func.file_manager import is_root_user
from common_func.msprof_common import check_path_valid
from common_func.msprof_common import check_path_char_valid
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_exception import ProfException
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode
from msinterface.msprof_analyze import AnalyzeCommand
from msinterface.msprof_export import ExportCommand
from msinterface.msprof_import import ImportCommand
from msinterface.msprof_query import QueryCommand
from msinterface.msprof_query_summary_manager import QueryDataType


class MsprofEntrance:
    """
    entrance of msprof
    """

    FILE_NAME = os.path.basename(__file__)

    @staticmethod
    def _add_collect_path_argument(parser: any) -> None:
        parser.add_argument(
            '-dir',
            '--collection-dir',
            dest='collection_path',
            default='',
            metavar='<dir>',
            type=MsprofEntrance._expanduser_for_argument_path,
            help='<Mandatory> Specify the directory that is used for creating data collection results.',
            required=True,
        )

    @staticmethod
    def _add_reports_argument(parser: any) -> None:
        parser.add_argument(
            '-reports',
            dest='reports_path',
            default='',
            metavar='reports',
            type=MsprofEntrance._expanduser_for_argument_path,
            help='<Optional> Path of the reports JSON configuration'
            ' file, which is used to control the export scope of collection results.',
            required=False,
        )

    @staticmethod
    def _handle_export_command(parser: any, args: any) -> None:
        if len(sys.argv) < 3:
            parser.print_help()
            raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        export_command = ExportCommand(sys.argv[2], args)
        export_command.process()

    @staticmethod
    def _handle_query_command(parser: any, args: any) -> None:
        _ = parser
        query_command = QueryCommand(args)
        query_command.process()

    @staticmethod
    def _handle_import_command(parser: any, args: any) -> None:
        _ = parser
        import_command = ImportCommand(args)
        import_command.process()

    @staticmethod
    def _handle_analyze_command(parser: any, args: any) -> None:
        _ = parser
        analyze_command = AnalyzeCommand(args)
        analyze_command.process()

    @staticmethod
    def _set_export_mode(args: any) -> None:
        if args.model_id is not None and args.iteration_id is not None:
            if args.model_id == NumberConstant.INVALID_MODEL_ID:
                # model_id==4294967295是按step导出
                ProfilingScene().set_mode(ExportMode.STEP_EXPORT)
            else:
                # 按子图导出
                ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)

    @staticmethod
    def _validate_analyze_rule(value: str):
        elements = value.split(",")
        valid_elements = ['communication', 'communication_matrix']  # 后续可自行添加有效元素
        if not all(element in valid_elements for element in elements):
            raise argparse.ArgumentTypeError("Invalid elements in rule.")
        return value

    @staticmethod
    def _expanduser_for_argument_path(str_name: str):
        return os.path.expanduser(str_name.lstrip('='))

    def main(self: any) -> None:
        """
        parse argument and run command
        :return: None
        """
        parser, export_parser, import_parser, query_parser, analyze_parser = self.construct_arg_parser()

        args = parser.parse_args(sys.argv[1:])
        if len(sys.argv) < 2 or not hasattr(args, "collection_path"):
            parser.print_help()
            call_sys_exit(ProfException.PROF_INVALID_PARAM_ERROR)
        try:
            check_path_char_valid(args.collection_path)
            check_path_valid(args.collection_path, False)
        except ProfException as err:
            if err.message:
                error(self.FILE_NAME, str(err))
            call_sys_exit(err.code)
        # when setting 'iteration-id' and 'model-id' args, export one iteration in one model
        if sys.argv[1] == 'export' and hasattr(args, "model_id") and hasattr(args, "iteration_id"):
            self._set_export_mode(args)
            # 'iteration-id' and 'model-id' must be set simultaneously
            if (args.model_id is None) ^ (args.iteration_id is None):
                error(
                    self.FILE_NAME,
                    "Please set 'model-id' and 'iteration-id' simultaneously "
                    "(recommend using 'query' to obtain proper 'model-id' and 'iteration-id' values).",
                )
                call_sys_exit(ProfException.PROF_INVALID_PARAM_ERROR)

        command_handler = {
            'export': {'parser': export_parser, 'handler': self._handle_export_command},
            'query': {'parser': query_parser, 'handler': self._handle_query_command},
            'import': {'parser': import_parser, 'handler': self._handle_import_command},
            'analyze': {'parser': analyze_parser, 'handler': self._handle_analyze_command},
        }
        handler = command_handler.get(sys.argv[1])

        # parsing data with analysis should inform user of security warning
        if is_root_user():
            warn(
                self.FILE_NAME,
                "Msprof analysis is parsing data as root, which may cause potential system security risks.",
            )
        try:
            handler.get('handler')(handler.get('parser'), args)
        except ProfException as err:
            if err.message:
                error(self.FILE_NAME, err)
            call_sys_exit(err.code)
        except Exception as err:
            error(self.FILE_NAME, err)
            call_sys_exit(NumberConstant.ERROR)
        call_sys_exit(ProfException.PROF_NONE_ERROR)

    def construct_arg_parser(self: any) -> tuple:
        """
        construct arg parser
        :return: tuple of parsers
        """
        parser = argparse.ArgumentParser()
        subparsers = parser.add_subparsers()
        import_parser = subparsers.add_parser('import', help='Parse original profiling data by collected data.')
        export_parser = subparsers.add_parser('export', help='Export profiling data by collected data.')
        query_parser = subparsers.add_parser('query', help='Query specified info.')
        analyzer_parser = subparsers.add_parser(
            'analyze', help='Analyze parsed profiling data and generate analysis report.'
        )
        self._query_parser(query_parser)
        self._export_parser(export_parser)
        self._import_parser(import_parser)
        self._analyze_parser(analyzer_parser)
        parser_tuple = (parser, export_parser, import_parser, query_parser, analyzer_parser)
        return parser_tuple

    def _query_parser(self: any, query_parser: any) -> None:
        data_type_values = list(map(int, QueryDataType))
        data_type_tips = ", ".join(map(str, data_type_values))
        self._add_collect_path_argument(query_parser)
        query_parser.add_argument(
            '--id', dest='id', default=None, metavar='<id>', type=int, help='<Optional> the npu device ID'
        )
        query_parser.add_argument(
            '--data-type',
            dest='data_type',
            default=None,
            metavar='<data_type>',
            type=int,
            choices=data_type_values,
            help='<Optional> the data type to query, support {}.'.format(data_type_tips),
        )
        query_parser.add_argument(
            '--model-id', dest='model_id', default=None, metavar='<model_id>', type=int, help='<Optional> the model ID'
        )
        query_parser.add_argument(
            '--iteration-id',
            dest='iteration_id',
            default=None,
            metavar='<iteration_id>',
            type=int,
            help='<Optional> the iteration ID',
        )

    def _add_export_argument(self: any, parser: any) -> None:
        self._add_collect_path_argument(parser)
        parser.add_argument(
            '--iteration-id',
            dest='iteration_id',
            default=None,
            metavar='<iteration_id>',
            type=int,
            help='<Optional> the iteration ID',
        )
        parser.add_argument(
            '--model-id', dest='model_id', default=None, metavar='<model_id>', type=int, help='<Optional> the model ID'
        )
        parser.add_argument(
            '--iteration-count',
            dest='iteration_count',
            default=1,
            metavar='<iteration_count>',
            type=int,
            help='<Optional> the number of iterations exported',
        )
        parser.add_argument(
            '--clear', dest='clear_mode', action='store_true', default=False, help='<Optional> the clear mode flag'
        )

    def _export_parser(self: any, export_parser: any) -> None:
        subparsers = export_parser.add_subparsers()
        summary_parser = subparsers.add_parser('summary', help='Get summary data.')
        timeline_parser = subparsers.add_parser('timeline', help='Get timeline data.')
        self._add_export_argument(summary_parser)
        summary_parser.add_argument(
            '--format',
            dest='export_format',
            default='csv',
            metavar='<export_format>',
            choices=['csv', 'json'],
            type=str,
            help='<Optional> the format for export, supports csv and json.',
        )
        self._add_export_argument(timeline_parser)
        self._add_reports_argument(timeline_parser)
        db_parser = subparsers.add_parser('db', help='Get db data.')
        self._add_collect_path_argument(db_parser)

    def _import_parser(self: any, import_parser: any) -> None:
        self._add_collect_path_argument(import_parser)
        import_parser.add_argument(
            '--cluster',
            dest='cluster_flag',
            action='store_true',
            default=False,
            help='<Optional> the cluster scene flag',
        )

    def _analyze_parser(self: any, analyze_parser: any) -> None:
        self._add_collect_path_argument(analyze_parser)
        analyze_parser.add_argument(
            '--rule',
            '-r',
            type=self._validate_analyze_rule,
            required=True,
            help='Switch specified rule for using msprof to analyze collecting data. '
            'The options are: [communication, communication_matrix], they can be set at the same time '
            'and separated by a comma (,), for example, :--rule=communication,communication_matrix.',
        )
        analyze_parser.add_argument(
            '--clear', dest='clear_mode', action='store_true', default=False, help='<Optional> the clear mode flag'
        )
        analyze_parser.add_argument(
            '--type',
            dest='export_type',
            type=str,
            help='Specify the output file type, db or text',
            required=False,
            default="text",
            choices=['db', 'text'],
        )
