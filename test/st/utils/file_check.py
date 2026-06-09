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
import re
import csv
import json
from utils.db_check import DBManager
import pandas as pd
from bs4 import BeautifulSoup
from typing import Iterable, Optional, Callable


class FileChecker:
    @classmethod
    def check_file_exists(cls, file_path: str) -> None:
        """
        Check if a file whether exist.

        Args:
            file_path (str): file path to check.
        """
        if not (os.path.exists(file_path) and os.path.isfile(file_path)):
            raise FileNotFoundError(f"The {file_path} does not exist")

    @classmethod
    def check_file_has_content(cls, file_path: str, min_lines: int = 1) -> None:
        """
        Generic method to check if any text file has content.

        Args:
            file_path (str): Path to the file.
            min_lines (int): Minimum number of non-empty lines required.
        """
        try:
            cls.check_file_exists(file_path)
            with open(file_path, "r", encoding="utf-8") as file:
                non_empty_lines = [line for line in file if line.strip()]
                if len(non_empty_lines) < min_lines:
                    raise ValueError(f"The file {file_path} has less than {min_lines} non-empty lines.")

        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to read file {file_path}, ERROR: {e}")

    @classmethod
    def check_txt_not_empty(cls, txt_path: str) -> None:
        """
        Check if a TXT file is not empty.

        Args:
            txt_path (str): Path to the TXT file.
        """
        try:
            cls.check_file_exists(txt_path)
            with open(txt_path, "r", encoding="utf-8") as txtfile:
                content = txtfile.read()
                if not content:
                    raise ValueError(f"The file {txt_path} is empty.")
        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to read TXT file, ERROR: {e}")

    @classmethod
    def check_csv_headers(cls, csv_path: str, headers: list) -> None:
        """
        Check if the headers of a CSV file match the given headers list.

        Args:
            csv_path (str): Path to the CSV file.
            headers (list): List of expected headers.

        Example:
            To verify that a CSV file contains the headers "Op Name" and "Op Type", you can call the method like this:
            FileChecker.check_csv_headers(csv_path, ["Op Name", "Op Type"])
        """
        try:
            cls.check_file_exists(csv_path)
            with open(csv_path, newline="", encoding="utf-8") as csvfile:
                reader = csv.reader(csvfile)
                first_row = next(reader)  # Get the first row
                csv_headers_set = set(first_row)
                expected_headers_set = set(headers)
                if not expected_headers_set.issubset(csv_headers_set):
                    raise ValueError(f"{csv_path} Missing headers: {expected_headers_set - csv_headers_set}")
        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to read CSV file, ERROR: {e}")

    @classmethod
    def check_csv_items(cls, csv_path: str, item_pattern: dict, fuzzy_match: bool = True) -> None:
        """
        Check if items in specified columns of a CSV file match given patterns, including fuzzy match

        Args:
            csv_path (str): Path to the CSV file.
            item_pattern (dict): Dictionary containing patterns.
            fuzzy_match (bool, optional): Whether to enable fuzzy matching using regex. Defaults to True.

        Example:
            Given a CSV file with the following content:
            Op Name                        Op Type
            aclnnAdd_AddAiCore_Add         Add
            aclnnMul_MulAiCore_Mul         Mul

            Use the following call to match patterns:
            FileChecker.check_csv_items(csv_path, {"Op Name": ["*Add*", "*Mul*"], "Op Type": "Add"}, fuzzy_match=True)
            This will match because "Op Name" contains "Add" and "Mul", and "Op Type" contains "Add"
        """
        try:
            cls.check_file_exists(csv_path)
            cls.check_csv_headers(csv_path, list(item_pattern.keys()))
            with open(csv_path, "r", newline="", encoding="utf-8") as csvfile:
                reader = csv.DictReader(csvfile)
                csv_data = list(reader)
            for column, patterns in item_pattern.items():
                patterns = [patterns] if not isinstance(patterns, list) else patterns
                if fuzzy_match:
                    regex_patterns = [
                        re.compile(re.escape(pattern).replace(r"\*", ".*"), re.IGNORECASE) for pattern in patterns
                    ]
                    found_match = all(any(rp.search(row[column]) for row in csv_data) for rp in regex_patterns)
                else:
                    found_match = all(any(row[column] == pattern for row in csv_data) for pattern in patterns)
                if not found_match:
                    raise ValueError(f"No value in column '{column}' matches patterns '{patterns}'")
        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to read CSV file, ERROR: {e}")

    @classmethod
    def check_timeline_values(
        cls,
        timeline_path: str,
        key: str = "name",
        value_list: list = None,
        fuzzy_match: bool = True,
    ) -> None:
        """
        Check if a timeline file contains the specified list of values for a given key.

        Args:
            timeline_path (str): Path to the JSON file.
            key (str, optional): The key to check in the timeline data. Defaults to "name".
            value_list (list, optional): List of values to check for the specified key. Defaults to None.
            fuzzy_match (bool, optional): Whether to enable fuzzy matching. Defaults to True.

        Example:
            Given a timeline JSON file with the following content:
            [
                {"name": "event1", "duration": 100},
                {"name": "event2", "duration": 200},
                {"name": "event3", "duration": 300}
            ]
            To check if the timeline contains events with names like "event*":
            FileChecker.check_timeline_values(timeline_path, "name", key=["event*"], fuzzy_match=True)
            It will verify that events with names starting with "event" exist in the "name" field of the timeline file.
        """
        if not value_list:
            value_list = []
        try:
            cls.check_file_exists(timeline_path)
            with open(timeline_path, "r", encoding="utf-8") as timelinefile:
                data = json.load(timelinefile)
                for value in value_list:
                    if fuzzy_match:
                        pattern = re.compile(re.escape(value).replace(r"\*", ".*"), re.IGNORECASE)
                        found_match = any(pattern.search(item.get(key, "")) for item in data)
                    else:
                        found_match = any(item.get(key, None) == value for item in data)
                    if not found_match:
                        raise ValueError(f"Value '{value}' for key '{key}' not found in Timeline file.")
        except (IOError, OSError, json.JSONDecodeError) as e:
            raise RuntimeError(f"Failed to read Timeline file, ERROR: {e}")

    @classmethod
    def check_json_items(cls, json_path: str, item_pattern: dict) -> None:
        """
        Check if a JSON file contains the specified keys with the expected values, including nested keys.

        Args:
            json_path (str): Path to the JSON file.
            item_pattern (dict): Dictionary containing expected key-value pairs, including nested keys.

        Example:
            Given a JSON file with the following content:
            {
                "a": {
                    "b": {
                        "c": "value"
                    }
                }
            }
            You can set item_pattern to {"a.b.c": "value"} and call:
            FileChecker.check_json_items(json_path, {"a.b.c": "value"})
            This will verify that the value associated with the nested key "a.b.c" is "value".
        """
        try:
            cls.check_file_exists(json_path)
            with open(json_path, "r", encoding="utf-8") as jsonfile:
                data = json.load(jsonfile)
                for nested_key, value in item_pattern.items():
                    keys = nested_key.split(".")
                    current = data
                    for key in keys:
                        if isinstance(current, dict) and key in current:
                            current = current[key]
                        else:
                            raise KeyError(f"Key '{nested_key}' not found in JSON file.")
                    if current != value:
                        raise ValueError(
                            f"Value for key '{nested_key}' does not match. Expected '{value}', found '{current}'"
                        )
        except (IOError, OSError, json.JSONDecodeError) as e:
            raise RuntimeError(f"Failed to read JSON file, ERROR: {e}")

    @classmethod
    def check_json_keys(cls, json_path: str, keys: list) -> None:
        """
        Check if a JSON file contains the specified keys with the expected values, including nested keys.

        Args:
            json_path (str): Path to the JSON file.
            keys (list): Dictionary containing expected keys

        Example:
            Given a JSON file with the following content:
            {
                "a": "1",
                "b": "2"
            }
            You can set keys to ["a", "b"] and call:
            FileChecker.check_json_keys(json_path, ["a", "b"])
        """
        try:
            cls.check_file_exists(json_path)
            with open(json_path, "r", encoding="utf-8") as jsonfile:
                data = json.load(jsonfile)
                for key in keys:
                    if key not in data:
                        raise KeyError(f"Key '{key}' not found in JSON file.")
        except (IOError, OSError, json.JSONDecodeError) as e:
            raise RuntimeError(f"Failed to read JSON file, ERROR: {e}")

    @classmethod
    def check_json_nested_keys(cls, json_path: str, keys: list) -> None:
        """
        Check if a JSON file contains the specified keys with the expected values, including nested keys.

        Args:
            json_path (str): Path to the JSON file.
            keys (list): Dictionary containing expected keys

        Example:
            Given a JSON file with the following content:
            {
                "a": {
                    "b": {
                        "c": "value"
                    }
                },
                "d": {
                }
            }
            You can set keys to ["a.b.c", "d"] and call:
            FileChecker.check_json_keys(json_path, ["a.b.c", "d"])
        """
        try:
            cls.check_file_exists(json_path)
            with open(json_path, "r", encoding="utf-8") as jsonfile:
                data = json.load(jsonfile)
                for nested_key in keys:
                    keys = nested_key.split(".")
                    current = data
                    for key in keys:
                        if isinstance(current, dict) and key in current:
                            current = current[key]
                        else:
                            raise KeyError(f"Key '{nested_key}' not found in JSON file.")
        except (IOError, OSError, json.JSONDecodeError) as e:
            raise RuntimeError(f"Failed to read JSON file, ERROR: {e}")

    @classmethod
    def check_file_line_count(cls, file_path: str, expected_line_count: int) -> None:
        """
        Check if a file (CSV or TXT) contains the expected number of lines.

        Args:
            file_path (str): Path to the file.
            expected_line_count (int): The expected number of lines in the file.
        """
        try:
            cls.check_file_exists(file_path)
            with open(file_path, "r", encoding="utf-8") as file:
                line_count = sum(1 for _ in file)
            if line_count != expected_line_count:
                raise ValueError(f"Expected {expected_line_count} lines, but found {line_count} in file {file_path}.")
        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to read file, ERROR: {e}")

    @classmethod
    def check_db_table_exist(cls, db_path: str, table_name: str) -> None:
        """
        Check db has target table

        Args:
            db_path (str): Path to the db.
            table_name (str): Table to be verified.
            table_struct (list): List of expected headers.
        """
        conn, curs = None, None
        try:
            cls.check_file_exists(db_path)
            conn, curs = DBManager.create_connect_db(db_path)
            if not (conn and curs):
                raise RuntimeError(f"Failed to connect to database: {db_path}")
            if not DBManager.judge_table_exists(curs, table_name):
                raise ValueError(f"Table '{table_name}' does not exist in database '{db_path}'.")
        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to read db, ERROR: {e}")
        finally:
            DBManager.destroy_db_connect(conn, curs)

    @classmethod
    def check_db_table_struct(cls, db_path: str, table_name: str, table_struct: list) -> None:
        """
        Check if the struct of a table match the given struct list.

        Args:
            db_path (str): Path to the db.
            table_name (str): Table to be verified.
            table_struct (list): List of expected headers.
        """
        try:
            cls.check_db_table_exist(db_path, table_name)
            conn, curs = DBManager.create_connect_db(db_path)
            curs.execute(f"PRAGMA table_info({table_name})")
            columns_info = curs.fetchall()
            column_names = {info[1] for info in columns_info}
            expected_struct_set = set(table_struct)
            if not expected_struct_set.issubset(column_names):
                raise ValueError(f"{table_name} Missing headers: {expected_struct_set - column_names}")
        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to read db, ERROR: {e}")
        finally:
            DBManager.destroy_db_connect(conn, curs)

    @classmethod
    def compare_csv_num_with_table(cls, csv_path: str, db_path: str, table_name: str) -> None:
        """
        Check if the struct of a table match the given struct list.

        Args:
            db_path (str): Path to the db.
            table_name (str): Table to be verified.
            table_struct (list): List of expected headers.
        """
        try:
            cls.check_db_table_exist(db_path, table_name)
            cls.check_file_exists(csv_path)
            df = pd.read_csv(csv_path)
            # 查询数据库中api数量
            conn, curs = DBManager.create_connect_db(db_path)
            res = curs.execute(f"SELECT COUNT(*) FROM {table_name}").fetchall()
            data_num = res[0][0]
            if data_num != len(df):
                raise ValueError(f"{csv_path} is different from {table_name} in {db_path}")
        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to read db, ERROR: {e}")
        finally:
            DBManager.destroy_db_connect(conn, curs)

    @classmethod
    def check_file_for_keyword(cls, file_path: str, keyword: str) -> None:
        """
        Check if a file contains a specific keyword (case-insensitive).

        Args:
            file_path (str): Path to the file.
            keyword (str): The keyword to search for in the file.
        """
        try:
            cls.check_file_exists(file_path)
            matched_lines = []
            with open(file_path, "r", encoding="utf-8") as file:
                for line_no, line in enumerate(file, start=1):
                    if keyword.lower() in line.lower():
                        matched_lines.append(f"{line_no}: {line.rstrip()}")

            if matched_lines:
                matched_content = "\n".join(matched_lines)
                raise RuntimeError(f"file {file_path} contains the keyword '{keyword}':\n{matched_content}")
        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to read file, ERROR: {e}")

    @classmethod
    def check_communication_fields_from_trace_view(
        cls, trace_view_path: str, communication_operator: str, fields: list
    ) -> None:
        """
        Check whether the communication operator in the trace_view.json file contains the specified field.

        Args:
            trace_view_path (str): Path to the trace_view.json file.
            communication_operator (str): Communication Operator Name.
            fields (list): Unique fields of communication operators.

        Example:
            Given a trace_view.json file with the following content:
            {
                "name": 'hcom_allReduce__612_6_1',
                "args": {
                        'alg_type': 'MESH-RING-NHR',
                        'connection_id': 1172,
                        'count': 16781312,
                        'data_type': 'FP32',
                        'model id': 4294967295,
                        'relay': 'no',
                        'retry': 'no'
                        }
            }
            FileChecker.check_json_keys(trace_view_json_path, "allReduce", ["alg_type", "connection_id", "count",
            "data_type", "model id", "relay", "retry"])
        """
        communication_operators = []
        try:
            cls.check_file_exists(trace_view_path)
            with open(trace_view_path, "r", encoding="utf-8") as jsonfile:
                data = json.load(jsonfile)
                for item in data:
                    if item.get("name", "").startswith(communication_operator):
                        communication_operators.append(item)
        except (IOError, OSError, json.JSONDecodeError) as e:
            raise RuntimeError(f"Failed to read JSON file, ERROR: {e}")
        for communication_item in communication_operators:
            args = communication_item.get("args")
            if set(fields) != args.keys():
                raise ValueError(f"args of {args.keys()} does not equal {fields}")

    @classmethod
    def check_communication_fields_from_communication_json(
        cls, communication_json_path: str, communication_operator: str, fields: list
    ) -> None:
        """
        Check whether the communication operator in the communication.json file contains the specified field.

        Args:
            communication_json_path (str): Path to the trace_view.json file.
            communication_operator (str): Communication Operator Name.
            fields (list): Unique fields of communication operators.

        Example:
            Given a communication.jsonn file with the following content:
            {
                "step1": {
                    "p2p": {},
                    "collective": {
                        "hcom_broadcast__612_0_1@5862276093215481612": {
                            "Communication Time Info": {
                            "Start Timestamp(us)": 1757052765319042.5,
                            "Transit Time(ms)": 0,
                            "Wait Time(ms)": 0.00164,
                            "Synchronization Time(ms)": 0.00164,
                            "Idle Time(ms)": 0.01794,
                            "Wait Time Ratio": 1.0,
                            "Synchronization Time Ratio": 1.0
                            },
                    }
                }
            }
            FileChecker.check_json_keys(communication_json_path, "allReduce", ["Start Timestamp(us)", "Elapse Time(ms)",
            "Transit Time(ms)", "Wait Time(ms)", "Synchronization Time(ms)","Idle Time(ms)", "Wait Time Ratio",
            "Synchronization Time Ratio"])
        """
        communication_fields = []
        try:
            cls.check_file_exists(communication_json_path)
            with open(communication_json_path, "r", encoding="utf-8") as jsonfile:
                data = json.load(jsonfile)
                for step_data in data.values():
                    communications = step_data.get("collective", {})
                    for key, value in communications.items():
                        if "allReduce" in key:
                            fields = value.get("Communication Time Info", {}).keys()
                            communication_fields.append(fields)
        except (IOError, OSError, json.JSONDecodeError) as e:
            raise RuntimeError(f"Failed to read JSON file, ERROR: {e}")
        for communication_field in communication_fields:
            if set(fields) != communication_field:
                raise ValueError(f"communication of {communication_operator} does not equal {fields}")

    @classmethod
    def check_excel_headers(cls, excel_path: str, sheet_name: str, headers: list) -> None:
        """
        Check if the headers of a specified sheet in an Excel file exactly match the given headers list.

        Args:
            excel_path (str): Path to the Excel file.
            sheet_name (str): Name of the sheet to check.
            headers (list): List of expected headers (order matters).

        Example:
            FileChecker.check_excel_headers_strict(excel_path, "problems", ["Op Name", "Op Type", "Status"])
        """
        try:
            cls.check_file_exists(excel_path)
            # Read the first row of the specified sheet as the header.
            df = pd.read_excel(excel_path, sheet_name=sheet_name, nrows=0)
            excel_headers = df.columns.tolist()
            if excel_headers != headers:
                raise ValueError(
                    f"{excel_path} (Sheet: {sheet_name}) Headers mismatch. Expected: {headers}, Actual: {excel_headers}"
                )
        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to read Excel file, ERROR: {e}")
        except ValueError as e:
            raise RuntimeError(f"Sheet '{sheet_name}' not found in Excel file, ERROR: {e}")

    @classmethod
    def check_excel_items(
        cls,
        excel_path: str,
        sheet_name: str,
        item_pattern: dict,
        fuzzy_match: bool = True,
    ) -> None:
        """
        Enhanced version with detailed error reporting for Excel sheet data validation
        """
        try:
            cls.check_file_exists(excel_path)
            df = pd.read_excel(excel_path, sheet_name=sheet_name)
            for column, patterns in item_pattern.items():
                patterns = [patterns] if not isinstance(patterns, list) else patterns
                column_data = df[column].dropna().astype(str).tolist()

                missing_patterns = []

                for pattern in patterns:
                    if fuzzy_match:
                        # Fuzzy matching
                        regex_pattern = re.compile(re.escape(pattern).replace(r"\*", ".*"), re.IGNORECASE)
                        pattern_found = any(regex_pattern.search(cell_value) for cell_value in column_data)
                    else:
                        # Exact match
                        pattern_found = any(cell_value == pattern for cell_value in column_data)

                    if not pattern_found:
                        missing_patterns.append(pattern)

                if missing_patterns:
                    raise ValueError(
                        f"Column '{column}' in sheet '{sheet_name}' missing patterns: {missing_patterns}. "
                        f"Available data: {column_data}"
                    )
        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to read Excel file, ERROR: {e}")
        except ValueError as e:
            raise RuntimeError(f"Sheet '{sheet_name}' not found in Excel file, ERROR: {e}")

    @classmethod
    def check_excel_row_fields(cls, excel_path: str, sheet_name: str, row_index: int, expected_fields: list) -> None:
        """
        Check if the specified row in a specified sheet of an Excel file contains the given fields.

        Args:
            excel_path (str): Path to the Excel file.
            sheet_name (str): Name of the sheet to check.
            row_index (int): Index of the row to check (0-based index).
            expected_fields (list): List of expected fields (order matters).

        Example:
            FileChecker.check_excel_row_fields("D:\\project\\project_1\\your_excel_file.xlsx", "Sheet1", 2,
            ["Index", "Duration(ms)", "Duration Ratio", "Number", "Duration(ms)", "Duration Ratio", "Number", "Diff Duration(ms)", "Diff Ratio"])
        """
        try:
            cls.check_file_exists(excel_path)
            df = pd.read_excel(excel_path, sheet_name=sheet_name)
            row_data = df.iloc[row_index].tolist()
            if len(row_data) != len(expected_fields):
                raise ValueError(
                    f"{excel_path} (Sheet: {sheet_name}, Row: {row_index + 1}) Field count mismatch. "
                    f"Expected: {len(expected_fields)}, Actual: {len(row_data)}"
                )
            for i, (actual, expected) in enumerate(zip(row_data, expected_fields)):
                if actual != expected:
                    raise ValueError(
                        f"{excel_path} (Sheet: {sheet_name}, Row: {row_index + 1}, Column: {i + 1}) Field mismatch. "
                        f"Expected: {expected}, Actual: {actual}"
                    )
        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to read Excel file, ERROR: {e}")
        except ValueError as e:
            raise RuntimeError(f"Sheet '{sheet_name}' not found in Excel file, ERROR: {e}")
        except IndexError as e:
            raise RuntimeError(
                f"Row index {row_index} is out of range for sheet '{sheet_name}' in Excel file, ERROR: {e}"
            )

    @classmethod
    def check_html_table_headers(cls, html_path: str, table_title: str, headers: list) -> None:
        """
        Check if the headers of a specified table in an HTML file exactly match the given headers list.

        Args:
            html_path (str): Path to the HTML file.
            table_title (str): Title of the table to check.
            headers (list): List of expected headers (order matters).

        Example:
            FileChecker.check_html_table_headers(html_path, "Kernel compare of Target and Benchmark",
                                                 ["Order Id", "Kernel Type", "Core Type", "Total Duration(us)",
                                                  "Avg Duration(us)", "Max Duration(us)", "Min Duration(us)",
                                                  "Calls", "Benchmark Total Duration(us)", "Benchmark Avg Duration(us)",
                                                  "Benchmark Max Duration(us)", "Benchmark Min Duration(us)",
                                                  "Benchmark Calls", "Diff Total Ratio", "Diff Avg Ratio"])
        """
        try:
            cls.check_file_exists(html_path)
            with open(html_path, "r", encoding="utf-8") as file:
                soup = BeautifulSoup(file, "html.parser")

            # Find the h2 tag with the specified table title
            h2 = soup.find("h2", string=table_title)
            if h2:
                table = h2.find_next("table")
                if table:
                    header_row = table.find("tr")
                    if header_row:
                        actual_headers = [th.get_text(strip=True) for th in header_row.find_all("th")]
                        if actual_headers != headers:
                            raise ValueError(
                                f"{html_path} (Table: {table_title}) Headers mismatch. Expected: {headers}, "
                                f"Actual: {actual_headers}"
                            )
                    else:
                        raise RuntimeError(f"Header row not found in the table: {table_title}")
                else:
                    raise RuntimeError(f"Table not found for title: {table_title}")
            else:
                raise RuntimeError(f"Title not found: {table_title}")
        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to read HTML file, ERROR: {e}")

    @classmethod
    def check_csv_data_non_negative(
        cls,
        file_path: str,
        comparison_func: Callable[[str], bool],
        columns: Optional[Iterable[str]] = None,
    ) -> None:
        """
        Check whether the specified columns of the CSV file satisfy
        the condition given by `comparison_func`.

        Args:
            file_path: Path to the CSV file.
            comparison_func: A function that takes a cell value (str) and
                returns True when the value meets the condition.
            columns: List/tuple of column names to be checked.
                If None, check all columns.
        Raises:
            AssertionError: When any value in the specified columns
                fails the comparison, or the file cannot be read.
        """
        try:
            cls.check_file_exists(file_path)

            with open(file_path, mode="r", newline="", encoding="utf-8") as csvfile:
                reader = csv.DictReader(csvfile)
                if columns is not None:
                    columns = set(columns) & set(reader.fieldnames or [])
                    if not columns:
                        raise AssertionError("None of the specified columns exist in CSV")

                for row in reader:
                    iter_cols = columns if columns is not None else row.keys()
                    for col in iter_cols:
                        value = row[col]
                        if not comparison_func(value):
                            raise AssertionError(
                                f"CSV column '{col}' contains value that does not meet the condition: {value}"
                            )
        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to read CSV file, ERROR: {e}")
