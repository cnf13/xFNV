#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/hkMemUtil.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <cstdarg>

hkStringBuf::hkStringBuf() : m_string(0) {
    m_string.setSize(1);
    m_string.m_data[0] = 0;
}

// The text starts in the 128 inline characters: the terminating zero, then s0 to s5.
hkStringBuf::hkStringBuf(const char* s0, const char* s1, const char* s2, const char* s3, const char* s4, const char* s5)
  : m_string(0) {
    m_string.setSize(1);
    m_string.m_data[0] = 0;
    appendJoin(s0, s1, s2, s3, s4, s5);
}

// A copy of s, or the empty string for a null s.
hkStringBuf::hkStringBuf(const char* s) : m_string(0) {
    if (s) {
        const int32_t iLen = hkString::strLen(s);
        m_string.setSize(iLen + 1);
        m_string.m_data[iLen] = 0;
        hkString::memCpy(m_string.m_data, s, iLen);
    }
    else {
        m_string.setSize(1);
        m_string.m_data[0] = 0;
    }
}

// s's characters, its terminating zero included.
hkStringBuf::hkStringBuf(const hkStringBuf& s) : m_string(0) {
    m_string.setSize(s.m_string.getSize());
    hkString::memCpy(m_string.m_data, s.m_string.m_data, s.m_string.getSize());
}

// aiSize characters from apSrc, then the terminating zero.
hkStringBuf::hkStringBuf(const char* apSrc, int32_t aiSize) : m_string(0) {
    m_string.setSize(aiSize + 1);
    m_string.m_data[aiSize] = 0;
    hkString::memCpy(m_string.m_data, apSrc, aiSize);
}

// m_string frees a storage it has outgrown the inline characters for.
hkStringBuf::~hkStringBuf() {}

// Compares s with the last characters before the terminating zero; the sizes compare signed.
int32_t hkStringBuf::endsWith(const char* s) const {
    int32_t ilen = hkString::strLen(s);
    int32_t isize = m_string.m_size;
    if (ilen > isize - 1) {
        return 0;
    }
    const char* pend = m_string.m_data + (isize - ilen - 1);
    for (int32_t i = 0; i < ilen; ++i) {
        if (pend[i] != s[i]) {
            return 0;
        }
    }
    return 1;
}

hkBool hkStringBuf::operator<(const char* other) const {
    hkBool result;
    char v3 = hkString::strCmp(m_string.begin(), other) < 0;
    result.m_bool = v3;
    return result;
}

// The insertion point is the terminating zero, read before the allocator is fetched.
hkStringBuf& hkStringBuf::operator+=(const char* other) {
    int32_t len = hkString::strLen(other);
    int32_t index = m_string.m_size - 1;
    m_string._insertAt(*hkMemoryRouter::getInstance().m_temp, index, other, len);
    return *this;
}

void hkStringBuf::clear() {
    hkMemoryAllocator* m_temp;
    int8_t* m_data;
    m_temp = (&hkMemoryRouter::getInstance())->m_temp;
    if ((m_string.m_capacityAndFlags & 1073741823) == 0) {
        hkArrayUtil::_reserve(m_temp, &m_string, 1, sizeof(char));
    }
    m_data = static_cast<int8_t*>(static_cast<void*>(m_string.m_data));
    m_string.m_size = (uint32_t)1;
    *m_data = 0;
}

hkStringBuf& hkStringBuf::operator=(const char* s) {
    if (s) {
        int32_t v3 = hkString::strLen(s);
        int32_t v4 = v3 + 1;
        hkMemoryAllocator* m_temp = (&hkMemoryRouter::getInstance())->m_temp;
        int32_t v6 = m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK;
        if (v6 < v3 + 1) {
            int32_t v7 = 2 * v6;
            if (v4 >= v7) {
                v7 = v3 + 1;
            }
            hkArrayUtil::_reserve(m_temp, &m_string, v7, sizeof(char));
        }
        char* m_data = m_string.begin();
        m_string.m_size = v4;
        m_data[v3] = 0;
        hkString::memCpy(m_string.begin(), s, v3);
        return *this;
    }

    hkMemoryAllocator* v10 = (&hkMemoryRouter::getInstance())->m_temp;
    if ((m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK) == 0) {
        hkArrayUtil::_reserve(v10, &m_string, 1, sizeof(char));
    }
    char* v11 = m_string.begin();
    m_string.m_size = 1;
    *v11 = 0;
    return *this;
}

