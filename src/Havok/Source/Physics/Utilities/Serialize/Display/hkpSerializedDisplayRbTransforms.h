#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Reconstruction.hpp"

class hkpRigidBody;

// Havok SDK signature (hkpUtilities.lib, tied by reloc,target).
const void* getVtablehkpSerializedDisplayRbTransforms();

// Owns the serialized display transforms associated with rigid bodies.
class hkpSerializedDisplayRbTransforms : public hkReferencedObject {
public:
	// A rigid body and the transform from its local space to its display's (the PDB's members; the exe's hkClass
	// hkpSerializedDisplayRbTransformsDisplayTransformPairClass has rb at 0x0, localToDisplay at 0x10 and an object
	// size of 0x50).
	struct DisplayTransformPair {
		// The reflection class hkpSerializedDisplayRbTransformsDisplayTransformPairClass.
		static const hkClass& staticClass();

		hkpRigidBody* m_rb;
		hkTransform m_localToDisplay;
	};

	hkpSerializedDisplayRbTransforms(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_transforms(aFlag) {}
	~hkpSerializedDisplayRbTransforms() override;

	// The reflection class hkpSerializedDisplayRbTransformsClass.
	static const hkClass& staticClass();

	hkArray<DisplayTransformPair, hkContainerHeapAllocator> m_transforms;
};

inline constexpr fnvd::Vtable hkpSerializedDisplayRbTransforms_vftable{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpSerializedDisplayRbTransformsDisplayTransformPairTypeInfo;
extern hkClass hkpSerializedDisplayRbTransformsClass;
extern hkTypeInfo hkpSerializedDisplayRbTransformsTypeInfo;
extern hkClass hkpSerializedDisplayRbTransformsDisplayTransformPairClass;
