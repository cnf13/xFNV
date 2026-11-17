#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexTransformShapeBase.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// A convex child shape placed under a transform with scale, plus an extra scale vector.
class hkpConvexTransformShape : public hkpConvexTransformShapeBase {
public:
	// The constructor, run in place on apMemory with the child shape and the transform (thiscall, ret 0xC,
	// 0xD5D7A0). The PDB's parameter is an hkTransform, which every exe caller passes (0xD55CE0 builds one with
	// hkRotation::set); the exe copies its four columns into m_transform.
	hkpConvexTransformShape(hkpConvexShape* apChildShape, const hkTransform& arTransform,
		hkpShapeContainer::ReferencePolicy aeRef);

	// Assumes hkpConvexTransformShapeBase's finish constructor (the SDK's).
	hkpConvexTransformShape(hkFinishLoadedObjectFlag aFlag) : hkpConvexTransformShapeBase(aFlag) {
		m_type.m_storage = HK_SHAPE_CONVEX_TRANSFORM;
	}

	// The exe's slot 0 is the base's deleting destructor (0xD5E070, folded by the linker), so hdrgen's vtable check
	// sees it as the base's; the PDB lists this class's own and C++ gives every class its own destructor slot.
	~hkpConvexTransformShape() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	float getMaximumProjection(const hkVector4& direction) const override;
	const hkpShapeContainer* getContainer() const override;
	// Havok SDK signature (hkpCollide.lib, tied by bytes,vtable).
	int32_t calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const override;
	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
	void castRayWithCollectorImpl(const hkpShapeRayCastInput& input, const hkpCdBody& cdBody,
		hkpRayHitCollector& collector) const override;
	int32_t getNumCollisionSpheresImpl() const override;
	const hkSphere* getCollisionSpheresImpl(hkSphere* sphereBuffer) const override;
	void getFirstVertex(hkVector4& aVertexOut) const override;
	void getSupportingVertexImpl(const hkVector4& direction, hkpCdVertex& supportingVertexOut) const override;
	void convertVertexIdsToVerticesImpl(const uint16_t* ids, int32_t numIds, hkpCdVertex* verticesOut) const override;
	void getCentreImpl(hkVector4& centreOut) const override;

	void setTransform(const hkTransform& transform);

	// The SDK's inline getChildShape: the child, a convex shape.
	const hkpConvexShape* getChildShape() const {
		return static_cast<const hkpConvexShape*>(m_childShape.m_childShape);
	}

	static const hkClass& staticClass();

	// The PDB's and the SDK's hkTransform: the exe reads the rotation's three columns at +0x20 and the translation at
	// +0x50, and passes &m_transform to hkTransform::setMul (0xCB2240) and hkVector4::setRotatedDir (0xC7F700).
	hkTransform m_transform;
};
inline constexpr fnvd::Vtable hkpConvexTransformShape_vftable{};

// The class's reflection record and type info (hkpConvexTransformShape.cpp).
extern hkClass hkpConvexTransformShapeClass;
extern hkTypeInfo hkpConvexTransformShapeTypeInfo;
