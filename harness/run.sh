#!/usr/bin/env bash
# Runs the game once, headless, inside the isolated environment (harness/env.sh).
#
# Usage: harness/run.sh [options] [-- extra arguments for FalloutNV.exe]
#   -s secs    stop the game after this many seconds (default 300)
#   -o dir     output folder (default env/out/<timestamp>): game.log, exit.txt (how wine, or the -L program, ended;
#              absent if the game outlived -s)
#              Exit status: the game's (exit.txt; gamescope's own shutdown segfault is ignored), else gamescope's or
#              timeout's (124/137 when the game outlived -s). run.sh's own errors: 64 bad arguments, 69 setup or a
#              build missing.
#   -n         vanilla: the exe and the game's own files only (the default without -X or -L)
#   -X exe     native: run this exe (the no-exe build, build/noexe/FalloutNV.exe) in place of the original; it goes
#              into the run tree as FalloutNV.native.exe, beside the original exe. Its crash log and
#              minidump (harness/crashlog) are moved into the output folder
#   -L prog    native Linux: run this ELF program (the Linux build, build/linux/FalloutNV) with no Wine, from the
#              run tree (it goes there as FalloutNV.linux, with the shared libraries beside it, lib*.so.*: its run path
#              is its own folder). Direct3D is dxvk-native (DXVK_WSI_DRIVER=SDL3; its log
#              FalloutNV.linux_d3d9.log in the output folder); the program's user folders are under
#              env/linux-data[-N] (XDG_DATA_HOME: FalloutNV/plugins.txt, FalloutNV/Documents/My Games/FalloutNV/);
#              its crash log is moved into the output folder
#   -c cell    sStartingCell: skip the main menu and start in this cell (editor id)
#   -i spec    extra ini override, 'Section:key=value' (repeatable)
#   -D         load the DLC and pre-order packs too, as a player's install does (the default is FalloutNV.esm only);
#              their "items added" message boxes open on the first cell load
#   -w         windowed: a visible gamescope window on the user's desktop, for playing by hand (the user's call; the
#              one hole in the isolation: only gamescope gets the display, harness/env.sh FNVD_HOST_*). Without -s
#              the limit is 24 hours; quit from the game's menu. No game audio either way (Audio:bEnableAudio below)
#   -k         a restart: keep the slot's My Games/FalloutNV from its last run (saves, and FalloutPrefs.ini as the game
#              wrote it; Fallout.ini is still rebuilt). The run before must have used the same program kind (-L or not)
#
# The ini files are rebuilt each run from the game's defaults plus the overrides below, so no run inherits another's
# state (but see -k). The display is gamescope's headless backend (unless -w): nothing appears on the user's desktop.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

seconds=""
out=""
native=""
linux=""
cell=""
dlc=0
windowed=0
keep=0
overrides=()
while [[ $# -gt 0 ]]; do
    case $1 in
    -s) seconds=$2; shift 2 ;;
    -o) out=$2; shift 2 ;;
    -n) shift ;;
    -X) native=$2; shift 2 ;;
    -L) linux=$2; shift 2 ;;
    -c) cell=$2; shift 2 ;;
    -D) dlc=1; shift ;;
    -i) overrides+=("$2"); shift 2 ;;
    -w) windowed=1; shift ;;
    -k) keep=1; shift ;;
    --) shift; break ;;
    *) echo "run: unknown argument $1" >&2; exit 64 ;;
    esac
done
if [[ -z $seconds ]]; then
    seconds=300
    [[ $windowed == 1 ]] && seconds=86400
fi
[[ -z $native || -f $native ]] || { echo "run: no such file $native" >&2; exit 64; }
[[ -z $linux || -x $linux ]] || { echo "run: no such program $linux" >&2; exit 64; }
[[ -z $native || -z $linux ]] || { echo "run: -X and -L exclude each other" >&2; exit 64; }
if [[ $windowed == 1 && -z $FNVD_HOST_WAYLAND && -z $FNVD_HOST_DISPLAY ]]; then
    echo "run: -w needs a desktop session (WAYLAND_DISPLAY or DISPLAY in the calling shell)" >&2
    exit 64
fi

# a new slot: copies of the default prefix and run tree (the run tree's game files are links; the copy keeps them),
# taken under the default slot's game lock: wineserver rewrites the registry files while a game runs there
if [[ -n ${FNVD_SLOT:-} && ! -f $WINEPREFIX/system.reg ]]; then
    (
        flock 9
        if [[ ! -f $WINEPREFIX/system.reg ]]; then
            rm -rf "$WINEPREFIX" "$FNVD_RUN"
            flock "$FNVD_ENV/game.lock" bash -c 'cp -a "$1/run" "$2" && cp -a "$1/prefix" "$3"' _ \
                "$FNVD_ENV" "$FNVD_RUN" "$WINEPREFIX"
            rm -f "$FNVD_RUN/dinput8.dll"
            mkdir -p "$XDG_RUNTIME_DIR" && chmod 700 "$XDG_RUNTIME_DIR"
        fi
    ) 9>"$FNVD_ENV/slot-setup.lock"
