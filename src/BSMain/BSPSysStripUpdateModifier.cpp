#include "BSMain/BSPSysStripUpdateModifier.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/BSStripPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

NiRTTI BSPSysStripUpdateModifier::ms_RTTI("BSPSysStripUpdateModifier", &NiPSysModifier::ms_RTTI);

// The default constructor deliberately leaves fUpdateDeltaTime unset.
BSPSysStripUpdateModifier::BSPSysStripUpdateModifier() {}

// The base takes the name as a temporary NiFixedString, at order 2500.
BSPSysStripUpdateModifier::BSPSysStripUpdateModifier(const char* apcName, float afDelta)
  : NiPSysModifier(NiFixedString(apcName), 2500u), fUpdateDeltaTime(afDelta) {}

BSPSysStripUpdateModifier::~BSPSysStripUpdateModifier() {}

NiObject* BSPSysStripUpdateModifier::CreateClone(NiCloningProcess& kCloning) {
    BSPSysStripUpdateModifier* updated;
    updated = new BSPSysStripUpdateModifier;
    CopyMembers(updated, kCloning);
    return updated;
}

// The float is read through NiStreamLoadBinary<uint32_t>, which treats the field as 4 untyped bytes.
void BSPSysStripUpdateModifier::LoadBinary(NiStream& kStream) {
    NiPSysModifier::LoadBinary(kStream);

    NiStreamLoadBinary<uint32_t>(kStream, reinterpret_cast<uint32_t&>(fUpdateDeltaTime));
}

void BSPSysStripUpdateModifier::LinkObject(NiStream& kStream) {
    NiPSysModifier::LinkObject(kStream);
}

bool BSPSysStripUpdateModifier::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifier::RegisterStreamables(kStream);
}

// The float goes through NiAnimationKey::SaveBinary, which writes the 4 bytes at the pointer it is given as untyped
// memory.
void BSPSysStripUpdateModifier::SaveBinary(NiStream& kStream) {
    NiPSysModifier::SaveBinary(kStream);

    NiAnimationKey::SaveBinary(kStream, reinterpret_cast<NiAnimationKey*>(&fUpdateDeltaTime));
}

bool BSPSysStripUpdateModifier::IsEqual(NiObject* pkObject) {
    if (!NiPSysModifier::IsEqual(pkObject)) {
        return false;
    }

    BSPSysStripUpdateModifier* pkOther = static_cast<BSPSysStripUpdateModifier*>(pkObject);
    return pkOther->fUpdateDeltaTime == fUpdateDeltaTime;
}

void BSPSysStripUpdateModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifier::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(BSPSysStripUpdateModifier::ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("fUpdateDeltaTime", fUpdateDeltaTime));
}

// Advances every strip whose last update is more than fUpdateDeltaTime old. The particle pointer pkInfo is never read.
void BSPSysStripUpdateModifier::Update(float afTime, NiPSysData* apData) {
    BSStripPSysData* pkData = static_cast<BSStripPSysData*>(apData);
    uint16_t usActive = apData->m_usActive;
    NiParticleInfo* pkInfo = apData->m_pkParticleInfo;
    for (uint16_t usStrip = 0; usStrip < usActive; ++usStrip, ++pkInfo) {
        float fElapsed = afTime - pkData->GetPointLastUpdateTime(usStrip);
        if (fUpdateDeltaTime < fElapsed) {
            pkData->UpdatePoints(usStrip, afTime);
        }
    }
}

void BSPSysStripUpdateModifier::Initialize(NiPSysData* apData, uint16_t ausNewParticle) {
    BSStripPSysData* pkData = static_cast<BSStripPSysData*>(apData);
    pkData->InitializePoint(ausNewParticle);
}

void BSPSysStripUpdateModifier::CopyMembers(BSPSysStripUpdateModifier* pkDest, NiCloningProcess& kCloning) {
    NiPSysModifier::CopyMembers(pkDest, kCloning);
}

NiObject* BSPSysStripUpdateModifier::CreateObject() {
    return new BSPSysStripUpdateModifier;
}
