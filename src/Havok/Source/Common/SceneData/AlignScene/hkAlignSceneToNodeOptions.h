#pragma once

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// Defined in hkObjectCopier.cpp.
void finishLoadedObjecthkAlignSceneToNodeOptions(void* p, int32_t finishing);

// Selects the components of a scene node transform to apply to its scene.
class hkAlignSceneToNodeOptions : public hkReferencedObject {
public:
    hkAlignSceneToNodeOptions();
    // The SDK's finish constructor: the loaded options keep their fields; the node name is finished.
    hkAlignSceneToNodeOptions(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_nodeName(aFlag) {}
    ~hkAlignSceneToNodeOptions() override;

    void setNodeName(const char* name);
    const hkStringPtr& getNodeName();

    // Havok SDK signature (hkSceneData.lib, tied by target).
    static const hkClass& staticClass();

    hkBool m_invert;
    hkBool m_transformPositionX;
    hkBool m_transformPositionY;
    hkBool m_transformPositionZ;
    hkBool m_transformRotation;
    hkBool m_transformScale;
    hkBool m_transformSkew;
    int32_t m_keyframe;
    hkStringPtr m_nodeName;
};

extern hkClass hkAlignSceneToNodeOptionsClass;

extern hkTypeInfo hkAlignSceneToNodeOptionsTypeInfo;
