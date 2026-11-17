#include "BSMain/BSParentVelocityModifier.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI BSParentVelocityModifier::ms_RTTI("BSParentVelocityModifier", &NiPSysModifier::ms_RTTI);

// fTimeLast is left unset, as in the exe; the order is 3000.
BSParentVelocityModifier::BSParentVelocityModifier()
	: fLastTimeDelta(0.0f), fDamping(0.0f), pntLast(NiPoint3_ZERO), velLast(NiPoint3_ZERO) {
	m_uiOrder = 3000;
}

// The base takes the name as a temporary NiFixedString, at order 3000; fTimeLast is left unset, as in the exe.
BSParentVelocityModifier::BSParentVelocityModifier(const char* apcName)
	: NiPSysModifier(NiFixedString(apcName), 3000u), fLastTimeDelta(0.0f), fDamping(0.0f), pntLast(NiPoint3_ZERO),
	  velLast(NiPoint3_ZERO) {}

BSParentVelocityModifier::~BSParentVelocityModifier() {}

NiObject* BSParentVelocityModifier::CreateClone(NiCloningProcess& arCloneData) {
	BSParentVelocityModifier* v3;
	void* v5 = NiMemObject::operator new(sizeof(BSParentVelocityModifier));
	if (v5) {
		v3 = ::new (v5) BSParentVelocityModifier;
	} else {
		v3 = nullptr;
	}
	CopyMembers(v3, arCloneData);
	return v3;
}

void BSParentVelocityModifier::LoadBinary(NiStream& arStream) {
	NiPSysModifier::LoadBinary(arStream);

	// The exe loads the float through its out-of-line 4-byte transfer, NiStreamLoadBinary<uint32_t> (0xA26DF0), which
	// moves four untyped bytes: the field's address is passed as that instance's argument.
	NiStreamLoadBinary<uint32_t>(arStream, reinterpret_cast<uint32_t&>(fDamping));
}

void BSParentVelocityModifier::LinkObject(NiStream& arStream) {
	NiPSysModifier::LinkObject(arStream);
}

bool BSParentVelocityModifier::RegisterStreamables(NiStream& arStream) {
	return NiPSysModifier::RegisterStreamables(arStream);
}

void BSParentVelocityModifier::SaveBinary(NiStream& arStream) {
	NiPSysModifier::SaveBinary(arStream);

	// The exe saves the float through its out-of-line 4-byte write at 0xA3DA50, NiAnimationKey::SaveBinary's address,
	// which writes four untyped bytes at the key's start: the field's address is passed as the key.
	NiAnimationKey::SaveBinary(arStream, reinterpret_cast<NiAnimationKey*>(&fDamping));
}

bool BSParentVelocityModifier::IsEqual(NiObject* apObject) {
	bool bEqual = NiPSysModifier::IsEqual(apObject);
	if (bEqual) {
		BSParentVelocityModifier* pkOther = static_cast<BSParentVelocityModifier*>(apObject);
		bEqual = pkOther->fDamping == fDamping;
	}
	return bEqual;
}

void BSParentVelocityModifier::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	NiPSysModifier::GetViewerStrings(apStrings);

	apStrings->Add(NiGetViewerString(BSParentVelocityModifier::ms_RTTI.GetName()));
	apStrings->Add(NiGetViewerString("Damping", fDamping));
}

// Measures how far the parent moved since the last update and adds the change of its damped velocity to every active
// particle. Without a parent, or on the first update after a reset, the history is restarted instead.
void BSParentVelocityModifier::Update(float afTime, NiPSysData* apData) {
	uint16_t usActive = apData->m_usActive;
	if (usActive == 0) {
		return;
	}

	NiNode* pkParent = m_pkTarget->m_pkParent;
	if (pkParent == nullptr) {
		HandleReset();
		return;
	}

	// The exe inlines NiPoint3::operator== here, comparing x, y and z in turn.
	// EXE-INLINED 0x4390C0
	if (pntLast.x == NiPoint3_ZERO.x && pntLast.y == NiPoint3_ZERO.y && pntLast.z == NiPoint3_ZERO.z) {
		velLast = NiPoint3_ZERO;
		fTimeLast = afTime;
		pntLast = pkParent->m_kWorld.m_Translate;
	}

	uint32_t uiCount = apData->m_usActive;
	NiPoint3 kPos = pkParent->m_kWorld.m_Translate;
	float fDelta = afTime - fTimeLast;
	if (fDelta != 0.0) {
		NiPoint3 kVel = (fDamping * (kPos - pntLast)) / fDelta;
		NiPoint3 kDiff = kVel - velLast;
		if (kDiff != NiPoint3_ZERO) {
			NiParticleInfo* pkInfo = apData->m_pkParticleInfo;
			while (uiCount--) {
				pkInfo->m_kVelocity += kDiff;
				++pkInfo;
			}
		}
		velLast = kVel;
	}
	fLastTimeDelta = fDelta;
	fTimeLast = afTime;
	pntLast = kPos;
}

// A new particle starts with the parent's velocity, and its position is wound back by that velocity over the last
// time step.
void BSParentVelocityModifier::Initialize(NiPSysData* apData, uint16_t aiNewParticle) {
	NiParticleInfo* pkInfo = apData->m_pkParticleInfo + aiNewParticle;
	pkInfo->m_kVelocity += velLast;

	NiPoint3* pkVertices = apData->m_pkVertex;
	pkVertices[aiNewParticle] -= velLast * fLastTimeDelta;
}

void BSParentVelocityModifier::HandleReset() {
	pntLast = NiPoint3_ZERO;
}

void BSParentVelocityModifier::CopyMembers(BSParentVelocityModifier* apDest, NiCloningProcess& arCloneData) {
	NiPSysModifier::CopyMembers(apDest, arCloneData);
	apDest->fDamping = fDamping;
}

NiObject* BSParentVelocityModifier::CreateObject() {
	void* apMemory = NiMemObject::operator new(sizeof(BSParentVelocityModifier));
	if (apMemory) {
		return ::new (apMemory) BSParentVelocityModifier;
	} else {
		return nullptr;
	}
}
