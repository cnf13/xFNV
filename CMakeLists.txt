cmake_minimum_required(VERSION 3.25)
set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type")
project(fnv_decomp LANGUAGES C CXX)

# src/ holds the game's code only, laid out like the original D:\_Fallout3\Platforms\Common\Code\ (folder names as
# the exe's own source paths spell them). Everything that exists only for the reconstruction work lives in
# harness/: the seam (harness/seam/Reconstruction.hpp).
#
# The builds of the reconstruction:
#   no-exe      Windows (cmake/i686-windows-clang-cl.cmake, cmake/x86_64-windows-clang-cl.cmake): the program without
#               the original exe (harness/tools/noexe.py)
#   linux       native Linux x64 (cmake/x86_64-linux-clang.cmake): the program as an ELF executable, fnvd_linux
#               (harness/tools/noexe.py --target linux)
#   standalone  native (64-bit Linux): the reconstruction as a library, so every change keeps compiling where the
#               port has to end up
# FNVD_NO_EXE: the Windows program, defined on every Windows build. FNVD_LINUX: the Linux program, on every Linux build.
if(WIN32)
    set(FNVD_NO_EXE ON)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(FNVD_LINUX ON)
else()
    set(FNVD_LINUX OFF)
endif()
# For now: the no-exe program never starts Steam (no SteamAPI_Init, so no stats, achievements or callbacks), until
# its Steam paths are debugged. OFF gives the game's own behaviour.
option(FNVD_NO_STEAM "the no-exe program does not start Steam" ON)
# Fixes of the game's own behaviour, for the no-exe program only: each is FNVD_FIX_<NAME> on the files that test it,
# 0 or 1.
option(FNVD_FIX_FAST_MAIN_MENU "skip the start-up's welcome screens once the main menu exists" ON)
# Memory debugging for the no-exe program (docs/ENVIRONMENT.md). FNVD_SYSTEM_ALLOC: the game's allocators
# (MemoryManager, ScrapHeap, Havok's routers) pass straight through to the C runtime. FNVD_GUARD_ALLOC (implies it):
# the pass-through takes its memory from harness/debugheap's guard-page heap.
option(FNVD_SYSTEM_ALLOC "the no-exe program's game allocators are the C runtime's" OFF)
option(FNVD_GUARD_ALLOC "the no-exe program's game allocators are harness/debugheap's guard-page heap" OFF)
if(FNVD_GUARD_ALLOC)
    set(FNVD_SYSTEM_ALLOC ON)
endif()
# The no-exe program's crash log (harness/crashlog): FalloutNV.crash-<time>.log and .dmp beside the exe on an unhandled
# exception.
option(FNVD_CRASH_LOG "the no-exe program writes a crash log" ON)
# The game's audio through external/dsound (DirectSound, winmm's mmio and DirectShow on SDL3's audio). Always so on
# Linux, which has no other; OFF on Windows, whose program keeps Microsoft's DLLs until compared by ear.
option(FNVD_SDL_AUDIO "the program's audio is external/dsound's layer on SDL3" OFF)
if(FNVD_LINUX)
    set(FNVD_SDL_AUDIO ON)
endif()
# AddressSanitizer on every target of the Linux program, SDL3 included, in a build folder of its own
# (`cmake -S . -B build/linux-asan -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/x86_64-linux-clang.cmake -DFNVD_ASAN=ON`).
# It sees the C heap only: the game's own heap (MemoryManager) hands out blocks ASan does not know.
option(FNVD_ASAN "the Linux program is built with AddressSanitizer" OFF)
if(FNVD_ASAN)
    if(NOT FNVD_LINUX)
        message(FATAL_ERROR "FNVD_ASAN is for the Linux program")
    endif()
    # Recoverable: with ASAN_OPTIONS=halt_on_error=0 a run reports every bad access (each site once) and goes on.
    add_compile_options(-fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer)
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -fsanitize=address")
    # The runtime is the program's: libSDL3.so leaves its ASan symbols undefined, so SDL's -Wl,--no-undefined goes
    # (its linker check, answered here).
    set(LINKER_SUPPORTS_WL_NO_UNDEFINED OFF)
