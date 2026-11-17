#include "SpeedTree/SpeedTreeRT.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/FileFinder.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/iosfwd.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "SpeedTree/BillboardLeaf.h"
#include "SpeedTree/Branch.h"
#include "SpeedTree/CIdvFilename.h"
#include "SpeedTree/CRotTransform.h"
#include "SpeedTree/Camera.h"
#include "SpeedTree/EFileToken.h"
#include "SpeedTree/FileAccess.h"
#include "SpeedTree/FrondEngine.h"
#include "SpeedTree/IndexedGeometry.h"
#include "SpeedTree/Instances.h"
#include "SpeedTree/LeafGeometry.h"
#include "SpeedTree/LibGlobals_Source/IdvGlobals.h"
#include "SpeedTree/LibSpline_Source/IdvSpline.h"
#include "SpeedTree/LibVector_Source/IdvVector.h"
#include "SpeedTree/LightingEngine.h"
#include "SpeedTree/ProjectedShadow.h"
#include "SpeedTree/SimpleBillboard.h"
#include "SpeedTree/TreeEngine.h"
#include "SpeedTree/WindEngine.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

SInstanceList::TreeVector SInstanceList::m_vUniqueTrees;
BSCriticalSection SInstanceList::m_UniqueTreesCritSection;
std::string g_strError;
static std::string g_strKey;

// The shapes' vector under one name, for the registration: a macro argument can't hold the template's commas.
using SShapeVector = std::vector<SShape, std::allocator<SShape>>;

SEmbeddedTexCoords::SEmbeddedTexCoords()
	: m_nNumLeafMaps(0), m_pLeafTexCoords(nullptr), m_nNumBillboards(0), m_pBillboardTexCoords(nullptr),
	  m_nNumFrondMaps(0), m_pFrondTexCoords(nullptr),
	  m_afShadowTexCoords{1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f} {}

SEmbeddedTexCoords::~SEmbeddedTexCoords() {
	MemoryManager_Singleton.Deallocate(m_pLeafTexCoords);
	m_pLeafTexCoords = nullptr;
	MemoryManager_Singleton.Deallocate(m_pBillboardTexCoords);
	m_pBillboardTexCoords = nullptr;
	MemoryManager_Singleton.Deallocate(m_pFrondTexCoords);
	m_pFrondTexCoords = nullptr;
}

CTransform* CTransform::B3CCD0() {
	m_afData[0][0] = (float)1.0;
	m_afData[0][1] = (float)0.0;
	m_afData[0][2] = (float)0.0;
	m_afData[0][3] = (float)0.0;
	m_afData[1][0] = (float)0.0;
	m_afData[1][1] = (float)1.0;
	m_afData[1][2] = (float)0.0;
	m_afData[1][3] = (float)0.0;
	m_afData[2][0] = (float)0.0;
	m_afData[2][1] = (float)0.0;
	m_afData[2][2] = (float)1.0;
	m_afData[2][3] = (float)0.0;
	m_afData[3][0] = (float)0.0;
	m_afData[3][1] = (float)0.0;
	m_afData[3][2] = (float)0.0;
	m_afData[3][3] = (float)1.0;
	return this;
}

CTransform* CTransform::B3CCB0() {
	B3CCD0();
	return this;
}

CVec3::CVec3(float fX, float fY, float fZ) : m_afData{fX, fY, fZ} {}

// The squared distance is summed on the x87 stack (kept as double here), z first, and stored as a float; the root
// is the fast approximation that halves the float's bits and adds back half the exponent bias.
float CVec3::Distance(const CVec3& cVec) const {
	double dZ = (double)cVec.m_afData[2] - m_afData[2];
	double dY = (double)cVec.m_afData[1] - m_afData[1];
	double dX = (double)cVec.m_afData[0] - m_afData[0];
	float fSquared = (float)(dZ * dZ + (dY * dY + dX * dX));
	int32_t nBits = (__builtin_bit_cast(int32_t, fSquared) >> 1) + 0x1FC00000;
	return __builtin_bit_cast(float, nBits);
}

float CVec3::operator*(const CVec3& cVec) const {
	return (float)(((m_afData[0] * (&cVec)->m_afData[0]) + (m_afData[1] * (&cVec)->m_afData[1])) +
		(m_afData[2] * (&cVec)->m_afData[2]));
}

CVec3 CVec3::operator*(const CTransform& cTransform) const {
	CVec3 a2;
	CVec3 v4;
	v4.m_afData[0] = cTransform.m_afData[3][0] +
		(cTransform.m_afData[2][0] * m_afData[2] +
			(cTransform.m_afData[1][0] * m_afData[1] + cTransform.m_afData[0][0] * m_afData[0]));
	v4.m_afData[1] = cTransform.m_afData[3][1] +
		(cTransform.m_afData[2][1] * m_afData[2] +
			(cTransform.m_afData[1][1] * m_afData[1] + cTransform.m_afData[0][1] * m_afData[0]));
	v4.m_afData[2] = cTransform.m_afData[3][2] +
		(cTransform.m_afData[2][2] * m_afData[2] +
			(cTransform.m_afData[1][2] * m_afData[1] + cTransform.m_afData[0][2] * m_afData[0]));
	a2 = v4;
	return a2;
}

CVec3 CVec3::operator*(const CRotTransform& cTransform) const {
	CVec3 out;
	CVec3 v4;
	v4.m_afData[0] = cTransform.m_afData[2][0] * m_afData[2] +
		(cTransform.m_afData[1][0] * m_afData[1] + cTransform.m_afData[0][0] * m_afData[0]);
	v4.m_afData[1] = cTransform.m_afData[2][1] * m_afData[2] +
		(cTransform.m_afData[1][1] * m_afData[1] + cTransform.m_afData[0][1] * m_afData[0]);
	v4.m_afData[2] = cTransform.m_afData[2][2] * m_afData[2] +
		(cTransform.m_afData[1][2] * m_afData[1] + cTransform.m_afData[0][2] * m_afData[0]);
	out = v4;
	return out;
}

CVec3* CVec3::Scale(CVec3* a2, float a3) const {
	CVec3 v4;
	v4.m_afData[0] = (float)(a3 * m_afData[0]);
	v4.m_afData[1] = (float)(a3 * m_afData[1]);
	v4.m_afData[2] = (float)(a3 * m_afData[2]);
	*a2 = v4;
	return a2;
}

CVec3* CVec3::Diff(CVec3* a2, const CVec3* a3) const {
	CVec3 v4;
	v4.m_afData[0] = (float)(m_afData[0] - a3->m_afData[0]);
	v4.m_afData[1] = (float)(m_afData[1] - a3->m_afData[1]);
	v4.m_afData[2] = (float)(m_afData[2] - a3->m_afData[2]);
	*a2 = v4;
	return a2;
}

// The sums run on the x87 stack and are stored once as floats.
CVec3* CVec3::Add(CVec3* apResult, const CVec3* apOther) const {
	CVec3 cSum;
	cSum.m_afData[0] = apOther->m_afData[0] + m_afData[0];
	cSum.m_afData[1] = apOther->m_afData[1] + m_afData[1];
	cSum.m_afData[2] = apOther->m_afData[2] + m_afData[2];
	*apResult = cSum;
	return apResult;
}

CVec3 CVec3::Cross(const CVec3& cVec) const {
	CVec3 result;
	CVec3 v4;
	v4.m_afData[0] = cVec.m_afData[2] * m_afData[1] - cVec.m_afData[1] * m_afData[2];
	v4.m_afData[1] = cVec.m_afData[0] * m_afData[2] - cVec.m_afData[2] * m_afData[0];
	v4.m_afData[2] = cVec.m_afData[1] * m_afData[0] - cVec.m_afData[0] * m_afData[1];
	result = v4;
	return result;
}

void CVec3::Set(float fX, float fY, float fZ) {
	m_afData[0] = fX;
	m_afData[1] = fY;
	m_afData[2] = fZ;
}

CVec3& CVec3::operator+=(const CVec3& cVec) {
	m_afData[0] = m_afData[0] + cVec.m_afData[0];
	m_afData[1] = m_afData[1] + cVec.m_afData[1];
	m_afData[2] = m_afData[2] + cVec.m_afData[2];
	return *this;
}

CSpeedTreeRT::SGeometry::SIndexed::SIndexed()
	: m_nDiscreteLodLevel(-1), m_usNumStrips(0), m_pStripLengths(nullptr), m_pStrips(nullptr), m_usVertexCount(0),
	  m_pColors(nullptr), m_pNormals(nullptr), m_pBinormals(nullptr), m_pTangents(nullptr), m_pCoords(nullptr),
	  m_pTexCoords0(nullptr), m_pTexCoords1(nullptr), m_pWindWeights(nullptr), m_pWindMatrixIndices(nullptr) {}

// The arrays belong to the engines that filled them: only the pointers are cleared.
CSpeedTreeRT::SGeometry::SIndexed::~SIndexed() {
	m_pColors = nullptr;
	m_pNormals = nullptr;
	m_pBinormals = nullptr;
	m_pTangents = nullptr;
	m_pCoords = nullptr;
	m_pTexCoords0 = nullptr;
	m_pTexCoords1 = nullptr;
	m_pWindWeights = nullptr;
	m_pWindMatrixIndices = nullptr;
	m_pStripLengths = nullptr;
	m_pStrips = nullptr;
}

// The geometry's arrays belong to the engines that filled them: only the pointers are cleared.
CSpeedTreeRT::SGeometry::SLeaf::~SLeaf() {
	m_pLeafMapIndices = nullptr;
	m_pLeafClusterIndices = nullptr;
	m_pCenterCoords = nullptr;
	m_pColors = nullptr;
	m_pNormals = nullptr;
	m_pBinormals = nullptr;
	m_pTangents = nullptr;
	m_pLeafMapTexCoords = nullptr;
	m_pLeafMapCoords = nullptr;
	m_pWindWeights = nullptr;
	m_pWindMatrixIndices = nullptr;
}

CSpeedTreeRT::SGeometry::SLeaf* CSpeedTreeRT::SGeometry::SLeaf::Init() {
	m_bIsActive = 0;
	m_fAlphaTestValue = -1.0f;
	m_nDiscreteLodLevel = -1;
	m_usLeafCount = 0;
	m_pLeafMapIndices = 0;
	m_pLeafClusterIndices = 0;
	m_pCenterCoords = 0;
	m_pLeafMapTexCoords = 0;
	m_pLeafMapCoords = 0;
	m_pColors = 0;
	m_pNormals = 0;
	m_pBinormals = 0;
	m_pTangents = 0;
	m_pWindWeights = 0;
	m_pWindMatrixIndices = 0;
	return this;
}

CSpeedTreeRT::SGeometry::SBillboard::SBillboard()
	: m_bIsActive(false), m_pTexCoords(nullptr), m_pCoords(nullptr), m_fAlphaTestValue(-1.0f), m_usWhich(0) {}

CSpeedTreeRT::SGeometry::SBillboard::~SBillboard() {
	m_pTexCoords = nullptr;
	m_pCoords = nullptr;
}

// The exe's cleanup-only C++ EH frame is left out.
CSpeedTreeRT::SGeometry* CSpeedTreeRT::SGeometry::Init_00B029F0() {
	m_sBranches.~SIndexed();
	::new (&m_sBranches) CSpeedTreeRT::SGeometry::SIndexed;
	m_fBranchAlphaTestValue = -1.0f;
	m_sFronds.~SIndexed();
	::new (&m_sFronds) CSpeedTreeRT::SGeometry::SIndexed;
	m_fFrondAlphaTestValue = -1.0f;
	m_sLeaves0.Init();
	m_sLeaves1.Init();
	m_sBillboard0.~SBillboard();
	::new (&m_sBillboard0) CSpeedTreeRT::SGeometry::SBillboard;
	m_sBillboard1.~SBillboard();
	::new (&m_sBillboard1) CSpeedTreeRT::SGeometry::SBillboard;
	m_sHorizontalBillboard.~SBillboard();
	::new (&m_sHorizontalBillboard) CSpeedTreeRT::SGeometry::SBillboard;
	return this;
}

// The exe's cleanup-only C++ EH frame is left out.
void CSpeedTreeRT::SGeometry::Destroy() {}

CSpeedTreeRT::STextures::STextures()
	: m_pBranchTextureFilename(nullptr), m_uiLeafTextureCount(0), m_pLeafTextureFilenames(nullptr),
	  m_uiFrondTextureCount(0), m_pFrondTextureFilenames(nullptr), m_pCompositeFilename(nullptr),
	  m_pSelfShadowFilename(nullptr) {}

// GetTextures allocates the two name arrays; the names themselves belong to the tree.
CSpeedTreeRT::STextures::~STextures() {
	MemoryManager_Singleton.Deallocate(m_pLeafTextureFilenames);
	m_pLeafTextureFilenames = nullptr;
	MemoryManager_Singleton.Deallocate(m_pFrondTextureFilenames);
	m_pFrondTextureFilenames = nullptr;
	m_pBranchTextureFilename = nullptr;
	m_pCompositeFilename = nullptr;
	m_pSelfShadowFilename = nullptr;
}

// The handlers report through SetError as the hybrid body's do; only src throws reach them here.
CSpeedTreeRT::CSpeedTreeRT()
	: m_pEngine(nullptr), m_pBranchGeometry(nullptr), m_pLeafGeometry(nullptr), m_pLightingEngine(nullptr),
	  m_pWindEngine(nullptr), m_pSimpleBillboard(nullptr), m_eLeafLodMethod(LOD_SMOOTH),
	  m_fLeafLodTransitionRadius(0.07f), m_fLeafLodCurveExponent(0.7f), m_fLeafSizeIncreaseFactor(0.5f),
	  m_fLeafTransitionFactor(0.0f), m_pLeafLodSizeFactors(nullptr), m_pInstanceRefCount(nullptr),
	  m_pInstanceData(nullptr), m_nFrondLevel(-1), m_pTreeSizes(nullptr), m_ucTargetAlphaValue(0x54),
	  m_bTreeComputed(false), m_nBranchWindLevel(-1), m_pEmbeddedTexCoords(nullptr), m_pProjectedShadow(nullptr),
	  m_usNum360Billboards(0), m_pCollisionObjects(nullptr), m_pFrondEngine(nullptr), m_usNumFrondLodLevels(0),
	  m_pUserData(nullptr), m_b360Billboard(false), m_bHorizontalBillboard(false) {
	try {
		void* pMemory = MemoryManager_Singleton.Allocate(sizeof(CWindEngine));
		m_pWindEngine = pMemory ? ::new (pMemory) CWindEngine() : nullptr;
		pMemory = MemoryManager_Singleton.Allocate(sizeof(CIndexedGeometry));
		m_pBranchGeometry = pMemory ? ::new (pMemory) CIndexedGeometry(m_pWindEngine, false) : nullptr;
		pMemory = MemoryManager_Singleton.Allocate(sizeof(CTreeEngine));
		m_pEngine = pMemory ? ::new (pMemory) CTreeEngine(m_pBranchGeometry) : nullptr;
		pMemory = MemoryManager_Singleton.Allocate(sizeof(CLightingEngine));
		m_pLightingEngine = pMemory ? ::new (pMemory) CLightingEngine() : nullptr;
		pMemory = MemoryManager_Singleton.Allocate(sizeof(CLeafGeometry));
		m_pLeafGeometry = pMemory ? ::new (pMemory) CLeafGeometry(m_pWindEngine) : nullptr;
		pMemory = MemoryManager_Singleton.Allocate(sizeof(CSimpleBillboard));
		m_pSimpleBillboard = pMemory ? ::new (pMemory) CSimpleBillboard() : nullptr;
		m_pTreeSizes = static_cast<float*>(MemoryManager_Singleton.Allocate(STS_COUNT * sizeof(float)));
		m_pInstanceRefCount = static_cast<uint32_t*>(MemoryManager_Singleton.Allocate(sizeof(uint32_t)));
		*m_pInstanceRefCount = 1;
		pMemory = MemoryManager_Singleton.Allocate(sizeof(SInstanceList));
		m_pInstanceList = pMemory ? ::new (pMemory) SInstanceList() : nullptr;
		pMemory = MemoryManager_Singleton.Allocate(sizeof(CFrondEngine));
		m_pFrondEngine = pMemory ? ::new (pMemory) CFrondEngine() : nullptr;
		pMemory = MemoryManager_Singleton.Allocate(sizeof(CIndexedGeometry));
		m_pFrondGeometry = pMemory ? ::new (pMemory) CIndexedGeometry(m_pWindEngine, true) : nullptr;
		EnterCriticalSection(&SInstanceList::m_UniqueTreesCritSection.CriticalSection);
		CSpeedTreeRT* pTree = this;
		SInstanceList::m_vUniqueTrees.push_back(pTree);
		++CSpeedTreeRT::m_uiAllRefCount;
		LeaveCriticalSection(&SInstanceList::m_UniqueTreesCritSection.CriticalSection);
		CIdvBranch::m_pLightingEngine = m_pLightingEngine;
		m_pTreeSizes[STS_MIN_BOX + 0] = 0.0f;
		m_pTreeSizes[STS_MIN_BOX + 1] = 0.0f;
		m_pTreeSizes[STS_MIN_BOX + 2] = 0.0f;
		m_pTreeSizes[STS_MAX_BOX + 0] = 1.0f;
		m_pTreeSizes[STS_MAX_BOX + 1] = 1.0f;
		m_pTreeSizes[STS_MAX_BOX + 2] = 1.0f;
		m_pTreeSizes[STS_BB_SIZE] = 1.0f;
		for (int32_t i = 0; i < 12; ++i) {
			m_afHorizontalCoords[i] = 0.0f;
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::CSpeedTreeRT( )", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::CSpeedTreeRT( )");
		SetError(pError->c_str());
	}
}

