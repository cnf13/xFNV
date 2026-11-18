// Stand-ins for Fallout Shared/GlobalFunc.h's functions. Warning records its message in test::Reported(); the number
// checks are GlobalFunc.cpp's, written out again here.
#include "Fallout Shared/GlobalFunc.h"

#include "support/Messages.h"

#include <cstdarg>
#include <cstdio>

int32_t Warning(const char* apMessage, ...) {
    char acBuffer[4096];
    va_list kArgs;
    va_start(kArgs, apMessage);
    std::vsnprintf(acBuffer, sizeof(acBuffer), apMessage, kArgs);
    va_end(kArgs);
    test::Reported().warnings.emplace_back(acBuffer);
    return 0;
}

namespace {

    bool IsDigit(char acChar) {
        return acChar >= '0' && acChar <= '9';
    }

} // namespace

// An optional '-' and at least one digit, and nothing else.
int32_t IsStringInteger(char* a1) {
    if (!IsDigit(*a1) && *a1 != '-') {
        return 0;
    }
    if (*a1 == '-' && !a1[1]) {
        return 0;
    }
    for (const char* p = a1 + 1; *p; ++p) {
        if (!IsDigit(*p)) {
            return 0;
        }
    }
    return 1;
}

// Digits and at most one '.', the first character also '-'; a lone '-' or '.' is not a number.
int32_t IsStringFloat(char* a1) {
    const char cFirst = *a1;
    if (!cFirst || (!IsDigit(cFirst) && cFirst != '-' && cFirst != '.')) {
        return 0;
    }
    if (!a1[1] && (cFirst == '-' || cFirst == '.')) {
        return 0;
    }
    bool bPoint = cFirst == '.';
    for (const char* p = a1 + 1; *p; ++p) {
        if (!IsDigit(*p) && (*p != '.' || bPoint)) {
            return 0;
        }
        if (*p == '.') {
            bPoint = true;
        }
    }
    return 1;
}
