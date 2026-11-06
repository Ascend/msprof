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

import importlib.util
import pathlib
import tempfile
import unittest
from unittest import mock


def _load_module():
    root = next(parent for parent in pathlib.Path(__file__).resolve().parents if (parent / ".agents").is_dir())
    path = root / "scripts" / "ai" / "validate_skills.py"
    spec = importlib.util.spec_from_file_location("validate_skills", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


VALIDATE = _load_module()


def _write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


class ValidateSkillsTest(unittest.TestCase):
    def test_parse_frontmatter_block_scalar(self):
        text = (
            "---\n"
            "name: test-skill\n"
            "description: >\n"
            "  first line\n"
            "  second line\n"
            "metadata:\n"
            "  version: 1.0.0\n"
            "  repository: Ascend/msprof\n"
            "---\n"
        )
        data, error = VALIDATE.parse_frontmatter(text)
        self.assertIsNone(error)
        self.assertEqual(data["description"], "first line second line")
        self.assertEqual(data["metadata.version"], "1.0.0")

    def test_parse_frontmatter_literal_block_scalar(self):
        text = "---\nname: test-skill\ndescription: |\n  first\n  second\n---\n"
        data, error = VALIDATE.parse_frontmatter(text)
        self.assertIsNone(error)
        self.assertEqual(data["description"], "first\nsecond")

    def test_check_index_requires_table_link(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            index = root / "README.md"
            agents = root / "AGENTS.md"
            _write(index, "我们使用 msprof-dev 做开发。\n")
            _write(agents, "| 开发 | [msprof-dev](.agents/skills/msprof-dev/SKILL.md) |\n")
            with mock.patch.object(VALIDATE, "INDEX_FILE", index):
                with mock.patch.object(VALIDATE, "AGENTS_FILE", agents):
                    errors = VALIDATE.check_index(["msprof-dev"])
            self.assertTrue(any("README.md" in error for error in errors))

            _write(index, "| Skill | 说明 |\n| --- | --- |\n| [msprof-dev](skills/msprof-dev/SKILL.md) | 开发 |\n")
            with mock.patch.object(VALIDATE, "INDEX_FILE", index):
                with mock.patch.object(VALIDATE, "AGENTS_FILE", agents):
                    errors = VALIDATE.check_index(["msprof-dev"])
            self.assertEqual(errors, [])

    def test_check_links_skips_fenced_code(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            doc = root / "docs" / "doc.md"
            _write(
                doc,
                "[ok](missing.md)\n\n```\n[示例](missing.md)\n```\n",
            )
            with mock.patch.object(VALIDATE, "ROOT", root):
                errors = VALIDATE.check_links(doc)
            self.assertEqual(len(errors), 1)
            self.assertIn(":1:", errors[0])


if __name__ == "__main__":
    unittest.main()
