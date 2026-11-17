#pragma once

#include "Reconstruction.hpp"
#include "SpeedTree/SpeedTreeRT.h"

// The shadow a tree projects on the ground, computed from its geometry. Only the members src calls are declared.
class CProjectedShadow {
public:
	CProjectedShadow();

protected:
	// The point of the line through cStart and cEnd closest to cVertex; cStart when the two ends are equal.
	// The PDB makes it private.
	CVec3 ClosestPoint(const CVec3& cStart, const CVec3& cEnd, const CVec3& cVertex) const;

public:
	void ComputeTexCoords(CIndexedGeometry* pGeometry, CVec3 cCenter, float fRadius, float* pShadowCoords) const;
	// Reads the axes and the self-shadow map name from cFile.
	void Parse(CTreeFileAccess& cFile);

	// The shadow's projection axes and the file name of the self-shadow map.
	CVec3 m_cRight;
	CVec3 m_cUp;
	CVec3 m_cOut;
	std::string m_strSelfShadowMap;
};
