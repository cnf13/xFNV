#!/usr/bin/env python3
"""The no-exe build (the 32-bit Windows program without FalloutNV.exe) as the standalone work's measure.

Usage: env/venv/bin/python harness/tools/noexe.py build [--jobs N] [--arch x86|x64] [--target windows|linux]
       env/venv/bin/python harness/tools/noexe.py status [--table TSV] [--arch x86|x64] [--target ...]
  build    configures build/noexe when it has no build.ninja (`cmake -G Ninja` with
           cmake/i686-windows-clang-cl.cmake and -DFNVD_NO_EXE=ON), then builds the program target fnvd_noexe with
           `-k 0` in a capped scope (MemoryMax 12G, CPUs 0-12,16-28, nice 19, at most 8 jobs; default 8). When a
           file fails to compile, ninja does not link: the objects that did compile are linked by hand from
           build/noexe/noexe-link.rsp (CMake writes it: the same flags and libraries), so the undefined symbols are
           counted either way. Logs: build/noexe/build.log (ninja), build/noexe/link.log (the hand link, if any),
           build/noexe/noexe-state.json (how it linked). It exits 1 when the build failed (an object did not
           compile or the program did not link), after the hand link; the tool's own failures exit 2; `status` says
           what is wrong with src. Its last line counts the build's ccache hits and misses (CMakeLists.txt's
           launcher logs them in build/noexe/ccache-stats.log).
  status   reads those logs and src/, prints one line per kind (`KIND<TAB>COUNT`, the compile kinds with the number
           of objects failing), then `total N`, and exits 1 while the total is not 0. The table (default
           build/noexe/status.tsv): kind, file, line, symbol or message.

  --arch x64     the 64-bit Windows program instead (build/noexe64, cmake/x86_64-windows-clang-cl.cmake): tried beside
                 the 32-bit one, which stays the default and the routine build.
  --sdl-audio    the Windows program with its audio on external/dsound's SDL3 layer (-DFNVD_SDL_AUDIO=ON), in its
                 own folder (build/noexe-sdlaudio, build/noexe64-sdlaudio), so the routine build keeps Microsoft's
                 DirectSound; the Linux program always has the layer
  --target linux the native Linux x64 program instead (build/linux, cmake/x86_64-linux-clang.cmake, CMake target
                 fnvd_linux: the ELF executable FalloutNV; --arch does not apply), the measure
                 of the Linux plan's milestone L1 (it links: `status` total 0). The same build and hand link
                 (build/linux/linux-link.rsp, linked by clang++ with lld). Its `status` has its own kinds:
    compile: object fails to compile   one row per object, with its first error; the summary lists the commonest
                                       first errors
    link: Win32 system, AREA           an undefined kernel32, advapi32 or shell32 function (the DLL from the exe's
                                       import table), by area: sync, threads, time, files, memory and system info,
                                       ini, registry, shell and user folders, dynamic loading, debug and process,
                                       strings, other
    link: Win32 DLL                    user32 (user32, gdi32, comctl32), COM (ole32, oleaut32), winmm, dsound,
                                       d3dx9, d3d9, Winsock, DirectInput/XInput, GUIDs (IID_, CLSID_...), other DLL
    link: third-party DLL              steam_api, binkw32
    link: CRT (msvc9)                  a C runtime function glibc lacks (src/SDK/MSVC's forwarders), or msvc9::
    link: entry point                  main
    link: ...                          the game's own: function with no body, global or static member, template
                                       instance member, vtable or RTTI; while objects fail to compile these include
                                       symbols the failing objects define
    library: missing, link: other error   as for Windows
  Each link row's detail ends with its reference count (lld's "referenced by" lines), and each link kind's summary
  line with the kind's total.

The Windows program's kinds:
  compile: ...   unique error diagnostics (one per file, line and message, however many objects include it; an error
                 that follows from another, as in an unclosed namespace, counts too): incomplete type; undeclared
                 name; a hybrid-only name or form in shared code (heuristics: an undeclared name that occurs on an
                 `#if FNVD_HYBRID` side somewhere in src/, gen/ or the seam; a member a mismatched out-of-line
                 definition names that its class's files declare only there; `static FNVD_GLOBAL`, which is
                 `static extern` without the exe); no matching function or overload; access; other
  link: ...      undefined symbols, demangled by llvm-undname: a global or static member with no definition; a
                 function with no body; a template instance member (or function template instance) with no generic
                 body; a vtable or RTTI record; an exe import slot (class I of harness/staticinit.tsv) of a Windows
                 DLL; one of a third-party DLL
  library: missing   a third-party DLL the exe imports with no import library in external/lib and no stand-in
                     (build/noexe/missing-libraries.txt, written by CMake)
  run: ...       what would stop the program though it links, counted in src/ and gen/ outside the hybrid-only
                 sides of `#if FNVD_HYBRID`: fnvd::original calls (by target: an address src has a body for, a CRT or
                 SDK forwarder under src/SDK/, an address with no body, a virtual slot); FNVD_ORIGINAL_FUNCTION lines
                 (null without the exe); uses of the exe's vtable constants (FNVD_VFTABLE names, fnvd::setVtable;
                 not the FNVD_INLINED_LOCAL arguments the standalone build drops);
                 __RTDynamicCast calls; import slots (class I) with no definition the link did not already report
                 (an FNVD_IMPORT, FNVD_IMPORT_AS or FNVD_IMPORT_STUB line in shared code defines its slot)
"""

