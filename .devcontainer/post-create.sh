#!/bin/bash
# -*- coding: utf-8 -*-
# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#           http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------
# -------------------------------------------------------------------------
# 容器首次创建后的幂等初始化脚本 (msprof)
# 所有动作必须幂等，失败不阻塞容器创建
# 不使用 set -e，每个步骤独立处理错误
# -------------------------------------------------------------------------

log() { echo "[post-create] $*"; }
warn() { echo "[post-create] WARN: $*"; }

# ──────────────────────────────────────────────────────────────────────────────
# 1. 用户级命令目录
# ──────────────────────────────────────────────────────────────────────────────
configure_user_bin() {
    log "Configuring user bin directory..."
    mkdir -p "$HOME/.local/bin"
    npm config set prefix "$HOME/.local" 2>/dev/null || warn "npm config set prefix failed"

    local marker="# msprof-devcontainer-user-bin"
    for rcfile in "$HOME/.bashrc" "$HOME/.bash_profile"; do
        if [ -f "$rcfile" ] && ! grep -qF "$marker" "$rcfile"; then
            cat >> "$rcfile" <<EOF
$marker
export PATH="\$HOME/.local/bin:\$PATH"
EOF
            log "Appended PATH to $rcfile"
        fi
    done
}

# ──────────────────────────────────────────────────────────────────────────────
# 2. Python 3 (最低要求 3.8)
# ──────────────────────────────────────────────────────────────────────────────
configure_python3() {
    log "Configuring Python 3..."
    if [ -f "/etc/profile.d/pyenv.sh" ]; then
        source /etc/profile.d/pyenv.sh 2>/dev/null || warn "Failed to source pyenv.sh"
        log "Loaded pyenv profile"
    fi

    # 如果存在 pyenv 管理的 Python，将其优先于系统 python3
    for candidate in /opt/python/cp*-cp*; do
        if [ -d "$candidate/bin" ] && [ -x "$candidate/bin/python3" ]; then
            local pyenv_python="$candidate/bin"
            log "Python (pyenv) found at $pyenv_python"

            local marker="# msprof-pyenv-python"
            for rcfile in "$HOME/.bashrc" "$HOME/.bash_profile"; do
                if [ -f "$rcfile" ] && ! grep -qF "$marker" "$rcfile"; then
                    cat >> "$rcfile" <<EOF
$marker
export PATH="$pyenv_python:\$PATH"
EOF
                    log "Prepended pyenv Python to PATH in $rcfile"
                fi
            done
            export PATH="$pyenv_python:$PATH"
            break
        fi
    done

    if command -v python3 &>/dev/null; then
        log "Python 3 found: $(python3 --version 2>&1)"
    else
        warn "python3 not found in PATH"
    fi
}

