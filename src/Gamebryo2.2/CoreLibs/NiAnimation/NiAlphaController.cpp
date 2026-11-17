#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAlphaController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cfloat>

NiRTTI NiAlphaController::ms_RTTI("NiAlphaController", &NiFloatInterpController::ms_RTTI);

NiAlphaController::NiAlphaController() : NiFloatInterpController() {}

NiAlphaController::~NiAlphaController() {}

const NiRTTI* NiAlphaController::GetRTTI() const {
	return &ms_RTTI;
}

NiObject* NiAlphaController::CreateClone(NiCloningProcess& kCloning) {
	NiAlphaController* v3 = static_cast<NiAlphaController*>(NiMemObject::operator new(sizeof(NiAlphaController)));
	if (v3) {
		::new (v3) NiAlphaController;
		CopyMembers(v3, kCloning);
		return v3;
	} else {
		CopyMembers(0, kCloning);
		return 0;
	}
}

void NiAlphaController::LoadBinary(NiStream& kStream) {
	NiFloatInterpController::LoadBinary(kStream);
	if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104))
		kStream.ReadLinkID();
}

void NiAlphaController::LinkObject(NiStream& kStream) {
	NiFloatData* objectFromLinkID;
	void* v4;
	NiFloatInterpolator* v5;
	NiInterpolator* m_pObject;
	NiFloatInterpController::LinkObject(kStream);
	if ((&kStream)->m_uiNifFileVersion < 167837800) {
		objectFromLinkID = static_cast<NiFloatData*>((&kStream)->GetObjectFromLinkID());
		v4 = NiMemObject::operator new(sizeof(NiFloatInterpolator));
		if (v4) {
			v5 = ::new (v4) NiFloatInterpolator(objectFromLinkID);
		} else {
			v5 = nullptr;
		}
		m_pObject = m_spInterpolator.m_pObject;
		if (static_cast<const void*>(m_pObject) != static_cast<const void*>(v5)) {
			if (m_pObject) {
				if (!InterlockedDecrementUInt32(&m_pObject->m_uiRefCount)) {
					m_pObject->DeleteThis();
				}
			}
			m_spInterpolator.m_pObject = v5;
			if (v5) {
				InterlockedIncrementUInt32(&v5->m_uiRefCount);
			}
		}
		m_spInterpolator.m_pObject->Collapse();
	}
}

bool NiAlphaController::RegisterStreamables(NiStream& kStream) {
	return NiFloatInterpController::RegisterStreamables(kStream);
}

void NiAlphaController::SaveBinary(NiStream& kStream) {
	NiFloatInterpController::SaveBinary(kStream);
}

bool NiAlphaController::IsEqual(NiObject* pkObject) {
	return NiFloatInterpController::IsEqual(pkObject);
}

void NiAlphaController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiFloatInterpController::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// A manager-controlled controller always samples its interpolator, at the invalid time (-FLT_MAX). The target
// material is not null-checked.
void NiAlphaController::Update(NiUpdateData& UpdateData) {
	if (m_usFlags & MANAGERCONTROLLED_MASK)
		m_fScaledTime = -FLT_MAX;
	else if (DontDoUpdate(UpdateData.fTime) && (!m_spInterpolator || !m_spInterpolator->AlwaysUpdate()))
		return;

	if (!m_spInterpolator)
		return;

	float fValue;
	if (m_spInterpolator->Update(m_fScaledTime, m_pkTarget, fValue)) {
		NiMaterialProperty* pkMaterial = static_cast<NiMaterialProperty*>(m_pkTarget);
		++pkMaterial->m_uiRevID;
		pkMaterial->m_fAlpha = fValue;
	}
}

// Same function as NiMaterialColorController::InterpTargetIsCorrectType (0xBC3050).
bool NiAlphaController::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
	return NiObject::IsKindOf(&NiMaterialProperty::ms_RTTI, pkTarget);
}

void NiAlphaController::GetTargetFloatValue(float& fValue) {
	fValue = static_cast<NiMaterialProperty*>(m_pkTarget)->m_fAlpha;
}

NiObject* NiAlphaController::CreateObject() {
	NiAlphaController* v0 = static_cast<NiAlphaController*>(NiMemObject::operator new(sizeof(NiAlphaController)));
	if (!v0) {
		return 0;
	}
	return ::new (v0) NiAlphaController;
}
