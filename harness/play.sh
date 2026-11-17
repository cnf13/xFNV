#!/usr/bin/env bash
# A scripted headless run: harness/run.sh in a slot, then a list of steps (keys, console commands, screenshots)
# sent to the game from outside.
#
# Usage: harness/play.sh [options] STEPS        STEPS: a '|'-separated step list, or @FILE holding one
#   -X exe        a Windows program (default build/noexe/FalloutNV.exe when neither -X nor -L is given)
#   -L program    the Linux program (build/linux/FalloutNV)
#   -n            the original game (run.sh's vanilla run), to tell the port's faults from the game's own behaviour
#   -w dir        the checkout or worktree whose harness runs the game and whose build the paths above are in
#                 (default: this script's checkout)
#   -S slot       FNVD_SLOT (default $FNVD_SLOT, else 7 for a Windows program and G for the Linux one)
#   -o dir        output folder (default env/out/play-SLOT-TIMESTAMP): run.sh's files, the screenshots NAME.png,
#                 play.log (every step with its time)
#   -s secs       the run's limit (default 200); after the steps the script waits for the game to end
#   -d secs       wait before the first step, for the main menu (default 70)
#   -C cell       first `coc CELL` from the main menu, then wait -l secs (default 20) and take shot c0
#   -i spec, -D, -k   passed to run.sh (an ini override; the DLC; a restart keeping the last run's My Games)
#   -y            after the steps, a screenshot every 20 s until the game ends
# Environment: SDL_AUDIODRIVER, ALSOFT_DRIVERS (default null), NOSHOT=1 (no screenshots).
#
# Steps: every harness/play/keys.py step (tap:, hold:, text:, con:, click, mhold:, move:, turn:, look:, rel:, code:),
# and:
#   shot:NAME     a screenshot (gamescopectl) to OUT/NAME.png
#   sleep:SECS
#   cn:TEXT       open the console, type TEXT ('+' = space), Return; the console stays open
#   ct:TEXT       TEXT and Return into the open console
#   bat:NAME      the console's `bat NAME` (NAME.txt in the run tree, one command per line), console closed after
#   bt:NAME       every thread's backtrace to OUT/bt-NAME.txt (harness/play/backtrace.sh)
#   sh:COMMAND    a shell command (eval'd in this script; $out, $FNVD_RUN, $disp are set)
#
# One game per slot: the script waits while another run holds the slot (its lock), then starts run.sh, which takes
# it. Console notes (docs/ENVIRONMENT.md "Scripting a run"): `scof FILE` writes the console's output to FILE in the
# run tree; a console key sent while the game loads or saves is lost. Exit status: run.sh's (the game's).
set -u
here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
wt=$(cd "$here/.." && pwd)
native=""
linux=""
vanilla=""
slot=${FNVD_SLOT:-}
out=""
secs=200
delay=70
cell=""
loads=20
yshots=""
runargs=()
while getopts "X:L:nw:S:o:s:d:C:l:i:Dky" opt; do
	case $opt in
	X) native=$OPTARG ;;
	L) linux=$OPTARG ;;
	n) vanilla=1 ;;
	w) wt=$(cd "$OPTARG" && pwd) ;;
	S) slot=$OPTARG ;;
	o) out=$OPTARG ;;
	s) secs=$OPTARG ;;
	d) delay=$OPTARG ;;
	C) cell=$OPTARG ;;
	l) loads=$OPTARG ;;
	i) runargs+=(-i "$OPTARG") ;;
	D) runargs+=(-D) ;;
	k) runargs+=(-k) ;;
	y) yshots=1 ;;
	*) sed -n '2,/^set -u/p' "$0" | sed '$d' >&2; exit 64 ;;
	esac
done
shift $((OPTIND - 1))
[[ $# -eq 1 ]] || { echo "play: one STEPS argument (see the header)" >&2; exit 64; }
steplist=$1
[[ $steplist == @* ]] && steplist=$(tr '\n' '|' <"${steplist#@}" | sed 's/|*$//')
[[ -z $native && -z $linux && -z $vanilla ]] && native=build/noexe/FalloutNV.exe
if [[ -n $linux ]]; then
	prog=(-L "$linux")
	slot=${slot:-G}
elif [[ -n $vanilla ]]; then
	prog=(-n)
	slot=${slot:-7}
else
	prog=(-X "$native")
	slot=${slot:-7}
fi
export FNVD_SLOT=$slot ALSOFT_DRIVERS=${ALSOFT_DRIVERS:-null}
source "$wt/harness/env.sh"
out=${out:-$FNVD_ENV/out/play-$slot-$(date +%Y%m%d-%H%M%S)}
mkdir -p "$out"
out=$(cd "$out" && pwd)
exec > >(tee "$out/play.log") 2>&1

# Another run in this slot: wait for its lock (run.sh takes it again at once).
while ! flock -n "$FNVD_ENV/game$FNVD_SUFFIX.lock" true; do
	echo "play: slot $slot busy, waiting"
	sleep 15
done
( cd "$wt" && exec harness/run.sh "${prog[@]}" "${runargs[@]}" -s "$((secs + 10))" -o "$out" ) >"$out/run.txt" 2>&1 &
rp=$!
SECONDS=0
until read -r _xw disp wl < <("$here/play/xdisplay.sh" "$((SECONDS + 2))"); do
	kill -0 "$rp" 2>/dev/null || { echo "play: the run ended before its display came up"; cat "$out/run.txt"; exit 1; }
	sleep 1
done
echo "play: slot $slot, display $disp, output $out"
shot() {
	if [[ -n ${NOSHOT:-} ]]; then echo "noshot $1"; return; fi
	GAMESCOPE_WAYLAND_DISPLAY=$wl timeout 10 gamescopectl screenshot "$out/$1.png" >/dev/null 2>&1
	echo "shot $1 at $SECONDS"
}
k() { /usr/bin/python3 "$here/play/keys.py" "$disp" "$@"; }
SECONDS=0
sleep "$delay"
pre="move:600:350"
[[ -n $cell ]] && pre+="|hold:grave:0.4|sleep:1|text:coc+$cell|tap:Return|sleep:$loads|shot:c0"
IFS='|' read -r -a steps <<<"$pre|$steplist"
for s in "${steps[@]}"; do
	[[ -z $s ]] && continue
	echo "step $s at $SECONDS"
	case $s in
	shot:*) shot "${s#shot:}" ;;
	sleep:*) sleep "${s#sleep:}" ;;
	sh:*) eval "${s#sh:}" ;;
	bt:*) "$here/play/backtrace.sh" "$out/bt-${s#bt:}.txt" ;;
	bat:*) k "con:bat+${s#bat:}" ;;
	cn:*) k hold:grave:0.4; sleep 1; k "text:${s#cn:}" tap:Return ;;
	ct:*) k "text:${s#ct:}" tap:Return; sleep 1 ;;
	*) k "$s" ;;
	esac
	kill -0 "$rp" 2>/dev/null || { echo "game gone at $SECONDS"; break; }
done
echo "steps done at $SECONDS"
if [[ -n $yshots ]]; then
	while kill -0 "$rp" 2>/dev/null; do sleep 20; shot "y$SECONDS"; done
fi
wait "$rp"
status=$?
cat "$out/run.txt"
exit $status
