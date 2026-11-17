#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLightColorController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPoint3InterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPoint3Interpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cfloat>

NiRTTI NiLightColorController::ms_RTTI("NiLightColorController", &NiPoint3InterpController::ms_RTTI);

NiLightColorController::NiLightColorController() : m_uFlags(0) {}

NiLightColorController::~NiLightColorController() {}

const NiRTTI* NiLightColorController::GetRTTI() const {
	return &ms_RTTI;
}

NiObject* NiLightColorController::CreateClone(NiCloningProcess& kCloning) {
	void* pkObject = NiMemObject::operator new(sizeof(NiLightColorController));
	NiLightColorController* pkObject_2 = 0;
	if (pkObject) {
		pkObject_2 = ::new (pkObject) NiLightColorController;
	}
	CopyMembers(pkObject_2, kCloning);
	pkObject_2->m_uFlags = m_uFlags;
	return pkObject_2;
}

// Files before 10.0.1.2 keep the colour choice in the stream's time controller flags; files older than
// 10.1.0.104 also carry a link to an old data block, which is skipped.
void NiLightColorController::LoadBinary(NiStream& kStream) {
	NiPoint3InterpController::LoadBinary(kStream);
	if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 0, 1, 2))
		NiStreamLoadBinary(kStream, m_uFlags);
	else
		m_uFlags = static_cast<uint16_t>((kStream.m_usNiTimeControllerFlags >> 5) & 0x3F);

	if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104))
		kStream.ReadLinkID();
}

void NiLightColorController::LinkObject(NiStream& kStream) {
	NiPoint3InterpController::LinkObject(kStream);
	if ((&kStream)->GetFileVersion() < 0xA010068) {
		NiPoint3Interpolator* v5;
		NiPosData* ObjectFromLinkID = static_cast<NiPosData*>(kStream.GetObjectFromLinkID());
		if (ObjectFromLinkID) {
			NiIncRefCount(ObjectFromLinkID);
		}
		NiPoint3Interpolator* v4 =
			static_cast<NiPoint3Interpolator*>(NiMemObject::operator new(sizeof(NiPoint3Interpolator)));
		if (v4) {
			v5 = ::new (v4) NiPoint3Interpolator(ObjectFromLinkID);
		} else {
			v5 = 0;
		}
		m_spInterpolator = v5;
		m_spInterpolator.m_pObject->Collapse();
		if (ObjectFromLinkID) {
			NiDecRefCount(ObjectFromLinkID);
		}
	}
}

bool NiLightColorController::RegisterStreamables(NiStream& kStream) {
	return NiPoint3InterpController::RegisterStreamables(kStream);
}

void NiLightColorController::SaveBinary(NiStream& kStream) {
	NiPoint3InterpController::SaveBinary(kStream);
	NiStreamSaveBinary(kStream, m_uFlags);
}

bool NiLightColorController::IsEqual(NiObject* pkObject) {
	if (!NiPoint3InterpController::IsEqual(pkObject))
		return false;

	return (m_uFlags & ISAMBIENT_MASK) == (static_cast<NiLightColorController*>(pkObject)->m_uFlags & ISAMBIENT_MASK);
}

void NiLightColorController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiPoint3InterpController::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// The colour is clamped to [0, 1] (NaN passes) and set on the animated slot; the other slot is set to black.
void NiLightColorController::Update(NiUpdateData& UpdateData) {
	if (m_usFlags & MANAGERCONTROLLED_MASK)
		m_fScaledTime = -FLT_MAX;
	else if (DontDoUpdate(UpdateData.fTime) && (!m_spInterpolator || !m_spInterpolator->AlwaysUpdate()))
		return;

	if (!m_spInterpolator)
		return;

	NiPoint3 kValue;
	if (!m_spInterpolator->Update(m_fScaledTime, m_pkTarget, kValue))
		return;

	NiColor kColor;
	kColor.r = kValue.x;
	kColor.g = kValue.y;
	kColor.b = kValue.z;
	if (kColor.r < 0.0f)
		kColor.r = 0.0f;
	else if (kColor.r > 1.0f)
		kColor.r = 1.0f;
	if (kColor.g < 0.0f)
		kColor.g = 0.0f;
	else if (kColor.g > 1.0f)
		kColor.g = 1.0f;
	if (kColor.b < 0.0f)
		kColor.b = 0.0f;
	else if (kColor.b > 1.0f)
		kColor.b = 1.0f;

	if (!m_pkTarget)
		return;

	NiLight* pkLight = static_cast<NiLight*>(m_pkTarget->DynamicCastMember(&NiLight::ms_RTTI));
	if (m_uFlags & ISAMBIENT_MASK) {
		pkLight->SetAmbientColor(kColor);
		pkLight->SetDiffuseColor(NiColor_BLACK);
	} else {
		pkLight->SetAmbientColor(NiColor_BLACK);
		pkLight->SetDiffuseColor(kColor);
	}
}

const char* NiLightColorController::GetCtlrID() {
	return (m_uFlags & ISAMBIENT_MASK) ? "Ambient" : "Diffuse";
}

// Same function as NiLightDimmerController::InterpTargetIsCorrectType (0xA4D210).
bool NiLightColorController::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
	return NiObject::IsKindOf(&NiLight::ms_RTTI, pkTarget);
}

// Without a target akValue is left as it is.
void NiLightColorController::GetTargetPoint3Value(NiPoint3& kValue) {
	if (!m_pkTarget)
		return;

	NiLight* pkLight = static_cast<NiLight*>(m_pkTarget);
	const NiColor& kColor = (m_uFlags & ISAMBIENT_MASK) ? pkLight->m_kAmb : pkLight->m_kDiff;
	kValue.x = kColor.r;
	kValue.y = kColor.g;
	kValue.z = kColor.b;
}

NiObject* NiLightColorController::CreateObject() {
	void* pkObject = NiMemObject::operator new(sizeof(NiLightColorController));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) NiLightColorController;
}