import argparse
import collections
import json
import os
import re
import shutil
import subprocess
import sys

FAILED = re.compile(r"^FAILED: (?:\[code=\d+\] )?(.+\.(?:obj|o))\s*$", re.M)

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BUILD = os.path.join(ROOT, "build", "noexe")
TOOLCHAIN = "i686-windows-clang-cl.cmake"
# CMake options of the build folder's first configure (set_target).
OPTIONS = ["-DFNVD_NO_EXE=ON"]
LLVM = os.path.join(os.environ.get("FNVD_USER_HOME", os.path.expanduser("~")), ".local/opt/LLVM-23.1.3-Linux-X64/bin")
EXE = os.path.join(ROOT, "env", "exe", "FalloutNV.exe")
STATICINIT = os.path.join(ROOT, "harness", "staticinit.tsv")
PROGRAM = "FalloutNV.exe"
CMAKE_TARGET = "fnvd_noexe"
RSP = "noexe-link.rsp"
LINUX = False


def set_target(target, arch, sdl_audio=False):
    """Points the build folder, its logs and the toolchain at the 32-bit (x86) or the 64-bit (x64) Windows program
    (with the SDL3 audio layer in a folder of its own), or at the Linux program."""
    global BUILD, TOOLCHAIN, BUILD_LOG, LINK_LOG, STATE, OPTIONS, PROGRAM, CMAKE_TARGET, RSP, LINUX
    if target == "linux":
        LINUX = True
        BUILD = os.path.join(ROOT, "build", "linux")
        TOOLCHAIN = "x86_64-linux-clang.cmake"
        OPTIONS = []
        PROGRAM = "FalloutNV"
        CMAKE_TARGET = "fnvd_linux"
        RSP = "linux-link.rsp"
    else:
        if arch == "x64":
            BUILD = os.path.join(ROOT, "build", "noexe64")
            TOOLCHAIN = "x86_64-windows-clang-cl.cmake"
        if sdl_audio:
            BUILD += "-sdlaudio"
            OPTIONS = OPTIONS + ["-DFNVD_SDL_AUDIO=ON"]
    BUILD_LOG = os.path.join(BUILD, "build.log")
    LINK_LOG = os.path.join(BUILD, "link.log")
    STATE = os.path.join(BUILD, "noexe-state.json")


CAP = ["systemd-run", "--user", "--scope", "-q", "-p", "MemoryMax=12G", "-p", "MemorySwapMax=1G", "--",
       "taskset", "-c", "0-12,16-28", "nice", "-n", "19"]
# The exe's imports outside the Windows SDK.
THIRD_PARTY_DLLS = {"steam_api.dll", "d3dx9_38.dll", "binkw32.dll", "libvorbisfile.dll"}


def tool_env():
    env = dict(os.environ)
    env["PATH"] = LLVM + os.pathsep + env.get("PATH", "")
    return env


# --- build ---------------------------------------------------------------------------------------------------------


def cmd_build(args):
    jobs = max(1, min(args.jobs, 8))
    env = tool_env()
    if not os.path.exists(os.path.join(BUILD, "build.ninja")):
        configure = ["cmake", "-S", ROOT, "-B", BUILD, "-G", "Ninja",
                     "-DCMAKE_TOOLCHAIN_FILE=" + os.path.join(ROOT, "cmake", TOOLCHAIN)] + OPTIONS
        if subprocess.run(configure, env=env).returncode != 0:
            sys.exit("noexe: configure failed")
    output = os.path.join(BUILD, PROGRAM)
    stats_log = os.path.join(BUILD, "ccache-stats.log")
    for stale in (LINK_LOG, STATE, stats_log):
        if os.path.exists(stale):
            os.remove(stale)
    with open(BUILD_LOG, "w") as log:
        built = subprocess.run(CAP + ["cmake", "--build", BUILD, "--target", CMAKE_TARGET, "-j", str(jobs), "--",
                                      "-k", "0"], stdout=log, stderr=subprocess.STDOUT, env=env).returncode
    failed = failed_objects(read(BUILD_LOG))
    state = {"build_exit": built, "objects_failed": len(failed)}
    if built == 0:
        state["link"] = "full"
    elif linked_by_ninja(read(BUILD_LOG)):
        state["link"] = "full, failed"
    else:
        state.update(link_compiled(failed, env))
    with open(STATE, "w") as f:
        json.dump(state, f, indent=1)
    print("noexe: build exit %d, %d objects failed, link: %s%s" % (
        built, len(failed), state["link"], "" if built == 0 else " (noexe.py status: what is wrong)"))
    if not os.path.exists(output) and state["link"] == "full":
        sys.exit("noexe: no %s after a clean build" % output)
    print(ccache_summary(stats_log))
    if built != 0:
        sys.exit(1)


def ccache_summary(stats_log):
    """One line: this build's compiles by their ccache result, from the launcher's stats log (one `# SOURCE` line
    per compile, then its counters)."""
    if not os.path.exists(stats_log):
        return "ccache: no compile logged (nothing to build, or a folder configured without the launcher)"
    compiles = hits = misses = 0
    for line in read(stats_log).splitlines():
        if line.startswith("# "):
            compiles += 1
        elif line in ("direct_cache_hit", "preprocessed_cache_hit"):
            hits += 1
        elif line == "cache_miss":
            misses += 1
    return "ccache: %d compiles, %d hits, %d misses, %d uncached" % (
        compiles, hits, misses, compiles - hits - misses)


