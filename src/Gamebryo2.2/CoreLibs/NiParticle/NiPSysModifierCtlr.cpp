#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSingleInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

NiRTTI NiPSysModifierCtlr::ms_RTTI("NiPSysModifierCtlr", &NiSingleInterpController::ms_RTTI);

NiPSysModifierCtlr::NiPSysModifierCtlr() : m_pkModifier(nullptr) {}

NiPSysModifierCtlr::NiPSysModifierCtlr(const NiFixedString& akModifierName)
	: m_kModifierName(akModifierName), m_pkModifier(nullptr) {}

// The name's reference is dropped by NiFixedString's destructor, the interpolator's by NiSingleInterpController's.
NiPSysModifierCtlr::~NiPSysModifierCtlr() {}

void NiPSysModifierCtlr::LoadBinary(NiStream& kStream) {
	NiSingleInterpController::LoadBinary(kStream);
	if (kStream.GetFileVersion() < NiStream::GetVersion(20, 1, 0, 1))
		kStream.LoadCStringAsFixedString(m_kModifierName);
	else
		kStream.LoadFixedString(m_kModifierName);
}

void NiPSysModifierCtlr::LinkObject(NiStream& kStream) {
	NiSingleInterpController::LinkObject(kStream);
}

bool NiPSysModifierCtlr::RegisterStreamables(NiStream& kStream) {
	return NiSingleInterpController::RegisterStreamables(kStream);
}

void NiPSysModifierCtlr::SaveBinary(NiStream& kStream) {
	NiSingleInterpController::SaveBinary(kStream);
	kStream.SaveFixedString(m_kModifierName);
}

// Modifiers compare by value; the names by handle.
bool NiPSysModifierCtlr::IsEqual(NiObject* pkObject) {
	if (!NiSingleInterpController::IsEqual(pkObject))
		return false;

	NiPSysModifierCtlr* pkOther = static_cast<NiPSysModifierCtlr*>(pkObject);
	if ((m_pkModifier && !pkOther->m_pkModifier) || (!m_pkModifier && pkOther->m_pkModifier))
		return false;

	if (m_pkModifier && pkOther->m_pkModifier && !m_pkModifier->IsEqual(pkOther->m_pkModifier))
		return false;

	return m_kModifierName == pkOther->m_kModifierName;
}

void NiPSysModifierCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiSingleInterpController::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pkStrings->Add(NiGetViewerString("Modifier Name", static_cast<const char*>(m_kModifierName)));
}

// The clone points at the clone of this controller's modifier. One local takes both lookups, as the game does: when the
// modifier is not in the clone map, the clone's modifier is the controller clone itself; a cloning pass always maps
// every object it processes.
void NiPSysModifierCtlr::ProcessClone(NiCloningProcess& kCloning) {
	NiSingleInterpController::ProcessClone(kCloning);

	NiObject* pkClone = nullptr;
	kCloning.m_pkCloneMap->GetAt(this, pkClone);
	if (!m_pkTarget)
		return;

	NiPSysModifierCtlr* pkDest = static_cast<NiPSysModifierCtlr*>(pkClone);
	if (!pkDest->m_pkTarget)
		return;

	kCloning.m_pkCloneMap->GetAt(m_pkModifier, pkClone);
	pkDest->m_pkModifier = static_cast<NiPSysModifier*>(pkClone);
}

void NiPSysModifierCtlr::PostLinkObject(NiStream& kStream) {
	NiSingleInterpController::PostLinkObject(kStream);
	if (m_pkTarget)
		GetModifierPointerFromName();
}

void NiPSysModifierCtlr::SetTarget(NiObjectNET* pkTarget) {
	m_pkModifier = nullptr;
	NiSingleInterpController::SetTarget(pkTarget);
	if (m_pkTarget)
		GetModifierPointerFromName();
}

// The target must be a particle system that holds a modifier with this controller's (non-null) name, and this
// controller must already have a target.
bool NiPSysModifierCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
	if (!NiObject::IsKindOf(&NiParticleSystem::ms_RTTI, pkTarget))
		return false;

	if (m_kModifierName.GetHandle() == NiGlobalStringTable::NULL_STRING || !m_pkTarget)
		return false;

	return static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName) != nullptr;
}

void NiPSysModifierCtlr::CopyMembers(NiPSysModifierCtlr* pkDest, NiCloningProcess& kCloning) {
	NiSingleInterpController::CopyMembers(pkDest, kCloning);
	pkDest->m_kModifierName = m_kModifierName;
}

// The target must be a particle system.
void NiPSysModifierCtlr::GetModifierPointerFromName() {
	NiParticleSystem* pkSystem = static_cast<NiParticleSystem*>(m_pkTarget);
	NiTListItem<NiPointer<NiPSysModifier>>* pkPos = pkSystem->m_kModifierList.GetHeadPos();
	while (pkPos) {
		NiPSysModifier* pkModifier = pkSystem->m_kModifierList.GetNext(pkPos);
		if (pkModifier && pkModifier->m_kName == m_kModifierName) {
			m_pkModifier = pkModifier;
			return;
		}
	}
	m_pkModifier = nullptr;
}