# ──────────────────────────────────────────────────────────────────────────────
# 3. 安装编译和测试依赖 (幂等)
# ──────────────────────────────────────────────────────────────────────────────
install_build_deps() {
    log "Installing system build dependencies..."

    # --- 系统包 (dnf) ---
    if command -v dnf &>/dev/null; then
        local sys_pkgs=(
            python3-devel
        )
        for pkg in "${sys_pkgs[@]}"; do
            if ! rpm -q "$pkg" &>/dev/null; then
                log "  Installing: $pkg"
                sudo dnf install -y "$pkg" 2>/dev/null || warn "dnf install failed: $pkg"
            else
                log "  System pkg OK: $pkg"
            fi
        done
    elif command -v apt-get &>/dev/null; then
        local sys_pkgs=(
            python3-dev
        )
        for pkg in "${sys_pkgs[@]}"; do
            if ! dpkg -s "$pkg" &>/dev/null; then
                log "  Installing: $pkg"
                sudo apt-get update -qq && sudo apt-get install -y "$pkg" 2>/dev/null || warn "apt-get install failed: $pkg"
            else
                log "  System pkg OK: $pkg"
            fi
        done
    fi

    # gitleaks 用于 pre-commit 密钥扫描，根据架构自动选择下载
    if ! command -v gitleaks &>/dev/null; then
        log "  Installing gitleaks..."
        GITLEAKS_VER="8.18.4"
        case "$(uname -m)" in
            x86_64)  GITLEAKS_ARCH="amd64";;
            aarch64) GITLEAKS_ARCH="arm64";;
            *)       warn "Unsupported architecture $(uname -m), skipping gitleaks"; return;;
        esac
        GITLEAKS_INSTALLED=false
        for MIRROR in \
            "https://ghproxy.com/https://github.com/gitleaks/gitleaks/releases/download/v${GITLEAKS_VER}/gitleaks_${GITLEAKS_VER}_linux_${GITLEAKS_ARCH}.tar.gz" \
            "https://mirror.ghproxy.com/https://github.com/gitleaks/gitleaks/releases/download/v${GITLEAKS_VER}/gitleaks_${GITLEAKS_VER}_linux_${GITLEAKS_ARCH}.tar.gz" \
            "https://github.com/gitleaks/gitleaks/releases/download/v${GITLEAKS_VER}/gitleaks_${GITLEAKS_VER}_linux_${GITLEAKS_ARCH}.tar.gz"; do
            log "    Trying: ${MIRROR}"
            curl -fsSL "${MIRROR}" -o /tmp/gitleaks.tar.gz --connect-timeout 10 2>/dev/null && \
                sudo tar -xzf /tmp/gitleaks.tar.gz -C /usr/local/bin gitleaks 2>/dev/null && \
                GITLEAKS_INSTALLED=true && break
            rm -f /tmp/gitleaks.tar.gz
        done
        if ${GITLEAKS_INSTALLED}; then
            sudo chmod +x /usr/local/bin/gitleaks
            log "  gitleaks ${GITLEAKS_VER} (${GITLEAKS_ARCH}) installed"
        else
            warn "gitleaks install failed (all mirrors unreachable)"
        fi
    else
        log "  System pkg OK: gitleaks"
    fi

    # --- pip 包 ---
    local pip_pkgs=(
        packaging
        wheel
        pytest
        coverage
        pre-commit
        "bandit[toml]"
    )

    for PY in $(command -v python3 2>/dev/null) $(command -v python 2>/dev/null); do
        log "Installing pip packages for: $($PY --version 2>&1)"
        "$PY" -m pip install --quiet --upgrade pip setuptools >/dev/null 2>&1 || warn "pip/setuptools upgrade failed for $PY"

        for pkg in "${pip_pkgs[@]}"; do
            # pip show 不解析 extras（如 bandit[toml]），用裸包名检查
            local pkg_name="${pkg%%[*}"
            if ! "$PY" -m pip show "$pkg_name" &>/dev/null; then
                log "  Installing for $PY: $pkg"
                "$PY" -m pip install "$pkg" || warn "pip install failed for $PY: $pkg"
            else
                log "  Pip pkg OK ($PY): $pkg"
            fi
        done
    done

    log "Build dependencies check complete"
}

# ──────────────────────────────────────────────────────────────────────────────
# 4. Git 身份同步
# ──────────────────────────────────────────────────────────────────────────────
sync_git_identity() {
    log "Syncing Git identity..."
    local gitconfig="$HOME/.devcontainer-host-gitconfig"
    if [ -f "$gitconfig" ] && [ -s "$gitconfig" ]; then
        local name email
        name=$(git config --file "$gitconfig" --get user.name 2>/dev/null) || true
        email=$(git config --file "$gitconfig" --get user.email 2>/dev/null) || true
        [ -n "$name" ] && git config --global user.name "$name"
        [ -n "$email" ] && git config --global user.email "$email"
        log "Git identity synced from host"
    else
        warn "No host Git config found, skipping identity sync"
    fi
}

