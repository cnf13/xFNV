# xFNV

A reconstruction of Fallout: New Vegas as C++ source, recovered from the decompiled exe and kept in the original's
folder layout. It builds a standalone `FalloutNV` program: 32- and 64-bit Windows, and 64-bit Linux (Direct3D 9 through
DXVK, windowing, input and audio through SDL3). It started as a hybrid DLL running inside the original exe; the "seam"
header that bridged the two (`src/xfnv/seam/Reconstruction.hpp`) was removed in `0b9021fa`.

The rules for working here are in `.omp/RULES.md`. Open work is in `FOLLOWUPS.md` (git-ignored).

## Layout

| Path | Contents |
|---|---|
| `src/BS*` | Bethesda's engine layer: core containers and memory (`BSCore`), audio, Havok glue (`BSHavok`, `bhk*`), shaders, menus, system |
| `src/Fallout`, `src/Fallout Shared` | The game: AI, combat, interface and menus, forms (`TES*`, `BGS*`), scripts, pathfinding, save/load |
| `src/Gamebryo2.2` | Gamebryo 2.2 (`Ni*`): NiMain, NiAnimation, NiDX9Renderer, NiCollision, particles |
| `src/Havok` | Havok SDK (`hk*`, `hkp*`, `hka*`) |
| `src/SpeedTree` | SpeedTreeRT |
| `src/SDK` | Third-party headers and code the exe links: MSVC CRT pieces, DirectX, DirectShow, Steam, Bink, FaceGen, Windows. Not clang-formatted (tabs). |
| `src/platform` | The layer between the game and the OS (`Win32/`, `Posix/`), and stand-ins for imported DLLs (`d3dx9`, `dinput_sdl`, `dsound`, `bink`, `steam`). `src/platform/README` documents each call. |
| `src/xfnv` | The project's own code: `Compiler.h` (clang/cl differences), `crashlog`, `debugheap` |
| `external` | Vendored libraries (dxvk, SDL3, ogg, vorbis, zlib, minimp3, doctest, import libs); `external/README` records versions and hashes |
| `tests` | doctest unit tests built from real game files plus link-seam fakes; see `tests/README.md` |
| `cmake` | Toolchain files, `Ccache.cmake`, `Dxvk.cmake` |

The game target is every `.cpp` under `src/` except `platform/` and `xfnv/`. Paths contain spaces
(`src/Fallout Shared`): quote them, and never split file lists on whitespace.

## Building and testing

- Main tree: `build/` (Ninja, clang 23, RelWithDebInfo, `XFNV_TESTS=ON`). `build/compile_commands.json` is what the
  tools below read. Build: `cmake --build build`; list every error: `cmake --build build -- -k 0`. A header change
  rebuilds most of the tree, so run it in the background.
- Tests: `ctest --test-dir build`, or `cmake --build build --target xfnv_test` to build and run all. There is one test
  program, `xfnv_test_scriptcompiler`. Tests use the Itanium ABI and are not built under MSVC.
- C++26 (C++23 under cl). Clang builds pass `-fno-delete-null-pointer-checks` and `-ffp-contract=off`.
- Other configurations:
  - `cmake/i686-windows-clang-cl.cmake` and `cmake/x86_64-windows-clang-cl.cmake`: clang-cl and lld-link against the
    xwin splat (`XFNV_XWIN`, default `~/.local/opt/xwin-sdk`). Use RelWithDebInfo; the Debug link lacks
    `libcmtd`/`libcpmtd`.
  - `cmake/x86_64-windows-msvc-wine.cmake`: Microsoft's cl under Wine (msvc-wine); the user's tree is `build-msvc-x64/`.
  - `cmake/x86_64-linux-clang.cmake`: forces clang; without a toolchain file CMake may pick gcc.
  - `cmake-build-*` are CLion's profile directories.
- Options: `XFNV_NO_STEAM`, `XFNV_FIX_FAST_MAIN_MENU`, `XFNV_CRASH_LOG`, `XFNV_SYSTEM_ALLOC`, `XFNV_GUARD_ALLOC`,
  `XFNV_ASAN`, `XFNV_SDL_AUDIO`, `XFNV_D3DX`, `XFNV_TESTS`. `XFNV_64BIT` is a global definition (1 on 64-bit targets).
- Comments in CMake and `.gitignore` mention `harness/`, `build.py` and `docs/ENVIRONMENT.md`; this checkout has none
  of them.

## Verification tools (`.omp/tools/`, git-ignored)

- `syntax.py <file>...`: `-fsyntax-only` with each file's exact flags from `build/compile_commands.json`.
- `asmdiff.py [-v] [--noeh] [--base=REV] <file>...`: compiles REV's (default HEAD) and the on-disk version of each file
  with its flags at the build's optimisation, disassembles both and compares every function after normalising
  relocations, jump offsets and padding. Prints `OK` or `DIFF` with the differing functions; `-v` shows the
  instructions. `--noeh` adds `-fignore-exceptions`, hiding new-expression landing pads. Headers come from disk on
  both sides, so header edits need a full build to judge.
- Run them as `python3 -I ...` (keeps stray `.py` files in the working directory from shadowing modules), one file
  per process when output is redirected in parallel.

### Accepted equivalences

