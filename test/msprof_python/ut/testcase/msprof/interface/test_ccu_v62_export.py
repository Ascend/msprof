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
import csv
import json
import os
import sqlite3
import struct

import pytest

from common_func.file_manager import FileManager
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.platform.chip_manager import ChipManager
from msinterface.msprof_export import ExportCommand
from msinterface.msprof_export_data import MsProfExportDataUtils
from msmodel.add_info.ccu_add_info_model import CCUGroupInfoModel
from msmodel.add_info.ccu_add_info_model import CCUWaitSignalInfoModel
from msparser.hardware.ccu_channel_parser import CCUChannelParser
from msparser.hardware.ccu_mission_parser import CCUMissionParser
from profiling_bean.hardware.ccu_mission_v62_bean import CCUMissionV62Bean
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.data_tag import DataTag


def test_v62_binary_to_summary_and_timeline(tmp_path, monkeypatch):
    device = tmp_path / "device_0"
    for folder in ("data", "sqlite", "summary", "timeline"):
        (device / folder).mkdir(parents=True)
    host = tmp_path / "host"
    (host / "sqlite").mkdir(parents=True)
    with CCUGroupInfoModel(str(host)) as model:
        group = (0, "1", "2", 0, 2, 0, 99, 1, 0, 5, 0, "SUM", "FP32", "FP32", 4096)
        assert model.flush([group + (17, 1), group + (21, 1)])
    with CCUWaitSignalInfoModel(str(host)) as model:
        wait = (0, "1", "2", 0, 2, 0, 99)
        assert model.flush(
            [
                wait + (2, 0, 7, 0, 1, 0, 17, 1),
                wait + (3, 0, 7, 0, 1, 255, 17, 1),
                wait + (3, 0, 7, 0, 1, 255, 21, 1),
                wait + (3, 1, 7, 0, 1, 3, 17, 1),
            ]
        )
    monkeypatch.setattr(ChipManager(), "chip_id", ChipModel.CHIP_V6_2_0)
    reader = InfoConfReader()
    monkeypatch.setattr(
        reader, "_info_json", {"pid": 1, "devices": "0", "DeviceInfo": [{"hwts_frequency": "25", "id": 0}]}
    )
    for attr in ("_dev_cnt", "_host_mon", "_local_time_offset"):
        monkeypatch.setattr(reader, attr, 0)
    mission = "ccu0.instr.0.slice_0"
    other_die_mission = "ccu1.instr.0.slice_0"
    channel = "ccu0.stat.0.slice_0"
    other_die_channel = "ccu1.stat.0.slice_0"
    loop_setckb = b"".join(
        struct.pack("=8I", 5, (loop_count << 19) | (loop_id << 9) | (syscnt_type << 2) | 3, 8, counter, 0, 0, 0, 0)
        for loop_count, loop_id in ((2, 9), (1, 8))
        for syscnt_type, counter in ((0, 30), (2, 50))
    )
    (device / "data" / mission).write_bytes(
        struct.pack("=8I", 1, 5 << 16, 25, 0, 50, 0, 0, 0)
        + struct.pack("=8I", 2, 5, 7, 50, 0, 0, 0, 0)
        + struct.pack("=8I", 3, 1, 7, 30, 0, 0, 0, 0)
        + struct.pack("=8I", 3, 9, 7, 50, 0, 0, 0, 0)
        + struct.pack("=8I", 4, 1, 7, 30, 0, 0, 0, 0)
        + loop_setckb
    )
    (device / "data" / other_die_mission).write_bytes(
        struct.pack("=8I", 3, 1, 7, 40, 0, 0, 0, 0)
        + struct.pack("=8I", 3, 9, 7, 60, 0, 0, 0, 0)
        + struct.pack("=8I", 4, 9, 7, 50, 0, 0, 0, 0)
    )
    words = [0] * 96
    words[0], words[11], words[12] = 17 | (21 << 10), 3, 25
    words[14:18] = [10, 20, 10, 500]
    before_end = struct.pack("=96I", *words)
    words[12], words[14], words[15] = 50, 999, 1000
    (device / "data" / channel).write_bytes(before_end + struct.pack("=96I", *words))
    words[12], words[14], words[15] = 25, 1000, 2000
    (device / "data" / other_die_channel).write_bytes(struct.pack("=96I", *words))
    config = {"result_dir": str(device), "chip_model": 16}
    for parser, tag, names in (
        (CCUMissionParser, DataTag.CCU_MISSION, [mission, other_die_mission]),
        (CCUChannelParser, DataTag.CCU_CHANNEL, [channel, other_die_channel]),
    ):
        parser({tag: names}, config).ms_run()
        assert all((device / "data" / (name + ".complete")).exists() for name in names)
        parser({tag: names}, config).ms_run()
    summary_types = {"ccu_mission_loopgroup", "ccu_mission_setckb", "ccu_channel_v62"}
    for mode, expected_types in (("summary", summary_types), ("timeline", {"ccu_mission"})):
        selected = {
            item["export_type"].name.lower()
            for item in ExportCommand.EXPORT_HANDLE_MAP[mode]
            if item["export_type"].name.startswith("CCU_") and item["handler"](str(device), 0)
        }
        assert selected == expected_types
        for data_type in selected:
            result = MsProfExportDataUtils.export_data(
                {
                    "project": str(device),
                    "device_id": "0",
                    "data_type": data_type,
                    "export_type": mode,
                    "export_format": "csv" if mode == "summary" else "json",
                    "export_dump_folder": mode,
                }
            )
            assert json.loads(result)["status"] == 0, result
    for name, first_column, first_value in (
        ("ccu_mission_loopgroup", "Task Id", "1"),
        ("ccu_mission_setckb", "Task Id", "2"),
        ("ccu_channel_v62", "Channel Id", "17"),
    ):
        with next((device / "summary").glob(name + "_*.csv")).open(newline="") as report:
            rows = csv.reader(report)
            header, *records = rows
        record = records[0]
        assert (header[0], record[0]) == (first_column, first_value)
        if name == "ccu_channel_v62":
            assert header == ["Channel Id", "Die Id", "Timestamp(us)", "Avg Delay", "Max Delay"]
            assert record[1] == "0"
            assert record[-2:] == ["10", "20"]
            assert [row[-2:] for row in records if row[:2] == ["17", "1"]] == [["1000", "2000"]]
    events = json.loads(next((device / "timeline").glob("*.json")).read_text())
    instructions = {
        (event["args"]["Task Id"], event["args"]["Die Id"]): event for event in events if event["ph"] == "X"
    }
    assert {key: float(event["dur"]) for key, event in instructions.items()} == pytest.approx(
        {(1, 0): 4.0, (2, 0): 0.0, (3, 0): 3.2, (3, 1): 3.2, (5, 0): 3.2}
    )
    assert instructions[1, 0]["args"] == {
        "Task Id": 1,
        "Instruction ID": 5,
        "Die Id": 0,
        "Data Size": 4096,
        "Bandwidth (MB/s)": 1024.0,
        "Reduce Op Type": "SUM",
        "Input Data Type": "FP32",
        "Output Data Type": "FP32",
    }
    assert instructions[2, 0]["args"] == {
        "Task Id": 2,
        "Instruction ID": 7,
        "CKB Info": "0" * 16,
        "Notify Instruction ID": 7,
        "Die Id": 0,
        "Mask": 0,
    }
    assert instructions[3, 0]["args"] == {
        "Task Id": 3,
        "Instruction ID": 7,
        "CKB Info": "0" * 16,
        "Notify Instruction ID": 7,
        "Die Id": 0,
        "Mask": 255,
        "Maximum Delay Channel": 17,
        "Maximum Channel Delay": 10,
    }
    assert instructions[3, 1]["args"] == {
        **instructions[3, 0]["args"],
        "Die Id": 1,
        "Mask": 3,
        "Maximum Channel Delay": 1000,
    }
    assert instructions[3, 0]["name"] == "SetCKB Mission"
    assert instructions[5, 0]["name"] == "SetCKB Loop"
    loop_args = instructions[5, 0]["args"]
    assert (loop_args["Earliest Loop Id"], loop_args["Latest Loop Id"], loop_args["Longest Duration Loop Id"]) == (
        8,
    ) * 3


