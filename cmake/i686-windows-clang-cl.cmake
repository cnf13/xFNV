# The 32-bit Windows program.
set(XFNV_WINDOWS_PROCESSOR x86)
set(XFNV_WINDOWS_TRIPLE i686-pc-windows-msvc)
set(XFNV_WINDOWS_LIB_ARCH x86)
include("${CMAKE_CURRENT_LIST_DIR}/windows-clang-cl.cmake")