endif()

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# Debug info inside each object (/Z7: what clang-cl's /Zi writes too, and the form ccache caches for cl as well): the
# linked program still gets its .pdb from lld-link /debug. Debug paths relative to the build folder
# (-fdebug-compilation-dir=.): an object is the same in every checkout, and lld-link makes the PDB's paths absolute
# against the linking folder.
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
add_compile_options(-fdebug-compilation-dir=.)
# ccache for every C and C++ compile (not llvm-rc), one cache for all checkouts: the main checkout's env/cache/ccache,
# which a worktree's env link reaches. base_dir: this checkout's root, so that ccache hashes its absolute paths
# (-I, the source) relative to the build folder and another checkout's build of the same source hits; hash_dir off:
# the build folder's own path is not in an object (above). absolute_paths_in_stderr: a hit replays its diagnostics
# with this checkout's paths. stats_log: this build folder's hits and misses, which noexe.py build prints.
find_program(FNVD_CCACHE ccache)
if(FNVD_CCACHE)
    get_filename_component(FNVD_CCACHE_DIR "${CMAKE_SOURCE_DIR}/env/cache/ccache" REALPATH)
    set(FNVD_CCACHE_LAUNCHER "${FNVD_CCACHE}" "cache_dir=${FNVD_CCACHE_DIR}" "base_dir=${CMAKE_SOURCE_DIR}"
        hash_dir=false max_size=30G absolute_paths_in_stderr=true "stats_log=${CMAKE_BINARY_DIR}/ccache-stats.log")
    set(CMAKE_C_COMPILER_LAUNCHER ${FNVD_CCACHE_LAUNCHER})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${FNVD_CCACHE_LAUNCHER})
    # clang-cl's dependencies as cl's /showIncludes: CMake's default depfile flags are /clang: forms, which ccache
    # refuses to cache.
    if(MSVC)
        foreach(LANG C CXX)
            set(CMAKE_DEPFILE_FLAGS_${LANG} "/showIncludes")
            set(CMAKE_${LANG}_DEPFILE_FORMAT msvc)
        endforeach()
    endif()
endif()

