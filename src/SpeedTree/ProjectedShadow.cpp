#include "SpeedTree/ProjectedShadow.h"

#include "Reconstruction.hpp"
#include "SpeedTree/CIdvFilename.h"
#include "SpeedTree/FileAccess.h"
#include "SpeedTree/FrondEngine.h"
#include "SpeedTree/IndexedGeometry.h"
#include "SpeedTree/LibGlobals_Source/IdvGlobals.h"
#include "SpeedTree/SpeedTreeRT.h"

#include <memory>
#include <string>
#include <vector>

CProjectedShadow::CProjectedShadow() : m_cRight(0.0f, 0.0f, 0.0f), m_cUp(0.0f, 0.0f, 0.0f), m_cOut(0.0f, 0.0f, 0.0f) {}

// Projects cVertex - cStart onto the unit direction from cStart to cEnd.
CVec3 CProjectedShadow::ClosestPoint(const CVec3& cStart, const CVec3& cEnd, const CVec3& cVertex) const {
	CVec3 cResult;
	if (cStart.m_afData[0] == cEnd.m_afData[0] && cStart.m_afData[1] == cEnd.m_afData[1] &&
		cStart.m_afData[2] == cEnd.m_afData[2]) {
		cResult = cStart;
	} else {
		CVec3 cDirection;
		cEnd.Diff(&cDirection, &cStart);
		cDirection.Normalize();
		CVec3 cOffset;
		cVertex.Diff(&cOffset, &cStart);
		float fDistance = cOffset * cDirection;
		CVec3 cScaled;
		cStart.Add(&cResult, cDirection.Scale(&cScaled, fDistance));
	}
	return cResult;
}

void CProjectedShadow::ComputeTexCoords(CIndexedGeometry* pGeometry, CVec3 cCenter, float fRadius,
	float* pShadowCoords) const {
	CVec3 v14;
	CVec3 v15;
	CVec3 v16;
	CVec3 v17;
	// The vertex's (s, t): AddVertexTexCoord1 reads both through one pointer.
	float afTexCoords[2];
	CVec3 cStartS;
	CVec3 cEndS;
	CVec3 cStartT;
	CVec3 cEndT;
	CVec3* a3 = m_cRight.Scale(&v17, fRadius);
	cCenter.Diff(&cStartS, a3);
	CVec3* v6 = m_cRight.Scale(&v16, fRadius);
	cCenter.Add(&cEndS, v6);
	CVec3* v7 = m_cUp.Scale(&v15, fRadius);
	cCenter.Diff(&cStartT, v7);
	CVec3* v8 = m_cUp.Scale(&v14, fRadius);
	cCenter.Add(&cEndT, v8);
	pGeometry->m_nVertexCounter = 0;
	for (uint16_t i = 0; i < static_cast<int32_t>(static_cast<uint16_t>(pGeometry->m_vCoords.size() / 3u)); ++i) {
		CVec3 cVertex;
		const float* pVertex = pGeometry->GetVertexCoord(i);
		float v12 = pVertex[1];
		float v13 = pVertex[2];
		cVertex.m_afData[0] = *pVertex;
		cVertex.m_afData[1] = v12;
		cVertex.m_afData[2] = v13;
		CVec3 cBisectorS = ClosestPoint(cStartS, cEndS, cVertex);
		double v9 = cStartS.Distance(cBisectorS);
		afTexCoords[0] = v9 / (fRadius + fRadius);
		if (pShadowCoords) {
			afTexCoords[0] = afTexCoords[0] * (*pShadowCoords - pShadowCoords[2]) + pShadowCoords[2];
		}
		CVec3 cBisectorT = ClosestPoint(cStartT, cEndT, cVertex);
		double v10 = cStartT.Distance(cBisectorT);
		afTexCoords[1] = v10 / (fRadius + fRadius);
		if (pShadowCoords) {
			afTexCoords[1] = afTexCoords[1] * (pShadowCoords[1] - pShadowCoords[5]) + pShadowCoords[5];
		}
		pGeometry->AddVertexTexCoord1(afTexCoords);
	}
}

// IdvFileError is thrown with a C++ throw; it is caught by LoadTree(pBlock), the caller of ParseShadowProjectionInfo.
// The error text says "frond", as the game does.
void CProjectedShadow::Parse(CTreeFileAccess& cFile) {
	EFileToken eToken = cFile.ParseToken();
	do {
		switch (eToken) {
		case File_ShadowRightVector: {
			float afVector[3];
			float* pVector = cFile.ParseVector3(afVector);
			m_cRight.m_afData[0] = pVector[0];
			m_cRight.m_afData[1] = pVector[1];
			m_cRight.m_afData[2] = pVector[2];
			break;
		}
		case File_ShadowUpVector: {
			float afVector[3];
			float* pVector = cFile.ParseVector3(afVector);
			m_cUp.m_afData[0] = pVector[0];
			m_cUp.m_afData[1] = pVector[1];
			m_cUp.m_afData[2] = pVector[2];
			break;
		}
		case File_ShadowOutVector: {
			float afVector[3];
			float* pVector = cFile.ParseVector3(afVector);
			m_cOut.m_afData[0] = pVector[0];
			m_cOut.m_afData[1] = pVector[1];
			m_cOut.m_afData[2] = pVector[2];
			break;
		}
		case File_ShadowMap: {
			// The parsed string is built in the by-value argument of CIdvFilename's constructor, which frees it.
			CIdvFilename cFilename;
			FNVD_RECONSTRUCT(CIdvFilename)(&cFilename, cFile.ParseString());
			CIdvFilename cNoPath;
			m_strSelfShadowMap.assign(*cFilename.NoPath(&cNoPath), 0, std::string::npos);
			break;
		}
		default: {
			std::string strDetails = IdvFormatString("malformed frond info (token %d)", static_cast<int32_t>(eToken));
			throw IdvFileError(strDetails, false);
		}
		}
		eToken = cFile.ParseToken();
	} while (eToken != File_EndShadowProjectionInfo);
}
