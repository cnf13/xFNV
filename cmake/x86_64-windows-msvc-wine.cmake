# The 64-bit Windows program, cross-compiled from Linux with Microsoft's own cl and link, run under Wine by msvc-wine
# (github.com/mstorsjo/msvc-wine), whose wrappers set the MSVC CRT's and Windows SDK's paths themselves.
# Cache variable:
#   XFNV_MSVC_WINE  msvc-wine's install folder, holding bin/x64/ (-D, or the environment variable of that name)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

set(XFNV_MSVC_WINE "$ENV{XFNV_MSVC_WINE}" CACHE PATH "msvc-wine install folder (bin/x64/)")
# try_compile's projects run this file again without the cache.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES XFNV_MSVC_WINE)
if(NOT IS_DIRECTORY "${XFNV_MSVC_WINE}/bin/x64")
    message(FATAL_ERROR "XFNV_MSVC_WINE (\"${XFNV_MSVC_WINE}\") is not an msvc-wine install with bin/x64/")
endif()

find_program(CMAKE_C_COMPILER cl HINTS "${XFNV_MSVC_WINE}/bin/x64" NO_DEFAULT_PATH REQUIRED)
find_program(CMAKE_CXX_COMPILER cl HINTS "${XFNV_MSVC_WINE}/bin/x64" NO_DEFAULT_PATH REQUIRED)
find_program(CMAKE_LINKER link HINTS "${XFNV_MSVC_WINE}/bin/x64" NO_DEFAULT_PATH REQUIRED)
find_program(CMAKE_AR lib HINTS "${XFNV_MSVC_WINE}/bin/x64" NO_DEFAULT_PATH REQUIRED)
find_program(CMAKE_RC_COMPILER rc HINTS "${XFNV_MSVC_WINE}/bin/x64" NO_DEFAULT_PATH REQUIRED)
find_program(CMAKE_MT mt HINTS "${XFNV_MSVC_WINE}/bin/x64" NO_DEFAULT_PATH REQUIRED)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
