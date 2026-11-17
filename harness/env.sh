# Sourced by every harness script: confines Wine, the game and every tool to this checkout.
# Nothing outside the repository is written, except the runtime sockets Wine and Xwayland create under /tmp
# (/tmp/.wine-<uid>/, /tmp/.X11-unix/) which their programs hard-code.
#   FNVD_ROOT     the repository
#   FNVD_GAME     the pristine Steam copy (read-only; never run in place)
#   FNVD_EXE      the DRM-free vanilla exe (Steamless, see docs/ENVIRONMENT.md)
#   FNVD_RUN      the run tree: the game's files linked, the exe and our DLLs copied
#   FNVD_SLOT     unset: env/run, env/prefix, env/runtime, env/game.lock. N: env/run-N, env/prefix-N,
#                 env/runtime-N, env/game-N.lock, so runs in different slots play at the same time (run.sh creates a
#                 slot from the default tree on first use; fnvd_cleanup only ever sees its own slot's prefix)
FNVD_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
# physical paths: a worktree (harness/worktree.sh) links env/ and game/ to the main checkout's, and its Wine
# processes must carry the main checkout's WINEPREFIX string (fnvd_cleanup matches on it)
FNVD_ENV=$(realpath -m "$FNVD_ROOT/env")
FNVD_GAME=$(realpath -m "$FNVD_ROOT/game")
FNVD_EXE=$FNVD_ENV/exe/FalloutNV.exe
FNVD_SUFFIX=${FNVD_SLOT:+-$FNVD_SLOT}
FNVD_RUN=$FNVD_ENV/run$FNVD_SUFFIX

# the user's home before the redirect: the toolchain's SDK and compilers live there (cmake/i686-windows-clang-cl.cmake
# reads FNVD_XWIN), so a build configured from a shell that sourced this file still finds them
export FNVD_USER_HOME=${FNVD_USER_HOME:-$HOME}
export FNVD_XWIN=${FNVD_XWIN:-$FNVD_USER_HOME/.local/opt/xwin-sdk}
export HOME=$FNVD_ENV/home
export XDG_CONFIG_HOME=$HOME/.config
export XDG_CACHE_HOME=$HOME/.cache
export XDG_DATA_HOME=$HOME/.local/share
export XDG_STATE_HOME=$HOME/.local/state
# the user's display, kept aside before the redirect below and handed only to the gamescope of `run.sh -w` (a visible
# window the user asked for); the wayland socket as an absolute path, since XDG_RUNTIME_DIR changes below
if [[ -z ${FNVD_HOST_WAYLAND:-} && -n ${WAYLAND_DISPLAY:-} ]]; then
    if [[ $WAYLAND_DISPLAY == /* ]]; then
        FNVD_HOST_WAYLAND=$WAYLAND_DISPLAY
    else
        FNVD_HOST_WAYLAND=${XDG_RUNTIME_DIR:-/run/user/$(id -u)}/$WAYLAND_DISPLAY
    fi
fi
export FNVD_HOST_WAYLAND=${FNVD_HOST_WAYLAND:-}
export FNVD_HOST_DISPLAY=${FNVD_HOST_DISPLAY:-${DISPLAY:-}}
export FNVD_HOST_XAUTHORITY=${FNVD_HOST_XAUTHORITY:-${XAUTHORITY:-}}
# gamescope, Xwayland and Wine put sockets and state files here; the user's /run/user/<uid> must stay untouched
export XDG_RUNTIME_DIR=$FNVD_ENV/runtime$FNVD_SUFFIX
export WINEPREFIX=$FNVD_ENV/prefix$FNVD_SUFFIX
export WINEARCH=win64
export WINEDEBUG=${WINEDEBUG:--all}
# no Mono / Gecko download prompts; our proxy DLLs win over Wine's builtins
export WINEDLLOVERRIDES="mscoree,mshtml=;dinput8=n,b${FNVD_DLLOVERRIDES:+;$FNVD_DLLOVERRIDES}"
export PIP_CACHE_DIR=$FNVD_ENV/pip-cache
export DOTNET_CLI_HOME=$HOME DOTNET_CLI_TELEMETRY_OPTOUT=1 DOTNET_NOLOGO=1
# never reach the user's Steam client: the prefix has no Steam registry keys, and this keeps steam_api from
# finding a running client through the environment
unset SteamAppId SteamGameId SteamPath SteamClientLaunch STEAM_COMPAT_CLIENT_INSTALL_PATH STEAM_COMPAT_DATA_PATH
# the user's desktop session: gamescope and Wine must never reach it (display, X auth, session bus with its
# portals and screensaver inhibit); headless gamescope hands its children its own DISPLAY
unset WAYLAND_DISPLAY DISPLAY XAUTHORITY DBUS_SESSION_BUS_ADDRESS

mkdir -p "$HOME" "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME" "$XDG_DATA_HOME" "$XDG_STATE_HOME" "$XDG_RUNTIME_DIR"
chmod 0700 "$XDG_RUNTIME_DIR"

# Ends every Wine / gamescope process of this prefix (game, wineserver, orphaned winedevice, gamescopereaper),
# matched by the exact WINEPREFIX in its environment, so nothing of the user's (Steam, Proton prefixes) can match.
# Shells that sourced this file carry the same WINEPREFIX: only Wine and gamescope binaries are ended, never shells.
fnvd_cleanup() {
    local pid comm cmd0 pids=()
    for pid in $(pgrep -u "$(id -u)"); do
        comm=$(cat "/proc/$pid/comm" 2>/dev/null) || continue
        # the game renames its process after its main thread ("Main"): match its argv[0] (`Z:\...\FalloutNV.exe`,
        # or run.sh -L's `./FalloutNV.linux`) too, or every crashed or hung game outlives its run. Only argv[0]:
        # tools and shells naming an .exe in their arguments are never matched
        cmd0=
        # read fails at end of input (an empty command line): harmless, but fatal under run.sh's set -e
        IFS= read -r -d '' cmd0 <"/proc/$pid/cmdline" 2>/dev/null || true
        [[ $comm =~ ^(wine|gamescope|Xwayland)|\.exe$ || $cmd0 == *.exe || $cmd0 == *FalloutNV.linux ]] || continue
        { tr '\0' '\n' <"/proc/$pid/environ"; } 2>/dev/null | grep -qxF "WINEPREFIX=$WINEPREFIX" && pids+=("$pid")
    done
    [[ ${#pids[@]} -gt 0 ]] && kill -9 "${pids[@]}" 2>/dev/null
    rm -rf "${XDG_RUNTIME_DIR:?}"/gamescope-* "${XDG_RUNTIME_DIR:?}"/wayland-* 2>/dev/null
    return 0
}
