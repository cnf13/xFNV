#include "Havok/Source/Common/Base/Memory/Util/hkMemUtil.h"

#include "SDK/MSVC/string.h"

// memCpyBackwards' 8-byte step: copies nbytes / 8 pairs of words from the end down, the low word of each pair first.
void memCopyBackwards8(const void* src, void* dst, uint32_t nbytes) {
    uint32_t uiCount = nbytes >> 3;
    if (nbytes >= 8) {
        uint32_t* pDst = static_cast<uint32_t*>(dst) + 2 * (uiCount - 1);
        const uint32_t* pSrc = static_cast<const uint32_t*>(src) + 2 * (uiCount - 1);
        do {
            pDst[0] = pSrc[0];
            pDst[1] = pSrc[1];
            pDst -= 2;
            pSrc -= 2;
        }
        while (--uiCount);
    }
}

// memCpyBackwards' 4-byte step: copies nbytes / 4 words from the end down.
void memCopyBackwards4(void* dst, const void* src, uint32_t nbytes) {
    uint32_t uiCount = nbytes >> 2;
    if (nbytes >= 4) {
        uint32_t* pDst = static_cast<uint32_t*>(dst) + (uiCount - 1);
        const uint32_t* pSrc = static_cast<const uint32_t*>(src) + (uiCount - 1);
        do {
            *pDst-- = *pSrc--;
        }
        while (--uiCount);
    }
}

// memCpyBackwards' 2-byte step: copies nbytes / 2 halfwords from the end down.
void memCopyBackwards2(void* dst, const void* src, uint32_t nbytes) {
    uint32_t uiCount = nbytes >> 1;
    if (nbytes >= 2) {
        uint16_t* pDst = static_cast<uint16_t*>(dst) + (uiCount - 1);
        const uint16_t* pSrc = static_cast<const uint16_t*>(src) + (uiCount - 1);
        do {
            *pDst-- = *pSrc--;
        }
        while (--uiCount);
    }
}

// memCpyBackwards' 1-byte step: copies nbytes bytes from the end down.
void memCopyBackwards1(const void* src, uint32_t nbytes, void* dst) {
    if (nbytes >= 1) {
        char* pDst = static_cast<char*>(dst) + (nbytes - 1);
        const char* pSrc = static_cast<const char*>(src) + (nbytes - 1);
        do {
            *pDst-- = *pSrc--;
        }
        while (--nbytes);
    }
}

void hkMemUtil::memCpy(void* dst, const void* src, int32_t nbytes) {
    rt::memcpy(dst, src, nbytes);
}

void hkMemUtil::memMove(void* dst, const void* src, int32_t nbytes) {
    if (dst <= src) {
        if (dst < src) {
            hkMemUtil::memCpy(dst, src, nbytes);
        }
    }
    else {
        hkMemUtil::memCpyBackwards(dst, src, nbytes);
    }
}

// Copies from the end down, in the widest of 8, 4, 2 and 1 byte steps that the size and both addresses allow: each step
// copies its whole multiple and leaves nothing for the narrower ones. The 8-byte step moves two 4-byte words, low word
// first. Sizes are unsigned. The buffers are untyped memory, read and written as raw words through
// reinterpret_cast.
void hkMemUtil::memCpyBackwards(void* dst, const void* src, int32_t nbytes) {
    char* d = static_cast<char*>(dst);
    const char* s = static_cast<const char*>(src);
    uint32_t n = static_cast<uint32_t>(nbytes);
    uint32_t whole = n & ~7u;
    if (whole != 0 &&
        (n & 7) == 0 &&
        (reinterpret_cast<uintptr_t>(d) & 7) == 0 &&
        (reinterpret_cast<uintptr_t>(s) & 7) == 0) {
        uint32_t count = n >> 3;
        if (n >= 8) {
            uint32_t* pDst = reinterpret_cast<uint32_t*>(d) + 2 * (count - 1);
            const uint32_t* pSrc = reinterpret_cast<const uint32_t*>(s) + 2 * (count - 1);
            do {
                pDst[0] = pSrc[0];
                pDst[1] = pSrc[1];
                pDst -= 2;
                pSrc -= 2;
            }
            while (--count);
        }
        d += whole;
        s += whole;
        n = 0;
    }
    whole = n & ~3u;
    if (whole != 0 &&
        (n & 3) == 0 &&
        (reinterpret_cast<uintptr_t>(d) & 3) == 0 &&
        (reinterpret_cast<uintptr_t>(s) & 3) == 0) {
        uint32_t count = n >> 2;
        if (n >= 4) {
            uint32_t* pDst = reinterpret_cast<uint32_t*>(d) + (count - 1);
            const uint32_t* pSrc = reinterpret_cast<const uint32_t*>(s) + (count - 1);
            do {
                *pDst-- = *pSrc--;
            }
            while (--count);
        }
        d += whole;
        s += whole;
        n = 0;
    }
    whole = n & ~1u;
    if (whole != 0 &&
        (n & 1) == 0 &&
        (reinterpret_cast<uintptr_t>(d) & 1) == 0 &&
        (reinterpret_cast<uintptr_t>(s) & 1) == 0) {
        uint32_t count = n >> 1;
        if (n >= 2) {
            uint16_t* pDst = reinterpret_cast<uint16_t*>(d) + (count - 1);
            const uint16_t* pSrc = reinterpret_cast<const uint16_t*>(s) + (count - 1);
            do {
                *pDst-- = *pSrc--;
            }
            while (--count);
        }
        d += whole;
        s += whole;
        n = 0;
    }
    if (n >= 1) {
        char* pDst = d + (n - 1);
        const char* pSrc = s + (n - 1);
        do {
            *pDst-- = *pSrc--;
        }
        while (--n);
    }
}

void hkMemUtil::memSet(void* dst, const int32_t c, int32_t n) {
    std::memset(dst, c, n);
}
