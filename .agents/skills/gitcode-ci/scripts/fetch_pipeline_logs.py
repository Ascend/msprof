#!/usr/bin/env python3
# -*- coding: utf-8 -*-
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

import argparse
import html
import io
import json
import os
import re
import shutil
import subprocess  # nosec B404
import sys
import urllib.parse
import urllib.request
from datetime import datetime, timezone


STATUS_MAP = {
    "9989": "passed",
    "10060": "failed",
    "128346": "running",
}

TEXT_STATUS_MAP = {
    "SUCCESS": "passed",
    "FAILED": "failed",
    "RUNNING": "running",
    "ABORTED": "aborted",
    "CANCELED": "canceled",
    "INIT": "init",
    "QUEUED": "queued",
    "UNSELECTED": "unselected",
    "SKIPPED": "skipped",
}

SENSITIVE_PATTERNS = [
    (re.compile(r"(?i)(authorization\s*:\s*bearer\s+)[^\s]+"), r"\1[REDACTED]"),
    (re.compile(r"(?i)(authorization\s*:\s*)[^\r\n]+"), r"\1[REDACTED]"),
    (re.compile(r"(?i)(token=)[^&\s]+"), r"\1[REDACTED]"),
    (re.compile(r"(?i)(access_token=)[^&\s]+"), r"\1[REDACTED]"),
    (re.compile(r"(?i)(refresh_token=)[^&\s]+"), r"\1[REDACTED]"),
    (re.compile(r"(?i)(signature=)[^&\s]+"), r"\1[REDACTED]"),
    (re.compile(r"(?i)(sig=)[^&\s]+"), r"\1[REDACTED]"),
    (re.compile(r"(?i)(x-amz-signature=)[^&\s]+"), r"\1[REDACTED]"),
    (re.compile(r"(?i)(cookie\s*:\s*)[^\r\n]+"), r"\1[REDACTED]"),
    (re.compile(r"(?i)(set-cookie\s*:\s*)[^\r\n]+"), r"\1[REDACTED]"),
    (re.compile(r"(?i)\b(ak|sk|secret|password|passwd)\b\s*[:=]\s*[^\s,;]+"), "[REDACTED_SECRET]"),
    (
        re.compile(
            r'(?i)(["\'](?:access_token|refresh_token|token|password|passwd|secret|ak|sk)["\']'
            r'\s*:\s*["\'])[^"\']*(["\'])'
        ),
        r"\1[REDACTED]\2",
    ),
]

OPENLIBING_PREFIX = "https://www.openlibing.com/"
PIPELINE_LINK_RE = re.compile(r"https://www\.openlibing\.com/apps/pipelineDetail\?[^'\"\s<)]+")
ACTIONS_RUN_RE = re.compile(r"(https?://[^\s\"'<>)]*/actions/runs/([0-9a-fA-F]{8,}))")
INVALIDATION_RE = re.compile(r"source code change are detected, tasks labels is removed", re.I)
MIN_TIME = datetime.min.replace(tzinfo=timezone.utc)
MAX_LOG_PAGES = 12
FAILED_STEP_STATUSES = {"FAILED", "FAILURE", "ERROR", "ABORTED", "TIMEOUT"}
EXPECTED_FETCH_ERRORS = (SystemExit, OSError, ValueError)


def urlopen_openlibing(request, timeout=20):
    url = getattr(request, "full_url", request)
    if not str(url).startswith(OPENLIBING_PREFIX):
        raise SystemExit("Refused non-openLiBing URL: %s" % url)
    return urllib.request.urlopen(request, timeout=timeout)  # nosec B310


def get_gitcode_binary() -> str:
    candidate = os.environ.get("GITCODE_BIN") or shutil.which("gitcode")
    if not candidate:
        raise SystemExit("gitcode executable not found; set GITCODE_BIN or install GitCode CLI")
    return candidate


def parse_comment_time(value):
    if not isinstance(value, str) or not value:
        return None
    text = value[:-1] + "+00:00" if value.endswith("Z") else value
    try:
        moment = datetime.fromisoformat(text)
    except (TypeError, ValueError):
        return None
    if moment.tzinfo is None:
        moment = moment.replace(tzinfo=timezone.utc)
    return moment.astimezone(timezone.utc)


