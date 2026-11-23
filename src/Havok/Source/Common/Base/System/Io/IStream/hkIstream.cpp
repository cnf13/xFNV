#include "Havok/Source/Common/Base/System/Io/IStream/hkIstream.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Io/FileSystem/hkFileSystem.h"
#include "Havok/Source/Common/Base/System/Io/Reader/Memory/hkMemoryStreamReader.h"
#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/System/Io/Writer/Array/hkArrayStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "SDK/MSVC/stdlib.h"

// Reads one character from the stream: the character, or -1 at the end. hkIstream passes its m_streamReader.
static int32_t readChar(hkStreamReader* sr);

// Returns the character read through the stream's read, sign-extended, or -1 when nothing was read.
static int32_t readChar(hkStreamReader* sr) {
    char c;
    if (sr->read(&c, 1)) {
        return c;
    }
    return -1;
}

int32_t eatWhiteSpace(hkStreamReader* sr) {
    uint8_t v4[64];
    sr->setMark(64);
    int32_t result = sr->read(v4, 64);
    while (result) {
        int32_t v2 = 0;
        if (result > 0) {
            while (true) {
                char v3 = v4[v2];
                if (v3 != 32 && (v3 != 9 && (v3 != 13 && v3 != 10))) {
                    sr->rewindToMark();
                    return sr->skip(v2);
                }
                if (++v2 >= result) {
                    break;
                }
            }
        }
        sr->setMark(64);
        result = sr->read(v4, 64);
    }
    return result;
}

uint64_t readInteger(hkStreamReader* sr, char* negativeOut) {
    char v6;
    char v7;
    uint32_t v8;
    int32_t i;
    uint32_t v12;
    uint8_t v15[256];
    eatWhiteSpace(sr);
    *negativeOut = 0;
    int32_t v4 = 0;
    uint64_t v14 = 0;
    sr->setMark(255);
    int32_t v5 = sr->read(v15, 255);
    if (!v5) {
        return 0;
    }
    if (v15[0] == 43) {
        v4 = 1;
    }
    else if (v15[0] == 45) {
        *negativeOut = 1;
        v4 = 1;
    }
    if (v4 + 3 < v5 &&
        (v15[v4] == 48 &&
         (((v6 = v15[v4 + 1]) == 120 || v6 == 88) &&
          (((v7 = v15[v4 + 2]) >= 48 && v7 <= 57) || ((v7 >= 97 && v7 <= 102) || (v7 >= 65 && v7 <= 70)))))) {
        v8 = 16;
        v4 += 2;
    }
    else {
        char v9;
        if (v4 + 2 < v5 && (v15[v4] == 48 && ((v9 = v15[v4 + 1]) >= 48 && v9 <= 57))) {
            v8 = 8;
            ++v4;
        }
        else {
            v8 = 10;
        }
    }
    for (i = v4; i < v5; v14 = v12 + v8 * v14) {
        char v11 = v15[i];
        v12 = -1;
        if (v11 < 48 || v11 > 57) {
            if (v11 <= 65 || v11 > 70) {
                if (v11 > 97 && v11 <= 102) {
                    v12 = v11 - 87;
                }
            }
            else {
                v12 = v11 - 55;
            }
        }
        else {
            v12 = v11 - 48;
        }
        if (v12 >= v8) {
            break;
        }
        ++i;
    }
    sr->rewindToMark();
    sr->skip(i);
    return v14;
}

double readFloat(hkStreamReader* sr) {
    char String[4];
    uint8_t v9[260];
    eatWhiteSpace(sr);
    int32_t i = 0;
    sr->setMark(255);
    int32_t v3 = sr->read(v9, 255);
    if (v3) {
        uint8_t v8[8];
        if ((v8[0] >= 48 && v8[0] <= 57) || (v8[0] == 43 || (v8[0] == 45 || (v8[0] == 46 || v8[0] == 44)))) {
            for (i = 1; i < v3; ++i) {
                char v4 = v8[i];
                if ((v4 < 48 || v4 > 57) && (v4 != 43 && (v4 != 45 && (v4 != 69 && v4 != 101)))) {
                    if (v4 == 46) {
                        v8[i] = 46;
                        continue;
                    }
                    if (v4 != 44) {
                        break;
                    }
                }
                if (v4 == 44) {
                    v8[i] = 46;
                }
            }
        }
    }
    sr->rewindToMark();
    sr->skip(i);
    String[i] = 0;
    if (i <= 0) {
        return -1.0f;
    }
    return static_cast<float>(std::strtod(String, nullptr));
}

hkIstream::hkIstream(hkStreamReader* sr) : m_streamReader(sr) {
    sr->addReference();
}

hkIstream::hkIstream(const hkMemoryTrack* apTrack) {
    m_streamReader = new (hkReferencedObject::newObject(sizeof(hkMemoryTrackStreamReader)))
        hkMemoryTrackStreamReader(apTrack, hkMemoryTrackStreamReader::MEMORY_INPLACE);
}

hkIstream::hkIstream(const char* fname) : m_streamReader(hkFileSystem::s_instance->openReader(fname)) {}

