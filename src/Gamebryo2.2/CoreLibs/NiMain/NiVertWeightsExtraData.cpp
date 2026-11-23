#include "Gamebryo2.2/CoreLibs/NiMain/NiVertWeightsExtraData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiVertWeightsExtraData::ms_RTTI("NiVertWeightsExtraData", &NiExtraData::ms_RTTI);

NiVertWeightsExtraData::NiVertWeightsExtraData() {}

NiVertWeightsExtraData::~NiVertWeightsExtraData() {}

NiObject* NiVertWeightsExtraData::CreateClone(NiCloningProcess& kCloning) {
    NiVertWeightsExtraData* pkObject = new NiVertWeightsExtraData;
    NiExtraData::CopyMembers(pkObject, kCloning);
    return pkObject;
}

// Only files before 5.0.0.16 hold any data (not even the name after that); the weights are read and dropped.
void NiVertWeightsExtraData::LoadBinary(NiStream& kStream) {
    if (kStream.GetFileVersion() >= NiStream::GetVersion(5, 0, 0, 16)) {
        return;
    }

    NiExtraData::LoadBinary(kStream);

    // uiNumWeights is pre-set to the stream's address, so a read that stores nothing leaves that value as the count.
    uint32_t uiNumWeights = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&kStream));
    NiStreamLoadBinary(kStream, uiNumWeights);
    for (uint32_t i = 0; i < uiNumWeights; i++) {
        float fWeight;
        NiStreamLoadBinary(kStream, fWeight);
    }
}

void NiVertWeightsExtraData::LinkObject(NiStream& kStream) {
    NiExtraData::LinkObject(kStream);
}

bool NiVertWeightsExtraData::RegisterStreamables(NiStream& kStream) {
    return NiExtraData::RegisterStreamables(kStream);
}

bool NiVertWeightsExtraData::IsEqual(NiObject* pObject) {
    return NiExtraData::IsEqual(pObject);
}

void NiVertWeightsExtraData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiExtraData::GetViewerStrings(pStrings);

    pStrings->Add(NiGetViewerString(NiVertWeightsExtraData::ms_RTTI.GetName()));
}

NiObject* NiVertWeightsExtraData::CreateObject() {
    NiVertWeightsExtraData* pkObject = new NiVertWeightsExtraData;
    return pkObject;
}