def run_gitcode_comments(repo: str, pr: int) -> list:
    base_cmd = [get_gitcode_binary(), "pr", "comments", str(pr), "-R", repo, "--limit", "0"]
    try:
        result = subprocess.run(  # nosec B603
            base_cmd + ["--json"],
            check=True,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=300,
        )
    except subprocess.TimeoutExpired as exc:
        raise SystemExit("gitcode pr comments timed out after 300s") from exc
    except subprocess.SubprocessError as exc:
        raise SystemExit("gitcode pr comments failed: %s" % exc) from exc

    try:
        items = json.loads(result.stdout or "[]")
    except json.JSONDecodeError as exc:
        raise SystemExit("gitcode pr comments returned invalid JSON: %s" % exc) from exc
    if not isinstance(items, list):
        raise SystemExit("gitcode pr comments returned unexpected payload")

    comments = []
    for item in items:
        if not isinstance(item, dict):
            continue
        user = item.get("user") or {}
        comments.append(
            {
                "id": item.get("id", ""),
                "author": (user.get("login") or "") if isinstance(user, dict) else "",
                "created_at": item.get("created_at", "") or "",
                "body": item.get("body", "") or "",
            }
        )
    return comments


def parse_pipeline_comments(comments):
    blocks = []
    legacy_pattern = re.compile(
        r'流水线 <a href="(?P<link>[^"]+)">(?P<name>[^<]+)</a> (?P<state>[^<]+)</div>(?P<table>.*?</table>)',
        re.S,
    )
    for comment in comments:
        body = comment.get("body") or ""
        comment_time = parse_comment_time(comment.get("created_at"))
        parsed = False
        if "流水线任务触发成功" in body:
            link_match = PIPELINE_LINK_RE.search(body)
            if link_match:
                link = html.unescape(link_match.group(0))
                if urllib.parse.parse_qs(urllib.parse.urlparse(link).query).get("pipelineId"):
                    table_match = re.search(r"(<table.*?</table>)", body, re.S)
                    table = table_match.group(1) if table_match else ""
                    blocks.append(
                        {
                            "name": "pipeline",
                            "state": "triggered",
                            "link": link,
                            "rows": parse_table_rows(table),
                            "comment_time": comment_time,
                            "format": "openlibing",
                        }
                    )
                    parsed = True
        elif "CI 流水线已启动" in body:
            run_match = ACTIONS_RUN_RE.search(body)
            if run_match:
                blocks.append(
                    {
                        "name": "pipeline",
                        "state": "triggered",
                        "link": html.unescape(run_match.group(1)),
                        "rows": [],
                        "comment_time": comment_time,
                        "format": "actions",
                        "run_id": run_match.group(2),
                    }
                )
                parsed = True
        if parsed:
            continue

        # Backward-compatible parser for older comment HTML (completion reports).
        for match in legacy_pattern.finditer(body):
            blocks.append(
                {
                    "name": clean_html(match.group("name")),
                    "state": clean_html(match.group("state")),
                    "link": html.unescape(match.group("link")),
                    "rows": parse_table_rows(match.group("table")),
                    "comment_time": comment_time,
                    "format": "openlibing",
                }
            )
    return blocks


def redact_text(text: str) -> str:
    value = text or ""
    for pattern, repl in SENSITIVE_PATTERNS:
        value = pattern.sub(repl, value)
    return value


def redact_url(url: str) -> str:
    if not url:
        return url
    parsed = urllib.parse.urlparse(url)
    query = urllib.parse.parse_qsl(parsed.query, keep_blank_values=True)
    redacted_query = []
    for key, value in query:
        if re.search(r"(?i)(token|signature|sig|secret|password|passwd|cookie)", key):
            redacted_query.append((key, "[REDACTED]"))
        else:
            redacted_query.append((key, value))
    return urllib.parse.urlunparse(parsed._replace(query=urllib.parse.urlencode(redacted_query)))


def clean_html(text: str) -> str:
    text = re.sub(r"<.*?>", "", text or "")
    return html.unescape(text).strip()


