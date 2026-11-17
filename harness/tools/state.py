#!/usr/bin/env python3
"""The facts a session resumes from, derived rather than remembered.

    env/venv/bin/python harness/tools/state.py

Prints: master and the main checkout's uncommitted files; the branches with commits master lacks that still have a
worktree (and how many more have none); this project's OMP subagents that are running (a session file with no report
yet, written to in the last 20 minutes); our running jobs (by command, with cwd in this repository); every IDA
database copy under env/ other than the canonical and its snapshots. The .omp/hooks/post/session-state.ts hook shows
it to the main session.
"""
from __future__ import annotations

import os
import pwd
import re
import subprocess
import time

ROOT = os.path.realpath(os.path.join(os.path.dirname(__file__), "..", ".."))
MAIN_SESSION = re.compile(r"^\d{4}-\d\d-\d\dT[\d-]+Z_[\w-]+$")
JOBS = re.compile(r"idakit|srcdb index|cmake --build|run\.sh")
RUNNING_S = 20 * 60


def git(*args: str, strip: bool = True) -> str:
    out = subprocess.run(["git", "-C", ROOT, *args], capture_output=True, text=True).stdout
    return out.strip() if strip else out


def age(seconds: float) -> str:
    return f"{int(seconds // 60)}m" if seconds < 3600 else f"{seconds / 3600:.1f}h"


def checkout() -> list[str]:
    out = [f"master {git('log', '-1', '--format=%h %s', 'master')[:110]}"]
    status = git("status", "--porcelain", "--untracked-files=normal", strip=False)
    changed = [line[3:] for line in status.splitlines() if line]
    shown = ": " + ", ".join(changed[:6]) + (f" (+{len(changed) - 6})" if len(changed) > 6 else "") if changed else ""
    out.append(f"main checkout: {len(changed)} uncommitted{shown}")
    return out


def branches() -> list[str]:
    trees = {}
    for block in git("worktree", "list", "--porcelain").split("\n\n"):
        fields = dict(line.split(" ", 1) for line in block.splitlines() if " " in line)
        if fields.get("branch", "").startswith("refs/heads/"):
            trees[fields["branch"][len("refs/heads/"):]] = os.path.relpath(fields["worktree"], ROOT)
    out, bare = [], 0
    for name in git("branch", "--format=%(refname:short)").splitlines():
        if name == "master":
            continue
        counts = git("rev-list", "--left-right", "--count", f"{name}...master").split()
        if len(counts) != 2 or counts[0] == "0":
            continue
        if name not in trees:
            bare += 1
            continue
        when = git("log", "-1", "--format=%cr", name)
        out.append(f"  {name}: +{counts[0]}/-{counts[1]}, {trees[name]}, last commit {when}")
    more = f" ({bare} more have no worktree)" if bare else ""
    return [f"unmerged branches with a worktree: {len(out)}{more}"] + out


def agents() -> list[str]:
    home = pwd.getpwuid(os.getuid()).pw_dir
    sessions = os.path.join(home, ".omp", "agent", "sessions", "--" + ROOT.strip("/").replace("/", "-") + "--")
    now, running = time.time(), []
    if os.path.isdir(sessions):
        for parent in os.listdir(sessions):
            folder = os.path.join(sessions, parent)
            if not MAIN_SESSION.match(parent) or not os.path.isdir(folder) or now - os.path.getmtime(folder) > 86400:
                continue
            for entry in os.listdir(folder):
                if not entry.endswith(".jsonl") or entry.startswith("__"):
                    continue
                path = os.path.join(folder, entry)
                if not os.path.exists(path[:-6] + ".md") and now - os.path.getmtime(path) < RUNNING_S:
                    running.append(entry[:-6])
    return [f"agents running: {len(running)}" + (f": {', '.join(sorted(running))}" if running else "")]


def jobs() -> list[str]:
    ps = subprocess.run(["ps", "-u", str(os.getuid()), "-o", "pid=,ppid=,etimes=,args="], capture_output=True,
                        text=True)
    found = {}
    for line in ps.stdout.splitlines():
        pid, ppid, elapsed, args = line.strip().split(None, 3)
        if not JOBS.search(args) or "state.py" in args:
            continue
        try:
            cwd = os.path.realpath(os.readlink(f"/proc/{pid}/cwd"))
        except OSError:
            continue
        if cwd == ROOT or cwd.startswith(ROOT + "/"):
            found[pid] = (ppid, f"  {pid} {age(int(elapsed))} {os.path.relpath(cwd, ROOT)}: {args[:110]}")
    # A job's own workers (index-tu, ninja's compilers) are not jobs of their own.
    out = [text for pid, (ppid, text) in found.items() if ppid not in found]
    return [f"jobs running: {len(out)}"] + out


def copies() -> list[str]:
    env = os.path.join(ROOT, "env")
    canonical = os.path.join(env, "ida", "FalloutNV.exe.i64")
    snapshots = os.path.join(env, "ida", "snapshots")
    now, found = time.time(), []
    for folder, dirs, files in os.walk(env):
        if folder == snapshots:
            dirs[:] = []
            continue
        for name in files:
            path = os.path.join(folder, name)
            if name.endswith(".i64") and path != canonical and not os.path.islink(path):
                found.append((path, os.path.getsize(path), os.path.getmtime(path)))
    if not found:
        return []
    out = [f"database copies: {len(found)}"]
    return out + [f"  {os.path.relpath(p, ROOT)}  {size / 2**20:.0f} MB  {age(now - mtime)}" for p, size, mtime in found]


def main() -> int:
    print("\n".join(checkout() + branches() + agents() + jobs() + copies()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
