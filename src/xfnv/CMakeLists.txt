# The project's own utilities, which are not the game's code: the seam header src is still written through (seam/, an
# include folder of its own), the crash log and the debug heap.

if (WIN32)
    set(FNVD_PLATFORM Windows)
    set(FNVD_RTTI Msvc)
else ()
    set(FNVD_PLATFORM Posix)
    set(FNVD_RTTI Itanium)
endif ()

# crashlog: shared sources plus one platform side and the compiler ABI's RTTI reader; no platform #if in them.
if (FNVD_CRASH_LOG)
    # Objects, not a library: the crash log installs itself from a static initialiser. The checkout's commit is
    # written into a header at each build (crashlog/version.cmake).
    add_library(fnvd_crashlog OBJECT crashlog/Annotate.cpp crashlog/Identity.cpp crashlog/Out.cpp
            crashlog/Report.cpp crashlog/Capture${FNVD_PLATFORM}.cpp crashlog/Rtti${FNVD_RTTI}.cpp)
    target_link_libraries(fnvd_crashlog PRIVATE fnvd_game_options)
    # crashlog/BuildInfo.h
    target_include_directories(fnvd_crashlog PRIVATE "${CMAKE_BINARY_DIR}")
    add_custom_target(fnvd_crashlog_version BYPRODUCTS "${CMAKE_BINARY_DIR}/crashlog/BuildInfo.h"
            COMMAND "${CMAKE_COMMAND}" "-DSOURCE=${CMAKE_SOURCE_DIR}" "-DOUT=${CMAKE_BINARY_DIR}/crashlog/BuildInfo.h"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/crashlog/version.cmake"
            VERBATIM)
    add_dependencies(fnvd_crashlog fnvd_crashlog_version)
endif ()

# debugheap: the pass-through the game's allocators call with FNVD_SYSTEM_ALLOC, and the guard-page heap it allocates
# from with FNVD_GUARD_ALLOC.
add_library(fnvd_debugheap STATIC debugheap/DebugHeap.cpp debugheap/SystemAlloc.cpp
        debugheap/Platform${FNVD_PLATFORM}.cpp)
target_link_libraries(fnvd_debugheap PRIVATE fnvd_game_options)
target_compile_definitions(fnvd_debugheap PRIVATE FNVD_GUARD_ALLOC=$<BOOL:${FNVD_GUARD_ALLOC}>)
