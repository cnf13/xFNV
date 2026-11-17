#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenLODData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLODData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLODNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/string.h"

#include <cstring>

NiRTTI NiScreenLODData::ms_RTTI("NiScreenLODData", &NiLODData::ms_RTTI);

NiScreenLODData::NiScreenLODData() : m_uiNumProportions(0), m_pfProportions(nullptr) {
	m_kBound.m_kCenter = NiPoint3_ZERO;
	m_kBound.m_fRadius = 0.0f;
	m_kWorldBound.m_kCenter = NiPoint3_ZERO;
	m_kWorldBound.m_fRadius = 0.0f;
}

NiScreenLODData::~NiScreenLODData() {
	_NiFree(m_pfProportions);
}

// The clone takes the model bound and the proportions; the world bound is not copied and no clone is recorded.
NiObject* NiScreenLODData::CreateClone(NiCloningProcess&) {
	// EXE-INLINED 0xA95F20
	NiScreenLODData* pkMemory = static_cast<NiScreenLODData*>(NiMemObject::operator new(sizeof(NiScreenLODData)));
	NiScreenLODData* pkClone = pkMemory ? ::new (pkMemory) NiScreenLODData : nullptr;
	pkClone->m_kBound = m_kBound;
	pkClone->SetNumProportions(m_uiNumProportions);
	rt::memcpy(pkClone->m_pfProportions, m_pfProportions, m_uiNumProportions * sizeof(float));
	return pkClone;
}

void NiScreenLODData::LoadBinary(NiStream& kStream) {
	NiLODData::LoadBinary(kStream);
	m_kBound.LoadBinary(kStream);
	m_kWorldBound.LoadBinary(kStream);

	uint32_t uiNumProportions;
	NiStreamLoadBinary(kStream, uiNumProportions);
	SetNumProportions(uiNumProportions);
	for (uint32_t i = 0; i < uiNumProportions; i++)
		NiStreamLoadBinary(kStream, m_pfProportions[i]);
}

// KEEP: the exe reads the proportion count into the slot its `push ecx` filled with this object's address, so a short
// read (malformed data) leaves that address as the count driving SetNumProportions and the loop, which src cannot
// express; porting.md's unwritten-slot exception covers GetChunkData only.
void NiScreenLODData::LinkObject(NiStream& kStream) {
	NiLODData::LinkObject(kStream);
}

bool NiScreenLODData::RegisterStreamables(NiStream& kStream) {
	return NiLODData::RegisterStreamables(kStream);
}

void NiScreenLODData::SaveBinary(NiStream& kStream) {
	NiLODData::SaveBinary(kStream);
	m_kBound.SaveBinary(kStream);
	m_kWorldBound.SaveBinary(kStream);

	NiStreamSaveBinary(kStream, m_uiNumProportions);
	for (uint32_t i = 0; i < m_uiNumProportions; i++)
		NiStreamSaveBinary(kStream, m_pfProportions[i]);
}

// Compares both bounds and the proportions; the base comparison is not run.
bool NiScreenLODData::IsEqual(NiObject* pkObject) {
	NiScreenLODData* pkScreen = static_cast<NiScreenLODData*>(pkObject);
	if (m_kBound != pkScreen->m_kBound || m_kWorldBound != pkScreen->m_kWorldBound)
		return false;

	if (m_uiNumProportions != pkScreen->m_uiNumProportions)
		return false;

	for (uint32_t i = 0; i < m_uiNumProportions; i++) {
		if (m_pfProportions[i] != pkScreen->m_pfProportions[i])
			return false;
	}
	return true;
}

// The base classes' strings are not listed.
void NiScreenLODData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pkStrings->Add(m_kBound.GetViewerString("Bound"));
	pkStrings->Add(m_kWorldBound.GetViewerString("World Bound"));

	for (uint32_t i = 0; i < m_uiNumProportions; i++) {
		char* pcString = static_cast<char*>(_NiMalloc(128));
		NiSprintf(pcString, 128, "Proportion[%d] = %g", i, static_cast<double>(m_pfProportions[i]));
		pkStrings->Add(pcString);
	}
}

