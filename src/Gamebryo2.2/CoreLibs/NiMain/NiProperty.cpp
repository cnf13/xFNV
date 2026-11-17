#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiProperty::ms_RTTI("NiProperty", &NiObjectNET::ms_RTTI);

NiProperty::~NiProperty() {}

// Files older than 10.0.1.2 keep the property flags here; the derived LoadBinary functions read them back.
// KEEP, unregistered (niread): the exe reads the flags into its arStream argument slot (lea [esp+0x1c] at 0xA5D81B)
// and stores that word; a short read leaves the stream pointer's low half there, which src cannot reproduce.
void NiProperty::LoadBinary(NiStream& kStream) {
	NiObjectNET::LoadBinary(kStream);
	if (kStream.m_uiNifFileVersion < 0x0A000102) {
		uint16_t usFlags;
		NiStreamLoadBinary(kStream, usFlags);
		kStream.m_usNiPropertyFlags = usFlags;
	}
}

void NiProperty::LinkObject(NiStream& kStream) {
	NiObjectNET::LinkObject(kStream);
}

bool NiProperty::RegisterStreamables(NiStream& kStream) {
	return NiObjectNET::RegisterStreamables(kStream);
}

void NiProperty::SaveBinary(NiStream& kStream) {
	NiObjectNET::SaveBinary(kStream);
}

bool NiProperty::IsEqual(NiObject* pkObject) {
	return NiObjectNET::IsEqual(pkObject);
}

void NiProperty::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiObjectNET::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

void NiProperty::Update(NiUpdateData& UpdateData) {
	for (NiTimeController* pkControl = m_spControllers; pkControl; pkControl = pkControl->m_spNext) {
		pkControl->Update(UpdateData);
	}
}

void NiProperty::CopyMembers(NiProperty* pkDest, NiCloningProcess& kCloning) {
	NiObjectNET::CopyMembers(pkDest, kCloning);
}
