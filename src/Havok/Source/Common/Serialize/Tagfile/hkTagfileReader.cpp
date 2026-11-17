#include "Havok/Source/Common/Serialize/Tagfile/hkTagfileReader.h"

#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Tagfile/Binary/hkBinaryTagfileCommon.h"
#include "Havok/Source/Common/Serialize/Util/hkSerializeUtil.h"

// The SDK's detectFormat: the stream's first 16 bytes, read under a mark and rewound, are tested for the binary
// tagfile's magic (0xD483A0, whose hkBool result the exe keeps in the stream argument's slot).
hkEnum<hkTagfileReader::FormatType, int32_t>*
hkTagfileReader::detectFormat(hkEnum<hkTagfileReader::FormatType, int32_t>* apResult, hkStreamReader* stream) {
	int32_t buf[4];
	stream->setMark(16);
	if (stream->read(buf, 16) != 16) {
		// EXE-INLINED none: hkEnum<hkTagfileReader::FormatType, int32_t>::operator=
		apResult->m_storage = FORMAT_ERROR;
		return apResult;
	}
	stream->rewindToMark();
	uint8_t ucIsBinary;
	if (*hkBinaryTagfile::isBinaryMagic(&ucIsBinary, buf[0], buf[1])) {
		// EXE-INLINED none: hkEnum<hkTagfileReader::FormatType, int32_t>::operator=
		apResult->m_storage = FORMAT_BINARY;
	} else {
		// EXE-INLINED none: hkEnum<hkTagfileReader::FormatType, int32_t>::operator=
		apResult->m_storage = FORMAT_UNKNOWN;
	}
	return apResult;
}
