#!/usr/bin/env python3
"""The Windows program's import check: the game's own code imports nothing from user32, gdi32, comctl32, dinput8 or
XInput (window, message, cursor, display, input: SDL3 and external/dinput_sdl stand for them).

Usage: env/venv/bin/python harness/tools/sdl-imports.py [BUILD]     (default build/noexe; build/noexe64 for x64)

The exe's import table alone cannot tell the game's imports from those of the static SDL linked into it (SDL makes its
own window with the same user32 functions), so a name counts when an object references its import slot (an `__imp_`
symbol, llvm-nm) and the exe imports it from a DLL (llvm-readobj; a function the external/ stand-ins define is bound
to them, not imported). The objects are grouped by their CMake target: the game's (fnvd_engine, fnvd_havok,
fnvd_dinput_sdl) and the rest (external/platform, SDL3, the crash log, the debug heap and the program's support).
Prints `FORBIDDEN DLL NAME OBJECTS` per game import from a forbidden DLL, `left DLL NAME OBJECTS` per other game import
from advapi32 (what the port beyond Windows still has to replace), `other DLL NAME TARGETS` per forbidden-DLL import
of the other targets, then `forbidden N`; exit 1 when N is not 0.
"""
import glob
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
LLVM = os.path.join(os.environ.get("FNVD_USER_HOME", os.path.expanduser("~")), ".local/opt/LLVM-23.1.3-Linux-X64/bin")
GAME_TARGETS = ("fnvd_engine", "fnvd_havok", "fnvd_dinput_sdl")
FORBIDDEN_DLLS = ("user32.dll", "gdi32.dll", "comctl32.dll", "dinput8.dll")
REVIEW_DLLS = ("advapi32.dll",)


def forbidden_dll(dll):
	return dll in FORBIDDEN_DLLS or dll.startswith("xinput")


def target_of(obj):
	"""The CMake target an object belongs to: the `<target>.dir` folder above it."""
	m = re.search(r"([^/]+)\.dir/", obj)
	return m.group(1) if m else obj


def referenced(build):
	"""name -> the objects (relative to build) referencing its import slot."""
	objects = glob.glob(os.path.join(build, "**", "CMakeFiles", "*.dir", "**", "*.obj"), recursive=True)
	if not any(target_of(o) in GAME_TARGETS for o in objects):
		sys.exit("sdl-imports: no game objects under %s" % build)
	names = {}
	for start in range(0, len(objects), 200):
		out = subprocess.run([os.path.join(LLVM, "llvm-nm"), "-u", "--no-sort"] + objects[start:start + 200],
			capture_output=True, text=True).stdout
		current = None
		for line in out.splitlines():
			if line.endswith(":") and not line.startswith(" "):
				current = os.path.relpath(line[:-1], build)
				continue
			m = re.search(r"__imp__?(\w+?)(?:@\d+)?$", line.strip())
			if m:
				names.setdefault(m.group(1), set()).add(current)
	return names


def imported(exe):
	"""name -> DLL (lower case) of every function the exe imports."""
	out = subprocess.run([os.path.join(LLVM, "llvm-readobj"), "--coff-imports", exe], capture_output=True,
		text=True).stdout
	found = {}
	dll = None
	for line in out.splitlines():
		m = re.match(r"\s*Name: (\S+)", line)
		if m:
			dll = m.group(1).lower()
			continue
		m = re.match(r"\s*Symbol: (\w+)", line)
		if m:
			found[m.group(1)] = dll
	return found


def main():
	build = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "build", "noexe"))
	names = referenced(build)
	dlls = imported(os.path.join(build, "FalloutNV.exe"))
	bad = 0
	others = {}
	for name in sorted(names):
		dll = dlls.get(name)
		if dll is None:
			continue
		game = sorted(o for o in names[name] if target_of(o) in GAME_TARGETS)
		if game and forbidden_dll(dll):
			bad += 1
			print("FORBIDDEN\t%s\t%s\t%s" % (dll, name, " ".join(game)))
		elif game and dll in REVIEW_DLLS:
			print("left\t%s\t%s\t%s" % (dll, name, " ".join(game)))
		if forbidden_dll(dll):
			for target in {target_of(o) for o in names[name]} - set(GAME_TARGETS):
				others.setdefault((dll, target), []).append(name)
	# The forbidden DLLs' imports of the other targets: their count, and their names when few.
	for (dll, target), found in sorted(others.items()):
		print("other\t%s\t%s\t%d\t%s" % (dll, target, len(found), " ".join(found) if len(found) <= 10 else ""))
	print("forbidden %d" % bad)
	sys.exit(1 if bad else 0)


if __name__ == "__main__":
	main()
