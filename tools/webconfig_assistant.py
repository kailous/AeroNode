#!/usr/bin/env python3
"""Build cemuhook/WebConfig.h from a source HTML file.

- Detects repo URL from git remote.
- Converts relative JS/CSS URLs to raw GitHub URLs.
- Backs up the original WebConfig.h before writing.
"""

from __future__ import annotations

import datetime as _dt
from pathlib import Path
import re
import subprocess
import sys
from typing import Optional


REPO_ROOT_FALLBACK = Path.cwd()
DEFAULT_HTML = Path("WebPlugins/phoenix_debug.html")
DEFAULT_OUTPUT = Path("cemuhook/WebConfig.h")


def _run_git(args: list[str]) -> str:
    result = subprocess.run(
        ["git"] + args,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "git command failed")
    return result.stdout.strip()


def _get_repo_root() -> Path:
    try:
        root = _run_git(["rev-parse", "--show-toplevel"])
        return Path(root)
    except Exception:
        return REPO_ROOT_FALLBACK


def _get_origin_url() -> str:
    return _run_git(["remote", "get-url", "origin"])


def _get_default_branch() -> str:
    try:
        ref = _run_git(["symbolic-ref", "refs/remotes/origin/HEAD"])
        # refs/remotes/origin/main -> main
        return ref.rsplit("/", 1)[-1]
    except Exception:
        return _run_git(["rev-parse", "--abbrev-ref", "HEAD"])


def _parse_github_repo(url: str) -> tuple[str, str]:
    """Return (owner, repo) for a GitHub remote URL."""
    # https://github.com/owner/repo(.git)
    https_match = re.match(r"https://github.com/([^/]+)/([^/]+?)(?:\.git)?$", url)
    if https_match:
        return https_match.group(1), https_match.group(2)

    # git@github.com:owner/repo(.git)
    ssh_match = re.match(r"git@github.com:([^/]+)/([^/]+?)(?:\.git)?$", url)
    if ssh_match:
        return ssh_match.group(1), ssh_match.group(2)

    # ssh://git@github.com/owner/repo(.git)
    ssh_url_match = re.match(r"ssh://git@github.com/([^/]+)/([^/]+?)(?:\.git)?$", url)
    if ssh_url_match:
        return ssh_url_match.group(1), ssh_url_match.group(2)

    raise ValueError(f"Unsupported or non-GitHub remote: {url}")


def _is_relative_url(url: str) -> bool:
    return not re.match(r"^(?:[a-zA-Z][a-zA-Z0-9+.-]*:|//)", url)


def _rewrite_asset_urls(html: str, html_path: Path, repo_root: Path, raw_base: str) -> str:
    def replace(match: re.Match[str]) -> str:
        attr = match.group("attr")
        url = match.group("url")
        if not (url.endswith(".js") or url.endswith(".css")):
            return match.group(0)
        if not _is_relative_url(url):
            return match.group(0)
        # Resolve relative to HTML file location
        resolved = (html_path.parent / url).resolve()
        try:
            rel = resolved.relative_to(repo_root.resolve())
        except ValueError:
            return match.group(0)
        rel_posix = rel.as_posix()
        return f'{attr}="{raw_base}/{rel_posix}"'

    pattern = re.compile(r'(?P<attr>\b(?:src|href))=["\"](?P<url>[^"\"]+)["\"]')
    return pattern.sub(replace, html)


def _inline_assets(html: str, html_path: Path, repo_root: Path) -> str:
    def inline_css(match: re.Match[str]) -> str:
        url = match.group("url")
        if not _is_relative_url(url):
            return match.group(0)
        resolved = (html_path.parent / url).resolve()
        try:
            resolved.relative_to(repo_root.resolve())
        except ValueError:
            return match.group(0)
        if not resolved.exists():
            return match.group(0)
        css = resolved.read_text(encoding="utf-8")
        return f"<style>\n{css}\n</style>"

    def inline_js(match: re.Match[str]) -> str:
        url = match.group("url")
        if not _is_relative_url(url):
            return match.group(0)
        resolved = (html_path.parent / url).resolve()
        try:
            resolved.relative_to(repo_root.resolve())
        except ValueError:
            return match.group(0)
        if not resolved.exists():
            return match.group(0)
        js = resolved.read_text(encoding="utf-8")
        return f"<script type=\"module\">\n{js}\n</script>"

    # inline CSS links
    html = re.sub(
        r'<link[^>]*rel=["\']stylesheet["\'][^>]*href=["\'](?P<url>[^"\']+)["\'][^>]*>',
        inline_css,
        html,
        flags=re.IGNORECASE,
    )
    # inline module scripts
    html = re.sub(
        r'<script[^>]*type=["\']module["\'][^>]*src=["\'](?P<url>[^"\']+)["\'][^>]*>\s*</script>',
        inline_js,
        html,
        flags=re.IGNORECASE,
    )
    return html


def _backup_file(path: Path, repo_root: Path) -> Optional[Path]:
    if not path.exists():
        return None
    timestamp = _dt.datetime.now().strftime("%Y%m%d-%H%M%S")
    backup_dir = repo_root / "backup"
    backup_dir.mkdir(parents=True, exist_ok=True)
    backup = backup_dir / f"{path.name}.bak-{timestamp}"
    backup.write_bytes(path.read_bytes())
    return backup


def _wrap_as_webconfig(html: str) -> str:
    return (
        "#ifndef WEBCONFIG_H\n"
        "#define WEBCONFIG_H\n\n"
        "#include <pgmspace.h>\n\n"
        "const char index_html[] PROGMEM = R\"rawliteral(\n"
        f"{html}\n"
        ")rawliteral\";\n\n"
        "#endif\n"
    )


def main() -> int:
    repo_root = _get_repo_root()
    html_path = repo_root / (sys.argv[1] if len(sys.argv) > 1 else DEFAULT_HTML)
    output_path = repo_root / (sys.argv[2] if len(sys.argv) > 2 else DEFAULT_OUTPUT)

    if not html_path.exists():
        print(f"Source HTML not found: {html_path}", file=sys.stderr)
        return 1

    origin_url = _get_origin_url()
    owner, repo = _parse_github_repo(origin_url)
    branch = _get_default_branch()
    raw_base = f"https://raw.githubusercontent.com/{owner}/{repo}/{branch}"

    html = html_path.read_text(encoding="utf-8")
    html = _inline_assets(html, html_path, repo_root)
    html = _rewrite_asset_urls(html, html_path, repo_root, raw_base)

    backup = _backup_file(output_path, repo_root)
    output_path.write_text(_wrap_as_webconfig(html), encoding="utf-8")

    print(f"Repo: {origin_url}")
    print(f"Branch: {branch}")
    print(f"Raw base: {raw_base}")
    if backup:
        print(f"Backup: {backup}")
    print(f"Updated: {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
