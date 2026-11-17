# DXVK (external/dxvk): the program's Direct3D 9 on Vulkan, wherever Windows' own is absent.
#
# DXVK builds with Meson. CMake runs it as an external project and hands it this build's own compilers and target in
# a machine file, so a cross build of the program cross-builds DXVK the same way, from any host. Needed on the build
# machine: meson, glslang, and the Vulkan headers on the compiler's include path.
# Defines fnvd_dxvk (the imported library) and fnvd_dxvk_headers (DXVK's Direct3D and Windows headers, for code
# written against them and not against src/SDK).

find_program(FNVD_MESON meson REQUIRED)
find_program(FNVD_GLSLANG NAMES glslang glslangValidator REQUIRED)
set(FNVD_DXVK_JOBS 8 CACHE STRING "Parallel jobs of DXVK's own build")

set(FNVD_DXVK_SOURCE "${CMAKE_SOURCE_DIR}/external/dxvk")
set(FNVD_DXVK_BUILD "${CMAKE_BINARY_DIR}/dxvk")

# The compiler arguments that select the target, as Meson's quoted list items.
set(FNVD_DXVK_TARGET_ARGS "")
if(CMAKE_CXX_COMPILER_TARGET)
    string(APPEND FNVD_DXVK_TARGET_ARGS "'--target=${CMAKE_CXX_COMPILER_TARGET}', ")
endif()
if(CMAKE_SYSROOT)
    string(APPEND FNVD_DXVK_TARGET_ARGS "'--sysroot=${CMAKE_SYSROOT}', ")
endif()
if(CMAKE_CROSSCOMPILING)
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" FNVD_DXVK_SYSTEM)
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" FNVD_DXVK_CPU)
    if(FNVD_DXVK_CPU STREQUAL "amd64")
        set(FNVD_DXVK_CPU x86_64)
    endif()
    set(FNVD_DXVK_MACHINE_OPTION --cross-file)
    set(FNVD_DXVK_HOST_MACHINE "[host_machine]\nsystem = '${FNVD_DXVK_SYSTEM}'\ncpu_family = '${FNVD_DXVK_CPU}'\n\
cpu = '${FNVD_DXVK_CPU}'\nendian = 'little'\n")
else()
    set(FNVD_DXVK_MACHINE_OPTION --native-file)
    set(FNVD_DXVK_HOST_MACHINE "")
endif()
# DXVK compiles against SDL3's headers only (it opens libSDL3.so.0 itself at run time): the program's own, which
# Meson finds through this pkg-config file, or without pkg-config through SDL's CMake package in the build tree.
file(WRITE "${FNVD_DXVK_BUILD}-config/sdl3.pc" "Name: sdl3\nDescription: the program's SDL3 (headers)\nVersion: 3\n\
Cflags: -I${CMAKE_SOURCE_DIR}/external/SDL3/include -I${CMAKE_BINARY_DIR}/sdl3/include-revision\n")
configure_file("${CMAKE_CURRENT_LIST_DIR}/dxvk-machine.ini.in" "${FNVD_DXVK_BUILD}-config/machine.ini" @ONLY)

# Only Direct3D 9, with SDL3 as the one window system. Built once per build folder: after an edit under external/dxvk,
# build the target fnvd_dxvk_build-build (or delete the folder's dxvk/).
include(ExternalProject)
ExternalProject_Add(fnvd_dxvk_build
    SOURCE_DIR "${FNVD_DXVK_SOURCE}"
    BINARY_DIR "${FNVD_DXVK_BUILD}"
    CONFIGURE_COMMAND "${FNVD_MESON}" setup "${FNVD_DXVK_BUILD}" "${FNVD_DXVK_SOURCE}"
        ${FNVD_DXVK_MACHINE_OPTION} "${FNVD_DXVK_BUILD}-config/machine.ini" --buildtype=release
        -Denable_d3d9=true -Denable_d3d8=false -Denable_d3d10=false -Denable_d3d11=false -Denable_dxgi=false
        -Dnative_sdl3=enabled -Dnative_sdl2=disabled -Dnative_glfw=disabled
    BUILD_COMMAND "${FNVD_MESON}" compile -C "${FNVD_DXVK_BUILD}" -j ${FNVD_DXVK_JOBS}
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS "${FNVD_DXVK_BUILD}/src/d3d9/libdxvk_d3d9.so"
    STEP_TARGETS build
    LOG_CONFIGURE ON LOG_BUILD ON LOG_OUTPUT_ON_FAILURE ON)

add_library(fnvd_dxvk SHARED IMPORTED GLOBAL)
set_target_properties(fnvd_dxvk PROPERTIES IMPORTED_LOCATION "${FNVD_DXVK_BUILD}/src/d3d9/libdxvk_d3d9.so"
    IMPORTED_SONAME libdxvk_d3d9.so.0)
add_dependencies(fnvd_dxvk fnvd_dxvk_build)

add_library(fnvd_dxvk_headers INTERFACE)
target_include_directories(fnvd_dxvk_headers INTERFACE "${FNVD_DXVK_SOURCE}/include/native/directx"
    "${FNVD_DXVK_SOURCE}/include/native/windows")