@pytest.mark.parametrize(
    ("chip", "data_type"),
    [
        (ChipModel.CHIP_V6_2_0, "ccu_channel"),
        (ChipModel.CHIP_V6_1_0, "ccu_channel_v62"),
        (ChipModel.CHIP_V4_1_0, "ccu_channel"),
    ],
)
def test_direct_channel_export_handles_unsupported_type(tmp_path, monkeypatch, chip, data_type):
    monkeypatch.setattr(ChipManager(), "chip_id", chip)
    result = MsProfExportDataUtils.export_data(
        {
            "project": str(tmp_path),
            "device_id": "0",
            "data_type": data_type,
            "export_type": "summary",
            "export_format": "csv",
            "export_dump_folder": "summary",
        }
    )
    assert json.loads(result)["status"] == NumberConstant.WARN


@pytest.mark.parametrize(
    ("chip", "enabled"),
    [(ChipModel.CHIP_V6_1_0, True), (ChipModel.CHIP_V6_1_1, True), (ChipModel.CHIP_V4_1_0, False)],
)
def test_v61_summary_requires_a_chip_in_the_ccu_profile(tmp_path, monkeypatch, chip, enabled):
    data_dir = tmp_path / "data"
    data_dir.mkdir()
    for file_name in ("ccu0.instr.0.slice_0", "ccu0.stat.0.slice_0"):
        (data_dir / file_name).write_bytes(b"\x01")
    monkeypatch.setattr(ChipManager(), "chip_id", chip)
    expected = {"ccu_mission", "ccu_channel"} if enabled else set()

    selected = {
        item["export_type"].name.lower()
        for item in ExportCommand.EXPORT_HANDLE_MAP["summary"]
        if item["export_type"].name.startswith("CCU_") and item["handler"](str(tmp_path), 0)
    }
    assert selected == expected