def read(path):
    if not os.path.exists(path):
        return ""
    with open(path, errors="replace") as f:
        return f.read()


def failed_objects(log):
    return {os.path.normpath(os.path.join(BUILD, m.group(1))) for m in FAILED.finditer(log)}


def linked_by_ninja(log):
    return bool(re.search(r"Linking CXX executable \S*" + re.escape(PROGRAM), log))


def link_compiled(failed, env):
    """Links the objects that compiled (the link's response file minus the failed and the missing ones) into
    BUILD/partial/, so that the undefined symbols are listed while some files fail."""
    rsp = os.path.join(BUILD, RSP)
    if not os.path.exists(rsp):
        sys.exit("noexe: no %s: reconfigure %s" % (rsp, BUILD))
    kept, dropped = [], 0
    for line in read(rsp).splitlines():
        path = line.strip('"')
        if path.endswith((".obj", ".o")):
            path = os.path.normpath(path)
            if path in failed or not os.path.exists(path):
                dropped += 1
                continue
            line = '"%s"' % path
        elif path.endswith((".a", ".lib")) and path.startswith(BUILD) and not os.path.exists(path):
            # a library of this build that failed to build (its objects are among the failed ones)
            continue
        if line:
            kept.append(line)
    os.makedirs(os.path.join(BUILD, "partial"), exist_ok=True)
    partial = os.path.join(BUILD, "partial", "link.rsp")
    with open(partial, "w") as f:
        f.write("\n".join(kept) + "\n")
    out = os.path.join(BUILD, "partial", PROGRAM)
    if LINUX:
        link = ["clang++", "-o", out, "@" + partial]
    else:
        link = ["lld-link", "/out:" + out, "@" + partial]
    with open(LINK_LOG, "w") as log:
        code = subprocess.run(CAP + link, stdout=log, stderr=subprocess.STDOUT, env=env).returncode
    return {"link": "partial (objects that compiled)", "link_exit": code, "objects_left_out": dropped}


# --- status: compile -----------------------------------------------------------------------------------------------

DIAG = re.compile(r"^(?P<file>[^\s(][^(]*?)(?:\((?P<l1>\d+),(?P<c1>\d+)\)|:(?P<l2>\d+):(?P<c2>\d+)): "
                  r"(?P<level>(?:fatal )?error|note): (?P<msg>.*)$")
HYBRID_ONLY = "hybrid-only name or form in shared code"
UNDECLARED = re.compile(r"(?:use of undeclared identifier|unknown type name|no template named|"
                        r"use of undeclared) '([^']+)'")
# A member the message names, with its class: (member, class).
MEMBER = (re.compile(r"(?:out-of-line (?:declaration|definition) of|no member named|no type named) '([^']+)' "
                     r"(?:does not match any declaration )?in '([^']+)'"),
          re.compile(r"specialization of member '(.+)::(\w+)' does not specialize"))


def compile_diagnostics(log):
    """(file, line, message, object) per error, the object being the one whose FAILED block printed it. An error
    inside the seam is placed at the first src/ line its notes name (the instantiation a form's call requested)."""
    current, pending = None, None
    for text in log.splitlines():
        m = FAILED.match(text)
        if m:
            if pending:
                yield pending
            current, pending = m.group(1), None
            continue
        m = DIAG.match(text)
        if not m:
            continue
        rel = os.path.relpath(os.path.normpath(os.path.join(BUILD, m.group("file"))), ROOT)
        line = int(m.group("l1") or m.group("l2"))
        if m.group("level") == "note":
            if (pending and pending[0].startswith("harness/") and rel.startswith(("src/", "gen/"))
                    and "instantiation" in m.group("msg")):
                yield (rel, line) + pending[2:]
                pending = None
            continue
        if pending:
            yield pending
        pending = (rel, line, m.group("msg"), current)
    if pending:
        yield pending


def compile_kind(message, source):
    low = message.lower()
    if "incomplete" in low:
        return "incomplete type"
    if "cannot combine with previous 'static' declaration specifier" in message:
        return HYBRID_ONLY
    m = UNDECLARED.search(message)
    if m:
        return HYBRID_ONLY if m.group(1).split("::")[-1] in source.hybrid_names() else "undeclared name"
    m = MEMBER[0].search(message)
    member, owner = (m.group(1), m.group(2)) if m else (None, None)
    m = MEMBER[1].search(message)
    if m:
        member, owner = m.group(2), m.group(1)
    if member:
        if source.hybrid_only_member(owner, member):
            return HYBRID_ONLY
        return "undeclared name" if low.startswith("no ") else "no matching function or overload"
    if re.search(r"is a (private|protected) member|(private|protected) (base|member)|inaccessible|"
                 r"protected|private", low):
        return "access"
    if re.search(r"no matching|no viable|ambiguous|cannot initialize|no known conversion|address of overloaded",
                 low):
        return "no matching function or overload"
    return "other"


# --- status: src scanning ------------------------------------------------------------------------------------------

CONDITION = re.compile(r"^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)$")


