#include "BSSystem/WildCard.h"

#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cctype>

bool WildCard::Match(const char* apString, const char* apPattern) {
    int8_t apDest[264];
    int8_t pcDest[268];
    WildCard::MakeString(static_cast<char*>(static_cast<void*>(pcDest)), apString);
    WildCard::MakeString(static_cast<char*>(static_cast<void*>(apDest)), apPattern);
    return (bool)((uint8_t)(WildCard::RecMatch(
                                static_cast<const char*>(static_cast<const void*>(pcDest)),
                                static_cast<const char*>(static_cast<const void*>(apDest))
                            ) == 1));
}

int32_t WildCard::RecMatch(const char* apString, const char* apPattern) {
    char c = *apPattern;
    const char* apPatterna = apPattern + 1;
    switch (c) {
        case 0: {
            return *apString == 0;
        }
        case 63: {
            if (apString) {
                return WildCard::RecMatch(apString + 1, apPatterna);
            }

            return 0;
        }
        case 42: {
            if (*apPatterna) {
                while (*apString) {
                    int32_t i = WildCard::RecMatch(apString, apPatterna);
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
                int32_t v3 = *apPatterna;
                bool v12 = v3 == 94;
                if (v3 == 94) {
                    ++apPatterna;
                }
                const char* cpend = apPatterna;
                char v14 = 0;
                while (*cpend) {
                    if (v14) {
                        v14 = 0;
                    }
                    else {
                        if (*cpend == 92) {
                            v14 = 1;
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
                    char v18 = 0;
                    bool v15 = *apPatterna == 45;
                    while (apPatterna < cpend) {
                        if (v15 || *apPatterna != 92) {
                            if (v15 || *apPatterna != 45) {
                                char v10;
                                if ((WildCard::iFlags & 1) != 0) {
                                    v10 = *apString;
                                }
                                else {
                                    v10 = std::toupper(*apString);
                                }
                                if (apPatterna[1] != 45) {
                                    if (!v18) {
                                        v18 = *apPatterna;
                                    }
                                    while (v18 <= *apPatterna) {
                                        uint8_t v9;
                                        if ((WildCard::iFlags & 1) != 0) {
                                            v9 = v18;
                                        }
                                        else {
                                            v9 = std::toupper(v18);
                                        }
                                        if (v9 == v10) {
                                            if (v12) {
                                                return 0;
                                            }

                                            return WildCard::RecMatch(apString + 1, cpend + 1);
                                        }
                                        ++v18;
                                    }
                                }
                                v15 = false;
                                v18 = 0;
                            }
                            else {
                                v18 = *(apPatterna - 1);
                            }
                        }
                        else {
                            v15 = true;
                        }
                        ++apPatterna;
                    }
                    if (v12) {
                        return WildCard::RecMatch(apString + 1, cpend + 1);
                    }

                    return 0;
                }

                return 0;
            }

            return 0;
        }
        default: {
            if (c != 92 || (c = *apPatterna, (++apPatterna, c))) {
                char v5;
                char v6;
                if ((WildCard::iFlags & 1) != 0) {
                    v6 = c;
                }
                else {
                    v6 = std::toupper(c);
                }
                if ((WildCard::iFlags & 1) != 0) {
                    v5 = *apString;
                }
                else {
                    v5 = std::toupper(*apString);
                }
                if (v6 == v5) {
                    return WildCard::RecMatch(apString + 1, apPatterna);
                }

                return 0;
            }

            return 0;
        }
    }
}

void WildCard::MakeString(char* apDest, const char* apSrc) {
    if ((WildCard::iFlags & 8) != 0) {
        while (*apSrc && *apSrc == 32) {
            ++apSrc;
        }
    }
    if ((WildCard::iFlags & 6) != 0) {
        char dir[260];
        char drive[4];
        char ext[256];
        char fname[260];
        _splitpath_s(apSrc, drive, 3u, dir, 256u, fname, 256u, ext, 256u);
        if ((WildCard::iFlags & 2) == 0) {
            drive[0] = 0;
            dir[0] = 0;
        }
        if ((WildCard::iFlags & 4) == 0) {
            ext[0] = 0;
        }
        _makepath_s(apDest, 260u, drive, dir, fname, ext);
    }
    else {
        strcpy_s(apDest, 260, apSrc);
    }
}

bool WildCard::IsWild(const char* apPattern) {
    int8_t v3;
    while (*apPattern) {
        if (((*apPattern) == 92) && apPattern[1]) {
            ++apPattern;
        }
        else {
            v3 = *apPattern;
            if (((*apPattern) == 63) || ((v3 == 91) || ((v3 == 93) || ((v3 == 42) || (v3 == 94))))) {
                return true;
            }
        }
        ++apPattern;
    }
    return false;
}
