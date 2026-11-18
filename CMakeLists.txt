cmake_minimum_required(VERSION 3.25)
set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type")
project(FalloutNV LANGUAGES C CXX)

# One program, FalloutNV, built from src/: the game's code in the original's folder layout, with src/platform (the
# layer between the game and the OS, and stand-ins for the DLLs the original game imports) and src/xfnv (the
# project's own utilities: the seam header, the crash log, the debug heap). external/ holds other people's libraries;
# tests/ holds the unit tests, each a program of a few of the game's files and fakes of what they call.
# The toolchain file, when one is given, chooses the platform (cmake/i686-windows-clang-cl.cmake,
# cmake/x86_64-windows-clang-cl.cmake, cmake/x86_64-linux-clang.cmake).

option(FNVD_NO_STEAM "Never start Steam (no SteamAPI_Init: no stats, achievements or callbacks)" ON)
option(FNVD_FIX_FAST_MAIN_MENU "Skip the start-up's welcome screens once the main menu exists" ON)
option(FNVD_CRASH_LOG "Write FalloutNV.crash-<time>.log and .dmp beside the program on an unhandled exception" ON)
# Memory debugging (docs/ENVIRONMENT.md).
option(FNVD_SYSTEM_ALLOC "The game's allocators pass straight through to the C runtime" OFF)
option(FNVD_GUARD_ALLOC "The pass-through allocates from src/xfnv/debugheap's guard-page heap" OFF)
option(FNVD_ASAN "Linux: everything, SDL3 included, built with AddressSanitizer (the C heap only)" OFF)
# Stand-ins the Windows program may use in place of Microsoft's DLLs. Linux has no such DLL and always uses them.
option(FNVD_SDL_AUDIO "DirectSound, mmio and DirectShow from src/platform/dsound, on SDL3's audio" OFF)
# The unit tests (tests/README.md). Their fakes are written for the Itanium C++ ABI: there are none on MSVC.
option(FNVD_TESTS "Build the unit tests in tests/, which ctest runs" ON)
option(FNVD_D3DX "The Direct3D extras from src/platform/d3dx9, not d3dx9_38.dll" OFF)
if(NOT WIN32)
    set(FNVD_SDL_AUDIO ON)
    set(FNVD_D3DX ON)
endif()
if(FNVD_GUARD_ALLOC)
    set(FNVD_SYSTEM_ALLOC ON)
endif()
if(FNVD_ASAN)
    # Recoverable: with ASAN_OPTIONS=halt_on_error=0 a run reports every bad access (each site once) and goes on.
    add_compile_options(-fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer)
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -fsanitize=address")
    # libSDL3.so leaves its ASan symbols undefined (the runtime is the program's), so SDL must not link with
    # -Wl,--no-undefined: this answers its check for the flag.
    set(LINKER_SUPPORTS_WL_NO_UNDEFINED OFF)
endif()

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# Debug info inside each object (ccache can cache it; the program's .pdb still comes from the link), with paths
# relative to the build folder, so that an object is the same in every checkout.
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
add_compile_options(-fdebug-compilation-dir=.)
include(cmake/Ccache.cmake)