def test_mission_write_failure_rolls_back_both_tables(tmp_path, monkeypatch):
    (tmp_path / "data").mkdir()
    (tmp_path / "sqlite").mkdir()
    name = "ccu0.instr.0.slice_0"
    path = tmp_path / "data" / name
    setckb = struct.pack("=8I", 2, 5, 7, 50, 0, 0, 0, 0)
    # Restoring the upper bits makes this LoopGroup counter exceed SQLite INTEGER.
    invalid_loopgroup = struct.pack("=8I", 1, 0, 25, 0, 50, 1 << 31, 0, 0)
    path.write_bytes(setckb + invalid_loopgroup)
    config = {"result_dir": str(tmp_path), "chip_model": 16}
    monkeypatch.setattr(ChipManager(), "chip_id", ChipModel.CHIP_V6_2_0)
    CCUMissionParser({DataTag.CCU_MISSION: [name]}, config).ms_run()
    assert not path.with_name(name + ".complete").exists()
    channel = "ccu0.stat.0.slice_0"
    (tmp_path / "data" / channel).write_bytes(bytes(384))
    CCUChannelParser({DataTag.CCU_CHANNEL: [channel]}, config).ms_run()

    with sqlite3.connect(tmp_path / "sqlite" / "ccu.db") as conn:
        assert conn.execute("SELECT COUNT(*) FROM OriginSetCKBV62").fetchone() == (0,)
        assert conn.execute("SELECT COUNT(*) FROM OriginLoopGroupV62").fetchone() == (0,)
        assert conn.execute("SELECT COUNT(*) FROM OriginChannelV62").fetchone() == (0,)
    for mode in ("summary", "timeline"):
        assert not any(
            item["handler"](str(tmp_path), 0)
            for item in ExportCommand.EXPORT_HANDLE_MAP[mode]
            if item["export_type"].name.startswith("CCU_")
        )

    path.write_bytes(setckb + struct.pack("=8I", 1, 0, 25, 0, 50, 0, 0, 0))
    CCUMissionParser({DataTag.CCU_MISSION: [name]}, config).ms_run()
    assert path.with_name(name + ".complete").exists()


def test_mission_retries_partial_completion_without_reinserting(tmp_path, monkeypatch):
    data_dir = tmp_path / "data"
    data_dir.mkdir()
    (tmp_path / "sqlite").mkdir()
    names = ["ccu0.instr.0.slice_0", "ccu0.instr.0.slice_1"]
    for name, task in zip(names, (2, 3)):
        (data_dir / name).write_bytes(struct.pack("=8I", task, 5, 7, 50, 0, 0, 0, 0))
    config = {"result_dir": str(tmp_path), "chip_model": 16}
    files = {DataTag.CCU_MISSION: names}
    add_complete_file = FileManager.add_complete_file

    def mark_first_file(project_path, file_name):
        if file_name == names[0]:
            add_complete_file(project_path, file_name)

    with monkeypatch.context() as patch:
        patch.setattr(FileManager, "add_complete_file", mark_first_file)
        CCUMissionParser(files, config).ms_run()
    assert (data_dir / (names[0] + ".complete")).exists()
    assert not (data_dir / (names[1] + ".complete")).exists()
    with sqlite3.connect(tmp_path / "sqlite" / "ccu.db") as conn:
        assert conn.execute("SELECT COUNT(*) FROM OriginSetCKBV62").fetchone() == (2,)

    CCUMissionParser(files, config).ms_run()
    assert all((data_dir / (name + ".complete")).exists() for name in names)
    copied_file = data_dir / names[0]
    stamp = copied_file.stat()
    os.utime(copied_file, ns=(stamp.st_atime_ns, stamp.st_mtime_ns + 2_000_000_000))
    parser = CCUMissionParser(files, config)
    parser.parse()
    assert parser.save()
    with sqlite3.connect(tmp_path / "sqlite" / "ccu.db") as conn:
        assert conn.execute("SELECT COUNT(*) FROM OriginSetCKBV62").fetchone() == (2,)


@pytest.mark.parametrize("decoded", [None, object()])
def test_invalid_mission_decoder_result_leaves_file_uncommitted(tmp_path, monkeypatch, caplog, decoded):
    (tmp_path / "data").mkdir()
    (tmp_path / "sqlite").mkdir()
    name = "ccu0.instr.0.slice_0"
    path = tmp_path / "data" / name
    path.write_bytes(struct.pack("=8I", 2, 5, 7, 50, 0, 0, 0, 0))
    parser = CCUMissionParser({DataTag.CCU_MISSION: [name]}, {"result_dir": str(tmp_path), "chip_model": 16})
    monkeypatch.setattr(CCUMissionV62Bean, "decode", lambda self, _: decoded)

    parser.ms_run()

    assert "Invalid CCU v6.2 mission record 0" in caplog.text
    assert not path.with_name(name + ".complete").exists()
    assert not (tmp_path / "sqlite" / "ccu.db").exists()
