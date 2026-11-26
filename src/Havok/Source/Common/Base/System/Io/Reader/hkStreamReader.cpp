#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

int32_t hkStreamReader::skip(int32_t nbytes) {
    int32_t i;
    int32_t v5;
    for (i = nbytes; i; i -= v5) {
        char v7[512];
        int32_t v4 = 512;
        if (i <= 512) {
            v4 = i;
        }
        v5 = read(v7, v4);
        if (!v5) {
            break;
        }
    }
    return nbytes - i;
}

hkBool hkStreamReader::markSupported() const {
    return 0;
}

hkResult hkStreamReader::setMark(int32_t markLimit) {
    return HK_FAILURE;
}

hkResult hkStreamReader::rewindToMark() {
    return HK_FAILURE;
}

hkBool hkStreamReader::seekTellSupported() const {
    return 0;
}

hkResult hkStreamReader::seek(int32_t offset, SeekWhence whence) {
    return HK_FAILURE;
}

int32_t hkStreamReader::tell() const {
    return (-1);
}

hkStreamReader::~hkStreamReader() {}
