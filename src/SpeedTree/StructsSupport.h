#pragma once

#include "Reconstruction.hpp"
#include "SpeedTree/CIdvFilename.h"
#include "SpeedTree/SIdvBranchVertex.h"
#include "SpeedTree/SpeedTreeRT.h"

#include <__stddef_size_t.h>
#include <memory>
#include <vector>

// One leaf texture of a tree: whether it is a blossom, its colour and variance, file, origin, size and used size.
// speedtree-2's StructsSupport.h; SIdvBranchVertex is split out into SIdvBranchVertex.h and SIdvBranch into Branch.h.
struct SIdvLeafTexture {
	SIdvLeafTexture();
	SIdvLeafTexture(const SIdvLeafTexture& arOther);
	~SIdvLeafTexture();

	// The copy assignment: the fields in order, the file name's
	// string assignment among them. Declared as Assign, not operator=, so that src's copies of the type stay trivial.
	// Defined in TreeEngine.cpp.
	SIdvLeafTexture& Assign(const SIdvLeafTexture& arOther);

	bool m_bBlossom;
	CVec3 m_cColor;
	float m_fColorVariance;
	CIdvFilename m_strFilename;
	CVec3 m_cOrigin;
	CVec3 m_cSize;
	CVec3 m_cSizeUsed;
};

// One flare of a branch's trunk: the angle, the radial and length influences and exponents, and the distance.
struct SIdvBranchFlare {
	// Reads the flare only; const so that the const branch's checked operator[] can call it.
	float Distance(float fInputAngle, float fInputProgress) const;

	float m_fAngle;
	float m_fRadialInfluence;
	float m_fRadialExponent;
	float m_fLengthInfluence;
	float m_fLengthExponent;
	float m_fDistance;
};
