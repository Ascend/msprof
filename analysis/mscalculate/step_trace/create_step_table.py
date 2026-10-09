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

import logging
import typing
from collections import deque

from common_func.ai_stack_data_check_manager import AiStackDataCheckManager
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from common_func.step_trace_constant import StepTraceConstant
from mscalculate.step_trace.tag_handler.tag_dispatch_handler import DispatchModelHandler
from profiling_bean.db_dto.step_trace_dto import StepTraceOriginDto
from msmodel.step_trace.ts_track_model import TsTrackModel


class CreateSubTable:
    """
    create sub table
    """

    data = []
    sample_config = {}
    db_name = None
    table_name = None

    @classmethod
    def extract_data(cls: any, collect_data: any) -> None:
        """
        extract data
        :param collect_data: collect data
        :return: void
        """

    @classmethod
    def create_table(cls: any, conn: any) -> None:
        """
        get table info
        :param conn: connect to database
        :return: create sql, insert sql
        """

    @classmethod
    def connect_db(cls: any) -> None:
        """
        connect and destroy database
        :return: void
        """
        db_path = PathManager.get_db_path(cls.sample_config.get("result_dir"), cls.db_name)
        conn, cur = DBManager.create_connect_db(db_path)
        cls.create_table(conn)
        DBManager.destroy_db_connect(conn, cur)

    @classmethod
    def run(cls: any, sample_config: dict, model_handler: any) -> None:
        """
        extract data and build table
        :param sample_config: sample config
        :param model_handler: model handler
        :return: void
        """
        collect_data = model_handler.get_data()
        cls.sample_config = sample_config
        try:
            cls.extract_data(collect_data)
        except ProfException as step_error:
            logging.error("Fail to create step trace sub table, error code: %s", step_error)
        cls.connect_db()


class CreateStepTraceData(CreateSubTable):
    """
    create step trace table
    step_trace_data存储每个子图迭代的开始结束时间
    """

    data = []
    sample_config = {}
    db_name = DBNameConstant.DB_STEP_TRACE
    table_name = DBNameConstant.TABLE_STEP_TRACE_DATA

    @classmethod
    def extract_data(cls: any, collect_data: any) -> None:
        for model_id, model_data in collect_data.items():
            for index_id, index_data in model_data.items():
                cls.data.append(
                    [
                        index_id,
                        model_id,
                        index_data.get(StepTraceConstant.STEP_START),
                        index_data.get(StepTraceConstant.STEP_END),
                    ]
                )

    @classmethod
    def create_table(cls: any, conn: any) -> None:
        # sorted data by step end. Index 3 is step end
        cls.data = sorted(cls.data, key=lambda x: x[3])

        # calculate iter id
        for index, datum in enumerate(cls.data):
            datum.append(index + 1)

        create_sql = (
            "create table if not exists {0} (index_id int, model_id int, "
            "step_start int, step_end int, iter_id int)".format(cls.table_name)
        )

        DBManager.execute_sql(conn, create_sql)

        if not cls.data:
            return

        insert_sql = 'insert into {0} values ({1})'.format(
            DBNameConstant.TABLE_STEP_TRACE_DATA, ",".join("?" * len(cls.data[0]))
        )
        DBManager.executemany_sql(conn, insert_sql, cls.data)


class CreateStepTime(CreateSubTable):
    """
    create StepTime table
    StepTime存储每个Step的开始结束时间
    """

    data = []
    sample_config = {}
    db_name = DBNameConstant.DB_STEP_TRACE
    table_name = DBNameConstant.TABLE_STEP_TIME

    @classmethod
    def extract_data(cls: any, collect_data: any = None) -> None:
        with TsTrackModel(
            cls.sample_config.get("result_dir"), DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_STEP_TRACE]
        ) as ts:
            step_data = ts.get_step_trace_with_tag([StepTraceConstant.STEP_START_TAG, StepTraceConstant.STEP_END_TAG])
        step_time = {}
        for data in step_data:
            step_time.setdefault(data.index_id, []).append(data)
        for index_id, data in step_time.items():
            if len(data) != 2:  # 有2个数据：step开始和step结束的打点
                logging.error("The step trace data is missing in step %d.", index_id)
                continue
            cls.data.append([index_id, data[0].model_id, data[0].timestamp, data[1].timestamp, index_id])

    @classmethod
    def create_table(cls: any, conn: any) -> None:
        # sorted by step end syscnt. Index 3 is step end syscnt
        if not cls.data:
            return

        cls.data = sorted(cls.data, key=lambda x: x[3])
        create_sql = (
            "create table if not exists {0} (index_id int, model_id int, "
            "step_start int, step_end int, iter_id int)".format(cls.table_name)
        )
        DBManager.execute_sql(conn, create_sql)

        insert_sql = 'insert into {0} values ({1})'.format(cls.table_name, ",".join("?" * len(cls.data[0])))
        DBManager.executemany_sql(conn, insert_sql, cls.data)


