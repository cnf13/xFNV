#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
// The native finish constructor destroys m_shape on unwinding through ~hkRefPtr, which releases the shape.
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Reconstruction.hpp"

// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
const void* getVtablehkpShapeInfo();

// A serialized shape with the names and transforms of its child shapes (the SDK's shape info for tools).
// IDA places m_transform at 0x28; the exe's reflection data (hkpShapeInfoClass, members at 0x10E8790: transform at
// 0x30, object size 112) and the PC PDB put it at 0x30, where hkTransform's 16-byte alignment takes it.
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
inline constexpr fnvd::Vtable hkpShapeInfo_vftable{};
