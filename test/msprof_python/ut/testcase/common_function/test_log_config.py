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
from unittest import mock

import pytest

from common_func import common
from common_func.file_manager import FileOpen


@pytest.fixture
def config_file(tmp_path, monkeypatch):
    analysis = tmp_path / "analysis"
    config = analysis / "msconfig" / "msprof_config.json"
    config.parent.mkdir(parents=True)
    monkeypatch.setattr(common, "__file__", str(analysis / "common_func" / "common.py"))
    monkeypatch.chdir(tmp_path)
    return config


@pytest.mark.parametrize("level, threshold", [(0, logging.DEBUG), (1, logging.INFO),
                                            (2, logging.WARNING), (3, logging.ERROR)])
@pytest.mark.parametrize("entry", ["init_log", "Log"])
def test_file_log_threshold(config_file, tmp_path, level, threshold, entry):
    config_file.write_text('{"log":{"level":%d}}' % level, encoding="utf-8")
    output = tmp_path / "result"
    log_dir = tmp_path / "mindstudio_profiler_log"
    log_dir.mkdir()
    logger = logging.getLogger() if entry == "init_log" else logging.getLogger("test-msprof-log-config")
    old_handlers, old_level = logger.handlers[:], logger.level
    logger.handlers = []
    try:
        if entry == "init_log":
            common.init_log(str(output))
        else:
            common.Log(str(output), logger.name)
        for name in ("DEBUG", "INFO", "WARN", "ERROR"):
            logger.log(getattr(logging, name), "marker_%s", name)
        log_file = next(log_dir.iterdir())
        content = log_file.read_text(encoding="utf-8")
        assert logger.level == threshold
        for name in ("DEBUG", "INFO", "WARN", "ERROR"):
            assert ("marker_%s" % name in content) == (getattr(logging, name) >= threshold)
    finally:
        for handler in logger.handlers[:]:
            if handler not in old_handlers:
                handler.close()
        logger.handlers = old_handlers
        logger.setLevel(old_level)


@pytest.mark.parametrize("content, warned", [
    (None, False), ("{}", False), ('{"log":{}}', False),
    ("{", True), ("[]", True), ('{"log":null}', True),
    ('{"log":{"level":"DEBUG"}}', True), ('{"log":{"level":"0"}}', True),
    ('{"log":{"level":-1}}', True), ('{"log":{"level":4}}', True),
    ('{"log":{"level":18446744073709551615}}', True),
    ('{"log":{"level":true}}', True), ('{"log":{"level":false}}', True),
    ('{"log":{"level":1.0}}', True),
    ('{"log":{"level":[]}}', True), ('{"log":{"level":null}}', True),
    ('{"unused":' + "[" * 10000 + "0" + "]" * 10000 + "}", True),
])
def test_config_fallback(config_file, content, warned):
    if content is not None:
        config_file.write_text(content, encoding="utf-8")
    with mock.patch.object(common, "warn") as warning:
        assert common.get_log_level() == logging.INFO
        assert warning.called == warned


def test_config_read_failure(config_file):
    config_file.write_text("{}", encoding="utf-8")
    with mock.patch.object(FileOpen, "__enter__", side_effect=PermissionError), \
            mock.patch.object(common, "warn") as warning:
        assert common.get_log_level() == logging.INFO
        warning.assert_called_once()


def test_config_larger_than_64_kib(config_file):
    config_file.write_text(" " * (64 * 1024 + 1) + '{"log":{"level":0}}', encoding="utf-8")
    with mock.patch.object(common, "warn") as warning:
        assert common.get_log_level() == logging.DEBUG
        warning.assert_not_called()
