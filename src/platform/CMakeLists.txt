# The layer between the game and the platform it runs on: what the game needs from the OS, and stand-ins for the DLLs
# the original game imports. A stand-in is an object library: its definitions must reach the link before an import
# library can supply the same names.

# The OS layer: what the game needs from the OS beyond the standard library (paths, ini files, virtual memory, the
# process), one header per area in this folder and one implementation per platform (README).
if (WIN32)
    file(GLOB FNVD_PLATFORM_SOURCES CONFIGURE_DEPENDS Win32/*.cpp)
else ()
    file(GLOB FNVD_PLATFORM_SOURCES CONFIGURE_DEPENDS Posix/*.cpp)
endif ()
add_library(fnvd_platform STATIC ${FNVD_PLATFORM_SOURCES})
target_include_directories(fnvd_platform PUBLIC "${CMAKE_SOURCE_DIR}/src")
if (WIN32)
    target_link_libraries(fnvd_platform PRIVATE kernel32 user32 advapi32 shell32)
else ()
    target_link_libraries(fnvd_platform PRIVATE SDL3::SDL3 ${CMAKE_DL_LIBS})
endif ()

# dinput_sdl: DirectInput and XInput on SDL3, for dinput8.dll and xinput1_3.dll. src's SDK/Windows headers give it
# the DLLs' calling conventions; the OS layer tells it when the display is lost, which the mouse grab follows. It
# reads SDL's own scancode table (scancodes_windows.h), whose one include dinput_sdl/sdl answers.
add_library(fnvd_dinput_sdl OBJECT dinput_sdl/DInputSDL.cpp dinput_sdl/XInputSDL.cpp)
target_include_directories(fnvd_dinput_sdl PRIVATE dinput_sdl/sdl "${CMAKE_SOURCE_DIR}/external/SDL3/src/events"
        "${CMAKE_SOURCE_DIR}/src/xfnv/seam")
target_link_libraries(fnvd_dinput_sdl PRIVATE SDL3::Headers fnvd_platform)

# bink: a stub for binkw32.dll (no video).
add_library(fnvd_bink OBJECT bink/BinkStub.cpp)
target_link_libraries(fnvd_bink PRIVATE fnvd_game_options)

# steam: a stub for steam_api.dll wherever the game's own 32-bit DLL cannot load. It only stands in for a Steam that
# is never started.
if (NOT (WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4))
    if (NOT FNVD_NO_STEAM)
        message(FATAL_ERROR "this target has no steam_api.dll: FNVD_NO_STEAM must be ON")
    endif ()
    add_library(fnvd_steam OBJECT steam/SteamStub.cpp)
    target_link_libraries(fnvd_steam PRIVATE fnvd_game_options)
endif ()

# dsound: DirectSound, winmm's mmio and DirectShow's music on SDL3's audio (MP3 through minimp3).
if (FNVD_SDL_AUDIO)
    file(GLOB FNVD_DSOUND_SOURCES CONFIGURE_DEPENDS dsound/*.cpp)
    add_library(fnvd_dsound OBJECT ${FNVD_DSOUND_SOURCES})
    # minimp3, as "minimp3/minimp3_ex.h"
    target_include_directories(fnvd_dsound PRIVATE "${CMAKE_SOURCE_DIR}/external")
    target_link_libraries(fnvd_dsound PRIVATE fnvd_game_options SDL3::Headers)
endif ()

# d3dx9: the functions the game imports from d3dx9_38.dll (d3dx9/README.md). Its float code is never contracted
# into fused multiply-adds: that is the code its comparison with the DLL measured.
if (FNVD_D3DX)
    add_library(fnvd_d3dx9 OBJECT d3dx9/D3DX9Math.cpp d3dx9/D3DX9Pixels.cpp d3dx9/D3DX9Shader.cpp
            d3dx9/D3DX9Texture.cpp)
    target_include_directories(fnvd_d3dx9 PRIVATE "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/src/xfnv/seam")
    target_link_libraries(fnvd_d3dx9 PRIVATE fnvd_zlib)
    if (MSVC)
        # NiD3DDefines.h, whose prototypes it implements, declares the functions dllimport on Windows; the
        # program's calls then resolve to these definitions.
        target_compile_options(fnvd_d3dx9 PRIVATE /W4 -Wno-inconsistent-dllimport /clang:-ffp-contract=off)
        target_compile_definitions(fnvd_d3dx9 PRIVATE _CRT_SECURE_NO_WARNINGS)
    else ()
        target_compile_options(fnvd_d3dx9 PRIVATE -Wall -Wextra -ffp-contract=off)
    endif ()
    # The comparison with Microsoft's DLL (32-bit Windows, run under Wine; d3dx9/README.md).
    if (WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
        add_executable(fnvd_d3dx9_compare EXCLUDE_FROM_ALL d3dx9/test/D3DX9Compare.cpp d3dx9/test/TestWindows.cpp)
        target_include_directories(fnvd_d3dx9_compare PRIVATE "${CMAKE_SOURCE_DIR}/src"
                "${CMAKE_SOURCE_DIR}/src/xfnv/seam" d3dx9)
        target_compile_options(fnvd_d3dx9_compare PRIVATE -Wno-inconsistent-dllimport)
        target_compile_definitions(fnvd_d3dx9_compare PRIVATE _CRT_SECURE_NO_WARNINGS)
        target_link_libraries(fnvd_d3dx9_compare PRIVATE fnvd_d3dx9 fnvd_zlib d3d9 user32)
    endif ()
endif ()