// The screen proportion is the world bound's projected radius (divided by its distance unless the camera is
// orthographic) times the larger frustum scale and the camera's LOD adjustment. The level is the first whose
// proportion threshold it reaches; level 0 while the camera is inside the bound or on its center.
int32_t NiScreenLODData::GetLODLevel(const NiCamera* pkCamera, NiLODNode*) {
	const NiPoint3& kCameraPosition = pkCamera->m_kWorld.m_Translate;
	float fDiffX = m_kWorldBound.m_kCenter.x - kCameraPosition.x;
	float fDiffY = m_kWorldBound.m_kCenter.y - kCameraPosition.y;
	float fDiffZ = m_kWorldBound.m_kCenter.z - kCameraPosition.z;
	float fSqrDistance = fDiffX * fDiffX + fDiffY * fDiffY + fDiffZ * fDiffZ;
	float fDistance = std::sqrt(fSqrDistance);

	if (m_kWorldBound.m_fRadius > fDistance || fDistance <= 1e-06f)
		return 0;

	const NiFrustum& kFrustum = pkCamera->m_kViewFrustum;
	float fRadius = kFrustum.m_bOrtho ? m_kWorldBound.m_fRadius : m_kWorldBound.m_fRadius / fDistance;
	float fScaleX = 2.0 / (kFrustum.m_fRight - kFrustum.m_fLeft);
	float fScaleY = 2.0 / (kFrustum.m_fTop - kFrustum.m_fBottom);
	float fScale = fScaleY < fScaleX ? fScaleX : fScaleY;
	float fProportion = fScale * fRadius * pkCamera->m_fLODAdjust;

	uint32_t i = 0;
	for (; i < m_uiNumProportions; i++) {
		if (m_pfProportions[i] <= fProportion)
			break;
	}
	return static_cast<int32_t>(i);
}

// A zero model radius asks for the bound to be fitted to the children first.
void NiScreenLODData::UpdateWorldData(NiLODNode* pkLOD) {
	if (m_kBound.m_fRadius == 0.0f)
		AutoSetBound(pkLOD);
	else
		m_kWorldBound.Update(m_kBound, pkLOD->m_kWorld);
}

// Same as CreateClone: copies the model bound and the proportions.
NiLODData* NiScreenLODData::Duplicate() {
	// EXE-INLINED 0xA95F20
	NiScreenLODData* pkMemory = static_cast<NiScreenLODData*>(NiMemObject::operator new(sizeof(NiScreenLODData)));
	NiScreenLODData* pkDuplicate = pkMemory ? ::new (pkMemory) NiScreenLODData : nullptr;
	pkDuplicate->m_kBound = m_kBound;
	pkDuplicate->SetNumProportions(m_uiNumProportions);
	rt::memcpy(pkDuplicate->m_pfProportions, m_pfProportions, m_uiNumProportions * sizeof(float));
	return pkDuplicate;
}

// Levels past the last proportion map to the count itself.
int32_t NiScreenLODData::GetLODIndex(int32_t iLODLevel) {
	if (iLODLevel < 0)
		return 0;
	int32_t iNumProportions = static_cast<int32_t>(m_uiNumProportions);
	return iLODLevel < iNumProportions ? iLODLevel : iNumProportions;
}

void NiScreenLODData::SetNumProportions(uint32_t uiNumProportions) {
	if (uiNumProportions == m_uiNumProportions)
		return;

	if (uiNumProportions) {
		float* pfProportions = static_cast<float*>(_NiMalloc(uiNumProportions * sizeof(float)));
		if (m_pfProportions) {
			int32_t iNumKept = static_cast<int32_t>(m_uiNumProportions) < static_cast<int32_t>(uiNumProportions)
				? static_cast<int32_t>(m_uiNumProportions)
				: static_cast<int32_t>(uiNumProportions);
			memcpy(pfProportions, m_pfProportions, iNumKept * sizeof(float));
		}
		_NiFree(m_pfProportions);
		m_pfProportions = pfProportions;
		m_uiNumProportions = uiNumProportions;
	} else {
		_NiFree(m_pfProportions);
		m_pfProportions = nullptr;
		m_uiNumProportions = 0;
	}
}

void NiScreenLODData::AutoSetBound(NiLODNode* pkLOD) {
	NiAVObject* m_pObject;
	NiBound* m_kWorldBound;
	NiTransform a2;
	pkLOD->UpdateAllChildren();
	this->m_kWorldBound.m_fRadius = (float)0.0;
	int32_t v3 = 0;
	if (pkLOD->m_kChildren.m_usSize) {
		do {
			m_pObject = pkLOD->m_kChildren.m_pBase[v3].m_pObject;
			if (m_pObject) {
				m_kWorldBound = m_pObject->m_kWorldBound;
				if (!m_kWorldBound) {
					m_kWorldBound = &NiAVObject::NullBoundS;
				}
				(&this->m_kWorldBound)->Merge(m_kWorldBound);
			}
			++v3;
		} while (v3 < ((int32_t)pkLOD->m_kChildren.m_usSize));
	}
	(&pkLOD->m_kWorld)->Invert(a2);
	(&m_kBound)->Update(this->m_kWorldBound, a2);
}

NiObject* NiScreenLODData::CreateObject() {
	NiScreenLODData* v0 = static_cast<NiScreenLODData*>(NiMemObject::operator new(sizeof(NiScreenLODData)));
	if (v0) {
		return ::new (v0) NiScreenLODData;
	} else {
		return 0;
	}
}
