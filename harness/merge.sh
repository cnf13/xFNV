#!/bin/bash
# Merge agent branches into master and build master's program.
#   harness/merge.sh BRANCH...   merges each branch in the worktree env/wt/merge (branch `merge`: a conflict's markers
#                                never sit in the main checkout), builds the native program there (x86; X64=1 the x64
#                                one too, both at once with 12 jobs each: the user, 2026-11-15), and only then
#                                fast-forwards master and copies each exe and its PDB to the main checkout's
#                                build/noexe and build/noexe64 (where the user and run.sh -X find master's programs).
#   harness/merge.sh             the same without a merge: after a conflict or a build failure was fixed and committed
#                                in env/wt/merge.
# NOBUILD=1 lands without the build (the first branches of a batch; the last call builds).
# It stops at the first failure and says why (env/scratch/merge.log); master has not moved then. One instance at a time.
# A long batch runs as a unit:
#   systemd-run --user --unit=fnvd-merge --collect -p WorkingDirectory=$PWD harness/merge.sh BRANCH...
set -u
cd "$(dirname "$0")/.." || exit 1
export PATH=${FNVD_USER_HOME:-$HOME}/.local/opt/LLVM-23.1.3-Linux-X64/bin:$PATH
export XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR:-/run/user/$UID}
m=env/wt/merge
log=env/scratch/merge.log
capped="systemd-run --user --scope -q -p MemoryMax=12G -p MemorySwapMax=1G -- taskset -c 0-12,16-28 nice -n 19"
say() { echo "$(date +%H:%M) $*" | tee -a $log; }
fail() { say "STOPPED: $*"; exit 1; }

exec 9>env/scratch/merge.lock
flock 9
[ -d $m ] || harness/worktree.sh merge >/dev/null || fail "no merge worktree"
[ -z "$(git -C $m status --porcelain | grep -v '^??')" ] || fail "$m has uncommitted changes"
git -C $m merge -q --ff-only master >/dev/null 2>&1 || fail "$m has diverged from master: finish or abort what is there"
for b in "$@"; do
	git diff --name-only "master...$b" | grep -qE '^(env|game|resources)(/|$)' && fail "$b touches env, game or resources"
	git -C $m merge --no-edit "$b" >>$log 2>&1 \
		|| fail "conflicts merging $b: resolve and commit in $m, then harness/merge.sh with the branches after it"
done
# The mergiraf merge driver can leave conflict markers in a file without flagging it as conflicted.
marked=$(git diff --name-only --diff-filter=d -z master merge \
	| (cd $m && xargs -0 -r grep -lE '<<<<<<< |^=======$|>>>>>>> |^\|\|\|\|\|\|\| ' 2>/dev/null) | tr '\n' ' ')
[ -z "$marked" ] || fail "conflict markers in $m: $marked"
if [ -z "${NOBUILD:-}" ]; then
	pids=()
	for arch in x86 ${X64:+x64}; do
		out=env/scratch/merge.build.$arch.log
		(cd $m && $capped env/venv/bin/python harness/tools/noexe.py build --arch $arch --jobs 12; \
			env/venv/bin/python harness/tools/noexe.py status --arch $arch) >$out 2>&1 &
		pids+=($!)
	done
	wait "${pids[@]}"
	for arch in x86 ${X64:+x64}; do
		out=env/scratch/merge.build.$arch.log
		grep -qP '^total\t0$' $out || fail "the native build fails ($out): fix and commit in $m, then harness/merge.sh"
	done
fi
git merge -q --ff-only merge >>$log 2>&1 || fail "master cannot fast-forward to the merge branch"
if [ -z "${NOBUILD:-}" ]; then
	for dir in noexe ${X64:+noexe64}; do
		mkdir -p build/$dir
		install -m 0755 $m/build/$dir/FalloutNV.exe build/$dir/FalloutNV.exe
		[ -f $m/build/$dir/FalloutNV.pdb ] && install -m 0644 $m/build/$dir/FalloutNV.pdb build/$dir/FalloutNV.pdb
	done
fi
say "master $(git rev-parse --short HEAD)${*:+: merged $*}${NOBUILD:+ (not built)}"