hkStringBuf& hkStringBuf::operator=(const hkStringBuf& s) {
    int32_t m_size;
    hkMemoryAllocator* m_temp = (&hkMemoryRouter::getInstance())->m_temp;
    const hkStringBuf* v4 = &s;
    if ((m_string.m_capacityAndFlags & 0x3FFFFFFF) < s.m_string.m_size) {
        if (m_string.m_capacityAndFlags >= 0) {
            m_temp->bufFree(m_string.begin(), m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
        }
        m_size = v4->m_string.m_size;
        m_string.m_data = static_cast<char*>(m_temp->bufAlloc(m_size));
        m_string.m_capacityAndFlags = m_size;
    }
    int32_t v6 = v4->m_string.m_size;
    char* m_data = m_string.begin();
    m_string.m_size = v6;
    if (v6 > 0) {
        int32_t v8 = v4->m_string.m_data - m_data;
        int32_t v9 = v6;
        do {
            *m_data = m_data[v8];
            ++m_data;
            --v9;
        }
        while (v9);
    }
    return *this;
}

void hkStringBuf::replace(char a, char b, ReplaceType replaceType) {
    for (int32_t i = 0; i < static_cast<int32_t>(m_string.m_size - 1); ++i) {
        char* v5 = &m_string.m_data[i];
        if (*v5 == a) {
            *v5 = b;
            if (replaceType == REPLACE_ONE) {
                break;
            }
        }
    }
}

// Measures each string up to the first null one, grows the storage to the joined length plus the terminating zero
// (capacity doubled, or the exact need when that is larger), writes the zero, then copies each string after the old
// text.
void hkStringBuf::appendJoin(
    const char* s0,
    const char* s1,
    const char* s2,
    const char* s3,
    const char* s4,
    const char* s5
) {
    const char* strings[7] = {s0, s1, s2, s3, s4, s5, nullptr};
    int32_t sizes[6] = {0, 0, 0, 0, 0, 0};
    const int32_t iOldLength = m_string.m_size - 1;
    int32_t iNewLength = iOldLength;
    for (int32_t i = 0; strings[i] != nullptr; ++i) {
        sizes[i] = hkString::strLen(strings[i]);
        iNewLength += sizes[i];
    }
    const int32_t iNumBytes = iNewLength + 1;
    hkMemoryAllocator* pTempAllocator = hkMemoryRouter::getInstance().m_temp;
    const int32_t iHave = static_cast<int32_t>(m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
    if (iHave < iNumBytes) {
        int32_t iRequest = 2 * iHave;
        if (iNumBytes >= iRequest) {
            iRequest = iNumBytes;
        }
        hkArrayUtil::_reserve(pTempAllocator, &m_string, iRequest, sizeof(char));
    }
    m_string.m_size = static_cast<uint32_t>(iNumBytes);
    m_string.m_data[iNewLength] = 0;
    int32_t iOffset = iOldLength;
    for (int32_t i = 0; strings[i] != nullptr; ++i) {
        hkString::memCpy(m_string.m_data + iOffset, strings[i], sizes[i]);
        iOffset += sizes[i];
    }
}

// The length is measured before the temp allocator is fetched.
void hkStringBuf::insert(int32_t pos, const char* s, int32_t len) {
    if (len < 0) {
        len = hkString::strLen(s);
    }
    m_string._insertAt(*hkMemoryRouter::getInstance().m_temp, pos, s, len);
}

// Formats into the buffer and retries with more storage until the text fits. A negative length means the text does
// not fit at all and the capacity doubles, or goes to 255 when the double is smaller; a length that fills the buffer
// means the text needs length+1 characters. Each pass writes the terminating zero at the end of the text and counts
// it in the size. The temp allocator is fetched on every pass, before the capacity is read back.
void hkStringBuf::printf(const char* fmt, ...) {
    va_list arguments;
    va_start(arguments, fmt);
    for (;;) {
        const int32_t iCapacity = static_cast<int32_t>(m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
        const int32_t iLength = hkString::vsnprintf(m_string.m_data, iCapacity, fmt, arguments);
        if (iLength < 0) {
            int32_t iNewCapacity = 2 * iCapacity;
            if (iNewCapacity <= 255) {
                iNewCapacity = 255;
            }
            const int32_t iNumBytes = iNewCapacity + 1;
            hkMemoryAllocator* pTempAllocator = hkMemoryRouter::getInstance().m_temp;
            const int32_t iHave = static_cast<int32_t>(m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
            if (iHave < iNumBytes) {
                int32_t iRequest = 2 * iHave;
                if (iNumBytes >= iRequest) {
                    iRequest = iNumBytes;
                }
                hkArrayUtil::_reserve(pTempAllocator, &m_string, iRequest, sizeof(char));
            }
            m_string.m_size = static_cast<uint32_t>(iNumBytes);
            m_string.m_data[iNewCapacity] = 0;
            continue;
        }
        const int32_t iNumBytes = iLength + 1;
        hkMemoryAllocator* pTempAllocator = hkMemoryRouter::getInstance().m_temp;
        const int32_t iHave = static_cast<int32_t>(m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
        if (iHave < iNumBytes) {
            int32_t iRequest = 2 * iHave;
            if (iNumBytes >= iRequest) {
                iRequest = iNumBytes;
            }
            hkArrayUtil::_reserve(pTempAllocator, &m_string, iRequest, sizeof(char));
        }
        m_string.m_size = static_cast<uint32_t>(iNumBytes);
        m_string.m_data[iLength] = 0;
        if (iLength < iCapacity) {
            break;
        }
    }
}

// Formats into a temporary buffer and appends the text to this one, retrying with more storage until the text fits;
// the temporary buffer's storage goes back to the temp allocator unless it never grew out of its inline characters.
// The size is cleared before the storage is freed.
void hkStringBuf::appendPrintf(const char* fmt, ...) {
    va_list arguments;
    va_start(arguments, fmt);
    FNVD_INLINED_LOCAL(hkStringBuf, kBuffer, std::nullopt, std::nullopt);
    ::new (&kBuffer) hkStringBuf;
    for (;;) {
        const int32_t iCapacity =
            static_cast<int32_t>(kBuffer.m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
        const int32_t iLength = hkString::vsnprintf(kBuffer.m_string.m_data, iCapacity, fmt, arguments);
        if (iLength < 0) {
            int32_t iNewCapacity = 2 * iCapacity;
            if (iNewCapacity <= 255) {
                iNewCapacity = 255;
            }
            const int32_t iNumBytes = iNewCapacity + 1;
            hkMemoryAllocator* pTempAllocator = hkMemoryRouter::getInstance().m_temp;
            const int32_t iHave =
                static_cast<int32_t>(kBuffer.m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
            if (iHave < iNumBytes) {
                int32_t iRequest = 2 * iHave;
                if (iNumBytes >= iRequest) {
                    iRequest = iNumBytes;
                }
                hkArrayUtil::_reserve(pTempAllocator, &kBuffer.m_string, iRequest, sizeof(char));
            }
            kBuffer.m_string.m_size = static_cast<uint32_t>(iNumBytes);
            kBuffer.m_string.m_data[iNewCapacity] = 0;
            continue;
        }
        const int32_t iNumBytes = iLength + 1;
        hkMemoryAllocator* pTempAllocator = hkMemoryRouter::getInstance().m_temp;
        const int32_t iHave =
            static_cast<int32_t>(kBuffer.m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
        if (iHave < iNumBytes) {
            int32_t iRequest = 2 * iHave;
            if (iNumBytes >= iRequest) {
                iRequest = iNumBytes;
            }
            hkArrayUtil::_reserve(pTempAllocator, &kBuffer.m_string, iRequest, sizeof(char));
        }
        kBuffer.m_string.m_size = static_cast<uint32_t>(iNumBytes);
        kBuffer.m_string.m_data[iLength] = 0;
        if (iLength < iCapacity) {
            break;
        }
    }
    append(kBuffer.m_string.m_data, -1);
    hkMemoryAllocator* pTempAllocator = hkMemoryRouter::getInstance().m_temp;
    kBuffer.m_string.m_size = 0;
    if (kBuffer.m_string.m_capacityAndFlags >= 0) {
        pTempAllocator->bufFree(
            kBuffer.m_string.m_data,
            static_cast<int32_t>(kBuffer.m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK)
        );
    }
}

void hkStringBuf::append(const char* s, int32_t len) {
    if (len < 0) {
        len = hkString::strLen(s);
    }
    int32_t v4 = m_string.m_size - 1;
    int32_t v5 = m_string.m_size + len;
    hkMemoryAllocator* m_temp = (&hkMemoryRouter::getInstance())->m_temp;
    int32_t v7 = m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK;
    if (v7 < v5) {
        int32_t v8 = 2 * v7;
        if (v5 >= v8) {
            v8 = v5;
        }
        hkArrayUtil::_reserve(m_temp, &m_string, v8, sizeof(char));
    }
    char* m_data = m_string.begin();
    m_string.m_size = v5;
    m_data[v4 + len] = 0;
    hkMemUtil::memCpy(&m_string.m_data[v4], s, len);
}

// The inline characters become the storage, flagged not to be freed, then the text is assigned. Clears the ownership
// flag off arString's pointer, and returns the buffer itself.
hkStringBuf* hkStringBuf::constructFrom(const hkStringPtr& arString) {
    m_string.m_data = m_string.m_storage;
    m_string.m_size = 0;
    m_string.m_capacityAndFlags = hkArrayBase<char>::DONT_DEALLOCATE_FLAG | 128;
    operator=(arString.cString());
    return this;
}

int32_t hkStringBuf::compareTo(const char* apcRight) const {
    return hkString::strCmp(m_string.m_data, apcRight);
}

int32_t hkStringBuf::lastIndexOf(char a2, int32_t a3, int32_t a4) {
    int8_t* m_data;
    int32_t v4 = a4;
    if (a4 > ((int32_t)(m_string.m_size - 1))) {
        v4 = (int32_t)(m_string.m_size - 1);
    }
    int32_t result = v4 - 1;
    if ((v4 - 1) < a3) {
        return (-1);
    }
    m_data = static_cast<int8_t*>(static_cast<void*>(m_string.m_data));
    while (m_data[result] != a2) {
        if ((--result) < a3) {
            return (-1);
        }
    }
    return result;
}

void hkStringBuf::setJoin(
    const char* a1,
    const char* a2,
    const char* a3,
    const char* a4,
    const char* a5,
    const char* a6
) {
    clear();
    appendJoin(a1, a2, a3, a4, a5, a6);
}

void hkStringBuf::pathBasename() {
    int32_t v2 = lastIndexOf(47, 0, 2147483647);
    int32_t v3 = lastIndexOf(92, 0, 2147483647);
    if (v3 <= v2) {
        v3 = v2;
    }
    if (v3 >= 0) {
        chompStart(v3 + 1);
    }
}

void hkStringBuf::lowerCase() {
    int32_t result = (int32_t)(m_string.m_size - 1);
    int32_t v2 = 0;
    if (result > 0) {
        do {
            result = (int32_t)m_string.m_data[v2];
            if (((uint32_t)((int32_t)((uint8_t)(m_string.m_data[v2] - 65)))) <= 0x19u) {
                result += 32;
            }
            m_string.m_data[v2++] = (int8_t)result;
        }
        while (v2 < ((int32_t)(m_string.m_size - 1)));
    }
}

void hkStringBuf::upperCase() {
    int32_t result = (int32_t)(m_string.m_size - 1);
    int32_t v2 = 0;
    if (result > 0) {
        do {
            result = (int32_t)m_string.m_data[v2];
            if (((uint32_t)((int32_t)((uint8_t)(m_string.m_data[v2] - 97)))) <= 0x19u) {
                result -= 32;
            }
            m_string.m_data[v2++] = (int8_t)result;
        }
        while (v2 < ((int32_t)(m_string.m_size - 1)));
    }
}

int32_t hkStringBuf::compareToIgnoreCase(const char* a1) const {
    return hkString::strCasecmp(m_string.m_data, a1);
}

int32_t hkStringBuf::startsWithCase(const char* a1) const {
    int8_t* i;
    int8_t v4;
    int8_t v5;
    if (*m_string.m_data) {
        for (i = static_cast<int8_t*>(static_cast<void*>(m_string.m_data)); *i; ++i) {
            v4 = *a1;
            if (!(*a1)) {
                break;
            }
            v5 = *i;
            if (((uint32_t)((int32_t)((uint8_t)((*i) - 65)))) <= 0x19u) {
                v5 += 32;
            }
            if (((uint32_t)((int32_t)((uint8_t)(v4 - 65)))) <= 0x19u) {
                v4 += 32;
            }
            if (v5 != v4) {
                return 0;
            }
            ++a1;
        }
    }
    return 1;
}

uint32_t hkStringBuf::startsWith(const char* s) const {
    const char* m_data = m_string.begin();
    int32_t v3 = 0;
    if (*m_string.begin()) {
        do {
            char v4 = s[v3];
            if (!v4) {
                break;
            }
            if (m_data[v3] != v4) {
                return 0;
            }
            ++v3;
        }
        while (m_data[v3]);
    }
    return s[v3] == 0;
}

int32_t hkStringBuf::operator==(const char* s) const {
    return compareTo(s) == 0;
}

int32_t hkStringBuf::indexOf(const char* a1, int32_t a2, int32_t a3) {
    const char* v5 = hkString::strStr(&m_string.m_data[a2], a1);
    if (v5) {
        return v5 -
            static_cast<hkArrayBase<char>&>(static_cast<hkArray<char, hkContainerTempAllocator>&>(m_string)).begin();
    }

    return -1;
}

int32_t hkStringBuf::slice(int32_t a1, int32_t a2) {
    if (a1) {
        hkMemUtil::memMove(m_string.begin(), &m_string.m_data[a1], a2);
    }
    int32_t v4 = a2 + 1;
    hkMemoryAllocator* m_temp = (&hkMemoryRouter::getInstance())->m_temp;
    int32_t result = m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK;
    if (result < a2 + 1) {
        int32_t v7 = 2 * result;
        if (v4 >= v7) {
            v7 = a2 + 1;
        }
        hkArrayUtil::_reserve(m_temp, &m_string, v7, sizeof(char));
    }
    char* m_data = m_string.begin();
    m_string.m_size = v4;
    m_data[a2] = 0;
    return result;
}

void* hkStringBuf::getArray() {
    return this;
}

int32_t hkStringBuf::indexOfCase(const char* needle) {
    char* m_data =
        static_cast<hkArrayBase<char>&>(static_cast<hkArray<char, hkContainerTempAllocator>&>(m_string)).begin();
    int32_t result = 0;
    char* v8 = static_cast<hkArrayBase<char>&>(static_cast<hkArray<char, hkContainerTempAllocator>&>(m_string)).begin();
    if (!*static_cast<hkArrayBase<char>&>(static_cast<hkArray<char, hkContainerTempAllocator>&>(m_string)).begin()) {
        return -1;
    }
    while (true) {
        char v4 = *needle;
        int32_t v5 = 0;
        if (*needle) {
            char* v6 = &m_data[result];
            do {
                char v7 = v6[v5];
                if (static_cast<uint8_t>(v7 - 65) <= 25u) {
                    v7 += 32;
                }
                if (static_cast<uint8_t>(v4 - 65) <= 25u) {
                    v4 += 32;
                }
                if (v7 != v4) {
                    break;
                }
                v4 = needle[++v5];
            }
            while (v4);
            m_data = v8;
        }
        if (!needle[v5]) {
            break;
        }
        if (!m_data[++result]) {
            return -1;
        }
    }
    return result;
}

int32_t hkStringBuf::lastIndexOf(const char* needle, int32_t startIndex, int32_t endIndex) {
    int32_t v5 = -1;
    for (const char* i = hkString::strStr(
             static_cast<hkArrayBase<char>&>(static_cast<hkArray<char, hkContainerTempAllocator>&>(m_string)).begin(),
             needle
         );
         i; i = hkString::strStr(i + 1, needle)) {
        v5 = i -
            static_cast<hkArrayBase<char>&>(static_cast<hkArray<char, hkContainerTempAllocator>&>(m_string)).begin();
    }
    return v5;
}

int32_t hkStringBuf::indexOf(char a1, int32_t a2, int32_t a3) {
    int32_t v4 = a3;
    if (a3 >= static_cast<int32_t>(m_string.m_size - 1)) {
        v4 = m_string.m_size - 1;
    }
    int32_t result = a2;
    if (a2 >= v4) {
        return -1;
    }
    while (m_string.m_data[result] != a1) {
        if (++result >= v4) {
            return -1;
        }
    }
    return result;
}

int32_t hkStringBuf::endsWithCase(const char* s) {
    int32_t v3 = hkString::strLen(s);
    uint32_t m_size = m_string.m_size;
    if (v3 > static_cast<int32_t>(m_size - 1)) {
        return 0;
    }
    int32_t v7 = m_size - v3 - 1;
    int32_t v8 = 0;
    if (v3 <= 0) {
        return 1;
    }
    char* v9 = &m_string.m_data[v7];
    while (true) {
        char v10 = v9[v8];
        if (static_cast<uint8_t>(v10 - 65) <= 25u) {
            v10 = v9[v8] + 32;
        }
        char v11 = s[v8];
        if (static_cast<uint8_t>(v11 - 65) <= 25u) {
            v11 += 32;
        }
        if (v10 != v11) {
            break;
        }
        if (++v8 >= v3) {
            return 1;
        }
    }
    return 0;
}

int32_t hkStringBuf::split(int32_t sep, hkArray<const char*, hkContainerTempAllocator>& bits) {
    int32_t v8;
    char* m_data = m_string.begin();
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (bits.m_size == (bits.m_capacityAndFlags & hkArrayBase<const char*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_temp, &bits, sizeof(const char*));
    }
    bits.m_data[bits.m_size++] = m_data;
    int32_t m_size = bits.m_size;
    for (int32_t i = indexOf(sep, 1, 0x7FFFFFFF); i >= 0; i = indexOf(sep, v8, 0x7FFFFFFF)) {
        m_string.m_data[i] = 0;
        v8 = i + 1;
        char* v9 = &m_string.m_data[i + 1];
        hkMemoryRouter* v10 = &hkMemoryRouter::getInstance();
        if (bits.m_size == (bits.m_capacityAndFlags & hkArrayBase<const char*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*v10->m_temp, &bits, sizeof(const char*));
        }
        bits.m_data[bits.m_size++] = v9;
        m_size = bits.m_size;
    }
    return m_size;
}

void hkStringBuf::chompEnd(int32_t n) {
    if (n > 0) {
        int32_t v3 = m_string.m_size - n - 1;
        int32_t v4 = m_string.m_size - n;
        hkMemoryAllocator* m_temp = (&hkMemoryRouter::getInstance())->m_temp;
        int32_t v6 = m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK;
        if (v6 < v4) {
            int32_t v7 = 2 * v6;
            if (v4 >= v7) {
                v7 = v3 + 1;
            }
            hkArrayUtil::_reserve(m_temp, &m_string, v7, sizeof(char));
        }
        char* m_data = m_string.begin();
        m_string.m_size = v4;
        m_data[v3] = 0;
    }
}

void hkStringBuf::chompStart(int32_t n) {
    if (n > 0) {
        m_string.m_size -= n;
        int32_t size = m_string.m_size;
        char* data = m_string.begin();
        if (size > 0) {
            const char* source = data + n;
            do {
                *data++ = *source++;
                --size;
            }
            while (size);
        }
    }
}

void hkStringBuf::set(const char* s, int32_t len) {
    int32_t v4 = len + 1;
    hkMemoryAllocator* m_temp = (&hkMemoryRouter::getInstance())->m_temp;
    int32_t v6 = m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK;
    if (v6 < len + 1) {
        int32_t v7 = 2 * v6;
        if (v4 >= v7) {
            v7 = len + 1;
        }
        hkArrayUtil::_reserve(m_temp, &m_string, v7, sizeof(char));
    }
    char* m_data = m_string.begin();
    m_string.m_size = v4;
    m_data[len] = 0;
    hkMemUtil::memCpy(m_string.begin(), s, len);
}

void hkStringBuf::pathDirname() {
    int32_t v2 = lastIndexOf(47, 0, 0x7FFFFFFF);
    int32_t v3 = lastIndexOf(92, 0, 0x7FFFFFFF);
    if (v3 <= v2) {
        v3 = v2;
    }
    if (v3 < 0) {
        clear();
    }
    else {
        slice(0, v3);
    }
}

void hkStringBuf::prepend(const char* s, int32_t len) {
    insert(0, s, len);
}

void replace(hkStringBuf* apThis, const char* oldText, const char* newText, int32_t type) {
    int32_t v5 = hkString::strLen(oldText);
    int32_t v6 = 0;
    int32_t len = hkString::strLen(newText);
    if (len <= v5) {
        char* m_data = apThis->m_string.begin();
        int32_t v10 = 0;
        int32_t v11 = 0;
        for (int32_t i = apThis->indexOf(oldText, 0, 0x7FFFFFFF); i != -1;
             i = apThis->indexOf(oldText, v5 + i, 0x7FFFFFFF)) {
            for (; v11 < i; ++v10) {
                m_data[v10] = m_data[v11++];
            }
            for (int32_t j = 0; j < len; ++v10) {
                m_data[v10] = newText[j++];
            }
            v11 += v5;
            if (!type) {
                break;
            }
        }
        for (; v11 < apThis->m_string.m_size - 1; ++v10) {
            m_data[v10] = m_data[v11++];
        }
        m_data[v10] = 0;
        int32_t v14 = v10 + 1;
        hkMemoryAllocator* m_temp = (&hkMemoryRouter::getInstance())->m_temp;
        int32_t v16 = apThis->m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK;
        if (v16 < v10 + 1) {
            int32_t v17 = 2 * v16;
            if (v14 >= v17) {
                v17 = v10 + 1;
            }
            hkArrayUtil::_reserve(*m_temp, apThis, v17, sizeof(char));
        }
        char* v18 = apThis->m_string.begin();
        apThis->m_string.m_size = v14;
        v18[v10] = 0;
    }
    else {
        FNVD_INLINED_LOCAL(hkStringBuf, v21, std::nullopt, std::nullopt);
        ::new (&v21) hkStringBuf(*apThis);
        apThis->clear();
        for (int32_t k = v21.indexOf(oldText, 0, 0x7FFFFFFF); k >= 0; k = v21.indexOf(oldText, k + v5, 0x7FFFFFFF)) {
            apThis->append(&v21.m_string.m_data[v6], k - v6);
            apThis->append(newText, len);
            v6 = k + v5;
            if (!type) {
                break;
            }
        }
        apThis->append(&v21.m_string.m_data[v6], v21.m_string.m_size - v6 - 1);
        hkMemoryAllocator* v8 = (&hkMemoryRouter::getInstance())->m_temp;
        v21.m_string.m_size = 0;
        if (v21.m_string.m_capacityAndFlags >= 0) {
            v8->bufFree(v21.m_string.begin(), v21.m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
        }
    }
}
