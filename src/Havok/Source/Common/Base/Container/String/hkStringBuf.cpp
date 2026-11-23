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
#include <limits>

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
    char cLess = hkString::strCmp(m_string.begin(), other) < 0;
    result.m_bool = cLess;
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
    m_string.setSize(1);
    m_string.m_data[0] = 0;
}

hkStringBuf& hkStringBuf::operator=(const char* s) {
    if (s) {
        int32_t iLength = hkString::strLen(s);
        m_string.setSize(iLength + 1);
        m_string.m_data[iLength] = 0;
        hkString::memCpy(m_string.begin(), s, iLength);
        return *this;
    }

    m_string.setSize(1);
    m_string.m_data[0] = 0;
    return *this;
}

hkStringBuf& hkStringBuf::operator=(const hkStringBuf& s) {
    int32_t m_size;
    hkMemoryAllocator* m_temp = hkMemoryRouter::getInstance().m_temp;
    const hkStringBuf* pSource = &s;
    if (m_string.getCapacity() < s.m_string.m_size) {
        if (m_string.m_capacityAndFlags >= 0) {
            m_temp->bufFree(m_string.begin(), m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
        }
        m_size = pSource->m_string.m_size;
        m_string.m_data = static_cast<char*>(m_temp->bufAlloc(m_size));
        m_string.m_capacityAndFlags = m_size;
    }
    int32_t iSize = pSource->m_string.m_size;
    char* pcString = m_string.begin();
    m_string.m_size = iSize;
    if (iSize > 0) {
        int32_t iSourceOffset = pSource->m_string.m_data - pcString;
        int32_t iRemaining = iSize;
        do {
            *pcString = pcString[iSourceOffset];
            ++pcString;
            --iRemaining;
        }
        while (iRemaining);
    }
    return *this;
}

void hkStringBuf::replace(char a, char b, ReplaceType replaceType) {
    for (int32_t i = 0; i < static_cast<int32_t>(m_string.m_size - 1); ++i) {
        char* pcChar = &m_string.m_data[i];
        if (*pcChar == a) {
            *pcChar = b;
            if (replaceType == REPLACE_ONE) {
                break;
            }
        }
    }
}

// Measures each string up to the first null one, sizes the array to the joined length plus the terminating zero,
// writes the zero, then copies each string after the old text.
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
    m_string.setSize(iNewLength + 1);
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
// it in the size.
void hkStringBuf::printf(const char* fmt, ...) {
    va_list arguments;
    va_start(arguments, fmt);
    for (;;) {
        const int32_t iCapacity = m_string.getCapacity();
        const int32_t iLength = hkString::vsnprintf(m_string.m_data, iCapacity, fmt, arguments);
        if (iLength < 0) {
            int32_t iNewCapacity = 2 * iCapacity;
            if (iNewCapacity <= 255) {
                iNewCapacity = 255;
            }
            m_string.setSize(iNewCapacity + 1);
            m_string.m_data[iNewCapacity] = 0;
            continue;
        }
        m_string.setSize(iLength + 1);
        m_string.m_data[iLength] = 0;
        if (iLength < iCapacity) {
            break;
        }
    }
}

// Formats into a temporary buffer and appends the text to this one, retrying with more storage until the text fits.
void hkStringBuf::appendPrintf(const char* fmt, ...) {
    va_list arguments;
    va_start(arguments, fmt);
    hkStringBuf kBuffer;
    for (;;) {
        const int32_t iCapacity = kBuffer.m_string.getCapacity();
        const int32_t iLength = hkString::vsnprintf(kBuffer.m_string.m_data, iCapacity, fmt, arguments);
        if (iLength < 0) {
            int32_t iNewCapacity = 2 * iCapacity;
            if (iNewCapacity <= 255) {
                iNewCapacity = 255;
            }
            kBuffer.m_string.setSize(iNewCapacity + 1);
            kBuffer.m_string.m_data[iNewCapacity] = 0;
            continue;
        }
        kBuffer.m_string.setSize(iLength + 1);
        kBuffer.m_string.m_data[iLength] = 0;
        if (iLength < iCapacity) {
            break;
        }
    }
    append(kBuffer.m_string.m_data, -1);
}

void hkStringBuf::append(const char* s, int32_t len) {
    if (len < 0) {
        len = hkString::strLen(s);
    }
    int32_t iOldLength = m_string.m_size - 1;
    m_string.setSize(m_string.m_size + len);
    m_string.m_data[iOldLength + len] = 0;
    hkMemUtil::memCpy(&m_string.m_data[iOldLength], s, len);
}

// The inline characters become the storage, flagged not to be freed, then the text is assigned. Clears the ownership
// flag off arString's pointer, and returns the buffer itself.
hkStringBuf* hkStringBuf::constructFrom(const hkStringPtr& arString) {
    m_string.m_data = m_string.m_storage;
    m_string.m_size = 0;
    m_string.m_capacityAndFlags = hkArrayBase<char>::DONT_DEALLOCATE_FLAG | 128;
    *this = arString.cString();
    return this;
}

int32_t hkStringBuf::compareTo(const char* apcRight) const {
    return hkString::strCmp(m_string.m_data, apcRight);
}

int32_t hkStringBuf::lastIndexOf(char c, int32_t startIndex, int32_t endIndex) {
    int8_t* pcString;
    int32_t iEnd = endIndex;
    if (endIndex > (m_string.m_size - 1)) {
        iEnd = m_string.m_size - 1;
    }
    int32_t result = iEnd - 1;
    if ((iEnd - 1) < startIndex) {
        return (-1);
    }
    pcString = static_cast<int8_t*>(static_cast<void*>(m_string.m_data));
    while (pcString[result] != c) {
        if ((--result) < startIndex) {
            return (-1);
        }
    }
    return result;
}

void hkStringBuf::setJoin(
    const char* s0,
    const char* s1,
    const char* s2,
    const char* s3,
    const char* s4,
    const char* s5
) {
    clear();
    appendJoin(s0, s1, s2, s3, s4, s5);
}

void hkStringBuf::pathBasename() {
    int32_t iLastSlash = lastIndexOf('/', 0, std::numeric_limits<int32_t>::max());
    int32_t iLastSeparator = lastIndexOf('\\', 0, std::numeric_limits<int32_t>::max());
    if (iLastSeparator <= iLastSlash) {
        iLastSeparator = iLastSlash;
    }
    if (iLastSeparator >= 0) {
        chompStart(iLastSeparator + 1);
    }
}

void hkStringBuf::lowerCase() {
    int32_t result = m_string.m_size - 1;
    int32_t iIndex = 0;
    if (result > 0) {
        do {
            result = m_string.m_data[iIndex];
            if (static_cast<uint8_t>(m_string.m_data[iIndex] - 65) <= 0x19u) {
                result += 32;
            }
            m_string.m_data[iIndex++] = static_cast<int8_t>(result);
        }
        while (iIndex < (m_string.m_size - 1));
    }
}

void hkStringBuf::upperCase() {
    int32_t result = m_string.m_size - 1;
    int32_t iIndex = 0;
    if (result > 0) {
        do {
            result = m_string.m_data[iIndex];
            if (static_cast<uint8_t>(m_string.m_data[iIndex] - 97) <= 0x19u) {
                result -= 32;
            }
            m_string.m_data[iIndex++] = static_cast<int8_t>(result);
        }
        while (iIndex < (m_string.m_size - 1));
    }
}

int32_t hkStringBuf::compareToIgnoreCase(const char* other) const {
    return hkString::strCasecmp(m_string.m_data, other);
}

int32_t hkStringBuf::startsWithCase(const char* s) const {
    int8_t* i;
    int8_t cPrefix;
    int8_t cString;
    if (*m_string.m_data) {
        for (i = static_cast<int8_t*>(static_cast<void*>(m_string.m_data)); *i; ++i) {
            cPrefix = *s;
            if (!(*s)) {
                break;
            }
            cString = *i;
            if (static_cast<uint8_t>((*i) - 65) <= 0x19u) {
                cString += 32;
            }
            if (static_cast<uint8_t>(cPrefix - 65) <= 0x19u) {
                cPrefix += 32;
            }
            if (cString != cPrefix) {
                return 0;
            }
            ++s;
        }
    }
    return 1;
}

uint32_t hkStringBuf::startsWith(const char* s) const {
    const char* pcString = m_string.begin();
    int32_t iIndex = 0;
    if (*m_string.begin()) {
        do {
            char cPrefix = s[iIndex];
            if (!cPrefix) {
                break;
            }
            if (pcString[iIndex] != cPrefix) {
                return 0;
            }
            ++iIndex;
        }
        while (pcString[iIndex]);
    }
    return s[iIndex] == 0;
}

int32_t hkStringBuf::operator==(const char* s) const {
    return compareTo(s) == 0;
}

int32_t hkStringBuf::indexOf(const char* s, int32_t startIndex, int32_t endIndex) {
    const char* pcMatch = hkString::strStr(&m_string.m_data[startIndex], s);
    if (pcMatch) {
        return pcMatch - m_string.begin();
    }

    return -1;
}

int32_t hkStringBuf::slice(int32_t startOffset, int32_t length) {
    if (startOffset) {
        hkMemUtil::memMove(m_string.begin(), &m_string.m_data[startOffset], length);
    }
    int32_t result = m_string.getCapacity();
    m_string.setSize(length + 1);
    m_string[length] = 0;
    return result;
}

void* hkStringBuf::getArray() {
    return this;
}

int32_t hkStringBuf::indexOfCase(const char* needle) {
    char* pcString = m_string.begin();
    int32_t result = 0;
    char* pcStart = m_string.begin();
    if (!*m_string.begin()) {
        return -1;
    }
    while (true) {
        char cNeedle = *needle;
        int32_t iMatch = 0;
        if (*needle) {
            char* pcCandidate = &pcString[result];
            do {
                char cString = pcCandidate[iMatch];
                if (static_cast<uint8_t>(cString - 65) <= 25u) {
                    cString += 32;
                }
                if (static_cast<uint8_t>(cNeedle - 65) <= 25u) {
                    cNeedle += 32;
                }
                if (cString != cNeedle) {
                    break;
                }
                cNeedle = needle[++iMatch];
            }
            while (cNeedle);
            pcString = pcStart;
        }
        if (!needle[iMatch]) {
            break;
        }
        if (!pcString[++result]) {
            return -1;
        }
    }
    return result;
}

int32_t hkStringBuf::lastIndexOf(const char* needle, int32_t startIndex, int32_t endIndex) {
    int32_t iLastIndex = -1;
    for (const char* i = hkString::strStr(m_string.begin(), needle); i; i = hkString::strStr(i + 1, needle)) {
        iLastIndex = i - m_string.begin();
    }
    return iLastIndex;
}

int32_t hkStringBuf::indexOf(char c, int32_t startIndex, int32_t endIndex) {
    int32_t iEnd = endIndex;
    if (endIndex >= static_cast<int32_t>(m_string.m_size - 1)) {
        iEnd = m_string.m_size - 1;
    }
    int32_t result = startIndex;
    if (startIndex >= iEnd) {
        return -1;
    }
    while (m_string.m_data[result] != c) {
        if (++result >= iEnd) {
            return -1;
        }
    }
    return result;
}

int32_t hkStringBuf::endsWithCase(const char* s) {
    int32_t iLength = hkString::strLen(s);
    uint32_t m_size = m_string.m_size;
    if (iLength > static_cast<int32_t>(m_size - 1)) {
        return 0;
    }
    int32_t iOffset = m_size - iLength - 1;
    int32_t iIndex = 0;
    if (iLength <= 0) {
        return 1;
    }
    char* pcTail = &m_string.m_data[iOffset];
    while (true) {
        char cString = pcTail[iIndex];
        if (static_cast<uint8_t>(cString - 65) <= 25u) {
            cString = pcTail[iIndex] + 32;
        }
        char cSuffix = s[iIndex];
        if (static_cast<uint8_t>(cSuffix - 65) <= 25u) {
            cSuffix += 32;
        }
        if (cString != cSuffix) {
            break;
        }
        if (++iIndex >= iLength) {
            return 1;
        }
    }
    return 0;
}

int32_t hkStringBuf::split(int32_t sep, hkArray<const char*, hkContainerTempAllocator>& bits) {
    int32_t iNext;
    char* pFirst = m_string.begin();
    bits.pushBack(pFirst);
    int32_t iNumBits = bits.m_size;
    for (int32_t i = indexOf(sep, 1, std::numeric_limits<int32_t>::max()); i >= 0;
         i = indexOf(sep, iNext, std::numeric_limits<int32_t>::max())) {
        m_string.m_data[i] = 0;
        iNext = i + 1;
        char* pBit = &m_string.m_data[i + 1];
        bits.pushBack(pBit);
        iNumBits = bits.m_size;
    }
    return iNumBits;
}

void hkStringBuf::chompEnd(int32_t n) {
    if (n > 0) {
        int32_t iNewLength = m_string.m_size - n - 1;
        m_string.setSize(m_string.m_size - n);
        m_string.m_data[iNewLength] = 0;
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
    m_string.setSize(len + 1);
    m_string.m_data[len] = 0;
    hkMemUtil::memCpy(m_string.begin(), s, len);
}

void hkStringBuf::pathDirname() {
    int32_t iLastSlash = lastIndexOf('/', 0, std::numeric_limits<int32_t>::max());
    int32_t iLastSeparator = lastIndexOf('\\', 0, std::numeric_limits<int32_t>::max());
    if (iLastSeparator <= iLastSlash) {
        iLastSeparator = iLastSlash;
    }
    if (iLastSeparator < 0) {
        clear();
    }
    else {
        slice(0, iLastSeparator);
    }
}

void hkStringBuf::prepend(const char* s, int32_t len) {
    insert(0, s, len);
}

void replace(hkStringBuf* apThis, const char* oldText, const char* newText, int32_t type) {
    int32_t iOldTextLength = hkString::strLen(oldText);
    int32_t iCopyFrom = 0;
    int32_t len = hkString::strLen(newText);
    if (len <= iOldTextLength) {
        char* pcString = apThis->m_string.begin();
        int32_t iWriteIndex = 0;
        int32_t iReadIndex = 0;
        for (int32_t i = apThis->indexOf(oldText, 0, std::numeric_limits<int32_t>::max()); i != -1;
             i = apThis->indexOf(oldText, iOldTextLength + i, std::numeric_limits<int32_t>::max())) {
            for (; iReadIndex < i; ++iWriteIndex) {
                pcString[iWriteIndex] = pcString[iReadIndex++];
            }
            for (int32_t j = 0; j < len; ++iWriteIndex) {
                pcString[iWriteIndex] = newText[j++];
            }
            iReadIndex += iOldTextLength;
            if (!type) {
                break;
            }
        }
        for (; iReadIndex < apThis->m_string.m_size - 1; ++iWriteIndex) {
            pcString[iWriteIndex] = pcString[iReadIndex++];
        }
        pcString[iWriteIndex] = 0;
        apThis->m_string.setSize(iWriteIndex + 1);
        apThis->m_string.m_data[iWriteIndex] = 0;
    }
    else {
        hkStringBuf kCopy(*apThis);
        apThis->clear();
        for (int32_t k = kCopy.indexOf(oldText, 0, std::numeric_limits<int32_t>::max()); k >= 0;
             k = kCopy.indexOf(oldText, k + iOldTextLength, std::numeric_limits<int32_t>::max())) {
            apThis->append(&kCopy.m_string.m_data[iCopyFrom], k - iCopyFrom);
            apThis->append(newText, len);
            iCopyFrom = k + iOldTextLength;
            if (!type) {
                break;
            }
        }
        apThis->append(&kCopy.m_string.m_data[iCopyFrom], kCopy.m_string.m_size - iCopyFrom - 1);
    }
}
