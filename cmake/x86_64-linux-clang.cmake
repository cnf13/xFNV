# The Linux x64 program: the project's LLVM clang, so that it and the Windows programs are built by the same
# compiler.
#   FNVD_LLVM_BIN   folder holding clang / clang++ / ld.lld (default: from PATH)
set(FNVD_LLVM_BIN "" CACHE PATH "LLVM bin folder (empty: PATH)")

find_program(CMAKE_C_COMPILER clang HINTS "${FNVD_LLVM_BIN}" REQUIRED)
find_program(CMAKE_CXX_COMPILER clang++ HINTS "${FNVD_LLVM_BIN}" REQUIRED)
