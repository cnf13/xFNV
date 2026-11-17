# Builds the 32-bit Windows program (the reconstruction without the original FalloutNV.exe) on Windows itself with
# clang-cl + lld-link: the native counterpart of i686-windows-clang-cl.cmake. The MSVC CRT / STL and the Windows SDK
# are the installed Visual Studio's and Windows Kits', which clang-cl and lld-link find themselves (no vcvars, no
# xwin splat). Windows' file names ignore case, so the splat's Advapi32.lib link is not needed here.
#   FNVD_LLVM_BIN   folder holding clang-cl / lld-link / llvm-lib / llvm-rc / llvm-mt (default: PATH, then the LLVM
#                   installer's folder)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

set(FNVD_LLVM_BIN "" CACHE PATH "LLVM bin folder (empty: PATH, then the LLVM installer's folder)")
set(_hints "${FNVD_LLVM_BIN}" "$ENV{ProgramFiles}/LLVM/bin")

find_program(CMAKE_C_COMPILER clang-cl HINTS ${_hints} REQUIRED)
find_program(CMAKE_CXX_COMPILER clang-cl HINTS ${_hints} REQUIRED)
find_program(CMAKE_LINKER lld-link HINTS ${_hints} REQUIRED)
find_program(CMAKE_AR llvm-lib HINTS ${_hints} REQUIRED)
find_program(CMAKE_RC_COMPILER llvm-rc HINTS ${_hints} REQUIRED)
find_program(CMAKE_MT llvm-mt HINTS ${_hints} REQUIRED)

set(CMAKE_C_FLAGS_INIT "--target=i686-pc-windows-msvc")
set(CMAKE_CXX_FLAGS_INIT "--target=i686-pc-windows-msvc")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
