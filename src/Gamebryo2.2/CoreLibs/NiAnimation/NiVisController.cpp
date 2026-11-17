#include "Gamebryo2.2/CoreLibs/NiAnimation/NiVisController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cfloat>

NiRTTI NiVisController::ms_RTTI("NiVisController", &NiBoolInterpController::ms_RTTI);

NiVisController::NiVisController() {}

NiVisController::~NiVisController() {}

const NiRTTI* NiVisController::GetRTTI() const {
	return &ms_RTTI;
}

NiObject* NiVisController::CreateClone(NiCloningProcess& kCloning) {
	NiVisController* pkClone = static_cast<NiVisController*>(NiMemObject::operator new(sizeof(NiVisController)));
	if (pkClone) {
		::new (pkClone) NiVisController;
		CopyMembers(pkClone, kCloning);
		return pkClone;
	}
	CopyMembers(nullptr, kCloning);
	return nullptr;
}

// Files older than 10.1.0.104 carry a link to an old data block, which is skipped. NiAlphaController,
// BSFrustumFOVController and the BS float controllers use the same function.
void NiVisController::LoadBinary(NiStream& kStream) {
	NiBoolInterpController::LoadBinary(kStream);
	if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104))
		kStream.ReadLinkID();
}

void NiVisController::LinkObject(NiStream& kStream) {
	NiBoolInterpController::LinkObject(kStream);
	if ((&kStream)->GetFileVersion() < 0xA010068) {
		NiBoolInterpolator* v5;
		NiBoolData* pkBoolData = static_cast<NiBoolData*>(kStream.GetObjectFromLinkID());
		NiBoolInterpolator* v4 =
			static_cast<NiBoolInterpolator*>(NiMemObject::operator new(sizeof(NiBoolInterpolator)));
		if (v4) {
			v5 = ::new (v4) NiBoolInterpolator(pkBoolData);
		} else {
			v5 = 0;
		}
		m_spInterpolator = v5;
		m_spInterpolator.m_pObject->Collapse();
	}
}

// NiAlphaController, BSFrustumFOVController and the BS float controllers use the same function.
bool NiVisController::RegisterStreamables(NiStream& kStream) {
	return NiBoolInterpController::RegisterStreamables(kStream);
}

// NiAlphaController, NiLightDimmerController, BSFrustumFOVController and the BS float controllers use the same
// function.
void NiVisController::SaveBinary(NiStream& kStream) {
	NiBoolInterpController::SaveBinary(kStream);
}

// NiAlphaController, BSFrustumFOVController and the BS float controllers use the same function.
bool NiVisController::IsEqual(NiObject* pkObject) {
	return NiBoolInterpController::IsEqual(pkObject);
}

void NiVisController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiBoolInterpController::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// The animated value is visibility: true clears the target's app-culled flag. Showing a hidden object marks the
// scene graph as changed. The target is not null-checked.
void NiVisController::Update(NiUpdateData& UpdateData) {
	if (m_usFlags & MANAGERCONTROLLED_MASK)
		m_fScaledTime = -FLT_MAX;
	else if (DontDoUpdate(UpdateData.fTime) && (!m_spInterpolator || !m_spInterpolator->AlwaysUpdate()))
		return;

	if (!m_spInterpolator)
		return;

	bool bVisible;
	if (!m_spInterpolator->Update(m_fScaledTime, m_pkTarget, bVisible))
		return;

	NiAVObject* pkObject = static_cast<NiAVObject*>(m_pkTarget);
	// EXE-INLINED 0x456610
	bool bWasCulled = (pkObject->m_uFlags & NiAVObject::APP_CULLED) != 0;
	// EXE-INLINED 0x450F90
	if (bVisible)
		pkObject->m_uFlags &= ~static_cast<uint32_t>(NiAVObject::APP_CULLED);
	else
		pkObject->m_uFlags |= NiAVObject::APP_CULLED;
	if (bWasCulled && bVisible)
		UpdateData.bSceneGraphChange = true;
}

bool NiVisController::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
	return NiObject::IsKindOf(&NiAVObject::ms_RTTI, pkTarget);
}

void NiVisController::GetTargetBoolValue(bool& bValue) {
	// EXE-INLINED 0x456610
	bValue = !(static_cast<NiAVObject*>(m_pkTarget)->m_uFlags & NiAVObject::APP_CULLED);
}

NiObject* NiVisController::CreateObject() {
	NiVisController* v0 = static_cast<NiVisController*>(NiMemObject::operator new(sizeof(NiVisController)));
	if (!v0) {
		return 0;
	}
	return ::new (v0) NiVisController;
}