# the reconstruction: every .cpp under src/
file(GLOB_RECURSE FNVD_ENGINE_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
# Havok's own sources: a target of their own, so they build without the game's 1,700 files
file(GLOB_RECURSE FNVD_HAVOK_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/Havok/*.cpp")
list(REMOVE_ITEM FNVD_ENGINE_SOURCES ${FNVD_HAVOK_SOURCES})
# zlib: upstream zlib (external/zlib, C), whose header src/SDK/ZLIB/zlib121/zlib.h forwards to there.
add_library(fnvd_zlib OBJECT external/zlib/adler32.c external/zlib/compress.c external/zlib/crc32.c
    external/zlib/deflate.c external/zlib/inffast.c external/zlib/inflate.c external/zlib/inftrees.c
    external/zlib/trees.c external/zlib/uncompr.c external/zlib/zutil.c)
# As upstream's own CMakeLists builds it with MSVC.
if(MSVC)
    target_compile_definitions(fnvd_zlib PRIVATE _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE)
endif()

# Ogg Vorbis: upstream libogg and libvorbis (external/ogg, external/vorbis, C; external/README), static on every
# target. The game includes the vendor's <vorbis/vorbisfile.h> and calls vorbisfile; libvorbis's decoder and libogg
# are what vorbisfile links against (no encoder). libogg's config_types.h (POSIX only) as upstream's configure
# writes it, from the fixed-width types.
set(INCLUDE_INTTYPES_H 0)
set(INCLUDE_STDINT_H 1)
set(INCLUDE_SYS_TYPES_H 0)
set(SIZE16 int16_t)
set(USIZE16 uint16_t)
set(SIZE32 int32_t)
set(USIZE32 uint32_t)
set(SIZE64 int64_t)
set(USIZE64 uint64_t)
configure_file(external/ogg/include/ogg/config_types.h.in "${CMAKE_BINARY_DIR}/ogg/include/ogg/config_types.h" @ONLY)
add_library(fnvd_vorbis STATIC external/ogg/src/bitwise.c external/ogg/src/framing.c
    external/vorbis/lib/analysis.c external/vorbis/lib/bitrate.c external/vorbis/lib/block.c
    external/vorbis/lib/codebook.c external/vorbis/lib/envelope.c external/vorbis/lib/floor0.c
    external/vorbis/lib/floor1.c external/vorbis/lib/info.c external/vorbis/lib/lookup.c external/vorbis/lib/lpc.c
    external/vorbis/lib/lsp.c external/vorbis/lib/mapping0.c external/vorbis/lib/mdct.c external/vorbis/lib/psy.c
    external/vorbis/lib/registry.c external/vorbis/lib/res0.c external/vorbis/lib/sharedbook.c
    external/vorbis/lib/smallft.c external/vorbis/lib/synthesis.c external/vorbis/lib/window.c
    external/vorbis/lib/vorbisfile.c)
target_include_directories(fnvd_vorbis PUBLIC "${CMAKE_SOURCE_DIR}/external/ogg/include"
    "${CMAKE_SOURCE_DIR}/external/vorbis/include" "${CMAKE_BINARY_DIR}/ogg/include"
    PRIVATE "${CMAKE_SOURCE_DIR}/external/vorbis/lib")
# Its users read through their own callbacks: vorbisfile.h's static stdio ones (OV_CALLBACKS_DEFAULT and friends,
# unused static data in every file that includes it) are left out, as upstream's header offers.
target_compile_definitions(fnvd_vorbis INTERFACE OV_EXCLUDE_STATIC_CALLBACKS)
# As upstream's own build: MSVC's CRT deprecations off; alloca from <alloca.h> elsewhere (its configure's
# HAVE_ALLOCA_H). The math library on POSIX.
if(MSVC)
    target_compile_definitions(fnvd_vorbis PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_SECURE_NO_DEPRECATE
        _CRT_NONSTDC_NO_DEPRECATE)
else()
    target_compile_definitions(fnvd_vorbis PRIVATE HAVE_ALLOCA_H)
    target_link_libraries(fnvd_vorbis PUBLIC m)
endif()

# external/d3dx9: the stand-in for d3dx9_38.dll (the functions the game imports; external/d3dx9/README.md). Off on
# Windows, where the program links Microsoft's DLL until parity. The Linux program, which has no DLL, takes its sources
# by their folder (FNVD_LINUX below). Wherever they are compiled, their float code is never contracted into fused
# multiply-adds: it is the code the comparison with the DLL measured.
option(FNVD_D3DX "the no-exe program's d3dx9 is the stand-in (external/d3dx9), not d3dx9_38.dll" OFF)
set(FNVD_D3DX9_SOURCES external/d3dx9/D3DX9Math.cpp external/d3dx9/D3DX9Pixels.cpp external/d3dx9/D3DX9Shader.cpp
    external/d3dx9/D3DX9Texture.cpp)
if(MSVC)
    set_source_files_properties(${FNVD_D3DX9_SOURCES} PROPERTIES COMPILE_OPTIONS /clang:-ffp-contract=off)
else()
    set_source_files_properties(${FNVD_D3DX9_SOURCES} PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
endif()
if(FNVD_D3DX)
    # Not an fnvd_src_target: its float code is the compiler's default (SSE), the code its comparison with the DLL
    # measured. NiD3DDefines.h, whose types and prototypes it implements, declares the functions dllimport on Windows;
    # the program's calls then resolve to these definitions (lld's LNK4217).
    add_library(fnvd_d3dx9 OBJECT ${FNVD_D3DX9_SOURCES})
    target_include_directories(fnvd_d3dx9 PRIVATE "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/harness/seam"
        "${CMAKE_SOURCE_DIR}/external/zlib")
    target_compile_options(fnvd_d3dx9 PRIVATE /W4 -Wno-inconsistent-dllimport)
    target_compile_definitions(fnvd_d3dx9 PRIVATE _CRT_SECURE_NO_WARNINGS)
    # The comparison with Microsoft's DLL (32-bit Windows, run under Wine; external/d3dx9/README.md).
    if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
        add_executable(fnvd_d3dx9_compare EXCLUDE_FROM_ALL external/d3dx9/test/D3DX9Compare.cpp
            external/d3dx9/test/TestWindows.cpp $<TARGET_OBJECTS:fnvd_d3dx9> $<TARGET_OBJECTS:fnvd_zlib>)
        target_include_directories(fnvd_d3dx9_compare PRIVATE "${CMAKE_SOURCE_DIR}/src"
            "${CMAKE_SOURCE_DIR}/harness/seam" "${CMAKE_SOURCE_DIR}/external/zlib" "${CMAKE_SOURCE_DIR}/external/d3dx9")
        target_compile_options(fnvd_d3dx9_compare PRIVATE -Wno-inconsistent-dllimport)
        target_compile_definitions(fnvd_d3dx9_compare PRIVATE _CRT_SECURE_NO_WARNINGS)
        target_link_libraries(fnvd_d3dx9_compare PRIVATE d3d9 user32)
    endif()
endif()

# The game's window and input are SDL3's (external/SDL3) on every target, DirectInput and XInput external/dinput_sdl's.
# Static on Windows (one exe); shared elsewhere, where dxvk-native loads libSDL3.so.0 itself and must find the
# program's own SDL there. HAVE_DINPUT_H 0: no DirectInput in SDL, whose joystick backend would link dinput8, the DLL
# external/dinput_sdl stands in for. A normal variable, which SDL's preseeded MSVC cache (it sets the cache entry to
# 1) does not override.
if(WIN32)
    set(SDL_SHARED OFF)
    set(SDL_STATIC ON)
else()
    set(SDL_SHARED ON)
    set(SDL_STATIC OFF)
endif()
set(SDL_TESTS OFF)
set(SDL_TEST_LIBRARY OFF)
set(SDL_EXAMPLES OFF)
set(SDL_INSTALL OFF)
set(HAVE_DINPUT_H 0)
add_subdirectory(external/SDL3 "${CMAKE_BINARY_DIR}/sdl3" EXCLUDE_FROM_ALL)

# external/platform: what the game needs from the OS beyond the standard library (paths, ini files, virtual memory,
# the process), one header per area; Win32/ holds today's Win32 calls, Posix/ the POSIX side (SDL where it covers it).
if(WIN32)
    file(GLOB FNVD_PLATFORM_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/external/platform/Win32/*.cpp")
else()
    file(GLOB FNVD_PLATFORM_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/external/platform/Posix/*.cpp")
endif()
add_library(fnvd_platform STATIC ${FNVD_PLATFORM_SOURCES})
target_include_directories(fnvd_platform PUBLIC "${CMAKE_SOURCE_DIR}/external/platform/include"
    PRIVATE "${CMAKE_SOURCE_DIR}/external/platform")
if(WIN32)
    target_link_libraries(fnvd_platform PRIVATE kernel32 user32 advapi32 shell32)
else()
    target_link_libraries(fnvd_platform PRIVATE SDL3::SDL3 ${CMAKE_DL_LIBS})
endif()

# Float code, the same on every target (32- and 64-bit Windows, Linux): SSE2 scalar math (the i686 target's default,
# x86-64's baseline), each operation rounded to its type, no fast-math, the default rounding mode (src never changes
# it). Contraction is off: clang would otherwise fuse `a*b + c` into one fused multiply-add wherever the CPU has one,
# so results would depend on the machine the program runs on. clang-cl takes it as -Xclang (its /clang: form is one
# ccache refuses to cache).
if(MSVC)
    set(FNVD_FLOAT_OPTIONS "SHELL:-Xclang -ffp-contract=off")
else()
    set(FNVD_FLOAT_OPTIONS -ffp-contract=off)
endif()
# The 32-bit game heap hands out 4-byte aligned blocks, as the exe's does, and the exe's code never assumed more of a
# heap object. A class with a 16-byte aligned member has alignof 16 in C++, from which clang would write aligned SSE
# stores (movaps) through `this` and fault: no pointer is assumed aligned past 4 unless its type says so itself.
if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    list(APPEND FNVD_FLOAT_OPTIONS "SHELL:-Xclang -fmax-type-align=4")
endif()

# The include directories and compile options every target of src/ sources shares.
function(fnvd_src_target NAME)
    # gen/ for gen/Prototypes.hpp only: the generated declarations of code not reconstructed yet, whose forwarders
    # take the object as their first parameter
    target_include_directories(${NAME} PUBLIC "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/harness/seam"
        "${CMAKE_SOURCE_DIR}/gen")

    # The engine calls members on null pointers and the members test `this` (BSSimpleList walks start at `this`);
    # MSVC keeps those tests, so the reconstruction must not let clang delete them as undefined behaviour.
    if(MSVC)
        target_compile_options(${NAME} PUBLIC /W4 /EHsc -Wno-invalid-offsetof -Wno-microsoft-cast
            -fno-delete-null-pointer-checks)
    else()
        # No error limit: noexe.py --target linux reports each failing file's first error, and the standalone
        # build's every error is wanted.
        target_compile_options(${NAME} PUBLIC -Wall -Wextra -Wno-invalid-offsetof -fno-delete-null-pointer-checks
            -ferror-limit=0)
    endif()
    target_compile_options(${NAME} PRIVATE ${FNVD_FLOAT_OPTIONS})
    target_include_directories(${NAME} PUBLIC "${CMAKE_SOURCE_DIR}/external/zlib")
    # external/platform's and Ogg Vorbis's headers for the game's sites (the program links the libraries itself)
    target_link_libraries(${NAME} PUBLIC fnvd_platform fnvd_vorbis)
    if(FNVD_NO_EXE OR FNVD_LINUX)
        # On the one file that tests it, so that switching it recompiles one object.
        set_source_files_properties("${CMAKE_SOURCE_DIR}/src/BSSystemUtilities/SteamAPICallbacks.cpp"
            PROPERTIES COMPILE_DEFINITIONS "FNVD_NO_STEAM=$<BOOL:${FNVD_NO_STEAM}>")
        set_source_files_properties("${CMAKE_SOURCE_DIR}/src/Fallout/Interface/Menus/StartMenu.cpp"
            PROPERTIES COMPILE_DEFINITIONS "FNVD_FIX_FAST_MAIN_MENU=$<BOOL:${FNVD_FIX_FAST_MAIN_MENU}>")
        # On the files that test them, so that switching them recompiles those alone.
        foreach(FILE BSCore/MemoryManager.cpp BSCore/ScrapHeap.cpp BSHavok/bhkWorld.cpp
                Havok/Source/Common/Base/Memory/System/FreeList/hkFreeListMemorySystem.cpp)
            set_source_files_properties("${CMAKE_SOURCE_DIR}/src/${FILE}" PROPERTIES
                COMPILE_DEFINITIONS "FNVD_SYSTEM_ALLOC=$<BOOL:${FNVD_SYSTEM_ALLOC}>"
                INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/harness")
        endforeach()
    endif()
    # No error limit: noexe.py counts every error of a file, not the first 20.
    # _CRT_SECURE_NO_WARNINGS: the exe's code calls the CRT's plain forms (strcpy, sprintf, fopen); the no-exe program
    # links the same CRT, so its deprecation warnings say nothing about the reconstruction.
    if(FNVD_NO_EXE)
        target_compile_definitions(${NAME} PUBLIC FNVD_NO_EXE=1 _CRT_SECURE_NO_WARNINGS)
        target_compile_options(${NAME} PRIVATE -ferror-limit=0)
    endif()
endfunction()

# object libraries, so that every file's objects reach the program (a static library would drop the objects nothing
# references)
add_library(fnvd_engine OBJECT ${FNVD_ENGINE_SOURCES})
add_library(fnvd_havok OBJECT ${FNVD_HAVOK_SOURCES})
fnvd_src_target(fnvd_engine)
fnvd_src_target(fnvd_havok)

target_link_libraries(fnvd_engine PRIVATE SDL3::Headers)

# The crash log (harness/crashlog) and the memory debugging (harness/debugheap): shared sources, plus the platform's
# side (the capture, the pages and fault hook) and the compiler ABI's RTTI reader; no platform #if in them.
set(FNVD_CRASH_LOG_SOURCES harness/crashlog/Annotate.cpp harness/crashlog/Identity.cpp harness/crashlog/Out.cpp
    harness/crashlog/Report.cpp)
set(FNVD_DEBUG_HEAP_SOURCES harness/debugheap/DebugHeap.cpp harness/debugheap/SystemAlloc.cpp)
if(WIN32)
    list(APPEND FNVD_CRASH_LOG_SOURCES harness/crashlog/CaptureWindows.cpp harness/crashlog/RttiMsvc.cpp)
    list(APPEND FNVD_DEBUG_HEAP_SOURCES harness/debugheap/PlatformWindows.cpp)
else()
    list(APPEND FNVD_CRASH_LOG_SOURCES harness/crashlog/CapturePosix.cpp harness/crashlog/RttiItanium.cpp)
    list(APPEND FNVD_DEBUG_HEAP_SOURCES harness/debugheap/PlatformPosix.cpp)
endif()

# What both programs (Windows no-exe, Linux) link besides src/, zlib and their own support: the crash log and the
# DirectInput/XInput stand-ins (FNVD_PROGRAM_EXTRA, after the others in the link).
if(FNVD_NO_EXE OR FNVD_LINUX)
    set(FNVD_PROGRAM_EXTRA "")
    if(FNVD_CRASH_LOG)
        # It installs itself from a static initialiser; the commit is baked in at each build (version.cmake).
        set(FNVD_CRASH_LOG_INFO "${CMAKE_BINARY_DIR}/crashlog/BuildInfo.h")
        add_custom_target(fnvd_crashlog_version BYPRODUCTS "${FNVD_CRASH_LOG_INFO}"
            COMMAND "${CMAKE_COMMAND}" "-DSOURCE=${CMAKE_SOURCE_DIR}" "-DOUT=${FNVD_CRASH_LOG_INFO}"
                -P "${CMAKE_SOURCE_DIR}/harness/crashlog/version.cmake"
            VERBATIM)
        add_library(fnvd_crashlog OBJECT ${FNVD_CRASH_LOG_SOURCES})
        fnvd_src_target(fnvd_crashlog)
        target_include_directories(fnvd_crashlog PRIVATE "${CMAKE_SOURCE_DIR}/harness" "${CMAKE_BINARY_DIR}")
        add_dependencies(fnvd_crashlog fnvd_crashlog_version)
        list(APPEND FNVD_PROGRAM_EXTRA $<TARGET_OBJECTS:fnvd_crashlog>)
    endif()
    # external/dinput_sdl stands in for dinput8.dll and xinput1_3.dll on SDL: the program links neither, so a
    # DirectInput or XInput call the stand-in does not define is a link error.
    add_library(fnvd_dinput_sdl OBJECT external/dinput_sdl/DInputSDL.cpp external/dinput_sdl/XInputSDL.cpp)
    # src's SDK/Windows headers give the stand-ins the Windows SDK's names of the DLLs' calling conventions.
    target_include_directories(fnvd_dinput_sdl PRIVATE "${CMAKE_SOURCE_DIR}/external/dinput_sdl/sdl"
        "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/harness/seam")
    # fnvd_platform: DisplayLost, which the mouse's grab follows.
    target_link_libraries(fnvd_dinput_sdl PRIVATE SDL3::Headers fnvd_platform)
    list(APPEND FNVD_PROGRAM_EXTRA $<TARGET_OBJECTS:fnvd_dinput_sdl>)
    set_source_files_properties(harness/debugheap/SystemAlloc.cpp PROPERTIES
        COMPILE_DEFINITIONS "FNVD_GUARD_ALLOC=$<BOOL:${FNVD_GUARD_ALLOC}>")
endif()

if(FNVD_NO_EXE)
    # The program: WinMain is src's (Fallout/Misc/Main.cpp); external/ stands in for binkw32.dll, and on x64 for
    # steam_api.dll (the game's is 32-bit, and this program does not start Steam: FNVD_NO_STEAM); harness/debugheap
    # is the memory debugging's pass-through and guard-page heap.
    set(FNVD_NO_EXE_SUPPORT external/bink/BinkStub.cpp ${FNVD_DEBUG_HEAP_SOURCES})
    set(FNVD_NO_EXE_THIRD_PARTY d3dx9_38 steam_api)
    set(FNVD_NO_EXE_LIB_DIR "${CMAKE_SOURCE_DIR}/external/lib")
    set(FNVD_NO_EXE_MACHINE x86)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        if(NOT FNVD_NO_STEAM)
            message(FATAL_ERROR "the 64-bit program has no steam_api.dll: FNVD_NO_STEAM must be ON")
        endif()
        list(APPEND FNVD_NO_EXE_SUPPORT external/steam/SteamStub.cpp)
        list(REMOVE_ITEM FNVD_NO_EXE_THIRD_PARTY steam_api)
        set(FNVD_NO_EXE_LIB_DIR "${CMAKE_SOURCE_DIR}/external/lib/x64")
        set(FNVD_NO_EXE_MACHINE x64)
    endif()
    add_library(fnvd_noexe_support OBJECT ${FNVD_NO_EXE_SUPPORT})
    fnvd_src_target(fnvd_noexe_support)
    set(FNVD_NO_EXE_OBJECTS $<TARGET_OBJECTS:fnvd_engine> $<TARGET_OBJECTS:fnvd_havok>
        $<TARGET_OBJECTS:fnvd_noexe_support> $<TARGET_OBJECTS:fnvd_zlib> ${FNVD_PROGRAM_EXTRA})
    if(FNVD_D3DX)
        # The stand-in in place of d3dx9_38.dll: a d3dx9 call it does not define is a link error.
        list(APPEND FNVD_NO_EXE_OBJECTS $<TARGET_OBJECTS:fnvd_d3dx9>)
        list(REMOVE_ITEM FNVD_NO_EXE_THIRD_PARTY d3dx9_38)
    endif()
    if(FNVD_SDL_AUDIO)
        # The audio layer in place of dsound.dll's, winmm's mmio and DirectShow's exports: it defines their import
        # slots, so the import libraries never supply them.
        file(GLOB FNVD_SDL_AUDIO_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/external/dsound/*.cpp")
        add_library(fnvd_sdl_audio OBJECT ${FNVD_SDL_AUDIO_SOURCES})
        fnvd_src_target(fnvd_sdl_audio)
        target_include_directories(fnvd_sdl_audio PRIVATE "${CMAKE_SOURCE_DIR}/external")
        target_link_libraries(fnvd_sdl_audio PRIVATE SDL3::Headers)
        list(APPEND FNVD_NO_EXE_OBJECTS $<TARGET_OBJECTS:fnvd_sdl_audio>)
    endif()
    add_executable(fnvd_noexe WIN32 ${FNVD_NO_EXE_OBJECTS})
    set_target_properties(fnvd_noexe PROPERTIES LINKER_LANGUAGE CXX OUTPUT_NAME FalloutNV)
    # The exe's imports from Windows and DirectX, by the Windows SDK's import libraries (dxguid, strmiids: the GUIDs
    # src names), less dinput8 and xinput (external/dinput_sdl); user32 and gdi32 for SDL and external/platform.
    set(FNVD_NO_EXE_LIBS kernel32 user32 gdi32 advapi32 shell32 ole32 d3d9 dsound winmm wsock32 dxguid strmiids)
    # Its third-party DLLs: linked from external/lib (external/lib/x64 for the 64-bit program) when their import
    # library is there; noexe.py reports the missing ones (build/noexe/missing-libraries.txt).
    set(FNVD_NO_EXE_MISSING "")
    foreach(LIB ${FNVD_NO_EXE_THIRD_PARTY})
        find_library(FNVD_LIB_${LIB} ${LIB} PATHS "${FNVD_NO_EXE_LIB_DIR}" NO_DEFAULT_PATH NO_CACHE)
        if(FNVD_LIB_${LIB})
            list(APPEND FNVD_NO_EXE_LIBS "${FNVD_LIB_${LIB}}")
        else()
            string(APPEND FNVD_NO_EXE_MISSING "${LIB}\n")
        endif()
    endforeach()
    file(WRITE "${CMAKE_BINARY_DIR}/missing-libraries.txt" "${FNVD_NO_EXE_MISSING}")
    target_link_libraries(fnvd_noexe PRIVATE ${FNVD_NO_EXE_LIBS})
    target_link_libraries(fnvd_noexe PRIVATE SDL3::SDL3-static)
    # For the response file below: SDL's library and the system libraries it adds (external/SDL3/CMakeLists.txt)
    list(APPEND FNVD_NO_EXE_LIBS "$<TARGET_LINKER_FILE:SDL3-static>" imm32 oleaut32 version uuid setupapi)
    # external/platform and Ogg Vorbis (static libraries: only what the game calls is linked)
    target_link_libraries(fnvd_noexe PRIVATE fnvd_platform fnvd_vorbis)
    list(APPEND FNVD_NO_EXE_LIBS "$<TARGET_LINKER_FILE:fnvd_platform>" "$<TARGET_LINKER_FILE:fnvd_vorbis>")
    # Every undefined symbol listed, by its mangled name (noexe.py demangles and sorts them). A fixed image base, as
    # the original exe has, so a crash's addresses are the same in every run. The guard-page heap reserves a large
    # region, so that build may use the whole 32-bit address space.
    set(FNVD_NO_EXE_LINK_OPTIONS /errorlimit:0 /demangle:no /dynamicbase:no)
    # The toolchain file's folder for library names in a casing the SDK lacks ("Advapi32.lib"). The toolchain puts
    # it in the linker flags too, but those are cached at a build folder's first configure: an existing folder
    # gets it from here.
    list(APPEND FNVD_NO_EXE_LINK_OPTIONS "/libpath:${CMAKE_BINARY_DIR}/libcase")
    if(FNVD_GUARD_ALLOC)
        list(APPEND FNVD_NO_EXE_LINK_OPTIONS /largeaddressaware)
    endif()
    target_link_options(fnvd_noexe PRIVATE ${FNVD_NO_EXE_LINK_OPTIONS})
    # The same link as a response file, for noexe.py to link the objects that compiled while others fail.
    list(TRANSFORM FNVD_NO_EXE_LIBS REPLACE "^([a-z0-9_]+)$" "\\1.lib")
    string(REPLACE " " "\n" FNVD_NO_EXE_LINK_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
    list(JOIN FNVD_NO_EXE_LIBS "\n" FNVD_NO_EXE_LIBS)
    list(JOIN FNVD_NO_EXE_LINK_OPTIONS "\n" FNVD_NO_EXE_LINK_OPTIONS)
    # The objects quoted, one per line ("Fallout Shared" has a space).
    set(FNVD_NO_EXE_RSP_OBJECTS "\"$<JOIN:${FNVD_NO_EXE_OBJECTS},\"\n\">\"")
    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/noexe-link.rsp" CONTENT "/machine:${FNVD_NO_EXE_MACHINE}\n/subsystem:windows\n\
${FNVD_NO_EXE_LINK_OPTIONS}\n${FNVD_NO_EXE_LINK_FLAGS}\n${FNVD_NO_EXE_RSP_OBJECTS}\n${FNVD_NO_EXE_LIBS}\n")
else()
    # external/: what stands in for the DLLs the exe imports and this build has no library for
    add_library(fnvd_standalone STATIC external/bink/BinkStub.cpp
        $<TARGET_OBJECTS:fnvd_engine> $<TARGET_OBJECTS:fnvd_havok> $<TARGET_OBJECTS:fnvd_zlib>)
    target_include_directories(fnvd_standalone PUBLIC "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/harness/seam"
        "${CMAKE_SOURCE_DIR}/gen")
endif()

if(FNVD_LINUX)
    # The Linux program (noexe.py --target linux): the same objects as the Windows program, an ELF executable
    # `FalloutNV`. external/ stands in for binkw32.dll and steam_api.dll; harness/debugheap only with the memory
    # debugging options that need it.
    set(FNVD_LINUX_SUPPORT external/bink/BinkStub.cpp external/steam/SteamStub.cpp)
    if(FNVD_SYSTEM_ALLOC OR FNVD_GUARD_ALLOC)
        list(APPEND FNVD_LINUX_SUPPORT ${FNVD_DEBUG_HEAP_SOURCES})
    endif()
    # The stand-ins the Linux plan's later steps add (env/scratch/Linux/PLAN.md): each is linked once its folder
    # exists. external/d3dx9 for d3dx9_38.dll, external/dsound for DirectSound, DirectShow and winmm's mmio.
    # external/platform is step D's library fnvd_platform, linked below.
    foreach(STANDIN d3dx9 dsound)
        if(EXISTS "${CMAKE_SOURCE_DIR}/external/${STANDIN}")
            file(GLOB FNVD_LINUX_STANDIN CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/external/${STANDIN}/*.cpp"
                "${CMAKE_SOURCE_DIR}/external/${STANDIN}/*.c")
            list(APPEND FNVD_LINUX_SUPPORT ${FNVD_LINUX_STANDIN})
        endif()
    endforeach()
    add_library(fnvd_linux_support OBJECT ${FNVD_LINUX_SUPPORT})
    fnvd_src_target(fnvd_linux_support)
    target_include_directories(fnvd_linux_support PRIVATE "${CMAKE_SOURCE_DIR}/external")
    # external/dsound's output is SDL3's audio.
    target_link_libraries(fnvd_linux_support PRIVATE SDL3::Headers)
    set(FNVD_LINUX_OBJECTS $<TARGET_OBJECTS:fnvd_engine> $<TARGET_OBJECTS:fnvd_havok>
        $<TARGET_OBJECTS:fnvd_linux_support> $<TARGET_OBJECTS:fnvd_zlib> ${FNVD_PROGRAM_EXTRA})
    add_executable(fnvd_linux ${FNVD_LINUX_OBJECTS})
    set_target_properties(fnvd_linux PROPERTIES LINKER_LANGUAGE CXX OUTPUT_NAME FalloutNV)
    # Outside `all`: the standalone build (harness/ci.sh) builds `all`, and the program does not link before the
    # Linux plan's milestone L1.
    foreach(TARGET fnvd_linux fnvd_linux_support fnvd_crashlog fnvd_dinput_sdl)
        if(TARGET ${TARGET})
            set_target_properties(${TARGET} PROPERTIES EXCLUDE_FROM_ALL ON)
        endif()
    endforeach()
    # SDL3 (shared: dxvk-native loads libSDL3.so.0 itself), the threads library and Ogg Vorbis (static).
    find_package(Threads REQUIRED)
    set(FNVD_LINUX_LIBS "$<TARGET_LINKER_FILE:SDL3-shared>" "$<TARGET_LINKER_FILE:fnvd_vorbis>")
    file(WRITE "${CMAKE_BINARY_DIR}/missing-libraries.txt" "")
    target_link_libraries(fnvd_linux PRIVATE SDL3::SDL3-shared Threads::Threads fnvd_vorbis)
    # external/platform (fnvd_platform: a static library, whose POSIX side defines `main`), once it exists
    if(TARGET fnvd_platform)
        target_link_libraries(fnvd_linux PRIVATE fnvd_platform)
        list(APPEND FNVD_LINUX_LIBS "$<TARGET_LINKER_FILE:fnvd_platform>")
        if(CMAKE_DL_LIBS)
            list(APPEND FNVD_LINUX_LIBS "-l${CMAKE_DL_LIBS}")
        endif()
    endif()
    # Direct3D 9: dxvk-native's libdxvk_d3d9.so (harness/tools/dxvk-native.sh unpacks it into env/cache/dxvk-native).
    # It opens the program's own SDL3 (libSDL3.so.0, already loaded: one SDL in the process) for its window when
    # DXVK_WSI_DRIVER=SDL3; the renderer opens it again at run time by its soname (platform::LibraryLoad of d3d9.dll).
    set(FNVD_DXVK_DIR "${CMAKE_SOURCE_DIR}/env/cache/dxvk-native" CACHE PATH
        "dxvk-native's folder: lib/libdxvk_d3d9.so (harness/tools/dxvk-native.sh)")
    set(FNVD_DXVK_D3D9 "${FNVD_DXVK_DIR}/lib/libdxvk_d3d9.so")
    # The shared libraries the program needs that are not the system's go beside it, under their sonames, and the
    # run path is the program's own folder ($ORIGIN): the folder can be moved or shipped as it is.
    add_custom_target(fnvd_linux_libs
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL3-shared>"
            "${CMAKE_BINARY_DIR}/$<TARGET_SONAME_FILE_NAME:SDL3-shared>"
        VERBATIM)
    add_dependencies(fnvd_linux_libs SDL3-shared)
    add_dependencies(fnvd_linux fnvd_linux_libs)
    if(EXISTS "${FNVD_DXVK_D3D9}")
        target_link_libraries(fnvd_linux PRIVATE "${FNVD_DXVK_D3D9}")
        list(APPEND FNVD_LINUX_LIBS "${FNVD_DXVK_D3D9}")
        # copy follows the symlink libdxvk_d3d9.so.0 (the soname) to the file it names
        add_custom_command(TARGET fnvd_linux_libs POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${FNVD_DXVK_DIR}/lib/libdxvk_d3d9.so.0"
                "${CMAKE_BINARY_DIR}/libdxvk_d3d9.so.0"
            VERBATIM)
        # harness/run.sh -L's adapter probe (build/linux/adapter): adapter 0's description as dxvk-native reports it,
        # which the game compares with Display:sD3DDevice. On dxvk's own headers; built with the program.
        add_executable(fnvd_adapter EXCLUDE_FROM_ALL harness/tools/adapter.cpp)
        target_include_directories(fnvd_adapter PRIVATE "${FNVD_DXVK_DIR}/include")
        target_link_libraries(fnvd_adapter PRIVATE "${FNVD_DXVK_D3D9}" SDL3::SDL3-shared)
        set_target_properties(fnvd_adapter PROPERTIES OUTPUT_NAME adapter
            BUILD_WITH_INSTALL_RPATH ON INSTALL_RPATH "\$ORIGIN")
        add_dependencies(fnvd_adapter fnvd_linux_libs)
        add_dependencies(fnvd_linux fnvd_adapter)
    else()
        file(APPEND "${CMAKE_BINARY_DIR}/missing-libraries.txt" "dxvk-native\n")
    endif()
    # The install run path at build time too: CMake's build run path would add the libraries' build-time folders.
    set_target_properties(fnvd_linux PROPERTIES BUILD_WITH_INSTALL_RPATH ON INSTALL_RPATH "\$ORIGIN")
    list(APPEND FNVD_LINUX_LIBS "-Wl,-rpath,\$ORIGIN")
    # lld, listing every undefined symbol by its mangled name (noexe.py demangles and groups them). __ImageBase is the
    # Windows linker's name for the image's start (bhkPropertyKey, src/BSHavok/bhkWorldObject.h): here the ELF
    # header's, which lld defines as __ehdr_start.
    set(FNVD_LINUX_LINK_OPTIONS -fuse-ld=lld -Wl,--error-limit=0 -Wl,--no-demangle
        -Wl,--defsym=__ImageBase=__ehdr_start)
    target_link_options(fnvd_linux PRIVATE ${FNVD_LINUX_LINK_OPTIONS})
    # The same link as a response file for the compiler driver, for noexe.py to link the objects that compiled while
    # others fail.
    string(REPLACE " " "\n" FNVD_LINUX_LINK_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
    list(APPEND FNVD_LINUX_LIBS -pthread)
    list(JOIN FNVD_LINUX_LIBS "\n" FNVD_LINUX_LIBS)
    list(JOIN FNVD_LINUX_LINK_OPTIONS "\n" FNVD_LINUX_LINK_OPTIONS)
    # The objects quoted, one per line ("Fallout Shared" has a space).
    set(FNVD_LINUX_RSP_OBJECTS "\"$<JOIN:${FNVD_LINUX_OBJECTS},\"\n\">\"")
    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/linux-link.rsp" CONTENT "${FNVD_LINUX_LINK_OPTIONS}\n\
${FNVD_LINUX_LINK_FLAGS}\n${FNVD_LINUX_RSP_OBJECTS}\n${FNVD_LINUX_LIBS}\n")
endif()
