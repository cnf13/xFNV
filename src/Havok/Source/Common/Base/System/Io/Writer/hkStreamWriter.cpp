#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"

#include "Havok/Source/Animation/Animation/Rig/hkaBone.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// hkStreamWriterClass's enum SeekWhence and its items.
using hkStreamWriterSeekWhenceEnumItemArray = const hkClassEnum::Item[3];
static hkStreamWriterSeekWhenceEnumItemArray hkStreamWriterSeekWhenceEnumItems{{0, "STREAM_SET"}, {1, "STREAM_CUR"},
	{2, "STREAM_END"}};
using hkStreamWriterEnumArray = const hkClassEnum[1];
static hkStreamWriterEnumArray hkStreamWriterEnums{{"SeekWhence", hkStreamWriterSeekWhenceEnumItems, 3, nullptr, 0}};

hkClass hkStreamWriterClass("hkStreamWriter", &hkReferencedObjectClass, 8, nullptr, 0, hkStreamWriterEnums, 1, nullptr,
	0, nullptr, nullptr, 1, 0);

hkStreamWriter::~hkStreamWriter() {}

hkBool* hkStreamWriter::seekTellSupported(hkBool* arResult) const {
	arResult->m_bool = 0;
	return arResult;
}

hkResult hkStreamWriter::seek(int32_t offset, SeekWhence whence) {
	return static_cast<hkResult>(1);
}

int32_t hkStreamWriter::tell() const {
	return (-1);
}
