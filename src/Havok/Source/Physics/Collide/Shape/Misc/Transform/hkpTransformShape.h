#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Bv/hkpBvShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// A child shape placed under a rigid transform, kept both as the transform and as its rotation quaternion.
class hkpTransformShape : public hkpShape {
public:
	// The reflection's member table (the PDB's hkpTransformShape::Members).
	using MembersArray = const hkClassMember[4];

	hkpTransformShape(const hkpShape* childShape, const hkTransform& transform);

	// The SDK's finish constructor of a loaded object.
	hkpTransformShape(hkFinishLoadedObjectFlag aFlag) : hkpShape(aFlag), m_childShape(aFlag) {
		m_type.m_storage = HK_SHAPE_TRANSFORM;
	}

	~hkpTransformShape() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	float getMaximumProjection(const hkVector4& direction) const override;
	const hkpShapeContainer* getContainer() const override;
	// Havok SDK signature (hkpCollide.lib, tied by bytes,vtable).
	int32_t calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const override;
	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
	void castRayWithCollectorImpl(const hkpShapeRayCastInput& input, const hkpCdBody& cdBody,
		hkpRayHitCollector& collector) const override;

	// The placed shape (the SDK's inline, its m_childShape.getChild() read directly; the exe's out-of-line copy
	// 0x4D8670 is registered in bhkTransformShape.cpp).
	const hkpShape* getChildShape() const { return m_childShape.m_childShape; }
	// Stores arTransform whole and then reads its rotation back into the quaternion member.
	void setTransform(const hkTransform& transform);

	static const hkClass& staticClass();

	hkpSingleShapeContainer m_childShape;
	// mutable as in the SDK: calcSizeForSpu, a const member, stores the child's size here.
	mutable int32_t m_childShapeSize;
	hkQuaternion m_rotation;
	hkTransform m_transform;

	static MembersArray Members;
};

extern hkClass hkpTransformShapeClass;
extern hkTypeInfo hkpTransformShapeTypeInfo;
inline constexpr fnvd::Vtable hkpTransformShape_vftable{};
