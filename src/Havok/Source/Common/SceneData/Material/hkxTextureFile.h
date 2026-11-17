#pragma once

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

#include <stdint.h>

// A texture resource referenced by its exported and original filenames.
class hkxTextureFile : public hkReferencedObject {
public:
	// The SDK's finish constructor: a loaded object keeps the fields the load wrote.
	hkxTextureFile(hkFinishLoadedObjectFlag aFlag)
		: hkReferencedObject(aFlag), m_filename(aFlag), m_name(aFlag), m_originalFilename(aFlag) {}

	~hkxTextureFile() override;

	static const hkClass& staticClass();

	hkStringPtr m_filename;
	hkStringPtr m_name;
	hkStringPtr m_originalFilename;
};

extern hkClass hkxTextureFileClass;
extern hkTypeInfo hkxTextureFileTypeInfo;