When code cannot stay identical, these differences have been accepted:

- a commuted single add or multiply;
- packed versus scalar SSE where only lane 0 is read;
- store order on a fresh or owned object with no call in between and the same final values;
- the same constant read from a different, never-written source;
- a null test that is always true, and the placement-new null test MSVC emits and clang drops;
- a call the compiler chose not to inline, or calls to a function the repo defines out of line;
- extra read-only calls;
- exception landing pads from new-expressions;
- a destructor that only frees memory, drops a string-table reference or is trivial running at the end of its scope
  instead of at the hand-written point. A destructor that can run other code (refcount drops that may delete,
  writers, locks) keeps its position, using a `{ }` block or a temporary.

## Reading the code

The decompiler lowered a lot of C++; much of the project's work is writing it back. Forms to recognise:

- **`// EXE-INLINED 0xADDR` / `// EXE-INLINED none: X::f`** mark a hand-written body of a function the exe inlined;
  `0xADDR` is the exe's out-of-line copy, `none:` means the exe has none. Replacing a marker means calling the
  function instead, when its body in the repo matches the exe's. 1,949 remain (694 address-only).
- **Hidden result pointers.** MSVC returns classes through a caller-provided slot; the decompiled form is
  `T* f(T* apResult, ...)` with decompiler-named caller temporaries (`out`, `v55`, `apResult`). These are restored to
  `T f(...)`; a few are left (FOLLOWUPS.md).
- **Identical-code folding.** MSVC merged identical functions, and IDA named each merged copy after one class. A
  `delete static_cast<Date*>(x)` or a `BSSimpleList<ActiveEffect*>` cast is usually the folded deleting destructor,
  not the real type.
- **Allocation.** `NiMemObject::operator new` is `noexcept`, so `new T` null-tests like the hand code did. The class
  allocators of `bhkRagdollController`, `bhkRagdollPenetrationUtil`, `hkReferencedObject` and `hkpEntity` are not
  `noexcept`. `ourMalloc(sizeof(T))` + placement new and `MemoryManager_Singleton.Allocate` are the game's other
  forms. Global `new`/`delete` go through MemoryManager.
- **x87 precision.** Values the exe keeps on the x87 stack are written as `double`, with `static_cast<float>` where
  the exe stores a dword.
- **Null `this`.** Game code calls members on null pointers and the members test `this` (BSSimpleList walks start at
  `this`); the build keeps those tests.
- **Containers.** `hkArrayUtil::construct` default-initialises, as the SDK does. `NiTArray`, `NiTLargeArray` and
  `BSSimpleArray` have `begin()`/`end()` over `[0, size)`, including NiTArray's empty slots. `hkLocalBuffer` has no
  `end()`. `BSSimpleList::ItemsUntilNull()` stops at a null item; a plain range-for over an empty list yields its null
  head.
- **Memory contexts.** The thread's memory context lives in two places, `GetThreadMemContext()` (a `thread_local`)
  and `TLSData::eMemContext`; in the exe they are one value.

## History

| Commits | Work |
|---|---|
| `78e25a2d`..`ca7216d5` | Initial import, clang-format, behaviour-preserving clang-tidy passes |
| `806309b8`, `30d3c8b9`, `339ced97` | TESScriptCompiler cleanup, unit tests, `fnvd` → `xfnv` rename |
| `851d1b8c`..`0b9021fa` | Seam removal: one-line forms, return by value per type, raw locals made real, `XFNV_RECONSTRUCT`, adapters, DLL import prototypes, header deleted |
| `4f4b7e3a`..`c0777267` | EXE-INLINED groups 1–4: stores and smart pointers, containers/streams/endian, world locks, math |
| `91e40f2e`..`2b1a04de` | Full-precision NiPoint3/NiMatrix3/NiColor math, default-initialising construct, missing SDK inlines, forced-inline removal |
| `9d8e61c8`..`fd4f641b` | Explicit operator calls written as operators; hand-expanded allocations written as new-expressions |
| `af019a51`..`9631f568` | Range-for over C arrays and containers; deletes through real types; `void*` temporaries removed |
| `b9440dba` | MSVC (cl) build support: builtins replaced with standard library calls and `Compiler.h` macros |

The `NiVulkanRenderer` branch (worktree `.omp/wt/NiVulkanRenderer`, from `6d003afb`) predates the rename and still
uses `fnvd` names.

## Open work

`FOLLOWUPS.md` is the list. Main groups, counted at `b9440dba`:

- 1,949 EXE-INLINED markers; the container and math markers left by the group passes have recorded reasons in
  FOLLOWUPS.md.
- About 300 `operator->()` uses as raw pointers, 145 explicit conversion operators, 29 explicit `.operator=` calls and
  30 other explicit operator calls (some kept for evaluation order).
- 96 hand-expanded `operator new(sizeof ...)` sites and 14 hand-expanded array news (`>> 32 ? 0xFFFFFFFF`).
- Raw-pointer members that should be `NiPointer`s, SDK default constructors (`hkpMassProperties` and others),
  `hkpProcessCollisionInput`'s spare-sector frees: these need IDA.
- The two memory contexts: the one item that may be a real behaviour difference.
- The `void*` audit table at the end of FOLLOWUPS.md.
