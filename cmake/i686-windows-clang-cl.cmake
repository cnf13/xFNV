# Cross-compiles the 32-bit Windows program (the reconstruction without the original FalloutNV.exe) from Linux with
# clang-cl + lld-link against an xwin-splatted MSVC CRT / Windows SDK. Both are only read.
#   FNVD_LLVM_BIN   folder holding clang-cl / lld-link / llvm-lib / llvm-rc / llvm-mt (default: from PATH)
#   FNVD_XWIN       xwin splat output with crt/ and sdk/ (default ~/.local/opt/xwin-sdk)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

set(FNVD_XWIN "$ENV{HOME}/.local/opt/xwin-sdk" CACHE PATH "xwin splat output (crt/, sdk/)")
if(DEFINED ENV{FNVD_XWIN})
    set(FNVD_XWIN "$ENV{FNVD_XWIN}" CACHE PATH "" FORCE)
endif()
set(FNVD_LLVM_BIN "" CACHE PATH "LLVM bin folder (empty: PATH)")

find_program(CMAKE_C_COMPILER clang-cl HINTS "${FNVD_LLVM_BIN}" REQUIRED)
find_program(CMAKE_CXX_COMPILER clang-cl HINTS "${FNVD_LLVM_BIN}" REQUIRED)
find_program(CMAKE_LINKER lld-link HINTS "${FNVD_LLVM_BIN}" REQUIRED)
find_program(CMAKE_AR llvm-lib HINTS "${FNVD_LLVM_BIN}" REQUIRED)
find_program(CMAKE_RC_COMPILER llvm-rc HINTS "${FNVD_LLVM_BIN}" REQUIRED)
find_program(CMAKE_MT llvm-mt HINTS "${FNVD_LLVM_BIN}" REQUIRED)

set(_inc
    "/imsvc${FNVD_XWIN}/crt/include"
    "/imsvc${FNVD_XWIN}/sdk/include/ucrt"
    "/imsvc${FNVD_XWIN}/sdk/include/um"
    "/imsvc${FNVD_XWIN}/sdk/include/shared"
    "/imsvc${FNVD_XWIN}/sdk/include/winrt")
list(JOIN _inc " " _inc)
set(CMAKE_C_FLAGS_INIT "--target=i686-pc-windows-msvc ${_inc}")
set(CMAKE_CXX_FLAGS_INIT "--target=i686-pc-windows-msvc ${_inc}")
# llvm-rc's include paths (external/SDL3's version.rc includes winresrc.h)
set(CMAKE_RC_FLAGS_INIT "/I${FNVD_XWIN}/sdk/include/um /I${FNVD_XWIN}/sdk/include/shared")

# MS STL's tzdb.obj (std::chrono::current_zone, src/SDK/Windows/winbase.cpp) asks for "Advapi32.lib", a casing the
# splat lacks: a folder in the build tree holds that name as a link to the splat's advapi32.lib.
set(_libcase "${CMAKE_BINARY_DIR}/libcase")
file(MAKE_DIRECTORY "${_libcase}")
file(CREATE_LINK "${FNVD_XWIN}/sdk/lib/um/x86/advapi32.lib" "${_libcase}/Advapi32.lib" SYMBOLIC)
set(_lib
    "/libpath:${FNVD_XWIN}/crt/lib/x86"
    "/libpath:${FNVD_XWIN}/sdk/lib/ucrt/x86"
    "/libpath:${FNVD_XWIN}/sdk/lib/um/x86"
    "/libpath:${_libcase}")
list(JOIN _lib " " _lib)
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_lib}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_lib}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_lib}")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
