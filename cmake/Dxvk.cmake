# DXVK (external/dxvk): the program's Direct3D 9 on Vulkan, wherever Windows' own is absent.
#
# DXVK builds with Meson. CMake runs it as an external project and hands it this build's own compilers and target in
# a machine file, so a cross build of the program cross-builds DXVK the same way, from any host. Needed on the build
# machine: meson, glslang, and the Vulkan headers on the compiler's include path.
# Defines xfnv_dxvk (the imported library) and xfnv_dxvk_headers (DXVK's Direct3D and Windows headers, for code
# written against them and not against src/SDK).

find_program(XFNV_MESON meson REQUIRED)
find_program(XFNV_GLSLANG NAMES glslang glslangValidator REQUIRED)
set(XFNV_DXVK_JOBS 8 CACHE STRING "Parallel jobs of DXVK's own build")

set(XFNV_DXVK_SOURCE "${CMAKE_SOURCE_DIR}/external/dxvk")
set(XFNV_DXVK_BUILD "${CMAKE_BINARY_DIR}/dxvk")

# The compiler arguments that select the target, as Meson's quoted list items.
set(XFNV_DXVK_TARGET_ARGS "")
if(CMAKE_CXX_COMPILER_TARGET)
    string(APPEND XFNV_DXVK_TARGET_ARGS "'--target=${CMAKE_CXX_COMPILER_TARGET}', ")
endif()
if(CMAKE_SYSROOT)
    string(APPEND XFNV_DXVK_TARGET_ARGS "'--sysroot=${CMAKE_SYSROOT}', ")
endif()
if(CMAKE_CROSSCOMPILING)
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" XFNV_DXVK_SYSTEM)
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" XFNV_DXVK_CPU)
    if(XFNV_DXVK_CPU STREQUAL "amd64")
        set(XFNV_DXVK_CPU x86_64)
    endif()
    set(XFNV_DXVK_MACHINE_OPTION --cross-file)
    set(XFNV_DXVK_HOST_MACHINE "[host_machine]\nsystem = '${XFNV_DXVK_SYSTEM}'\ncpu_family = '${XFNV_DXVK_CPU}'\n\
cpu = '${XFNV_DXVK_CPU}'\nendian = 'little'\n")
else()
    set(XFNV_DXVK_MACHINE_OPTION --native-file)
    set(XFNV_DXVK_HOST_MACHINE "")
endif()
# DXVK compiles against SDL3's headers only (it opens libSDL3.so.0 itself at run time): the program's own, which
# Meson finds through this pkg-config file, or without pkg-config through SDL's CMake package in the build tree.
file(WRITE "${XFNV_DXVK_BUILD}-config/sdl3.pc" "Name: sdl3\nDescription: the program's SDL3 (headers)\nVersion: 3\n\
Cflags: -I${CMAKE_SOURCE_DIR}/external/SDL3/include -I${CMAKE_BINARY_DIR}/sdl3/include-revision\n")
configure_file("${CMAKE_CURRENT_LIST_DIR}/dxvk-machine.ini.in" "${XFNV_DXVK_BUILD}-config/machine.ini" @ONLY)

# Only Direct3D 9, with SDL3 as the one window system. Built once per build folder: after an edit under external/dxvk,
# build the target xfnv_dxvk_build-build (or delete the folder's dxvk/).
include(ExternalProject)
ExternalProject_Add(xfnv_dxvk_build
    SOURCE_DIR "${XFNV_DXVK_SOURCE}"
    BINARY_DIR "${XFNV_DXVK_BUILD}"
    CONFIGURE_COMMAND "${XFNV_MESON}" setup "${XFNV_DXVK_BUILD}" "${XFNV_DXVK_SOURCE}"
        ${XFNV_DXVK_MACHINE_OPTION} "${XFNV_DXVK_BUILD}-config/machine.ini" --buildtype=release
        -Denable_d3d9=true -Denable_d3d8=false -Denable_d3d10=false -Denable_d3d11=false -Denable_dxgi=false
        -Dnative_sdl3=enabled -Dnative_sdl2=disabled -Dnative_glfw=disabled
    BUILD_COMMAND "${XFNV_MESON}" compile -C "${XFNV_DXVK_BUILD}" -j ${XFNV_DXVK_JOBS}
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS "${XFNV_DXVK_BUILD}/src/d3d9/libdxvk_d3d9.so"
    STEP_TARGETS build
    LOG_CONFIGURE ON LOG_BUILD ON LOG_OUTPUT_ON_FAILURE ON)

add_library(xfnv_dxvk SHARED IMPORTED GLOBAL)
set_target_properties(xfnv_dxvk PROPERTIES IMPORTED_LOCATION "${XFNV_DXVK_BUILD}/src/d3d9/libdxvk_d3d9.so"
    IMPORTED_SONAME libdxvk_d3d9.so.0)
add_dependencies(xfnv_dxvk xfnv_dxvk_build)

add_library(xfnv_dxvk_headers INTERFACE)
target_include_directories(xfnv_dxvk_headers INTERFACE "${XFNV_DXVK_SOURCE}/include/native/directx"
    "${XFNV_DXVK_SOURCE}/include/native/windows")