# ──────────────────────────────────────────────────────────────────────────────
# 5. 开发命令提示
# ──────────────────────────────────────────────────────────────────────────────
append_dev_hint_once() {
    local marker="# msprof-dev-hint"
    local rcfile="$HOME/.bashrc"
    if grep -qF "$marker" "$rcfile" 2>/dev/null; then return; fi

    cat >> "$rcfile" <<EOF
$marker
# msprof development commands:
#   python3 build.py              Build Release (full)
#   python3 build.py local        Build Release (skip deps)
#   python3 build.py test         Build and run unit tests
#   python3 build.py test local   Run unit tests (skip deps)
EOF
    log "Appended dev hints to $rcfile"
}

# ──────────────────────────────────────────────────────────────────────────────
# 6. pre-commit 自动安装
# ──────────────────────────────────────────────────────────────────────────────
install_pre_commit_hook() {
    log "Installing pre-commit hook..."
    export PATH="$HOME/.local/bin:$PATH"

    local pre_commit_cmd=""
    if command -v pre-commit &>/dev/null; then
        pre_commit_cmd="pre-commit"
    elif python3 -m pre_commit --version &>/dev/null 2>&1; then
        pre_commit_cmd="python3 -m pre_commit"
    elif python -m pre_commit --version &>/dev/null 2>&1; then
        pre_commit_cmd="python -m pre_commit"
    else
        warn "pre-commit not found, skipping hook installation"
        return
    fi

    if ! git rev-parse --git-dir &>/dev/null; then
        warn "Not a Git repository, skipping pre-commit hook"
        return
    fi

    $pre_commit_cmd install || warn "pre-commit install failed"
    log "pre-commit hook installed via: $pre_commit_cmd"
}

# ──────────────────────────────────────────────────────────────────────────────
# 7. clangd
# ──────────────────────────────────────────────────────────────────────────────
setup_clangd() {
    log "Setting up clangd..."
    if command -v clangd &>/dev/null; then
        log "clangd found: $(clangd --version 2>&1 | head -1)"
        return
    fi
    warn "clangd not found, attempting to install..."
    if command -v dnf &>/dev/null; then
        sudo dnf install -y clangd 2>/dev/null || warn "Failed to install clangd via dnf"
    elif command -v apt-get &>/dev/null; then
        sudo apt-get update -qq && sudo apt-get install -y clangd 2>/dev/null || warn "Failed to install clangd via apt-get"
    else
        warn "Cannot install clangd automatically"
    fi
}

# ──────────────────────────────────────────────────────────────────────────────
# 8. 忽略本地可修改的文件
# ──────────────────────────────────────────────────────────────────────────────
ignore_local_changes() {
    log "Setting up skip-worktree for local-modifiable files..."
    if [ -f ".vscode/settings.json" ]; then
        git update-index --skip-worktree .vscode/settings.json 2>/dev/null || true
        log "  skip-worktree: .vscode/settings.json"
    fi
    if [ -f "version.info" ]; then
        git update-index --skip-worktree version.info 2>/dev/null || true
        log "  skip-worktree: version.info"
    fi
}

# ──────────────────────────────────────────────────────────────────────────────
# 9. compile_commands.json 提示
# ──────────────────────────────────────────────────────────────────────────────
check_compile_commands() {
    if [ ! -f "build/compile_commands.json" ]; then
        warn "build/compile_commands.json not found (expected on cold start)"
        warn "Run 'python3 build.py' to generate it for clangd support"
    else
        log "compile_commands.json found, clangd ready"
    fi
}

# ──────────────────────────────────────────────────────────────────────────────
# 主流程
# ──────────────────────────────────────────────────────────────────────────────
log "Starting container initialization (msprof)..."

configure_user_bin
configure_python3
install_build_deps
sync_git_identity
append_dev_hint_once
install_pre_commit_hook
setup_clangd
ignore_local_changes
check_compile_commands

log "Container initialization complete!"
