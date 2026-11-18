# The unit tests (tests/README.md): small programs, each built from a few of the game's files and fakes of what those
# files call, so that a part of the game is tested without the rest of it. ctest runs them all.

# What a game file needs to compile here: the game's headers and its compile options, without the libraries
# fnvd_game_options links (no file a test builds calls them).
add_library(fnvd_test_options INTERFACE)
target_include_directories(fnvd_test_options INTERFACE "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/src/xfnv/seam"
    "${CMAKE_SOURCE_DIR}/external/doctest" "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(fnvd_test_options INTERFACE $<TARGET_PROPERTY:fnvd_game_options,INTERFACE_COMPILE_OPTIONS>)

# doctest's main, and stand-ins for the game functions most files call, each file of them for one game header. A
# static library: when a test builds the game file that defines a function, the stand-in's object is left out.
add_library(fnvd_test_support STATIC support/TestMain.cpp support/Messages.cpp support/BSString.cpp
    support/GlobalFunc.cpp support/MemoryManager.cpp)
target_link_libraries(fnvd_test_support PUBLIC fnvd_test_options)

# fnvd_test builds every test program and runs them all.
add_custom_target(fnvd_test COMMAND "${CMAKE_CTEST_COMMAND}" --output-on-failure
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}" USES_TERMINAL VERBATIM)

# A test program: its own sources and the game's files it tests, registered with ctest under its name and run by
# fnvd_test.
function(fnvd_add_test NAME)
    add_executable(${NAME} ${ARGN})
    target_link_libraries(${NAME} PRIVATE fnvd_test_support ${CMAKE_DL_LIBS})
    add_test(NAME ${NAME} COMMAND ${NAME})
    add_dependencies(fnvd_test ${NAME})
endfunction()

set(FNVD_SHARED "${CMAKE_SOURCE_DIR}/src/Fallout Shared")

# ScriptCompiler, with the real compile data, expression parser and script variables. The forms, scripts and function
# tables it works on are scriptcompiler/World's.
fnvd_add_test(fnvd_test_scriptcompiler
    scriptcompiler/World.cpp
    scriptcompiler/ScriptCompilerTest.cpp
    "${FNVD_SHARED}/TESScriptCompiler.cpp"
    "${FNVD_SHARED}/ScriptCompileData.cpp"
    "${FNVD_SHARED}/Expression.cpp"
    "${FNVD_SHARED}/TESScriptVariable.cpp"
    "${CMAKE_SOURCE_DIR}/src/SDK/MSVC/string.cpp"
    "${CMAKE_SOURCE_DIR}/src/SDK/MSVC/crtdefs.cpp")
