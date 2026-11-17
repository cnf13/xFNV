#include "BSMain/BSPSysSimpleColorModifier.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

NiRTTI BSPSysSimpleColorModifier::ms_RTTI("BSPSysSimpleColorModifier", &NiPSysModifier::ms_RTTI);

// The colors start black and transparent; the fade points are deliberately left unset, as the game does.
BSPSysSimpleColorModifier::BSPSysSimpleColorModifier() : kColor1(), kColor2(), kColor3() {}

// The base takes the name as a temporary NiFixedString, at order 3000.
BSPSysSimpleColorModifier::BSPSysSimpleColorModifier(
    char* pcString,
    NiColorA* a3,
    NiColorA* a4,
    NiColorA* a5,
    float a6,
    float a7,
    float a8,
    float a9,
    float a10,
    float a11
)
  : NiPSysModifier(NiFixedString(pcString), 3000u), kColor1(*a3), kColor2(*a4), kColor3(*a5), fFadeInPct(a6),
    fFadeOutPct(a7), fColor1EndPct(a8), fColor2StartPct(a9), fColor2EndPct(a10), fColor3StartPct(a11) {}

BSPSysSimpleColorModifier::~BSPSysSimpleColorModifier() {}

NiObject* BSPSysSimpleColorModifier::CreateClone(NiCloningProcess& kCloning) {
    BSPSysSimpleColorModifier* pkObject;
    BSPSysSimpleColorModifier* apMemory =
        static_cast<BSPSysSimpleColorModifier*>(NiMemObject::operator new(sizeof(BSPSysSimpleColorModifier)));
    if (apMemory) {
        pkObject = ::new (apMemory) BSPSysSimpleColorModifier;
    }
    else {
        pkObject = nullptr;
    }
    CopyMembers(pkObject, kCloning);
    return pkObject;
}

// The percentages come first, then the three colors. Each float is loaded with NiStreamLoadBinary<uint32_t>, which
// treats the field as 4 untyped bytes.
void BSPSysSimpleColorModifier::LoadBinary(NiStream& kStream) {
    NiPSysModifier::LoadBinary(kStream);

    NiStreamLoadBinary<uint32_t>(kStream, reinterpret_cast<uint32_t&>(fFadeInPct));
    NiStreamLoadBinary<uint32_t>(kStream, reinterpret_cast<uint32_t&>(fFadeOutPct));
    NiStreamLoadBinary<uint32_t>(kStream, reinterpret_cast<uint32_t&>(fColor1EndPct));
    NiStreamLoadBinary<uint32_t>(kStream, reinterpret_cast<uint32_t&>(fColor2StartPct));
    NiStreamLoadBinary<uint32_t>(kStream, reinterpret_cast<uint32_t&>(fColor2EndPct));
    NiStreamLoadBinary<uint32_t>(kStream, reinterpret_cast<uint32_t&>(fColor3StartPct));
    kColor1.LoadBinary(kStream);
    kColor2.LoadBinary(kStream);
    kColor3.LoadBinary(kStream);
}

void BSPSysSimpleColorModifier::LinkObject(NiStream& kStream) {
    NiPSysModifier::LinkObject(kStream);
}

bool BSPSysSimpleColorModifier::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifier::RegisterStreamables(kStream);
}

// Each float goes through NiAnimationKey::SaveBinary, which writes the 4 bytes at the pointer it is given as untyped
// memory.
void BSPSysSimpleColorModifier::SaveBinary(NiStream& kStream) {
    NiPSysModifier::SaveBinary(kStream);

    NiAnimationKey::SaveBinary(kStream, reinterpret_cast<NiAnimationKey*>(&fFadeInPct));
    NiAnimationKey::SaveBinary(kStream, reinterpret_cast<NiAnimationKey*>(&fFadeOutPct));
    NiAnimationKey::SaveBinary(kStream, reinterpret_cast<NiAnimationKey*>(&fColor1EndPct));
    NiAnimationKey::SaveBinary(kStream, reinterpret_cast<NiAnimationKey*>(&fColor2StartPct));
    NiAnimationKey::SaveBinary(kStream, reinterpret_cast<NiAnimationKey*>(&fColor2EndPct));
    NiAnimationKey::SaveBinary(kStream, reinterpret_cast<NiAnimationKey*>(&fColor3StartPct));
    kColor1.SaveBinary(kStream);
    kColor2.SaveBinary(kStream);
    kColor3.SaveBinary(kStream);
}

// The percentages are compared first, then the colors.
bool BSPSysSimpleColorModifier::IsEqual(NiObject* pkObject) {
    if (!NiPSysModifier::IsEqual(pkObject)) {
        return false;
    }

    BSPSysSimpleColorModifier* pkOther = static_cast<BSPSysSimpleColorModifier*>(pkObject);
    if (fFadeInPct != pkOther->fFadeInPct || fFadeOutPct != pkOther->fFadeOutPct ||
        fColor1EndPct != pkOther->fColor1EndPct || fColor2StartPct != pkOther->fColor2StartPct ||
        fColor2EndPct != pkOther->fColor2EndPct || fColor3StartPct != pkOther->fColor3StartPct) {
        return false;
    }
    if (!(pkOther->kColor1 == kColor1) || !(pkOther->kColor2 == kColor2) || !(pkOther->kColor3 == kColor3)) {
        return false;
    }
    return true;
}

