#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <limits>

NiRTTI NiKeyBasedInterpolator::ms_RTTI("NiKeyBasedInterpolator", &NiInterpolator::ms_RTTI);

NiKeyBasedInterpolator::NiKeyBasedInterpolator() {}

NiKeyBasedInterpolator::~NiKeyBasedInterpolator() {}

const NiRTTI* NiKeyBasedInterpolator::GetRTTI() const {
    return &ms_RTTI;
}

// NiInterpolator adds nothing to the stream, so this ends in NiObject's body. The other key based interpolators call
// this function directly.
void NiKeyBasedInterpolator::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);
}

void NiKeyBasedInterpolator::LinkObject(NiStream& kStream) {
    NiInterpolator::LinkObject(kStream);
}

bool NiKeyBasedInterpolator::RegisterStreamables(NiStream& kStream) {
    return NiObject::RegisterStreamables(kStream);
}

void NiKeyBasedInterpolator::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);
}

bool NiKeyBasedInterpolator::IsEqual(NiObject* pkOther) {
    return NiObject::IsEqual(pkOther);
}

void NiKeyBasedInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// The span from the earliest first key to the latest last key over all channels; 0 to 0 when no channel has keys.
void NiKeyBasedInterpolator::GetActiveTimeRange(float& fBeginKeyTime, float& fEndKeyTime) const {
    fBeginKeyTime = std::numeric_limits<float>::max();
    fEndKeyTime = -std::numeric_limits<float>::max();
    for (uint16_t usChannel = 0; usChannel < GetKeyChannelCount(); usChannel++) {
        NiAnimationKey* pkBeginKey = GetKeyAt(0, usChannel);
        NiAnimationKey* pkEndKey = GetKeyAt(GetKeyCount(usChannel) - 1, usChannel);
        if (pkBeginKey && pkEndKey) {
            if (pkBeginKey->GetTime() < fBeginKeyTime) {
                fBeginKeyTime = pkBeginKey->GetTime();
            }
            if (pkEndKey->GetTime() > fEndKeyTime) {
                fEndKeyTime = pkEndKey->GetTime();
            }
        }
    }

    if (fBeginKeyTime == std::numeric_limits<float>::max() && fEndKeyTime == -std::numeric_limits<float>::max()) {
        fBeginKeyTime = 0.0f;
        fEndKeyTime = 0.0f;
    }
}

// Null-safe: a channel without keys yields a null array, which GetActiveTimeRange tests for.
NiAnimationKey* NiKeyBasedInterpolator::GetKeyAt(uint32_t uiWhichKey, uint16_t usChannel) const {
    uint8_t ucKeySize = GetKeyStride(usChannel);
    uintptr_t uiKeys = reinterpret_cast<uintptr_t>(GetKeyArray(usChannel));
    return reinterpret_cast<NiAnimationKey*>(uiKeys + uiWhichKey * ucKeySize);
}

void NiKeyBasedInterpolator::CopyMembers(NiKeyBasedInterpolator* pkDest, NiCloningProcess& kCloning) {
    NiInterpolator::CopyMembers(pkDest, kCloning);
}
