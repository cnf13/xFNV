#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSequenceStreamHelper.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"

NiRTTI NiSequenceStreamHelper::ms_RTTI("NiSequenceStreamHelper", &NiObjectNET::ms_RTTI);

NiSequenceStreamHelper::NiSequenceStreamHelper() : NiObjectNET() {}

NiSequenceStreamHelper::~NiSequenceStreamHelper() {}

const NiRTTI* NiSequenceStreamHelper::GetRTTI() const {
    return &ms_RTTI;
}

// The streaming slots below are jumps to the NiObjectNET bodies, shared with other NiObjectNET subclasses.
void NiSequenceStreamHelper::LoadBinary(NiStream& stream) {
    NiObjectNET::LoadBinary(stream);
}

void NiSequenceStreamHelper::LinkObject(NiStream& stream) {
    NiObjectNET::LinkObject(stream);
}

bool NiSequenceStreamHelper::RegisterStreamables(NiStream& stream) {
    return NiObjectNET::RegisterStreamables(stream);
}

void NiSequenceStreamHelper::SaveBinary(NiStream& stream) {
    NiObjectNET::SaveBinary(stream);
}

bool NiSequenceStreamHelper::IsEqual(NiObject* pObject) {
    return NiObjectNET::IsEqual(pObject);
}

NiObject* NiSequenceStreamHelper::CreateObject() {
    return new NiSequenceStreamHelper;
}