// The instance shares pOrig's engines and tables; its own instance data is new'd and set from pOrig.
CSpeedTreeRT::CSpeedTreeRT(const CSpeedTreeRT* pOrig)
	: m_pEngine(pOrig->m_pEngine), m_pBranchGeometry(pOrig->m_pBranchGeometry), m_pLeafGeometry(pOrig->m_pLeafGeometry),
	  m_pLightingEngine(pOrig->m_pLightingEngine), m_pWindEngine(pOrig->m_pWindEngine),
	  m_pSimpleBillboard(pOrig->m_pSimpleBillboard), m_eLeafLodMethod(pOrig->m_eLeafLodMethod),
	  m_fLeafLodTransitionRadius(pOrig->m_fLeafLodTransitionRadius),
	  m_fLeafLodCurveExponent(pOrig->m_fLeafLodCurveExponent),
	  m_fLeafSizeIncreaseFactor(pOrig->m_fLeafSizeIncreaseFactor),
	  m_fLeafTransitionFactor(pOrig->m_fLeafTransitionFactor), m_pLeafLodSizeFactors(pOrig->m_pLeafLodSizeFactors),
	  m_pInstanceRefCount(pOrig->m_pInstanceRefCount), m_pInstanceList(pOrig->m_pInstanceList),
	  m_nFrondLevel(pOrig->m_nFrondLevel), m_pTreeSizes(pOrig->m_pTreeSizes),
	  m_ucTargetAlphaValue(pOrig->m_ucTargetAlphaValue), m_bTreeComputed(pOrig->m_bTreeComputed),
	  m_nBranchWindLevel(pOrig->m_nBranchWindLevel), m_pEmbeddedTexCoords(pOrig->m_pEmbeddedTexCoords),
	  m_pProjectedShadow(pOrig->m_pProjectedShadow), m_usNum360Billboards(pOrig->m_usNum360Billboards),
	  m_pCollisionObjects(pOrig->m_pCollisionObjects), m_pFrondEngine(pOrig->m_pFrondEngine),
	  m_pFrondGeometry(pOrig->m_pFrondGeometry), m_usNumFrondLodLevels(pOrig->m_usNumFrondLodLevels),
	  m_pUserData(pOrig->m_pUserData), m_b360Billboard(pOrig->m_b360Billboard),
	  m_bHorizontalBillboard(pOrig->m_bHorizontalBillboard) {
	try {
		CSpeedTreeRT* pTree = this;
		m_pInstanceList->m_vInstances.push_back(pTree);
		rt::memcpy(m_afHorizontalCoords, pOrig->m_afHorizontalCoords, sizeof(m_afHorizontalCoords));
		++*m_pInstanceRefCount;
		STreeInstanceData* pInstanceData =
			static_cast<STreeInstanceData*>(MemoryManager_Singleton.Allocate(sizeof(STreeInstanceData)));
		pInstanceData->m_pParent = const_cast<CSpeedTreeRT*>(pOrig);
		pInstanceData->m_cPosition = pOrig->m_pEngine->m_cPosition;
		pInstanceData->m_fLodLevel = pOrig->GetLodLevel();
		m_pInstanceData = pInstanceData;
		++CSpeedTreeRT::m_uiAllRefCount;
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError = IdvFormatString(&strError, "%s - failed [%s]",
			"CSpeedTreeRT::CSpeedTreeRT(CSpeedTreeRT*)", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError = IdvFormatString(&strError, "%s - threw an unknown system exception",
			"CSpeedTreeRT::CSpeedTreeRT(CSpeedTreeRT*)");
		SetError(pError->c_str());
	}
}

// Drops the tree from the unique trees or its instance list; the last reference frees what the trees share.
CSpeedTreeRT::~CSpeedTreeRT() {
	if (!m_pInstanceData) {
		EnterCriticalSection(&SInstanceList::m_UniqueTreesCritSection.CriticalSection);
		SInstanceList::TreeVector& vUniqueTrees = SInstanceList::m_vUniqueTrees;
		SInstanceList::TreeVector::iterator kIter = vUniqueTrees.begin();
		while (kIter != vUniqueTrees.end() && *kIter != this) {
			++kIter;
		}
		if (kIter != vUniqueTrees.end()) {
			vUniqueTrees.erase(kIter);
		}
		LeaveCriticalSection(&SInstanceList::m_UniqueTreesCritSection.CriticalSection);
	}
	--*m_pInstanceRefCount;
	if (m_pInstanceData) {
		// An instance is always in its tree's instance list: the exe erases what it finds without checking.
		SInstanceList::TreeVector& vInstances = m_pInstanceList->m_vInstances;
		SInstanceList::TreeVector::iterator kIter = vInstances.begin();
		while (kIter != vInstances.end() && *kIter != this) {
			++kIter;
		}
		vInstances.erase(kIter);
		MemoryManager_Singleton.Deallocate(m_pInstanceData);
		m_pInstanceData = nullptr;
	}
	if (!*m_pInstanceRefCount) {
		DeleteTransientData();
		if (m_pBranchGeometry) {
			m_pBranchGeometry->~CIndexedGeometry();
			MemoryManager_Singleton.Deallocate(m_pBranchGeometry);
		}
		MemoryManager_Singleton.Deallocate(m_pLightingEngine);
		MemoryManager_Singleton.Deallocate(m_pWindEngine);
		if (m_pLeafGeometry) {
			m_pLeafGeometry->~CLeafGeometry();
			MemoryManager_Singleton.Deallocate(m_pLeafGeometry);
		}
		MemoryManager_Singleton.Deallocate(m_pSimpleBillboard);
		delete m_pEngine;
		MemoryManager_Singleton.Deallocate(m_pInstanceRefCount);
		if (m_pInstanceList) {
			m_pInstanceList->~SInstanceList();
			MemoryManager_Singleton.Deallocate(m_pInstanceList);
		}
		MemoryManager_Singleton.Deallocate(m_pTreeSizes);
		if (m_pCollisionObjects) {
			m_pCollisionObjects->~SCollisionObjects();
			MemoryManager_Singleton.Deallocate(m_pCollisionObjects);
		}
		if (m_pEmbeddedTexCoords) {
			m_pEmbeddedTexCoords->~SEmbeddedTexCoords();
			MemoryManager_Singleton.Deallocate(m_pEmbeddedTexCoords);
		}
		if (m_pFrondEngine) {
			m_pFrondEngine->~CFrondEngine();
			MemoryManager_Singleton.Deallocate(m_pFrondEngine);
		}
		if (m_pFrondGeometry) {
			m_pFrondGeometry->~CIndexedGeometry();
			MemoryManager_Singleton.Deallocate(m_pFrondGeometry);
		}
		MemoryManager_Singleton.Deallocate(m_pLeafLodSizeFactors);
		if (m_pProjectedShadow) {
			m_pProjectedShadow->~CProjectedShadow();
			MemoryManager_Singleton.Deallocate(m_pProjectedShadow);
		}
		MemoryManager_Singleton.Deallocate(m_pUserData);
	}
	m_pBranchGeometry = nullptr;
	m_pLightingEngine = nullptr;
	m_pWindEngine = nullptr;
	m_pLeafGeometry = nullptr;
	m_pSimpleBillboard = nullptr;
	m_pEngine = nullptr;
	m_pInstanceRefCount = nullptr;
	m_pTreeSizes = nullptr;
	m_pInstanceList = nullptr;
	m_pCollisionObjects = nullptr;
	m_pEmbeddedTexCoords = nullptr;
	m_pFrondEngine = nullptr;
	m_pFrondGeometry = nullptr;
	m_pLeafLodSizeFactors = nullptr;
	m_pProjectedShadow = nullptr;
	m_pUserData = nullptr;
	if (--CSpeedTreeRT::m_uiAllRefCount == 0) {
		CIdvBezierSpline::ClearCache();
	}
}

// The exe starts the local at -1 and stores each branch's value through it as a float before returning it in ST0.
float CSpeedTreeRT::GetLodLevel() const {
	float fLodLevel = -1.0f;
	if (m_pInstanceData)
		fLodLevel = m_pInstanceData->m_fLodLevel;
	else
		fLodLevel = m_pEngine->m_fLod;
	return fLodLevel;
}

uint16_t CSpeedTreeRT::GetNumBranchLodLevels() {
	return (uint16_t)m_pEngine->m_nNumBranchLodLevels;
}

uint16_t CSpeedTreeRT::GetNumLeafLodLevels() {
	return (uint16_t)m_pEngine->m_sLeafInfo.m_nNumLeafLodLevels;
}

uint16_t CSpeedTreeRT::GetNumFrondLodLevels() const {
	return m_usNumFrondLodLevels;
}

void CSpeedTreeRT::FreeBranchColors() {
	if (m_pBranchGeometry) {
		m_pBranchGeometry->m_vColors.clear();
	}
}

// The exe loads m_pBranchGeometry once for both calls.
void CSpeedTreeRT::FreeBranchWindData() {
	if (m_pBranchGeometry) {
		CIndexedGeometry* pBranchGeometry = m_pBranchGeometry;
		pBranchGeometry->m_vWindWeights.clear();
		pBranchGeometry->m_vWindMatrixIndices.clear();
	}
}

// The level is computed on the x87 stack (kept as double here) and truncated through _ftol2.
int16_t CSpeedTreeRT::GetDiscreteBranchLodLevel(float fLodLevel) {
	if (fLodLevel == -1.0) {
		fLodLevel = GetLodLevel();
	}
	int32_t nNumLodLevels = (uint16_t)GetNumBranchLodLevels();
	int16_t sLevel = (int16_t)(int32_t)((1.0 - fLodLevel) * nNumLodLevels);
	if (sLevel == nNumLodLevels) {
		--sLevel;
	}
	return sLevel;
}

int16_t CSpeedTreeRT::GetDiscreteFrondLodLevel(float fLodLevel) {
	if (fLodLevel == -1.0) {
		fLodLevel = GetLodLevel();
	}
	int32_t nNumLodLevels = GetNumFrondLodLevels();
	int16_t sLevel = (int16_t)(int32_t)((1.0 - fLodLevel) * nNumLodLevels);
	if (sLevel == nNumLodLevels) {
		--sLevel;
	}
	return sLevel;
}

const CSpeedTreeRT* CSpeedTreeRT::InstanceOf() const {
	CSpeedTreeRT* pParent = nullptr;
	if (m_pInstanceData) {
		pParent = m_pInstanceData->m_pParent;
	}
	return pParent;
}

void CSpeedTreeRT::GetTreeSize(float& fSize, float& fVariance) const {
	m_pEngine->GetSize(fSize, fVariance);
}

// The level is truncated as the exe's fld/fld1/fsubrp/fimul/fistp under a chopping control word do; an out-of-range
// product gives the integer indefinite (0x80000000, low word 0), as in the exe.
uint16_t CSpeedTreeRT::GetDiscreteLeafLodLevel(float fLodLevel) {
	uint16_t nLevel = 0;
	if (fLodLevel == -1.0) {
		fLodLevel = GetLodLevel();
	}
	if (m_bDropToBillboard) {
		int32_t nNumLodLevels = (uint16_t)GetNumLeafLodLevels() + 1;
		nLevel = (uint16_t)(int32_t)((1.0 - fLodLevel) * nNumLodLevels);
		if (nLevel == nNumLodLevels) {
			--nLevel;
		}
	} else {
		int32_t nNumLodLevels = (uint16_t)GetNumLeafLodLevels();
		nLevel = (uint16_t)(int32_t)((1.0 - fLodLevel) * nNumLodLevels);
		if (nLevel == nNumLodLevels) {
			--nLevel;
		}
	}
	return nLevel;
}

// The engine's position goes through three float locals.
void CSpeedTreeRT::SetTreePosition(float x, float y, float z) {
	if (m_pInstanceData) {
		CVec3& cPosition = m_pInstanceData->m_cPosition;
		cPosition.m_afData[0] = x;
		cPosition.m_afData[1] = y;
		cPosition.m_afData[2] = z;
	} else {
		CVec3 cPosition;
		cPosition.m_afData[0] = x;
		cPosition.m_afData[1] = y;
		cPosition.m_afData[2] = z;
		m_pEngine->m_cPosition = cPosition;
	}
}

// The exe's first test (ah & 1) also rejects an unordered level.
void CSpeedTreeRT::SetLodLevel(float fLevel) {
	if (!(fLevel >= 0.0) || fLevel > 1.0) {
		SetError(const_cast<char*>("SetLodLevel() expects a value in the range of 0.0 to 1.0"));
	} else if (m_pInstanceData) {
		m_pInstanceData->m_fLodLevel = fLevel;
	} else {
		m_pEngine->m_fLod = fLevel;
	}
}

void CSpeedTreeRT::GetBoundingBox(float* pBounds) const {
	if (pBounds) {
		rt::memcpy(pBounds, m_pTreeSizes, sizeof(float) * 6);
	} else {
		SetError(const_cast<char*>("GetBoundingBox() expects a non-NULL parameter"));
	}
}

void CSpeedTreeRT::SetLeafRockingState(bool bFlag) {
	m_pWindEngine->m_bRockingLeaves = (bool)((uint8_t)bFlag);
}

CSpeedTreeRT::EWindMethod CSpeedTreeRT::GetLeafWindMethod() const {
	return static_cast<CSpeedTreeRT::EWindMethod>((int32_t)m_pWindEngine->m_eLeafMethod);
}

CSpeedTreeRT::EWindMethod CSpeedTreeRT::GetBranchWindMethod() const {
	return static_cast<CSpeedTreeRT::EWindMethod>((int32_t)m_pWindEngine->m_eBranchMethod);
}

CSpeedTreeRT::EWindMethod CSpeedTreeRT::GetFrondWindMethod() const {
	return static_cast<CSpeedTreeRT::EWindMethod>((int32_t)m_pWindEngine->m_eFrondMethod);
}

void CSpeedTreeRT::GetSimpleBillboardGeometry(CSpeedTreeRT::SGeometry& sGeometry) {
	float fHighLodAlphaValue;
	float fLowLodAlphaValue;
	int16_t sLowLod;
	int16_t sHighLod;
	if (CSpeedTreeRT::m_bDropToBillboard) {
		uint16_t usLodStageCount = GetNumLeafLodLevels() + 1;
		fHighLodAlphaValue = -1.0f;
		fLowLodAlphaValue = -1.0f;
		sHighLod = -1;
		sLowLod = -1;
		float fTargetAlphaValue = static_cast<float>(m_ucTargetAlphaValue);
		float fCurveExponent = m_fLeafLodCurveExponent;
		float fTransitionFactor = m_fLeafTransitionFactor;
		float fOverlapRadius = m_fLeafLodTransitionRadius;
		float fLodLevel = GetLodLevel();
		CSpeedTreeRT::GetTransitionValues(fLodLevel, usLodStageCount, fOverlapRadius, fTransitionFactor, fCurveExponent,
			fTargetAlphaValue, fHighLodAlphaValue, fLowLodAlphaValue, sHighLod, sLowLod);
		sGeometry.m_sBillboard0.m_fAlphaTestValue = -1.0f;
		if (sHighLod == usLodStageCount - 1) {
			sGeometry.m_sBillboard0.m_fAlphaTestValue = fHighLodAlphaValue;
		} else {
			if (sLowLod == usLodStageCount - 1) {
				sGeometry.m_sBillboard0.m_fAlphaTestValue = fLowLodAlphaValue;
			}
		}
		sGeometry.m_sBillboard0.m_bIsActive = sGeometry.m_sBillboard0.m_fAlphaTestValue != -1.0;
		sGeometry.m_sBillboard0.m_usWhich = 0;
		if (sGeometry.m_sBillboard0.m_bIsActive) {
			sGeometry.m_sBillboard0.m_pCoords =
				m_pSimpleBillboard->GetBillboardCoords(m_pTreeSizes[6], m_pTreeSizes[5]);
			if (m_pEmbeddedTexCoords) {
				sGeometry.m_sBillboard0.m_pTexCoords = m_pEmbeddedTexCoords->m_pBillboardTexCoords;
			} else {
				sGeometry.m_sBillboard0.m_pTexCoords = 0;
			}
		}
		sGeometry.m_sBillboard1.m_bIsActive = 0;
		sGeometry.m_sBillboard1.m_usWhich = 0;
		if (m_bHorizontalBillboard) {
			sGeometry.m_sHorizontalBillboard.m_pCoords = m_afHorizontalCoords;
			if (m_pEmbeddedTexCoords) {
				sGeometry.m_sHorizontalBillboard.m_pTexCoords =
					&m_pEmbeddedTexCoords->m_pBillboardTexCoords[8 * m_pEmbeddedTexCoords->m_nNumBillboards - 8];
			} else {
				sGeometry.m_sHorizontalBillboard.m_pTexCoords = 0;
			}
			sGeometry.m_sHorizontalBillboard.m_fAlphaTestValue = sGeometry.m_sBillboard0.m_fAlphaTestValue;
			sGeometry.m_sHorizontalBillboard.m_bIsActive = sGeometry.m_sHorizontalBillboard.m_fAlphaTestValue >= 0.0;
			sGeometry.m_sHorizontalBillboard.m_fAlphaTestValue = CSpeedTreeRT::m_fHorizontalFadeValue;
		} else {
			sGeometry.m_sHorizontalBillboard.m_bIsActive = 0;
		}
		sGeometry.m_sHorizontalBillboard.m_usWhich = 0;
	} else {
		sGeometry.m_sBillboard0.m_bIsActive = 0;
		sGeometry.m_sBillboard1.m_bIsActive = 0;
		sGeometry.m_sHorizontalBillboard.m_bIsActive = 0;
	}
}

const char* CSpeedTreeRT::GetUserData() const {
	return m_pUserData;
}

void CSpeedTreeRT::SetBranchDimming(float a2) {
	if (m_pEngine) {
		m_pEngine->m_sLeafInfo.m_fBranchDimmingScalar = a2;
	}
}

void CSpeedTreeRT::SetLeafDimming(float a2) {
	if (m_pEngine) {
		m_pEngine->m_sLeafInfo.m_fDimmingScalar = a2;
	}
}

void CSpeedTreeRT::SetBudAngleMin(float a2) {
	if (m_pEngine) {
		m_pEngine->m_sLeafInfo.m_fMinimumBudAngle = a2;
	}
}

void CSpeedTreeRT::SetBudAngleMax(float a2) {
	if (m_pEngine) {
		m_pEngine->m_sLeafInfo.m_fMaximumBudAngle = a2;
	}
}

void CSpeedTreeRT::FreeLeafLODDataArrays() {
	if (m_pLeafGeometry) {
		m_pLeafGeometry->FreeLODDataArrays();
	}
}

void CSpeedTreeRT::DeleteTransientData() {
	bool bTransientDataIntact = m_pEngine->m_bTransientDataIntact;
	if (bTransientDataIntact)
		m_pEngine->FreeTransientData();
	else
		SetError(const_cast<char*>("DeleteTransientData() called with no intact transient data"));
}

// The exe reads the branch geometry through the engine and returns its value through a float local.
float CSpeedTreeRT::GetTrunkWidth() const {
	if (m_pEngine && m_pEngine->m_pBranchGeometry) {
		float fWidth = m_pEngine->m_pBranchGeometry->m_fTrunkWidth;
		return fWidth;
	}
	return 0.0f;
}

float CSpeedTreeRT::GetTrunkLength() const {
	if (m_pEngine && m_pEngine->m_pBranchGeometry) {
		float fLength = m_pEngine->m_pBranchGeometry->m_fTrunkLength;
		return fLength;
	}
	return 0.0f;
}

void CSpeedTreeRT::SetLodLimits(float fNear, float fFar) {
	m_pEngine->SetLodLimits(fNear, fFar);
}

void CSpeedTreeRT::ComputeSelfShadowTexCoords() {
	CVec3* v1;
	float v3;
	float v4;
	float v5;
	float v6;
	CVec3 v7;
	CVec3 v8;
	CVec3 a2;
	CVec3 v10;
	float fRadius;
	if (m_pBranchGeometry) {
		if (m_pFrondGeometry) {
			if (m_pProjectedShadow) {
				v5 = m_pTreeSizes[1];
				v6 = m_pTreeSizes[2];
				v10.m_afData[0] = (float)(*m_pTreeSizes);
				v10.m_afData[1] = v5;
				v10.m_afData[2] = v6;
				v3 = m_pTreeSizes[4];
				v4 = m_pTreeSizes[5];
				a2.m_afData[0] = m_pTreeSizes[3];
				a2.m_afData[1] = v3;
				a2.m_afData[2] = v4;
				fRadius = (float)((&v10)->Distance(a2) * 0.5);
				v1 = (&v10)->Add(&v7, &a2);
				v1->Scale(&v8, 0x1p-1f);
				if (m_pEmbeddedTexCoords) {
					m_pProjectedShadow->ComputeTexCoords(m_pBranchGeometry, v8, fRadius,
						m_pEmbeddedTexCoords->m_afShadowTexCoords);
					m_pProjectedShadow->ComputeTexCoords(m_pFrondGeometry, v8, fRadius,
						m_pEmbeddedTexCoords->m_afShadowTexCoords);
				} else {
					m_pProjectedShadow->ComputeTexCoords(m_pBranchGeometry, v8, fRadius, nullptr);
					m_pProjectedShadow->ComputeTexCoords(m_pFrondGeometry, v8, fRadius, nullptr);
				}
			}
		}
	}
}

// ~CProjectedShadow inlined (the self-shadow map name's _Tidy); the geometry's FreeVertexTexCoords1 inlined as the
// float vector's clear.
void CSpeedTreeRT::FreeProjectedShadowData() {
	if (m_pProjectedShadow) {
		CProjectedShadow* pProjectedShadow = m_pProjectedShadow;
		if (pProjectedShadow) {
			FNVD_DESTRUCT(std::string)(&pProjectedShadow->m_strSelfShadowMap);
			MemoryManager_Singleton.Deallocate(pProjectedShadow);
		}
		m_pProjectedShadow = nullptr;
	}
	if (m_pBranchGeometry) {
		m_pBranchGeometry->m_vTexCoords1.clear();
	}
	if (m_pFrondGeometry) {
		m_pFrondGeometry->m_vTexCoords1.clear();
	}
}

// speedtree-2's try { } catch (std::exception&) { } catch (...) { }: each handler sets the error to IdvFormatString's
// text and frees it. src has three handlers: catch (rt::exception&) for src throws, catch (std::exception&) by
// name for exe throws (both call what(), vtable slot 1, as the exe's handler does), then catch (...). The try body
// throws nothing in practice. The check is the exe's "not above 0.0" (test ah,0x41: NaN is an error too)
// against the double 0.0. The checker's Windows models (harness/checker/emu.py MODELS) do not include
// RaiseException, so a call that throws ends as "unverifiable: calls RaiseException"; only paths that don't throw
// are compared.
void CSpeedTreeRT::SetTreeSize(float fNewSize, float fNewVariance) {
	try {
		bool bTransientDataIntact = m_pEngine->m_bTransientDataIntact;
		if (bTransientDataIntact) {
			if (fNewSize > 0.0) {
				m_pEngine->SetSize(fNewSize, fNewVariance);
			} else {
				SetError(const_cast<char*>("SetTreeSize() is only valid for size values greater than 0.0"));
			}
		} else {
			SetError(const_cast<char*>("SetTreeSize() has no effect after DeleteTransientData() has been called"));
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetTreeSize", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetTreeSize", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::SetTreeSize");
		SetError(pError->c_str());
	}
}

// The exe reuses SetTreeSize's error text and name (speedtree-2's copy-paste). The same try/catch as SetTreeSize.
// The checker's Windows models (harness/checker/emu.py MODELS) do not include RaiseException, so a call that throws
// ends as "unverifiable: calls RaiseException"; only paths that don't throw are compared.
uint32_t CSpeedTreeRT::GetSeed() const {
	uint32_t nSeed = 0;
	try {
		bool bTransientDataIntact = m_pEngine->m_bTransientDataIntact;
		if (bTransientDataIntact) {
			nSeed = static_cast<uint32_t>(m_pEngine->m_sTreeInfo.m_nSeed);
		} else {
			SetError(const_cast<char*>("SetTreeSize() has no effect after DeleteTransientData() has been called"));
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetTreeSize", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetTreeSize", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::SetTreeSize");
		SetError(pError->c_str());
	}
	return nSeed;
}

// The same try/catch as SetTreeSize, with this setter's name. The checker's Windows models (harness/checker/emu.py
// MODELS) do not include RaiseException, so a call that throws ends as "unverifiable: calls RaiseException"; only
// paths that don't throw are compared.
void CSpeedTreeRT::SetBranchLightingMethod(ELightingMethod eMethod) {
	try {
		if (m_bTreeComputed) {
			SetError(const_cast<char*>("SetBranchLightingMethod() has no effect after Compute() has been called"));
		} else {
			CIndexedGeometry* pBranchGeometry = m_pBranchGeometry;
			pBranchGeometry->m_bManualLighting = eMethod == LIGHT_STATIC;
			CLightingEngine* pLightingEngine = m_pLightingEngine;
			pLightingEngine->m_eBranchLightingMethod = eMethod;
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetBranchLightingMethod", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetBranchLightingMethod", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError = IdvFormatString(&strError, "%s - threw an unknown system exception",
			"CSpeedTreeRT::SetBranchLightingMethod");
		SetError(pError->c_str());
	}
}

// The same try/catch as SetTreeSize, with this setter's name. The checker's Windows models (harness/checker/emu.py
// MODELS) do not include RaiseException, so a call that throws ends as "unverifiable: calls RaiseException"; only
// paths that don't throw are compared.
void CSpeedTreeRT::SetLeafLightingMethod(ELightingMethod eMethod) {
	try {
		if (m_bTreeComputed) {
			SetError(const_cast<char*>("SetLeafLightingMethod() has no effect after Compute() has been called"));
		} else {
			CLightingEngine* pLightingEngine = m_pLightingEngine;
			pLightingEngine->m_eLeafLightingMethod = eMethod;
			CLeafGeometry* pLeafGeometry = m_pLeafGeometry;
			pLeafGeometry->m_bManualLighting = eMethod == LIGHT_STATIC;
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetLeafLightingMethod", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetLeafLightingMethod", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::SetLeafLightingMethod");
		SetError(pError->c_str());
	}
}

// The same try/catch as SetTreeSize, with this setter's name. The checker's Windows models (harness/checker/emu.py
// MODELS) do not include RaiseException, so a call that throws ends as "unverifiable: calls RaiseException"; only
// paths that don't throw are compared.
void CSpeedTreeRT::SetFrondLightingMethod(ELightingMethod eMethod) {
	try {
		if (m_bTreeComputed) {
			SetError(const_cast<char*>("SetFrondLightingMethod() has no effect after Compute() has been called"));
		} else {
			CIndexedGeometry* pFrondGeometry = m_pFrondGeometry;
			pFrondGeometry->m_bManualLighting = eMethod == LIGHT_STATIC;
			CLightingEngine* pLightingEngine = m_pLightingEngine;
			pLightingEngine->m_eFrondLightingMethod = eMethod;
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetFrondLightingMethod", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetFrondLightingMethod", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError = IdvFormatString(&strError, "%s - threw an unknown system exception",
			"CSpeedTreeRT::SetFrondLightingMethod");
		SetError(pError->c_str());
	}
}

// A count of 0 counts as 1. The same try/catch as SetTreeSize, with this setter's name. The checker's Windows models
// (harness/checker/emu.py MODELS) do not include RaiseException, so a call that throws ends as "unverifiable: calls
// RaiseException"; only paths that don't throw are compared.
void CSpeedTreeRT::SetNumLeafRockingGroups(uint32_t nRockingGroups) {
	try {
		if (m_bTreeComputed) {
			SetError(const_cast<char*>("SetNumLeafRockingGroups() has no effect after Compute() has been called"));
		} else {
			if (nRockingGroups == 0) {
				nRockingGroups = 1;
			}
			CTreeEngine* pEngine = m_pEngine;
			pEngine->m_sLeafInfo.m_nNumRockingGroups = static_cast<int32_t>(nRockingGroups);
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetLeafRockingState", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetLeafRockingState", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::SetLeafRockingState");
		SetError(pError->c_str());
	}
}

// The same try/catch as SetTreeSize, with this setter's name. The checker's Windows models (harness/checker/emu.py
// MODELS) do not include RaiseException, so a call that throws ends as "unverifiable: calls RaiseException"; only
// paths that don't throw are compared.
void CSpeedTreeRT::SetLeafWindMethod(EWindMethod eMethod) {
	try {
		if (m_bTreeComputed) {
			SetError(const_cast<char*>("SetLeafWindMethod() has no effect after Compute() has been called"));
		} else {
			CWindEngine* pWindEngine = m_pWindEngine;
			pWindEngine->m_eLeafMethod = eMethod;
			CLeafGeometry* pLeafGeometry = m_pLeafGeometry;
			pLeafGeometry->m_bVertexWeighting = eMethod != WIND_NONE;
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetLeafWindMethod", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetLeafWindMethod", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::SetLeafWindMethod");
		SetError(pError->c_str());
	}
}

// Turning CPU wind off first invalidates the branches and computes their wind at LOD 0. The same try/catch as
// SetTreeSize, with this setter's name. The checker's Windows models (harness/checker/emu.py MODELS) do not include
// RaiseException, so a call that throws ends as "unverifiable: calls RaiseException"; only paths that don't throw
// are compared.
void CSpeedTreeRT::SetBranchWindMethod(EWindMethod eMethod) {
	try {
		if (m_bTreeComputed) {
			SetError(const_cast<char*>("SetBranchWindMethod() has no effect after Compute() has been called"));
		} else {
			if (eMethod == WIND_NONE && m_pWindEngine->m_eBranchMethod == WIND_CPU) {
				CIndexedGeometry* pBranchGeometry = m_pBranchGeometry;
				pBranchGeometry->m_bValid = false;
				m_pBranchGeometry->ComputeWindEffect(0);
			}
			CWindEngine* pWindEngine = m_pWindEngine;
			pWindEngine->m_eBranchMethod = eMethod;
			CIndexedGeometry* pWeightedGeometry = m_pBranchGeometry;
			pWeightedGeometry->m_bVertexWeighting = eMethod != WIND_NONE;
			CIndexedGeometry* pMethodGeometry = m_pBranchGeometry;
			pMethodGeometry->m_eWindMethod = eMethod;
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetBranchWindMethod", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetBranchWindMethod", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::SetBranchWindMethod");
		SetError(pError->c_str());
	}
}

// SetBranchWindMethod's shape for the fronds. The checker's Windows models (harness/checker/emu.py MODELS) do not
// include RaiseException, so a call that throws ends as "unverifiable: calls RaiseException"; only paths that don't
// throw are compared.
void CSpeedTreeRT::SetFrondWindMethod(EWindMethod eMethod) {
	try {
		if (m_bTreeComputed) {
			SetError(const_cast<char*>("SetFrondWindMethod() has no effect after Compute() has been called"));
		} else {
			if (eMethod == WIND_NONE && m_pWindEngine->m_eFrondMethod == WIND_CPU) {
				CIndexedGeometry* pFrondGeometry = m_pFrondGeometry;
				pFrondGeometry->m_bValid = false;
				m_pFrondGeometry->ComputeWindEffect(0);
			}
			CWindEngine* pWindEngine = m_pWindEngine;
			pWindEngine->m_eFrondMethod = eMethod;
			CIndexedGeometry* pWeightedGeometry = m_pFrondGeometry;
			pWeightedGeometry->m_bVertexWeighting = eMethod != WIND_NONE;
			CIndexedGeometry* pMethodGeometry = m_pFrondGeometry;
			pMethodGeometry->m_eWindMethod = eMethod;
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetFrondWindMethod", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetFrondWindMethod", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::SetFrondWindMethod");
		SetError(pError->c_str());
	}
}

// The strength is rejected when below the double 0.0 or unordered (test ah,1); -1.0 (a double) keeps the engine's
// strength or time shift (test ah,0x44; jp: equal only). The engine's result is stored as a float and returned in
// ST0. The same try/catch as SetTreeSize, with this setter's name. The checker's Windows models
// (harness/checker/emu.py MODELS) do not include RaiseException, so a call that throws ends as "unverifiable: calls
// RaiseException"; only paths that don't throw are compared.
float CSpeedTreeRT::SetWindStrength(float fNewStrength, float fOldStrength, float fFrequencyTimeOffset) {
	float fNewFrequencyTimeShift = 0.0f;
	try {
		if (!(fNewStrength >= 0.0)) {
			SetError(const_cast<char*>("SetWindStrength() expects new wind strength >= 0.0"));
		} else {
			if (fOldStrength == -1.0) {
				fOldStrength = m_pWindEngine->m_fWindStrength;
			}
			if (fFrequencyTimeOffset == -1.0) {
				CWindEngine* pWindEngine = m_pWindEngine;
				fFrequencyTimeOffset = pWindEngine->m_fTimeFrequencyShift;
			}
			float fShift = m_pWindEngine->SetWindStrength(fNewStrength, fOldStrength, fFrequencyTimeOffset);
			fNewFrequencyTimeShift = fShift;
			if (GetBranchWindMethod() == WIND_CPU && m_pBranchGeometry) {
				CIndexedGeometry* pBranchGeometry = m_pBranchGeometry;
				pBranchGeometry->m_bValid = false;
			}
			if (GetFrondWindMethod() == WIND_CPU && m_pFrondGeometry) {
				CIndexedGeometry* pFrondGeometry = m_pFrondGeometry;
				pFrondGeometry->m_bValid = false;
			}
			if (GetLeafWindMethod() == WIND_CPU || m_pWindEngine->m_bRockingLeaves) {
				m_pLeafGeometry->Invalidate();
			}
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetWindStrength", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetWindStrength", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::SetWindStrength");
		SetError(pError->c_str());
	}
	return fNewFrequencyTimeShift;
}

// The last matrix is compared unsigned (jae) with the shared matrix count. The same try/catch as SetTreeSize, with
// this setter's name. The checker's Windows models (harness/checker/emu.py MODELS) do not include RaiseException, so
// a call that throws ends as "unverifiable: calls RaiseException"; only paths that don't throw are compared.
void CSpeedTreeRT::SetLocalMatrices(uint32_t nStartingMatrix, uint32_t nMatrixSpan) {
	try {
		if (m_bTreeComputed) {
			SetError(const_cast<char*>("SetLocalMatrices() has no effect after Compute() has been called"));
		} else if (static_cast<uint32_t>(nStartingMatrix + nMatrixSpan - 1) >=
			CWindEngine::m_cWindMatrices.m_usNumMatrices) {
			SetError(const_cast<char*>("SetLocalMatrices() parameters exceed available wind matrices"));
		} else {
			m_pWindEngine->SetLocalMatrices(nStartingMatrix, nMatrixSpan);
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetLocalMatrices", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetLocalMatrices", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::SetLocalMatrices");
		SetError(pError->c_str());
	}
}

// speedtree-2's Compute, with its try { } catch (std::exception&) { } catch (...) { } as in SetTreeSize (both
// rt::exception& and std::exception& handlers here). CTreeEngine::InitTables is inlined: the leaf info's InitTables
// with the leaf texture count (a signed idiv by the 0x54-byte element). GetLeafLightingMethod, GetNumLodLevels,
// GetWindData, GetAllLeaves, GetLeafInfo and GetNumLeafLodLevels are inlined as field reads; the leaf LOD count is
// passed as its low word. CombineStrips gets false (push 0). memcpy is called out of line. Each __max is the macro's
// expansion: the running maximum is kept only when above the new distance (fcompp, test ah,0x41: equal or NaN take
// the new side), which is Distance called again on a second temporary; every distance goes through a float. The
// region's CVecs are destroyed through the exe's empty CVec destructor, m_cMax first; the exe's cleanup-only unwind
// of them is left out. Returns only al (the PDB's bool). The checker's Windows models (harness/checker/emu.py
// MODELS) do not include RaiseException, so a call that throws ends as "unverifiable: calls RaiseException"; only
// paths that don't throw are compared.
bool CSpeedTreeRT::Compute(const float* pTransform, uint32_t nSeed, bool bCompositeStrips) {
	try {
		if (!m_bTreeComputed) {
			CIdvBranch::m_pFrondEngine = m_pFrondEngine;
			m_pEngine->SetSeed(static_cast<int32_t>(nSeed));
			m_pEngine->Compute(m_fLeafSizeIncreaseFactor);
			m_pFrondEngine->Compute(m_pFrondGeometry, m_pLightingEngine);
			m_usNumFrondLodLevels = m_pFrondGeometry->m_nNumLodLevels;

			CTreeEngine* pEngine = m_pEngine;
			pEngine->m_sLeafInfo.InitTables(static_cast<int32_t>(pEngine->m_sLeafInfo.m_vLeafTextures.size()));

			if (m_pLightingEngine->m_eLeafLightingMethod == LIGHT_STATIC) {
				ComputeLeafStaticLighting();
			}

			m_pWindEngine->Init(m_pEngine->m_sWindInfo);
			m_pLeafGeometry->Init(static_cast<uint16_t>(m_pEngine->m_sLeafInfo.m_nNumLeafLodLevels),
				m_pEngine->m_vLeafLods, m_pEngine->m_sLeafInfo);

			if (m_pEmbeddedTexCoords) {
				for (int32_t i = 0; i < m_pEmbeddedTexCoords->m_nNumLeafMaps; ++i) {
					m_pLeafGeometry->SetTextureCoords(i, &m_pEmbeddedTexCoords->m_pLeafTexCoords[i * 8]);
				}
				for (int32_t i = 0; i < m_pEmbeddedTexCoords->m_nNumFrondMaps; ++i) {
					m_pFrondEngine->SetTextureCoords(m_pFrondGeometry, i,
						&m_pEmbeddedTexCoords->m_pFrondTexCoords[i * 8], m_bTextureFlip);
				}
			}

			if (bCompositeStrips) {
				m_pBranchGeometry->CombineStrips(false);
				m_pFrondGeometry->CombineStrips(false);
			}

			if (pTransform) {
				CTransform cTransform;
				cTransform.B3CCB0();
				rt::memcpy(cTransform.m_afData, pTransform, 16 * sizeof(float));
				m_pBranchGeometry->Transform(cTransform);
				m_pLeafGeometry->Transform(cTransform);
				m_pFrondGeometry->Transform(cTransform);
				if (m_pCollisionObjects) {
					m_pCollisionObjects->TransformAll(cTransform);
				}
			}

			CRegion cExtents;
			cExtents.~CRegion();
			::new (&cExtents) CRegion;
			m_pBranchGeometry->ComputeExtents(cExtents);
			m_pLeafGeometry->ComputeExtents(cExtents);
			m_pFrondGeometry->ComputeExtents(cExtents);
			rt::memcpy(m_pTreeSizes + STS_MIN_BOX, cExtents.m_cMin.m_afData, 3 * sizeof(float));
			rt::memcpy(m_pTreeSizes + STS_MAX_BOX, cExtents.m_cMax.m_afData, 3 * sizeof(float));

			CVec3 cOrigin = {{0.0f, 0.0f, 0.0f}};
			float fDimension = cOrigin.Distance(CVec3{{m_pTreeSizes[0], m_pTreeSizes[1], 0.0f}});
			float fDistance = cOrigin.Distance(CVec3{{m_pTreeSizes[0], m_pTreeSizes[4], 0.0f}});
			if (!(fDimension > fDistance)) {
				fDimension = cOrigin.Distance(CVec3{{m_pTreeSizes[0], m_pTreeSizes[4], 0.0f}});
			}
			fDistance = cOrigin.Distance(CVec3{{m_pTreeSizes[3], m_pTreeSizes[1], 0.0f}});
			if (!(fDimension > fDistance)) {
				fDimension = cOrigin.Distance(CVec3{{m_pTreeSizes[3], m_pTreeSizes[1], 0.0f}});
			}
			fDistance = cOrigin.Distance(CVec3{{m_pTreeSizes[3], m_pTreeSizes[4], 0.0f}});
			if (!(fDimension > fDistance)) {
				fDimension = cOrigin.Distance(CVec3{{m_pTreeSizes[3], m_pTreeSizes[4], 0.0f}});
			}
			m_pTreeSizes[STS_BB_SIZE] = fDimension + fDimension;

			if (m_pProjectedShadow) {
				ComputeSelfShadowTexCoords();
			}

			SetupHorizontalBillboard();

			m_bTreeComputed = true;
		} else {
			SetError(const_cast<char*>("Compute() called more than once for single tree model (ignored)"));
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::Compute", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::Compute", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::Compute");
		SetError(pError->c_str());
	}

	return m_bTreeComputed;
}

// speedtree-2 makes the instance only while the engine's transient data is intact; the exe (Bethesda's) drops that
// test and always makes it. The same try/catch as SetTreeSize, with this function's name; the exe's cleanup-only
// unwind (the free when the constructor throws) is left out. The checker's Windows models (harness/checker/emu.py
// MODELS) do not include RaiseException, so a call that throws ends as "unverifiable: calls RaiseException"; only
// paths that don't throw are compared.
CSpeedTreeRT* CSpeedTreeRT::MakeInstance() {
	CSpeedTreeRT* pInstance = nullptr;
	try {
		void* pMemory = MemoryManager_Singleton.Allocate(sizeof(CSpeedTreeRT));
		CSpeedTreeRT* pNew;
		if (pMemory) {
			pNew = ::new (pMemory) CSpeedTreeRT(this);
		} else {
			pNew = nullptr;
		}
		pInstance = pNew;
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::MakeInstance", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::MakeInstance", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::MakeInstance");
		SetError(pError->c_str());
	}
	return pInstance;
}

// An instance's LOD is computed by the engine at the instance's position; the engine's position and LOD are saved
// to locals first and restored after. ComputeLod's result goes through a float local. The same try/catch as
// SetTreeSize, with this function's name. The checker's Windows models (harness/checker/emu.py MODELS) do not
// include RaiseException, so a call that throws ends as "unverifiable: calls RaiseException"; only paths that don't
// throw are compared.
void CSpeedTreeRT::ComputeLodLevel() {
	try {
		if (m_pInstanceData) {
			CVec3 cPosition = m_pEngine->m_cPosition;
			float fLod = m_pEngine->m_fLod;
			CVec3* pInstancePosition = &m_pInstanceData->m_cPosition;
			CTreeEngine* pEngine = m_pEngine;
			pEngine->m_cPosition = *pInstancePosition;
			float fInstanceLod = m_pEngine->ComputeLod();
			m_pInstanceData->m_fLodLevel = fInstanceLod;
			CTreeEngine* pLodEngine = m_pEngine;
			pLodEngine->m_fLod = fLod;
			CTreeEngine* pPositionEngine = m_pEngine;
			pPositionEngine->m_cPosition = cPosition;
		} else {
			m_pEngine->ComputeLod();
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::ComputeLodLevel", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::ComputeLodLevel", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::ComputeLodLevel");
		SetError(pError->c_str());
	}
}

// Bethesda's LoadTree(pFilename) (IDA's CSpeedTreeRT::Load; the PDB name by its call of LoadTree(pBlock), its
// messages and its place). speedtree-2's fopen/fseek/ftell/fread/fclose are replaced by FileFinder::GetFile
// (READ_ONLY, a 0x2800-byte buffer, the mesh archives), the BSFile's GetSize (vtable slot 10; the seek's error code is
// a constant 0), NiBinaryStream::Read inlined (BinaryRead with a one-byte component, the read function through
// m_pfnRead) and a delete of the file (vtable slot 0, flag 1). The block comes from MemoryManager::Allocate and is
// freed after LoadTree(pBlock). The throws are the exe's std::exception(const char* const&) of the formatted text,
// modelled as rt::exception and caught here; the formatted string's _Tidy after each throw is unreachable. Each
// catch handler deletes the file (when there is one) and drops the block without freeing it (speedtree-2's
// pMemory = NULL). src has three handlers: catch (rt::exception&) for src throws, catch (std::exception&) by name
// for exe throws (both call what(), vtable slot 1, as the exe's handler does), then catch (...). The checker's
// Windows models (harness/checker/emu.py MODELS) do not include RaiseException, so a call that throws ends as
// "unverifiable: calls RaiseException"; only paths that don't throw are compared.
bool CSpeedTreeRT::LoadTree(const char* pFilename) {
	bool bSuccess = false;
	uint8_t* pMemory = nullptr;
	BSFile* pFile = nullptr;
	try {
		if (pFilename) {
			pFile =
				static_cast<BSFile*>(FileFinder::GetFile(pFilename, NiFile::READ_ONLY, 0x2800, ARCHIVE_TYPE_MESHES));
			if (pFile) {
				int32_t nNumBytes = static_cast<int32_t>(pFile->GetSize());
				int32_t nErrorCode = 0;
				if (nNumBytes > 0 && nErrorCode >= 0) {
					pMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(nNumBytes));
					uint32_t uiComponentSize = 1;
					int32_t nBytesRead =
						static_cast<int32_t>(pFile->BinaryRead(pMemory, nNumBytes, &uiComponentSize, 1));
					if (nBytesRead == nNumBytes) {
						bSuccess = LoadTree(pMemory, nNumBytes);
						MemoryManager_Singleton.Deallocate(pMemory);
						pMemory = nullptr;
					} else {
						std::string strError;
						std::string* pError = IdvFormatString(&strError, "only read %d of %d from %s [%s]", nBytesRead,
							nNumBytes, pFilename, std::strerror(errno));
						const char* pText = pError->c_str();
						throw rt::exception(pText);
					}
					delete pFile;
					pFile = nullptr;
				} else {
					std::string strError;
					std::string* pError =
						IdvFormatString(&strError, "file seek failed on '%s' [%s]", pFilename, std::strerror(errno));
					const char* pText = pError->c_str();
					throw rt::exception(pText);
				}
			} else {
				std::string strError;
				std::string* pError =
					IdvFormatString(&strError, "failed to load file '%s' [%s]", pFilename, std::strerror(errno));
				const char* pText = pError->c_str();
				throw rt::exception(pText);
			}
		}
	} catch (rt::exception& cException) {
		if (pFile) {
			delete pFile;
			pFile = nullptr;
		}
		pMemory = nullptr;
		std::string strError;
		std::string* pError = IdvFormatString(&strError, "CSpeedTreeRT::Load Tree - %s", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		if (pFile) {
			delete pFile;
			pFile = nullptr;
		}
		pMemory = nullptr;
		std::string strError;
		std::string* pError = IdvFormatString(&strError, "CSpeedTreeRT::Load Tree - %s", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		if (pFile) {
			delete pFile;
			pFile = nullptr;
		}
		pMemory = nullptr;
		std::string strError;
		std::string* pError = IdvFormatString(&strError, "CSpeedTreeRT::LoadTree - threw an unknown system exception");
		SetError(pError->c_str());
	}
	return bSuccess;
}

// speedtree-2's LoadTree(pBlock, nNumBytes), with Bethesda's changes (the asm wins): three more tokens (the
// SpeedWind rock and rustle scalars, stored in the wind engine through float locals, and the branches' propagated
// flexibility through bool locals), the old LOD method 2 turned smooth with a transition factor of 0.5 (speedtree-2:
// 0.1, and 0.1 again unconditionally after; the exe has neither 0.1), and the wind engine's local matrices set to all
// the shared wind matrices. CTreeFileAccess::EndOfFile is inlined (an unsigned compare of the position with the
// buffer's size), as are the lighting engine's three method getters (each reloads m_pLightingEngine) and
// CTreeEngine::GetBranchLevelForWeighting. The file's destructor is inlined at the end of the try: the byte vector's
// out-of-line _Tidy (0x657F60), then its iterator proxy freed with MemoryManager::Deallocate. The exe's cleanup-only
// unwind of the file on a throw is left out. This is the catcher of the parsers' IdvFileError throws (the tree
// engine's branch info parser through CTreeEngine::Parse, ParseCollisionObjects): src has three handlers, catch
// (rt::exception&) for src throws, catch (std::exception&) by name for exe throws (both call what(), vtable slot
// 1, as the exe's handler does), then catch (...). The checker's Windows models (harness/checker/emu.py MODELS) do
// not include RaiseException, so a call that throws ends as "unverifiable: calls RaiseException"; only paths that
// don't throw are compared.
bool CSpeedTreeRT::LoadTree(const uint8_t* pBlock, uint32_t nNumBytes) {
	bool bSuccess = false;
	try {
		CTreeFileAccess cFile;
		cFile.~CTreeFileAccess();
		::new (&cFile) CTreeFileAccess(pBlock, nNumBytes);
		if (m_pEngine->Parse(cFile)) {
			if (!(static_cast<uint32_t>(cFile.m_nIndex) >= static_cast<uint32_t>(cFile.m_vMemory.size()))) {
				bool bEndOfFile = false;
				int32_t nToken = cFile.ParseToken();
				do {
					switch (nToken) {
					case File_BeginLightingInfo:
						m_pLightingEngine->Parse(cFile);
						break;
					case File_BeginLodInfo:
						ParseLodInfo(&cFile);
						break;
					case File_BeginNewWindInfo:
						ParseWindInfo(&cFile);
						break;
					case File_BeginTextureCoordInfo:
						ParseTextureCoordInfo(&cFile);
						break;
					case File_BeginCollisionInfo:
						ParseCollisionObjects(&cFile);
						break;
					case File_BeginFrondInfo:
						m_pFrondEngine->Parse(cFile);
						break;
					case File_BeginTextureControls:
						m_pEngine->ParseTextureControls(cFile);
						break;
					case File_BeginFlareInfo:
						m_pEngine->ParseFlareInfo(cFile);
						break;
					case File_FlareSeed:
						m_pEngine->ParseFlareSeed(cFile);
						break;
					case File_LeafTransitionFactor: {
						float fFactor = cFile.ParseFloat();
						m_fLeafTransitionFactor = fFactor;
						break;
					}
					case File_BeginShadowProjectionInfo:
						ParseShadowProjectionInfo(&cFile);
						break;
					case File_BeginUserData:
						ParseUserData(&cFile);
						break;
					case File_BeginSupplementalTexCoordInfo:
						ParseSupplementalTexCoordInfo(&cFile);
						break;
					case File_SpeedWindRockScalar: {
						float fScalar = cFile.ParseFloat();
						CWindEngine* pWindEngine = m_pWindEngine;
						pWindEngine->m_fSpeedWindRockScalar = fScalar;
						break;
					}
					case File_SpeedWindRustleScalar: {
						float fScalar = cFile.ParseFloat();
						CWindEngine* pWindEngine = m_pWindEngine;
						pWindEngine->m_fSpeedWindRustleScalar = fScalar;
						break;
					}
					case File_PropagateFlexibility: {
						bool bPropagate = cFile.ParseBool();
						CIdvBranch::m_bPropagateFlexibility = bPropagate;
						break;
					}
					default:
						bEndOfFile = true;
					}
					if (static_cast<uint32_t>(cFile.m_nIndex) >= static_cast<uint32_t>(cFile.m_vMemory.size())) {
						bEndOfFile = true;
					} else {
						nToken = cFile.ParseToken();
					}
				} while (!bEndOfFile);
			}
			SetBranchLightingMethod(m_pLightingEngine->m_eBranchLightingMethod);
			SetLeafLightingMethod(m_pLightingEngine->m_eLeafLightingMethod);
			SetFrondLightingMethod(m_pLightingEngine->m_eFrondLightingMethod);
			m_nBranchWindLevel = m_pEngine->m_nBranchesWeightLevel;
			m_nFrondLevel = m_pFrondEngine->GetLevel();
			// speedtree-2's old LOD method 2 (no ELodMethod enumerator in the PDB).
			if (m_eLeafLodMethod == static_cast<ELodMethod>(2)) {
				m_eLeafLodMethod = LOD_SMOOTH;
				m_fLeafTransitionFactor = 0.5f;
			}
			m_pWindEngine->SetLocalMatrices(0, CWindEngine::m_cWindMatrices.m_usNumMatrices);
			bSuccess = true;
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::LoadTree", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::LoadTree", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::LoadTree");
		SetError(pError->c_str());
	}
	return bSuccess;
}

// It throws IdvFileError with a C++ throw. Its only exe caller is LoadTree(pBlock) (0xB059E0), registered in src
// with catch (rt::exception&) ahead of the exe's catch (std::exception&), so the error is caught by its own
// handler (the Catchers ruling, Main, 2026-11-04); registered together with it. The checker's
// Windows models (harness/checker/emu.py MODELS) do not include RaiseException, so a call that throws ends as
// "unverifiable: calls RaiseException". The exe's cleanup-only EH frame is left out; its _Tidy of each message
// string after the throw is unreachable. The end-of-file test is CTreeFileAccess::EndOfFile inlined (an unsigned
// compare of the position with the buffer's size).
void CSpeedTreeRT::ParseCollisionObjects(CTreeFileAccess* pFile) {
	void* pMemory = MemoryManager_Singleton.Allocate(sizeof(SCollisionObjects));
	SCollisionObjects* pCollisionObjects;
	if (pMemory) {
		::new (pMemory) SShapeVector;
		pCollisionObjects = static_cast<SCollisionObjects*>(pMemory);
	} else {
		pCollisionObjects = nullptr;
	}
	m_pCollisionObjects = pCollisionObjects;
	int32_t eToken = pFile->ParseToken();
	do {
		SShape sShape;
		sShape.m_eType = CO_SPHERE;
		sShape.m_afPosition[2] = 0.0f;
		sShape.m_afPosition[1] = 0.0f;
		sShape.m_afPosition[0] = 0.0f;
		sShape.m_afDimensions[2] = 0.0f;
		sShape.m_afDimensions[1] = 0.0f;
		sShape.m_afDimensions[0] = 0.0f;
		switch (eToken) {
		case File_CollisionSphere:
			sShape.m_eType = CO_SPHERE;
			break;
		case File_CollisionCylinder:
			sShape.m_eType = CO_CYLINDER;
			break;
		case File_CollisionBox:
			sShape.m_eType = CO_BOX;
			break;
		default: {
			std::string strDetails;
			FNVD_RECONSTRUCT(std::string)(&strDetails, "malformed collision object info");
			throw IdvFileError(strDetails, false);
		}
		}
		float fX = pFile->ParseFloat();
		float fY = pFile->ParseFloat();
		float fZ = pFile->ParseFloat();
		sShape.m_afPosition[0] = fX;
		sShape.m_afPosition[1] = fY;
		sShape.m_afPosition[2] = fZ;
		switch (sShape.m_eType) {
		case CO_SPHERE:
			sShape.m_afDimensions[0] = pFile->ParseFloat();
			break;
		case CO_CYLINDER:
			sShape.m_afDimensions[0] = pFile->ParseFloat();
			sShape.m_afDimensions[1] = pFile->ParseFloat();
			break;
		case CO_BOX:
			sShape.m_afDimensions[0] = pFile->ParseFloat();
			sShape.m_afDimensions[1] = pFile->ParseFloat();
			sShape.m_afDimensions[2] = pFile->ParseFloat();
			break;
		default: {
			std::string strDetails;
			FNVD_RECONSTRUCT(std::string)(&strDetails, "unknown collision object type");
			throw IdvFileError(strDetails, false);
		}
		}
		m_pCollisionObjects->m_vObjects.push_back(sShape);
		if (static_cast<uint32_t>(pFile->m_nIndex) >= pFile->m_vMemory.size()) {
			std::string strDetails;
			FNVD_RECONSTRUCT(std::string)(&strDetails, "premature end of file reached parsing collision object info");
			throw IdvFileError(strDetails, false);
		}
		eToken = pFile->ParseToken();
	} while (eToken != File_EndCollisionInfo);
}

// The exe's cleanup-only EH frame (the free when the constructor throws) is left out.
void CSpeedTreeRT::ParseShadowProjectionInfo(CTreeFileAccess* pFile) {
	void* pMemory = MemoryManager_Singleton.Allocate(sizeof(CProjectedShadow));
	CProjectedShadow* pProjectedShadow;
	if (pMemory) {
		pProjectedShadow = ::new (pMemory) CProjectedShadow;
	} else {
		pProjectedShadow = nullptr;
	}
	m_pProjectedShadow = pProjectedShadow;
	m_pProjectedShadow->Parse(*pFile);
}

// The vertex count is the coordinates' size (a signed shift) divided by 3, unsigned. GetTransitionValues gets the
// target alpha as a float; the alpha test values are stored as floats.
void CSpeedTreeRT::GetFrondGeometry(SGeometry& sGeometry, int16_t sOverrideLodValue) {
	if (m_pFrondGeometry) {
		SGeometry::SIndexed& sFronds = sGeometry.m_sFronds;
		sFronds.m_usVertexCount = static_cast<uint16_t>(m_pFrondGeometry->m_vCoords.size() / 3);
		sFronds.m_pColors = m_pFrondGeometry->GetVertexColors();
		sFronds.m_pCoords = m_pFrondGeometry->GetVertexCoords();
		sFronds.m_pNormals = m_pFrondGeometry->GetVertexNormals();
		sFronds.m_pBinormals = m_pFrondGeometry->GetVertexBinormals();
		sFronds.m_pTangents = m_pFrondGeometry->GetVertexTangents();
		sFronds.m_pTexCoords0 = m_pFrondGeometry->GetVertexTexCoords0();
		sFronds.m_pTexCoords1 = m_pFrondGeometry->GetVertexTexCoords1();
		sFronds.m_pWindWeights = m_pFrondGeometry->GetVertexWindWeights();
		sFronds.m_pWindMatrixIndices = m_pFrondGeometry->GetVertexWindMatrixIndices();
		int16_t sLodLevel;
		if (sOverrideLodValue == -1) {
			sLodLevel = GetDiscreteFrondLodLevel(-1.0f);
		} else {
			sLodLevel = sOverrideLodValue;
		}
		sFronds.m_nDiscreteLodLevel = sLodLevel;
		sFronds.m_usNumStrips = m_pFrondGeometry->GetNumStrips(static_cast<uint16_t>(sLodLevel));
		sFronds.m_pStripLengths = m_pFrondGeometry->GetStripLengthsPointer(static_cast<uint16_t>(sLodLevel));
		sFronds.m_pStrips =
			const_cast<const uint16_t**>(m_pFrondGeometry->GetStripsPointer(static_cast<uint16_t>(sLodLevel)));
		if (m_bDropToBillboard) {
			uint16_t usLodStageCount = static_cast<uint16_t>(static_cast<uint16_t>(GetNumLeafLodLevels()) + 1);
			float fHighValue = -1.0f;
			float fLowValue = -1.0f;
			int16_t sHighLod = -1;
			int16_t sLowLod = -1;
			GetTransitionValues(GetLodLevel(), usLodStageCount, m_fLeafLodTransitionRadius, m_fLeafTransitionFactor,
				m_fLeafLodCurveExponent, static_cast<float>(m_ucTargetAlphaValue), fHighValue, fLowValue, sHighLod,
				sLowLod);
			if (sHighLod == usLodStageCount - 2) {
				sGeometry.m_fFrondAlphaTestValue = fHighValue;
			} else if (sHighLod == usLodStageCount - 1) {
				sGeometry.m_fFrondAlphaTestValue = 255.0f;
				sFronds.m_nDiscreteLodLevel = -1;
			} else {
				sGeometry.m_fFrondAlphaTestValue = static_cast<float>(m_ucTargetAlphaValue);
			}
		} else {
			sGeometry.m_fFrondAlphaTestValue = static_cast<float>(m_ucTargetAlphaValue);
		}
	} else {
		SetError(const_cast<char*>("no frond geometry exists, possible prior call to DeleteFrondGeometry"));
	}
}

// usLodStageCount adds 1 for m_bDropToBillboard alone (the exe drops speedtree-2's embedded-billboard test).
// Before choosing, it runs CLeafGeometry::Update on every LOD into the first leaves. GetCamera, Assign3d,
// the radian-to-degree scaling (a double constant) and the CVec3 zeroing are inlined.
void CSpeedTreeRT::GetLeafGeometry(SGeometry& sGeometry, int16_t sOverrideLeafLodValue) {
	uint16_t usLeafLodCount = static_cast<uint16_t>(GetNumLeafLodLevels());
	uint16_t usLodStageCount = static_cast<uint16_t>(usLeafLodCount + (m_bDropToBillboard ? 1 : 0));
	CVec3 cCameraPos;
	cCameraPos.m_afData[2] = 0.0f;
	cCameraPos.m_afData[1] = 0.0f;
	cCameraPos.m_afData[0] = 0.0f;
	CVec3 cCameraDirection;
	cCameraDirection.m_afData[2] = 0.0f;
	cCameraDirection.m_afData[1] = 0.0f;
	cCameraDirection.m_afData[0] = 0.0f;
	CVec3 cAdjustedDirection;
	cAdjustedDirection.m_afData[2] = 0.0f;
	cAdjustedDirection.m_afData[1] = 0.0f;
	cAdjustedDirection.m_afData[0] = 0.0f;
	cCameraPos = CIdvCamera::m_cCameraPos;
	cCameraDirection = CIdvCamera::m_cCameraDir;
	cAdjustedDirection.m_afData[0] = cCameraDirection.m_afData[0];
	cAdjustedDirection.m_afData[1] = cCameraDirection.m_afData[1];
	cAdjustedDirection.m_afData[2] = cCameraDirection.m_afData[2];
	float fY = cAdjustedDirection.m_afData[1];
	float fX = cAdjustedDirection.m_afData[0];
	float fAzimuthRadians = static_cast<float>(std::atan2(static_cast<double>(fY), static_cast<double>(fX)));
	float fAzimuth = static_cast<float>(fAzimuthRadians * 57.295780181884766);
	float fZ = cAdjustedDirection.m_afData[2];
	float fPitchRadians = static_cast<float>(std::asin(static_cast<double>(fZ)));
	float fPitchDegrees = static_cast<float>(fPitchRadians * 57.295780181884766);
	float fPitch = -fPitchDegrees;
	for (uint16_t usLod = 0; usLod < static_cast<uint16_t>(GetNumLeafLodLevels()); ++usLod) {
		m_pLeafGeometry->Update(sGeometry.m_sLeaves0, usLod, fAzimuth, fPitch, m_fLeafSizeIncreaseFactor);
	}
	if (sOverrideLeafLodValue > -1) {
		m_pLeafGeometry->SmallUpdate(sGeometry.m_sLeaves0, static_cast<uint16_t>(sOverrideLeafLodValue), fAzimuth,
			fPitch, m_fLeafSizeIncreaseFactor);
		sGeometry.m_sLeaves0.m_bIsActive = true;
		sGeometry.m_sLeaves0.m_fAlphaTestValue = static_cast<float>(m_ucTargetAlphaValue);
		sGeometry.m_sLeaves1.m_bIsActive = false;
	} else if (m_eLeafLodMethod == LOD_SMOOTH) {
		float fHighLodAlphaValue = -1.0f;
		float fLowLodAlphaValue = -1.0f;
		int16_t sHighLod = -1;
		int16_t sLowLod = -1;
		GetTransitionValues(GetLodLevel(), usLodStageCount, m_fLeafLodTransitionRadius, m_fLeafTransitionFactor,
			m_fLeafLodCurveExponent, static_cast<float>(m_ucTargetAlphaValue), fHighLodAlphaValue, fLowLodAlphaValue,
			sHighLod, sLowLod);
		sGeometry.m_sLeaves0.m_bIsActive = sHighLod != -1 && sHighLod < usLeafLodCount;
		if (sGeometry.m_sLeaves0.m_bIsActive) {
			m_pLeafGeometry->SmallUpdate(sGeometry.m_sLeaves0, static_cast<uint16_t>(sHighLod), fAzimuth, fPitch,
				m_fLeafSizeIncreaseFactor);
			sGeometry.m_sLeaves0.m_fAlphaTestValue = fHighLodAlphaValue;
		}
		sGeometry.m_sLeaves1.m_bIsActive = sLowLod != -1 && sLowLod < usLeafLodCount;
		if (sGeometry.m_sLeaves1.m_bIsActive) {
			m_pLeafGeometry->SmallUpdate(sGeometry.m_sLeaves1, static_cast<uint16_t>(sLowLod), fAzimuth, fPitch,
				m_fLeafSizeIncreaseFactor);
			sGeometry.m_sLeaves1.m_fAlphaTestValue = fLowLodAlphaValue;
		}
	} else if (m_eLeafLodMethod == LOD_NONE) {
		m_pLeafGeometry->SmallUpdate(sGeometry.m_sLeaves0, 0, fAzimuth, fPitch, m_fLeafSizeIncreaseFactor);
		sGeometry.m_sLeaves0.m_bIsActive = true;
		sGeometry.m_sLeaves0.m_fAlphaTestValue = static_cast<float>(m_ucTargetAlphaValue);
		sGeometry.m_sLeaves1.m_bIsActive = false;
	} else {
		uint16_t usDiscreteLodLevel = GetDiscreteLeafLodLevel(-1.0f);
		if (usDiscreteLodLevel < static_cast<uint16_t>(GetNumLeafLodLevels())) {
			m_pLeafGeometry->SmallUpdate(sGeometry.m_sLeaves0, usDiscreteLodLevel, fAzimuth, fPitch,
				m_fLeafSizeIncreaseFactor);
			sGeometry.m_sLeaves0.m_bIsActive = true;
			sGeometry.m_sLeaves0.m_fAlphaTestValue = static_cast<float>(m_ucTargetAlphaValue);
		}
		sGeometry.m_sLeaves1.m_bIsActive = false;
	}
}

void CSpeedTreeRT::SetupHorizontalBillboard() {
	if (m_bHorizontalBillboard) {
		float* pTreeSizes = m_pTreeSizes;
		float v5 = *pTreeSizes;
		float v6 = pTreeSizes[1];
		float fHalfHeight = (pTreeSizes[2] + pTreeSizes[5]) * 0.5;
		float v2 = pTreeSizes[3];
		float v3 = pTreeSizes[4];
		m_afHorizontalCoords[0] = v2;
		m_afHorizontalCoords[1] = v3;
		m_afHorizontalCoords[2] = fHalfHeight;
		m_afHorizontalCoords[3] = v5;
		m_afHorizontalCoords[4] = v3;
		m_afHorizontalCoords[5] = fHalfHeight;
		m_afHorizontalCoords[6] = v5;
		m_afHorizontalCoords[7] = v6;
		m_afHorizontalCoords[8] = fHalfHeight;
		m_afHorizontalCoords[9] = v2;
		m_afHorizontalCoords[10] = v6;
		m_afHorizontalCoords[11] = fHalfHeight;
	}
}

// The billboard's alpha is compared with -1.0 as a double ("not equal or unordered" is
// active), and the leaves are filled when fLodLevel is not below 0.0 (asm: test ah,5; jp, so NaN fills them too).
// CTreeEngine::GetCamera, Assign3d, the radian-to-degree scaling (a double constant) and the CVec3 zeroing are
// inlined. LOD_SMOOTH first updates LOD 0 of the first leaves, unlike speedtree-2's GetLeafGeometry.
void CSpeedTreeRT::GetBSGeometryLODData(SGeometry& sGeometry, float afLODlevel) {
	uint16_t usLeafLodCount = static_cast<uint16_t>(GetNumLeafLodLevels());
	uint16_t usLodStageCount = static_cast<uint16_t>(usLeafLodCount + (m_bDropToBillboard ? 1 : 0));
	float fHighLodAlphaValue = -1.0f;
	float fLowLodAlphaValue = -1.0f;
	int16_t sHighLod = -1;
	int16_t sLowLod = -1;
	GetTransitionValues(afLODlevel, usLodStageCount, m_fLeafLodTransitionRadius, m_fLeafTransitionFactor,
		m_fLeafLodCurveExponent, static_cast<float>(m_ucTargetAlphaValue), fHighLodAlphaValue, fLowLodAlphaValue,
		sHighLod, sLowLod);
	if (m_bDropToBillboard) {
		sGeometry.m_sBillboard0.m_fAlphaTestValue = -1.0f;
		if (sHighLod == usLodStageCount - 1) {
			sGeometry.m_sBillboard0.m_fAlphaTestValue = fHighLodAlphaValue;
		} else if (sLowLod == usLodStageCount - 1) {
			sGeometry.m_sBillboard0.m_fAlphaTestValue = fLowLodAlphaValue;
		}
		sGeometry.m_sBillboard0.m_bIsActive = static_cast<double>(sGeometry.m_sBillboard0.m_fAlphaTestValue) != -1.0;
	} else {
		sGeometry.m_sBillboard0.m_bIsActive = false;
	}
	if (static_cast<double>(afLODlevel) < 0.0) {
		return;
	}
	if (m_bDropToBillboard) {
		if (sHighLod == usLodStageCount - 2) {
			sGeometry.m_fBranchAlphaTestValue = fHighLodAlphaValue;
		} else if (sHighLod == usLodStageCount - 1) {
			sGeometry.m_fBranchAlphaTestValue = 255.0f;
		} else {
			sGeometry.m_fBranchAlphaTestValue = static_cast<float>(m_ucTargetAlphaValue);
		}
	} else {
		sGeometry.m_fBranchAlphaTestValue = static_cast<float>(m_ucTargetAlphaValue);
	}
	CVec3 cCameraPos;
	cCameraPos.m_afData[2] = 0.0f;
	cCameraPos.m_afData[1] = 0.0f;
	cCameraPos.m_afData[0] = 0.0f;
	CVec3 cCameraDirection;
	cCameraDirection.m_afData[2] = 0.0f;
	cCameraDirection.m_afData[1] = 0.0f;
	cCameraDirection.m_afData[0] = 0.0f;
	CVec3 cAdjustedDirection;
	cAdjustedDirection.m_afData[2] = 0.0f;
	cAdjustedDirection.m_afData[1] = 0.0f;
	cAdjustedDirection.m_afData[0] = 0.0f;
	cCameraPos = CIdvCamera::m_cCameraPos;
	cCameraDirection = CIdvCamera::m_cCameraDir;
	cAdjustedDirection.m_afData[0] = cCameraDirection.m_afData[0];
	cAdjustedDirection.m_afData[1] = cCameraDirection.m_afData[1];
	cAdjustedDirection.m_afData[2] = cCameraDirection.m_afData[2];
	float fY = cAdjustedDirection.m_afData[1];
	float fX = cAdjustedDirection.m_afData[0];
	float fAzimuthRadians = static_cast<float>(std::atan2(static_cast<double>(fY), static_cast<double>(fX)));
	float fAzimuth = static_cast<float>(fAzimuthRadians * 57.295780181884766);
	float fZ = cAdjustedDirection.m_afData[2];
	float fPitchRadians = static_cast<float>(std::asin(static_cast<double>(fZ)));
	float fPitchDegrees = static_cast<float>(fPitchRadians * 57.295780181884766);
	float fPitch = -fPitchDegrees;
	if (m_eLeafLodMethod == LOD_SMOOTH) {
		m_pLeafGeometry->SmallUpdate(sGeometry.m_sLeaves0, 0, fAzimuth, fPitch, m_fLeafSizeIncreaseFactor);
		sGeometry.m_sLeaves0.m_bIsActive = sHighLod != -1 && sHighLod < usLeafLodCount;
		if (sGeometry.m_sLeaves0.m_bIsActive) {
			m_pLeafGeometry->SmallUpdate(sGeometry.m_sLeaves0, static_cast<uint16_t>(sHighLod), fAzimuth, fPitch,
				m_fLeafSizeIncreaseFactor);
			sGeometry.m_sLeaves0.m_fAlphaTestValue = fHighLodAlphaValue;
		}
		sGeometry.m_sLeaves1.m_bIsActive = sLowLod != -1 && sLowLod < usLeafLodCount;
		if (sGeometry.m_sLeaves1.m_bIsActive) {
			m_pLeafGeometry->SmallUpdate(sGeometry.m_sLeaves1, static_cast<uint16_t>(sLowLod), fAzimuth, fPitch,
				m_fLeafSizeIncreaseFactor);
			sGeometry.m_sLeaves1.m_fAlphaTestValue = fLowLodAlphaValue;
		}
	} else if (m_eLeafLodMethod == LOD_NONE) {
		m_pLeafGeometry->SmallUpdate(sGeometry.m_sLeaves0, 0, fAzimuth, fPitch, m_fLeafSizeIncreaseFactor);
		sGeometry.m_sLeaves0.m_bIsActive = true;
		sGeometry.m_sLeaves0.m_fAlphaTestValue = static_cast<float>(m_ucTargetAlphaValue);
		sGeometry.m_sLeaves1.m_bIsActive = false;
	} else {
		uint16_t usDiscreteLodLevel = GetDiscreteLeafLodLevel(-1.0f);
		if (usDiscreteLodLevel < static_cast<uint16_t>(GetNumLeafLodLevels())) {
			m_pLeafGeometry->SmallUpdate(sGeometry.m_sLeaves0, usDiscreteLodLevel, fAzimuth, fPitch,
				m_fLeafSizeIncreaseFactor);
			sGeometry.m_sLeaves0.m_bIsActive = true;
			sGeometry.m_sLeaves0.m_fAlphaTestValue = static_cast<float>(m_ucTargetAlphaValue);
		}
		sGeometry.m_sLeaves1.m_bIsActive = false;
	}
}

// The centre is each axis' (min + max) * the double 0.5, stored as floats, then copied into the CVec3. The leaf
// LODs are read before the out-of-line GetNumLeafLodLevels, whose count is passed zero-extended. The same try/catch
// as DeleteBranchGeometry. The checker's Windows models (harness/checker/emu.py MODELS) do not include
// RaiseException, so a call that throws ends as "unverifiable: calls RaiseException"; only paths that don't throw
// are compared.
void CSpeedTreeRT::ComputeLeafStaticLighting() {
	try {
		float afBoundingBox[6];
		GetBoundingBox(afBoundingBox);
		float fX = (float)((afBoundingBox[3] + afBoundingBox[0]) * 0.5);
		float fY = (float)((afBoundingBox[4] + afBoundingBox[1]) * 0.5);
		float fZ = (float)((afBoundingBox[5] + afBoundingBox[2]) * 0.5);
		CVec3 cTreeCenter;
		cTreeCenter.m_afData[0] = fX;
		cTreeCenter.m_afData[1] = fY;
		cTreeCenter.m_afData[2] = fZ;
		std::vector<CBillboardLeaf*, std::allocator<CBillboardLeaf*>>* pAllLeaves = m_pEngine->m_vLeafLods;
		uint16_t usNumLeafLods = (uint16_t)GetNumLeafLodLevels();
		m_pLightingEngine->ComputeLeafStaticLighting(cTreeCenter, pAllLeaves, usNumLeafLods);
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError = IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::ComputeLeafStaticLighting",
			cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError = IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::ComputeLeafStaticLighting",
			cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError = IdvFormatString(&strError, "%s - threw an unknown system exception",
			"CSpeedTreeRT::ComputeLeafStaticLighting");
		SetError(pError->c_str());
	}
}

// A do-while over the tokens up to File_EndUserData; each File_UserData string is copied by CopyUserData and its
// temporary freed through _Tidy(true, 0). No try/catch (a cleanup-only EH frame, left out).
void CSpeedTreeRT::ParseUserData(CTreeFileAccess* pFile) {
	int32_t nToken = pFile->ParseToken();
	do {
		if (nToken == File_UserData) {
			std::string strUserData = pFile->ParseString();
			m_pUserData = CopyUserData(strUserData.c_str());
		}
		nToken = pFile->ParseToken();
	} while (nToken != File_EndUserData);
}

// IdvFileError is thrown with a C++ throw: the exe's only caller, LoadTree(pBlock) (0xB059E0), which catches it, is
// registered in src. The exe's cleanup-only C++ EH frame is left out; its _Tidy of each message string after the
// throw is unreachable. The end-of-file test is CTreeFileAccess::EndOfFile inlined (an unsigned compare of the
// position with the buffer's size).
void CSpeedTreeRT::ParseLodInfo(CTreeFileAccess* pFile) {
	EFileToken eToken = pFile->ParseToken();
	do {
		switch (eToken) {
		case File_LeafTransitionMethod:
			m_eLeafLodMethod = static_cast<ELodMethod>(pFile->ParseInt());
			break;
		case File_LeafTransitionRadius:
			m_fLeafLodTransitionRadius = pFile->ParseFloat();
			break;
		case File_LeafCurveExponent:
			m_fLeafLodCurveExponent = pFile->ParseFloat();
			break;
		case File_BeginEngineLodInfo:
			m_pEngine->ParseLodInfo(*pFile);
			break;
		case File_LeafSizeIncreaseFactor:
			m_fLeafSizeIncreaseFactor = pFile->ParseFloat();
			break;
		default: {
			std::string strDetails;
			FNVD_RECONSTRUCT(std::string)(&strDetails, "malformed lod info");
			throw IdvFileError(strDetails, false);
		}
		}
		if (static_cast<uint32_t>(pFile->m_nIndex) >= pFile->m_vMemory.size()) {
			std::string strDetails;
			FNVD_RECONSTRUCT(std::string)(&strDetails, "premature end of file reached parsing new lod info");
			throw IdvFileError(strDetails, false);
		}
		eToken = pFile->ParseToken();
	} while (eToken != File_EndLodInfo);
}

// The throws, the EH frame and the end-of-file test as in ParseLodInfo.
void CSpeedTreeRT::ParseWindInfo(CTreeFileAccess* pFile) {
	EFileToken eToken = pFile->ParseToken();
	do {
		if (eToken != File_WindLevel) {
			std::string strDetails;
			FNVD_RECONSTRUCT(std::string)(&strDetails, "malformed new wind info");
			throw IdvFileError(strDetails, false);
		}
		m_pEngine->m_nBranchesWeightLevel = pFile->ParseInt();
		if (static_cast<uint32_t>(pFile->m_nIndex) >= pFile->m_vMemory.size()) {
			std::string strDetails;
			FNVD_RECONSTRUCT(std::string)(&strDetails, "premature end of file reached parsing new wind info");
			throw IdvFileError(strDetails, false);
		}
		eToken = pFile->ParseToken();
	} while (eToken != File_EndNewWindInfo);
}

// The throws, the EH frame and the end-of-file test as in ParseLodInfo (0xB092A0). Each array is `new float[8 * n]`:
// the exe's operator new[] size test (all bits set when the byte count overflows), then MemoryManager's Allocate.
void CSpeedTreeRT::ParseTextureCoordInfo(CTreeFileAccess* pFile) {
	void* pMemory = MemoryManager_Singleton.Allocate(sizeof(SEmbeddedTexCoords));
	SEmbeddedTexCoords* pTexCoords;
	if (pMemory) {
		pTexCoords = ::new (pMemory) SEmbeddedTexCoords;
	} else {
		pTexCoords = nullptr;
	}
	m_pEmbeddedTexCoords = pTexCoords;
	EFileToken eToken = pFile->ParseToken();
	do {
		switch (eToken) {
		case File_LeafTextureCoords:
			m_pEmbeddedTexCoords->m_nNumLeafMaps = pFile->ParseInt();
			if (m_pEmbeddedTexCoords->m_nNumLeafMaps > 0) {
				uint32_t uiCount = static_cast<uint32_t>(m_pEmbeddedTexCoords->m_nNumLeafMaps) << 3;
				size_t uiSize = uiCount > SIZE_MAX / sizeof(float) ? SIZE_MAX : sizeof(float) * uiCount;
				m_pEmbeddedTexCoords->m_pLeafTexCoords = static_cast<float*>(MemoryManager_Singleton.Allocate(uiSize));
				for (int32_t i = 0; i < m_pEmbeddedTexCoords->m_nNumLeafMaps; ++i) {
					for (int32_t j = 0; j < 8; ++j) {
						m_pEmbeddedTexCoords->m_pLeafTexCoords[8 * i + j] = pFile->ParseFloat();
					}
				}
			}
			break;
		case File_BillboardTextureCoords:
			m_pEmbeddedTexCoords->m_nNumBillboards = pFile->ParseInt();
			if (m_pEmbeddedTexCoords->m_nNumBillboards > 0) {
				uint32_t uiCount = static_cast<uint32_t>(m_pEmbeddedTexCoords->m_nNumBillboards) << 3;
				size_t uiSize = uiCount > SIZE_MAX / sizeof(float) ? SIZE_MAX : sizeof(float) * uiCount;
				m_pEmbeddedTexCoords->m_pBillboardTexCoords =
					static_cast<float*>(MemoryManager_Singleton.Allocate(uiSize));
				for (int32_t i = 0; i < m_pEmbeddedTexCoords->m_nNumBillboards; ++i) {
					for (int32_t j = 0; j < 8; ++j) {
						m_pEmbeddedTexCoords->m_pBillboardTexCoords[8 * i + j] = pFile->ParseFloat();
						// Each odd coordinate (v) is negated when textures are flipped.
						if (j % 2 && m_bTextureFlip) {
							m_pEmbeddedTexCoords->m_pBillboardTexCoords[8 * i + j] =
								-m_pEmbeddedTexCoords->m_pBillboardTexCoords[8 * i + j];
						}
					}
				}
			}
			break;
		case File_FrondTextureCoords:
			m_pEmbeddedTexCoords->m_nNumFrondMaps = pFile->ParseInt();
			if (m_pEmbeddedTexCoords->m_nNumFrondMaps > 0) {
				uint32_t uiCount = static_cast<uint32_t>(m_pEmbeddedTexCoords->m_nNumFrondMaps) << 3;
				size_t uiSize = uiCount > SIZE_MAX / sizeof(float) ? SIZE_MAX : sizeof(float) * uiCount;
				m_pEmbeddedTexCoords->m_pFrondTexCoords = static_cast<float*>(MemoryManager_Singleton.Allocate(uiSize));
				for (int32_t i = 0; i < m_pEmbeddedTexCoords->m_nNumFrondMaps; ++i) {
					for (int32_t j = 0; j < 8; ++j) {
						m_pEmbeddedTexCoords->m_pFrondTexCoords[8 * i + j] = pFile->ParseFloat();
					}
				}
			}
			break;
		case File_CompositeFilename: {
			// The parsed string is built in the by-value argument of CIdvFilename's constructor, which frees it;
			// the exe frees both file names with _Tidy (their destructors inlined).
			CIdvFilename cFilename;
			FNVD_RECONSTRUCT(CIdvFilename)(&cFilename, pFile->ParseString());
			CIdvFilename cNoPath;
			m_pEmbeddedTexCoords->m_strCompositeFilename.assign(*cFilename.NoPath(&cNoPath), 0, std::string::npos);
			break;
		}
		case File_HorizontalBillboard:
			m_bHorizontalBillboard = pFile->ParseBool();
			break;
		case File_360Billboard:
			m_b360Billboard = pFile->ParseBool();
			break;
		default: {
			std::string strDetails;
			FNVD_RECONSTRUCT(std::string)(&strDetails, "malformed texture coord info");
			throw IdvFileError(strDetails, false);
		}
		}
		if (static_cast<uint32_t>(pFile->m_nIndex) >= pFile->m_vMemory.size()) {
			std::string strDetails;
			FNVD_RECONSTRUCT(std::string)(&strDetails, "premature end of file reached parsing texture coordinate info");
			throw IdvFileError(strDetails, false);
		}
		eToken = pFile->ParseToken();
	} while (eToken != File_EndTextureCoordInfo);
}

// The throws, the EH frame and the end-of-file test as in ParseLodInfo. The parsed composite file name
// is copied into a CIdvFilename (the base string's copy constructor) and freed.
void CSpeedTreeRT::ParseSupplementalTexCoordInfo(CTreeFileAccess* pFile) {
	if (m_pEmbeddedTexCoords) {
		EFileToken eToken = pFile->ParseToken();
		do {
			switch (eToken) {
			case File_SupplementalCompositeFilename: {
				std::string strFilename = pFile->ParseString();
				CIdvFilename cFilename;
				// The copy constructor, called out of line.
				FNVD_RECONSTRUCT_AS(CopyConstruct, std::string)(&cFilename, strFilename);
				CIdvFilename cNoPath;
				m_pEmbeddedTexCoords->m_strCompositeFilename.assign(*cFilename.NoPath(&cNoPath), 0, std::string::npos);
				break;
			}
			case File_SupplementalHorizontalBillboard:
				m_bHorizontalBillboard = pFile->ParseBool();
				break;
			case File_Supplemental360Billboard:
				m_b360Billboard = pFile->ParseBool();
				break;
			case File_SupplementalShadowTexCoords:
				for (int32_t i = 0; i < 8; ++i) {
					m_pEmbeddedTexCoords->m_afShadowTexCoords[i] = pFile->ParseFloat();
				}
				break;
			default: {
				std::string strDetails;
				FNVD_RECONSTRUCT(std::string)(&strDetails, "malformed texture coord info");
				throw IdvFileError(strDetails, false);
			}
			}
			if (static_cast<uint32_t>(pFile->m_nIndex) >= pFile->m_vMemory.size()) {
				std::string strDetails;
				FNVD_RECONSTRUCT(std::string)(&strDetails,
					"premature end of file reached parsing texture coordinate info");
				throw IdvFileError(strDetails, false);
			}
			eToken = pFile->ParseToken();
		} while (eToken != File_EndSupplementalTexCoordInfo);
	}
}

// speedtree-2's try { } catch (std::exception&) { } catch (...) { }, as in SetTreeSize; each handler sets the error to
// IdvFormatString's text with this function's name. The checker's Windows models (harness/checker/emu.py MODELS) do
// not include RaiseException, so a call that throws ends as "unverifiable: calls RaiseException"; only paths that
// don't throw are compared. The geometry is deleted inline (destructor, then MemoryManager's free).
void CSpeedTreeRT::DeleteBranchGeometry() {
	try {
		if (m_bTreeComputed && m_pBranchGeometry && !m_pInstanceData && *m_pInstanceRefCount == 1) {
			CIndexedGeometry* pBranchGeometry = m_pBranchGeometry;
			if (pBranchGeometry) {
				pBranchGeometry->~CIndexedGeometry();
				MemoryManager_Singleton.Deallocate(pBranchGeometry);
			}
			m_pBranchGeometry = nullptr;
			if (m_pEngine) {
				CTreeEngine* pEngine = m_pEngine;
				pEngine->m_pBranchGeometry = nullptr;
			}
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::DeleteBranchGeometry", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::DeleteBranchGeometry", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::DeleteBranchGeometry");
		SetError(pError->c_str());
	}
}

// DeleteBranchGeometry's shape for the fronds: the reference count pointer is null-checked, the engine untouched.
// The checker's Windows models (harness/checker/emu.py MODELS) do not include RaiseException, so a call that throws
// ends as "unverifiable: calls RaiseException"; only paths that don't throw are compared.
void CSpeedTreeRT::DeleteFrondGeometry() {
	try {
		if (m_bTreeComputed && m_pFrondGeometry && !m_pInstanceData && m_pInstanceRefCount &&
			*m_pInstanceRefCount == 1) {
			CIndexedGeometry* pFrondGeometry = m_pFrondGeometry;
			if (pFrondGeometry) {
				pFrondGeometry->~CIndexedGeometry();
				MemoryManager_Singleton.Deallocate(pFrondGeometry);
			}
			m_pFrondGeometry = nullptr;
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::DeleteFrondGeometry", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::DeleteFrondGeometry", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::DeleteFrondGeometry");
		SetError(pError->c_str());
	}
}

// The same try/catch as DeleteBranchGeometry, with this function's name; the table is null after an exception.
// The checker's Windows models (harness/checker/emu.py MODELS) do not include RaiseException, so a call that throws
// ends as "unverifiable: calls RaiseException"; only paths that don't throw are compared.
const float* CSpeedTreeRT::GetLeafBillboardTable(uint32_t& nEntryCount) const {
	const float* pTable = nullptr;
	try {
		pTable = m_pLeafGeometry->GetLeafBillboardTable(nEntryCount);
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::GetLeafBillboardTable", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::GetLeafBillboardTable", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::GetLeafBillboardTable");
		SetError(pError->c_str());
	}
	return pTable;
}

// IDA's CTreeEngine::B0A2A0; the PDB name by its body (this is the tree: the collision objects at +0x58). The size
// is the byte difference divided signed (idiv) by 0x1C (size(); a pointer difference would be an exact division)
// and compared unsigned; each subscript reloads the collision objects and is checked inline through
// _invalid_parameter_noinfo. The position and dimensions are copied by the out-of-line memcpy. No try/catch.
void CSpeedTreeRT::GetCollisionObject(uint32_t nIndex, ECollisionObjectType& eType, float* pPosition,
	float* pDimensions) {
	if (m_pCollisionObjects) {
		SCollisionObjects* pObjects = m_pCollisionObjects;
		if (nIndex < pObjects->m_vObjects.size()) {
			SCollisionObjects* pTypeObjects = m_pCollisionObjects;
			eType = pTypeObjects->m_vObjects[nIndex].m_eType;
			SCollisionObjects* pPositionObjects = m_pCollisionObjects;
			rt::memcpy(pPosition, pPositionObjects->m_vObjects[nIndex].m_afPosition, sizeof(float) * 3);
			SCollisionObjects* pSizeObjects = m_pCollisionObjects;
			rt::memcpy(pDimensions, pSizeObjects->m_vObjects[nIndex].m_afDimensions, sizeof(float) * 3);
		} else {
			SCollisionObjects* pErrorObjects = m_pCollisionObjects;
			int32_t nCount = (int32_t)pErrorObjects->m_vObjects.size();
			std::string strError;
			std::string* pError =
				IdvFormatString(&strError, "collision object index (%d) exceeds maximum index (%d)", nIndex, nCount);
			SetError(pError->c_str());
		}
	} else {
		SetError(const_cast<char*>("no collision objects are stored with this tree"));
	}
}

// The vector's size() inlined: the exe divides the byte span by the 0x1C-byte shape with idiv, which size() does.
uint32_t CSpeedTreeRT::GetCollisionObjectCount() {
	uint32_t uiCount = 0;
	if (m_pCollisionObjects) {
		uiCount = static_cast<uint32_t>(m_pCollisionObjects->m_vObjects.size());
	}
	return uiCount;
}

// The LOD values are passed zero-extended; the 360-degree billboards are used when the tree has them and bit 0x10
// is clear. The same try/catch as DeleteBranchGeometry. The checker's Windows models (harness/checker/emu.py
// MODELS) do not include RaiseException, so a call that throws ends as "unverifiable: calls RaiseException"; only
// paths that don't throw are compared.
void CSpeedTreeRT::GetGeometry(SGeometry& sGeometry, uint32_t ulBitVector, int16_t sOverrideBranchLodValue,
	int16_t sOverrideFrondLodValue, int16_t sOverrideLeafLodValue) {
	try {
		if (ulBitVector & 1) {
			GetBranchGeometry(sGeometry, sOverrideBranchLodValue);
		}
		if (ulBitVector & 2) {
			GetFrondGeometry(sGeometry, sOverrideFrondLodValue);
		}
		if (ulBitVector & 4) {
			GetLeafGeometry(sGeometry, sOverrideLeafLodValue);
		}
		if (ulBitVector & 8) {
			if (m_b360Billboard && !(ulBitVector & 0x10)) {
				Get360BillboardGeometry(sGeometry, ulBitVector);
			} else {
				GetSimpleBillboardGeometry(sGeometry);
			}
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::GetGeometry", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::GetGeometry", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::GetGeometry");
		SetError(pError->c_str());
	}
}

void CSpeedTreeRT::GetBranchGeometry(CSpeedTreeRT::SGeometry& sGeometry, int16_t sOverrideLodValue) {
	if (m_pBranchGeometry) {
		int16_t sLodLevel;
		CSpeedTreeRT::SGeometry::SIndexed* sBranches = &sGeometry.m_sBranches;
		sGeometry.m_sBranches.m_usVertexCount = m_pBranchGeometry->m_vCoords.size() / 3u;
		sBranches->m_pColors = m_pBranchGeometry->GetVertexColors();
		sBranches->m_pCoords = m_pBranchGeometry->GetVertexCoords();
		sBranches->m_pNormals = m_pBranchGeometry->GetVertexNormals();
		sBranches->m_pBinormals = m_pBranchGeometry->GetVertexBinormals();
		sBranches->m_pTangents = m_pBranchGeometry->GetVertexTangents();
		sBranches->m_pTexCoords0 = m_pBranchGeometry->GetVertexTexCoords0();
		sBranches->m_pTexCoords1 = m_pBranchGeometry->GetVertexTexCoords1();
		sBranches->m_pWindWeights = m_pBranchGeometry->GetVertexWindWeights();
		sBranches->m_pWindMatrixIndices = m_pBranchGeometry->GetVertexWindMatrixIndices();
		if (sOverrideLodValue == -1) {
			sLodLevel = GetDiscreteBranchLodLevel(-1.0f);
		} else {
			sLodLevel = sOverrideLodValue;
		}
		sBranches->m_nDiscreteLodLevel = sLodLevel;
		sBranches->m_usNumStrips = m_pBranchGeometry->GetNumStrips(sLodLevel);
		sBranches->m_pStripLengths = m_pBranchGeometry->GetStripLengthsPointer(sLodLevel);
		sBranches->m_pStrips = const_cast<const uint16_t**>(m_pBranchGeometry->GetStripsPointer(sLodLevel));
		if (CSpeedTreeRT::m_bDropToBillboard) {
			int16_t sLowLod[2];
			int16_t sHighLod[2];
			uint16_t usLodStageCount = GetNumLeafLodLevels() + 1;
			float fHighLodAlphaValue = -1.0f;
			float fLowLodAlphaValue = -1.0f;
			sHighLod[0] = -1;
			sLowLod[0] = -1;
			float fTargetAlphaValue = static_cast<float>(m_ucTargetAlphaValue);
			float fCurveExponent = m_fLeafLodCurveExponent;
			float fTransitionFactor = m_fLeafTransitionFactor;
			float fOverlapRadius = m_fLeafLodTransitionRadius;
			float fLodLevel = GetLodLevel();
			CSpeedTreeRT::GetTransitionValues(fLodLevel, usLodStageCount, fOverlapRadius, fTransitionFactor,
				fCurveExponent, fTargetAlphaValue, fHighLodAlphaValue, fLowLodAlphaValue, *sHighLod, *sLowLod);
			if (sHighLod[0] == usLodStageCount - 2) {
				sGeometry.m_fBranchAlphaTestValue = fHighLodAlphaValue;
			} else {
				if (sHighLod[0] == usLodStageCount - 1) {
					sGeometry.m_fBranchAlphaTestValue = 255.0f;
					sBranches->m_nDiscreteLodLevel = -1;
				} else {
					sGeometry.m_fBranchAlphaTestValue = static_cast<float>(m_ucTargetAlphaValue);
				}
			}
		} else {
			sGeometry.m_fBranchAlphaTestValue = static_cast<float>(m_ucTargetAlphaValue);
		}
	} else {
		CSpeedTreeRT::SetError("no branch geometry exists, possible prior call to DeleteBranchGeometry");
	}
}

void CSpeedTreeRT::Get360BillboardGeometry(CSpeedTreeRT::SGeometry& sGeometry, uint32_t ulBitVector) {
	float fLowValue;
	float fHighValue;
	GetSimpleBillboardGeometry(sGeometry);
	if (sGeometry.m_sBillboard0.m_bIsActive) {
		if (this->m_usNum360Billboards) {
			float a1[3];
			int16_t sHighLod[2];
			int16_t sLowLod[2];
			float fFade =
				(sGeometry.m_sBillboard0.m_fAlphaTestValue - static_cast<double>(this->m_ucTargetAlphaValue)) /
				(255.0 - static_cast<double>(this->m_ucTargetAlphaValue));
			CVec3 cCameraPos = CIdvCamera::m_cCameraPos;
			float cCameraDir = -CIdvCamera::m_cCameraDir.m_afData[0];
			float cCameraDir_4 = -CIdvCamera::m_cCameraDir.m_afData[1];
			float cCameraDir_8 = -CIdvCamera::m_cCameraDir.m_afData[2];
			a1[0] = cCameraDir;
			a1[1] = cCameraDir_4;
			a1[2] = cCameraDir_8;
			float fAzimuth = ComputeAzimuth(a1);
			if (fAzimuth < 0.0) {
				fAzimuth = fAzimuth + 360.0;
			}
			fHighValue = -1.0f;
			fLowValue = -1.0f;
			sHighLod[0] = -1;
			sLowLod[0] = -1;
			uint16_t m_usNum360Billboards = this->m_usNum360Billboards;
			float fOverlapRadius = 0.5 / static_cast<double>(m_usNum360Billboards);
			float v5 = fAzimuth / 360.0;
			float v4 = 1.0 - fOverlapRadius;
			float fLodLevel = v5 * (v4 - fOverlapRadius) + fOverlapRadius;
			float fTargetAlphaValue = static_cast<float>(this->m_ucTargetAlphaValue);
			CSpeedTreeRT::GetTransitionValues(fLodLevel, m_usNum360Billboards, fOverlapRadius, m_fLeafTransitionFactor,
				m_fLeafLodCurveExponent, fTargetAlphaValue, fHighValue, fLowValue, sHighLod[0], sLowLod[0]);
			sHighLod[0] = this->m_usNum360Billboards - 1 - sHighLod[0];
			sLowLod[0] = this->m_usNum360Billboards - 1 - sLowLod[0];
			while (sHighLod[0] >= static_cast<int32_t>(m_usNum360Billboards)) {
				sHighLod[0] -= m_usNum360Billboards;
			}
			while (sLowLod[0] >= static_cast<int32_t>(m_usNum360Billboards)) {
				sLowLod[0] -= m_usNum360Billboards;
			}
			if ((ulBitVector & 32) != 0) {
				sGeometry.m_sBillboard0.m_bIsActive = 1;
				if (!(fLowValue > static_cast<double>(fHighValue))) {
					sGeometry.m_sBillboard0.m_pTexCoords = &m_pEmbeddedTexCoords->m_pBillboardTexCoords[8 * sLowLod[0]];
					float m_ucTargetAlphaValue = static_cast<float>(this->m_ucTargetAlphaValue);
					sGeometry.m_sBillboard0.m_fAlphaTestValue =
						fFade * (255.0f - m_ucTargetAlphaValue) + m_ucTargetAlphaValue;
				} else {
					sGeometry.m_sBillboard0.m_pTexCoords =
						&m_pEmbeddedTexCoords->m_pBillboardTexCoords[8 * sHighLod[0]];
					float v8 = static_cast<float>(this->m_ucTargetAlphaValue);
					sGeometry.m_sBillboard0.m_fAlphaTestValue = fFade * (255.0f - v8) + v8;
				}
				sGeometry.m_sBillboard1.m_bIsActive = 0;
			} else {
				if (m_pEmbeddedTexCoords) {
					sGeometry.m_sBillboard0.m_pTexCoords =
						&m_pEmbeddedTexCoords->m_pBillboardTexCoords[8 * sHighLod[0]];
				} else {
					sGeometry.m_sBillboard0.m_pTexCoords = 0;
				}
				sGeometry.m_sBillboard0.m_fAlphaTestValue = fFade * (255.0f - fHighValue) + fHighValue;
				sGeometry.m_sBillboard0.m_usWhich = sHighLod[0];
				sGeometry.m_sBillboard1.m_pCoords = sGeometry.m_sBillboard0.m_pCoords;
				if (m_pEmbeddedTexCoords) {
					sGeometry.m_sBillboard1.m_pTexCoords = &m_pEmbeddedTexCoords->m_pBillboardTexCoords[8 * sLowLod[0]];
				} else {
					sGeometry.m_sBillboard1.m_pTexCoords = 0;
				}
				sGeometry.m_sBillboard1.m_usWhich = sLowLod[0];
				sGeometry.m_sBillboard1.m_bIsActive = 1;
				sGeometry.m_sBillboard1.m_fAlphaTestValue = fFade * (255.0f - fLowValue) + fLowValue;
			}
		}
	}
}

// IDA's CTreeEngine::B0CB10; the PDB name by its body (this is the tree: the size increase factor at +0x24, the
// factors at +0x2C). The count is the out-of-line GetNumLeafLodLevels zero-extended; the size is new[]'s
// count * 4 saturated to 0xFFFFFFFF. Each factor is computed as a double (fild * the float factor + fld1) and
// stored as a float. The same try/catch as DeleteBranchGeometry. The checker's Windows models
// (harness/checker/emu.py MODELS) do not include RaiseException, so a call that throws ends as "unverifiable: calls
// RaiseException"; only paths that don't throw are compared.
const float* CSpeedTreeRT::GetLeafLodSizeAdjustments() {
	try {
		if (!m_pLeafLodSizeFactors) {
			int32_t nNumLeafLods = (uint16_t)GetNumLeafLodLevels();
			size_t uiSize =
				(uint32_t)nNumLeafLods > SIZE_MAX / sizeof(float) ? SIZE_MAX : sizeof(float) * (uint32_t)nNumLeafLods;
			m_pLeafLodSizeFactors = static_cast<float*>(MemoryManager_Singleton.Allocate(uiSize));
			for (int32_t i = 0; i < nNumLeafLods; ++i) {
				double dFactor;
				if (nNumLeafLods > 0) {
					dFactor = (double)i * (double)m_fLeafSizeIncreaseFactor + 1.0;
				} else {
					dFactor = 1.0;
				}
				m_pLeafLodSizeFactors[i] = (float)dFactor;
			}
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError = IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::GetLeafLodSizeAdjustments",
			cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError = IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::GetLeafLodSizeAdjustments",
			cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError = IdvFormatString(&strError, "%s - threw an unknown system exception",
			"CSpeedTreeRT::GetLeafLodSizeAdjustments");
		SetError(pError->c_str());
	}
	return m_pLeafLodSizeFactors;
}

// The leaf textures' count is the byte difference divided signed by 0x54 (size(); a pointer difference would be an
// exact division); the loop re-derives it each pass and compares unsigned, with the inline checked subscript. The
// frond count divides by 0x2C the same way; the frond loop compares against the stored count and calls the frond
// engine's out-of-line GetTextureFilename. Array sizes are new[]'s count * 4 saturated to 0xFFFFFFFF.
// The same try/catch as DeleteBranchGeometry. The checker's Windows models (harness/checker/emu.py MODELS) do not
// include RaiseException, so a call that throws ends as "unverifiable: calls RaiseException"; only paths that don't
// throw are compared.
void CSpeedTreeRT::GetTextures(STextures& sTextures) {
	try {
		CIdvFilename* pBranchTexture = &m_pEngine->m_sTreeInfo.m_strBranchTextureFilename;
		sTextures.m_pBranchTextureFilename = pBranchTexture->c_str();
		std::vector<SIdvLeafTexture, std::allocator<SIdvLeafTexture>>* pLeafTextures =
			&m_pEngine->m_sLeafInfo.m_vLeafTextures;
		sTextures.m_uiLeafTextureCount = (uint32_t)pLeafTextures->size();
		if (sTextures.m_uiLeafTextureCount) {
			uint32_t uiCount = sTextures.m_uiLeafTextureCount;
			size_t uiSize = uiCount > SIZE_MAX / sizeof(const char*) ? SIZE_MAX : sizeof(const char*) * uiCount;
			sTextures.m_pLeafTextureFilenames = static_cast<const char**>(MemoryManager_Singleton.Allocate(uiSize));
			for (uint32_t i = 0; i < (uint32_t)pLeafTextures->size(); ++i) {
				// operator[], inline: the range check, then the element.
				CIdvFilename* pFilename = &(*pLeafTextures)[i].m_strFilename;
				sTextures.m_pLeafTextureFilenames[i] = pFilename->c_str();
			}
		} else {
			sTextures.m_pLeafTextureFilenames = nullptr;
		}
		std::vector<SFrondTexture, std::allocator<SFrondTexture>>* pFrondTextures = &m_pFrondEngine->m_vTextures;
		sTextures.m_uiFrondTextureCount = (uint32_t)pFrondTextures->size();
		if (sTextures.m_uiFrondTextureCount) {
			uint32_t uiCount = sTextures.m_uiFrondTextureCount;
			size_t uiSize = uiCount > SIZE_MAX / sizeof(const char*) ? SIZE_MAX : sizeof(const char*) * uiCount;
			sTextures.m_pFrondTextureFilenames = static_cast<const char**>(MemoryManager_Singleton.Allocate(uiSize));
			for (uint32_t j = 0; j < sTextures.m_uiFrondTextureCount; ++j) {
				sTextures.m_pFrondTextureFilenames[j] = m_pFrondEngine->GetTextureFilename(j);
			}
		} else {
			sTextures.m_pFrondTextureFilenames = nullptr;
		}
		if (m_pProjectedShadow) {
			std::string* pSelfShadowMap = &m_pProjectedShadow->m_strSelfShadowMap;
			sTextures.m_pSelfShadowFilename = pSelfShadowMap->c_str();
		}
		if (m_pEmbeddedTexCoords) {
			std::string* pComposite = &m_pEmbeddedTexCoords->m_strCompositeFilename;
			sTextures.m_pCompositeFilename = pComposite->c_str();
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::GetTextures", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::GetTextures", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::GetTextures");
		SetError(pError->c_str());
	}
}

void CSpeedTreeRT::SetError(const char* pError) {
	size_t uiLength = std::strlen(pError);
	g_strError.assign(pError, uiLength);
}

bool CSpeedTreeRT::GetTextureFlip() {
	return m_bTextureFlip;
}

void CSpeedTreeRT::SetTextureFlip(bool bFlag) {
	m_bTextureFlip = bFlag;
}

void CSpeedTreeRT::GetTransitionValues(float fLodLevel, uint16_t usLodCount, float fOverlapRadius,
	float fTransitionFactor, float fCurveExponent, float fTargetAlphaValue, float& fHighValue, float& fLowValue,
	int16_t& sHighLod, int16_t& sLowLod) {
	float v10;
	float v11;
	float v12;
	float v13;
	float v14;
	float v15;
	int16_t v16;
	float v17;
	float v21;
	float v22;
	float fTransitionStage;
	float fLodWidth = (float)(1.0 / ((double)usLodCount));
	float v18 = (float)((1.0 - fLodLevel) / fLodWidth);
	int32_t usDiscreteTransitionPoint = (int32_t)v18;
	float v19 = (float)(v18 - ((double)usDiscreteTransitionPoint));
	if (v19 >= 0.5) {
		usDiscreteTransitionPoint =
			static_cast<int32_t>((static_cast<uint32_t>(usDiscreteTransitionPoint) & 0xffff0000u) |
				static_cast<uint32_t>((uint16_t)(usDiscreteTransitionPoint + 1)));
	}
	float fDistanceToTransitionPoint =
		(float)((1.0 - fLodLevel) - (((double)((uint16_t)usDiscreteTransitionPoint)) * fLodWidth));
	if (((uint16_t)usDiscreteTransitionPoint) &&
		((((uint16_t)usDiscreteTransitionPoint) != usLodCount) &&
			((v17 = (float)std::fabs((double)fDistanceToTransitionPoint)), (!(fOverlapRadius < ((double)v17)))))) {
		fTransitionStage = (float)(1.0 - ((fOverlapRadius - fDistanceToTransitionPoint) / (fOverlapRadius * 2.0)));
		v21 = (float)(1.0 - ((fTransitionStage - fTransitionFactor) / (1.0 - fTransitionFactor)));
		if (!(v21 < 1.0)) {
			v15 = (float)1.0;
		} else {
			v15 = (float)(1.0 - ((fTransitionStage - fTransitionFactor) / (1.0 - fTransitionFactor)));
		}
		sHighLod = (int16_t)(usDiscreteTransitionPoint - 1);
		v14 = (float)(1.0 - v15);
		v13 = (float)std::pow((double)v14, (double)fCurveExponent);
		fHighValue = (float)((v13 * (255.0f - fTargetAlphaValue)) + fTargetAlphaValue);
		v22 = (float)(fTransitionStage / (1.0 - fTransitionFactor));
		if (!(v22 < 1.0)) {
			v12 = (float)1.0;
		} else {
			v12 = (float)(fTransitionStage / (1.0 - fTransitionFactor));
		}
		sLowLod = (int16_t)usDiscreteTransitionPoint;
		v11 = (float)(1.0 - v12);
		v10 = (float)std::pow((double)v11, (double)fCurveExponent);
		fLowValue = (float)((v10 * (255.0f - fTargetAlphaValue)) + fTargetAlphaValue);
	} else {
		fHighValue = fTargetAlphaValue;
		sHighLod = (int16_t)((int32_t)((1.0 - fLodLevel) * ((double)usLodCount)));
		if (((int32_t)(sHighLod)) >= ((int32_t)((int16_t)(usLodCount - 1)))) {
			v16 = (int16_t)(usLodCount - 1);
		} else {
			v16 = sHighLod;
		}
		sHighLod = v16;
		fLowValue = 255.0f;
		sLowLod = (-1);
	}
}

void CSpeedTreeRT::SetDropToBillboard(bool bFlag) {
	CSpeedTreeRT::m_bDropToBillboard = (bool)((uint8_t)bFlag);
}

// The exe copies the camera's static vectors to locals, then memcpys them out.
void CSpeedTreeRT::GetCamera(float* pPosition, float* pDirection) {
	if (pPosition && pDirection) {
		CVec3 cCameraPos = CIdvCamera::m_cCameraPos;
		CVec3 cCameraDirection = CIdvCamera::m_cCameraDir;
		rt::memcpy(pPosition, &cCameraPos, sizeof(CVec3));
		rt::memcpy(pDirection, &cCameraDirection, sizeof(CVec3));
	} else {
		SetError(const_cast<char*>("GetCamera() requires non-NULL position and direction values"));
	}
}

void CSpeedTreeRT::operator delete(void* apMemory) {
	ourFree(apMemory);
}

// IDA's _sub_B07DB0; the PDB name by its place between SetWindStrength and SetLocalMatrices (speedtree-2's order)
// and its body (CWindEngine::SetNumWindMatrices inlined to the shared matrices' Resize, the count's low word). The
// same try/catch as SetTreeSize, with this function's name. The checker's Windows models (harness/checker/emu.py
// MODELS) do not include RaiseException, so a call that throws ends as "unverifiable: calls RaiseException"; only
// paths that don't throw are compared.
void CSpeedTreeRT::SetNumWindMatrices(int32_t nNumMatrices) {
	try {
		CWindEngine::m_cWindMatrices.Resize(static_cast<uint16_t>(nNumMatrices));
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetNumWindMatrices", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetNumWindMatrices", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::SetNumWindMatrices");
		SetError(pError->c_str());
	}
}

// speedtree-2's SetCamera with CTreeEngine::GetCamera/SetCamera and the CVec3 compares inlined (each component
// "not equal or unordered": test ah,0x44; jp), plus Bethesda's horizontal billboard fade after the two notifies: the
// direction's z through asin (a double call, the result stored as a float), times 57.295780181884766 (a double) as a
// float, then 1 - (angle - start) / (end - start) computed on the x87 stack (kept as double here) and stored as a
// float, clamped below at 0 ("less than" the double 0.0 only: NaN is kept) and above at 1 ("greater than" the double
// 1.0 only), each through a float local, and mapped to 84..255. The same try/catch as SetTreeSize, with this
// function's name. The checker's Windows models (harness/checker/emu.py MODELS) do not include RaiseException, so a
// call that throws ends as "unverifiable: calls RaiseException"; only paths that don't throw are compared.
void CSpeedTreeRT::SetCamera(const float* pPosition, const float* pDirection) {
	try {
		if (pPosition && pDirection) {
			CVec3 cOldPosition = CIdvCamera::m_cCameraPos;
			CVec3 cOldDirection = CIdvCamera::m_cCameraDir;
			float fPositionX = pPosition[0];
			float fPositionY = pPosition[1];
			float fPositionZ = pPosition[2];
			CVec3 cNewPosition;
			cNewPosition.m_afData[0] = fPositionX;
			cNewPosition.m_afData[1] = fPositionY;
			cNewPosition.m_afData[2] = fPositionZ;
			float fDirectionX = pDirection[0];
			float fDirectionY = pDirection[1];
			float fDirectionZ = pDirection[2];
			CVec3 cNewDirection;
			cNewDirection.m_afData[0] = fDirectionX;
			cNewDirection.m_afData[1] = fDirectionY;
			cNewDirection.m_afData[2] = fDirectionZ;
			bool bPositionChanged = cNewPosition.m_afData[0] != cOldPosition.m_afData[0] ||
				cNewPosition.m_afData[1] != cOldPosition.m_afData[1] ||
				cNewPosition.m_afData[2] != cOldPosition.m_afData[2];
			bool bChanged = bPositionChanged;
			if (!bChanged) {
				bChanged = cNewDirection.m_afData[0] != cOldDirection.m_afData[0] ||
					cNewDirection.m_afData[1] != cOldDirection.m_afData[1] ||
					cNewDirection.m_afData[2] != cOldDirection.m_afData[2];
			}
			if (bChanged) {
				CIdvCamera::m_cCameraPos = cNewPosition;
				CIdvCamera::m_cCameraDir = cNewDirection;
				NotifyAllTreesOfEvent(STIE_CLIENT_CHANGED_CAMERA);
				CSimpleBillboard::ComputeUnitBillboard(cNewDirection.m_afData);
				NotifyAllTreesOfEvent(STIE_CLIENT_CHANGED_CAMERA);
				CVec3 cDirection;
				cDirection.m_afData[0] = cNewDirection.m_afData[0];
				cDirection.m_afData[1] = cNewDirection.m_afData[1];
				cDirection.m_afData[2] = cNewDirection.m_afData[2];
				float fZ = cDirection.m_afData[2];
				float fAngle = static_cast<float>(std::asin(static_cast<double>(fZ)));
				float fDegrees = static_cast<float>(fAngle * 57.295780181884766);
				float fFade = static_cast<float>(1.0 -
					(static_cast<double>(fDegrees) - m_fHorizontalFadeStartAngle) /
						(static_cast<double>(m_fHorizontalFadeEndAngle) - m_fHorizontalFadeStartAngle));
				float fClamped;
				if (fFade < 0.0) {
					fClamped = 0.0f;
				} else {
					fClamped = fFade;
				}
				fFade = fClamped;
				if (fFade > 1.0) {
					fClamped = 1.0f;
				} else {
					fClamped = fFade;
				}
				fFade = fClamped;
				// The exe computes 255.0f - 84.0f at run time (171 exactly).
				m_fHorizontalFadeValue = static_cast<float>(fFade * static_cast<double>(255.0f - 84.0f) + 84.0f);
			}
		} else {
			SetError(const_cast<char*>("SetCamera() requires non-NULL position and direction values"));
		}
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetCamera", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetCamera", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::SetCamera");
		SetError(pError->c_str());
	}
}

// speedtree-2's SetTime: CWindEngine::SetTime inlined (the shared time stored). The same try/catch as SetTreeSize,
// with this function's name. The checker's Windows models (harness/checker/emu.py MODELS) do not include
// RaiseException, so a call that throws ends as "unverifiable: calls RaiseException"; only paths that don't throw
// are compared.
void CSpeedTreeRT::SetTime(float fTime) {
	try {
		CWindEngine::m_fTime = fTime;
		NotifyAllTreesOfEvent(STIE_CLIENT_CHANGED_WIND);
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetTime", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::SetTime", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::SetTime");
		SetError(pError->c_str());
	}
}

// map.tsv st2:differs: the exe walks the unique trees inside their critical section (Enter/Leave through the
// imports). The iterator comes from the out-of-line checked constructor, end() is rebuilt and compared out of line
// each pass, and every element access calls the out-of-line operator*. The same try/catch as DeleteBranchGeometry;
// an exception leaves the critical section held, as in the exe. The checker's Windows models
// (harness/checker/emu.py MODELS) do not include RaiseException, so a call that throws ends as "unverifiable: calls
// RaiseException"; only paths that don't throw are compared.
void CSpeedTreeRT::NotifyAllTreesOfEvent(int32_t nMessage) {
	using TreeIterator = SInstanceList::TreeVector::iterator;
	// begin() and end(), inline: the const iterator's out-of-line checked constructor.
	try {
		EnterCriticalSection(&SInstanceList::m_UniqueTreesCritSection.CriticalSection);
		TreeIterator kIter = SInstanceList::m_vUniqueTrees.begin();
		for (;; ++kIter) {
			TreeIterator kEnd = SInstanceList::m_vUniqueTrees.end();
			if (kIter == kEnd) {
				break;
			}
			if (nMessage == STIE_CLIENT_CHANGED_WIND) {
				if ((*kIter)->GetBranchWindMethod() == WIND_CPU) {
					CIndexedGeometry* pBranchGeometry = (*kIter)->m_pBranchGeometry;
					pBranchGeometry->m_bValid = false;
				}
				if ((*kIter)->GetFrondWindMethod() == WIND_CPU) {
					CIndexedGeometry* pFrondGeometry = (*kIter)->m_pFrondGeometry;
					pFrondGeometry->m_bValid = false;
				}
				if ((*kIter)->GetLeafWindMethod() == WIND_CPU || (*kIter)->m_pWindEngine->m_bRockingLeaves) {
					(*kIter)->m_pLeafGeometry->Invalidate();
				}
			} else if (nMessage == STIE_CLIENT_CHANGED_CAMERA) {
				(*kIter)->m_pLeafGeometry->Invalidate();
				CSimpleBillboard* pSimpleBillboard = (*kIter)->m_pSimpleBillboard;
				pSimpleBillboard->m_bValid = false;
			}
		}
		LeaveCriticalSection(&SInstanceList::m_UniqueTreesCritSection.CriticalSection);
	} catch (rt::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::NotifyAllTreesOfEvent", cException.what());
		SetError(pError->c_str());
	} catch (std::exception& cException) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - failed [%s]", "CSpeedTreeRT::NotifyAllTreesOfEvent", cException.what());
		SetError(pError->c_str());
	} catch (...) {
		std::string strError;
		std::string* pError =
			IdvFormatString(&strError, "%s - threw an unknown system exception", "CSpeedTreeRT::NotifyAllTreesOfEvent");
		SetError(pError->c_str());
	}
}

// The copy is a char array of the text's length plus its terminator.
char* CSpeedTreeRT::CopyUserData(const char* pOrig) {
	int32_t nLength = std::strlen(pOrig);
	char* pCopy = static_cast<char*>(MemoryManager_Singleton.Allocate(static_cast<size_t>(nLength + 1)));
	rt::memcpy(pCopy, pOrig, nLength + 1);
	return pCopy;
}

// The direction is copied into a zeroed CVec3 first; atan2's result is rounded to float before the radian-to-degree
// scaling by a double constant, and the float result is returned in ST(0).
float ComputeAzimuth(const float* pCameraDir) {
	CVec3 cDirection;
	cDirection.m_afData[2] = 0.0f;
	cDirection.m_afData[1] = 0.0f;
	cDirection.m_afData[0] = 0.0f;
	cDirection.m_afData[0] = pCameraDir[0];
	cDirection.m_afData[1] = pCameraDir[1];
	cDirection.m_afData[2] = pCameraDir[2];
	float fY = cDirection.m_afData[1];
	float fX = cDirection.m_afData[0];
	float fRadians = static_cast<float>(std::atan2(static_cast<double>(fY), static_cast<double>(fX)));
	float fDegrees = static_cast<float>(fRadians * 57.295780181884766);
	return fDegrees;
}

SShape* UncheckedCopyBackward(SShape* a1, SShape* a2, SShape* a3) {
	return ::CopyBackwardOpt(a1, a2, a3);
}

// _Copy_backward_opt over SShape: the elements of [_First, _Last) copied backward to end at _Dest, each a 28-byte
// assignment the exe inlines; returns the destination's start.
SShape* CopyBackwardOpt(SShape* _First, SShape* _Last, SShape* _Dest) {
	SShape* v4 = _Dest;
	for (SShape* i = _Last; _First != i; *v4 = *i) {
		--i;
		--v4;
	}
	return &_Dest[-(_Last - _First)];
}

// vector::push_back of CSpeedTreeRT* (0xB0DDC0): a forwarder below; vector.h's template would not register it (it
// copies an uninitialised tag byte within its frame).
// 0xB0E090 (SShape) is a forwarder below as well; its template instance would pass the uninitialised byte on to
// _Uninit_fill_n.
uint8_t* UninitFillN(uint8_t* a1, int32_t a2, uint8_t* a3) {
	_Fill_n(a1, a2, a3);
	return &a1[a2];
}

uint8_t* CopyOpt(const uint8_t* a1, const uint8_t* a2, uint8_t* a3) {
	if (a2 - a1 > 0) {
		memmove_s(a3, a2 - a1, a1, a2 - a1);
	}
	return &a3[a2 - a1];
}

// The signed byte count decides the copy (jle), as VC9's _Copy_backward_opt does. KEEP, unregistered (an
// uninitialised read): the inlined helpers' tag temporaries copy frame bytes nothing writes ([ebp-4] into [ebp-2],
// [ebp-3] into [ebp-0x35], [ebp-0x17] into [ebp-0x16]).
uint8_t* CopyBackwardOpt(uint8_t* a1, uint8_t* a2, uint8_t* a3) {
	ptrdiff_t _Count = a2 - a1;
	uint8_t* _Dest = a3 - _Count;
	if (_Count > 0) {
		memmove_s(_Dest, _Count, a1, _Count);
	}
	return _Dest;
}

uint8_t* UninitCopy(uint8_t* a1, uint8_t* a2, uint8_t* a3, [[maybe_unused]] std::allocator<uint8_t>& a4) {
	if (a2 != a1) {
		memmove_s(a3, a2 - a1, a1, a2 - a1);
	}
	return &a3[a2 - a1];
}

char _Fill_n(uint8_t* a1, int32_t a2, const uint8_t* a3) {
	int8_t result = 0;
	while (a2) {
		*a1 = *a3;
		result = (int8_t)(--a2);
		++a1;
	}
	return result;
}

// The shapes' iterator through the checked iterator's out-of-line members (their one body for every element type),
// operator* once per element access, as the exe calls it. The position is read through three accesses (the
// CVec3(x, y, z) arguments, the last first), transformed (0xB05000) and stored back; a sphere's radius and a
// cylinder's two dimensions scale by the transform's first diagonal entry, a box's three by the three diagonal
// entries.
void SCollisionObjects::TransformAll(CTransform& cTransform) {
	using ShapeIterator = SShapeVector::iterator;
	ShapeIterator kIter = m_vObjects.begin();
	for (;; ++kIter) {
		ShapeIterator kEnd = m_vObjects.end();
		if (kIter == kEnd) {
			break;
		}
		const SShape& sForZ = *kIter;
		const SShape& sForY = *kIter;
		const SShape& sForX = *kIter;
		CVec3 cPosition;
		cPosition.m_afData[0] = sForX.m_afPosition[0];
		cPosition.m_afData[1] = sForY.m_afPosition[1];
		cPosition.m_afData[2] = sForZ.m_afPosition[2];
		cPosition = cPosition * cTransform;
		(*kIter).m_afPosition[0] = cPosition.m_afData[0];
		(*kIter).m_afPosition[1] = cPosition.m_afData[1];
		(*kIter).m_afPosition[2] = cPosition.m_afData[2];
		if ((*kIter).m_eType == CSpeedTreeRT::CO_SPHERE) {
			(*kIter).m_afDimensions[0] *= cTransform.m_afData[0][0];
		} else if ((*kIter).m_eType == CSpeedTreeRT::CO_CYLINDER) {
			(*kIter).m_afDimensions[0] *= cTransform.m_afData[0][0];
			(*kIter).m_afDimensions[1] *= cTransform.m_afData[0][0];
		} else if ((*kIter).m_eType == CSpeedTreeRT::CO_BOX) {
			(*kIter).m_afDimensions[0] *= cTransform.m_afData[0][0];
			(*kIter).m_afDimensions[1] *= cTransform.m_afData[1][1];
			(*kIter).m_afDimensions[2] *= cTransform.m_afData[2][2];
		}
	}
}

// The base string is built empty, then given the name; the by-value parameter is destroyed by the caller's
// compiler-generated code.
CIdvFilename::CIdvFilename(std::basic_string<char, std::char_traits<char>, std::allocator<char>> strFilename) {
	assign(strFilename, 0, std::string::npos);
}

CIdvFilename::~CIdvFilename() {}

// The exe's cleanup-only C++ EH frame is left out. The name after the last '/' or '\\' (the whole name without
// either), each character read through the checked operator[] (inlined); the result string is built in apResult,
// its constructor inlined.
CIdvFilename* CIdvFilename::NoPath(CIdvFilename* apResult) {
	char szBuffer[1028];
	std::strcpy(szBuffer, c_str());
	int32_t nLength = static_cast<int32_t>(size());
	for (int32_t i = nLength - 1; i >= 0; --i) {
		if ((*this)[i] == '/' || (*this)[i] == '\\') {
			int32_t j = i + 1;
			int32_t k = 0;
			for (; j < nLength; ++j, ++k) {
				szBuffer[k] = (*this)[j];
			}
			szBuffer[k] = '\0';
			break;
		}
	}
	std::string strNoPath;
	FNVD_RECONSTRUCT(std::string)(&strNoPath, szBuffer);
	// The copy constructor, inlined (_Tidy(false, 0), then assign).
	::new (apResult) std::string(strNoPath);
	return apResult;
}

// The exe rebuilds the string as its copy constructor does (_Tidy(false, 0), then assign), inlined: the old text is
// not freed, and a self-assignment leaves the name empty. The standalone build assigns: the old text is freed and a
// self-assignment keeps the name (destroying and copy-constructing in place would read the destroyed string).
CIdvFilename& CIdvFilename::operator=(const CIdvFilename& aRight) {
	std::string::operator=(aRight);
	return *this;
}

bool CTreeFileAccess::ParseBool() {
	int32_t nIndex = m_nIndex;
	m_nIndex = m_nIndex + 1;
	return m_vMemory[nIndex] != 0;
}

// The vector's checked operator[] and the string's c_str (its _Myptr selection) inlined.
const char* CFrondEngine::GetTextureFilename(uint32_t nIndex) const {
	const std::string& strFilename = m_vTextures[nIndex].m_strFilename;
	return strFilename.c_str();
}

CIndexedGeometry::CIndexedGeometry(CWindEngine* pWindEngine, bool bRetainTexCoords)
	: m_bRetainTexCoords(bRetainTexCoords), m_pWindEngine(pWindEngine), m_bVertexWeighting(false),
	  m_bManualLighting(false), m_pVertexWindComputed(nullptr), m_nVertexSize(0), m_bValid(true),
	  m_eWindMethod(CSpeedTreeRT::WIND_NONE), m_nNumLodLevels(0), m_nVertexCounter(0), m_nVertexCounterLodLevel(0),
	  m_nStripCounter(0) {}

// CIndexedGeometry's array getters. Each exe body also holds an unreachable _SCL range check.
uint16_t CIndexedGeometry::GetNumStrips(int16_t sLodLevel) const {
	if (sLodLevel > -1) {
		return (uint16_t)m_vStrips[sLodLevel].size();
	}
	return 0;
}

const uint16_t* CIndexedGeometry::GetStripLengthsPointer(int16_t sLodLevel) const {
	if (sLodLevel >= 0 && sLodLevel < m_nNumLodLevels && !m_vStripLengths[sLodLevel].empty()) {
		return &m_vStripLengths[sLodLevel][0];
	}
	return nullptr;
}

const uint32_t* CIndexedGeometry::GetVertexColors() const {
	return m_vColors.empty() ? nullptr : &m_vColors[0];
}

const float* CIndexedGeometry::GetVertexCoords() const {
	return m_vCoords.empty() ? nullptr : &m_vCoords[0];
}

const float* CIndexedGeometry::GetVertexNormals() const {
	return m_vNormals.empty() ? nullptr : &m_vNormals[0];
}

const float* CIndexedGeometry::GetVertexBinormals() const {
	return m_vBinormals.empty() ? nullptr : &m_vBinormals[0];
}

const float* CIndexedGeometry::GetVertexTangents() const {
	return m_vTangents.empty() ? nullptr : &m_vTangents[0];
}

const float* CIndexedGeometry::GetVertexTexCoords0() const {
	return m_vTexCoords0.empty() ? nullptr : &m_vTexCoords0[0];
}

const float* CIndexedGeometry::GetVertexTexCoords1() const {
	return m_vTexCoords1.empty() ? nullptr : &m_vTexCoords1[0];
}

const float* CIndexedGeometry::GetVertexWindWeights() const {
	return m_vWindWeights.empty() ? nullptr : &m_vWindWeights[0];
}

const uint8_t* CIndexedGeometry::GetVertexWindMatrixIndices() const {
	return m_vWindMatrixIndices.empty() ? nullptr : &m_vWindMatrixIndices[0];
}

// IdvFileError (LibGlobals_Source/IdvGlobals.h; the PDB defines it here). The C++ constructors and implicit
// destructor are the exe's (IdvGlobals.h says why). The exe's cleanup-only EH frames are left out.
IdvFileError::IdvFileError(const std::string& strDetails, bool bAppendSystemError)
	: IdvFileError(strDetails, bAppendSystemError, MessageStrings{}) {}

IdvFileError::IdvFileError(const IdvFileError& cThat) : runtime_error(cThat) {}

// The exe frees the strings after the runtime_error constructor, the last built first, then sets the vftable.
IdvFileError::IdvFileError(const std::string& strDetails, bool bAppendSystemError, MessageStrings&& aStrings)
	: runtime_error(BuildMessage(strDetails, bAppendSystemError, aStrings)) {
	FNVD_DESTRUCT_IN_PLACE(std::string)(&aStrings[3]);
	FNVD_DESTRUCT_IN_PLACE(std::string)(&aStrings[2]);
	FNVD_DESTRUCT_IN_PLACE(std::string)(&aStrings[1]);
	FNVD_DESTRUCT_IN_PLACE(std::string)(&aStrings[0]);
}

void IdvFileError::operator delete(void* apMemory) {
	MemoryManager_Singleton.Deallocate(apMemory);
}

// The error text is strerror(errno), else the engine's empty string (the exe passes its address).
const std::string& IdvFileError::BuildMessage(const std::string& strDetails, bool bAppendSystemError,
	MessageStrings& aStrings) {
	if (bAppendSystemError) {
		FNVD_RECONSTRUCT(std::string)(&aStrings[0], std::strerror(errno));
	} else {
		FNVD_RECONSTRUCT(std::string)(&aStrings[0], "");
	}
	// operator+ (the exe's out-of-line _String_concat builds each result in its slot).
	aStrings[1] = strDetails + " [";
	aStrings[2] = aStrings[1] + aStrings[0];
	aStrings[3] = aStrings[2] + "]";
	return aStrings[3];
}

// The material arrays: diffuse, ambient, then zero specular, emissive and shininess.
CLightingEngine::CLightingEngine()
	: m_eBranchLightingMethod(CSpeedTreeRT::LIGHT_DYNAMIC),
	  m_afBranchMaterial{0.8f, 0.8f, 0.8f, 0.2f, 0.2f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
	  m_eLeafLightingMethod(CSpeedTreeRT::LIGHT_DYNAMIC),
	  m_afLeafMaterial{1.0f, 1.0f, 1.0f, 0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
	  m_fLeafLightingAdjustmentScalar(0.5f), m_eStaticLightingStyle(CSpeedTreeRT::SLS_BASIC),
	  m_eFrondLightingMethod(CSpeedTreeRT::LIGHT_DYNAMIC),
	  m_afFrondMaterial{1.0f, 1.0f, 1.0f, 0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f} {}

void CLightingEngine::ComputeLeafStaticLighting(const CVec3& cTreeCenter,
	std::vector<CBillboardLeaf*, std::allocator<CBillboardLeaf*>>* pLeafLods, int32_t nNumLeafLods) {
	CVec3 v20;
	CVec3 v21;
	CVec3 apResult;
	CVec3 v23;
	using LeafVector = std::vector<CBillboardLeaf*, std::allocator<CBillboardLeaf*>>;
	CVec3 cDiffuse;
	CVec3 cEmissive;
	CVec3 cAmbient;
	CVec3 cLightPos;
	if (m_eLeafLightingMethod == CSpeedTreeRT::LIGHT_STATIC) {
		if (m_eStaticLightingStyle) {
			for (int32_t nLod = 0; nLod < nNumLeafLods; ++nLod) {
				// begin() and end() of the const vector, inline: the checked constructor called out of line.
				const LeafVector& vLeaves = pLeafLods[nLod];
				LeafVector::const_iterator hProcess = vLeaves.begin();
				while (1) {
					CVec3 cDominantLight;
					CVec3 cColor;
					LeafVector::const_iterator apMemory = vLeaves.end();
					if (hProcess == apMemory) {
						break;
					}
					const CVec3* cNormal = &hProcess.operator*()->m_cNormal;
					const CVec3* cVertex = &hProcess.operator*()->m_cPosition;
					CVec3 cDiffuseColor = hProcess.operator*()->GetColor();
					cDominantLight.m_afData[2] = 0.0f;
					cDominantLight.m_afData[1] = 0.0f;
					cDominantLight.m_afData[0] = 0.0f;
					float fMaxDotProduct = -1.0f;
					cColor.m_afData[2] = 0.0f;
					cColor.m_afData[1] = 0.0f;
					cColor.m_afData[0] = 0.0f;
					for (int32_t nLight = 0; nLight < 8; ++nLight) {
						if (CLightingEngine::m_abLightEnabled[nLight]) {
							CVec3* p_cLightPos;
							float v18 = CLightingEngine::m_afLightAttributes[nLight][1];
							float v19 = CLightingEngine::m_afLightAttributes[nLight][2];
							cLightPos.m_afData[0] = CLightingEngine::m_afLightAttributes[nLight][0];
							cLightPos.m_afData[1] = v18;
							cLightPos.m_afData[2] = v19;
							if (CLightingEngine::m_afLightAttributes[nLight][12] == 0.0) {
								p_cLightPos = &cLightPos;
							} else {
								p_cLightPos = cLightPos.Diff(&v23, cVertex);
							}
							CVec3 cLightVector = *p_cLightPos;
							cLightVector.Normalize();
							float fDotProduct = cLightVector.operator*(*cNormal);
							if (fMaxDotProduct <= static_cast<double>(fDotProduct)) {
								fMaxDotProduct = fDotProduct;
								cDominantLight = cLightPos;
							}
							if (fDotProduct < 0.0) {
								fDotProduct = 0.0f;
							}
							fDotProduct =
								fDotProduct * (1.0 - m_fLeafLightingAdjustmentScalar) + m_fLeafLightingAdjustmentScalar;
							if ((m_eStaticLightingStyle & CSpeedTreeRT::SLS_USE_LIGHT_SOURCES) != 0) {
								float v16 = m_afLeafMaterial[10];
								float v17 = m_afLeafMaterial[11];
								cEmissive.m_afData[0] = m_afLeafMaterial[9];
								cEmissive.m_afData[1] = v16;
								cEmissive.m_afData[2] = v17;
								float v13 = m_afLeafMaterial[3] * CLightingEngine::m_afLightAttributes[nLight][6];
								float v14 = m_afLeafMaterial[4] * CLightingEngine::m_afLightAttributes[nLight][7];
								float v15 = m_afLeafMaterial[5] * CLightingEngine::m_afLightAttributes[nLight][8];
								cAmbient.m_afData[0] = v13;
								cAmbient.m_afData[1] = v14;
								cAmbient.m_afData[2] = v15;
								float fAttenuation = 1.0f;
								if (CLightingEngine::m_afLightAttributes[nLight][12] != 0.0) {
									float fDistance = cVertex->Distance(cLightPos);
									float fDenominator = CLightingEngine::m_afLightAttributes[nLight][14] * fDistance +
										CLightingEngine::m_afLightAttributes[nLight][13] +
										CLightingEngine::m_afLightAttributes[nLight][15] * fDistance * fDistance;
									if (fDenominator != 0.0) {
										fAttenuation = 1.0 / fDenominator;
									}
								}
								float v10 = cDiffuseColor.m_afData[0] *
									(CLightingEngine::m_afLightAttributes[nLight][3] * fDotProduct);
								float v11 = cDiffuseColor.m_afData[1] *
									(CLightingEngine::m_afLightAttributes[nLight][4] * fDotProduct);
								float v12 = cDiffuseColor.m_afData[2] *
									(CLightingEngine::m_afLightAttributes[nLight][5] * fDotProduct);
								cDiffuse.m_afData[0] = v10;
								cDiffuse.m_afData[1] = v11;
								cDiffuse.m_afData[2] = v12;
								float v7 = fAttenuation;
								CVec3* v4 = cAmbient.Add(&apResult, &cDiffuse);
								CVec3* v5 = v4->Scale(&v21, v7);
								CVec3* v6 = cEmissive.Add(&v20, v5);
								cColor.operator+=(*v6);
							}
						}
					}
					if (!(cColor.m_afData[0] < 0.0)) {
						if (cColor.m_afData[0] > 1.0) {
							cColor.m_afData[0] = 1.0f;
						}
					} else {
						cColor.m_afData[0] = 0.0f;
					}
					if (!(cColor.m_afData[1] < 0.0)) {
						if (cColor.m_afData[1] > 1.0) {
							cColor.m_afData[1] = 1.0f;
						}
					} else {
						cColor.m_afData[1] = 0.0f;
					}
					if (!(cColor.m_afData[2] < 0.0)) {
						if (cColor.m_afData[2] > 1.0) {
							cColor.m_afData[2] = 1.0f;
						}
					} else {
						cColor.m_afData[2] = 0.0f;
					}
					if ((m_eStaticLightingStyle & CSpeedTreeRT::SLS_USE_LIGHT_SOURCES) != 0) {
						hProcess.operator*()->SetColor(cColor, 0);
					}
					if ((m_eStaticLightingStyle & CSpeedTreeRT::SLS_SIMULATE_SHADOWS) != 0) {
						hProcess.operator*()->AdjustStaticLighting(cTreeCenter, cDominantLight,
							m_fLeafLightingAdjustmentScalar);
					}
					hProcess.operator++();
				}
			}
		}
	}
}

// It throws IdvFileError with a C++ throw, as SIdvBranchInfo::Parse (BranchInfo.cpp, which says where it is caught).
// The exe's cleanup-only C++ EH frame is left out.
void CLightingEngine::Parse(CTreeFileAccess& cFile) {
	int32_t nToken = cFile.ParseInt();
	do {
		switch (nToken) {
		case File_BranchLightingMethod: {
			m_eBranchLightingMethod = static_cast<CSpeedTreeRT::ELightingMethod>(cFile.ParseInt());
			break;
		}
		case File_BranchMaterial: {
			for (int32_t i = 0; i < 13; ++i) {
				m_afBranchMaterial[i] = cFile.ParseFloat();
			}
			break;
		}
		case File_LeafLightingMethod: {
			m_eLeafLightingMethod = static_cast<CSpeedTreeRT::ELightingMethod>(cFile.ParseInt());
			break;
		}
		case File_LeafMaterial: {
			for (int32_t i_2 = 0; i_2 < 13; ++i_2) {
				m_afLeafMaterial[i_2] = cFile.ParseFloat();
			}
			break;
		}
		case File_LeafLightingAdjustment: {
			m_fLeafLightingAdjustmentScalar = cFile.ParseFloat();
			break;
		}
		case File_StaticLightingStyle: {
			m_eStaticLightingStyle = static_cast<CSpeedTreeRT::EStaticLightingStyle>(cFile.ParseInt());
			break;
		}
		case File_FrondLightingMethod: {
			m_eFrondLightingMethod = static_cast<CSpeedTreeRT::ELightingMethod>(cFile.ParseInt());
			break;
		}
		case File_FrondMaterial: {
			for (int32_t i_3 = 0; i_3 < 13; ++i_3) {
				m_afFrondMaterial[i_3] = cFile.ParseFloat();
			}
			break;
		}
		default: {
			using apMemory_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
			FNVD_CONSTRUCTED_LOCAL(apMemory_t, apMemory, nullptr, "malformed lighting information");
			throw IdvFileError(apMemory, false);
		}
		}
		nToken = cFile.ParseInt();
	} while (nToken != File_EndLightingInfo);
}

CSimpleBillboard::CSimpleBillboard() : m_bValid(false) {}

const float* CSimpleBillboard::GetBillboardCoords(float fWidth, float fHeight) {
	if (!m_bValid) {
		m_acTreeBillboard[0] = fWidth * CSimpleBillboard::m_acUnitBillboard[0][0];
		m_acTreeBillboard[1] = fWidth * CSimpleBillboard::m_acUnitBillboard[0][1];
		m_acTreeBillboard[2] = fHeight * CSimpleBillboard::m_acUnitBillboard[0][2];
		m_acTreeBillboard[3] = fWidth * CSimpleBillboard::m_acUnitBillboard[1][0];
		m_acTreeBillboard[4] = fWidth * CSimpleBillboard::m_acUnitBillboard[1][1];
		m_acTreeBillboard[5] = fHeight * CSimpleBillboard::m_acUnitBillboard[1][2];
		m_acTreeBillboard[6] = fWidth * CSimpleBillboard::m_acUnitBillboard[2][0];
		m_acTreeBillboard[7] = fWidth * CSimpleBillboard::m_acUnitBillboard[2][1];
		m_acTreeBillboard[8] = fHeight * CSimpleBillboard::m_acUnitBillboard[2][2];
		m_acTreeBillboard[9] = fWidth * CSimpleBillboard::m_acUnitBillboard[3][0];
		m_acTreeBillboard[10] = fWidth * CSimpleBillboard::m_acUnitBillboard[3][1];
		m_acTreeBillboard[11] = fHeight * CSimpleBillboard::m_acUnitBillboard[3][2];
		m_bValid = 1;
	}
	return m_acTreeBillboard;
}

// The exe's cleanup-only C++ EH frame is left out.
void CWindMatrices::Resize(uint16_t usCount) {
	if (m_usNumMatrices != usCount) {
		CTransform* v2;
		MemoryManager_Singleton.Deallocate(m_pMatrices);
		m_usNumMatrices = usCount;
		CTransform* v6 =
			static_cast<CTransform*>(MemoryManager_Singleton.Allocate(usCount <= SIZE_MAX / sizeof(CTransform)
					? sizeof(CTransform) * usCount
					: SIZE_MAX));
		if (v6) {
			int32_t v4 = usCount;
			for (CTransform* i = v6; --v4 >= 0; ++i) {
				i->B3CCB0();
			}
			v2 = v6;
		} else {
			v2 = 0;
		}
		m_pMatrices = v2;
	}
	for (int32_t j = 0; j < usCount; ++j) {
		m_pMatrices[j].B3CCD0();
	}
}