class CreateAllReduce(CreateSubTable):
    """
    create all reduce table
    """

    data = []
    sample_config = {}
    db_name = DBNameConstant.DB_TRACE
    table_name = DBNameConstant.TABLE_ALL_REDUCE

    @classmethod
    def extract_data(cls: any, collect_data: any) -> None:
        for model_id, model_data in collect_data.items():
            for index_id, index_data in model_data.items():
                for each_reduce in index_data[StepTraceConstant.ALL_REDUCE]:
                    cls.data.append(
                        [
                            cls.sample_config.get("devices"),
                            model_id,
                            index_id,
                            index_data[StepTraceConstant.STEP_END],
                            each_reduce[StepTraceConstant.REDUCE_START],
                            each_reduce[StepTraceConstant.REDUCE_END],
                        ]
                    )

    @classmethod
    def create_table(cls: any, conn: any) -> None:
        create_sql = (
            "create table if not exists {}"
            "(device_id int, model_id int, index_id int,"
            "iteration_end int, start int, end int, primary key(device_id,"
            "iteration_end, start))".format(DBNameConstant.TABLE_ALL_REDUCE)
        )

        DBManager.execute_sql(conn, create_sql)

        if not cls.data:
            return

        insert_sql = 'insert into {0} values ({1})'.format(
            DBNameConstant.TABLE_ALL_REDUCE, ",".join("?" * len(cls.data[0]))
        )

        DBManager.executemany_sql(conn, insert_sql, cls.data)


