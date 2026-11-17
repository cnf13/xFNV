#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Packfile/hkPackfileWriter.h"
#include "Havok/Source/Common/Serialize/Util/hkSerializeUtil.h"
#include "Reconstruction.hpp"

class hkObjectResource;
class hkResource;
class hkStreamReader;
class hkStreamWriter;

// The loader of Havok's old packfile versions and the XML packfile writer, which a program opts in to with
// initDeprecated. The exe's vtable overrides the destructor and adds slots 3 to 6. hkResult is spelled int32_t and
// hkBool32 int32_t.
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

	// hkSingleton<hkSerializeDeprecated>::s_instance: hkSerializeUtil calls its slots 3, 5 and 6 through it.
	static inline hkSerializeDeprecated* s_instance;
};
inline constexpr fnvd::Vtable hkSerializeDeprecated_vftable{};
