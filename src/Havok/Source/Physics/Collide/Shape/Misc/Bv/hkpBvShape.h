#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkAabb;
class hkStatisticsCollector;
class hkpRayHitCollector;
struct hkpShapeRayCastInput;

// A shape container that holds exactly one child shape (the PDB places it in the hkpBvShape sources).
class hkpSingleShapeContainer : public hkpShapeContainer {
public:
	// The reflection's member table (the PDB's hkpSingleShapeContainer::Members).
	using MembersArray = const hkClassMember[1];

	// The SDK's inline constructors: the exe inlines them into its holders' constructors.
	hkpSingleShapeContainer(const hkpShape* s, ReferencePolicy ref = REFERENCE_POLICY_INCREMENT);
	hkpSingleShapeContainer(hkFinishLoadedObjectFlag);
	~hkpSingleShapeContainer() override;

	int32_t getNumChildShapes() const override;
	uint32_t getFirstKey() const override;
	// Havok SDK signature (hkpCollide.lib, tied by vtable).
	uint32_t getNextKey(uint32_t oldKey) const override;
	const hkpShape* getChildShape(uint32_t, char (&)[512]) const override;

	// Havok SDK signature (hkpCollide.lib, tied by target).
	static const hkClass& staticClass();

	const hkpShape* m_childShape;

	static MembersArray Members;
};

// A shape that wraps a child shape in a bounding volume shape (the exe's collide shape that tests the bounding
// volume first).
class hkpBvShape : public hkpShape {
public:
	hkpBvShape(const hkpShape* boundingVolumeShape, const hkpShape* childShape);

	// The SDK's finish constructor of a loaded object.
	hkpBvShape(hkFinishLoadedObjectFlag aFlag) : hkpShape(aFlag), m_childShape(aFlag) {
		m_type.m_storage = HK_SHAPE_BV;
	}

	~hkpBvShape() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	const hkpShapeContainer* getContainer() const override;
	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
	void castRayWithCollectorImpl(const hkpShapeRayCastInput& input, const hkpCdBody& cdBody,
		hkpRayHitCollector& collector) const override;

	// Havok SDK signature (hkpCollide.lib, tied by target).
	static const hkClass& staticClass();

	const hkpShape* m_boundingVolumeShape;
	hkpSingleShapeContainer m_childShape;
};

inline constexpr fnvd::Vtable hkpSingleShapeContainer_vftable{};
inline constexpr fnvd::Vtable hkpBvShape_vftable{};
extern hkClass hkpSingleShapeContainerClass;
extern hkTypeInfo hkpSingleShapeContainerTypeInfo;
extern hkClass hkpBvShapeClass;
extern hkTypeInfo hkpBvShapeTypeInfo;
