#include "BSShader/BSMaterialEmittanceMultController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cfloat>

NiRTTI BSMaterialEmittanceMultController::ms_RTTI("BSMaterialEmittanceMultController",
	&NiFloatInterpController::ms_RTTI);

BSMaterialEmittanceMultController::BSMaterialEmittanceMultController() {}

// The interpolator pointer of the single-interpolator base releases the interpolator.
BSMaterialEmittanceMultController::~BSMaterialEmittanceMultController() {}

const NiRTTI* BSMaterialEmittanceMultController::GetRTTI() const {
	return &ms_RTTI;
}

// The base copies the members and the cloning process records the clone (a failed allocation is passed through as
// null).
NiObject* BSMaterialEmittanceMultController::CreateClone(NiCloningProcess& kCloning) {
	BSMaterialEmittanceMultController* pkClone = static_cast<
		BSMaterialEmittanceMultController*>(NiMemObject::operator new(sizeof(BSMaterialEmittanceMultController)));
	if (pkClone) {
		::new (pkClone) BSMaterialEmittanceMultController;
	}
	CopyMembers(pkClone, kCloning);
	return pkClone;
}

void BSMaterialEmittanceMultController::LoadBinary(NiStream& kStream) {
	NiFloatInterpController::LoadBinary(kStream);
}

// The exe's cleanup-only C++ EH frame is left out.
void BSMaterialEmittanceMultController::LinkObject(NiStream& kStream) {
	NiFloatInterpController::LinkObject(kStream);
	if ((&kStream)->GetFileVersion() < 0xA010068) {
		NiFloatInterpolator* v5;
		NiFloatData* ObjectFromLinkID = static_cast<NiFloatData*>(kStream.GetObjectFromLinkID());
		NiFloatInterpolator* v4 =
			static_cast<NiFloatInterpolator*>(NiMemObject::operator new(sizeof(NiFloatInterpolator)));
		if (v4) {
			v5 = ::new (v4) NiFloatInterpolator(ObjectFromLinkID);
		} else {
			v5 = 0;
		}
		m_spInterpolator = v5;
		m_spInterpolator.m_pObject->Collapse();
	}
}

bool BSMaterialEmittanceMultController::RegisterStreamables(NiStream& kStream) {
	return NiFloatInterpController::RegisterStreamables(kStream);
}

void BSMaterialEmittanceMultController::SaveBinary(NiStream& kStream) {
	NiFloatInterpController::SaveBinary(kStream);
}

bool BSMaterialEmittanceMultController::IsEqual(NiObject* pkObject) {
	return NiFloatInterpController::IsEqual(pkObject);
}

void BSMaterialEmittanceMultController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiFloatInterpController::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// A manager-controlled controller always samples its interpolator, at the invalid time (-FLT_MAX). The target
// material is not null-checked.
void BSMaterialEmittanceMultController::Update(NiUpdateData& UpdateData) {
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
		pkMaterial->m_fEmitMult = fValue;
	}
}

// Same function as NiMaterialColorController::InterpTargetIsCorrectType (0xBC3050).
bool BSMaterialEmittanceMultController::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
	return NiObject::IsKindOf(&NiMaterialProperty::ms_RTTI, pkTarget);
}

void BSMaterialEmittanceMultController::GetTargetFloatValue(float& fValue) {
	fValue = static_cast<NiMaterialProperty*>(m_pkTarget)->m_fEmitMult;
}

// The exe's stream factory for this class: a new controller, or null.
NiObject* BSMaterialEmittanceMultController::CreateObject() {
	BSMaterialEmittanceMultController* pkController = static_cast<
		BSMaterialEmittanceMultController*>(NiMemObject::operator new(sizeof(BSMaterialEmittanceMultController)));
	if (pkController) {
		::new (pkController) BSMaterialEmittanceMultController;
	}
	return pkController;
}
