#!/bin/bash
# The locals pass on the canonical IDA database (docs/IDAKIT.md "Frames"; harness/tools/pdb_locals.py): every game
# function's sp trace made right, then the debug build's locals paired again on the functions whose frame changed.
# Nothing else may hold the database open (the IDA GUI). Run it after a sync that counts `purge-changed`.
# Usage: harness/locals.sh                       (the functions idakit frames lists as changed)
#        harness/locals.sh --all [OPTION...]     (every function, from no settings: after a change to the pass;
#                                                 OPTION: pdb_locals.py's, e.g. --votes)
# Both pair with the names pdb_names.py voted (names-accepted.tsv) where that table exists: a function whose voted
# name src does not have yet pairs only through it.
set -eu
if [[ $# -gt 0 && $1 != --all ]]; then
    sed -n '2,9s/^# \{0,1\}//p' "$0"
    exit 2
fi
root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
export HOME=${FNVD_USER_HOME:-$HOME} IDADIR=${IDADIR:-$HOME/ida-pro-9.3}
idb=env/ida/FalloutNV.exe.i64
build/idakit/idakit frames --idb "$idb"
names=()
if [[ -s env/scratch/pdblocals/names-accepted.tsv ]]; then
    names=(--names env/scratch/pdblocals/names-accepted.tsv)
fi
if [[ ${1:-} == --all ]]; then
    shift
    env/venv/bin/python harness/tools/pdb_locals.py --fresh --apply "${names[@]}" "$@"
elif [[ -s env/cache/frames/frames-changed.txt ]]; then
    env/venv/bin/python harness/tools/pdb_locals.py --fresh --only env/cache/frames/frames-changed.txt \
        --out env/scratch/pdblocals/changed --apply "${names[@]}"
fi
