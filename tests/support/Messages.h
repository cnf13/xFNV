#pragma once

#include <string>
#include <vector>

namespace test {

    // What the game reported while a test ran, formatted: the stand-ins of its reporting functions add to it.
    struct Messages {
        // Warning's messages.
        std::vector<std::string> warnings;
        // What was printed to the console (Interface::PrintLine).
        std::vector<std::string> console;

        void Clear() {
            warnings.clear();
            console.clear();
        }
    };

    Messages& Reported();

} // namespace test