fi
[[ -f $WINEPREFIX/system.reg && -f $FNVD_RUN/FalloutNV.exe ]] || { echo "run: run harness/setup.sh first" >&2; exit 69; }
out=${out:-$FNVD_ENV/out/$(date +%Y%m%d-%H%M%S)}
mkdir -p "$out"
out=$(cd "$out" && pwd)

# The Linux program's user folders (external/platform's UserFolder: LOCAL_APPDATA is XDG_DATA_HOME, PERSONAL is
# XDG_DATA_HOME/FalloutNV/Documents), one tree per slot.
if [[ -n $linux ]]; then
    export XDG_DATA_HOME=$FNVD_ENV/linux-data$FNVD_SUFFIX
    mygames="$XDG_DATA_HOME/FalloutNV/Documents/My Games/FalloutNV"
    appdata="$XDG_DATA_HOME/FalloutNV"
else
    mygames="$WINEPREFIX/drive_c/users/$USER/Documents/My Games/FalloutNV"
    appdata="$WINEPREFIX/drive_c/users/$USER/AppData/Local/FalloutNV"
fi
if [[ $keep == 0 ]]; then
    rm -rf "$mygames"
fi
mkdir -p "$mygames"

# Base game only by default: without a plugins.txt the game loads every .esm in Data, and the DLC / pre-order
# packs then open "items added to inventory" message boxes on the first cell load that block a headless run.
# -D lists them all, in the launcher's order.
mkdir -p "$appdata"
plugins=(FalloutNV.esm)
[[ $dlc == 1 ]] && plugins+=(DeadMoney.esm HonestHearts.esm OldWorldBlues.esm LonesomeRoad.esm
    GunRunnersArsenal.esm ClassicPack.esm MercenaryPack.esm TribalPack.esm CaravanPack.esm)
# CRLF as the launcher writes it, for every program (the Linux one reads it as the Windows C runtime does)
printf '%s\r\n' "${plugins[@]}" >"$appdata/plugins.txt"

# WinMain starts FalloutNVLauncher.exe and exits unless Display:sD3DDevice equals adapter 0's Direct3D description
# (CreateDisplayDriver 0x4DA570), so the description the program's Direct3D reports is probed once and cached:
# Wine's (adapter.txt, build/hybrid/adapter.exe) or dxvk-native's (adapter-linux.txt, the Linux build's `adapter`,
# harness/tools/adapter.cpp on dxvk-native's headers). Delete the file after a GPU, Wine or dxvk change
if [[ -n $linux ]]; then
    adapter_file=$FNVD_ENV/adapter-linux.txt
    probe=$FNVD_ROOT/build/linux/adapter
else
    adapter_file=$FNVD_ENV/adapter.txt
    probe=$FNVD_ROOT/build/hybrid/adapter.exe
fi
if [[ ! -s $adapter_file ]]; then
    [[ -f $probe ]] || { echo "run: no $probe (build: cmake --build ${probe%/*})" >&2; exit 69; }
    fnvd_cleanup
    # gamescope 3.16 segfaults while shutting down (exit 139) even after a clean child exit: judge the output only
    if [[ -n $linux ]]; then
        timeout --kill-after=10 60 gamescope --backend headless -w 640 -h 360 -- \
            env DXVK_WSI_DRIVER=SDL3 DXVK_LOG_LEVEL=none bash -c '"$1" >"$2.part"' fnvd-adapter "$probe" \
            "$adapter_file" >/dev/null 2>&1 || true
    else
        timeout --kill-after=10 60 gamescope --backend headless -w 640 -h 360 -- \
            bash -c 'wine "$1" >"$2.part"; wineserver -k' fnvd-adapter "$probe" "$adapter_file" >/dev/null 2>&1 || true
    fi
    fnvd_cleanup
    tr -d '\r' <"$adapter_file.part" >"$adapter_file" 2>/dev/null || true
    rm -f "$adapter_file.part"
    [[ -s $adapter_file ]] || { echo "run: the adapter probe printed nothing" >&2; rm -f "$adapter_file"; exit 69; }
fi
adapter=$(<"$adapter_file")

width=1280
height=720
base=(
    "General:sIntroSequence="
    "General:SStartingCell=$cell"
    "Display:bFull Screen=0"
    "Display:iSize W=$width"
    "Display:iSize H=$height"
    "Display:iPresentInterval=0"
    "General:bAlwaysActive=1"
    "Display:bAllowScreenShot=1"
    "Display:sD3DDevice=$adapter"
    "Display:iMultiSample=0"
    "Interface:bAllowConsole=1"
    # Wine reports a gamepad through XInput here, and while one is in use the game reads Escape and the console
    # key from the gamepad only (Controls::GetUserAction): keys sent from outside need the controller off
    "Interface:bDisable360Controller=1"
    # Wine 11.20's dsound mixer thread divides by zero (DSOUND.dll +0x71F7, looping buffer) as soon as a cell
    # loads, with or without a working output device: run without game audio
    "Audio:bEnableAudio=0"
)
"$FNVD_ROOT/harness/ini.py" "$FNVD_RUN/Fallout_default.ini" "$mygames/Fallout.ini" "${base[@]}" "${overrides[@]}"
if [[ $keep == 0 || ! -f $mygames/FalloutPrefs.ini ]]; then
    "$FNVD_ROOT/harness/ini.py" "$FNVD_RUN/low.ini" "$mygames/FalloutPrefs.ini" "${base[@]}" "${overrides[@]}"
