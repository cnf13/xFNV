#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

// Havok SDK signature (hkaAnimation.lib, tied by reloc,target).
const void* getVtablehkaAnimationPreviewColorContainer();

// Packed colors used to display animation previews.
class hkaAnimationPreviewColorContainer : public hkReferencedObject {
public:
    hkaAnimationPreviewColorContainer();

    hkaAnimationPreviewColorContainer(hkFinishLoadedObjectFlag aFlag)
      : hkReferencedObject(aFlag), m_previewColor(aFlag) {}

    ~hkaAnimationPreviewColorContainer() override;

    void newPreviewColor(uint32_t color);

    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }
    // The reflection class hkaAnimationPreviewColorContainerClass.
    static const hkClass& staticClass();

    hkArray<uint32_t> m_previewColor;
};

extern hkClass hkaAnimationPreviewColorContainerClass;
extern hkTypeInfo hkaAnimationPreviewColorContainerTypeInfo;
