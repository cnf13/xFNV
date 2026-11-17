// The program's entry point on POSIX: the game's own entry is WinMain (src/Fallout/Misc/Main.cpp).
#include "platform/Posix/CommandLine.h"

#include <cstdint>
#include <cstdlib>
#include <string>

struct HINSTANCE__;
int32_t WinMain(HINSTANCE__* hInstance, HINSTANCE__* hPrevInstance, char* lpCmdLine, int32_t nShowCmd);

int main(int argc, char** argv) {
    platform::SetCommandLine(argc, argv);
    // Direct3D is dxvk-native, which takes its window from the library named here and stops the program when the
    // variable is unset: the program's window is SDL3's. A value the user set stays.
    setenv("DXVK_WSI_DRIVER", "SDL3", 0);
    // WinMain's lpCmdLine: the arguments after the program's name, quoted where they hold a space or are empty.
    std::string line;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (!line.empty()) {
            line += ' ';
        }
        if (argument.empty() || argument.find_first_of(" \t") != std::string::npos) {
            line += '"' + argument + '"';
        }
        else {
            line += argument;
        }
    }
    // SW_SHOWNORMAL, as a program started from a shell gets it.
    return WinMain(nullptr, nullptr, line.data(), 1);
}
