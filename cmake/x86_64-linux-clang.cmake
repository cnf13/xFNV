# The native Linux x64 program (fnvd_linux, harness/tools/noexe.py --target linux): the project's LLVM clang, so that
# the Linux program and the Windows ones are built by the same compiler (CMakeLists.txt links fnvd_linux with lld).
#   FNVD_LLVM_BIN   folder holding clang / clang++ / ld.lld (default: from PATH)
set(FNVD_LLVM_BIN "" CACHE PATH "LLVM bin folder (empty: PATH)")

find_program(CMAKE_C_COMPILER clang HINTS "${FNVD_LLVM_BIN}" REQUIRED)
find_program(CMAKE_CXX_COMPILER clang++ HINTS "${FNVD_LLVM_BIN}" REQUIRED)
