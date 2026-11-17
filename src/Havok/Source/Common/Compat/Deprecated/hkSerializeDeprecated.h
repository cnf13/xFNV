#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Packfile/hkPackfileWriter.h"
#include "Havok/Source/Common/Serialize/Util/hkSerializeUtil.h"

class hkObjectResource;
class hkResource;
class hkStreamReader;
class hkStreamWriter;

// The loader of Havok's old packfile versions and the XML packfile writer, which a program opts in to with
// initDeprecated.
class hkSerializeDeprecated : public hkReferencedObject, public hkSingleton<hkSerializeDeprecated> {
public:
	~hkSerializeDeprecated() override;

	virtual hkResult saveXmlPackfile(const void* object, const hkClass& klass, hkStreamWriter* writer,
		const hkPackfileWriter::Options& options, hkPackfileWriter::AddObjectListener* userListener);
	virtual int32_t isLoadable(const hkSerializeUtil::FormatDetails& details);
	virtual hkResource* loadOldPackfile(hkStreamReader& sr, const hkSerializeUtil::FormatDetails& details,
		hkSerializeUtil::ErrorDetails* errorOut);
	virtual hkObjectResource* loadOldPackfileOnHeap(hkStreamReader& sr, const hkSerializeUtil::FormatDetails& details,
		hkSerializeUtil::ErrorDetails* errorOut);

	static inline hkSerializeDeprecated* s_instance;
};