hkIstream::hkIstream(const void* mem, int32_t memSize) {
    m_streamReader = new (hkReferencedObject::newObject(sizeof(hkMemoryStreamReader)))
        hkMemoryStreamReader(mem, memSize, hkMemoryStreamReader::MEMORY_INPLACE);
}

hkIstream::~hkIstream() {
    if (m_streamReader) {
        m_streamReader->removeReference();
    }
}

hkBool hkIstream::isOk() {
    if (m_streamReader) {
        return m_streamReader->isOk();
    }

    return 0;
}

int32_t hkIstream::getline(char* str, int32_t maxlen, char delim) {
    eatWhiteSpace(m_streamReader);
    int32_t v5 = 0;
    if (maxlen <= 0) {
        return -1;
    }
    while (true) {
        int32_t v6 = readChar(m_streamReader);
        if (v6 == -1 || v6 == delim) {
            break;
        }
        str[v5] = v6;
        if (++v5 >= maxlen) {
            return -1;
        }
    }
    str[v5] = 0;
    return v5;
}

// Skips white space, then reads the characters up to the next white space or the end of the stream into str, the
// stream's mark kept after each one taken so that rewinding leaves the delimiter unread; the text is terminated. str's
// array is its m_string.
hkIstream& hkIstream::operator>>(hkStringBuf& str) {
    str.clear();
    hkArray<char, hkContainerTempAllocator>& buf = str.m_string;
    eatWhiteSpace(m_streamReader);
    m_streamReader->setMark(1);
    for (int32_t c = readChar(m_streamReader); c != -1; c = readChar(m_streamReader)) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            break;
        }
        buf.pushBack(static_cast<char>(c));
        m_streamReader->setMark(1);
    }
    m_streamReader->rewindToMark();
    buf.pushBack('\0');
    return *this;
}

hkIstream& hkIstream::operator>>(char& c) {
    m_streamReader->read(&c, 1);
    return *this;
}

hkIstream& hkIstream::operator>>(float& f) {
    f = readFloat(m_streamReader);
    return *this;
}

hkIstream& hkIstream::operator>>(double& d) {
    d = readFloat(m_streamReader);
    return *this;
}

int32_t hkIstream::read(void* buf, int32_t nbytes) {
    return m_streamReader->read(buf, nbytes);
}

hkIstream& hkIstream::operator>>(int16_t& s) {
    char negative;
    uint16_t v = static_cast<uint16_t>(readInteger(m_streamReader, &negative));
    if (negative) {
        v = static_cast<uint16_t>(0 - v);
    }
    s = static_cast<int16_t>(v);
    return *this;
}

hkIstream& hkIstream::operator>>(uint16_t& s) {
    char negative;
    uint16_t v = static_cast<uint16_t>(readInteger(m_streamReader, &negative));
    if (negative) {
        v = static_cast<uint16_t>(0 - v);
    }
    s = v;
    return *this;
}

hkIstream& hkIstream::operator>>(int32_t& i) {
    char negative;
    uint32_t v = static_cast<uint32_t>(readInteger(m_streamReader, &negative));
    if (negative) {
        v = 0u - v;
    }
    i = static_cast<int32_t>(v);
    return *this;
}

hkIstream& hkIstream::operator>>(uint32_t& u) {
    char negative;
    uint32_t v = static_cast<uint32_t>(readInteger(m_streamReader, &negative));
    if (negative) {
        v = 0u - v;
    }
    u = v;
    return *this;
}

hkIstream& hkIstream::operator>>(int64_t& i) {
    char negative;
    uint64_t v = readInteger(m_streamReader, &negative);
    if (negative) {
        v = 0 - v;
    }
    i = static_cast<int64_t>(v);
    return *this;
}

hkIstream& hkIstream::operator>>(uint64_t& u) {
    char negative;
    uint64_t v = readInteger(m_streamReader, &negative);
    if (negative) {
        v = 0 - v;
    }
    u = v;
    return *this;
}

hkIstream& hkIstream::operator>>(hkStringPtr& str) {
    hkStringBuf s;
    *this >> s;
    str = s.m_string.begin();
    return *this;
}

hkIstream& hkIstream::operator>>(hkBool& b) {
    char Str1[6];
    eatWhiteSpace(m_streamReader);
    m_streamReader->setMark(6);
    int32_t v3 = m_streamReader->read(Str1, 6);
    if (v3 >= 4 &&
        (!hkString::strNcmp(Str1, "true", 4) &&
         (v3 == 4 || (Str1[4] == 32 || (Str1[4] == 9 || (Str1[4] == 13 || Str1[4] == 10)))))) {
        b.m_bool = 1;
        m_streamReader->rewindToMark();
        m_streamReader->skip(4);
        return *this;
    }

    if (v3 >= 5 &&
        (!hkString::strNcmp(Str1, "false", 4) &&
         (v3 == 5 || (Str1[5] == 32 || (Str1[5] == 9 || (Str1[5] == 13 || Str1[5] == 10)))))) {
        b.m_bool = 0;
        m_streamReader->rewindToMark();
        m_streamReader->skip(5);
        return *this;
    }

    m_streamReader->rewindToMark();
    return *this;
}
