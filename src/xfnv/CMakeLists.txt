# The project's own utilities, which are not the game's code: the crash log and the debug heap.

if (WIN32)
    set(XFNV_PLATFORM Windows)
    set(XFNV_RTTI Msvc)
else ()
    set(XFNV_PLATFORM Posix)
    set(XFNV_RTTI Itanium)
endif ()

# crashlog: shared sources plus one platform side and the compiler ABI's RTTI reader; no platform #if in them.
if (XFNV_CRASH_LOG)
    # Objects, not a library: the crash log installs itself from a static initialiser. The checkout's commit is
    # written into a header at each build (crashlog/version.cmake).
    add_library(xfnv_crashlog OBJECT crashlog/Annotate.cpp crashlog/Identity.cpp crashlog/Out.cpp
            crashlog/Report.cpp crashlog/Capture${XFNV_PLATFORM}.cpp crashlog/Rtti${XFNV_RTTI}.cpp)
    target_link_libraries(xfnv_crashlog PRIVATE xfnv_game_options)
    # crashlog/BuildInfo.h
    target_include_directories(xfnv_crashlog PRIVATE "${CMAKE_BINARY_DIR}")
    add_custom_target(xfnv_crashlog_version BYPRODUCTS "${CMAKE_BINARY_DIR}/crashlog/BuildInfo.h"
            COMMAND "${CMAKE_COMMAND}" "-DSOURCE=${CMAKE_SOURCE_DIR}" "-DOUT=${CMAKE_BINARY_DIR}/crashlog/BuildInfo.h"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/crashlog/version.cmake"
            VERBATIM)
    add_dependencies(xfnv_crashlog xfnv_crashlog_version)
endif ()

# debugheap: the pass-through the game's allocators call with XFNV_SYSTEM_ALLOC, and the guard-page heap it allocates
# from with XFNV_GUARD_ALLOC.
add_library(xfnv_debugheap STATIC debugheap/DebugHeap.cpp debugheap/SystemAlloc.cpp
        debugheap/Platform${XFNV_PLATFORM}.cpp)
target_link_libraries(xfnv_debugheap PRIVATE xfnv_game_options)
target_compile_definitions(xfnv_debugheap PRIVATE XFNV_GUARD_ALLOC=$<BOOL:${XFNV_GUARD_ALLOC}>)
