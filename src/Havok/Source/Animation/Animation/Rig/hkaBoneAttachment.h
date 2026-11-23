#pragma once

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkRefVariant.h"

#include <stdint.h>

// Attaches a serialized object to a named skeleton bone through a local transform.
class hkaBoneAttachment : public hkReferencedObject {
public:
    hkaBoneAttachment(hkFinishLoadedObjectFlag aFlag)
      : hkReferencedObject(aFlag), m_originalSkeletonName(aFlag), m_attachment(aFlag), m_name(aFlag) {}

    ~hkaBoneAttachment() override;

    static const hkClass& staticClass();
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }

    hkStringPtr m_originalSkeletonName;
    // The 16-byte alignment of hkMatrix4 is what pads the class (0xC and, after m_boneIndex, to 0x60).
    hkMatrix4 m_boneFromAttachment;
    hkRefVariant m_attachment;
    hkStringPtr m_name;
    int16_t m_boneIndex;
};

extern hkClass hkaBoneAttachmentClass;
extern hkTypeInfo hkaBoneAttachmentTypeInfo;