# What everything compiled against the game's headers shares.
add_library(fnvd_game_options INTERFACE)
target_include_directories(fnvd_game_options INTERFACE src src/xfnv/seam)
# For their headers; static libraries, so the program links only what it calls.
target_link_libraries(fnvd_game_options INTERFACE fnvd_zlib fnvd_vorbis fnvd_platform)
# The game calls members on null pointers and the members test `this` (BSSimpleList walks start at `this`): clang
# must not delete those tests as undefined behaviour. Every error of a file is reported, not the first 20.
target_compile_options(fnvd_game_options INTERFACE -fno-delete-null-pointer-checks -ferror-limit=0)
# Float code is the same on every target and every machine: `a * b + c` is never contracted into a fused
# multiply-add, which only some CPUs have.
if(MSVC)
    # The game calls the C runtime's plain forms (strcpy, sprintf, fopen). clang-cl takes -ffp-contract as -Xclang:
    # ccache refuses to cache its /clang: form.
    target_compile_definitions(fnvd_game_options INTERFACE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(fnvd_game_options INTERFACE /W4 /EHsc -Wno-invalid-offsetof -Wno-microsoft-cast
        "SHELL:-Xclang -ffp-contract=off")
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        # The 32-bit game heap hands out 4-byte aligned blocks, as the original's does. A class with a 16-byte
        # aligned member has alignof 16, from which clang would write aligned SSE stores through `this` and fault:
        # no pointer is assumed aligned past 4 unless its type says so itself.
        target_compile_options(fnvd_game_options INTERFACE "SHELL:-Xclang -fmax-type-align=4")
    endif()
else()
    target_compile_options(fnvd_game_options INTERFACE -Wall -Wextra -Wno-invalid-offsetof -ffp-contract=off)
endif()

add_subdirectory(external)
add_subdirectory(src/platform)
add_subdirectory(src/xfnv)
if(FNVD_TESTS AND NOT MSVC)
    enable_testing()
    add_subdirectory(tests)
endif()

# The game: every .cpp under src/ outside platform/ and xfnv/, which are the libraries above. Its own objects, not a
# library, so that a file nothing references still reaches the program with its static initialisers.
file(GLOB_RECURSE FNVD_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
list(FILTER FNVD_SOURCES EXCLUDE REGEX "^${CMAKE_SOURCE_DIR}/src/(platform|xfnv)/")
add_executable(FalloutNV WIN32 ${FNVD_SOURCES})
target_link_libraries(FalloutNV PRIVATE fnvd_game_options fnvd_debugheap fnvd_dinput_sdl fnvd_bink SDL3::Headers)
# An option is defined for the files that test it and no others: switching one recompiles only those.
function(fnvd_option OPTION)
    set_property(SOURCE ${ARGN} APPEND PROPERTY COMPILE_DEFINITIONS "${OPTION}=$<BOOL:${${OPTION}}>")
endfunction()
fnvd_option(FNVD_NO_STEAM src/BSSystemUtilities/SteamAPICallbacks.cpp)
fnvd_option(FNVD_FIX_FAST_MAIN_MENU src/Fallout/Interface/Menus/StartMenu.cpp)
fnvd_option(FNVD_SYSTEM_ALLOC src/BSCore/MemoryManager.cpp src/BSCore/ScrapHeap.cpp src/BSHavok/bhkWorld.cpp
    src/Havok/Source/Common/Base/Memory/System/FreeList/hkFreeListMemorySystem.cpp)
# The parts the options and the platform choose.
foreach(PART fnvd_crashlog fnvd_steam fnvd_dsound fnvd_d3dx9)
    if(TARGET ${PART})
        target_link_libraries(FalloutNV PRIVATE ${PART})
    endif()
endforeach()

if(WIN32)
    # The Windows SDK's import libraries for what the game imports (dxguid, strmiids: the GUIDs it names), less
    # dinput8 and xinput, which src/platform/dinput_sdl stands in for. With FNVD_SDL_AUDIO src/platform/dsound's
    # objects define DirectSound's imports first, so dsound and winmm never supply them.
    target_link_libraries(FalloutNV PRIVATE kernel32 user32 gdi32 advapi32 shell32 ole32 d3d9 dsound winmm wsock32
        dxguid strmiids SDL3::SDL3-static)
    # The third-party DLLs still in use, by their import libraries (external/lib/make.sh): d3dx9_38 unless
    # FNVD_D3DX, and the game's 32-bit steam_api (the 64-bit program has src/platform/steam's stub).
    set(FNVD_IMPORT_LIBS "${CMAKE_SOURCE_DIR}/external/lib")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        string(APPEND FNVD_IMPORT_LIBS /x64)
    else()
        target_link_libraries(FalloutNV PRIVATE "${FNVD_IMPORT_LIBS}/steam_api.lib")
    endif()
    if(NOT FNVD_D3DX)
        target_link_libraries(FalloutNV PRIVATE "${FNVD_IMPORT_LIBS}/d3dx9_38.lib")
    endif()
    # A fixed image base, as the original has: a crash's addresses are the same in every run. The guard-page heap
    # reserves a large region, so that build may use the whole 32-bit address space. Every undefined symbol is
    # listed, by its mangled name, for build.py status.
    target_link_options(FalloutNV PRIVATE /dynamicbase:no /errorlimit:0 /demangle:no
        $<$<BOOL:${FNVD_GUARD_ALLOC}>:/largeaddressaware>)
else()
    # SDL3 is shared here: DXVK, the program's Direct3D 9, opens libSDL3.so.0 itself for its window and must find
    # the program's own.
    find_package(Threads REQUIRED)
    target_link_libraries(FalloutNV PRIVATE SDL3::SDL3-shared Threads::Threads fnvd_dxvk)
    # __ImageBase is the Windows linker's name for the image's start (bhkPropertyKey): here the ELF header's.
    # Every undefined symbol is listed, by its mangled name, for build.py status.
    target_link_options(FalloutNV PRIVATE -fuse-ld=lld -Wl,--defsym=__ImageBase=__ehdr_start -Wl,--error-limit=0
        -Wl,--no-demangle)
    # The shared libraries that are not the system's go beside the program under their sonames, and its run path is
    # its own folder: the folder can be moved or shipped as it is.
    set_target_properties(FalloutNV PROPERTIES BUILD_WITH_INSTALL_RPATH ON INSTALL_RPATH "\$ORIGIN")
    add_custom_command(TARGET FalloutNV POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL3-shared>"
            "$<TARGET_FILE_DIR:FalloutNV>/$<TARGET_SONAME_FILE_NAME:SDL3-shared>"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:fnvd_dxvk>"
            "$<TARGET_FILE_DIR:FalloutNV>/$<TARGET_SONAME_FILE_NAME:fnvd_dxvk>"
        VERBATIM)
endif()
