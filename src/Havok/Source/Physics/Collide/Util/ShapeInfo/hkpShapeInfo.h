#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
const void* getVtablehkpShapeInfo();

// A serialized shape with the names and transforms of its child shapes. m_transform sits where hkTransform's 16-byte
// alignment takes it.
class hkpShapeInfo : public hkReferencedObject {
public:
    // The SDK's finish constructor of a loaded object.
    hkpShapeInfo(hkFinishLoadedObjectFlag aFlag)
      : hkReferencedObject(aFlag), m_shape(aFlag), m_childShapeNames(aFlag), m_childTransforms(aFlag) {}

    ~hkpShapeInfo() override;

    // Havok SDK signature (hkpCollide.lib, tied by target).
    static const hkClass& staticClass();

    hkRefPtr<const hkpShape> m_shape;
    hkBool m_isHierarchicalCompound;
    hkBool m_hkdShapesCollected;
    hkArray<hkStringPtr, hkContainerHeapAllocator> m_childShapeNames;
    hkArray<hkTransform, hkContainerHeapAllocator> m_childTransforms;
    hkTransform m_transform;
};