def parse_table_rows(table_html: str):
    rows = []
    if not table_html:
        return rows

    tr_matches = re.findall(r"<tr[^>]*>(.*?)</tr>", table_html, re.S)
    if not tr_matches:
        return rows
    header_cells = [clean_html(cell) for cell in re.findall(r"<t[dh][^>]*>(.*?)</t[dh]>", tr_matches[0], re.S)]
    old_format = "阶段" in header_cells or "详情" in header_cells

    if not old_format:
        for tr in tr_matches[1:]:
            cells = re.findall(r"<t[dh][^>]*>(.*?)</t[dh]>", tr, re.S)
            if not cells:
                continue
            cleaned_cells = [clean_html(cell) for cell in cells]
            links = re.findall(r'href=["\']?([^"\' >]+)', tr)

            # Current robot format: 任务名称 / 状态 / 日志 / 下载链接
            if len(cleaned_cells) >= 2 and cleaned_cells[0] and cleaned_cells[0] != "任务名称":
                task_name = cleaned_cells[0]
                status_text = cleaned_cells[1].replace("✅", "").replace("❌", "").strip().upper()
                task_link = html.unescape(links[0]) if links else ""
                rows.append(
                    {
                        "stage": "",
                        "task": task_name,
                        "status": TEXT_STATUS_MAP.get(status_text, status_text.lower()),
                        "link": task_link,
                    }
                )
                continue

        if rows:
            return rows

    last_stage = ""
    for tr in tr_matches[1:]:
        cells = re.findall(r"<td(?: rowspan=\"\d+\")?>(.*?)</td>", tr, re.S)
        if len(cells) == 4:
            stage_raw, task_raw, status_raw, detail_raw = cells
            last_stage = clean_html(stage_raw)
        elif len(cells) == 3:
            stage_raw = last_stage
            task_raw, status_raw, detail_raw = cells
        else:
            continue
        link_match = re.search(r'href=["\']?([^"\'\s>]+)', detail_raw)
        if not link_match:
            print(f"Warning: cannot parse task link for {clean_html(task_raw)}", file=sys.stderr)
            continue
        status_code_match = re.search(r'&#(\d+);', status_raw)
        status_code = status_code_match.group(1) if status_code_match else ""
        rows.append(
            {
                "stage": clean_html(stage_raw),
                "task": clean_html(task_raw),
                "status": STATUS_MAP.get(status_code, clean_html(status_raw)),
                "link": html.unescape(link_match.group(1)),
            }
        )
    return rows


def choose_block(blocks, comments=()):
    if not blocks:
        return None
    invalidated_after = None
    for comment in comments:
        if INVALIDATION_RE.search(comment.get("body") or ""):
            moment = parse_comment_time(comment.get("created_at"))
            if moment and (invalidated_after is None or moment > invalidated_after):
                invalidated_after = moment
    effective_blocks = blocks
    if invalidated_after:
        effective_blocks = [
            block for block in blocks if block.get("comment_time") and block["comment_time"] > invalidated_after
        ]
        if not effective_blocks:
            return None
    effective_blocks = sorted(effective_blocks, key=lambda block: block.get("comment_time") or MIN_TIME)
    if effective_blocks[-1].get("rows"):
        effective_blocks = [block for block in effective_blocks if block.get("rows")]
    return effective_blocks[-1]


def find_task(block, task_name=None):
    rows = block["rows"]
    if task_name:
        for row in rows:
            if row["task"] == task_name:
                return row
        raise SystemExit(f"Task not found: {task_name}")
    for row in rows:
        if row["status"] == "failed":
            return row
    for row in rows:
        if row["status"] == "running":
            return row
    return None


def find_failed_tasks(block):
    return [row for row in block["rows"] if row["status"] == "failed" and row["stage"] != "流水线"]


def infer_project_id(block):
    candidates = [block.get("link", "")]
    candidates.extend(row.get("link", "") for row in block.get("rows", []))
    for link in candidates:
        query = urllib.parse.parse_qs(urllib.parse.urlparse(link).query)
        vals = query.get("projectId")
        if vals and vals[0]:
            return vals[0]
    return None


def extract_pipeline_params(block):
    query = urllib.parse.parse_qs(urllib.parse.urlparse(block["link"]).query)
    pipeline_id = (query.get("pipelineId") or [""])[0]
    pipeline_run_id = (query.get("pipelineRunId") or [""])[0]
    project_id = infer_project_id(block)
    if not pipeline_id or not pipeline_run_id:
        raise SystemExit("Missing pipelineId or pipelineRunId in pipeline link")
    if not project_id:
        raise SystemExit("Missing projectId in pipeline context; pass a PR whose comment links contain projectId")
    return {
        "projectId": project_id,
        "pipelineId": pipeline_id,
        "pipelineRunId": pipeline_run_id,
    }


