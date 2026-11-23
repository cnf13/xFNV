#!/bin/bash
# Makes the import library of d3dx9_38.dll, the game's one third-party DLL still linked (CMakeLists.txt, unless
# XFNV_D3DX), from d3dx9_38.def. The .def lists the stdcall names the 32-bit game imports (`Name@N`); x64 has one
# calling convention and no `@N` decoration, so the names are taken without it. steam_api is src/platform/steam's
# stand-in.
# Usage: external/lib/make.sh   (llvm-dlltool on PATH)
cd "$(dirname "$0")" || exit 1
tmp=$(mktemp) || exit 1
sed -E -e 's/@[0-9]+$//' d3dx9_38.def > "$tmp"
llvm-dlltool -m i386:x86-64 -d "$tmp" -D d3dx9_38.dll -l d3dx9_38.lib
rc=$?
rm "$tmp"
exit $rc
