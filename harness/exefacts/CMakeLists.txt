# exefacts: the exe facts idakit's sync and emit read, without IDA. Built by harness/tools/exefacts.py on first use:
#   cmake -S harness/exefacts -B build/exefacts -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
#   cmake --build build/exefacts
# It links the venv's capstone (the capstone wheel's include/ and lib/libcapstone.so), the one the Python tools use.
cmake_minimum_required(VERSION 3.20)
project(exefacts LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

get_filename_component(EXEFACTS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
if(NOT CAPSTONE_DIR)
    file(GLOB CAPSTONE_DIR "${EXEFACTS_ROOT}/env/venv/lib/python3*/site-packages/capstone")
endif()
if(NOT EXISTS "${CAPSTONE_DIR}/lib/libcapstone.so")
    message(FATAL_ERROR "exefacts: no capstone wheel at '${CAPSTONE_DIR}' (set CAPSTONE_DIR)")
endif()

# The wheel's library is named libcapstone.so but its soname is libcapstone.so.5, which the loader looks for: a copy
# under that name in the build tree, on the rpath of everything that links exefacts-core (idakit too), so the
# system's capstone is never loaded in its place.
set(CAPSTONE_LIB_DIR "${CMAKE_CURRENT_BINARY_DIR}/capstone")
configure_file("${CAPSTONE_DIR}/lib/libcapstone.so" "${CAPSTONE_LIB_DIR}/libcapstone.so.5" COPYONLY)

add_library(exefacts-core OBJECT
    src/Image.cpp src/Decoder.cpp src/Program.cpp src/Function.cpp src/StackWalk.cpp src/Facts.cpp src/EhFrame.cpp)
target_include_directories(exefacts-core PUBLIC src "${CAPSTONE_DIR}/include")
target_compile_options(exefacts-core PUBLIC -Wall -Wextra)
target_link_libraries(exefacts-core PUBLIC "${CAPSTONE_LIB_DIR}/libcapstone.so.5")
target_link_options(exefacts-core INTERFACE "-Wl,-rpath,${CAPSTONE_LIB_DIR}")

# the C API, for the ctypes binding
add_library(exefacts SHARED src/capi.cpp)
target_include_directories(exefacts PUBLIC include)
target_link_libraries(exefacts PRIVATE exefacts-core)

# the dump CLI
add_executable(exefacts-cli tools/main.cpp)
target_compile_definitions(exefacts-cli PRIVATE EXEFACTS_ROOT="${EXEFACTS_ROOT}")
target_link_libraries(exefacts-cli PRIVATE exefacts-core)
set_target_properties(exefacts-cli PROPERTIES OUTPUT_NAME exefacts)
