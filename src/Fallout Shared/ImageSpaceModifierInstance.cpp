#include "Fallout Shared/ImageSpaceModifierInstance.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceManager.h"
#include "Fallout Shared/ImageSpaceModifierInstanceDOF.h"
#include "Fallout Shared/ImageSpaceModifierInstanceDRB.h"
#include "Fallout Shared/ImageSpaceModifierInstanceRB.h"
#include "Fallout Shared/ImageSpaceModifierInstanceTemp.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESSound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

ImageSpaceModifierInstance::ImageSpaceModifierInstance(float afStrength, float afStartTime, NiAVObject* apTarget)
  : fStrength(afStrength), spTarget(apTarget), fStartTime(afStartTime) {}

ImageSpaceModifierInstance::~ImageSpaceModifierInstance() {}

void ImageSpaceModifierInstance::SetStrength(float afStrength) {
    fStrength = afStrength;
}

void ImageSpaceModifierInstance::SetPermanent(bool abValue) {
    if (abValue) {
        iFlags |= PERMANENT;
    }
    else {
        iFlags &= ~PERMANENT;
    }
}

void ImageSpaceModifierInstance::Stop(ImageSpaceModifierInstance* apInstance) {
    BSSimpleList<NiPointer<ImageSpaceModifierInstance>>* pkList = &TES::pInstance->listActiveImageSpaceModifiers;
    while (pkList) {
        ImageSpaceModifierInstance* pInstance = pkList->m_item.m_pObject;
        if (pInstance == apInstance) {
            NiPointer<ImageSpaceModifierInstance> spMemory(pInstance);
            pkList->Remove(spMemory);
        }
        else {
            pkList = pkList->m_pkNext;
        }
    }
}

ImageSpaceModifierInstanceForm::ImageSpaceModifierInstanceForm()
  : ImageSpaceModifierInstance(0.0f, 0.0f, nullptr), pImageSpaceMod(nullptr), pLastImageSpaceMod(nullptr),
    fLastStrength(0.0f), spLastTarget(nullptr), fTransitionTime(0.0f) {
    bIsWeather = false;
}

ImageSpaceModifierInstanceForm::ImageSpaceModifierInstanceForm(
    TESImageSpaceModifier* apMod,
    float afStrength,
    float afAge,
    NiAVObject* apTarget
)
  : ImageSpaceModifierInstance(afAge, afStrength, apTarget), pImageSpaceMod(apMod), pLastImageSpaceMod(nullptr),
    fLastStrength(0.0f), spLastTarget(nullptr), fTransitionTime(0.0f) {
    bIsWeather = false;
}

bool ImageSpaceModifierInstanceForm::IsExpired() {
    return pImageSpaceMod && pImageSpaceMod->IsExpired(fStartTime);
}

void ImageSpaceModifierInstanceForm::Apply() {
    if ((fStrength > 0.0 || fLastStrength > 0.0) && pImageSpaceMod) {
        if (fStartTime < -10000.0) {
            fStartTime = TESImageSpaceModifier::GetTickCount();
        }
        if (pLastImageSpaceMod) {
            float fTickCount = TESImageSpaceModifier::GetTickCount();
            float fPercent = (fTickCount - fStartTime) / fTransitionTime;
            if (fPercent < 1.0) {
                TESImageSpaceModifier::BlendApply(
                    fPercent,
                    pLastImageSpaceMod,
                    pImageSpaceMod,
                    fLastStrength,
                    fStrength,
                    spLastTarget.m_pObject,
                    spTarget.m_pObject
                );
                return;
            }
            pLastImageSpaceMod = nullptr;
            fLastStrength = 0.0f;
            if (bIsWeather) {
                pImageSpaceMod->ApplyWeather(fStartTime, fStrength);
                return;
            }
        }
        else if (bIsWeather) {
            pImageSpaceMod->ApplyWeather(fStartTime, fStrength);
            return;
        }
        pImageSpaceMod->Apply(fStartTime, fStrength, spTarget.m_pObject);
    }
}

// A form instance shows its modifier and, while blending from one, the previous modifier; otherwise its weather
// mark and strength.
void ImageSpaceModifierInstanceForm::PrintInfo(char* apBuffer) {
    if (pLastImageSpaceMod) {
        const char* pName = pImageSpaceMod ? pImageSpaceMod->GetFormEditorID() : "(NULL)";
        BSsprintf(apBuffer, 0x800, "B %s %s", pName, pLastImageSpaceMod->GetFormEditorID());
    }
    else {
        const char* pName = pImageSpaceMod ? pImageSpaceMod->GetFormEditorID() : "(NULL)";
        const char* pWeather = bIsWeather ? "W" : " ";
        BSsprintf(apBuffer, 0x800, "%s %s Str: %1.3f ", pWeather, pName, fStrength);
    }
}