def hybrid_mask(lines):
    """Per line, True when it is on the hybrid-only side of `#if FNVD_HYBRID` / `#if !FNVD_HYBRID`."""
    stack, mask = [], []
    for text in lines:
        m = CONDITION.match(text)
        if m:
            directive, rest = m.group(1), re.sub(r"//.*|/\*.*?\*/", "", m.group(2)).strip()
            if directive in ("if", "ifdef", "ifndef"):
                state = "other"
                if directive == "if" and rest == "FNVD_HYBRID":
                    state = "hybrid"
                elif directive == "if" and re.fullmatch(r"!\s*FNVD_HYBRID", rest):
                    state = "native"
                stack.append(state)
            elif directive == "elif" and stack:
                stack[-1] = "other"
            elif directive == "else" and stack:
                stack[-1] = {"hybrid": "native", "native": "hybrid"}.get(stack[-1], "other")
            elif directive == "endif" and stack:
                stack.pop()
            mask.append(True)
            continue
        mask.append("hybrid" in stack)
    return mask


def source_files():
    for top in ("src", "gen"):
        for base, _, names in os.walk(os.path.join(ROOT, top)):
            for name in names:
                if name.endswith((".h", ".hpp", ".inl", ".cpp")):
                    yield os.path.relpath(os.path.join(base, name), ROOT)


class Source:
    """src/ and gen/ read once: each file's lines and its hybrid-only mask."""

    def __init__(self):
        self.files = {}
        for rel in sorted(source_files()):
            lines = read(os.path.join(ROOT, rel)).splitlines()
            self.files[rel] = (lines, hybrid_mask(lines))
        self._hybrid_names = None
        self._class_files = None
        self._members = {}

    def shared_lines(self):
        """(file, line number, text without its // comment) outside the hybrid-only sides."""
        for rel, (lines, mask) in self.files.items():
            for i, (text, hybrid) in enumerate(zip(lines, mask)):
                if not hybrid:
                    code = text.split("//", 1)[0]
                    if code.strip():
                        yield rel, i + 1, code

    def hybrid_names(self):
        """Every identifier on a hybrid-only side (src/, gen/, the seam)."""
        if self._hybrid_names is None:
            names = set()
            seam = read(os.path.join(ROOT, "harness", "seam", "Reconstruction.hpp")).splitlines()
            for lines, mask in list(self.files.values()) + [(seam, hybrid_mask(seam))]:
                for text, hybrid in zip(lines, mask):
                    if hybrid:
                        names.update(re.findall(r"[A-Za-z_]\w*", text.split("//", 1)[0]))
            self._hybrid_names = names
        return self._hybrid_names

    def hybrid_only_member(self, owner, member):
        """True when the files defining the class `owner` (its last name, template arguments aside) name `member`
        only on hybrid-only sides, out-of-line definitions (`::member`) aside."""
        if self._class_files is None:
            self._class_files = collections.defaultdict(set)
            pattern = re.compile(r"^\s*(?:template\s*<.*>\s*)?(?:class|struct|union)\s+(?:\w+\s+)*?(\w+)\b[^;]*$")
            for rel, (lines, _) in self.files.items():
                for text in lines:
                    m = pattern.match(text)
                    if m:
                        self._class_files[m.group(1)].add(rel)
        name = re.sub(r"<.*", "", re.sub(r"^(?:class|struct|union) ", "", owner)).split("::")[-1]
        key = (name, member)
        if key not in self._members:
            word = re.compile(r"\b%s\b" % re.escape(member))
            hybrid = shared = False
            for rel in self._class_files.get(name, ()):
                lines, mask = self.files[rel]
                for text, on_hybrid in zip(lines, mask):
                    code = text.split("//", 1)[0]
                    if word.search(code):
                        if on_hybrid:
                            hybrid = True
                        elif "::" + member not in code.replace(" ", ""):
                            shared = True
            self._members[key] = hybrid and not shared
        return self._members[key]


def registered_addresses(source):
    found = set()
    pattern = re.compile(r"FNVD_(?:REPLACES\w*|ADDRESS|DEFINE_\w+)\(\s*(0x[0-9A-Fa-f]+)")
    for lines, _ in source.files.values():
        for text in lines:
            for m in pattern.finditer(text):
                found.add(int(m.group(1), 16))
    return found


ORIGINAL = re.compile(r"\boriginal::(thiscall|cdecl_|stdcall|fastcall|usercall|vcall)\s*<")


def original_calls(source, bodies):
    """(kind, file, line, detail) per fnvd::original call outside the hybrid-only sides."""
    for rel, (lines, mask) in source.files.items():
        for i, text in enumerate(lines):
            if mask[i]:
                continue
            code = text.split("//", 1)[0]
            for m in ORIGINAL.finditer(code):
                # the address: the first argument, after the template arguments (which may run on)
                rest = " ".join([code[m.end():]] + [x.split("//", 1)[0] for x in lines[i + 1:i + 4]])
                if m.group(1) == "vcall":
                    yield "run: fnvd::original call, virtual slot", rel, i + 1, "vcall"
                    continue
                a = re.search(r">\s*\(\s*(0x[0-9A-Fa-f]+)", rest)
                address = int(a.group(1), 16) if a else None
                detail = "%s 0x%X" % (m.group(1), address) if address is not None else m.group(1)
                if rel.startswith("src/SDK/"):
                    kind = "run: fnvd::original call, CRT or SDK forwarder"
                elif address is not None and address in bodies:
                    kind = "run: fnvd::original call, address with a src body"
                else:
                    kind = "run: fnvd::original call, address with no body"
                yield kind, rel, i + 1, detail


