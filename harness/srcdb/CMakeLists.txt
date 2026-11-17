# srcdb: clang's view of src for the harness's tools, on the system's libclang-cpp (the same clang the hybrid build's
# clang-cl is): srcindex's layout probe and the src model database (Model.hpp).
#   cmake -S harness/srcdb -B build/srcdb -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang \
#       -DCMAKE_CXX_COMPILER=clang++ && cmake --build build/srcdb
cmake_minimum_required(VERSION 3.20)
# C too: LLVMConfig's find modules run C include checks
project(srcdb C CXX)

find_package(Clang REQUIRED CONFIG)
find_package(SQLite3 REQUIRED)

add_executable(srcdb Main.cpp Clang.cpp Fold.cpp Globals.cpp Index.cpp Model.cpp Pdb.cpp Probe.cpp Rename.cpp
	Store.cpp)
set_target_properties(srcdb PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_include_directories(srcdb SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
separate_arguments(llvm_definitions NATIVE_COMMAND "${LLVM_DEFINITIONS}")
target_compile_definitions(srcdb PRIVATE ${llvm_definitions})
if(NOT LLVM_ENABLE_RTTI)
	target_compile_options(srcdb PRIVATE -fno-rtti)
endif()
target_link_libraries(srcdb PRIVATE clang-cpp LLVM SQLite::SQLite3)
