#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Reconstruction.hpp"

struct hkGskOut;
struct hkGskShape;
class hkTransform;

// The heuristic distance of shapes A and B (B in A's space through aTb, cdecl, ret 0, 0xE62740): samples the support
// points along the negated axes, then along four directions tilted ever less around the best normal so far; writes
// the best sample's support point on A to pointAinA, on B (in B's space) to pointBinB, its normal and distance and
// A's point to out. The exe leaves out's address in eax; its one caller (0xE4C230) does not read it.
void hkCvxCvxDistByHeuristicSampling(const hkGskShape* shapeA, const hkGskShape* shapeB, const hkTransform& btoa,
	hkVector4* pointAinAOut, hkVector4* pointBinBOut, hkGskOut& output);

// One sample of hkInternal's heuristic convex-convex distance (the GSK solver's fallback when the penetration depth
// fails: its penetration case, 0xE4C230, calls 0xE62740, which samples through 0xE625C0): the support points of A and
// B along the sample direction and the direction, its w the distance (the PDB's getDistance). The release
// hkinternal.pdb's members.
struct hkCvxCvxDistByHeuristicSamplingSample {
	// The distance, the separating normal's w (inline in the exe).
	float& getDistance() { return m_separatingNormalA(3); }

	hkVector4 m_vertexAinA;
	hkVector4 m_vertexBinA;
	hkVector4 m_vertexBinB;
	hkVector4 m_separatingNormalA;
};