def extract_params(detail_link: str):
    query = urllib.parse.parse_qs(urllib.parse.urlparse(detail_link).query)
    required = ["projectId", "pipelineId", "pipelineRunId", "jobRunId", "stepRunId"]
    params = {}
    for key in required:
        vals = query.get(key)
        if not vals:
            raise SystemExit(f"Missing {key} in task detail link")
        params[key] = vals[0]
    return params


def fetch_pipeline_detail(pipeline_params):
    url = (
        "https://www.openlibing.com/gateway/openlibing-cicd/project/pipeline/pipeline-run/detail?"
        + urllib.parse.urlencode(pipeline_params)
    )
    body = fetch_json(url)
    if body.get("code") != 200:
        raise SystemExit(f"pipeline detail request failed: {body}")
    return body["data"]


def attach_stage_from_detail(block, detail):
    stage_by_task = {}
    for stage in detail.get("stages", []):
        for job in stage.get("jobs") or []:
            stage_by_task[job.get("name")] = stage.get("name", "")
    for row in block["rows"]:
        if not row.get("stage"):
            row["stage"] = stage_by_task.get(row["task"], "")


def resolve_task_params_from_detail(task, block, detail):
    pipeline_params = extract_pipeline_params(block)
    for stage in detail.get("stages", []):
        for job in stage.get("jobs") or []:
            if job.get("name") != task["task"]:
                continue
            steps = job.get("steps") or []
            if not steps:
                continue
            failed_steps = [step for step in steps if str(step.get("status", "")).upper() in FAILED_STEP_STATUSES]
            if len(failed_steps) == 1:
                step = failed_steps[0]
            elif len(failed_steps) > 1:
                names = ", ".join(str(step.get("name") or step.get("id") or "?") for step in failed_steps)
                raise SystemExit(f"Multiple failed steps for task {task['task']}: {names}")
            elif len(steps) == 1:
                step = steps[0]
            else:
                raise SystemExit(f"Unable to determine failed step for task {task['task']}")
            return {
                "projectId": pipeline_params["projectId"],
                "pipelineId": pipeline_params["pipelineId"],
                "pipelineRunId": pipeline_params["pipelineRunId"],
                "jobRunId": job["id"],
                "stepRunId": step["id"],
            }
    raise SystemExit(f"Task not found in pipeline detail: {task['task']}")


def fetch_exec_log(params, sort="desc", limit=500, start_offset=0, end_offset=0):
    payload = dict(params)
    payload.update(
        {
            "sort": sort,
            "limit": limit,
            "startOffset": start_offset,
            "endOffset": end_offset,
        }
    )
    req = urllib.request.Request(
        "https://www.openlibing.com/gateway/openlibing-cicd/project/pipeline/exec-log",
        data=json.dumps(payload).encode(),
        headers={
            "User-Agent": "Mozilla/5.0",
            "Content-Type": "application/json",
            "Accept": "application/json, text/plain, */*",
        },
    )
    with urlopen_openlibing(req, timeout=20) as resp:
        body = json.loads(resp.read().decode("utf-8", "ignore"))
    if body.get("code") != 200:
        raise SystemExit(f"exec-log request failed: {body}")
    return body["data"]


def fetch_json(url: str, payload=None, method=None):
    data = None if payload is None else json.dumps(payload).encode()
    req = urllib.request.Request(
        url,
        data=data,
        headers={
            "User-Agent": "Mozilla/5.0",
            "Content-Type": "application/json",
            "Accept": "application/json, text/plain, */*",
        },
        method=method,
    )
    with urlopen_openlibing(req, timeout=20) as resp:
        return json.loads(resp.read().decode("utf-8", "ignore"))


