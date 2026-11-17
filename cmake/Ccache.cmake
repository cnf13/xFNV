# ccache for every C and C++ compile, one cache for all checkouts: the main checkout's env/cache/ccache, which a
# worktree's env link reaches. base_dir is the checkout's root, so that its absolute paths are hashed relative to the
# build folder and another checkout's build of the same source hits; hash_dir is off because the build folder's own
# path is not in an object. stats_log holds this build folder's hits and misses, which build.py build prints.
find_program(FNVD_CCACHE ccache)
if(NOT FNVD_CCACHE)
    return()
endif()
get_filename_component(FNVD_CCACHE_DIR "${CMAKE_SOURCE_DIR}/env/cache/ccache" REALPATH)
set(CMAKE_C_COMPILER_LAUNCHER "${FNVD_CCACHE}" "cache_dir=${FNVD_CCACHE_DIR}" "base_dir=${CMAKE_SOURCE_DIR}"
    hash_dir=false max_size=30G absolute_paths_in_stderr=true "stats_log=${CMAKE_BINARY_DIR}/ccache-stats.log")
set(CMAKE_CXX_COMPILER_LAUNCHER ${CMAKE_C_COMPILER_LAUNCHER})
# clang-cl's dependencies as cl's /showIncludes: CMake's default depfile flags are /clang: forms, which ccache refuses
# to cache.
if(MSVC)
    foreach(LANG C CXX)
        set(CMAKE_DEPFILE_FLAGS_${LANG} "/showIncludes")
        set(CMAKE_${LANG}_DEPFILE_FORMAT msvc)
    endforeach()
endif()
