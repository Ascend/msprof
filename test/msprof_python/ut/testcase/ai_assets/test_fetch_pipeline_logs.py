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

import contextlib
import importlib.util
import io
import pathlib
import unittest
from unittest import mock


def _load_module():
    root = next(parent for parent in pathlib.Path(__file__).resolve().parents if (parent / ".agents").is_dir())
    path = root / ".agents" / "skills" / "gitcode-ci" / "scripts" / "fetch_pipeline_logs.py"
    spec = importlib.util.spec_from_file_location("fetch_pipeline_logs", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


FETCH = _load_module()

OPENLIBING_LINK = "https://www.openlibing.com/apps/pipelineDetail?projectId=3&pipelineId=1&pipelineRunId=2"

OPENLIBING_BODY = f"""流水线任务触发成功
{OPENLIBING_LINK}
<table>
<tr><th>任务名称</th><th>状态</th><th>日志</th><th>下载链接</th></tr>
<tr><td>UT_test</td><td>failed</td>
<td><a href="{OPENLIBING_LINK}&jobRunId=4&stepRunId=5">日志</a></td>
<td></td></tr>
</table>"""

ACTIONS_BODY = """CI 流水线已启动
https://gitcode.com/Ascend/MindStudio-Profiler/actions/runs/9fa05b35817541bf86b7087d2f3ecca3"""

INVALIDATION_BODY = "source code change are detected, tasks labels is removed"

LEGACY_LINK = "https://www.openlibing.com/apps/pipelineDetail?pipelineId=1&projectId=3&pipelineRunId=2"

LEGACY_BODY = (
    f'流水线 <a href="{LEGACY_LINK}">PR-pipeline</a> 已完成</div><table>'
    "<tr><th>阶段</th><th>任务名称</th><th>状态</th><th>详情</th></tr>"
    "<tr><td>UT</td><td>UT_test</td><td>&#10060;</td>"
    '<td><a href="/detail?x=1">日志</a></td></tr>'
    "</table>"
)


class FetchPipelineLogsTest(unittest.TestCase):
    def test_parse_comment_time_keeps_seconds_and_timezone(self):
        moment = FETCH.parse_comment_time("2026-11-02T16:31:53+08:00")
        self.assertEqual(moment.isoformat(), "2026-11-02T08:31:53+00:00")
        self.assertEqual(
            FETCH.parse_comment_time("2026-11-02T16:31:53Z").isoformat(),
            "2026-11-02T16:31:53+00:00",
        )
        self.assertIsNone(FETCH.parse_comment_time(""))
        self.assertIsNone(FETCH.parse_comment_time(None))
        self.assertIsNone(FETCH.parse_comment_time(123))
        self.assertIsNone(FETCH.parse_comment_time("not-a-time"))

    def test_parse_pipeline_comments_accepts_any_query_order(self):
        comments = [{"id": 1, "created_at": "2026-11-02T16:31:53+08:00", "body": OPENLIBING_BODY}]
        blocks = FETCH.parse_pipeline_comments(comments)
        self.assertEqual(len(blocks), 1)
        self.assertEqual(blocks[0]["format"], "openlibing")
        self.assertEqual(
            [row["task"] for row in FETCH.find_failed_tasks(blocks[0])],
            ["UT_test"],
        )

    def test_parse_pipeline_comments_actions(self):
        comments = [{"id": 2, "created_at": "2026-11-02T16:31:53+08:00", "body": ACTIONS_BODY}]
        blocks = FETCH.parse_pipeline_comments(comments)
        self.assertEqual(len(blocks), 1)
        self.assertEqual(blocks[0]["format"], "actions")
        self.assertEqual(blocks[0]["run_id"], "9fa05b35817541bf86b7087d2f3ecca3")

    def test_parse_pipeline_comments_legacy_format(self):
        comments = [{"id": 3, "created_at": "2026-11-02T16:31:53+08:00", "body": LEGACY_BODY}]
        blocks = FETCH.parse_pipeline_comments(comments)
        self.assertEqual(len(blocks), 1)
        self.assertEqual(blocks[0]["name"], "PR-pipeline")
        self.assertIsNotNone(blocks[0]["comment_time"])
        self.assertEqual(blocks[0]["rows"][0]["task"], "UT_test")

    def test_choose_block_uses_seconds(self):
        comments = [
            {"id": 1, "created_at": "2026-11-02T10:00:10+08:00", "body": OPENLIBING_BODY},
            {"id": 2, "created_at": "2026-11-02T10:00:20+08:00", "body": INVALIDATION_BODY},
            {"id": 3, "created_at": "2026-11-02T10:00:45+08:00", "body": OPENLIBING_BODY},
        ]
        blocks = FETCH.parse_pipeline_comments(comments)
        chosen = FETCH.choose_block(blocks, comments)
        self.assertIsNotNone(chosen)
        self.assertEqual(chosen["comment_time"].isoformat(), "2026-11-02T02:00:45+00:00")

    def test_choose_block_returns_none_without_run_after_invalidation(self):
        comments = [
            {"id": 1, "created_at": "2026-11-02T10:00:10+08:00", "body": OPENLIBING_BODY},
            {"id": 2, "created_at": "2026-11-02T10:00:20+08:00", "body": INVALIDATION_BODY},
        ]
        blocks = FETCH.parse_pipeline_comments(comments)
        self.assertIsNone(FETCH.choose_block(blocks, comments))

    def test_choose_block_preserves_order_without_timestamps(self):
        comments = [
            {"id": 1, "created_at": "", "body": OPENLIBING_BODY},
            {"id": 2, "created_at": "", "body": OPENLIBING_BODY},
        ]
        blocks = FETCH.parse_pipeline_comments(comments)
        self.assertIsNotNone(FETCH.choose_block(blocks, comments))

    def test_parse_pipeline_comments_merges_new_and_legacy(self):
        comments = [
            {"id": 4, "created_at": "2026-11-02T10:00:10+08:00", "body": OPENLIBING_BODY},
            {"id": 5, "created_at": "2026-11-02T10:05:00+08:00", "body": LEGACY_BODY},
        ]
        blocks = FETCH.parse_pipeline_comments(comments)
        self.assertEqual(len(blocks), 2)
        chosen = FETCH.choose_block(blocks, comments)
        self.assertEqual(chosen["state"], "已完成")
        self.assertEqual(chosen["comment_time"].isoformat(), "2026-11-02T02:05:00+00:00")

    def test_choose_block_prefers_legacy_completion_after_invalidation(self):
        comments = [
            {"id": 6, "created_at": "2026-11-02T10:00:10+08:00", "body": OPENLIBING_BODY},
            {"id": 7, "created_at": "2026-11-02T10:00:20+08:00", "body": INVALIDATION_BODY},
            {"id": 8, "created_at": "2026-11-02T10:05:00+08:00", "body": LEGACY_BODY},
        ]
        blocks = FETCH.parse_pipeline_comments(comments)
        chosen = FETCH.choose_block(blocks, comments)
        self.assertIsNotNone(chosen)
        self.assertEqual(chosen["state"], "已完成")

    def test_run_gitcode_comments_returns_structured_comments(self):
        payload = '[{"id": 1, "created_at": "2026-11-02T16:31:53+08:00", "body": "hi", "user": {"login": "bot"}}]'
        completed = mock.Mock(stdout=payload)
        with mock.patch.object(FETCH, "get_gitcode_binary", return_value="gitcode"):
            with mock.patch.object(FETCH.subprocess, "run", return_value=completed):
                comments = FETCH.run_gitcode_comments("Ascend/msprof", 1)
        self.assertEqual(
            comments,
            [
                {
                    "id": 1,
                    "author": "bot",
                    "created_at": "2026-11-02T16:31:53+08:00",
                    "body": "hi",
                }
            ],
        )

    def test_run_gitcode_comments_rejects_non_list_payload(self):
        completed = mock.Mock(stdout='{"data": []}')
        with mock.patch.object(FETCH, "get_gitcode_binary", return_value="gitcode"):
            with mock.patch.object(FETCH.subprocess, "run", return_value=completed):
                with self.assertRaises(SystemExit):
                    FETCH.run_gitcode_comments("Ascend/msprof", 1)

    def test_run_gitcode_comments_does_not_fallback_to_text(self):
        error = FETCH.subprocess.CalledProcessError(1, "gitcode")
        with mock.patch.object(FETCH, "get_gitcode_binary", return_value="gitcode"):
            with mock.patch.object(FETCH.subprocess, "run", side_effect=error) as run:
                with self.assertRaises(SystemExit):
                    FETCH.run_gitcode_comments("Ascend/msprof", 1)
        self.assertEqual(run.call_count, 1)

    def test_parse_table_rows_old_format_accepts_various_href_quotes(self):
        table = (
            "<table>"
            "<tr><th>阶段</th><th>任务名称</th><th>状态</th><th>详情</th></tr>"
            "<tr><td>UT</td><td>single</td><td>&#10060;</td>"
            "<td><a href='/detail?x=1'>日志</a></td></tr>"
            "<tr><td>UT</td><td>unquoted</td><td>&#10060;</td>"
            "<td><a href=/detail?x=2>日志</a></td></tr>"
            "</table>"
        )
        rows = FETCH.parse_table_rows(table)
        self.assertEqual([row["link"] for row in rows], ["/detail?x=1", "/detail?x=2"])

    def test_redact_text_covers_json_credentials(self):
        text = '{"access_token": "abc", "password": "xyz", "ok": "visible"}'
        redacted = FETCH.redact_text(text)
        self.assertNotIn("abc", redacted)
        self.assertNotIn("xyz", redacted)
        self.assertIn('"ok": "visible"', redacted)

    def test_resolve_task_params_selects_failed_step(self):
        detail = {
            "stages": [
                {
                    "jobs": [
                        {
                            "name": "UT_test",
                            "id": "job-1",
                            "steps": [
                                {"id": "s1", "name": "checkout", "status": "SUCCESS"},
                                {"id": "s2", "name": "compile", "status": "FAILED"},
                            ],
                        }
                    ]
                }
            ]
        }
        params = FETCH.resolve_task_params_from_detail({"task": "UT_test"}, {"link": OPENLIBING_LINK}, detail)
        self.assertEqual(params["stepRunId"], "s2")

    def test_resolve_task_params_rejects_multiple_failed_steps(self):
        detail = {
            "stages": [
                {
                    "jobs": [
                        {
                            "name": "UT_test",
                            "id": "job-1",
                            "steps": [
                                {"id": "s1", "name": "compile", "status": "FAILED"},
                                {"id": "s2", "name": "test", "status": "FAILED"},
                            ],
                        }
                    ]
                }
            ]
        }
        with self.assertRaises(SystemExit):
            FETCH.resolve_task_params_from_detail({"task": "UT_test"}, {"link": OPENLIBING_LINK}, detail)

    def test_resolve_task_params_rejects_ambiguous_steps(self):
        detail = {
            "stages": [
                {
                    "jobs": [
                        {
                            "name": "UT_test",
                            "id": "job-1",
                            "steps": [
                                {"id": "s1", "name": "checkout", "status": "SUCCESS"},
                                {"id": "s2", "name": "compile", "status": "SUCCESS"},
                            ],
                        }
                    ]
                }
            ]
        }
        with self.assertRaises(SystemExit):
            FETCH.resolve_task_params_from_detail({"task": "UT_test"}, {"link": OPENLIBING_LINK}, detail)

    def test_find_failure_excerpt_respects_page_limit(self):
        calls = []

        def fake_exec_log(*_args, **_kwargs):
            calls.append(1)
            return {"log": "nothing", "has_more": True, "start_offset": 0, "end_offset": 0}

        with mock.patch.object(FETCH, "fetch_exec_log", side_effect=fake_exec_log):
            needle, excerpt = FETCH.find_failure_excerpt({"x": 1}, max_pages=3)
        self.assertIsNone(needle)
        self.assertIsNone(excerpt)
        self.assertEqual(len(calls), 3)

    def test_analyze_pr_skips_detail_for_actions(self):
        comments = [{"id": 2, "created_at": "2026-11-02T16:31:53+08:00", "body": ACTIONS_BODY}]
        with mock.patch.object(FETCH, "run_gitcode_comments", return_value=comments):
            with mock.patch.object(FETCH, "fetch_pipeline_detail") as fetch_detail:
                result = FETCH.analyze_pr("Ascend/msprof", 1)
        fetch_detail.assert_not_called()
        self.assertIsNone(result["detail_error"])

    def test_analyze_pr_reports_detail_error(self):
        comments = [{"id": 1, "created_at": "2026-11-02T16:31:53+08:00", "body": OPENLIBING_BODY}]
        with mock.patch.object(FETCH, "run_gitcode_comments", return_value=comments):
            with mock.patch.object(FETCH, "fetch_pipeline_detail", side_effect=SystemExit("detail failed")):
                result = FETCH.analyze_pr("Ascend/msprof", 1)
        self.assertIsNone(result["detail"])
        self.assertEqual(result["detail_error"], "detail failed")
        self.assertEqual(result["comment_count"], 1)

    def test_print_failed_task_summaries_continues_after_codecheck_error(self):
        block = {
            "rows": [
                {
                    "stage": "Check",
                    "task": "CodeCheck",
                    "status": "failed",
                    "link": "https://www.openlibing.com/apps/entryCheckDashCode/?taskId=1",
                },
                {
                    "stage": "UT",
                    "task": "UT_test",
                    "status": "failed",
                    "link": f"{OPENLIBING_LINK}&jobRunId=4&stepRunId=5",
                },
            ]
        }
        output = io.StringIO()
        with mock.patch.object(FETCH, "print_codecheck_summary", side_effect=SystemExit("missing params")):
            with mock.patch.object(FETCH, "find_failure_excerpt", return_value=("FAILED ", "excerpt")):
                with contextlib.redirect_stdout(output):
                    result = FETCH.print_failed_task_summaries(block)
        self.assertTrue(result)
        text = output.getvalue()
        self.assertIn("Unable to resolve codecheck task detail: missing params", text)
        self.assertIn("excerpt", text)

    def test_print_failed_task_summaries_reports_detail_error(self):
        block = {
            "rows": [
                {
                    "stage": "UT",
                    "task": "UT_test",
                    "status": "failed",
                    "link": "https://www.openlibing.com/apps/pipelineDetail?projectId=3",
                }
            ]
        }
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            FETCH.print_failed_task_summaries(block, detail_error="detail failed")
        self.assertIn("Unable to resolve task log parameters: detail failed", output.getvalue())

    def test_print_failed_task_summaries_continues_after_log_error(self):
        block = {
            "rows": [
                {
                    "stage": "UT",
                    "task": "First",
                    "status": "failed",
                    "link": f"{OPENLIBING_LINK}&jobRunId=1&stepRunId=1",
                },
                {
                    "stage": "UT",
                    "task": "Second",
                    "status": "failed",
                    "link": f"{OPENLIBING_LINK}&jobRunId=2&stepRunId=2",
                },
            ]
        }
        calls = []

        def fake_find_failure_excerpt(params):
            calls.append(params["jobRunId"])
            if params["jobRunId"] == "1":
                raise SystemExit("exec-log request failed: HTTP 500")
            return "FAILED ", "second excerpt"

        output = io.StringIO()
        with mock.patch.object(FETCH, "find_failure_excerpt", side_effect=fake_find_failure_excerpt):
            with contextlib.redirect_stdout(output):
                result = FETCH.print_failed_task_summaries(block)
        self.assertTrue(result)
        self.assertEqual(calls, ["1", "2"])
        text = output.getvalue()
        self.assertIn("exec-log request failed", text)
        self.assertIn("second excerpt", text)
        self.assertIn("incomplete", text.lower())


if __name__ == "__main__":
    unittest.main()
