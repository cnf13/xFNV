#!/usr/bin/env bash
# backtrace.sh OUT: every thread's backtrace of the game running in this slot (FNVD_SLOT), written to OUT, the game
# left running: the Linux program through gdb (attach, `thread apply all bt`, detach), a Windows one through winedbg
# (attach, `bt all`, detach). harness/play.sh's `bt:NAME` step.
out=${1:?usage: backtrace.sh OUT}
source "$(dirname "${BASH_SOURCE[0]}")/../env.sh" >/dev/null 2>&1
pid=
for p in $(pgrep -f 'FalloutNV.linux'); do
	tr '\0' '\n' <"/proc/$p/environ" 2>/dev/null | grep -qxF "WINEPREFIX=$WINEPREFIX" && pid=$p
done
if [[ -n $pid ]]; then
	echo "linux pid $pid" >"$out"
	timeout 120 gdb -batch -p "$pid" -ex 'set pagination off' -ex 'thread apply all bt' >>"$out" 2>&1
else
	pid=$(winedbg --command "info proc" 2>/dev/null | awk '/FalloutNV/ {print $1; exit}')
	echo "winpid $pid" >"$out"
	[[ -n $pid ]] && printf 'bt all\ndetach\nquit\n' | timeout 120 winedbg "0x$pid" >>"$out" 2>&1
fi
echo "backtrace: $(wc -l <"$out") lines in $out"
