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
import sqlite3
import stat


class DBOpen:
    def __init__(
        self,
        db_name,
        sqlite_dir=None,
    ):
        self._sqlite_path = os.path.dirname(os.path.abspath(__file__)) if sqlite_dir is None else sqlite_dir
        self._db_name = db_name
        self._db_path = os.path.join(self._sqlite_path, self._db_name)
        self._conn = None
        self._curs = None

    def __enter__(self):
        self._conn, self._curs = self._connect_db()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._destroy()

    def _connect_db(self):
        try:
            self._conn = sqlite3.connect(self._db_path)
            self._curs = self._conn.cursor()
            return self._conn, self._curs
        except (
            OSError,
            SystemError,
            ValueError,
            TypeError,
            RuntimeError,
            sqlite3.OperationalError,
            sqlite3.DatabaseError,
        ):
            return None, None

    @property
    def db_path(self):
        return self._db_path

    @property
    def db_conn(self):
        return self._conn

    @property
    def db_curs(self):
        return self._curs

    def create_table(self, create_sql):
        self._curs.execute(create_sql)
        self._conn.commit()

    def insert_data(self, table_name, data):
        insert_sql = "insert into {0} values ({value})".format(table_name, value="?," * (len(data[0]) - 1) + "?")
        self._curs.executemany(insert_sql, data)
        self._conn.commit()

    def _destroy(self):
        self._destroy_db_connect()
        self._destroy_db_file()

    def _destroy_db_connect(self):
        try:
            if isinstance(self._curs, sqlite3.Cursor):
                self._curs.close()
            if isinstance(self._conn, sqlite3.Connection):
                self._conn.close()
        except (
            OSError,
            SystemError,
            ValueError,
            TypeError,
            RuntimeError,
            sqlite3.OperationalError,
            sqlite3.DatabaseError,
        ):
            print("%s close failed" % self._db_path)

    def _destroy_db_file(self):
        if os.path.exists(self._db_path):
            os.remove(self._db_path)


class DBManager:
    def __init__(self):
        self.db_path, _ = os.path.split(os.path.abspath(__file__))
        self.conn = None
        self.curs = None
        self.db_name = None

    def create_sql(self, db_name):
        try:
            self.db_name = os.path.join(self.db_path, db_name)
            self.conn = sqlite3.connect(self.db_name)
            if isinstance(self.conn, sqlite3.Connection):
                self.curs = self.conn.cursor()
                os.chmod(self.db_path, stat.S_IRWXU + stat.S_IRWXG + stat.S_IRWXO)
                return self.conn, self.curs
            return None, None
        except (
            OSError,
            SystemError,
            ValueError,
            TypeError,
            RuntimeError,
            sqlite3.OperationalError,
            sqlite3.DatabaseError,
        ):
            return None, None

    def create_table(self, db_name, table='test', insert="", data=""):
        conn, curs = self.create_sql(db_name)
        if table and insert and data:
            curs.execute(table)
            self._insert_data(insert, data)
        return self.conn, self.curs

    def _insert_data(self, insert, data):
        self.curs.executemany(insert, data)
        self.conn.commit()

    def destroy(self, res):
        try:
            if isinstance(res[1], sqlite3.Cursor):
                res[1].close()
            if isinstance(res[0], sqlite3.Connection):
                res[0].close()
        except sqlite3.ProgrammingError:  # 重复close
            pass
        finally:
            if os.path.exists(self.db_name):
                os.remove(self.db_name)

    def clear_table(self, table_name):
        self.curs.execute("delete from {}".format(table_name))
        self.conn.commit()

    def attach(self, curs, file_name, db_name):
        curs.execute("attach database '{}' as {}".format(os.path.join(self.db_path, file_name), db_name))

    def connect_db(self, db_name):
        """check whether we are able to connect to the desired DB"""
        db_path = os.path.join(self.db_path, db_name)
        if not os.path.exists(db_path):
            return None, None
        try:
            conn = sqlite3.connect(db_path)
            curs = conn.cursor()
            return conn, curs
        except (
            OSError,
            SystemError,
            ValueError,
            TypeError,
            RuntimeError,
            sqlite3.OperationalError,
            sqlite3.DatabaseError,
        ):
            return None, None

    @staticmethod
    def insert_sql(table_name, data):
        insert = "insert into {0} values ({value})".format(table_name, value="?," * (len(data[0]) - 1) + "?")
        return insert


class ConnDemo:
    def __init__(self):
        pass

    def execute(self, sql_cmd):
        pass


class CursorDemo:
    def __init__(self):
        pass

    def execute(self, sql_cmd):
        pass
