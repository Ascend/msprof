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

from msconfig.meta_config import MetaConfig


class TablesOperatorConfig(MetaConfig):
    DATA = {
        'TaskTimeMap': [
            ('replayid', 'INTEGER,null'),
            ('device_id', 'INTEGER,null'),
            ('api', 'INTEGER,null'),
            ('apirowid', 'INTEGER,null'),
            ('tasktype', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('waittime', 'NUMERIC,null'),
            ('pendingtime', 'NUMERIC,null'),
            ('running', 'NUMERIC,null'),
            ('complete', 'NUMERIC,null'),
            ('index_id', 'INTEGER,null'),
            ('model_id', 'INTEGER,null'),
            ('batch_id', 'INTEGER,null'),
        ],
        'ReportTaskMap': [
            ('timeratio', 'REAL,null'),
            ('time', 'REAL,null'),
            ('count', 'INTEGER,null'),
            ('avg', 'REAL,null'),
            ('min', 'REAL,null'),
            ('max', 'REAL,null'),
            ('waiting', 'REAL,null'),
            ('running', 'REAL,null'),
            ('pending', 'REAL,null'),
            ('type', 'TEXT,null'),
            ('api', 'TEXT,null'),
            ('task_id', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('device_id', 'INTEGER,null'),
            ('batch_id', 'INTEGER,null'),
        ],
        'ModifiedTaskTimeMap': [
            ('task_id', 'INTEGER, null'),
            ('stream_id', 'INTEGER, null'),
            ('start_time', 'INTEGER,null'),
            ('duration_time', 'INTEGER, null'),
            ('wait_time', 'INTEGER, null'),
            ('task_type', 'INTEGER,null'),
            ('index_id', 'INTEGER,null'),
            ('model_id', 'INTEGER, null'),
            ('batch_id', 'INTEGER,null'),
            ('subtask_id', 'INTEGER,null'),
        ],
        'GeMergeMap': [
            ('model_id', 'INTEGER,null'),
            ('op_name', 'text,null'),
            ('op_type', 'text,null'),
            ('task_type', 'text,null'),
            ('task_id', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('batch_id', 'INTEGER,null'),
            ('context_id', 'INTEGER,null'),
        ],
        'RtsTaskMap': [
            ('task_id', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('start_time', 'INTEGER,null'),
            ('duration', 'INTEGER,null'),
            ('task_type', 'text,null'),
            ('index_id', 'INTEGER,null'),
            ('model_id', 'INTEGER,null'),
            ('batch_id', 'INTEGER,null'),
            ('subtask_id', 'INTEGER,null'),
        ],
        'OpReportMap': [
            ('op_type', 'text,null'),
            ('core_type', 'text,null'),
            ('occurrences', 'text,null'),
            ('total_time', 'REAL,null'),
            ('min', 'REAL,null'),
            ('avg', 'REAL,null'),
            ('max', 'REAL,null'),
            ('ratio', 'text,null'),
        ],
        'SummaryGeMap': [
            ('model_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('op_name', 'text,null'),
            ('op_type', 'text,null'),
            ('op_state', 'text,null'),
            ('block_num', 'INTEGER,null'),
            ('mix_block_num', 'INTEGER,null'),
            ('task_type', 'text,null'),
            ('tensor_num', 'INTEGER,null'),
            ('input_formats', 'TEXT,null'),
            ('input_data_types', 'TEXT,null'),
            ('input_shapes', 'TEXT,null'),
            ('output_formats', 'TEXT,null'),
            ('output_data_types', 'TEXT,null'),
            ('output_shapes', 'TEXT,null'),
            ('index_id', 'INTEGER,null'),
            ('timestamp', 'TEXT,null'),
            ('batch_id', 'INTEGER,null'),
            ('context_id', 'INTEGER,null'),
            ('op_flag', 'TEXT,null'),
        ],
    }
