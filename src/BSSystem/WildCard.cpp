#include "BSSystem/WildCard.h"

#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cctype>

bool WildCard::Match(const char* apString, const char* apPattern) {
    int8_t apDest[264];
    int8_t pcDest[268];
    WildCard::MakeString(static_cast<char*>(static_cast<void*>(pcDest)), apString);
    WildCard::MakeString(static_cast<char*>(static_cast<void*>(apDest)), apPattern);
    return WildCard::RecMatch(
               static_cast<const char*>(static_cast<const void*>(pcDest)),
               static_cast<const char*>(static_cast<const void*>(apDest))
           ) == 1;
}

int32_t WildCard::RecMatch(const char* apString, const char* apPattern) {
    char c = *apPattern;
    const char* cppat = apPattern + 1;
    switch (c) {
        case 0: {
            return *apString == 0;
        }
        case 63: {
            if (apString) {
                return WildCard::RecMatch(apString + 1, cppat);
            }

            return 0;
        }
        case 42: {
            if (*cppat) {
                while (*apString) {
                    int32_t i = WildCard::RecMatch(apString, cppat);
                    if (i) {
                        return i;
                    }
                    ++apString;
                }
                return 2;
            }

            return 1;
        }
        case 91: {
            if (*apString) {
                int32_t iFirstChar = *cppat;
                bool binvert = iFirstChar == 94;
                if (iFirstChar == 94) {
                    ++cppat;
                }
                const char* cpend = cppat;
                char cLiteral = 0;
                while (*cpend) {
                    if (cLiteral) {
                        cLiteral = 0;
                    }
                    else {
                        if (*cpend == 92) {
                            cLiteral = 1;
                        }
                        else {
                            if (*cpend == 93) {
                                break;
                            }
                        }
                    }
                    ++cpend;
                }
                if (*cpend == 93) {
                    char cRangeChar = 0;
                    bool bLiteral = *cppat == 45;
                    while (cppat < cpend) {
                        if (bLiteral || *cppat != 92) {
                            if (bLiteral || *cppat != 45) {
                                char cStringChar;
                                if ((WildCard::iFlags & WildCard::FLAG_CASE_SENSITIVE) != 0) {
                                    cStringChar = *apString;
                                }
                                else {
                                    cStringChar = std::toupper(*apString);
                                }
                                if (cppat[1] != 45) {
                                    if (!cRangeChar) {
                                        cRangeChar = *cppat;
                                    }
                                    while (cRangeChar <= *cppat) {
                                        uint8_t ucFoldedChar;
                                        if ((WildCard::iFlags & WildCard::FLAG_CASE_SENSITIVE) != 0) {
                                            ucFoldedChar = cRangeChar;
                                        }
                                        else {
                                            ucFoldedChar = std::toupper(cRangeChar);
                                        }
                                        if (ucFoldedChar == cStringChar) {
                                            if (binvert) {
                                                return 0;
                                            }

                                            return WildCard::RecMatch(apString + 1, cpend + 1);
                                        }
                                        ++cRangeChar;
                                    }
                                }
                                bLiteral = false;
                                cRangeChar = 0;
                            }
                            else {
                                cRangeChar = *(cppat - 1);
                            }
                        }
                        else {
                            bLiteral = true;
                        }
                        ++cppat;
                    }
                    if (binvert) {
                        return WildCard::RecMatch(apString + 1, cpend + 1);
                    }

                    return 0;
                }

                return 0;
            }

            return 0;
        }
        default: {
            if (c == 92) {
                c = *cppat;
                ++cppat;
                if (!c) {
                    return 0;
                }
            }
            char cStringChar;
            char cPatternChar;
            if ((WildCard::iFlags & WildCard::FLAG_CASE_SENSITIVE) != 0) {
                cPatternChar = c;
            }
            else {
                cPatternChar = std::toupper(c);
            }
            if ((WildCard::iFlags & WildCard::FLAG_CASE_SENSITIVE) != 0) {
                cStringChar = *apString;
            }
            else {
                cStringChar = std::toupper(*apString);
            }
            if (cPatternChar == cStringChar) {
                return WildCard::RecMatch(apString + 1, cppat);
            }

            return 0;
        }
    }
}

void WildCard::MakeString(char* apDest, const char* apSrc) {
    if ((WildCard::iFlags & WildCard::FLAG_SKIP_LEADING_SPACES) != 0) {
        while (*apSrc && *apSrc == 32) {
            ++apSrc;
        }
    }
    if ((WildCard::iFlags & (WildCard::FLAG_KEEP_PATH | WildCard::FLAG_KEEP_EXTENSION)) != 0) {
        char dir[260];
        char drive[4];
        char ext[256];
        char fname[260];
        _splitpath_s(apSrc, drive, 3u, dir, 256u, fname, 256u, ext, 256u);
        if ((WildCard::iFlags & WildCard::FLAG_KEEP_PATH) == 0) {
            drive[0] = 0;
            dir[0] = 0;
        }
        if ((WildCard::iFlags & WildCard::FLAG_KEEP_EXTENSION) == 0) {
            ext[0] = 0;
        }
        _makepath_s(apDest, 260u, drive, dir, fname, ext);
    }
    else {
        strcpy_s(apDest, 260, apSrc);
    }
}

bool WildCard::IsWild(const char* apPattern) {
    int8_t cChar;
    while (*apPattern) {
        if ((apPattern[0] == 92) && apPattern[1]) {
            ++apPattern;
        }
        else {
            cChar = *apPattern;
            if (((*apPattern) == 63) || ((cChar == 91) || ((cChar == 93) || ((cChar == 42) || (cChar == 94))))) {
                return true;
            }
        }
        ++apPattern;
    }
    return false;
}
