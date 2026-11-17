# The 64-bit Windows program.
set(FNVD_WINDOWS_PROCESSOR AMD64)
set(FNVD_WINDOWS_TRIPLE x86_64-pc-windows-msvc)
set(FNVD_WINDOWS_LIB_ARCH x86_64)
include("${CMAKE_CURRENT_LIST_DIR}/windows-clang-cl.cmake")
