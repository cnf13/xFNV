#!/usr/bin/env bash
# Builds the isolated runtime: the Wine prefix and the run tree. Idempotent; `--clean` rebuilds both.
#   env/prefix    Wine prefix (no Steam, no Mono/Gecko)
#   env/run       the game's files linked from game/ (read-only), FalloutNV.exe copied from env/exe (DRM-free)
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

if [[ ${1:-} == --clean ]]; then
    rm -rf "$WINEPREFIX" "$FNVD_RUN"
fi

[[ -f $FNVD_EXE ]] || { echo "setup: no $FNVD_EXE (docs/ENVIRONMENT.md: unpack the Steam exe with Steamless)" >&2; exit 1; }
[[ -f $FNVD_GAME/Data/FalloutNV.esm ]] || { echo "setup: no game copy in $FNVD_GAME" >&2; exit 1; }

if [[ ! -f $WINEPREFIX/system.reg ]]; then
    echo "setup: creating the Wine prefix"
    wineboot -i
    wineserver -w
fi

# no crash dialog: winedbg's window would freeze a crashed game until the run's timeout; without it winedbg
# --auto prints a backtrace to game.log and ends the process (the shim logs the exception first)
wine reg add 'HKCU\Software\Wine\WineDbg' /v ShowCrashDialog /t REG_DWORD /d 0 /f >/dev/null 2>&1
wineserver -w

# Audio: the private runtime dir keeps the game away from the user's pipewire/pulse. Give the isolated HOME a null
# ALSA default and make Wine use ALSA only, so nothing opens the real sound card. This does NOT fix the crash in
# Wine's dsound mixer (divide by zero once a cell loads): run.sh runs the game with Audio:bEnableAudio=0.
cat >"$HOME/.asoundrc" <<'ASOUND'
pcm.!default { type null }
ctl.!default { type hw card 0 }
ASOUND
wine reg add 'HKCU\Software\Wine\Drivers' /v Audio /t REG_SZ /d alsa /f >/dev/null 2>&1
wineserver -w

mkdir -p "$FNVD_RUN"
for entry in "$FNVD_GAME"/* "$FNVD_GAME"/.[!.]*; do
    [[ -e $entry ]] || continue
    name=$(basename "$entry")
    [[ $name == FalloutNV.exe || $name == Data ]] && continue
    [[ -L $FNVD_RUN/$name ]] || ln -s "$entry" "$FNVD_RUN/$name"
done

# Data is a real directory linking each entry, minus the .nam files: the engine force-loads every plugin that has
# a matching .nam, whatever plugins.txt says, and the DLC / pre-order packs then open "items added to inventory"
# message boxes that block a headless run. The runs are base game only (run.sh writes plugins.txt to match).
[[ -L $FNVD_RUN/Data ]] && rm "$FNVD_RUN/Data"
mkdir -p "$FNVD_RUN/Data"
for entry in "$FNVD_GAME"/Data/*; do
    name=$(basename "$entry")
    shopt -s nocasematch
    [[ $name == *.nam ]] && { shopt -u nocasematch; continue; }
    shopt -u nocasematch
    [[ -L $FNVD_RUN/Data/$name ]] || ln -s "$entry" "$FNVD_RUN/Data/$name"
done
install -m 0644 "$FNVD_EXE" "$FNVD_RUN/FalloutNV.exe"
echo "setup: prefix $WINEPREFIX, run tree $FNVD_RUN"
