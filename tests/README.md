# Unit tests

Each test program builds a few of the game's files, unchanged, together with fakes of everything those files call.
It tests that part of the game without the rest of it, and without the game's data or a window. The tests use
[doctest](https://github.com/doctest/doctest) (external/doctest).

## Building and running

The tests are built with the program when `FNVD_TESTS` is on (the default). They are not built under MSVC, because
the fakes are written for the Itanium C++ ABI that clang uses on Linux.

    cmake --build build --target fnvd_test                  # build every test program and run them all
    ctest --test-dir build                                  # run them all, already built
    build/tests/fnvd_test_scriptcompiler                    # one program, with doctest's options:
    build/tests/fnvd_test_scriptcompiler -tc="*GetNextLine*" -s   # matching cases, showing every check

## How a program is put together

A game file calls into the rest of the game. A test program links the file with definitions of what it calls, so
those calls land in code the test controls. These replacements are "link seams". `nm -C -u` on the file's object
lists the symbols they must provide.

- **support/** is a static library of stand-ins for functions most files call: the memory manager, the BS string
  functions, `Warning` and the number checks. Each file there stands in for one game header. Because it is a
  library, a program that also builds the game file defining one of these functions links the real one, and the
  stand-in is left out. `support/Messages.h` collects what `Warning` and the console reported, so tests can check
  error messages.
- **The program's own fakes** cover everything else. scriptcompiler/World.cpp is an example: it defines the members
  the compiler calls on forms and scripts (`TESForm::GetFormType`, `Script::FindVariable`...). They read a
  `FakeForm` in place of the game's class, so no real form is ever built. A fake form's first member points to a
  fake virtual table, which handles the few virtual calls the compiler makes; any other virtual call stops the test
  with a message. `dynamic_cast` is answered from the form's class bits. The type infos the casts refer to are
  defined under their mangled names, because the files that define them are not built.

A call that no fake covers stops the program with a message naming it. Add the fake that the test needs.

## Adding a program

1. Pick the game files to test, and compile them as part of a new `fnvd_add_test(...)` in tests/CMakeLists.txt.
   That registers the program with ctest and adds it to `fnvd_test`.
   Build the real files a fake would only copy: scriptcompiler builds the real compile data, expression parser and
   script variables.
2. Link it. Every undefined symbol needs a fake, either in support/ (for a function many files call) or in the
   program's own fakes.
3. Write `TEST_CASE`s. A fixture that resets the fakes keeps each case independent.

## What to keep in mind

- Static state inside the game's code persists from one case to the next. For example, `CompileLine` remembers the
  open block's length field until the block's `end`. A case that stops in the middle of a block leaves it set.
- The tests describe what the game does now, including its odd parts. A bare `\n` does not end a script line, an
  unnamed script is reported as `'(null)'`, and a console command in a script does not stop the compile. If a
  test fails after a change, either the change altered behaviour or the test pinned down something that was never
  intended.