def original_has_native(lines, line, code):
    """Whether the FNVD_ORIGINAL_FUNCTION call starting on `line` has its 4th argument (the src function it names
    without the exe): its arguments are read up to the closing parenthesis, over the next lines if they wrap."""
    text = " ".join([code[code.index("FNVD_ORIGINAL_FUNCTION(") + len("FNVD_ORIGINAL_FUNCTION("):]] +
                    [x.split("//", 1)[0] for x in lines[line:line + 5]])
    depth, args, current = 0, [], ""
    for ch in text:
        if ch in "(<[{":
            depth += 1
        elif ch in ")>]}":
            if depth == 0 and ch == ")":
                args.append(current)
                break
            depth -= 1
        elif ch == "," and depth == 0:
            args.append(current)
            current = ""
            continue
        current += ch
    return len(args) >= 4 and args[3].strip() != ""


# Seam forms whose standalone expansion drops some arguments (Reconstruction.hpp): form -> the dropped arguments'
# indices. An exe vtable constant there is never used natively.
DROPPED_ARGUMENTS = {"FNVD_INLINED_LOCAL": (2, 3)}
DROPPED_FORM = re.compile(r"\b(%s)\s*\(" % "|".join(DROPPED_ARGUMENTS))


def without_dropped_arguments(lines):
    """The file's lines with the arguments of DROPPED_ARGUMENTS' forms blanked out (their calls may wrap)."""
    text = "\n".join(lines)
    if not DROPPED_FORM.search(text):
        return lines
    chars = list(text)
    for m in DROPPED_FORM.finditer(text):
        dropped = DROPPED_ARGUMENTS[m.group(1)]
        depth, index, start = 0, 0, m.end()
        for pos in range(m.end(), len(text)):
            ch = text[pos]
            if ch in "(<[{":
                depth += 1
            elif ch in ")>]}" and depth:
                depth -= 1
            elif ch in ",)" and depth == 0:
                if index in dropped:
                    for i in range(start, pos):
                        if chars[i] != "\n":
                            chars[i] = " "
                if ch == ")":
                    break
                index, start = index + 1, pos + 1
    return "".join(chars).split("\n")


def run_rows(source, link_undefined_names):
    bodies = registered_addresses(source)
    rows = list(original_calls(source, bodies))
    vtables = set()
    for lines, _ in source.files.values():
        for text in lines:
            vtables.update(re.findall(r"FNVD_VFTABLE\(\s*(\w+)\s*,", text))
    import_slots = import_slot_names()
    defined_slots = set()
    # Per file, its lines as the standalone build sees the seam forms' arguments.
    native_lines = {}
    for rel, line, code in source.shared_lines():
        if "FNVD_ORIGINAL_FUNCTION(" in code and "#define" not in code and not original_has_native(
                source.files[rel][0], line, code):
            rows.append(("run: FNVD_ORIGINAL_FUNCTION with no native side", rel, line, code.strip()))
        if rel not in native_lines:
            native_lines[rel] = without_dropped_arguments(source.files[rel][0])
        native = native_lines[rel][line - 1].split("//", 1)[0]
        if "#define" not in code and "FNVD_VFTABLE(" not in code and (
                re.search(r"\bsetVtable\b", native) or not vtables.isdisjoint(re.findall(r"\w+", native))):
            rows.append(("run: exe vtable constant in shared code", rel, line, code.strip()))
        if "__RTDynamicCast(" in code and not rel.startswith("src/SDK/MSVC/"):
            rows.append(("run: __RTDynamicCast call", rel, line, code.strip()))
        d = re.search(r"FNVD_(?:DEFINE_\w+|IMPORT|IMPORT_AS|IMPORT_STUB)\(\s*[^,]+,\s*(\w+)\s*,", code)
        if d:
            defined_slots.add(d.group(1))
        d = re.match(r"^[A-Za-z_][\w:<>\s\*&]*?\b(\w+)\s*(?:=|\{)", code)
        if d and not re.match(r"\s*(extern|using|typedef|return|FNVD_)", code):
            defined_slots.add(d.group(1))
    for path in ("external",):
        for base, _, names in os.walk(os.path.join(ROOT, path)):
            for name in names:
                for code in read(os.path.join(base, name)).splitlines():
                    d = re.match(r"^[A-Za-z_][\w:<>\s\*&]*?\b(\w+)\s*(?:=|\{)", code)
                    if d:
                        defined_slots.add(d.group(1))
    for name, (address, dll) in sorted(import_slots.items()):
        if name not in defined_slots and name not in link_undefined_names:
            rows.append(("run: import slot with no definition", "", 0, "%s (%s, 0x%X)" % (name, dll, address)))
    return rows


def import_slot_names():
    """Class I rows of harness/staticinit.tsv: name -> (address, DLL from the exe's import table)."""
    slots = {}
    for text in read(STATICINIT).splitlines()[1:]:
        cols = text.split("\t")
        if len(cols) > 4 and cols[0] == "address" and cols[2] == "I":
            slots[cols[4]] = (int(cols[1], 16), "?")
    try:
        import pefile
        pe = pefile.PE(EXE, fast_load=True)
        pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_IMPORT"]])
        dll_at = {imp.address: entry.dll.decode().lower() for entry in pe.DIRECTORY_ENTRY_IMPORT
                  for imp in entry.imports}
        slots = {name: (address, dll_at.get(address, "?")) for name, (address, _) in slots.items()}
    except (ImportError, OSError):
        pass
    return slots


