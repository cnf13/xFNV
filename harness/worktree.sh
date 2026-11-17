#!/bin/bash
# An agent's worktree: env/wt/NAME on branch NAME (made from master when it does not exist), with env/, game/ and
# resources/ linked to the main checkout (one IDA database). The build directory is made by the first
# `harness/tools/noexe.py build` there (a full build).
# Usage: harness/worktree.sh NAME     (again on an existing worktree: only re-checks the links)
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
name=${1:?usage: harness/worktree.sh NAME}
wt=env/wt/$name
if [[ ! -d $wt ]]; then
    mkdir -p env/wt
    if git show-ref --verify --quiet "refs/heads/$name"; then
        git worktree add "$wt" "$name"
    else
        git worktree add -b "$name" "$wt" master
    fi
fi
for l in env game resources; do
    [[ -L $wt/$l ]] || ln -s "$root/$l" "$wt/$l"
done
echo "worktree: $wt on branch $name; build: (cd $wt && XDG_RUNTIME_DIR=/run/user/\$UID env/venv/bin/python harness/tools/noexe.py build)"
