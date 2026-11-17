#!/bin/bash
# Makes the import libraries of the game's third-party DLLs from the .def files here (the Windows programs link them:
# CMakeLists.txt). d3dx9_38.def lists the stdcall names the program imports (`Name@N`; -k keeps the
# DLL's export undecorated); steam_api.def is the DLL's export table (cdecl).
# x64/ holds the 64-bit program's: x64 has one calling convention and no `@N` decoration, so the names are taken
# without it; no steam_api there (the 64-bit program links src/platform/steam's stand-in).
# Usage: external/lib/make.sh   (llvm-dlltool on PATH)
cd "$(dirname "$0")" || exit 1
mkdir -p x64
for def in *.def; do
	name=${def%.def}
	llvm-dlltool -m i386 -k -d "$def" -D "$name.dll" -l "$name.lib" || exit 1
	[ "$name" = steam_api ] && continue
	dll=$name.dll
	sed -E -e 's/@[0-9]+$//' -e "s/^LIBRARY .*/LIBRARY $dll/" "$def" > "x64/$def"
	llvm-dlltool -m i386:x86-64 -d "x64/$def" -D "$dll" -l "x64/$name.lib" || exit 1
	rm "x64/$def"
done
