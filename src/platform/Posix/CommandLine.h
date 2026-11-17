#pragma once

// Inside the POSIX side only: main (Main.cpp) hands its arguments to CommandLine() (System.cpp).
namespace platform {

    void SetCommandLine(int argc, char** argv);

} // namespace platform
