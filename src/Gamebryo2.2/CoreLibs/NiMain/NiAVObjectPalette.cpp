#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObjectPalette.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiAVObjectPalette::ms_RTTI("NiAVObjectPalette", &NiObject::ms_RTTI);

const NiRTTI* NiAVObjectPalette::GetRTTI() const {
	return &ms_RTTI;
}

// A jump to the NiObject body, shared with other classes.
void NiAVObjectPalette::LoadBinary(NiStream& kStream) {
	NiObject::LoadBinary(kStream);
}

void NiAVObjectPalette::LinkObject(NiStream& kStream) {
	NiObject::LinkObject(kStream);
}

// Shared with BSAnimNote and the IK notes.
bool NiAVObjectPalette::RegisterStreamables(NiStream& kStream) {
	return NiObject::RegisterStreamables(kStream);
}

void NiAVObjectPalette::SaveBinary(NiStream& kStream) {
	NiObject::SaveBinary(kStream);
}

bool NiAVObjectPalette::IsEqual(NiObject* pkObject) {
	return NiObject::IsEqual(pkObject);
}

void NiAVObjectPalette::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiObject::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

void NiAVObjectPalette::CopyMembers(NiAVObjectPalette* pkDest, NiCloningProcess& kCloning) {
	NiObject::CopyMembers(pkDest, kCloning);
}
