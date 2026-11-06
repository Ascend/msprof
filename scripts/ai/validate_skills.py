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
"""校验 msprof AI 资产：Skill frontmatter、命名、索引覆盖和 Markdown 相对链接。"""

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SKILLS_DIR = ROOT / ".agents" / "skills"
AGENTS_FILE = ROOT / "AGENTS.md"
INDEX_FILE = ROOT / ".agents" / "README.md"

MANDATORY_FILES = [
    "AGENTS.md",
    "CLAUDE.md",
    ".agents/README.md",
    ".agents/references/pr-context.md",
    ".agents/references/line-mapping.md",
    "docs/zh/development_guide/msprof_ai_native_development_guide.md",
]

LINK_RE = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")
SKILL_LINK_RE = re.compile(r"\]\((?:\.agents/)?skills/([a-z0-9-]+)/SKILL\.md\)")
NAME_RE = re.compile(r"^[a-z0-9]+(?:-[a-z0-9]+)*$")
SEMVER_RE = re.compile(r"^\d+\.\d+\.\d+$")
BLOCK_SCALAR_RE = re.compile(r"^[|>](?:[1-9])?[+-]?$")


def read_text(path):
    return path.read_text(encoding="utf-8")


def parse_frontmatter(text):
    lines = text.splitlines()
    if not lines or lines[0].strip() != "---":
        return None, "缺少 YAML frontmatter"
    end = None
    for index in range(1, len(lines)):
        if lines[index].strip() == "---":
            end = index
            break
    if end is None:
        return None, "frontmatter 未闭合"

    data = {}
    current = None
    block_key = None
    block_style = None
    block_lines = []

    def flush_block():
        nonlocal block_key, block_style, block_lines
        if block_key is None:
            return
        if block_style.startswith(">"):
            value = " ".join(part for part in (line.strip() for line in block_lines) if part)
        else:
            value = "\n".join(line.strip() for line in block_lines)
        data[block_key] = value.strip()
        block_key = None
        block_style = None
        block_lines = []

    for raw in lines[1:end]:
        if raw.startswith((" ", "\t")):
            if block_key is not None:
                block_lines.append(raw)
                continue
            if current:
                key, sep, value = raw.strip().partition(":")
                if sep and value.strip():
                    data[current + "." + key.strip()] = value.strip().strip("'\"")
            continue
        if not raw.strip() or raw.lstrip().startswith("#"):
            continue
        flush_block()
        key, sep, value = raw.partition(":")
        if not sep:
            continue
        key = key.strip()
        value = value.strip().strip("'\"")
        if value and BLOCK_SCALAR_RE.match(value):
            block_key = key
            block_style = value
            block_lines = []
            current = None
        elif value:
            data[key] = value
            current = None
        else:
            current = key
    flush_block()
    return data, None


def markdown_files():
    files = set()
    for rel in MANDATORY_FILES + ["README.md"]:
        path = ROOT / rel
        if path.is_file():
            files.add(path)
    for folder in (SKILLS_DIR, ROOT / ".agents" / "references"):
        if folder.is_dir():
            files.update(sorted(folder.rglob("*.md")))
    return sorted(files)


def index_mentions(text, name):
    for line in text.splitlines():
        if not line.lstrip().startswith("|"):
            continue
        for match in SKILL_LINK_RE.finditer(line):
            if match.group(1) == name:
                return True
    return False


def check_links(path):
    errors = []
    fence = None
    for lineno, line in enumerate(read_text(path).splitlines(), 1):
        stripped = line.lstrip()
        if stripped.startswith(("```", "~~~")):
            marker = stripped[:3]
            if fence is None:
                fence = marker
            elif fence == marker:
                fence = None
            continue
        if fence is not None:
            continue
        for raw in LINK_RE.findall(line):
            target = raw.strip()
            if target.startswith("<") and target.endswith(">"):
                target = target[1:-1]
            if target.startswith(("http://", "https://", "mailto:", "#")):
                continue
            target = target.split("#", 1)[0].strip()
            if not target:
                continue
            target = target.split(" ", 1)[0]
            target_path = (path.parent / target).resolve()
            if not target_path.exists():
                errors.append("%s:%d: 链接目标不存在: %s" % (path.relative_to(ROOT).as_posix(), lineno, raw.strip()))
    return errors


def check_skills():
    errors = []
    names = []
    if not SKILLS_DIR.is_dir():
        return names, ["缺少 .agents/skills/ 目录"]

    for entry in sorted(SKILLS_DIR.iterdir()):
        if not entry.is_dir():
            continue
        skill_file = entry / "SKILL.md"
        if not skill_file.is_file():
            errors.append(".agents/skills/%s: 缺少 SKILL.md" % entry.name)
            continue

        data, error = parse_frontmatter(read_text(skill_file))
        if error:
            errors.append(".agents/skills/%s/SKILL.md: %s" % (entry.name, error))
            continue

        name = data.get("name", "")
        if not name:
            errors.append(".agents/skills/%s/SKILL.md: 缺少 name" % entry.name)
        else:
            if not NAME_RE.match(name):
                errors.append(".agents/skills/%s/SKILL.md: name 必须为 kebab-case" % entry.name)
            if name != entry.name:
                errors.append(".agents/skills/%s/SKILL.md: name `%s` 与目录名不一致" % (entry.name, name))
            names.append(name)

        description = data.get("description", "")
        if len(description) < 20:
            errors.append(".agents/skills/%s/SKILL.md: description 过短或缺失" % entry.name)

        version = data.get("metadata.version", "")
        if not SEMVER_RE.match(version):
            errors.append(".agents/skills/%s/SKILL.md: metadata.version 缺失或非 x.y.z" % entry.name)

        if data.get("metadata.repository", "") != "Ascend/msprof":
            errors.append(".agents/skills/%s/SKILL.md: metadata.repository 必须为 Ascend/msprof" % entry.name)

    return names, errors


def check_index(names):
    errors = []
    if not INDEX_FILE.is_file():
        return [".agents/README.md: 文件不存在"]
    index_text = read_text(INDEX_FILE)
    for name in names:
        if not index_mentions(index_text, name):
            errors.append(".agents/README.md: Skill 索引缺少 `%s`" % name)

    if not AGENTS_FILE.is_file():
        return errors
    agents_text = read_text(AGENTS_FILE)
    for name in names:
        if not index_mentions(agents_text, name):
            errors.append("AGENTS.md: 意图路由或资产目录缺少 `%s`" % name)
    return errors


def main(argv=None):
    parser = argparse.ArgumentParser(description="校验 msprof AI 资产")
    parser.add_argument("--json", action="store_true", help="以 JSON 输出结果")
    args = parser.parse_args(argv)

    errors = []
    for rel in MANDATORY_FILES:
        if not (ROOT / rel).is_file():
            errors.append("%s: 必需文件不存在" % rel)

    names, skill_errors = check_skills()
    errors.extend(skill_errors)
    errors.extend(check_index(names))

    files = markdown_files()
    for path in files:
        errors.extend(check_links(path))

    result = {
        "ok": not errors,
        "checked_files": len(files),
        "skills": names,
        "errors": errors,
    }

    if args.json:
        print(json.dumps(result, ensure_ascii=False, indent=2))
    elif errors:
        print("AI 资产校验失败（%d 项）：" % len(errors))
        for error in errors:
            print("  - %s" % error)
    else:
        print("AI 资产校验通过：%d 个 Skill，%d 个 Markdown 文件。" % (len(names), len(files)))

    return 0 if not errors else 1


if __name__ == "__main__":
    sys.exit(main())
