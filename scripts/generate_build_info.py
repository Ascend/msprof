#!/usr/bin/env python3
import argparse
import json
import os
import platform
import re
import socket
import subprocess
import sys
import sysconfig
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional


def run_git(root: Path, *args: str) -> Optional[str]:
    try:
        result = subprocess.run(["git", *args], cwd=root, capture_output=True, text=True, check=False)
    except OSError:
        return None
    return result.stdout.strip() if result.returncode == 0 else None


def get_git_info(root: Path) -> dict:
    commit = run_git(root, "rev-parse", "HEAD")
    if commit is None:
        return {
            "commit": None,
            "commit_short": None,
            "branch": None,
            "tag": None,
            "dirty": None,
            "commit_time": None,
        }

    status = run_git(root, "status", "--porcelain", "--untracked-files=normal")
    return {
        "commit": commit,
        "commit_short": run_git(root, "rev-parse", "--short=12", "HEAD"),
        "branch": run_git(root, "symbolic-ref", "--short", "-q", "HEAD"),
        "tag": run_git(root, "describe", "--tags", "--always"),
        "dirty": bool(status),
        "commit_time": run_git(root, "show", "-s", "--format=%cI", "HEAD"),
    }


def get_compiler() -> str:
    try:
        result = subprocess.run(["gcc", "--version"], capture_output=True, text=True, check=False)
    except OSError:
        return "unknown"
    if result.returncode != 0:
        return "unknown"

    first_line = (result.stdout or result.stderr).splitlines()
    if not first_line:
        return "unknown"
    line = first_line[0]
    versions = re.findall(r"\b[0-9]+\.[0-9]+(?:\.[0-9]+)?\b", line)
    return f"gcc {versions[-1]}" if versions else "unknown"


def get_platform_tag() -> str:
    machine = platform.machine().lower()
    machine = {"amd64": "x86_64", "arm64": "aarch64"}.get(machine, machine)
    libc_name, libc_version = platform.libc_ver()
    version = re.match(r"(\d+)\.(\d+)", libc_version)
    if version and libc_name.lower() == "glibc":
        return f"manylinux_{version.group(1)}_{version.group(2)}_{machine}"
    if version and libc_name.lower() == "musl":
        return f"musllinux_{version.group(1)}_{version.group(2)}_{machine}"
    return f"{sysconfig.get_platform().replace('-', '_')}"


def get_python_abi() -> str:
    implementation = sys.implementation.name
    prefix = "cp" if implementation == "cpython" else implementation[:2]
    return f"{prefix}{sys.version_info.major}{sys.version_info.minor}"


def get_source_date_epoch() -> Optional[int]:
    value = os.environ.get("SOURCE_DATE_EPOCH", "")
    return int(value) if value.isdigit() else None


def main() -> None:
    parser = argparse.ArgumentParser(description="Write build metadata for the msprof run package.")
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--package-name", required=True)
    parser.add_argument("--package-version", required=True)
    parser.add_argument("--build-type", required=True)
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[1]
    build_time = datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")
    build_info = {
        "schema": 1,
        "package": {"name": args.package_name, "version": args.package_version},
        "git": get_git_info(root),
        "build": {
            "time": build_time,
            "host": socket.gethostname(),
            "compiler": get_compiler(),
            "build_type": args.build_type,
            "source_date_epoch": get_source_date_epoch(),
        },
        "abi": {"python": get_python_abi(), "platform": get_platform_tag()},
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(build_info, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
