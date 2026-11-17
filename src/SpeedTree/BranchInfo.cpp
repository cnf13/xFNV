#include "SpeedTree/BranchInfo.h"

#include "BSCore/MemoryManager.h"
#include "Reconstruction.hpp"
#include "SpeedTree/EFileToken.h"
#include "SpeedTree/FileAccess.h"
#include "SpeedTree/LibGlobals_Source/IdvGlobals.h"
#include "SpeedTree/LibSpline_Source/IdvSpline.h"

SIdvBranchInfo::SIdvBranchInfo()
	: m_nCrossSectionSegments(6), m_nSegments(3), m_fFirstBranch(0.3f), m_fLastBranch(1.0f), m_fFrequency(0.3f),
	  m_fSTile(1.0f), m_fTTile(1.0f), m_bSTileAbsolute(true), m_bTTileAbsolute(false), m_bRandomTCoordOffset(false),
	  m_fTwist(0.0f), m_fSegmentPackingExponent(1.0f), m_nNumFlares(0), m_fFlareBalance(1.0f),
	  m_fRadialInfluence(30.0f), m_fRadialInfluenceVariance(10.0f), m_fRadialExponent(1.0f), m_fRadialDistance(0.5f),
	  m_fRadialVariance(0.25f), m_fLengthDistance(0.3f), m_fLengthVariance(0.1f), m_fLengthExponent(1.0f),
	  m_pDisturbance(nullptr), m_pGravity(nullptr), m_pFlexibility(nullptr), m_pFlexibilityScale(nullptr),
	  m_pLength(nullptr), m_pRadius(nullptr), m_pRadiusScale(nullptr), m_pStartAngle(nullptr),
	  m_pAngleProfile(nullptr) {
	m_pFlexibility = new CIdvBezierSpline;
	m_pGravity = new CIdvBezierSpline;
	m_pRadius = new CIdvBezierSpline;
	m_pStartAngle = new CIdvBezierSpline;
	m_pRadiusScale = new CIdvBezierSpline;
	m_pLength = new CIdvBezierSpline;
}

// The splines are owned: each is deleted, then the pointers cleared.
SIdvBranchInfo::~SIdvBranchInfo() {
	delete m_pFlexibility;
	delete m_pFlexibilityScale;
	delete m_pGravity;
	delete m_pDisturbance;
	delete m_pStartAngle;
	delete m_pRadius;
	delete m_pRadiusScale;
	delete m_pLength;
	delete m_pAngleProfile;
	m_pFlexibility = nullptr;
	m_pFlexibilityScale = nullptr;
	m_pGravity = nullptr;
	m_pDisturbance = nullptr;
	m_pStartAngle = nullptr;
	m_pRadius = nullptr;
	m_pRadiusScale = nullptr;
	m_pLength = nullptr;
	m_pAngleProfile = nullptr;
}

// The setters delete the old spline as `delete p`: the destructor, then the engine's free.
void SIdvBranchInfo::SetDisturbance(CIdvBezierSpline* pSpline) {
	if (m_pDisturbance != pSpline) {
		CIdvBezierSpline* pOld = m_pDisturbance;
		if (pOld) {
			pOld->~CIdvBezierSpline();
			ourFree(pOld);
		}
		m_pDisturbance = pSpline;
	}
}

void SIdvBranchInfo::SetLength(CIdvBezierSpline* pSpline) {
	if (m_pLength != pSpline) {
		CIdvBezierSpline* pOld = m_pLength;
		if (pOld) {
			pOld->~CIdvBezierSpline();
			ourFree(pOld);
		}
		m_pLength = pSpline;
	}
}

void SIdvBranchInfo::SetAngleProfile(CIdvBezierSpline* pSpline) {
	if (m_pAngleProfile != pSpline) {
		CIdvBezierSpline* pOld = m_pAngleProfile;
		if (pOld) {
			pOld->~CIdvBezierSpline();
			ourFree(pOld);
		}
		m_pAngleProfile = pSpline;
	}
}

void SIdvBranchInfo::SetGravity(CIdvBezierSpline* pSpline) {
	CIdvBezierSpline* block;
	if (m_pGravity != pSpline) {
		block = m_pGravity;
		if (block) {
			block->~CIdvBezierSpline();
			ourFree(block);
		}
		m_pGravity = pSpline;
	}
}

void SIdvBranchInfo::SetFlexibility(CIdvBezierSpline* pSpline) {
	CIdvBezierSpline* block;
	if (m_pFlexibility != pSpline) {
		block = m_pFlexibility;
		if (block) {
			block->~CIdvBezierSpline();
			ourFree(block);
		}
		m_pFlexibility = pSpline;
	}
}

void SIdvBranchInfo::SetFlexibilityScale(CIdvBezierSpline* pSpline) {
	CIdvBezierSpline* block;
	if (m_pFlexibilityScale != pSpline) {
		block = m_pFlexibilityScale;
		if (block) {
			block->~CIdvBezierSpline();
			ourFree(block);
		}
		m_pFlexibilityScale = pSpline;
	}
}

