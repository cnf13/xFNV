cmake_minimum_required(VERSION 3.25)
set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type")
project(FalloutNV LANGUAGES C CXX)
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "FalloutNV builds for 64-bit targets only")
endif()

# One program, FalloutNV, built from src/: the game's code in the original's folder layout, with src/platform (the
# layer between the game and the OS, and stand-ins for the DLLs the original game imports) and src/xfnv (the
# project's own utilities: the crash log, the debug heap). external/ holds other people's libraries.
# The toolchain file, when one is given, chooses the platform (cmake/x86_64-windows-clang-cl.cmake,
# cmake/x86_64-windows-msvc-wine.cmake, cmake/x86_64-linux-clang.cmake). Only 64-bit targets are built.

option(XFNV_NO_STEAM "Never start Steam (no SteamAPI_Init: no stats, achievements or callbacks)" ON)
option(XFNV_FIX_FAST_MAIN_MENU "Skip the start-up's welcome screens once the main menu exists" ON)
option(XFNV_CRASH_LOG "Write FalloutNV.crash-<time>.log and .dmp beside the program on an unhandled exception" ON)
# Memory debugging (docs/ENVIRONMENT.md).
option(XFNV_SYSTEM_ALLOC "The game's allocators pass straight through to the C runtime" OFF)
option(XFNV_GUARD_ALLOC "The pass-through allocates from src/xfnv/debugheap's guard-page heap" OFF)
option(XFNV_ASAN "Linux: everything, SDL3 included, built with AddressSanitizer (the C heap only)" OFF)
option(XFNV_UBSAN "Linux: the game's code checks every bool and enum it loads (UndefinedBehaviorSanitizer)" OFF)
# Stand-ins the Windows program may use in place of Microsoft's DLLs. Linux has no such DLL and always uses them.
option(XFNV_SDL_AUDIO "DirectSound, mmio and DirectShow from src/platform/dsound, on SDL3's audio" OFF)
option(XFNV_D3DX "The Direct3D extras from src/platform/d3dx9, not d3dx9_38.dll" OFF)
if(NOT WIN32)
    set(XFNV_SDL_AUDIO ON)
    set(XFNV_D3DX ON)
endif()
if(XFNV_GUARD_ALLOC)
    set(XFNV_SYSTEM_ALLOC ON)
endif()
if(XFNV_ASAN)
    # Recoverable: with ASAN_OPTIONS=halt_on_error=0 a run reports every bad access (each site once) and goes on.
    add_compile_options(-fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer)
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -fsanitize=address")
    # libSDL3.so leaves its ASan symbols undefined (the runtime is the program's), so SDL must not link with
    # -Wl,--no-undefined: this answers its check for the flag.
    set(LINKER_SUPPORTS_WL_NO_UNDEFINED OFF)
endif()

# Microsoft's cl knows no C++26: its newest is /std:c++latest, which CMake gives for 23.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_STANDARD 23)
else()
    set(CMAKE_CXX_STANDARD 26)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# No source is a module: no compile waits on a dependency scan (one more process per file, under Wine cl's).
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# Debug info inside each object (ccache can cache it; the program's .pdb still comes from the link), with clang's
# paths relative to the build folder, so that an object is the same in every checkout.
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fdebug-compilation-dir=.)
endif()
include(cmake/Ccache.cmake)

