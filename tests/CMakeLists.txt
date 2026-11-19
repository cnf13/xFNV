# The unit tests (tests/README.md): small programs, each built from a few of the game's files and fakes of what those
# files call, so that a part of the game is tested without the rest of it. ctest runs them all.

# What a game file needs to compile here: the game's headers and its compile options, without the libraries
# xfnv_game_options links (no file a test builds calls them).
add_library(xfnv_test_options INTERFACE)
target_include_directories(xfnv_test_options INTERFACE "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/external/doctest" "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(xfnv_test_options INTERFACE $<TARGET_PROPERTY:xfnv_game_options,INTERFACE_COMPILE_OPTIONS>)

# doctest's main, and stand-ins for the game functions most files call, each file of them for one game header. A
# static library: when a test builds the game file that defines a function, the stand-in's object is left out.
add_library(xfnv_test_support STATIC support/TestMain.cpp support/Messages.cpp support/BSString.cpp
    support/GlobalFunc.cpp support/MemoryManager.cpp)
target_link_libraries(xfnv_test_support PUBLIC xfnv_test_options)

# xfnv_test builds every test program and runs them all.
add_custom_target(xfnv_test COMMAND "${CMAKE_CTEST_COMMAND}" --output-on-failure
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}" USES_TERMINAL VERBATIM)

# A test program: its own sources and the game's files it tests, registered with ctest under its name and run by
# xfnv_test.
function(xfnv_add_test NAME)
    add_executable(${NAME} ${ARGN})
    target_link_libraries(${NAME} PRIVATE xfnv_test_support ${CMAKE_DL_LIBS})
    add_test(NAME ${NAME} COMMAND ${NAME})
    add_dependencies(xfnv_test ${NAME})
endfunction()

set(XFNV_SHARED "${CMAKE_SOURCE_DIR}/src/Fallout Shared")

# ScriptCompiler, with the real compile data, expression parser and script variables. The forms, scripts and function
# tables it works on are scriptcompiler/World's.
xfnv_add_test(xfnv_test_scriptcompiler
    scriptcompiler/World.cpp
    scriptcompiler/ScriptCompilerTest.cpp
    "${XFNV_SHARED}/TESScriptCompiler.cpp"
    "${XFNV_SHARED}/ScriptCompileData.cpp"
    "${XFNV_SHARED}/Expression.cpp"
    "${XFNV_SHARED}/TESScriptVariable.cpp"
    "${CMAKE_SOURCE_DIR}/src/SDK/MSVC/string.cpp"
    "${CMAKE_SOURCE_DIR}/src/SDK/MSVC/crtdefs.cpp")