void SIdvBranchInfo::SetRadius(CIdvBezierSpline* pSpline) {
	CIdvBezierSpline* block;
	if (m_pRadius != pSpline) {
		block = m_pRadius;
		if (block) {
			block->~CIdvBezierSpline();
			ourFree(block);
		}
		m_pRadius = pSpline;
	}
}

void SIdvBranchInfo::SetRadiusScale(CIdvBezierSpline* pSpline) {
	CIdvBezierSpline* block;
	if (m_pRadiusScale != pSpline) {
		block = m_pRadiusScale;
		if (block) {
			block->~CIdvBezierSpline();
			ourFree(block);
		}
		m_pRadiusScale = pSpline;
	}
}

void SIdvBranchInfo::SetStartAngle(CIdvBezierSpline* pSpline) {
	CIdvBezierSpline* block;
	if (m_pStartAngle != pSpline) {
		block = m_pStartAngle;
		if (block) {
			block->~CIdvBezierSpline();
			ourFree(block);
		}
		m_pStartAngle = pSpline;
	}
}

// Throws IdvFileError with a C++ throw; the error is caught by LoadTree. The two tile flags are read as
// CTreeFileAccess::ParseBool does: the index taken, then incremented, then the byte tested.
SIdvBranchInfo* SIdvBranchInfo::Parse(CTreeFileAccess& cFile) {
	void* pMemory = ourMalloc(sizeof(SIdvBranchInfo));
	SIdvBranchInfo* pInfo;
	if (pMemory != nullptr) {
		pInfo = ::new (pMemory) SIdvBranchInfo;
	} else {
		pInfo = nullptr;
	}
	int32_t eToken = cFile.ParseToken();
	if (eToken != File_BeginBranchLevel) {
		std::string strDetails;
		FNVD_RECONSTRUCT(std::string)(&strDetails, "malformed branch data");
		throw IdvFileError(strDetails, false);
	}
	eToken = cFile.ParseToken();
	do {
		switch (eToken) {
		case File_Branch_Disturbance:
			pInfo->SetDisturbance(cFile.ParseBranchParameter());
			break;
		case File_Branch_EndAngle:
			pInfo->SetGravity(cFile.ParseBranchParameter());
			break;
		case File_Branch_Flexibility:
			pInfo->SetFlexibility(cFile.ParseBranchParameter());
			break;
		case File_Branch_FlexibilityScale:
			pInfo->SetFlexibilityScale(cFile.ParseBranchParameter());
			break;
		case File_Branch_AngleProfile:
			pInfo->SetAngleProfile(cFile.ParseBranchParameter());
			break;
		case File_Branch_Length:
			pInfo->SetLength(cFile.ParseBranchParameter());
			break;
		case File_Branch_Radius:
			pInfo->SetRadius(cFile.ParseBranchParameter());
			break;
		case File_Branch_RadiusScale:
			pInfo->SetRadiusScale(cFile.ParseBranchParameter());
			break;
		case File_Branch_StartAngle:
			pInfo->SetStartAngle(cFile.ParseBranchParameter());
			break;
		case File_Branch_CrossSectionSegments:
			pInfo->m_nCrossSectionSegments = cFile.ParseToken();
			break;
		case File_Branch_Segments:
			pInfo->m_nSegments = cFile.ParseToken();
			break;
		case File_Branch_FirstBranch:
			pInfo->m_fFirstBranch = cFile.ParseFloat();
			break;
		case File_Branch_LastBranch:
			pInfo->m_fLastBranch = cFile.ParseFloat();
			break;
		case File_Branch_Frequency:
			pInfo->m_fFrequency = cFile.ParseFloat();
			break;
		case File_Branch_STile:
			pInfo->m_fSTile = cFile.ParseFloat();
			break;
		case File_Branch_TTile:
			pInfo->m_fTTile = cFile.ParseFloat();
			break;
		case File_Branch_STileAbsolute: {
			int32_t nIndex = cFile.m_nIndex;
			cFile.m_nIndex = cFile.m_nIndex + 1;
			// SWEEP-INLINE 0xB15170
			pInfo->m_bSTileAbsolute = cFile.m_vMemory[nIndex] != 0;
			break;
		}
		case File_Branch_TTileAbsolute: {
			int32_t nIndex = cFile.m_nIndex;
			cFile.m_nIndex = cFile.m_nIndex + 1;
			// SWEEP-INLINE 0xB15170
			pInfo->m_bTTileAbsolute = cFile.m_vMemory[nIndex] != 0;
			break;
		}
		default: {
			std::string strDetails;
			FNVD_RECONSTRUCT(std::string)(&strDetails, "malformed general branch information");
			throw IdvFileError(strDetails, false);
		}
		}
		eToken = cFile.ParseToken();
	} while (eToken != File_EndBranchLevel);
	return pInfo;
}
