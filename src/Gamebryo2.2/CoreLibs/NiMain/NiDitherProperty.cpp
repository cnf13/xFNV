#include "Gamebryo2.2/CoreLibs/NiMain/NiDitherProperty.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiDitherProperty::ms_RTTI("NiDitherProperty", &NiProperty::ms_RTTI);
NiPointer<NiDitherProperty> NiDitherProperty::ms_spDefault;

NiDitherProperty::NiDitherProperty() : m_uFlags(0) {}

NiDitherProperty::~NiDitherProperty() {}

NiObject* NiDitherProperty::CreateClone(NiCloningProcess& kCloning) {
	NiDitherProperty* pkObject = static_cast<NiDitherProperty*>(NiMemObject::operator new(sizeof(NiDitherProperty)));
	NiDitherProperty* pkObject_2 = pkObject;
	if (pkObject) {
		pkObject_2 = ::new (pkObject) NiDitherProperty;
	} else {
		pkObject_2 = 0;
	}
	CopyMembers(pkObject_2, kCloning);
	pkObject_2->m_uFlags = m_uFlags;
	return pkObject_2;
}

// Shared with NiShadeProperty and NiSpecularProperty (flags at +0x18 in all).
void NiDitherProperty::LoadBinary(NiStream& kStream) {
	NiProperty::LoadBinary(kStream);
	if (kStream.m_uiNifFileVersion < 0x0A000102)
		m_uFlags = kStream.m_usNiPropertyFlags & 1;
	else
		NiStreamLoadBinary(kStream, m_uFlags);
}

void NiDitherProperty::LinkObject(NiStream& kStream) {
	NiProperty::LinkObject(kStream);
}

bool NiDitherProperty::RegisterStreamables(NiStream& kStream) {
	return NiProperty::RegisterStreamables(kStream);
}

void NiDitherProperty::SaveBinary(NiStream& kStream) {
	NiProperty::SaveBinary(kStream);
	NiStreamSaveBinary(kStream, m_uFlags);
}

bool NiDitherProperty::IsEqual(NiObject* pkObject) {
	if (!NiProperty::IsEqual(pkObject))
		return false;

	return (m_uFlags & 1) == (static_cast<NiDitherProperty*>(pkObject)->m_uFlags & 1);
}

void NiDitherProperty::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiProperty::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pkStrings->Add(NiGetViewerString("m_bDither", (m_uFlags & 1) != 0));
}

NiObject* NiDitherProperty::CreateObject() {
	NiDitherProperty* pkObject = static_cast<NiDitherProperty*>(NiMemObject::operator new(sizeof(NiDitherProperty)));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) NiDitherProperty;
}
