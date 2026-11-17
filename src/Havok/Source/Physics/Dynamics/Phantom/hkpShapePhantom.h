#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Reconstruction.hpp"

class hkpCdBodyPairCollector;
class hkpCdPointCollector;
class hkpCollisionInput;
class hkpLinearCastInput;

void sub_C9E3E0();

// A phantom with a shape and a motion state of its own: it can be moved, swept along a path to find what the shape
// hits, and asked which bodies it touches or penetrates.
class hkpShapePhantom : public hkpPhantom {
public:
	hkpShapePhantom(hkpShape* a1, hkTransform* a2);
	// The SDK's finish constructor, which the exe inlines into its derived classes' finish constructors.
	hkpShapePhantom(hkFinishLoadedObjectFlag flag) : hkpPhantom(flag) {}

	hkWorldOperation::Result setShape(const hkpShape* shape) override;
	hkMotionState* getMotionState() override;
	void calcAabb(hkAabb& aAabb) override;
	void deallocateInternalArrays() override;
	virtual void setPositionAndLinearCast(const hkVector4&, const hkpLinearCastInput&, hkpCdPointCollector&,
		hkpCdPointCollector*) = 0;
	virtual void setTransformAndLinearCast(const hkTransform&, const hkpLinearCastInput&, hkpCdPointCollector&,
		hkpCdPointCollector*) = 0;
	virtual void getClosestPoints(hkpCdPointCollector&, const hkpCollisionInput*) = 0;
	virtual void getPenetrations(hkpCdBodyPairCollector&, const hkpCollisionInput*) = 0;

	// Moves the phantom to aPosition; extraTolerance widens its AABB.
	void setPosition(const hkVector4& position, float extraTolerance);
	// Moves the phantom to transform.
	void setTransform(const hkTransform& transform);
	void firePhantomShapeSet();

	// The PDB has m_motionState at 0xB0, 16-byte aligned (getMotionState 0xC9E810 returns this+0xB0 and the derived
	// constructors write from 0x160), so the 12 bytes after the base class are the compiler's alignment padding;
	// the class is 0x160. IDA places m_motionState at 0xA8 and sizes the class 0x158.
	alignas(16) hkMotionState m_motionState;
};
inline constexpr fnvd::Vtable hkpShapePhantom_vftable{};

// The class's reflection record.
extern hkClass hkpShapePhantomClass;