# --- status: link --------------------------------------------------------------------------------------------------


def link_undefined(log):
    """mangled name -> (file, line) of its first reference with a source location."""
    found = {}
    current = None
    for text in log.splitlines():
        m = re.search(r"error: undefined symbol: (\S+)", text)
        if m:
            current = m.group(1)
            found.setdefault(current, ("", 0))
            continue
        m = re.match(r">>> referenced by (.+?):(\d+)\s*$", text)
        if m and current and found[current] == ("", 0):
            found[current] = (os.path.relpath(m.group(1), ROOT) if m.group(1).startswith("/") else m.group(1),
                              int(m.group(2)))
    return found


def demangle(names):
    undname = shutil.which("llvm-undname") or os.path.join(LLVM, "llvm-undname")
    if not names or not os.path.exists(undname):
        return {n: n for n in names}
    out = subprocess.run([undname], input="\n".join(names) + "\n", capture_output=True, text=True).stdout
    # llvm-undname echoes each input line, then its demangled form, then a blank line
    lines = [x for x in out.splitlines() if x.strip()]
    result = {}
    it = iter(lines)
    for name in names:
        echoed = next(it, name)
        demangled = next(it, name) if echoed == name else echoed
        result[name] = demangled if not demangled.startswith("error:") else name
    return result


CONVENTION = re.compile(r"(?<!\()__(?:cdecl|thiscall|stdcall|fastcall|vectorcall|clrcall) ")


def link_kind(mangled, demangled, import_slots):
    if mangled.startswith(("??_7", "??_R")):
        return "link: vtable or RTTI"
    base = re.match(r"\?(\w+)@@3", mangled)
    if base and base.group(1) in import_slots:
        dll = import_slots[base.group(1)][1]
        return ("link: import slot, third-party DLL" if dll in THIRD_PARTY_DLLS
                else "link: import slot, Windows DLL")
    rest = re.sub(r"^\?\?_?[0-9A-Z]", "", mangled) if mangled.startswith("??") else mangled[1:]
    first = rest.find("@@")
    template = mangled.startswith("??$") or rest.startswith("?$") or (0 <= rest.find("@?$") < first)
    function = bool(CONVENTION.search(demangled))
    if template:
        return "link: template instance member with no generic body"
    if function:
        return "link: function with no body"
    if mangled.startswith("?"):
        return "link: global or static member with no definition"
    # a C name: `_f@8` (stdcall) or `@f@8` (fastcall) is a function
    if re.search(r"@\d+$", mangled):
        return "link: function with no body"
    return "link: other"


# --- status: the Linux program ---------------------------------------------------------------------------------------

# The exe's Windows DLLs other than kernel32/advapi32/shell32, by the group the Linux plan replaces them as.
DLL_GROUPS = {"user32.dll": "user32", "gdi32.dll": "user32", "comctl32.dll": "user32", "ole32.dll": "COM",
              "oleaut32.dll": "COM", "winmm.dll": "winmm", "dsound.dll": "dsound", "d3dx9_38.dll": "d3dx9",
              "d3d9.dll": "d3d9", "wsock32.dll": "Winsock", "ws2_32.dll": "Winsock",
              "dinput8.dll": "DirectInput/XInput", "xinput1_3.dll": "DirectInput/XInput"}
SYSTEM_DLLS = {"kernel32.dll", "advapi32.dll", "shell32.dll"}
# kernel32, advapi32 and shell32 functions by the Linux plan's areas (env/scratch/Linux/measurements.md section 2),
# the first pattern that matches.
SYSTEM_AREAS = [
    ("ini", r"PrivateProfile"),
    ("registry", r"^Reg"),
    ("shell and user folders", r"^SH|^Shell|CommandLine|GetUserName"),
    ("strings", r"^lstr|MultiByteToWideChar|WideCharToMultiByte"),
    ("sync", r"CriticalSection|^Interlocked|^Tls|Semaphore|Event[AW]?$|Mutex|^WaitFor|^CloseHandle$"),
    ("threads", r"Thread|^Sleep"),
    ("time", r"TickCount|Performance(Counter|Frequency)|Time"),
    ("files", r"File|Directory|^Find|LastError|FormatMessage|^LocalFree$|DiskFree|DriveType|PathName|TempPath"),
    ("memory and system info", r"^Virtual|^Global|^Heap|^Local|MemoryStatus|SystemInfo"),
    ("dynamic loading", r"LoadLibrary|GetProcAddress|FreeLibrary|GetModuleHandle|GetModuleFileName"),
    ("debug and process", r"Debug|RaiseException|Process|UnhandledExceptionFilter|ProcessorFeature"),
]
GUID = re.compile(r"^(?:IID|CLSID|GUID|LIBID|MEDIATYPE|MEDIASUBTYPE|FORMAT|TIME_FORMAT|PIN_CATEGORY|DS3DALG)_")
IMPORT_DECL = re.compile(r"FNVD_IMPORT(?:_STUB)?\(\s*[^,]+,\s*(\w+)\s*,\s*(0x[0-9A-Fa-f]+)")


