#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Reconstruction.hpp"

// What a character proxy takes from a surface it touches: the plane, its velocity and the friction limits. The PDB
// names its functions in BSHavok/bhkCharacterController.cpp. m_priority is the last member; the 12 bytes after it are
// the hkVector4 alignment, which gives the release PDB's sizeof 64 (hkputilities.pdb, hkpSurfaceConstraintInfo).
class hkpSurfaceConstraintInfo {
public:
	// Not named in the PDB or IDA; name chosen here. The exe's out-of-line copy assignment (the PDB's
	// compiler-generated operator=(const hkpSurfaceConstraintInfo&)): a member-wise copy of the two vectors and
	// five words, returning this (thiscall, ret 4, 0xC6D670). Declared as a static so that src's `a = b` sites keep
	// the implicit copy assignment.
	static hkpSurfaceConstraintInfo* Assign(hkpSurfaceConstraintInfo* apThis, const hkpSurfaceConstraintInfo& arOther);

	hkVector4 m_plane;
	hkVector4 m_velocity;
	float m_staticFriction;
	float m_extraUpStaticFriction;
	float m_extraDownStaticFriction;
	float m_dynamicFriction;
	int32_t m_priority;
};
