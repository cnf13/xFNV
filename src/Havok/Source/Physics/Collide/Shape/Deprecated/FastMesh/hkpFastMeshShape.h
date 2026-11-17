#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/Mesh/hkpMeshShape.h"

#include <stdint.h>

// A mesh shape whose child shapes are built from the first subpart alone, with 16-bit triangle indices and the shape
// key as the triangle index (getChildShape). It adds no fields; getChildShape is its only override.
class hkpFastMeshShape : public hkpMeshShape {
public:
	hkpFastMeshShape(hkFinishLoadedObjectFlag aFlag) : hkpMeshShape(aFlag) {
		if (aFlag.m_finishing) {
			m_type.m_storage = HK_SHAPE_TRIANGLE_COLLECTION;
		}
	}

	hkpFastMeshShape(float radius, int32_t numBitsForSubpartIndex) : hkpMeshShape(radius, numBitsForSubpartIndex) {}

	const hkpShape* getChildShape(uint32_t key, char (&buffer)[512]) const override;

	static const hkClass& staticClass();
};

extern hkClass hkpFastMeshShapeClass;
extern hkTypeInfo hkpFastMeshShapeTypeInfo;
