#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Reconstruction.hpp"

class hkClass;
class hkTypeInfo;

// The SDK's id of no contact point (an hkContactPointId, an unsigned short).
#define HK_INVALID_CONTACT_POINT (0xffff)

// A point where two bodies touch: its position, and the separating normal whose w holds the distance.
class hkContactPoint {
public:
	// The reflection's member table (the PDB's hkContactPoint::Members, .rdata).
	using MembersArray = const hkClassMember[2];

	// Sets the position and the separating normal, with dist in the normal's w. The exe's out-of-line copy
	// (0x66C9C0) copies both vectors through hkVector4::operator= out of line (0x4A3F10).
	void set(const hkVector4& position, const hkVector4& normal, float dist) {
		m_position = position;
		m_separatingNormal = normal;
		m_separatingNormal(3) = dist;
	}

	// Sets the separating normal's x, y and z to normal's, keeping the distance in w (the exe's out-of-line copy:
	// thiscall, ret 4, 0xCAAD60).
	void setNormal(const hkVector4& normal) { m_separatingNormal.setXYZ(normal); }
	// The PDB's operator=(__that), the member-wise copy: the exe's out-of-line copy 0x6281A0 (thiscall, ret 4;
	// hkVector4::operator= 0x4A3F10 on m_position, then on m_separatingNormal), which hkpRootCdPoint::Assign
	// (0x6212C0), LoadedAreaBound::Create (0x627980) and bhkAabbPhantom::GetAabb (0x6288C0) call. Defaulted, so every
	// src copy of an hkContactPoint stays the implicit one.
	hkContactPoint& operator=(const hkContactPoint& __that) = default;
	// Havok SDK (inline). m_position; the exe calls its shared identity function (0x6815C0), a trivial accessor.
	const hkVector4& getPosition() const { return m_position; }
	// Havok SDK (inline). m_separatingNormal; the exe calls its shared +0x10 getter (0x460140), a trivial accessor.
	const hkVector4& getNormal() const { return m_separatingNormal; }

	// Havok SDK signature (hkBase.lib, tied by target).
	static const hkClass& staticClass();

	hkVector4 m_position;
	hkVector4 m_separatingNormal;

	static MembersArray Members;
};

// The class's reflection record.
extern hkClass hkContactPointClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkContactPointTypeInfo;
