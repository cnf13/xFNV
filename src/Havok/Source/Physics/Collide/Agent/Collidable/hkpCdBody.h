#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Reconstruction.hpp"

class hkClass;
class hkMotionState;
class hkTransform;
class hkTypeInfo;
class hkpCollidable;
class hkpShape;

// The motion of a body in a collision hierarchy: a plain transform for a static body, a motion state for a moving
// one.
union HavokMotionTransform {
	hkMotionState* pState;
	hkTransform* pMotion;
};

// One level of the collision hierarchy: a shape, the transform it is placed by, and the level above it.
class hkpCdBody {
public:
	// The reflection's member table (the PDB's hkpCdBody::Members, .rdata).
	using MembersArray = const hkClassMember[4];

	// m_shape. The exe calls its shared one-field getter, NiPointer<NiRefObject>::operator-> (0x559450), a trivial
	// accessor.
	const hkpShape* getShape() const { return m_shape; }
	// The collidable at the top of the hierarchy: the body whose m_parent is null (Havok SDK, hkpCdBody.inl). The
	// exe's out-of-line copy is 0x624020.
	const hkpCollidable* getRootCollidable() const;

	// Havok SDK signature (hkpCollide.lib, tied by target).
	static const hkClass& staticClass();

	const hkpShape* m_shape;
	uint32_t m_shapeKey;
	HavokMotionTransform m_motion;
	const hkpCdBody* m_parent;

	static MembersArray Members;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpCdBodyTypeInfo;
