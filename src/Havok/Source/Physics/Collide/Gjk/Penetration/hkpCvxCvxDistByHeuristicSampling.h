#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Reconstruction.hpp"

// One sample of the distance search between two convex shapes by sampling directions: the support vertices of A and
// B and the separating normal in A's space (hkpinternal.pdb, sizeof 0x30; the game PDB's source
// hkpCvxCvxDistByHeuristicSampling.cpp; the SDK ships no header for it).
struct hkpCvxCvxDistByHeuristicSamplingSample {
	hkpCdVertex m_vertexAinA;
	hkpCdVertex m_vertexBinB;
	hkVector4 m_separatingNormalA;
};
