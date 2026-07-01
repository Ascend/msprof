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
from abc import ABC
import glob
import logging

from utils.db_check import DBManager
from utils.file_check import FileChecker
from utils.table_fields import TableFields

logging.basicConfig(
    level=logging.INFO, format="\n%(asctime)s %(filename)s [line:%(lineno)d] [%(levelname)s] %(message)s"
)


class BaseAscendMsprofChecker(ABC):
    EXPECT_DB_TABLES = []
    DB_VALUE_SPECS = []
    DISABLED_CHECKS = set()

    TIMELINE_CHECKS = [
        {"key": "cat", "values": ["HostToDevice"], "fuzzy_match": True},
        {"key": "name", "values": ["aclnnAdd_*", "Conv2D*", "*Relu*"], "fuzzy_match": True},
    ]

    OP_SUMMARY_SPEC = {
        "pattern": "op_summary_*.csv",
        "headers": [
            "Task Duration(us)",
            "Task Wait Time(us)",
            "Task Start Time(us)",
            "Block Num",
            "Mix Block Num",
            "Task ID",
            "Stream ID",
            "Device_id",
            "Model ID",
        ],
        "items": [
            {"pattern": {"Op Name": ["aclnnAdd_*", "Conv2D*", "*Relu*"]}},
            {"pattern": {"OP Type": ["Add", "Conv2D", "Relu"], "Task Type": ["AI_VECTOR_CORE", "AI_CORE", "MIX_AIV"]}, "fuzzy_match": False},
        ],
        "non_negative_columns": [
            "Task Duration(us)",
            "Task Wait Time(us)",
            "Task Start Time(us)",
            "Task ID",
            "Stream ID",
            "Device_id",
            "Model ID",
        ],
    }
    OP_STATISTIC_SPEC = {
        "pattern": "op_statistic_*.csv",
        "headers": [
            "Device_id",
            "OP Type",
            "Core Type",
            "Count",
            "Total Time(us)",
            "Min Time(us)",
            "Avg Time(us)",
            "Max Time(us)",
            "Ratio(%)",
        ],
        "items": [
            {"pattern": {"OP Type": ["Add", "Conv2D", "Relu"], "Core Type": ["AI_VECTOR_CORE", "AI_CORE", "MIX_AIV"]}, "fuzzy_match": False}
        ],
        "non_negative_columns": ["Count", "Total Time(us)", "Min Time(us)", "Avg Time(us)", "Max Time(us)", "Ratio(%)"],
    }
    AI_CORE_UTILIZATION_SPEC = None
    AI_VECTOR_CORE_UTILIZATION_SPEC = None
    API_STATISTIC_SPEC = {
        "pattern": "api_statistic*.csv",
        "headers": ["Device_id", "Level", "API Name", "Time(us)", "Count", "Avg(us)", "Min(us)", "Max(us)", "Variance"],
        "items": [
            {"pattern": {"Level": ["acl", "runtime", "node"]}, "fuzzy_match": False},
            {"pattern": {"API Name": ["Add*", "Cast*", "Relu*"]}},
        ],
        "non_negative_columns": ["Time(us)", "Count", "Avg(us)", "Min(us)", "Max(us)", "Variance"],
    }
    HBM_SPEC = {
        "pattern": "hbm*.csv",
        "headers": ["Device_id", "Metric", "Read(MB/s)", "Write(MB/s)"],
        "non_negative_columns": ["Read(MB/s)", "Write(MB/s)"],
    }
    L2_CACHE_SPEC = {
        "pattern": "l2_cache*.csv",
        "headers": ["Device_id", "Stream Id", "Task Id", "Hit Rate", "Victim Rate", "Op Name"],
        "items": [{"pattern": {"Op Name": ["Conv2D*", "Add*", "TransData*"]}}],
        "non_negative_columns": ["Stream Id", "Task Id", "Victim Rate"],
    }
    LLC_READ_WRITE_SPEC = {
        "pattern": "llc_read_write*.csv",
        "headers": ["Device_id", "Mode", "Task", "Hit Rate(%)", "Throughput(MB/s)"],
        "non_negative_columns": ["Hit Rate(%)", "Throughput(MB/s)"],
    }
    NPU_MEM_SPEC = {
        "pattern": "npu_mem*.csv",
        "headers": ["Device_id", "event", "ddr(KB)", "hbm(KB)", "memory(KB)", "timestamp(us)"],
        "non_negative_columns": ["ddr(KB)", "hbm(KB)", "memory(KB)", "timestamp(us)"],
    }
    NPU_MODULE_MEM_SPEC = {
        "pattern": "npu_module_mem*.csv",
        "headers": ["Device_id", "Component", "Timestamp(us)", "Total Reserved(KB)", "Device"],
        "items": [{"pattern": {"Component": ["APP"]}, "fuzzy_match": False}],
        "non_negative_columns": ["Timestamp(us)", "Total Reserved(KB)"],
    }
    SOC_PMU_SPEC = {
        "pattern": "soc_pmu*.csv",
        "headers": ["Device_id", "Stream Id", "Task Id", "TLB Hit Rate", "TLB Miss Rate", "Op Name"],
        "items": [{"pattern": {"Op Name": ["Conv2D*", "*TransData*", "*Add*"]}}],
        "non_negative_columns": ["Stream Id", "Task Id", "TLB Hit Rate", "TLB Miss Rate"],
    }
    TASK_TIME_SPEC = {
        "pattern": "task_time*.csv",
        "headers": ["Device_id", "kernel_name", "kernel_type", "stream_id", "task_id", "task_time(us)", "task_start(us)", "task_stop(us)"],
        "items": [
            {"pattern": {"kernel_name": ["Conv2D*", "*TransData*", "*Add*"]}},
            {"pattern": {"kernel_type": ["AI_VECTOR_CORE", "AI_CORE", "MIX_AIV"]}, "fuzzy_match": False},
        ],
        "non_negative_columns": ["stream_id", "task_id", "task_time(us)", "task_start(us)", "task_stop(us)"],
    }
    OTHER_CSV_SPECS = []

    def __init__(self, prof_path, expect_db_tables=None):
        self.prof_path = prof_path
        self.expect_db_tables = list(self.EXPECT_DB_TABLES if expect_db_tables is None else expect_db_tables)

    def _is_non_negative(self, value):
        return float(value) >= 0.0

    def _output_glob(self, pattern):
        return glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/{pattern}")

    def _check_timeline_files(self, pattern, display_name, checks):
        if not checks:
            return
        paths = self._output_glob(pattern)
        if not paths:
            raise FileNotFoundError(f"No {display_name} found in {self.prof_path}")
        for path in paths:
            for check in checks:
                FileChecker.check_timeline_values(
                    path,
                    check.get("key", "name"),
                    check.get("values", []),
                    fuzzy_match=check.get("fuzzy_match", True),
                )

    def _check_csv_files(self, display_name, spec):
        if not spec:
            return
        paths = self._output_glob(spec["pattern"])
        if not paths:
            raise FileNotFoundError(f"No {display_name} found in {self.prof_path}")
        for path in paths:
            headers = spec.get("headers")
            if headers:
                FileChecker.check_csv_headers(path, headers)
            for item in spec.get("items", []):
                FileChecker.check_csv_items(
                    path,
                    item["pattern"],
                    fuzzy_match=item.get("fuzzy_match", True),
                )
            columns = spec.get("non_negative_columns")
            if columns:
                FileChecker.check_csv_data_non_negative(path, comparison_func=self._is_non_negative, columns=columns)

    def check_msprof_file(self):
        self.check_msprof_text_file()
        self.check_msprof_db_file()

    def check_msprof_text_file(self):
        checks = [
            ("time_line", self.check_time_line_data),
            ("op_summary", self.check_op_summary_data),
            ("op_statistic", self.check_op_statistic_data),
            ("ai_core_utilization", self.check_ai_core_utilization_data),
            ("ai_vector_core_utilization", self.check_ai_vector_core_utilization_data),
            ("api_statistic", self.check_api_statistic_data),
            ("hbm", self.check_hbm_data),
            ("l2_cache", self.check_l2_cache_data),
            ("llc_read_write", self.check_llc_read_write_data),
            ("npu_mem", self.check_npu_mem_data),
            ("npu_module_mem", self.check_npu_module_mem_data),
            ("soc_pmu", self.check_soc_pmu_data),
            ("task_time", self.check_task_time_data),
            ("other", self.check_other_data),
            ("msprof_log", self.check_msprof_log),
        ]
        for check_name, check_func in checks:
            if check_name not in self.DISABLED_CHECKS:
                check_func()

    def check_time_line_data(self):
        self._check_timeline_files("msprof_*.json", "msprof.json", self.TIMELINE_CHECKS)

    def check_op_summary_data(self):
        self._check_csv_files("op_summary.csv", self.OP_SUMMARY_SPEC)

    def check_op_statistic_data(self):
        self._check_csv_files("op_statistic.csv", self.OP_STATISTIC_SPEC)

    def check_ai_core_utilization_data(self):
        self._check_csv_files("ai_core_utilization.csv", self.AI_CORE_UTILIZATION_SPEC)

    def check_ai_vector_core_utilization_data(self):
        self._check_csv_files("ai_vector_core_utilization.csv", self.AI_VECTOR_CORE_UTILIZATION_SPEC)

    def check_api_statistic_data(self):
        self._check_csv_files("api_statistic.csv", self.API_STATISTIC_SPEC)

    def check_hbm_data(self):
        self._check_csv_files("hbm.csv", self.HBM_SPEC)

    def check_l2_cache_data(self):
        self._check_csv_files("l2_cache.csv", self.L2_CACHE_SPEC)

    def check_llc_read_write_data(self):
        self._check_csv_files("llc_read_write.csv", self.LLC_READ_WRITE_SPEC)

    def check_npu_mem_data(self):
        self._check_csv_files("npu_mem.csv", self.NPU_MEM_SPEC)

    def check_npu_module_mem_data(self):
        self._check_csv_files("npu_module_mem.csv", self.NPU_MODULE_MEM_SPEC)

    def check_soc_pmu_data(self):
        self._check_csv_files("soc_pmu.csv", self.SOC_PMU_SPEC)

    def check_task_time_data(self):
        self._check_csv_files("task_time.csv", self.TASK_TIME_SPEC)

    def check_other_data(self):
        for spec in self.OTHER_CSV_SPECS:
            self._check_csv_files(spec.get("display_name", spec["pattern"]), spec)

    def check_msprof_log(self):
        msprof_log_paths = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_log/*.log")
        if not msprof_log_paths:
            raise FileNotFoundError(f"No msprof log file found in {self.prof_path}")
        for msprof_log_path in msprof_log_paths:
            FileChecker.check_file_for_keyword(msprof_log_path, "[ERROR]")

    def check_msprof_db_file(self):
        db_paths = glob.glob(f"{self.prof_path}/PROF_*/msprof_*.db")
        if not db_paths:
            raise FileNotFoundError(f"No db file found in {self.prof_path}")
        for db_path in db_paths:
            FileChecker.check_file_exists(db_path)
            for table_name in self.expect_db_tables:
                FileChecker.check_db_table_exist(db_path, table_name)
                res_table_fields = DBManager.fetch_all_field_name_in_table(db_path, table_name)
                if res_table_fields != TableFields.get_fields(table_name):
                    raise ValueError(
                        f"Fields for table '{table_name}' do not match. Expected: {TableFields.get_fields(table_name)}, "
                        f"Actual: {res_table_fields}"
                    )
            self.check_db_value_specs(db_path)

    def check_db_value_specs(self, db_path):
        for spec in self.DB_VALUE_SPECS:
            DBManager.check_table_field_expected_values(
                db_path=db_path,
                table_name=spec["table"],
                field_name=spec["field"],
                expected_values=spec["expected_values"],
                where_clause=spec.get("where"),
                allow_extra=spec.get("allow_extra", False),
            )
