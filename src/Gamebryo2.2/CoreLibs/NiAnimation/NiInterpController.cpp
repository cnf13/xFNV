#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpController.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiInterpController::ms_RTTI("NiInterpController", &NiTimeController::ms_RTTI);

NiInterpController::NiInterpController() {
    SetManagerControlled(false);
}

NiInterpController::~NiInterpController() {}

// Files from 10.1.0.104 up to 10.1.0.109 store the manager-controlled flag on its own; later ones keep it in the
// flag word.
// KEEP: the exe reads the flag byte into the storage of its own kStream argument, so a short read leaves the
// pointer's low byte there and the exe tests that; src cannot share that storage with the argument (checker: frame
// bytes passed to the stream's Read differ).
void NiInterpController::LoadBinary(NiStream& kStream) {
    NiTimeController::LoadBinary(kStream);

    if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 1, 0, 104) &&
        kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 109)) {
        bool bManagerControlled;
        NiStreamLoadBinary(kStream, bManagerControlled);
        SetBit(bManagerControlled, MANAGERCONTROLLED_MASK);
    }
}

void NiInterpController::LinkObject(NiStream& kStream) {
    NiTimeController::LinkObject(kStream);
}

bool NiInterpController::RegisterStreamables(NiStream& kStream) {
    return NiTimeController::RegisterStreamables(kStream);
}

void NiInterpController::SaveBinary(NiStream& kStream) {
    NiTimeController::SaveBinary(kStream);
}

bool NiInterpController::IsEqual(NiObject* pkObject) {
    return NiTimeController::IsEqual(pkObject);
}

void NiInterpController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiTimeController::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(NiInterpController::ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("IsManagerControlled", GetManagerControlled()));
}

// The key range of a manager-controlled controller belongs to its sequences and is left alone.
void NiInterpController::ResetTimeExtrema() {
    if (!GetManagerControlled()) {
        m_fLoKeyTime = 0.0f;
        m_fHiKeyTime = 0.0f;
    }
}

void NiInterpController::CopyMembers(NiInterpController* pkDest, NiCloningProcess& kCloning) {
    NiTimeController::CopyMembers(pkDest, kCloning);
}
