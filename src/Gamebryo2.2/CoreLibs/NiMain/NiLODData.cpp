#include "Gamebryo2.2/CoreLibs/NiMain/NiLODData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"

NiRTTI NiLODData::ms_RTTI("NiLODData", &NiObject::ms_RTTI);

// Unregistered (exe 0xAA09A0): NiLODData is abstract, so the constructor registration cannot construct it.
NiLODData::NiLODData() {}

NiLODData::~NiLODData() {}

void NiLODData::LoadBinary(NiStream& kStream) {
	NiObject::LoadBinary(kStream);
}

void NiLODData::LinkObject(NiStream& kStream) {
	NiObject::LinkObject(kStream);
}

bool NiLODData::RegisterStreamables(NiStream& kStream) {
	return NiObject::RegisterStreamables(kStream);
}

void NiLODData::SaveBinary(NiStream& kStream) {
	NiObject::SaveBinary(kStream);
}

// Any two LOD data objects compare equal here: the base comparison is not run, the derived classes compare their
// own members.
bool NiLODData::IsEqual(NiObject*) {
	return true;
}
