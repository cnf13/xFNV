#include "Havok/Source/Animation/Animation/Compression/Common/hkaProcessFlyingColors.h"

#include "Havok/Source/Common/Base/KeyCode.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Cpu/hkpCpuAutoRayBundleJob.h"

// Parses the key code "0x<expiry>-0x<hash>:<date>.<product>.<licensee>": each hex value starts at -1 and takes one
// digit at a time. An expiry whose top bit is clear checks the hash of the text after the second dot against the hash
// value; otherwise the masked expiry, in units of 256 seconds, is compared with the time: at most 617625 units (about
// five years) ahead, and the result 3 within 3375 units (ten days).
int32_t hkaProcessFlyingColors(uint8_t* flyingColors) {
    uint32_t uiExpiry;
    uint32_t uiHash;
    uint32_t uiNameHash;
    const char* pText;
    const char* pDot;
    char cChar;
    int32_t iDigit;
    int32_t iExpiry;
    int32_t iNow;
    int32_t iLeft;

    *flyingColors = 0;
    uiHash = 0xFFFFFFFFu;
    uiExpiry = 0xFFFFFFFFu;

    pText = HK_ANIMATION_KEYCODE + 2;
    cChar = *pText++;
    if (static_cast<uint8_t>(cChar - '0') <= 9) {
        iDigit = static_cast<int8_t>(cChar) - '0';
    }
    else if (static_cast<uint8_t>(cChar - 'A') <= 5) {
        iDigit = static_cast<int8_t>(cChar) - ('A' - 10);
    }
    else if (static_cast<uint8_t>(cChar - 'a') <= 5) {
        iDigit = static_cast<int8_t>(cChar) - ('a' - 10);
    }
    else {
        iDigit = -1;
    }
    while (iDigit >= 0) {
        uiExpiry = (uiExpiry << 4) + static_cast<uint32_t>(iDigit);
        cChar = *pText++;
        if (static_cast<uint8_t>(cChar - '0') <= 9) {
            iDigit = static_cast<int8_t>(cChar) - '0';
        }
        else if (static_cast<uint8_t>(cChar - 'A') <= 5) {
            iDigit = static_cast<int8_t>(cChar) - ('A' - 10);
        }
        else if (static_cast<uint8_t>(cChar - 'a') <= 5) {
            iDigit = static_cast<int8_t>(cChar) - ('a' - 10);
        }
        else {
            iDigit = -1;
        }
    }

    // pText is past the '-': skip "0x".
    pText += 2;
    cChar = *pText++;
    if (static_cast<uint8_t>(cChar - '0') <= 9) {
        iDigit = static_cast<int8_t>(cChar) - '0';
    }
    else if (static_cast<uint8_t>(cChar - 'A') <= 5) {
        iDigit = static_cast<int8_t>(cChar) - ('A' - 10);
    }
    else if (static_cast<uint8_t>(cChar - 'a') <= 5) {
        iDigit = static_cast<int8_t>(cChar) - ('a' - 10);
    }
    else {
        iDigit = -1;
    }
    while (iDigit >= 0) {
        uiHash = (uiHash << 4) + static_cast<uint32_t>(iDigit);
        cChar = *pText++;
        if (static_cast<uint8_t>(cChar - '0') <= 9) {
            iDigit = static_cast<int8_t>(cChar) - '0';
        }
        else if (static_cast<uint8_t>(cChar - 'A') <= 5) {
            iDigit = static_cast<int8_t>(cChar) - ('A' - 10);
        }
        else if (static_cast<uint8_t>(cChar - 'a') <= 5) {
            iDigit = static_cast<int8_t>(cChar) - ('a' - 10);
        }
        else {
            iDigit = -1;
        }
    }

    pDot = HK_ANIMATION_KEYCODE;
    while (*pDot != '\0' && *pDot != '.') {
        if (++pDot == nullptr) {
            goto invalid;
        }
    }
    if (pDot == nullptr) {
        goto invalid;
    }
    if (++pDot == nullptr) {
        goto invalid;
    }
    while (*pDot != '\0' && *pDot != '.') {
        if (++pDot == nullptr) {
            goto invalid;
        }
    }
    if (pDot == nullptr) {
        goto invalid;
    }
    pText = pDot + 1;

    if (static_cast<int32_t>(uiExpiry) >= 0) {
        // The hash starts at the character after the one pText points to.
        uiNameHash = 0;
        if (*pText != '\0') {
            do {
                cChar = *++pText;
                uiNameHash = uiNameHash * 23 + static_cast<uint32_t>(static_cast<int32_t>(static_cast<int8_t>(cChar)));
            }
            while (cChar != '\0');
        }
        if (uiHash == ((uiNameHash ^ 0xD4EFE34Au) & 0x7FFFFFFFu)) {
            *flyingColors = 1;
            return 0;
        }
        goto invalid;
    }

    iExpiry = static_cast<int32_t>((uiExpiry ^ 0xD4EFE34Au) & 0x7FFFFFFFu);
    iNow = getCurrentTime() >> 8;
    iLeft = static_cast<int32_t>(static_cast<uint32_t>(iExpiry) - static_cast<uint32_t>(iNow));
    if (iExpiry < iNow || iLeft > 617625) {
        _hkErrorMessage(
            "Product:hka Key Code has expired .\nPlease contact Havok.com for an extension.\nNo simulation "
            "possible."
        );
        return 2;
    }
    if (iLeft < 3375) {
        *flyingColors = 1;
        return 3;
    }
    *flyingColors = 1;
    return 0;

invalid:
    _hkErrorMessage("Product:hka Key Code is invalid.\nPlease contact Havok.com\nNo simulation possible.");
    return 1;
}
