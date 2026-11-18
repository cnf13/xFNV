#include "support/Messages.h"

namespace test {

    Messages& Reported() {
        static Messages kMessages;
        return kMessages;
    }

} // namespace test
