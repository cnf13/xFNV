#include "Gamebryo2.2/CoreLibs/NiAnimation/NiExtraDataController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSingleInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiExtraDataController::ms_RTTI("NiExtraDataController", &NiSingleInterpController::ms_RTTI);

NiExtraDataController::NiExtraDataController() {}

NiExtraDataController::~NiExtraDataController() {}

const NiRTTI* NiExtraDataController::GetRTTI() const {
    return &ms_RTTI;
}

// Files before 20.1.0.1 store the name as a plain string.
void NiExtraDataController::LoadBinary(NiStream& kStream) {
    NiSingleInterpController::LoadBinary(kStream);
    if (kStream.GetFileVersion() < NiStream::GetVersion(20, 1, 0, 1)) {
        kStream.LoadCStringAsFixedString(m_kExtraDataName);
    }
    else {
        kStream.LoadFixedString(m_kExtraDataName);
    }
}

void NiExtraDataController::LinkObject(NiStream& kStream) {
    NiSingleInterpController::LinkObject(kStream);
}

bool NiExtraDataController::RegisterStreamables(NiStream& kStream) {
    if (!NiSingleInterpController::RegisterStreamables(kStream)) {
        return false;
    }

    kStream.RegisterFixedString(m_kExtraDataName);
    return true;
}

void NiExtraDataController::SaveBinary(NiStream& kStream) {
    NiSingleInterpController::SaveBinary(kStream);
    kStream.SaveFixedString(m_kExtraDataName);
}

// Extra data compare by value; the names by handle.
bool NiExtraDataController::IsEqual(NiObject* pkObject) {
    if (!NiSingleInterpController::IsEqual(pkObject)) {
        return false;
    }

    NiExtraDataController* pkOther = static_cast<NiExtraDataController*>(pkObject);
    if ((m_spExtraData && !pkOther->m_spExtraData) || (!m_spExtraData && pkOther->m_spExtraData)) {
        return false;
    }

    if (m_spExtraData && pkOther->m_spExtraData && !m_spExtraData->IsEqual(pkOther->m_spExtraData)) {
        return false;
    }

    return pkOther->m_kExtraDataName == m_kExtraDataName;
}

void NiExtraDataController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiSingleInterpController::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("Extra Data Name", static_cast<const char*>(m_kExtraDataName)));
}

// The clone points at the clone of this controller's extra data. GetAt leaves pkClone untouched when the key is
// missing, so pkClone starts null. One local serves both lookups, so a miss on the second lookup leaves the first
// lookup's result, as the game does.
void NiExtraDataController::ProcessClone(NiCloningProcess& kCloning) {
    NiSingleInterpController::ProcessClone(kCloning);

    NiObject* pkClone = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkClone);
    if (!m_pkTarget) {
        return;
    }

    NiExtraDataController* pkDest = static_cast<NiExtraDataController*>(pkClone);
    if (!pkDest->m_pkTarget) {
        return;
    }

    kCloning.m_pkCloneMap->GetAt(m_spExtraData, pkClone);
    pkDest->m_spExtraData = static_cast<NiExtraData*>(pkClone);
}

void NiExtraDataController::PostLinkObject(NiStream& kStream) {
    NiSingleInterpController::PostLinkObject(kStream);
    if (m_pkTarget) {
        GetExtraDataPointerFromName();
    }
}

void NiExtraDataController::SetTarget(NiObjectNET* pkTarget) {
    m_spExtraData = nullptr;
    NiSingleInterpController::SetTarget(pkTarget);
    if (m_pkTarget) {
        GetExtraDataPointerFromName();
    }
}

// The target must be an NiObjectNET that holds an extra data with this controller's (non-null) name.
bool NiExtraDataController::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    if (!NiObject::IsKindOf(&NiObjectNET::ms_RTTI, pkTarget)) {
        return false;
    }

    if (m_kExtraDataName.GetHandle() == NiGlobalStringTable::NULL_STRING) {
        return false;
    }

    for (uint16_t i = 0; i < pkTarget->m_usExtraDataSize; ++i) {
        if (pkTarget->m_ppkExtra[i]->m_kName == m_kExtraDataName) {
            return true;
        }
    }
    return false;
}

// The name's assignment reads the source handle once.
void NiExtraDataController::CopyMembers(NiExtraDataController* pkDest, NiCloningProcess& kCloning) {
    NiSingleInterpController::CopyMembers(pkDest, kCloning);
    NiGlobalStringTable::GlobalStringHandle kHandle = m_kExtraDataName.GetHandle();
    if (pkDest->m_kExtraDataName.GetHandle() != kHandle) {
        NiGlobalStringTable::AcquireHandle(kHandle);
        // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
        NiGlobalStringTable::ReleaseHandle(pkDest->m_kExtraDataName.GetHandle());
        pkDest->m_kExtraDataName.SetHandle(kHandle);
    }
}

void NiExtraDataController::GetExtraDataPointerFromName() {
    m_spExtraData = m_pkTarget->GetExtraData(m_kExtraDataName);
}
