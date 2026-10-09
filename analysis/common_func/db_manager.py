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
import os
import sqlite3

from common_func.constant import Constant
from common_func.empty_class import EmptyClass
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_exception import ProfException
from common_func.msprof_object import CustomizedNamedtupleFactory
from common_func.msvp_common import path_check
from common_func.file_manager import check_db_path_valid
from common_func.path_manager import PathManager
from msconfig.config_manager import ConfigManager
from profiling_bean.db_dto.database_to_object import DataBaseToObject


class ClassRowType:
    """
    class row type for sqlite query type
    """

    @staticmethod
    def class_row(bean_class):
        """
        define the type with bean class
        :param bean_class: Bean Class
        :return: bean type
        """

        def row_factory(cursor, row):
            """
            trans the sqlite type for bean data
            :param cursor: sqlite cursor
            :param row: pear data
            :return: bean object
            """
            bean_obj = bean_class()
            for col_idx, col in enumerate(cursor.description):
                if hasattr(bean_obj, col[0]):
                    setattr(bean_obj, col[0], row[col_idx])
            return bean_obj

        return row_factory

    @staticmethod
    def create_object(cursor: any, row: any) -> any:
        """
        trans the sqlite type for bean data
        :param cursor: sqlite cursor
        :param row: pear data
        :return: bean object
        """
        bean_obj = DataBaseToObject()
        for col_idx, col in enumerate(cursor.description):
            setattr(bean_obj, col[0], row[col_idx])
        return bean_obj