def win32_dlls():
    """C name -> DLL (lower case): the exe's imports by name, and src's FNVD_IMPORT names by their slot's DLL."""
    by_name, by_address = {}, {}
    try:
        import pefile
        pe = pefile.PE(EXE, fast_load=True)
        pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_IMPORT"]])
        for entry in pe.DIRECTORY_ENTRY_IMPORT:
            dll = entry.dll.decode().lower()
            for imp in entry.imports:
                by_address[imp.address] = dll
                if imp.name:
                    by_name[imp.name.decode()] = dll
    except (ImportError, OSError):
        pass
    for rel in source_files():
        for m in IMPORT_DECL.finditer(read(os.path.join(ROOT, rel))):
            dll = by_address.get(int(m.group(2), 16))
            if dll:
                by_name.setdefault(m.group(1), dll)
    return by_name


def crt_names():
    """The C runtime functions src/SDK/MSVC forwards to (`::name(`) or declares `extern "C"`."""
    names = set()
    for base, _, files in os.walk(os.path.join(ROOT, "src", "SDK", "MSVC")):
        for name in files:
            text = read(os.path.join(base, name))
            names.update(re.findall(r"(?<![\w:])::(\w+)\s*\(", text))
            names.update(re.findall(r"extern \"C\"[^;(]*?\b(\w+)\s*\(", text))
    return names


def link_undefined_sites(log):
    """mangled name -> [file, line, references]: lld's first reference with a source location, and the number of
    references it lists (`referenced by` lines plus `referenced N more times`)."""
    found = {}
    current = None
    for text in log.splitlines():
        m = re.search(r"error: undefined symbol: (\S+)", text)
        if m:
            current = found.setdefault(m.group(1), ["", 0, 0])
            continue
        if current is None:
            continue
        m = re.match(r">>> referenced (\d+) more times", text)
        if m:
            current[2] += int(m.group(1))
            continue
        m = re.match(r">>> referenced by (.*?)\s*$", text)
        if m:
            current[2] += 1
            if not current[0]:
                where = re.search(r"\((/[^()]+):(\d+)\)$", m.group(1)) or re.match(r"(\S+):(\d+)", m.group(1))
                if where:
                    path = where.group(1)
                    current[0] = os.path.relpath(path, ROOT) if path.startswith("/") else path
                    current[1] = int(where.group(2))
            continue
        # the referencing object, which names the source file when the reference has no line
        m = re.match(r">>>\s+(\S.*?\.o):\(", text)
        if m and not current[0]:
            current[0] = object_source(m.group(1))
        elif not text.startswith(">>>"):
            current = None
    return found


def demangle_itanium(names):
    cxxfilt = shutil.which("llvm-cxxfilt") or os.path.join(LLVM, "llvm-cxxfilt")
    if not names or not os.path.exists(cxxfilt):
        return {n: n for n in names}
    out = subprocess.run([cxxfilt], input="\n".join(names) + "\n", capture_output=True, text=True).stdout
    return dict(zip(names, out.splitlines()))


def linux_link_kind(name, demangled, dlls, crt):
    if not name.startswith("_Z"):
        dll = dlls.get(name)
        if name == "main":
            return "link: entry point"
        if dll in SYSTEM_DLLS:
            area = next((a for a, p in SYSTEM_AREAS if re.search(p, name)), "other")
            return "link: Win32 system, " + area
        if dll in DLL_GROUPS:
            return "link: Win32 " + DLL_GROUPS[dll]
        if dll in THIRD_PARTY_DLLS:
            return "link: third-party DLL"
        if dll:
            return "link: Win32 other DLL"
        if GUID.match(name):
            return "link: Win32 GUIDs"
        if name.startswith("c_dfDI"):
            return "link: Win32 DirectInput/XInput"
        if name in crt or re.match(r"_[a-z]", name):
            return "link: CRT (msvc9)"
        # the Itanium ABI does not mangle a variable at global namespace scope: the game's own globals
        return "link: global or static member with no definition"
    if name.startswith(("_ZTV", "_ZTI", "_ZTS", "_ZTT")):
        return "link: vtable or RTTI"
    if demangled.startswith("msvc9::"):
        return "link: CRT (msvc9)"
    qualified = demangled.split("(", 1)[0]
    if "<" in qualified:
        return "link: template instance member with no generic body"
    if "(" in demangled:
        return "link: function with no body"
    return "link: global or static member with no definition"


def object_source(obj):
    """The source file of a CMake object (`.../fnvd_engine.dir/./src/X.cpp.o` -> src/X.cpp)."""
    m = re.search(r"\.dir/(?:\./)?(.+)\.o(?:bj)?$", obj)
    return m.group(1) if m else obj