# What everything compiled against the game's headers shares.
add_library(xfnv_game_options INTERFACE)
target_include_directories(xfnv_game_options INTERFACE src)
# For their headers; static libraries, so the program links only what it calls.
target_link_libraries(xfnv_game_options INTERFACE xfnv_zlib xfnv_vorbis xfnv_platform)
# The game calls members on null pointers and the members test `this` (BSSimpleList walks start at `this`): clang
# must not delete those tests as undefined behaviour. Every error of a file is reported, not the first 20.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(xfnv_game_options INTERFACE -fno-delete-null-pointer-checks -ferror-limit=0)
endif()
# Float code is the same on every target and every machine: `a * b + c` is never contracted into a fused
# multiply-add, which only some CPUs have. cl's /fp:precise never contracts (/fp:contract would).
if(MSVC)
    # The game calls the C runtime's plain forms (strcpy, sprintf, fopen).
    target_compile_definitions(xfnv_game_options INTERFACE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(xfnv_game_options INTERFACE /W4 /EHsc)
endif()
if(MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # clang-cl takes -ffp-contract as -Xclang: ccache refuses to cache its /clang: form.
    target_compile_options(xfnv_game_options INTERFACE -Wno-invalid-offsetof -Wno-microsoft-cast
        "SHELL:-Xclang -ffp-contract=off")
elseif(NOT MSVC)
    target_compile_options(xfnv_game_options INTERFACE -Wall -Wextra -Wno-invalid-offsetof -ffp-contract=off)
endif()
# A local read before it is written is an error: clang treats the read as undefined and may delete the code around it.
# -Wconditional-uninitialized also catches the reads on some paths only; where the code always writes the local first
# but clang cannot see it, the local is initialised where it is declared. cl: C4700, a read of a local never written,
# and C4701/C4703, a read some path leaves unwritten (XFNV_UNWRITTEN_READS_BEGIN marks the functions whose reads match
# the exe's).
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(xfnv_game_options INTERFACE -Wconditional-uninitialized -Werror=uninitialized
        -Werror=sometimes-uninitialized -Werror=conditional-uninitialized)
elseif(MSVC)
    target_compile_options(xfnv_game_options INTERFACE /we4700 /we4701 /we4703)
endif()
# A bool that holds neither 0 nor 1 (a byte count read into a bool, as NiStream::LoadHeader's string lengths were) or
# an enum without a fixed type holding a value outside its enumerators is reported where it is loaded, with its file
# and line, and the run goes on. Only the game's code is checked; the runtime comes with the program's link.
if(XFNV_UBSAN)
    if(MSVC)
        message(FATAL_ERROR "XFNV_UBSAN is for the Linux build")
    endif()
    target_compile_options(xfnv_game_options INTERFACE -fsanitize=bool,enum -fsanitize-recover=bool,enum)
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -fsanitize=bool,enum")
endif()

add_subdirectory(external)
add_subdirectory(src/platform)
add_subdirectory(src/xfnv)

# The game: every .cpp under src/ outside platform/ and xfnv/, which are the libraries above, in a library per
# folder: Gamebryo's NiSystem, NiMain, NiAnimation, NiParticle, NiCollision, NiXenonRenderer and NiDX9Renderer;
# BSCore, BSSystem, BSSystemUtilities, BSMain, BSShader, BSHavok, BSAudio, BSDevices, BSMenu and BSMovie; Havok,
# SpeedTree and FaceGen; and SDK, the rest of src/SDK (the C runtime's pieces, DirectShow's base classes). Fallout and
# Fallout Shared are the program's own sources. Each is an object library, not a static one, so that a file nothing
# references still reaches the program with its static initialisers.
#
# Header code: these files hold the instantiations of header-only templates over the game's types, so they are the
# program's sources.
set(XFNV_PROGRAM_HEADER_CODE
    BSCore/BSList.cpp BSCore/BSScrapArray.cpp BSCore/BSSimpleArray.cpp BSCore/BSSimpleList.cpp BSCore/BSTFreeList.cpp
    BSSystem/BSTMessageQueue.cpp Gamebryo2.2/CoreLibs/NiMain/NiTArray.cpp Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.cpp
    Gamebryo2.2/CoreLibs/NiMain/NiTMap.cpp Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.cpp
    Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.cpp Gamebryo2.2/CoreLibs/NiMain/NiTPool.cpp
    Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.cpp)
list(TRANSFORM XFNV_PROGRAM_HEADER_CODE PREPEND "${CMAKE_SOURCE_DIR}/src/")
set(XFNV_GAME_LIBRARIES)
set(XFNV_CLAIMED ${XFNV_PROGRAM_HEADER_CODE})
# A library of every .cpp under the folders of src/ given, less those an earlier library or the program claimed.
function(xfnv_game_library NAME)
    set(SOURCES)
    foreach(DIR IN LISTS ARGN)
        file(GLOB_RECURSE FOUND CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/${DIR}/*.cpp")
        list(APPEND SOURCES ${FOUND})
    endforeach()
    list(REMOVE_ITEM SOURCES ${XFNV_CLAIMED})
    add_library(${NAME} OBJECT ${SOURCES})
    target_link_libraries(${NAME} PRIVATE xfnv_game_options SDL3::Headers)
    set(XFNV_GAME_LIBRARIES ${XFNV_GAME_LIBRARIES} ${NAME} PARENT_SCOPE)
    set(XFNV_CLAIMED ${XFNV_CLAIMED} ${SOURCES} PARENT_SCOPE)
endfunction()
foreach(LIB NiSystem NiMain NiAnimation NiParticle NiCollision NiXenonRenderer NiDX9Renderer)
    xfnv_game_library(${LIB} Gamebryo2.2/CoreLibs/${LIB})
endforeach()
foreach(LIB BSCore BSSystem BSSystemUtilities BSMain BSShader BSHavok BSAudio BSDevices BSMenu BSMovie Havok SpeedTree)
    xfnv_game_library(${LIB} ${LIB})
endforeach()
xfnv_game_library(FaceGen SDK/FaceGen)
xfnv_game_library(SDK SDK)
file(GLOB_RECURSE XFNV_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/Fallout/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/Fallout Shared/*.cpp")
add_executable(FalloutNV WIN32 ${XFNV_SOURCES} ${XFNV_PROGRAM_HEADER_CODE})
target_link_libraries(FalloutNV PRIVATE ${XFNV_GAME_LIBRARIES} xfnv_game_options xfnv_debugheap xfnv_dinput_sdl
    xfnv_bink SDL3::Headers)
# An option is defined for the files that test it and no others: switching one recompiles only those.
function(xfnv_option OPTION)
    set_property(SOURCE ${ARGN} APPEND PROPERTY COMPILE_DEFINITIONS "${OPTION}=$<BOOL:${${OPTION}}>")
endfunction()
xfnv_option(XFNV_NO_STEAM src/BSSystemUtilities/SteamAPICallbacks.cpp)
xfnv_option(XFNV_FIX_FAST_MAIN_MENU src/Fallout/Interface/Menus/StartMenu.cpp)
xfnv_option(XFNV_SYSTEM_ALLOC src/BSCore/MemoryManager.cpp src/BSCore/ScrapHeap.cpp src/BSHavok/bhkWorld.cpp
    src/Havok/Source/Common/Base/Memory/System/FreeList/hkFreeListMemorySystem.cpp)
# The parts the options and the platform choose.
foreach(PART xfnv_crashlog xfnv_steam xfnv_dsound xfnv_d3dx9)
    if(TARGET ${PART})
        target_link_libraries(FalloutNV PRIVATE ${PART})
    endif()
endforeach()

if(WIN32)
    # The Windows SDK's import libraries for what the game imports (dxguid, strmiids: the GUIDs it names), less
    # dinput8 and xinput, which src/platform/dinput_sdl stands in for. With XFNV_SDL_AUDIO src/platform/dsound's
    # objects define DirectSound's imports first, so dsound and winmm never supply them.
    target_link_libraries(FalloutNV PRIVATE kernel32 user32 gdi32 advapi32 shell32 ole32 d3d9 dsound winmm wsock32
        dxguid strmiids SDL3::SDL3-static)
    # The third-party DLLs still in use, by their import libraries (external/lib/make.sh): d3dx9_38 unless
    # XFNV_D3DX (steam_api is src/platform/steam's stub). By name from their folder: Microsoft's link takes a path
    # beginning with / for an option.
    target_link_directories(FalloutNV PRIVATE external/lib)
    if(NOT XFNV_D3DX)
        target_link_libraries(FalloutNV PRIVATE d3dx9_38)
    endif()
    # A fixed image base, as the original has: a crash's addresses are the same in every run.
    target_link_options(FalloutNV PRIVATE /dynamicbase:no)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # lld-link lists every undefined symbol, by its mangled name, for build.py status.
        target_link_options(FalloutNV PRIVATE /errorlimit:0 /demangle:no)
    endif()
else()
    # SDL3 is shared here: DXVK, the program's Direct3D 9, opens libSDL3.so.0 itself for its window and must find
    # the program's own.
    find_package(Threads REQUIRED)
    target_link_libraries(FalloutNV PRIVATE SDL3::SDL3-shared Threads::Threads xfnv_dxvk)
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
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:xfnv_dxvk>"
            "$<TARGET_FILE_DIR:FalloutNV>/$<TARGET_SONAME_FILE_NAME:xfnv_dxvk>"
        VERBATIM)
endif()
