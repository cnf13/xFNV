#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Base/Types/hkRefVariant.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeHolder.h"

#include <stdint.h>

void cleanupLoadedObjecthkxNode(void* p);
// Havok SDK signature (hkSceneData.lib, tied by bytes,reloc).
const void* getVtablehkxNode();

// An attributed scene graph node with animated transforms and annotations.
class hkxNode : public hkxAttributeHolder {
public:
    struct AnnotationData {
        static const hkClass& staticClass();

        float m_time;
        hkStringPtr m_description;
    };

    hkxNode() = default;

    hkxNode(hkFinishLoadedObjectFlag aFlag)
      : hkxAttributeHolder(aFlag), m_name(aFlag), m_object(aFlag), m_keyFrames(aFlag), m_children(aFlag),
        m_annotations(aFlag), m_userProperties(aFlag) {}

    ~hkxNode() override;

    hkxNode* findChildByName(const char* childName) const;
    hkxNode* findDescendantByName(const char* name) const;
    void replaceAllObjects(const hkReferencedObject* oldObject, const hkReferencedObject* newObject);
    int32_t getNumDescendants() const;
    hkRefVariant findVariantByObject(const hkReferencedObject* object) const;
    hkResult getPathToNode(const hkxNode* theNode, hkArray<const hkxNode*, hkContainerHeapAllocator>& pathOut);

    static const hkClass& staticClass();

    hkStringPtr m_name;
    hkRefVariant m_object;
    hkArray<hkMatrix4> m_keyFrames;
    hkArray<hkRefPtr<hkxNode>> m_children;
    hkArray<AnnotationData> m_annotations;
    hkStringPtr m_userProperties;
    hkBool m_selected;
};

extern hkClass hkxNodeAnnotationDataClass;
extern hkClass hkxNodeClass;
extern hkTypeInfo hkxNodeTypeInfo;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkxNodeAnnotationDataTypeInfo;
