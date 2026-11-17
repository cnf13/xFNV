#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMorphData.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

NiRTTI NiMorphData::ms_RTTI("NiMorphData", &NiObject::ms_RTTI);

namespace {

// Bytes for uiCount elements of uiSize after uiHeader bytes; a size that overflows 32 bits becomes SIZE_MAX, as the
// game's array allocations do.
size_t GetArraySize(uint32_t uiCount, uint32_t uiSize, uint32_t uiHeader) {
	uint64_t ullSize = static_cast<uint64_t>(uiCount) * uiSize + uiHeader;
	return ullSize > UINT32_MAX ? SIZE_MAX : static_cast<size_t>(ullSize);
}

}

NiMorphData::MorphTarget::MorphTarget() : m_aTargetVerts(nullptr), m_fLegacyWeight(0.0f) {}

NiMorphData::MorphTarget::~MorphTarget() {
	// The vertices come from MemoryManager::Allocate (LoadBinary).
	MemoryManager_Singleton.Deallocate(m_aTargetVerts);
}

void NiMorphData::MorphTarget::LoadBinary(NiStream& kStream, uint32_t uiNumVerts) {
	if (kStream.GetFileVersion() >= 0x14010001)
		kStream.LoadFixedString(m_kName);
	else
		kStream.LoadCStringAsFixedString(m_kName);

	if (kStream.GetFileVersion() >= 0x0A010068 && kStream.GetFileVersion() < 0x14010003)
		NiStreamLoadBinary(kStream, m_fLegacyWeight);

	m_aTargetVerts =
		static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(GetArraySize(uiNumVerts, sizeof(NiPoint3), 0)));
	NiStreamLoadBinary(kStream, &m_aTargetVerts->x, uiNumVerts * 3);
}

bool NiMorphData::MorphTarget::IsEqual(MorphTarget* pkOther, uint32_t uiNumVerts) {
	for (uint32_t i = 0; i < uiNumVerts; i++) {
		const NiPoint3& kVert = m_aTargetVerts[i];
		const NiPoint3& kOther = pkOther->m_aTargetVerts[i];
		if (kOther.x != kVert.x || kOther.y != kVert.y || kOther.z != kVert.z)
			return false;
	}

	return m_kName == pkOther->m_kName;
}

const NiFixedString& NiMorphData::MorphTarget::GetName() const {
	return m_kName;
}

void NiMorphData::MorphTarget::operator delete(void* apMemory) {
	MemoryManager_Singleton.Deallocate(apMemory);
}

void NiMorphData::MorphTarget::operator delete[](void* apMemory) {
	MemoryManager_Singleton.Deallocate(apMemory);
}

NiMorphData::NiMorphData()
	: NiObject(), m_uiNumTargets(0), m_uiNumVertsPerTarget(0), m_aMorphTargets(nullptr), m_bRelativeTargets(false) {}

NiMorphData::~NiMorphData() {
	delete[] m_aMorphTargets;
}

const NiRTTI* NiMorphData::GetRTTI() const {
	return &ms_RTTI;
}

// Files before 10.1.0.104 store each target's weight as float keys, turned into a collapsed interpolator, and no
// name: the target is named "MT <index>".
void NiMorphData::LoadBinary(NiStream& stream) {
	NiObject::LoadBinary(stream);

	NiStreamLoadBinary(stream, m_uiNumTargets);
	NiStreamLoadBinary(stream, m_uiNumVertsPerTarget);
	uint8_t ucRelativeTargets;
	NiStreamLoadBinary(stream, ucRelativeTargets);
	m_bRelativeTargets = ucRelativeTargets == 1;

	// new MorphTarget[m_uiNumTargets] on the engine heap: the count, then the targets.
	size_t* puiArray = static_cast<size_t*>(MemoryManager_Singleton.Allocate(GetArraySize(m_uiNumTargets,
		sizeof(MorphTarget), sizeof(size_t))));
	if (puiArray) {
		*puiArray = m_uiNumTargets;
		m_aMorphTargets = reinterpret_cast<MorphTarget*>(puiArray + 1);
		for (uint32_t i = 0; i < m_uiNumTargets; i++)
			new (&m_aMorphTargets[i]) MorphTarget;
	} else {
		m_aMorphTargets = nullptr;
	}

	for (uint32_t i = 0; i < m_uiNumTargets; i++) {
		MorphTarget& kTarget = m_aMorphTargets[i];
		if (stream.GetFileVersion() >= 0x0A010068) {
			kTarget.LoadBinary(stream, m_uiNumVertsPerTarget);
			continue;
		}

		uint32_t uiNumKeys;
		NiStreamLoadBinary(stream, uiNumKeys);
		NiFloatKey::KeyType eType;
		NiStreamLoadBinary(stream, eType);
		if (uiNumKeys) {
			uint8_t ucSize = NiFloatKey::GetKeySize(eType);
			NiFloatKey* pkKeys = static_cast<NiFloatKey*>(NiAnimationKey::GetCreateFunction(NiAnimationKey::FLOATKEY,
				eType)(stream, uiNumKeys));
			NiAnimationKey::GetFillDerivedFunction(NiAnimationKey::FLOATKEY, eType)(pkKeys, uiNumKeys, ucSize);
			void* pvMem = NiMemObject::operator new(sizeof(NiFloatInterpolator));
			NiFloatInterpolator* pkInterp = pvMem ? ::new (pvMem) NiFloatInterpolator(nullptr) : nullptr;
			pkInterp->ReplaceKeys(pkKeys, uiNumKeys, eType);
			kTarget.m_spLegacyInterpolator = pkInterp;
			pkInterp->Collapse();
		}

		char acName[256];
		NiSprintf(acName, sizeof(acName), "MT %d", i);
		kTarget.m_kName = acName;

		NiPoint3* pkVerts = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(GetArraySize(m_uiNumVertsPerTarget,
			sizeof(NiPoint3), 0)));
		for (uint32_t j = 0; j < m_uiNumVertsPerTarget; j++)
			pkVerts[j].LoadBinary(stream);
		MemoryManager_Singleton.Deallocate(kTarget.m_aTargetVerts);
		kTarget.m_aTargetVerts = pkVerts;
	}
}

void NiMorphData::LinkObject(NiStream& stream) {
	NiObject::LinkObject(stream);
}

bool NiMorphData::RegisterStreamables(NiStream& stream) {
	if (!NiObject::RegisterStreamables(stream))
		return false;

	for (uint32_t i = 0; i < m_uiNumTargets; i++)
		stream.RegisterFixedString(m_aMorphTargets[i].m_kName);

	return true;
}

void NiMorphData::SaveBinary(NiStream& stream) {
	NiObject::SaveBinary(stream);

	NiStreamSaveBinary(stream, m_uiNumTargets);
	NiStreamSaveBinary(stream, m_uiNumVertsPerTarget);
	uint8_t ucRelativeTargets = m_bRelativeTargets;
	NiStreamSaveBinary(stream, ucRelativeTargets);

	for (uint32_t i = 0; i < m_uiNumTargets; i++) {
		MorphTarget& kTarget = m_aMorphTargets[i];
		stream.SaveFixedString(kTarget.m_kName);
		if (m_uiNumVertsPerTarget)
			NiStreamSaveBinary(stream, &kTarget.m_aTargetVerts->x, m_uiNumVertsPerTarget * 3);
	}
}

bool NiMorphData::IsEqual(NiObject* pObject) {
	if (!NiObject::IsEqual(pObject))
		return false;

	NiMorphData* pkData = static_cast<NiMorphData*>(pObject);
	if (m_uiNumTargets != pkData->m_uiNumTargets || m_uiNumVertsPerTarget != pkData->m_uiNumVertsPerTarget ||
		m_bRelativeTargets != pkData->m_bRelativeTargets)
		return false;

	for (uint32_t i = 0; i < m_uiNumTargets; i++) {
		if (!GetTarget(i)->IsEqual(pkData->GetTarget(i), m_uiNumVertsPerTarget))
			return false;
	}

	return true;
}

void NiMorphData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
	NiObject::GetViewerStrings(pStrings);
	pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pStrings->Add(NiGetViewerString("m_uiNumTargets", m_uiNumTargets));
	pStrings->Add(NiGetViewerString("m_uiNumVertsPerTarget", m_uiNumVertsPerTarget));
	pStrings->Add(NiGetViewerString("MorphTargets"));
	for (uint32_t i = 0; i < m_uiNumTargets; i++)
		pStrings->Add(NiGetViewerString("m_pcName", static_cast<const char*>(GetTarget(i)->m_kName)));
}

NiObject* NiMorphData::CreateObject() {
	NiMorphData* pkObject = static_cast<NiMorphData*>(NiMemObject::operator new(sizeof(NiMorphData)));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) NiMorphData;
}
