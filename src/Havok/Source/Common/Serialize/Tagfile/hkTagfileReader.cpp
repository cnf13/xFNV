#include "Havok/Source/Common/Serialize/Tagfile/hkTagfileReader.h"

#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Tagfile/Binary/hkBinaryTagfileCommon.h"
#include "Havok/Source/Common/Serialize/Util/hkSerializeUtil.h"

// The stream's first 16 bytes, read under a mark and rewound, are tested for the binary tagfile's magic.
hkEnum<hkTagfileReader::FormatType, int32_t>*
hkTagfileReader::detectFormat(hkEnum<hkTagfileReader::FormatType, int32_t>* apResult, hkStreamReader* stream) {
    int32_t buf[4];
    stream->setMark(16);
    if (stream->read(buf, 16) != 16) {
        *apResult = FORMAT_ERROR;
        return apResult;
    }
    stream->rewindToMark();
    uint8_t ucIsBinary;
    if (*hkBinaryTagfile::isBinaryMagic(&ucIsBinary, buf[0], buf[1])) {
        *apResult = FORMAT_BINARY;
    }
    else {
        *apResult = FORMAT_UNKNOWN;
    }
    return apResult;
}