// The percentages are shown scaled to 0..100.
void BSPSysSimpleColorModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifier::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(BSPSysSimpleColorModifier::ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("Color 1 End Pct", static_cast<float>(fColor1EndPct * 100.0)));
    pkStrings->Add(NiGetViewerString("Color 2 Start Pct", static_cast<float>(fColor2StartPct * 100.0)));
    pkStrings->Add(NiGetViewerString("Color 2 End Pct", static_cast<float>(fColor2EndPct * 100.0)));
    pkStrings->Add(NiGetViewerString("Color 3 Start Pct", static_cast<float>(fColor3StartPct * 100.0)));
    pkStrings->Add(NiGetViewerString("Fade In Pct", static_cast<float>(fFadeInPct * 100.0)));
    pkStrings->Add(NiGetViewerString("Fade Out Pct", static_cast<float>(fFadeOutPct * 100.0)));
    pkStrings->Add(kColor1.GetViewerString("Color 1"));
    pkStrings->Add(kColor2.GetViewerString("Color 2"));
    pkStrings->Add(kColor3.GetViewerString("Color 3"));
}

// Over each particle's life (age / life span): RGB holds color 1 until the color 1 end, blends to color 2 by the
// color 2 start, holds color 2 until the color 2 end, blends to color 3 by the color 3 start and then holds it.
// Alpha blends from color 1's up to color 2's over the fade-in and from color 2's to color 3's over the
// fade-out. A zero-length blend or a zero fade percentage skips that stage.
void BSPSysSimpleColorModifier::Update(float fTime, NiPSysData* pkData) {
    NiColorA* pkColor = pkData->m_pkColor;
    if (!pkColor) {
        return;
    }

    float fBlend12 = fColor2StartPct - fColor1EndPct;
    float fBlend23 = fColor3StartPct - fColor2EndPct;
    uint16_t usActive = pkData->m_usActive;
    for (uint16_t i = 0; i < usActive; i++) {
        NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[i];
        float fLifePct = pkInfo->m_fAge / pkInfo->m_fLifeSpan;

        if (fBlend12 != 0.0 && fColor2StartPct > fLifePct) {
            if (fColor1EndPct > fLifePct) {
                pkColor[i].r = kColor1.r;
                pkColor[i].g = kColor1.g;
                pkColor[i].b = kColor1.b;
            }
            else {
                float fT = (fLifePct - fColor1EndPct) / fBlend12;
                pkColor[i].r = (kColor2.r - kColor1.r) * fT + kColor1.r;
                pkColor[i].g = (kColor2.g - kColor1.g) * fT + kColor1.g;
                pkColor[i].b = (kColor2.b - kColor1.b) * fT + kColor1.b;
            }
        }
        else if (fBlend23 != 0.0 && fColor2EndPct < fLifePct) {
            if (fColor3StartPct < fLifePct) {
                pkColor[i].r = kColor3.r;
                pkColor[i].g = kColor3.g;
                pkColor[i].b = kColor3.b;
            }
            else {
                float fT = (fLifePct - fColor2EndPct) / fBlend23;
                pkColor[i].r = (kColor3.r - kColor2.r) * fT + kColor2.r;
                pkColor[i].g = (kColor3.g - kColor2.g) * fT + kColor2.g;
                pkColor[i].b = (kColor3.b - kColor2.b) * fT + kColor2.b;
            }
        }
        else {
            pkColor[i].r = kColor2.r;
            pkColor[i].g = kColor2.g;
            pkColor[i].b = kColor2.b;
        }

        if (fFadeOutPct != 0.0 && fFadeOutPct < fLifePct) {
            float fT = (fLifePct - fFadeOutPct) / (1.0f - fFadeOutPct);
            pkColor[i].a = (kColor3.a - kColor2.a) * fT + kColor2.a;
        }
        else if (fFadeInPct != 0.0 && fFadeInPct > fLifePct) {
            float fT = fLifePct / fFadeInPct;
            pkColor[i].a = (kColor2.a - kColor1.a) * fT + kColor1.a;
        }
        else {
            pkColor[i].a = kColor2.a;
        }
    }
}

void BSPSysSimpleColorModifier::CopyMembers(BSPSysSimpleColorModifier* pkDest, NiCloningProcess& kCloning) {
    NiPSysModifier::CopyMembers(pkDest, kCloning);
    pkDest->kColor1 = kColor1;
    pkDest->kColor2 = kColor2;
    pkDest->kColor3 = kColor3;
    pkDest->fFadeInPct = fFadeInPct;
    pkDest->fFadeOutPct = fFadeOutPct;
    pkDest->fColor1EndPct = fColor1EndPct;
    pkDest->fColor2StartPct = fColor2StartPct;
    pkDest->fColor2EndPct = fColor2EndPct;
    pkDest->fColor3StartPct = fColor3StartPct;
}

NiObject* BSPSysSimpleColorModifier::CreateObject() {
    void* apMemory = NiMemObject::operator new(sizeof(BSPSysSimpleColorModifier));
    if (apMemory) {
        return ::new (apMemory) BSPSysSimpleColorModifier;
    }

    return nullptr;
}