ImageSpaceModifierInstanceForm::~ImageSpaceModifierInstanceForm() {}

void ImageSpaceModifierInstanceForm::SetAge(float afAge) {
    fStartTime = afAge;
}

bool ImageSpaceModifierInstanceForm::StartTransition(
    TESImageSpaceModifier* apMod,
    float afTransitionTime,
    float afStrength,
    NiAVObject* apTarget
) {
    if ((pImageSpaceMod && pImageSpaceMod->Data.bAnimatable) || (apMod && apMod->Data.bAnimatable)) {
        return false;
    }
    pLastImageSpaceMod = pImageSpaceMod;
    fLastStrength = fStrength;
    spLastTarget = spTarget;
    fStartTime = TESImageSpaceModifier::GetTickCount();
    fTransitionTime = afTransitionTime;
    pImageSpaceMod = apMod;
    fStrength = afStrength;
    spTarget = apTarget;
    return true;
}

// Removes every active instance of apMod (the list's Remove copies the next node into a removed head, so the scan stays
// on the same node after a removal) and plays apMod's outro sound once.
void ImageSpaceModifierInstanceForm::Stop(TESImageSpaceModifier* apMod) {
    if (apMod) {
        bool bHasPlayed = false;
        BSSimpleList<NiPointer<ImageSpaceModifierInstance>>* pkList = &TES::pInstance->listActiveImageSpaceModifiers;
        while (pkList) {
            ImageSpaceModifierInstance* pInstance = pkList->m_item.m_pObject;
            if (pInstance && pInstance->IsForm() && pInstance->IsForm()->pImageSpaceMod == apMod) {
                pkList->Remove(NiPointer<ImageSpaceModifierInstance>(pInstance));
                if (apMod->pOutroSound && !bHasPlayed) {
                    bHasPlayed = true;
                    uint32_t uiSoundID = apMod->pOutroSound->iFormID;
                    BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, 0x200000u).Play(false);
                }
            }
            else {
                pkList = pkList->m_pkNext;
            }
        }
    }
}

ImageSpaceModifierInstanceForm*
ImageSpaceModifierInstanceForm::Trigger(TESImageSpaceModifier* apMod, float afStrength, NiAVObject* apTarget) {
    ImageSpaceModifierInstanceForm* pMod = nullptr;
    if (apMod) {
        ImageSpaceModifierInstanceForm*
            pNew = new ImageSpaceModifierInstanceForm(apMod, -10001.0f, afStrength, apTarget);
        pMod = pNew;
        TES::pInstance->listActiveImageSpaceModifiers.AddTail(
            NiPointer<ImageSpaceModifierInstance>(static_cast<ImageSpaceModifierInstance*>(pNew))
        );
        if (apMod->pIntroSound) {
            uint32_t uiSoundID = apMod->pIntroSound->iFormID;
            BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, 0x200000u).Play(false);
        }
    }
    return pMod;
}

ImageSpaceModifierInstanceForm* ImageSpaceModifierInstanceForm::TriggerIfNotActive(
    TESImageSpaceModifier* apMod,
    float afStrength,
    NiAVObject* apTarget
) {
    bool bFound = false;
    ImageSpaceModifierInstanceForm* pMod = nullptr;
    if (apMod) {
        for (BSSimpleList<NiPointer<ImageSpaceModifierInstance>>* pkList =
                 &TES::pInstance->listActiveImageSpaceModifiers;
             pkList;
             pkList = pkList->m_pkNext) {
            ImageSpaceModifierInstance* pInstance = pkList->m_item.m_pObject;
            if (pInstance && pInstance->IsForm() && pInstance->IsForm()->pImageSpaceMod == apMod) {
                bFound = true;
                break;
            }
        }
        if (!bFound) {
            ImageSpaceModifierInstanceForm*
                pNew = new ImageSpaceModifierInstanceForm(apMod, -10001.0f, afStrength, apTarget);
            pMod = pNew;
            TES::pInstance->listActiveImageSpaceModifiers.AddTail(
                NiPointer<ImageSpaceModifierInstance>(static_cast<ImageSpaceModifierInstance*>(pNew))
            );
            if (apMod->pIntroSound) {
                uint32_t uiSoundID = apMod->pIntroSound->iFormID;
                BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, 0x200000u).Play(false);
            }
        }
    }
    return pMod;
}