class DBManager:
    """
    class to manage DB operation
    """

    FETCH_SIZE = 10000
    INSERT_SIZE = 10000
    TENNSTONS = 10
    NSTOUS = 1000
    MAX_ROW_COUNT = 100000000

    @staticmethod
    def create_connect_db(db_path: str) -> tuple:
        """
        create and connect database
        """
        check_db_path_valid(db_path, True)
        try:
            conn = sqlite3.connect(db_path)
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return EmptyClass("empty conn"), EmptyClass("empty curs")

        try:
            if isinstance(conn, sqlite3.Connection):
                curs = conn.cursor()
                os.chmod(db_path, NumberConstant.FILE_AUTHORITY)
                return conn, curs
        except sqlite3.Error:
            return EmptyClass("empty conn"), EmptyClass("empty curs")
        return EmptyClass("empty conn"), EmptyClass("empty curs")

    @staticmethod
    def destroy_db_connect(conn: any, cur: any) -> None:
        """
        destroy the db connect
        """
        try:
            if isinstance(cur, sqlite3.Cursor):
                cur.close()
        except sqlite3.Error as error:
            logging.error(str(error), exc_info=Constant.TRACE_BACK_SWITCH)

        try:
            if isinstance(conn, sqlite3.Connection):
                conn.close()
        except sqlite3.Error as error:
            logging.error(str(error), exc_info=Constant.TRACE_BACK_SWITCH)

    @staticmethod
    def judge_table_exist(cursor: any, table_name: str) -> any:
        """
        judge table exist
        """
        if not isinstance(cursor, sqlite3.Cursor):
            return False
        try:
            cursor.execute("select count(*) from sqlite_master where type='table' and name=?", (table_name,))
            return cursor.fetchone()[0]
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return False

    @staticmethod
    def judge_index_exist(cursor: any, table_name: str) -> int:
        """
        judge index exist
        """
        if isinstance(cursor, sqlite3.Cursor):
            return cursor.execute(
                "select count(*) from sqlite_master where type='index' and name='{}'".format(table_name)
            ).fetchone()[0]
        return 0

    @staticmethod
    def judge_row_exist(cursor: any, table_name: str) -> bool:
        """
        judge row exist
        """
        if isinstance(cursor, sqlite3.Cursor):
            if not DBManager.fetch_one_data(cursor, "select * from {}".format(table_name)):
                return False
            return True
        return False

    @staticmethod
    def sql_create_table_with_key(
        map_name: str, table_name: str, map_file_path: str, key_list: any = None, fields_filter_list: list = None
    ) -> str:
        """
        Create a sqlite table with primary key
        """
        sql = DBManager.sql_create_general_table(map_name, table_name, map_file_path, fields_filter_list)
        try:
            if key_list:
                key_list_str = "(" + ",".join(key_list) + "))"
                sql = sql[:-1] + ", PRIMARY KEY" + key_list_str
            return sql
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return ""
        finally:
            pass

    @staticmethod
    def get_table_field_num(map_name: str, map_file_path: str) -> int:
        """
        get table field number
        :param map_name: map table name in table.ini
        :param map_file_path: map file path
        :return: success: table field number，otherwise return 0
        """
        num = NumberConstant.DEFAULT_TABLE_FIELD_NUM
        cfg_parser = ConfigManager.get(map_file_path)
        if cfg_parser.has_section(map_name):
            items = cfg_parser.items(map_name)
            num = len(items)
        return num

    @staticmethod
    def execute_sql(conn: any, sql: str, param: any = None) -> bool:
        """
        execute sql
        """
        if not isinstance(conn, (sqlite3.Connection, sqlite3.Cursor)):
            logging.error("conn is invalid param: must be sqlite3.Connection or sqlite3.Cursor")
            return False
        try:
            cursor = conn.cursor() if isinstance(conn, sqlite3.Connection) else conn
            cursor.execute(sql, param or ())
            if isinstance(conn, sqlite3.Connection):
                conn.commit()
            return True
        except sqlite3.Error as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return False

    @staticmethod
    def executemany_sql(conn: any, sql: str, param: any) -> bool:
        """
        execute many sql once
        """
        try:
            if isinstance(conn, sqlite3.Connection):
                conn.cursor().executemany(sql, param)
                conn.commit()
                return True
        except sqlite3.Error as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return False
        logging.error("conn is invalid param")
        return False

    @staticmethod
    def _get_headers_and_type_names(
        cfg_parser: any, map_name: str, fields_filter_list: list, map_file_path: str
    ) -> tuple:
        headers, type_names = [], []
        if cfg_parser.has_section(map_name):
            items = cfg_parser.items(map_name)

            for item in items:
                if item[0] in fields_filter_list:
                    continue
                headers.append(item[0])
                if not item[1]:
                    item[1] = "TEXT"
                type_names.append(item[1].split(",")[0])
        else:
            msg = "{} does not exist in {}!".format(map_name, map_file_path)
            logging.error(msg)
        return headers, type_names

    @staticmethod
    def _get_hd_with_type_list_str(headers: list, type_names: list) -> str:
        hd_with_type_list_str = "("
        hd_with_type_list = []
        for i, header in enumerate(headers):
            hd_with_type_list.append(header + " " + type_names[i])
        hd_with_type_list_str += ', '.join(hd_with_type_list)
        hd_with_type_list_str += ")"
        return hd_with_type_list_str

    @classmethod
    def attach_to_db(cls: any, conn: any, project_path: str, db_name: str, attach_name: str) -> bool:
        """
        attach to other database
        :param conn:
        :param project_path:
        :param db_name:
        :param attach_name:
        :return:
        """
        db_path = PathManager.get_db_path(project_path, db_name)
        conn_check, curs_check = cls.check_connect_db_path(db_path)
        if isinstance(conn_check, sqlite3.Connection):
            cls.destroy_db_connect(conn_check, curs_check)
            cls.execute_sql(conn, "attach database '{0}' as {1}".format(db_path, attach_name))
            return True
        return False

    @classmethod
    def sql_create_general_table(
        cls: any, map_name: str, table_name: str, map_path: str, fields_filter_list: list = None
    ) -> str:
        """
        use table.ini to generate sql table
        :param map_name: map table name in table.ini
        :param table_name: table name
        :param map_path: map file
        :param fields_filter_list: filtered list
        :return: success: sql sentence，otherwise return None
        """

        cfg_parser = ConfigManager.get(map_path)
        if fields_filter_list is None:
            fields_filter_list = []
        try:
            headers, type_names = cls._get_headers_and_type_names(cfg_parser, map_name, fields_filter_list, map_path)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as error:
            logging.exception(error)
            return ""
        hd_with_type_list_str = cls._get_hd_with_type_list_str(headers, type_names)
        sql = "CREATE TABLE IF NOT EXISTS " + table_name + hd_with_type_list_str
        return sql

    @classmethod
    def drop_table(cls: any, conn: any, table: str) -> None:
        """
        drop existing table
        :param conn: DB connection
        :param table: ready to drop table
        :return:
        """
        if not isinstance(conn, sqlite3.Connection):
            logging.error("%s table does not exist", table)
        cls.execute_sql(conn, 'DROP TABLE IF EXISTS {}'.format(table))

    @classmethod
    def clear_table(cls: any, conn: any, table_name: str):
        """
        delete all data in one table
        :param conn: DB connection
        :param table_name: ready to delete table data
        :return:
        """
        if not isinstance(conn, sqlite3.Connection):
            logging.error("%s table does not exist", table_name)
        cls.execute_sql(conn, "Delete from {}".format(table_name))

    @classmethod
    def check_tables_in_db(cls: any, db_path: str, *tables: any) -> bool:
        """
        check if tables in database
        """
        conn, curs = cls.check_connect_db_path(db_path)
        if not (conn and curs):
            return False
        res = True
        for table in tables:
            if not cls.judge_table_exist(curs, table):
                res = False
                break
        cls.destroy_db_connect(conn, curs)
        return res

    @classmethod
    def check_item_in_table(cls: any, db_path: str, table_name: str, col: str, item: any):
        """
        check if item is in table
        """
        conn, curs = cls.check_connect_db_path(db_path)
        if not (conn and curs):
            return False

        sql = "select * from {table_name} where {col}='{item}'".format(table_name=table_name, col=col, item=item)
        try:
            data = cls.fetch_all_data(curs, sql)
        except sqlite3.Error as _err:
            logging.error(str(_err), exc_info=Constant.TRACE_BACK_SWITCH)
            return False
        finally:
            cls.destroy_db_connect(conn, curs)
        return len(data) != 0

    @classmethod
    def get_table_data_count(cls: any, db_path: str, table_name: str):
        """
        get data count in datable
        """
        conn, curs = cls.check_connect_db_path(db_path)
        if not (conn and curs):
            return 0

        sql = "select count(*) from {table_name}".format(table_name=table_name)
        data = []
        try:
            data = cls.fetch_all_data(curs, sql)
        except sqlite3.Error as _err:
            logging.error(str(_err), exc_info=Constant.TRACE_BACK_SWITCH)
            return 0
        finally:
            cls.destroy_db_connect(conn, curs)
        if len(data) == 0 or len(data[0]) == 0:
            return 0
        return data[0][0]

    @classmethod
    def check_no_empty_tables_in_db(cls: any, db_path: str, *tables: any) -> bool:
        """
        check if tables are not empty in database
        """
        conn, curs = cls.check_connect_db_path(db_path)
        if not (conn and curs):
            return False
        res = True
        for table in tables:
            if not cls.judge_row_exist(curs, table):
                res = False
                break
        cls.destroy_db_connect(conn, curs)
        return res

    @classmethod
    def get_table_info(cls: any, curs: any, table_name: str) -> dict:
        """
        get table column name and type dictionary
        """
        table_info = cls.fetch_all_data(curs, "PRAGMA table_info({})".format(table_name))
        return {col_info[1]: col_info[2] for col_info in table_info}

    @classmethod
    def get_table_headers(cls: any, curs: any, table_name: str) -> list:
        """
        get all headers for certain table
        """
        table_headers = []
        for col_info in cls.fetch_all_data(curs, "PRAGMA table_info({})".format(table_name)):
            table_headers.append(col_info[1])
        return table_headers

    @classmethod
    def get_filtered_table_headers(cls: any, curs: any, table_name: str, *unused_headers: any) -> list:
        """
        get all headers for certain table
        """
        all_headers = cls.get_table_headers(curs, table_name)
        filtered_table_headers = []
        for sub in all_headers:
            if sub not in unused_headers:
                filtered_table_headers.append(sub)
        return filtered_table_headers

    @classmethod
    def insert_data_into_table(cls: any, conn: any, table_name: str, data: any) -> None:
        """
        insert data into certain table
        """
        index = 0
        if not data:
            return
        sql = "insert into {table_name} values({value_form})".format(
            table_name=table_name, value_form="?," * (len(data[0]) - 1) + "?"
        )
        while index < len(data):
            cls.executemany_sql(conn, sql, data[index : index + cls.INSERT_SIZE])
            index += cls.INSERT_SIZE

    @classmethod
    def fetch_all_data(cls: any, curs: any, sql: str, param: tuple = None, dto_class: any = None) -> list:
        """
        fetch 10000 num of data each time to get all data
        """
        if not isinstance(curs, sqlite3.Cursor):
            return []
        data = []
        try:
            if param:
                res = curs.execute(sql, param)
            else:
                res = curs.execute(sql)
        except sqlite3.Error as _err:
            logging.error("%s", str(_err), exc_info=Constant.TRACE_BACK_SWITCH)
            logging.debug("%s, sql: %s", str(_err), sql, exc_info=Constant.TRACE_BACK_SWITCH)
            curs.row_factory = None
            return []
        try:
            if dto_class:
                tuple_dto = CustomizedNamedtupleFactory.generate_named_tuple_from_dto(dto_class, res.description)
            while True:
                res = curs.fetchmany(cls.FETCH_SIZE)
                if dto_class:
                    data += [tuple_dto(*i) for i in res]
                else:
                    data += res
                if len(data) > cls.MAX_ROW_COUNT:
                    logging.error(
                        "Please check the record counts in %s's table",
                        os.path.basename(curs.execute("PRAGMA database_list;").fetchone()[-1]),
                    )
                    message = "The record counts in table exceed the limit!"
                    raise ProfException(ProfException.PROF_DB_RECORD_EXCEED_LIMIT, message)
                if len(res) < cls.FETCH_SIZE:
                    break
            return data
        except sqlite3.Error as _err:
            logging.error(str(_err), exc_info=Constant.TRACE_BACK_SWITCH)
            return []
        finally:
            curs.row_factory = None

    @classmethod
    def fetchone(cls: any, curs: any, sql: str, param: tuple = None, dto_class: any = None) -> any:
        """
        fetch one data
        """
        if not isinstance(curs, sqlite3.Cursor):
            return EmptyClass()
        if dto_class:
            curs.row_factory = ClassRowType.class_row(dto_class)
        try:
            if param:
                data = curs.execute(sql, param).fetchone()
            else:
                data = curs.execute(sql).fetchone()
            return data
        except sqlite3.Error as _err:
            logging.error(str(_err), exc_info=Constant.TRACE_BACK_SWITCH)
            return EmptyClass()
        finally:
            curs.row_factory = None

    @classmethod
    def fetch_one_data(cls: any, curs: any, sql: str, param: tuple = None, dto_class: any = None) -> any:
        """
        fetch the next row query result set as a list
        :return: one dto_class instance if dto_class, else one tuple
        """
        if not isinstance(curs, sqlite3.Cursor):
            return tuple()
        if dto_class:
            curs.row_factory = ClassRowType.class_row(dto_class)
        try:
            if param:
                curs.execute(sql, param)
            else:
                curs.execute(sql)
        except sqlite3.Error as _err:
            logging.error(str(_err), exc_info=Constant.TRACE_BACK_SWITCH)
            return tuple()
        except OverflowError as _err:
            logging.error(str(_err), exc_info=Constant.TRACE_BACK_SWITCH)
            return tuple()
        try:
            res = curs.fetchone()
        except sqlite3.Error as _err:
            logging.error(str(_err), exc_info=Constant.TRACE_BACK_SWITCH)
            return tuple()
        finally:
            curs.row_factory = None
        return res if res else tuple()

    @classmethod
    def add_new_column(cls: any, *args: str) -> None:
        """
        add a new column to certain table with default value
        """
        if len(args) < 5:
            return
        db_path = args[0]
        table_name = args[1]
        col_name = args[2]
        col_type = args[3]
        default_value = args[4]
        conn, curs = cls.check_connect_db_path(db_path)
        cls.execute_sql(
            conn,
            "alter table {table} add column {name} {type} default {default}".format(
                table=table_name, name=col_name, type=col_type, default=default_value
            ),
        )

    @classmethod
    def check_connect_db(cls: any, project_path: str, db_name: str) -> tuple:
        """
        check whether we are able to connect to the desired DB
        """
        project_path = path_check(project_path)
        if project_path:
            db_path = path_check(PathManager.get_db_path(project_path, db_name))
            if db_path:
                return cls.check_connect_db_path(db_path)
            return EmptyClass("empty conn"), EmptyClass("empty curs")
        return EmptyClass("empty conn"), EmptyClass("empty curs")

    @classmethod
    def check_connect_db_path(cls: any, db_path: str) -> tuple:
        """
        check whether we are able to connect to the desired DB
        """
        if path_check(db_path):
            check_db_path_valid(db_path)
            return cls.create_connect_db(db_path)
        return EmptyClass("empty conn"), EmptyClass("empty curs")
