#!/usr/bin/env bash
# xdisplay.sh SECS: the X display of this slot's game (harness/play.sh), as "XWAYLAND_PID DISPLAY WAYLAND_SOCKET";
# exit 1 while there is none. Run with the slot's harness/env.sh sourced: the Xwayland is the one whose environment
# holds this slot's WINEPREFIX (gamescope's headless Xwayland detaches from its parent, so ancestry cannot tell),
# started at most SECS seconds ago, so a leftover of an earlier run in the slot is never taken.
maxage=${1:?usage: xdisplay.sh SECS}
for p in $(pgrep -f '(^|/)Xwayland '); do
	tr '\0' '\n' <"/proc/$p/environ" 2>/dev/null | grep -qxF "WINEPREFIX=$WINEPREFIX" || continue
	age=$(ps -o etimes= -p "$p" 2>/dev/null) || continue
	((age <= maxage)) || continue
	d=$(tr '\0' '\n' <"/proc/$p/cmdline" | grep -m1 -E '^:[0-9]+$')
	w=$(tr '\0' '\n' <"/proc/$p/environ" 2>/dev/null | grep -m1 '^WAYLAND_DISPLAY=')
	w=${w#WAYLAND_DISPLAY=}
	echo "$p $d ${w:-gamescope-0}"
	exit 0
done
exit 1
