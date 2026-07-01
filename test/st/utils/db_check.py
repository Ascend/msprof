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

import os
import logging
import sqlite3

logging.basicConfig(level=logging.INFO,
                    format='\n%(asctime)s %(filename)s [line:%(lineno)d] [%(levelname)s] %(message)s')


class EmptyClass:
    """
    Empty class
    """

    def __init__(self: any, info: str = "") -> None:
        self._info = info

    @classmethod
    def __bool__(cls: any) -> bool:
        return False

    @classmethod
    def __str__(cls: any) -> str:
        return ""

    @property
    def info(self: any) -> str:
        """
        get info
        :return: _info
        """
        return self._info

    @staticmethod
    def is_empty() -> bool:
        """
        return this is a empty class
        """
        return True


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
        try:
            conn = sqlite3.connect(db_path)
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=False)
            return EmptyClass("empty conn"), EmptyClass("empty curs")

        try:
            if isinstance(conn, sqlite3.Connection):
                curs = conn.cursor()
                os.chmod(db_path, 0o640)
                return conn, curs
        except sqlite3.Error:
            logging.info("conn sqlite error")
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
            logging.error(str(error), exc_info=False)

        try:
            if isinstance(conn, sqlite3.Connection):
                conn.close()
        except sqlite3.Error as error:
            logging.error(str(error), exc_info=False)

    @staticmethod
    def judge_table_exists(curs: any, table_name: str) -> any:
        """
        judge table exists
        """
        if not isinstance(curs, sqlite3.Cursor):
            return False
        try:
            curs.execute("select count(*) from sqlite_master where type='table' and name=?", (table_name,))
            return curs.fetchone()[0]
        except sqlite3.Error as err:
            logging.error(err)
            return False

    @classmethod
    def fetch_all_data(cls: any, curs: any, sql: str, param: tuple = None) -> list:
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
            logging.error("%s", str(_err), exc_info=False)
            logging.debug("%s, sql: %s", str(_err), sql, exc_info=False)
            curs.row_factory = None
            return []
        try:
            while True:
                res = curs.fetchmany(cls.FETCH_SIZE)
                data += res
                if len(data) > cls.MAX_ROW_COUNT:
                    logging.error("Please check the record counts in %s's table",
                                  os.path.basename(curs.execute("PRAGMA database_list;").fetchone()[-1]))

                if len(res) < cls.FETCH_SIZE:
                    break
            return data
        except sqlite3.Error as _err:
            logging.error(str(_err), exc_info=False)
            return []
        finally:
            curs.row_factory = None

    @classmethod
    def check_item_in_table(cls: any, db_path: str, table_name: str, col: str, item: any):
        """
        check if item is in table
        """
        conn, curs = DBManager.create_connect_db(db_path)
        if not (conn and curs):
            return False

        sql = "select * from {table_name} where {col}='{item}' limit 1".format(table_name=table_name,
                                                                               col=col, item=item)
        try:
            data = DBManager.fetch_all_data(curs, sql)
        except sqlite3.Error as _err:
            raise RuntimeError(f"Failed to fetch data, ERROR: {_err}")
        finally:
            DBManager.destroy_db_connect(conn, curs)
        if len(data) == 0:
            raise ValueError("Item is not in table.")

    @classmethod
    def fetch_all_field_name_in_table(cls: any, db_path: str, table_name: str) -> list:
        """
        check all field name in table
        """
        conn, curs = DBManager.create_connect_db(db_path)
        if not (conn and curs):
            return []

        sql = f"PRAGMA table_info({table_name})"
        try:
            data = DBManager.fetch_all_data(curs, sql)
            return [info[1] for info in data]
        except sqlite3.Error as _err:
            raise RuntimeError(f"Failed to fetch data, ERROR: {_err}")
        finally:
            DBManager.destroy_db_connect(conn, curs)

    @classmethod
    def check_tables_in_db(cls, db_path: any, *tables: any) -> bool:
        if os.path.exists(db_path):
            conn, curs = cls.create_connect_db(db_path)
            if not (conn and curs):
                return False
            res = True
            for table in tables:
                if not cls.judge_table_exists(curs, table):
                    res = False
                    break
            cls.destroy_db_connect(conn, curs)
            return res
        return False

    @classmethod
    def check_table_field_content(cls, db_path: str, table_field_content: dict) -> None:
        """
        Check if the specified fields in the tables contain all the required content.

        Args:
            db_path (str): Path to the SQLite database file.
            table_field_content (dict): Dictionary where keys are table names and values are dictionaries of field names and required content lists.

        Raises:
            AssertionError: If any required content is missing.

        Example:
            table_field_content = {
                "table1": {
                    "field1": ["value1", "value2", "value3"],
                    "field2": ["value4", "value5"]
                },
                "table2": {
                    "field3": ["value6", "value7"]
                }
            }
            FileChecker.check_table_field_content(db_path, table_field_content)
        """
        conn, curs = DBManager.create_connect_db(db_path)
        if not (conn and curs):
            return

        try:
            for table_name, field_content_dict in table_field_content.items():
                for field_name, required_content in field_content_dict.items():
                    sql = f"SELECT {field_name} FROM {table_name}"
                    data = DBManager.fetch_all_data(curs, sql)
                    actual_content = [row[0] for row in data]
                    missing_content = [content for content in required_content if content not in actual_content]
                    if missing_content:
                        raise ValueError(
                            f"Field '{field_name}' in table '{table_name}' in database '{db_path}' "
                            f"is missing required content: {missing_content}")
        except sqlite3.Error as _err:
            raise RuntimeError(f"Failed to fetch data, ERROR: {_err}")
        finally:
            DBManager.destroy_db_connect(conn, curs)

    @classmethod
    def check_table_field_values(cls, db_path: str, table_name: str, field_names: list, comparison_func) -> None:
        """
        Check if all values in the specified fields of the table meet the given condition.

        Args:
            db_path (str): Path to the SQLite database file.
            table_name (str): Name of the table to check.
            field_names (list): List of field names to check.
            comparison_func (callable): A function that takes a value and returns True if the value meets the condition, otherwise False.

        Raises:
            AssertionError: If any value in the fields does not meet the condition.

        Example:
            table_name = "ClusterCommunicationBandwidth"
            field_names = ["value1", "value2", "value3"]

            # Define the comparison function
            def is_greater_than_zero(value):
                return value > 0

            FileChecker.check_field_values(db_path, table_name, field_names, is_greater_than_zero)

        """
        conn, curs = DBManager.create_connect_db(db_path)
        if not (conn and curs):
            return

        try:
            # Construct the SQL query to select all specified fields
            fields_str = ", ".join(field_names)
            sql = f"SELECT {fields_str} FROM {table_name}"
            data = DBManager.fetch_all_data(curs, sql)

            # Check if all values in the specified fields meet the condition
            invalid_values = []
            for row in data:
                for value in row:
                    if not comparison_func(value):
                        invalid_values.append(value)

            if invalid_values:
                raise ValueError(
                    f"Fields '{', '.join(field_names)}' in table '{table_name}' in database '{db_path}' "
                    f"contain values that do not meet the condition: {invalid_values}")
        except sqlite3.Error as _err:
            raise RuntimeError(f"Failed to fetch data, ERROR: {_err}")
        finally:
            DBManager.destroy_db_connect(conn, curs)

    @classmethod
    def check_table_field_expected_values(
        cls,
        db_path: str,
        table_name: str,
        field_name: str,
        expected_values: list,
        where_clause: str = None,
        allow_extra: bool = False,
    ) -> None:
        """
        Check whether the distinct values of the specified field match expectations.

        Args:
            db_path (str): Path to the SQLite database file.
            table_name (str): Name of the table to check.
            field_name (str): Field name to check.
            expected_values (list): Expected distinct values.
            where_clause (str, optional): Optional SQL WHERE clause without the ``WHERE`` keyword.
            allow_extra (bool, optional): If True, only require expected values to be a subset
                of actual values. If False, require exact set equality.
        """
        conn, curs = DBManager.create_connect_db(db_path)
        if not (conn and curs):
            return

        try:
            sql = f"SELECT DISTINCT {field_name} FROM {table_name}"
            if where_clause:
                sql += f" WHERE {where_clause}"
            data = DBManager.fetch_all_data(curs, sql)
            actual_values = [row[0] for row in data]
            actual_set = set(actual_values)
            expected_set = set(expected_values)

            if allow_extra:
                missing_values = expected_set - actual_set
                if missing_values:
                    raise ValueError(
                        f"Field '{field_name}' in table '{table_name}' in database '{db_path}' "
                        f"is missing expected values: {sorted(missing_values)}. Actual values: {sorted(actual_set)}"
                    )
            elif actual_set != expected_set:
                raise ValueError(
                    f"Field '{field_name}' in table '{table_name}' in database '{db_path}' "
                    f"does not match expected values. Expected: {sorted(expected_set)}, Actual: {sorted(actual_set)}"
                )
        except sqlite3.Error as _err:
            raise RuntimeError(f"Failed to fetch data, ERROR: {_err}")
        finally:
            DBManager.destroy_db_connect(conn, curs)
