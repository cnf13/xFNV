# Other people's code the program links (external/README says where each came from): zlib, Ogg Vorbis, SDL3, DXVK.
# minimp3 is header-only; lib/ holds import libraries for the Microsoft and Steam DLLs the Windows program still uses.

# zlib (C): the game includes <zlib.h>.
add_library(xfnv_zlib STATIC zlib/adler32.c zlib/compress.c zlib/crc32.c zlib/deflate.c zlib/inffast.c
    zlib/inflate.c zlib/inftrees.c zlib/trees.c zlib/uncompr.c zlib/zutil.c)
target_include_directories(xfnv_zlib PUBLIC zlib)

# Ogg Vorbis (C): the game calls vorbisfile; libvorbis's decoder and libogg are what that needs.
# libogg's config_types.h as its configure script writes it, from the fixed-width types.
set(INCLUDE_INTTYPES_H 0)
set(INCLUDE_STDINT_H 1)
set(INCLUDE_SYS_TYPES_H 0)
set(SIZE16 int16_t)
set(USIZE16 uint16_t)
set(SIZE32 int32_t)
set(USIZE32 uint32_t)
set(SIZE64 int64_t)
set(USIZE64 uint64_t)
configure_file(ogg/include/ogg/config_types.h.in "${CMAKE_BINARY_DIR}/ogg/include/ogg/config_types.h" @ONLY)
add_library(xfnv_vorbis STATIC ogg/src/bitwise.c ogg/src/framing.c vorbis/lib/analysis.c vorbis/lib/bitrate.c
    vorbis/lib/block.c vorbis/lib/codebook.c vorbis/lib/envelope.c vorbis/lib/floor0.c vorbis/lib/floor1.c
    vorbis/lib/info.c vorbis/lib/lookup.c vorbis/lib/lpc.c vorbis/lib/lsp.c vorbis/lib/mapping0.c vorbis/lib/mdct.c
    vorbis/lib/psy.c vorbis/lib/registry.c vorbis/lib/res0.c vorbis/lib/sharedbook.c vorbis/lib/smallft.c
    vorbis/lib/synthesis.c vorbis/lib/window.c vorbis/lib/vorbisfile.c)
target_include_directories(xfnv_vorbis PUBLIC ogg/include vorbis/include "${CMAKE_BINARY_DIR}/ogg/include"
    PRIVATE vorbis/lib)
# The game reads through its own callbacks: vorbisfile.h's static stdio ones are left out, as the header offers.
target_compile_definitions(xfnv_vorbis INTERFACE OV_EXCLUDE_STATIC_CALLBACKS)
if(NOT MSVC)
    target_compile_definitions(xfnv_vorbis PRIVATE HAVE_ALLOCA_H)
    target_link_libraries(xfnv_vorbis PUBLIC m)
endif()

# The C runtime's deprecation warnings say nothing about upstream's code.
if(MSVC)
    target_compile_definitions(xfnv_zlib PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
    target_compile_definitions(xfnv_vorbis PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
endif()

# SDL3: the game's window and input on every target. Static on Windows (one exe), shared elsewhere (the root
# CMakeLists.txt says why). No DirectInput in SDL: its joystick backend would link dinput8, the DLL
# src/platform/dinput_sdl stands in for; a normal variable, which SDL's preseeded MSVC cache does not override.
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
add_subdirectory(SDL3 "${CMAKE_BINARY_DIR}/sdl3" EXCLUDE_FROM_ALL)

# dxvk: the Linux program's Direct3D 9, on Vulkan.
if(NOT WIN32)
    include("${CMAKE_SOURCE_DIR}/cmake/Dxvk.cmake")
endif()