fi

# The wrapper records how the game ended (exit.txt) and then ends this prefix's wineserver and its services, which
# gamescope would otherwise wait on until the timeout, so an exit never looks like a hang. The Linux program runs
# without Wine, with dxvk-native on SDL3's window and dxvk's log in the output folder.
export FNVD_EXIT_FILE=$out/exit.txt FNVD_GAME_EXE=FalloutNV.exe FNVD_RUNNER=wine
[[ -n $native ]] && FNVD_GAME_EXE=FalloutNV.native.exe
if [[ -n $linux ]]; then
    export FNVD_GAME_EXE=./FalloutNV.linux FNVD_RUNNER="" DXVK_WSI_DRIVER=SDL3 DXVK_LOG_PATH=$out
fi
rm -f "$FNVD_EXIT_FILE"
wrapper='start=$(date +%s.%N)
$FNVD_RUNNER "$FNVD_GAME_EXE" "$@"
status=$?
echo "${FNVD_RUNNER:-linux} exit $status after $(awk -v a="$start" -v b="$(date +%s.%N)" "BEGIN { printf \"%.1f\", b - a }") s" >"$FNVD_EXIT_FILE"
if [[ -n $FNVD_RUNNER ]]; then wineserver -k; fi'
if [[ $windowed == 1 ]]; then
    # the host display goes to gamescope alone; its children get gamescope's own Xwayland (DISPLAY), not the host's
    host=()
    [[ -n $FNVD_HOST_WAYLAND ]] && host+=("WAYLAND_DISPLAY=$FNVD_HOST_WAYLAND")
    [[ -n $FNVD_HOST_DISPLAY ]] && host+=("DISPLAY=$FNVD_HOST_DISPLAY")
    [[ -n $FNVD_HOST_XAUTHORITY ]] && host+=("XAUTHORITY=$FNVD_HOST_XAUTHORITY")
    # relative mouse always: the game reads the mouse through DirectInput (Super+G toggles the keyboard grab)
    game=(env "${host[@]}" gamescope --force-grab-cursor -w "$width" -h "$height" -W "$width" -H "$height" --
        bash -c "unset WAYLAND_DISPLAY XAUTHORITY; $wrapper" fnvd-game "$@")
else
    game=(gamescope --backend headless -w "$width" -h "$height" -W "$width" -H "$height" --
        bash -c "$wrapper" fnvd-game "$@")
fi

# one game per slot (FNVD_SLOT, harness/env.sh): a slot's runs share its prefix, and fnvd_cleanup below kills
# whatever runs there. A second run.sh in the same slot (an agent's) waits here. The game's
# descriptors drop the lock so an orphan cannot hold it.
exec 8>"$FNVD_ENV/game$FNVD_SUFFIX.lock"
flock 8
# leftovers of an earlier run would share this prefix's wineserver
fnvd_cleanup
# a dinput8.dll left in the run tree would load into the exe (env.sh's WINEDLLOVERRIDES): it goes, under the lock
rm -f "$FNVD_RUN/dinput8.dll"
[[ -n $native ]] && install -m 0755 "$native" "$FNVD_RUN/FalloutNV.native.exe"
if [[ -n $linux ]]; then
    install -m 0755 "$linux" "$FNVD_RUN/FalloutNV.linux"
    for lib in "$(dirname "$linux")"/lib*.so.*; do
        [[ -f $lib ]] && install -m 0755 "$lib" "$FNVD_RUN/"
    done
fi
echo "run: $out (${seconds}s, native '${native}${linux}')"
set +e
cd "$FNVD_RUN"
timeout --kill-after=10 "$seconds" "${game[@]}" >"$out/game.log" 2>&1 8>&-
status=$?
set -e
wineserver -k 2>/dev/null || true
fnvd_cleanup
# the native program's crash log and minidump (harness/crashlog), written beside its exe
if [[ -n $native || -n $linux ]]; then
    for crash in "$FNVD_RUN"/FalloutNV.crash-*.{log,dmp}; do
        if [[ -f $crash ]]; then mv "$crash" "$out/"; echo "run: crash log $out/${crash##*/}"; fi
    done
fi
if [[ -f $out/exit.txt ]]; then
    echo "run: $(cat "$out/exit.txt")"
    # gamescope segfaults on its own shutdown (139): the game's status is wine's
    read -r _ _ status _ <"$out/exit.txt"
elif [[ $status == 124 || $status == 137 ]]; then
    echo "run: the game was still running at the ${seconds}s limit"
else
    echo "run: gamescope ended (status $status) before wine returned; no exit.txt"
fi
exec 8>&-
echo "run: exit $status, output $out"
exit $status