class CreateTrainingTrace(CreateSubTable):
    """
    create training trace table
    """

    data = []
    sample_config = {}
    db_name = DBNameConstant.DB_TRACE
    table_name = DBNameConstant.TABLE_TRAINING_TRACE

    @staticmethod
    def check_timestamp(model_id: int, index_id: int, step_start: int, step_end: int) -> None:
        """
        check timestamp
        :param model_id: model id
        :param index_id: index id
        :param step_start: starting timestamp of a iter
        :param step_end: ending timestamp of a iter
        :return:
        """
        if NumberConstant.NULL_NUMBER in (step_start, step_end):
            logging.error(
                "step time is None, step start: %d, step end: %d, model id: %d, index id: %d",
                step_start,
                step_end,
                model_id,
                index_id,
            )
            raise ProfException(ProfException.PROF_INVALID_STEP_TRACE_ERROR)

        if step_start == step_end:
            logging.error(
                "start time equals to end time, step start: %d, step end: %d, model id: %d, index id: %d",
                step_start,
                step_end,
                model_id,
                index_id,
            )
            raise ProfException(ProfException.PROF_INVALID_STEP_TRACE_ERROR)

    @classmethod
    def extract_data(cls: any, collect_data: any) -> None:
        for model_id, model_data in collect_data.items():
            for index_id, index_data in model_data.items():
                cls.update_data(model_id, index_id, index_data)

    @classmethod
    def create_table(cls: any, conn: any) -> None:
        cls.update_step_time()

        create_sql = (
            "create table if not exists {0} "
            "(device_id int, model_id int, iteration_id int, "
            "FP_start int, "
            "BP_end int, iteration_end int, "
            "iteration_time int, fp_bp_time int, grad_refresh_bound int, "
            "data_aug_bound int)".format(DBNameConstant.TABLE_TRAINING_TRACE)
        )

        DBManager.execute_sql(conn, create_sql)

        if not cls.data:
            return

        cls.update_data_aug()

        insert_sql = 'insert into {0} values ({1})'.format(
            DBNameConstant.TABLE_TRAINING_TRACE, ",".join("?" * len(cls.data[0]))
        )
        DBManager.executemany_sql(conn, insert_sql, cls.data)

    @classmethod
    def update_data(cls: any, model_id: int, index_id: int, index_data: dict) -> None:
        """
        update data
        :param model_id: model id
        :param index_id: index id
        :param index_data: index data
        :return:
        """
        training_trace = index_data.get(StepTraceConstant.TRAINING_TRACE, {})
        step_start = index_data.get(StepTraceConstant.STEP_START, NumberConstant.NULL_NUMBER)
        step_end = index_data.get(StepTraceConstant.STEP_END, NumberConstant.NULL_NUMBER)

        cls.check_timestamp(model_id, index_id, step_start, step_end)

        forward_pro = training_trace.get(StepTraceConstant.FORWARD_PROPAGATION, NumberConstant.NULL_NUMBER)

        back_pro = training_trace.get(StepTraceConstant.BACK_PROPAGATION, NumberConstant.NULL_NUMBER)

        iteration_time = step_end - step_start
        fp_bp_time = NumberConstant.NULL_NUMBER if not (forward_pro and back_pro) else back_pro - forward_pro
        grad_refresh_bound = NumberConstant.NULL_NUMBER if not back_pro else step_end - back_pro

        cls.data.append(
            [
                cls.sample_config.get("devices"),
                model_id,
                index_id,
                forward_pro,
                back_pro,
                step_end,
                iteration_time,
                fp_bp_time,
                grad_refresh_bound,
                NumberConstant.NULL_NUMBER,
            ]
        )

    @classmethod
    def update_step_time(cls: any):
        with TsTrackModel(
            cls.sample_config.get("result_dir"), DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_STEP_TIME]
        ) as ts:
            step_data = ts.get_step_trace_data(DBNameConstant.TABLE_STEP_TIME)
        for data in step_data:
            cls.data.append(
                [
                    cls.sample_config.get("devices"),
                    data.model_id,
                    data.index_id,
                    0,
                    0,
                    data.step_end,
                    data.step_end - data.step_start,
                    0,
                    0,
                    NumberConstant.NULL_NUMBER,
                ]
            )

    @classmethod
    def update_data_aug(cls: any) -> None:
        """
        update data aug
        :return: void
        """
        cls.data.sort(key=lambda datum: datum[NumberConstant.STEP_END])
        for current_iter_index, current_datum in enumerate(cls.data):
            if current_datum[NumberConstant.FORWARD_PROPAGATION]:
                last_iter_index = cls.__find_closest_step_end_index(current_iter_index)
                if last_iter_index >= 0:
                    current_datum[NumberConstant.DATA_AUG_BOUND] = (
                        current_datum[NumberConstant.FORWARD_PROPAGATION]
                        - cls.data[last_iter_index][NumberConstant.STEP_END]
                    )

    @classmethod
    def __find_closest_step_end_index(cls: any, current_iter_index: int) -> int:
        """
        find last iter index. Last iter is the closest to
        current iter, and its iter end is before fp of current iter
        :param current_iter_index: current_iter_index
        :return: void
        """
        last_iter_index = current_iter_index - 1

        while last_iter_index >= 0:
            if (
                cls.data[current_iter_index][NumberConstant.FORWARD_PROPAGATION]
                > cls.data[last_iter_index][NumberConstant.STEP_END]
            ):
                break
            last_iter_index = last_iter_index - 1

        if last_iter_index < current_iter_index - 1:
            logging.warning(
                "The last iter of the %s iter of total iters is not %s, but is %s",
                current_iter_index,
                current_iter_index - 1,
                last_iter_index,
            )

        return last_iter_index