def analyze_pr(repo: str, pr: int):
    comments = run_gitcode_comments(repo, pr)
    blocks = parse_pipeline_comments(comments)
    block = choose_block(blocks, comments)
    if not block:
        return {
            "pr": pr,
            "block": None,
            "detail": None,
            "detail_error": None,
            "comment_count": len(comments),
        }
    if block.get("format") == "actions":
        return {
            "pr": pr,
            "block": block,
            "detail": None,
            "detail_error": None,
            "comment_count": len(comments),
        }

    detail = None
    detail_error = None
    try:
        detail = fetch_pipeline_detail(extract_pipeline_params(block))
        attach_stage_from_detail(block, detail)
    except (SystemExit, OSError) as exc:
        detail_error = str(exc)
    return {
        "pr": pr,
        "block": block,
        "detail": detail,
        "detail_error": detail_error,
        "comment_count": len(comments),
    }


def fetch_log_pages(params, sort="desc", limit=500, max_pages=MAX_LOG_PAGES):
    start_offset = 0
    end_offset = 0
    for _ in range(max_pages):
        data = fetch_exec_log(
            params,
            sort=sort,
            limit=limit,
            start_offset=start_offset,
            end_offset=end_offset,
        )
        yield data
        if not data.get("has_more"):
            break
        start_offset = data.get("start_offset", 0)
        end_offset = data.get("end_offset", 0)


def find_failure_excerpt(params, max_pages=MAX_LOG_PAGES):
    needles = [
        "short test summary info",
        "FAILED ",
        "ERROR ",
        "Traceback (most recent call last)",
        "DID NOT RAISE",
    ]
    for data in fetch_log_pages(params, sort="asc", max_pages=max_pages):
        log = data.get("log", "")
        best_idx = -1
        best_needle = None
        for needle in needles:
            idx = log.find(needle)
            if idx != -1 and (best_idx == -1 or idx < best_idx):
                best_idx = idx
                best_needle = needle
        if best_idx != -1:
            excerpt = log[max(0, best_idx - 2500) : best_idx + 8000]
            return best_needle, excerpt
    return None, None


def get_task_kind(task):
    parsed = urllib.parse.urlparse(task["link"])
    path = parsed.path
    if "/apps/entryCheckDashCode/" in path:
        return "codecheck"
    if "/apps/personalScandTaskInfor/" in path:
        return "sca"
    if "pipelineDetail" in path and "stepRunId" in urllib.parse.parse_qs(parsed.query):
        return "exec-log"
    return "unknown"


def extract_codecheck_params(detail_link: str):
    parsed = urllib.parse.urlparse(detail_link)
    query = urllib.parse.parse_qs(parsed.query)
    parts = [part for part in parsed.path.split("/") if part]
    try:
        idx = parts.index("entryCheckDashCode")
        task_id = parts[idx + 1]
        uuid = parts[idx + 2]
    except (ValueError, IndexError):
        raise SystemExit(f"Unsupported CodeCheck detail link: {detail_link}")
    return {
        "taskId": task_id,
        "uuid": uuid,
        "projectId": (query.get("projectId") or [""])[0],
        "projectName": (query.get("projectName") or [""])[0],
    }


def fetch_codecheck_report(params):
    url = (
        "https://www.openlibing.com/gateway/openlibing-codecheck/"
        f"ci-portal/v1/codecheck/event/task/issues/report?uuid={params['uuid']}&taskId={params['taskId']}"
    )
    payload = {
        "pageNum": 1,
        "pageSize": 20,
        "date": None,
        "defectLevel": "",
        "ruleType": "",
        "filePath": "",
        "fileName": "",
        "defectStatus": "",
        "checkType": "",
        "trigger": "",
        "defectCheckerName": "",
        "isDelay": "",
    }
    body = fetch_json(url, payload=payload)
    if body.get("code") != 200:
        raise SystemExit(f"CodeCheck report request failed: {body}")
    return body["result"]


def fetch_codecheck_defects(params, report):
    report_vo = report.get("reportVo") or {}
    url = (
        "https://www.openlibing.com/gateway/openlibing-codecheck/"
        f"ci-portal/v1/event/codecheck/task?uuid={params['uuid']}&taskId={params['taskId']}"
    )
    payload = {
        "pageNum": 1,
        "pageSize": 20,
        "date": None,
        "defectLevel": "",
        "ruleType": "",
        "filePath": "",
        "fileName": "",
        "defectStatus": "",
        "checkType": "",
        "trigger": "",
        "defectCheckerName": "",
        "isDelay": "",
        "flag": "",
        "projectName": report_vo.get("projectName", params["projectName"]),
        "projectId": params["projectId"],
        "repoUrl": report_vo.get("repoUrl", ""),
        "repoName": report_vo.get("repoNameEn", ""),
        "branchName": report_vo.get("git_branch", ""),
        "ruleName": "",
    }
    body = fetch_json(url, payload=payload)
    if body.get("code") != 200:
        raise SystemExit(f"CodeCheck defect request failed: {body}")
    return body["result"]


