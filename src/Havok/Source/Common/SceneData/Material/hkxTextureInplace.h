#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

#include <stdint.h>

// Stores an embedded texture and its source filename metadata.
class hkxTextureInplace : public hkReferencedObject {
public:
    // The SDK's finish constructor: a loaded object keeps the fields the load wrote.
    hkxTextureInplace(hkFinishLoadedObjectFlag aFlag)
      : hkReferencedObject(aFlag), m_data(aFlag), m_name(aFlag), m_originalFilename(aFlag) {}

    ~hkxTextureInplace() override;

    static const hkClass& staticClass();

    char m_fileType[4];
    hkArray<uint8_t, hkContainerHeapAllocator> m_data;
    hkStringPtr m_name;
    hkStringPtr m_originalFilename;
};

extern hkClass hkxTextureInplaceClass;
extern hkTypeInfo hkxTextureInplaceTypeInfo;