class GetNextCreator(CreateSubTable):
    """
    create get_next table
    """

    data = []
    sample_config = {}
    db_name = DBNameConstant.DB_TRACE
    table_name = DBNameConstant.TABLE_GET_NEXT
    getnext_start = dict()
    getnext_end = dict()
    TIMESTAMP = "timestamp"

    @classmethod
    def extract_data(cls: typing.Any, collect_data: typing.Any) -> None:
        for model_data in collect_data.values():
            for index_data in model_data.values():
                cls.update_data(index_data)
        for model_id, model_data in cls.getnext_start.items():
            for index_id, index_data in model_data.items():
                for key, start_deque in index_data.items():
                    end_deque = cls.getnext_end.get(model_id, {}).get(index_id, {}).get(key, deque())
                    cls.match_getnext(start_deque, end_deque, model_id, index_id)

    @classmethod
    def match_getnext(cls: any, start_deque: deque, end_deque: deque, model_id: int, index_id: int) -> None:
        mismatch_count = 0
        while start_deque and end_deque:
            start_record = start_deque.popleft()
            end_record = end_deque.popleft()
            while end_deque and end_record.get(cls.TIMESTAMP, 0) < start_record.get(cls.TIMESTAMP, 0):
                # 保证 start time <= end time
                mismatch_count += 1
                end_record = end_deque.popleft()
            while start_deque and start_deque[0].get(cls.TIMESTAMP, 0) <= end_record.get(cls.TIMESTAMP, 0):
                # 保证 start time <= end time, 且start time最接近end time
                mismatch_count += 1
                start_record = start_deque.popleft()  # 下一个start record
            cls.data.append([model_id, index_id, start_record.get(cls.TIMESTAMP, 0), end_record.get(cls.TIMESTAMP, 0)])
        if start_deque or end_deque:
            logging.error(
                "The getnext mismatch happen with model_id: %d, index_id: %d, start len: %d, end len: %d.",
                model_id,
                index_id,
                len(start_deque),
                len(end_deque),
            )
        if mismatch_count > 0:
            logging.error("There are %d getnext mismatching.", mismatch_count)

    @classmethod
    def update_data(cls: typing.Any, index_data: typing.Dict) -> None:
        for key, records in index_data.get(StepTraceConstant.GET_NEXT, {}).items():
            # 一次迭代中可能会多次调用getnext来读取数据, 通过key来区分
            for record in records:
                model_id = record.get("model_id", 0)
                index_id = record.get("index_id", 1)
                stream_key = str(record.get("stream_id", 0)) + "_" + str(key)
                if record[StepTraceConstant.TAG_ID] % 2 == 0:
                    cls.getnext_start.setdefault(model_id, {}).setdefault(index_id, {}).setdefault(
                        stream_key, deque()
                    ).append(record)
                else:
                    cls.getnext_end.setdefault(model_id, {}).setdefault(index_id, {}).setdefault(
                        stream_key, deque()
                    ).append(record)

    @classmethod
    def create_table(cls: typing.Any, conn: typing.Any) -> None:
        create_sql = (
            "create table if not exists {0} "
            "(model_id INTEGER, index_id INTEGER, start_time INTEGER, end_time INTEGER)".format(cls.table_name)
        )
        DBManager.execute_sql(conn, create_sql)

        if not cls.data:
            return
        insert_sql = 'insert into {0} values ({1})'.format(cls.table_name, ",".join("?" * len(cls.data[0])))
        DBManager.executemany_sql(conn, insert_sql, cls.data)


class StepTableBuilder:
    """
    create table from step trace
    """

    TIMESTAMP_INDEX = 2
    model_handler = DispatchModelHandler()
    table_list = [CreateStepTraceData, CreateStepTime, CreateAllReduce, CreateTrainingTrace, GetNextCreator]
    step_conn = None
    step_curs = None

    @staticmethod
    def to_dict(record: list) -> dict:
        """
        transform list to dict
        :param record: contain "model_id", "tag_id", timestamp
        :return:
        """
        record_dict = {
            StepTraceConstant.INDEX_ID: record.index_id,
            StepTraceConstant.MODEL_ID: record.model_id,
            StepTraceConstant.TIME_STAMP: record.timestamp,
            StepTraceConstant.TAG_ID: record.tag_id,
            StepTraceConstant.STREAM_ID: record.stream_id,
        }

        return record_dict

    @classmethod
    def process_step_trace(cls: any, step_trace: list) -> None:
        """
        process step trace
        :param step_trace: contain model_id, tag_id, timestamp
        :return: void
        """
        step_trace.sort(key=lambda x: (x.model_id, x.timestamp))
        step_trace = StepTracePreProcess().reorder_step_trace_for_pipe_stage(step_trace)
        for record in step_trace:
            cls.model_handler.receive_record(cls.to_dict(record))

    @classmethod
    def build_table(cls: any, sample_config: dict) -> None:
        """
        build step trace data, all reduce, training trace table
        :param sample_config: sample config
        :return: void
        """
        if AiStackDataCheckManager.contain_training_trace_data(sample_config.get("result_dir")):
            cls.table_list.remove(CreateAllReduce)
            cls.table_list.remove(CreateTrainingTrace)
            cls.table_list.remove(GetNextCreator)

        for create_table in cls.table_list:
            create_table.run(sample_config, cls.model_handler)

    @classmethod
    def run(cls: any, sample_config: dict) -> None:
        """
        run class
        :param sample_config: sample config
        :return: void
        """
        cls._connect_step_db(sample_config)
        step_trace_data = cls._get_step_trace_data()
        if not step_trace_data:
            return
        cls.process_step_trace(step_trace_data)
        cls.build_table(sample_config)

    @classmethod
    def _get_step_data(cls: any, table_name: str, is_helper=False) -> list:
        if not DBManager.judge_table_exist(cls.step_curs, table_name):
            return []

        # iteration range table
        if is_helper:
            select_sql = "select DISTINCT index_id, model_id, timestamp, tag_id, 0 as stream_id from {}".format(
                table_name
            )
        else:
            select_sql = "select DISTINCT index_id, model_id, timestamp, tag_id, stream_id from {}".format(table_name)

        return DBManager.fetch_all_data(cls.step_curs, select_sql, dto_class=StepTraceOriginDto)

    @classmethod
    def _connect_step_db(cls: any, sample_config: dict) -> None:
        db_path = PathManager.get_db_path(sample_config.get("result_dir"), DBNameConstant.DB_STEP_TRACE)
        cls.step_conn, cls.step_curs = DBManager.check_connect_db_path(db_path)

    @classmethod
    def _get_step_trace_data(cls: any) -> list:
        _step_data = cls._get_step_data(DBNameConstant.TABLE_STEP_TRACE)
        _helper_data = cls._get_step_data(DBNameConstant.TABLE_MODEL_WITH_Q, is_helper=True)
        step_trace_data = _step_data + _helper_data
        if not step_trace_data:
            return []
        DBManager.destroy_db_connect(cls.step_conn, cls.step_curs)
        return step_trace_data