def print_codecheck_summary(task):
    params = extract_codecheck_params(task["link"])
    report = fetch_codecheck_report(params)
    report_vo = report.get("reportVo") or {}
    defects = fetch_codecheck_defects(params, report)
    defect_list = defects.get("defects") or []
    print("")
    print(f"Task: {task['task']}")
    print(f"Stage: {task['stage']}")
    print(f"Link: {redact_url(task['link'])}")
    print(f"Issue count: {report_vo.get('issue_count') or 0}")
    print(f"New count: {report_vo.get('new_count') or 0}")
    print(f"Risk coefficient: {report_vo.get('risk_coefficient')}")
    if not defect_list:
        print("No CodeCheck defects returned.")
        return
    for defect in defect_list[:5]:
        fragments = defect.get("fragment") or []
        line_nums = [frag.get("line_num") for frag in fragments if frag.get("line_num")]
        line_hint = line_nums[0] if line_nums else "?"
        file_label = defect.get("gitUrl") or defect.get("fileName") or defect.get("filepath") or ""
        print(f"- {file_label}:{line_hint} {redact_text(defect.get('ruleName', ''))}")
        print(f"  Rule: {redact_text(defect.get('defectCheckerName', ''))}")


def extract_sca_scan_id(detail_link: str):
    parsed = urllib.parse.urlparse(detail_link)
    parts = [part for part in parsed.path.split("/") if part]
    try:
        idx = parts.index("personalScandTaskInfor")
        return parts[idx + 2]
    except (ValueError, IndexError):
        raise SystemExit(f"Unsupported SCA detail link: {detail_link}")


def fetch_sca_scan_info(scan_id: str):
    url = f"https://www.openlibing.com/gateway/openlibing-sca/open/person/scan/id?scanId={urllib.parse.quote(scan_id)}"
    body = fetch_json(url)
    if body.get("code") != 200:
        raise SystemExit(f"SCA scan info request failed: {body}")
    return body["data"]


def fetch_sca_issues(scan_id: str):
    url = "https://www.openlibing.com/gateway/openlibing-sca/open/scan/scanIssue/query"
    payload = {"scanId": scan_id, "pageNo": 1, "pageSize": 20}
    body = fetch_json(url, payload=payload)
    if body.get("code") != 200:
        raise SystemExit(f"SCA issues request failed: {body}")
    return body["data"]


def print_sca_summary(task):
    scan_id = extract_sca_scan_id(task["link"])
    info = fetch_sca_scan_info(scan_id)
    data = fetch_sca_issues(scan_id)
    issues = data.get("list") or []
    print("")
    print(f"Task: {task['task']}")
    print(f"Stage: {task['stage']}")
    print(f"Link: {redact_url(task['link'])}")
    print(f"Scan result: {info.get('scanResult')}")
    print(f"Scanned files: {info.get('fileNum')}")
    print(f"Issue total: {data.get('total') or 0}")
    if not issues:
        print("No SCA issues returned.")
        return
    for issue in issues[:5]:
        print(
            f"- {issue.get('fileName')}: {issue.get('type')} {issue.get('matched')} "
            f"{issue.get('vendor')}/{issue.get('component')} {issue.get('version')}"
        )
        print(f"  licenseStatus={issue.get('licenseStatus')} copyrightStatus={issue.get('copyrightStatus')}")


def print_block_summary(block):
    print(f"Pipeline: {block['name']}")
    print(f"State: {block['state']}")
    print(f"Link: {redact_url(block['link'])}")
    print("")
    for row in block["rows"]:
        print(f"[{row['status']}] {row['stage']} / {row['task']}")
        print(f"  {redact_url(row['link'])}")