def cmd_status_linux(args):
    build_log, link_log = read(BUILD_LOG), read(LINK_LOG)
    state = json.loads(read(STATE) or "{}")
    rows = []
    first = {}
    for rel, line, message, obj in compile_diagnostics(build_log):
        if obj and obj not in first:
            first[obj] = (rel, line, message)
    # an object ninja reports failed with no diagnostic it could be tied to
    for obj in FAILED.findall(build_log):
        first.setdefault(obj, ("", 0, "(no error diagnostic in the log)"))
    for obj, (rel, line, message) in first.items():
        rows.append(("compile: object fails to compile", object_source(obj), 0,
                     "%s:%s: %s" % (rel, line, message) if rel else message))
    undefined = link_undefined_sites(build_log + "\n" + link_log)
    names = demangle_itanium(sorted(undefined))
    dlls, crt = win32_dlls(), crt_names()
    sites = collections.Counter()
    for name, (rel, line, refs) in sorted(undefined.items()):
        kind = linux_link_kind(name, names[name], dlls, crt)
        sites[kind] += refs
        rows.append((kind, rel, line, "%s (%d references)" % (names[name], refs)))
    for text in read(os.path.join(BUILD, "missing-libraries.txt")).split():
        rows.append(("library: missing", "", 0, text))
    for m in re.finditer(r"^ld\.lld: error: (?!undefined symbol)(.*)$", build_log + "\n" + link_log, re.M):
        rows.append(("link: other error", "", 0, m.group(1)))

    counts = write_table(rows, args)
    print("link: %s, %s objects failed to compile" % (state.get("link", "?"), state.get("objects_failed", "?")))
    for kind in sorted(counts):
        extra = ("\t(%d references)" % sites[kind]) if kind in sites else ""
        print("%s\t%d%s" % (kind, counts[kind], extra))
    if first:
        print("commonest first errors of the failing objects:")
        for (rel, line, message), n in collections.Counter(first.values()).most_common(10):
            print("  %d\t%s:%s: %s" % (n, rel, line, message[:140]))
        print("(while objects fail to compile, the link kinds of the game's own symbols include symbols the failing "
              "objects define)")
    finish(counts, args)



# --- status --------------------------------------------------------------------------------------------------------


def write_table(rows, args):
    """Writes the status table, returns the count per kind."""
    table = args.table or os.path.join(BUILD, "status.tsv")
    with open(table, "w") as f:
        f.write("kind\tfile\tline\tsymbol_or_message\n")
        for kind, rel, line, detail in sorted(rows, key=lambda r: (r[0], r[1], r[2])):
            f.write("%s\t%s\t%s\t%s\n" % (kind, rel, line or "", detail.replace("\t", " ")))
    return collections.Counter(r[0] for r in rows)


def finish(counts, args):
    total = sum(counts.values())
    print("total\t%d" % total)
    print("table: %s" % os.path.relpath(args.table or os.path.join(BUILD, "status.tsv"), os.getcwd()))
    sys.exit(1 if total else 0)


def cmd_status(args):
    if not os.path.exists(BUILD_LOG):
        sys.exit("noexe: no %s: run `noexe.py build` first" % BUILD_LOG)
    if LINUX:
        cmd_status_linux(args)
    build_log, link_log = read(BUILD_LOG), read(LINK_LOG)
    state = json.loads(read(STATE) or "{}")
    source = Source()
    rows = []
    compile_objects = collections.defaultdict(set)
    seen = set()
    kinds = {}
    for rel, line, message, obj in compile_diagnostics(build_log):
        if message not in kinds:
            kinds[message] = "compile: " + compile_kind(message, source)
        kind = kinds[message]
        if obj:
            compile_objects[kind].add(obj)
        if (rel, line, message) not in seen:
            seen.add((rel, line, message))
            rows.append((kind, rel, line, message))
    undefined = link_undefined(build_log + "\n" + link_log)
    import_slots = import_slot_names()
    names = demangle(sorted(undefined))
    undefined_slot_names = set()
    for mangled, (rel, line) in sorted(undefined.items()):
        kind = link_kind(mangled, names[mangled], import_slots)
        if kind.startswith("link: import slot"):
            undefined_slot_names.add(re.match(r"\?(\w+)@@3", mangled).group(1))
        rows.append((kind, rel, line, names[mangled]))
    for text in read(os.path.join(BUILD, "missing-libraries.txt")).split():
        rows.append(("library: missing", "", 0, text))
    for m in re.finditer(r"^lld-link: error: (?!undefined symbol)(.*)$", build_log + "\n" + link_log, re.M):
        rows.append(("link: other error", "", 0, m.group(1)))
    rows.extend(run_rows(source, undefined_slot_names))

    counts = write_table(rows, args)
    print("link: %s, %s objects failed to compile" % (state.get("link", "?"), state.get("objects_failed", "?")))
    for kind in sorted(counts):
        extra = ("\t(%d objects)" % len(compile_objects[kind])) if kind.startswith("compile: ") else ""
        print("%s\t%d%s" % (kind, counts[kind], extra))
    finish(counts, args)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    for name in ("build", "status"):
        p = sub.add_parser(name)
        if name == "build":
            p.add_argument("--jobs", type=int, default=8)
        else:
            p.add_argument("--table")
        p.add_argument("--arch", choices=("x86", "x64"), default="x86", help="the Windows program's architecture")
        p.add_argument("--target", choices=("windows", "linux"), default="windows",
                       help="the Windows no-exe program (default) or the native Linux x64 program (build/linux)")
        p.add_argument("--sdl-audio", action="store_true",
                       help="the Windows program with external/dsound's SDL3 audio (build/noexe[64]-sdlaudio)")
    args = parser.parse_args()
    if args.target == "linux" and args.arch != "x86":
        parser.error("--arch applies to the Windows program only")
    if args.target == "linux" and args.sdl_audio:
        parser.error("--sdl-audio applies to the Windows program only (the Linux program always has the layer)")
    set_target(args.target, args.arch, args.sdl_audio)
    {"build": cmd_build, "status": cmd_status}[args.command](args)


if __name__ == "__main__":
    main()