class StepTracePreProcess:
    MODEL_START_TAG = 0
    MODEL_END_TAG = 1
    MSTX_TAG = 11
    ITERATION_END_TAG = 4
    HCCL_START_TAG = 10000

    def __init__(self):
        self.reordered_step_trace = []
        self.current_model_id = None
        self.current_step_trace_queue = deque()

    def reorder_step_trace_for_pipe_stage(self: any, step_trace: list) -> list:
        for record in step_trace:
            if record.model_id != self.current_model_id:
                for data in self.current_step_trace_queue:
                    self.reordered_step_trace.extend(data["all_record"])
                self.current_step_trace_queue = deque()
                self.current_model_id = record.model_id
            if record.tag_id == self.MODEL_START_TAG:
                self.current_step_trace_queue.append({"tag": [record], "all_record": [record]})
            elif record.tag_id == self.MODEL_END_TAG:
                self.deal_model_end_tag(record)
            elif StepTraceConstant.GET_NEXT_START_TAG <= record.tag_id < StepTraceConstant.STEP_START_TAG:
                if self.current_step_trace_queue:
                    self.current_step_trace_queue[-1]["all_record"].append(record)
            elif record.tag_id >= self.HCCL_START_TAG:
                self.deal_hccl_tag(record)
            elif record.tag_id == self.MSTX_TAG:
                self.current_step_trace_queue.append({"tag": [record], "all_record": [record]})
            else:
                self.deal_iteration_tag(record)
        if self.current_step_trace_queue:
            for data in self.current_step_trace_queue:
                self.reordered_step_trace.extend(data.get("all_record", []))
        return self.reordered_step_trace

    def deal_model_end_tag(self, record: any):
        model_start_tag_num = 0
        while self.current_step_trace_queue:
            if self.current_step_trace_queue[0]["tag"][0].tag_id == self.MODEL_START_TAG:
                model_start_tag_num += 1
                if model_start_tag_num == 2:
                    break
            self.reordered_step_trace.extend(self.current_step_trace_queue.popleft()["all_record"])
        self.reordered_step_trace.append(record)

    def deal_hccl_tag(self, record: any):
        for data in self.current_step_trace_queue:
            if data["tag"][-1].tag_id != self.ITERATION_END_TAG:
                data["all_record"].append(record)
                break

    def deal_iteration_tag(self, record: any):
        is_new_iteration = True
        for data in self.current_step_trace_queue:
            if data["tag"][-1].tag_id < record.tag_id:
                data["tag"].append(record)
                data["all_record"].append(record)
                is_new_iteration = False
                break
        if is_new_iteration:
            self.current_step_trace_queue.append({"tag": [record], "all_record": [record]})