def print_log_excerpt(task, data):
    print("")
    print(f"Selected task: {task['task']}")
    print(f"Task state: {task['status']}")
    print(f"Task link: {redact_url(task['link'])}")
    print("")
    print("Log page info:")
    print(
        f"  has_more={data.get('has_more')} start_offset={data.get('start_offset')} end_offset={data.get('end_offset')}"
    )
    print("")
    print(redact_text(data.get("log", "")[:12000]))


def print_task_kind_summary(task):
    kind = get_task_kind(task)
    try:
        if kind == "codecheck":
            print_codecheck_summary(task)
            return True
        if kind == "sca":
            print_sca_summary(task)
            return True
    except EXPECTED_FETCH_ERRORS as exc:
        print(f"Unable to resolve {kind} task detail: {exc}")
        return True
    return False


def print_failed_task_summaries(block, detail=None, detail_error=None):
    failed_tasks = find_failed_tasks(block)
    if not failed_tasks:
        print("")
        print("No failed tasks found in the latest pipeline comment.")
        return False

    print("")
    print("Failed task summaries:")
    incomplete = False
    for task in failed_tasks:
        if print_task_kind_summary(task):
            continue
        print("")
        print(f"Task: {task['task']}")
        print(f"Stage: {task['stage']}")
        print(f"Link: {redact_url(task['link'])}")
        params = None
        try:
            params = extract_params(task["link"])
        except SystemExit:
            if detail:
                try:
                    params = resolve_task_params_from_detail(task, block, detail)
                except SystemExit:
                    params = None
        if not params:
            if detail_error:
                print(f"Unable to resolve task log parameters: {detail_error}")
            else:
                print("Unable to resolve task log parameters.")
            continue
        try:
            needle, excerpt = find_failure_excerpt(params)
        except EXPECTED_FETCH_ERRORS as exc:
            print(f"Unable to fetch task log for {task['task']}: {redact_text(str(exc))}")
            incomplete = True
            continue
        if not excerpt:
            print("No failure excerpt found in fetched log pages.")
            continue
        print(f"Failure excerpt trigger: {needle}")
        print(redact_text(excerpt[:8000]))
    if incomplete:
        print("")
        print("Failed task summary is incomplete: some task logs could not be fetched.")
    return True


def main():
    stdout_encoding = getattr(sys.stdout, "encoding", None)
    if stdout_encoding and str(stdout_encoding).lower() != "utf-8":
        try:
            sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        except (AttributeError, io.UnsupportedOperation):
            pass
    stderr_encoding = getattr(sys.stderr, "encoding", None)
    if stderr_encoding and str(stderr_encoding).lower() != "utf-8":
        try:
            sys.stderr.reconfigure(encoding="utf-8", errors="replace")
        except (AttributeError, io.UnsupportedOperation):
            pass
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", required=True)
    parser.add_argument("--pr", type=int, required=True)
    parser.add_argument("--task")
    parser.add_argument("--summary-only", action="store_true")
    args = parser.parse_args()

    analysis = analyze_pr(args.repo, args.pr)
    block = analysis["block"]
    if not block:
        if analysis["comment_count"] == 0:
            raise SystemExit("No comments returned for PR")
        raise SystemExit("No pipeline table found in PR comments")
    detail = analysis["detail"]
    detail_error = analysis["detail_error"]
    if detail_error:
        print(f"Pipeline detail unavailable: {detail_error}")

    print_block_summary(block)
    if block.get("format") == "actions":
        print("")
        print("GitCode Actions run detected; task and log details are not parsed.")
        return
    if args.summary_only:
        return

    if not args.task:
        found_failed = print_failed_task_summaries(block, detail, detail_error)
        if found_failed:
            return
        running_task = find_task(block)
        if not running_task:
            print("")
            print("No failed or running tasks found in the latest pipeline comment.")
            return

    task = find_task(block, args.task)
    if not task:
        raise SystemExit("No task rows found in latest pipeline comment")
    if print_task_kind_summary(task):
        return
    try:
        params = extract_params(task["link"])
    except SystemExit as exc:
        if not detail:
            message = f"Unable to resolve task log parameters: {detail_error or exc}"
            raise SystemExit(message) from exc
        params = resolve_task_params_from_detail(task, block, detail)
    data = fetch_exec_log(params)
    print_log_excerpt(task, data)


if __name__ == "__main__":
    main()
