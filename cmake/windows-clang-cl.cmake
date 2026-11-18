# Cross-compiles the Windows program from Linux with clang-cl and lld-link against an xwin-splatted MSVC CRT and
# Windows SDK (both only read). Included by the toolchain file of each architecture, which sets:
#   XFNV_WINDOWS_PROCESSOR  CMAKE_SYSTEM_PROCESSOR (x86, AMD64)
#   XFNV_WINDOWS_TRIPLE     clang's target (i686-pc-windows-msvc, x86_64-pc-windows-msvc)
#   XFNV_WINDOWS_LIB_ARCH   the splat's library folder (x86, x86_64)
# Cache variables:
#   XFNV_LLVM_BIN   folder holding clang-cl / lld-link / llvm-lib / llvm-rc / llvm-mt (default: from PATH)
#   XFNV_XWIN       xwin splat output with crt/ and sdk/ (default ~/.local/opt/xwin-sdk)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR ${XFNV_WINDOWS_PROCESSOR})

set(XFNV_XWIN "$ENV{HOME}/.local/opt/xwin-sdk" CACHE PATH "xwin splat output (crt/, sdk/)")
if(DEFINED ENV{XFNV_XWIN})
    set(XFNV_XWIN "$ENV{XFNV_XWIN}" CACHE PATH "" FORCE)
endif()
set(XFNV_LLVM_BIN "" CACHE PATH "LLVM bin folder (empty: PATH)")

find_program(CMAKE_C_COMPILER clang-cl HINTS "${XFNV_LLVM_BIN}" REQUIRED)
find_program(CMAKE_CXX_COMPILER clang-cl HINTS "${XFNV_LLVM_BIN}" REQUIRED)
find_program(CMAKE_LINKER lld-link HINTS "${XFNV_LLVM_BIN}" REQUIRED)
find_program(CMAKE_AR llvm-lib HINTS "${XFNV_LLVM_BIN}" REQUIRED)
find_program(CMAKE_RC_COMPILER llvm-rc HINTS "${XFNV_LLVM_BIN}" REQUIRED)
find_program(CMAKE_MT llvm-mt HINTS "${XFNV_LLVM_BIN}" REQUIRED)

set(_inc
    "/imsvc${XFNV_XWIN}/crt/include"
    "/imsvc${XFNV_XWIN}/sdk/include/ucrt"
    "/imsvc${XFNV_XWIN}/sdk/include/um"
    "/imsvc${XFNV_XWIN}/sdk/include/shared"
    "/imsvc${XFNV_XWIN}/sdk/include/winrt")
list(JOIN _inc " " _inc)
set(CMAKE_C_FLAGS_INIT "--target=${XFNV_WINDOWS_TRIPLE} ${_inc}")
set(CMAKE_CXX_FLAGS_INIT "--target=${XFNV_WINDOWS_TRIPLE} ${_inc}")
# llvm-rc's include paths (external/SDL3's version.rc includes winresrc.h)
set(CMAKE_RC_FLAGS_INIT "/I${XFNV_XWIN}/sdk/include/um /I${XFNV_XWIN}/sdk/include/shared")

# MS STL's tzdb.obj (std::chrono::current_zone, src/SDK/Windows/winbase.cpp) asks for "Advapi32.lib", a casing the
# splat lacks: a folder in the build tree holds that name as a link to the splat's advapi32.lib.
set(_libcase "${CMAKE_BINARY_DIR}/libcase")
file(MAKE_DIRECTORY "${_libcase}")
file(CREATE_LINK "${XFNV_XWIN}/sdk/lib/um/${XFNV_WINDOWS_LIB_ARCH}/advapi32.lib" "${_libcase}/Advapi32.lib" SYMBOLIC)
set(_lib
    "/libpath:${XFNV_XWIN}/crt/lib/${XFNV_WINDOWS_LIB_ARCH}"
    "/libpath:${XFNV_XWIN}/sdk/lib/ucrt/${XFNV_WINDOWS_LIB_ARCH}"
    "/libpath:${XFNV_XWIN}/sdk/lib/um/${XFNV_WINDOWS_LIB_ARCH}"
    "/libpath:${_libcase}")
list(JOIN _lib " " _lib)
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_lib}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_lib}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_lib}")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
