#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/Mesh/hkpMeshShape.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// A mesh shape whose child shapes are built from the first subpart alone, with 16-bit triangle indices and the shape
// key as the triangle index (getChildShape, 0xE48CB0). It adds no fields; getChildShape is its only override (the
// primary vtable equals hkpMeshShape's, the destructor included). Size from the PC PDB.
class hkpFastMeshShape : public hkpMeshShape {
public:
	hkpFastMeshShape(hkFinishLoadedObjectFlag aFlag) : hkpMeshShape(aFlag) {
		if (aFlag.m_finishing) {
			m_type.m_storage = HK_SHAPE_TRIANGLE_COLLECTION;
		}
	}

	hkpFastMeshShape(float radius, int32_t numBitsForSubpartIndex) : hkpMeshShape(radius, numBitsForSubpartIndex) {}

	// hkpShapeContainer's vtable at +0x10, slot 5.
	const hkpShape* getChildShape(uint32_t key, char (&buffer)[512]) const override;

	static const hkClass& staticClass();
};

extern hkClass hkpFastMeshShapeClass;
extern hkTypeInfo hkpFastMeshShapeTypeInfo;
inline constexpr fnvd::Vtable hkpFastMeshShape_vftable{};
inline constexpr fnvd::Vtable hkpFastMeshShape_vftable_hkpShapeContainer{};
