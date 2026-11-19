#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroup.h"

// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
const void* getVtablehkxAttributeHolder();

// A scene object's attribute groups: the base of the scene data classes that carry named attributes.
class hkxAttributeHolder : public hkReferencedObject {
public:
    // The SDK's: no groups. The derived classes' native constructors build their base with it.
    hkxAttributeHolder() = default;
    hkxAttributeHolder(const hkxAttributeHolder& other);
    hkxAttributeHolder(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_attributeGroups(aFlag) {}
    ~hkxAttributeHolder() override;

    // The group called apName, or null.
    const hkxAttributeGroup* findAttributeGroupByName(const char* name) const;
    // The object held by the attribute called apName, of class apType when given; or null.
    hkReferencedObject* findAttributeObjectByName(const char* name, const hkClass* type = nullptr) const;
    hkRefVariant findAttributeVariantByName(const char* name) const;

    // Havok SDK signature (hkSceneData.lib, tied by target).
    static const hkClass& staticClass();

    hkArray<hkxAttributeGroup> m_attributeGroups;
};

extern hkClass hkxAttributeHolderClass;
extern hkTypeInfo hkxAttributeHolderTypeInfo;