ImageSpaceModifierInstanceDOF::ImageSpaceModifierInstanceDOF(
    float afStrength,
    float afAge,
    DepthOfFieldMode aMode,
    float afDistance,
    float afRange,
    float afMotionBlur,
    NiAVObject* apTarget,
    float afDuration
)
  : ImageSpaceModifierInstanceTemp(afAge, afStrength, apTarget, afDuration), fDistance(afDistance), fRange(afRange),
    fMaskConst(afMotionBlur), Mode(aMode) {}

ImageSpaceModifierInstanceDOF::~ImageSpaceModifierInstanceDOF() {}

void ImageSpaceModifierInstanceDOF::Apply() {
    if (fStrength > 0.0) {
        if (fStartTime < -10000.0) {
            fStartTime = TESImageSpaceModifier::GetTickCount();
        }
        ImageSpaceManager* pImageSpaceManager = BSShaderManager::GetImageSpaceManager();
        if (spTarget) {
            spTarget->UpdateWorldBound();
            const NiBound& kWorldBound = spTarget->GetWorldBound();
            pImageSpaceManager->ApplyDepthOfFieldParamModifier_0(fStrength, Mode, fMaskConst, &kWorldBound.m_kCenter);
        }
        else {
            pImageSpaceManager->ApplyDepthOfFieldParamModifier(fStrength, Mode, fMaskConst, fDistance, fRange);
        }
    }
}

void ImageSpaceModifierInstanceDOF::PrintInfo(char* apBuffer) {
    BSsprintf(apBuffer, 0x800, "  DOF  Distance: %f  Range: %f  Str: %f", fDistance, fRange, fStrength);
}

ImageSpaceModifierInstanceDOF* ImageSpaceModifierInstanceDOF::Trigger(
    float afDistance,
    float afRange,
    ImageSpaceModifierInstanceDOF::DepthOfFieldMode aMode,
    float afStrength,
    float afMaskConst,
    float afDuration
) {
    ImageSpaceModifierInstanceDOF* pInstance = new ImageSpaceModifierInstanceDOF(
        afStrength,
        -10001.0f,
        aMode,
        afDistance,
        afRange,
        afMaskConst,
        nullptr,
        afDuration
    );
    NiPointer<ImageSpaceModifierInstance> spInstance(static_cast<ImageSpaceModifierInstance*>(pInstance));
    TES::pInstance->listActiveImageSpaceModifiers.AddTail(spInstance);
    return pInstance;
}

ImageSpaceModifierInstanceDRB::ImageSpaceModifierInstanceDRB(
    float afStrength,
    float afAge,
    NiAVObject* apTarget,
    float afDuration,
    DepthOfFieldMode aMode,
    float afDistance,
    float afRange,
    float afRampup,
    float afStart,
    float afRampdown,
    float afDownStart,
    const NiPoint2& aCenter
)
  : ImageSpaceModifierInstanceTemp(afAge, afStrength, apTarget, afDuration), fRampup(afRampup), fStart(afStart),
    fRampdown(afRampdown), fDownStart(afDownStart), Center(aCenter), fDistance(afDistance), fRange(afRange),
    Mode(aMode) {}

ImageSpaceModifierInstanceDRB::~ImageSpaceModifierInstanceDRB() {}

void ImageSpaceModifierInstanceDRB::Apply() {
    if (fStrength > 0.0) {
        if (fStartTime < -10000.0) {
            fStartTime = TESImageSpaceModifier::GetTickCount();
        }
        ImageSpaceManager* pImageSpaceManager = BSShaderManager::GetImageSpaceManager();
        if (spTarget.m_pObject) {
            spTarget->UpdateWorldBound();
            const NiBound& kWorldBound = spTarget->GetWorldBound();
            pImageSpaceManager->ApplyDepthRadialBlurParamModifier(fStrength, Mode, fRampup, fStart, kWorldBound);
        }
        else {
            pImageSpaceManager
                ->ApplyDepthRadialBlurParamModifier(fStrength, Mode, fDistance, fRange, fRampup, fStart, Center);
        }
    }
}

void ImageSpaceModifierInstanceDRB::PrintInfo(char* apBuffer) {
    BSsprintf(apBuffer, 0x800, "  Depth Radial Blur");
}

