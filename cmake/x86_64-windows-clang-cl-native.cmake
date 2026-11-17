# Builds the 64-bit Windows program without the exe on Windows itself with clang-cl + lld-link: the native
# counterpart of x86_64-windows-clang-cl.cmake (and the 64-bit twin of i686-windows-clang-cl-native.cmake). The MSVC
# CRT / STL and the Windows SDK are the installed Visual Studio's and Windows Kits', which clang-cl and lld-link find
# themselves (no vcvars, no xwin splat).
#   FNVD_LLVM_BIN   folder holding clang-cl / lld-link / llvm-lib / llvm-rc / llvm-mt (default: PATH, then the LLVM
#                   installer's folder)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

set(FNVD_LLVM_BIN "" CACHE PATH "LLVM bin folder (empty: PATH, then the LLVM installer's folder)")
set(_hints "${FNVD_LLVM_BIN}" "$ENV{ProgramFiles}/LLVM/bin")

find_program(CMAKE_C_COMPILER clang-cl HINTS ${_hints} REQUIRED)
find_program(CMAKE_CXX_COMPILER clang-cl HINTS ${_hints} REQUIRED)
find_program(CMAKE_LINKER lld-link HINTS ${_hints} REQUIRED)
find_program(CMAKE_AR llvm-lib HINTS ${_hints} REQUIRED)
find_program(CMAKE_RC_COMPILER llvm-rc HINTS ${_hints} REQUIRED)
find_program(CMAKE_MT llvm-mt HINTS ${_hints} REQUIRED)

set(CMAKE_C_FLAGS_INIT "--target=x86_64-pc-windows-msvc")
set(CMAKE_CXX_FLAGS_INIT "--target=x86_64-pc-windows-msvc")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