ImageSpaceModifierInstanceDRB* ImageSpaceModifierInstanceDRB::Trigger(
    float afDistance,
    float afRange,
    ImageSpaceModifierInstanceDRB::DepthOfFieldMode aMode,
    float afStrength,
    float afRampup,
    float afStart,
    float afRampdown,
    float afDownStart,
    const NiPoint2& aCenter,
    float afDuration
) {
    ImageSpaceModifierInstanceDRB* pInstance = new ImageSpaceModifierInstanceDRB(
        afStrength,
        -10001.0f,
        nullptr,
        afDuration,
        aMode,
        afDistance,
        afRange,
        afRampup,
        afStart,
        afRampdown,
        afDownStart,
        aCenter
    );
    NiPointer<ImageSpaceModifierInstance> spInstance(static_cast<ImageSpaceModifierInstance*>(pInstance));
    TES::pInstance->listActiveImageSpaceModifiers.AddTail(spInstance);
    return pInstance;
}

ImageSpaceModifierInstanceRB::ImageSpaceModifierInstanceRB(
    float afStrength,
    float afAge,
    NiAVObject* apTarget,
    float afDuration,
    float afRampup,
    float afStart,
    float afRampdown,
    float afDownStart,
    const NiPoint2& aCenter
)
  : ImageSpaceModifierInstanceTemp(afAge, afStrength, apTarget, afDuration), fRampup(afRampup), fStart(afStart),
    fRampdown(afRampdown), fDownStart(afDownStart), Center(aCenter) {}

ImageSpaceModifierInstanceRB::~ImageSpaceModifierInstanceRB() {}

// Starts the blur clock on the first apply (fStartTime below -10000), then hands the radial blur to the image space
// manager: around the target's world bound (after updating that bound) when there is a target, else around Center. Both
// compares are against doubles.
void ImageSpaceModifierInstanceRB::Apply() {
    if (fStrength > 0.0) {
        if (fStartTime < -10000.0) {
            fStartTime = TESImageSpaceModifier::GetTickCount();
        }
        ImageSpaceManager* pManager = BSShaderManager::GetImageSpaceManager();
        if (spTarget) {
            NiAVObject* pTarget = spTarget;
            pTarget->UpdateWorldBound();
            const NiBound& kBound = spTarget->GetWorldBound();
            pManager->ApplyRadialBlurParamModifier(fStrength, fRampup, fStart, fRampdown, fDownStart, kBound);
        }
        else {
            pManager->ApplyRadialBlurParamModifier(fStrength, fRampup, fStart, fRampdown, fDownStart, Center);
        }
    }
}

void ImageSpaceModifierInstanceRB::PrintInfo(char* apBuffer) {
    BSsprintf(
        apBuffer,
        0x800,
        "  RB  Rampup: %f  Start: %f  Rampdown: %f  DownStart: %f  Str: %f",
        fRampup,
        fStart,
        fRampdown,
        fDownStart,
        fStrength
    );
}

ImageSpaceModifierInstanceRB* ImageSpaceModifierInstanceRB::Trigger(
    float afStrength,
    float afRampup,
    float afStart,
    float afRampdown,
    float afDownStart,
    const NiPoint2& aCenter,
    float afDuration
) {
    ImageSpaceModifierInstanceRB* pInstance = new ImageSpaceModifierInstanceRB(
        afStrength,
        -10001.0f,
        nullptr,
        afDuration,
        afRampup,
        afStart,
        afRampdown,
        afDownStart,
        aCenter
    );
    NiPointer<ImageSpaceModifierInstance> spInstance(static_cast<ImageSpaceModifierInstance*>(pInstance));
    TES::pInstance->listActiveImageSpaceModifiers.AddTail(spInstance);
    return pInstance;
}

ImageSpaceModifierInstanceTemp::ImageSpaceModifierInstanceTemp(
    float afStartTime,
    float afStrength,
    NiAVObject* apTarget,
    float afDuration
)
  : ImageSpaceModifierInstance(afStrength, afStartTime, apTarget), fDuration(afDuration) {}

ImageSpaceModifierInstanceTemp::~ImageSpaceModifierInstanceTemp() {}

bool ImageSpaceModifierInstanceTemp::IsExpired() {
    if (fDuration != 0.0) {
        return fDuration < TESImageSpaceModifier::GetTickCount() - fStartTime;
    }
    return false;
}

void ImageSpaceModifierInstanceTemp::PrintInfo(char* apBuffer) {
    BSsprintf(apBuffer, 0x800, "  Temp");
}

// The image space modifiers' current time (fCurrentTime).
float TESImageSpaceModifier::GetTickCount() {
    return fCurrentTime;
}
