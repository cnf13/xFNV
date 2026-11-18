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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

ImageSpaceModifierInstance::ImageSpaceModifierInstance(float a1, float a2, NiAVObject* a3)
  : fStrength(a1), spTarget(a3), fStartTime(a2) {}

ImageSpaceModifierInstance::~ImageSpaceModifierInstance() {}

void ImageSpaceModifierInstance::SetStrength(float afStrength) {
    fStrength = afStrength;
}

void ImageSpaceModifierInstance::SetPermanent(bool abValue) {
    uint32_t v2;
    if (abValue) {
        v2 = iFlags | 1;
    }
    else {
        v2 = iFlags & 0xFFFFFFFEu;
    }
    iFlags = v2;
}

void ImageSpaceModifierInstance::Stop(ImageSpaceModifierInstance* apInstance) {
    BSSimpleList<NiPointer<ImageSpaceModifierInstance>>* pkList = &TES::pInstance->listActiveImageSpaceModifiers;
    while (pkList) {
        ImageSpaceModifierInstance* pinstance = pkList->m_item.m_pObject;
        if (pinstance == apInstance) {
            NiPointer<ImageSpaceModifierInstance> apMemory(pinstance);
            pkList->Remove(apMemory);
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
    return (bool)((uint8_t)(pImageSpaceMod && pImageSpaceMod->IsExpired(fStartTime)));
}

void ImageSpaceModifierInstanceForm::Apply() {
    NiAVObject* v1;
    NiAVObject* v2;
    NiAVObject* v3;
    int32_t bIsWeather;
    float tickCount;
    float afTime;
    if (((fStrength > 0.0) || (fLastStrength > 0.0)) && pImageSpaceMod) {
        if (fStartTime < (-10000.0)) {
            fStartTime = TESImageSpaceModifier::GetTickCount();
        }
        if (pLastImageSpaceMod) {
            tickCount = TESImageSpaceModifier::GetTickCount();
            afTime = (tickCount - fStartTime) / fTransitionTime;
            if (afTime < 1.0) {
                v3 = spTarget.m_pObject;
                v1 = spLastTarget.m_pObject;
                TESImageSpaceModifier::BlendApply(
                    afTime, pLastImageSpaceMod, pImageSpaceMod, fLastStrength, fStrength, v1, v3
                );
                return;
            }
            pLastImageSpaceMod = nullptr;
            fLastStrength = (float)0.0;
            if (this->bIsWeather) {
                pImageSpaceMod->ApplyWeather(fStartTime, fStrength);
                return;
            }
        }
        else {
            if (this->bIsWeather) {
                bIsWeather = (int32_t)this->bIsWeather;
                pImageSpaceMod->ApplyWeather(fStartTime, fStrength);
                return;
            }
        }
        v2 = spTarget.m_pObject;
        pImageSpaceMod->Apply(fStartTime, fStrength, v2);
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

void ImageSpaceModifierInstanceForm::SetAge(float a2) {
    fStartTime = a2;
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
    spLastTarget.operator=(spTarget);
    fStartTime = TESImageSpaceModifier::GetTickCount();
    fTransitionTime = afTransitionTime;
    pImageSpaceMod = apMod;
    fStrength = afStrength;
    spTarget.operator=(apTarget);
    return true;
}

// Removes every active instance of apMod (the list's Remove copies the next node into a removed head, so the scan stays
// on the same node after a removal) and plays apMod's outro sound once.
void ImageSpaceModifierInstanceForm::Stop(TESImageSpaceModifier* apMod) {
    XFNV_INLINED_LOCAL(NiPointer<ImageSpaceModifierInstance>, spInstance, std::nullopt, std::nullopt);
    if (apMod) {
        bool bHasPlayed = false;
        BSSimpleList<NiPointer<ImageSpaceModifierInstance>>* pkList = &TES::pInstance->listActiveImageSpaceModifiers;
        while (pkList) {
            ImageSpaceModifierInstance* pInstance = pkList->m_item.m_pObject;
            if (pInstance && (pInstance->IsForm() && pInstance->IsForm()->pImageSpaceMod == apMod)) {
                ::new (&spInstance) NiPointer<ImageSpaceModifierInstance>(pInstance);
                pkList->Remove(spInstance);
                spInstance.~NiPointer<ImageSpaceModifierInstance>();
                if ((apMod->pOutroSound) && (!bHasPlayed)) {
                    bHasPlayed = true;
                    uint32_t uiSoundID = apMod->pOutroSound->iFormID;
                    BSAudio* pAudio = &BSAudio::QInstance();
                    pAudio->GetSoundHandleByNumericID(uiSoundID, 0x200000u).Play(false);
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
    XFNV_INLINED_LOCAL(NiPointer<ImageSpaceModifierInstance>, spForm, std::nullopt, std::nullopt);
    ImageSpaceModifierInstanceForm* pMod = nullptr;
    if (apMod) {
        ImageSpaceModifierInstanceForm* pNew;
        void* pMemory = NiMemObject::operator new(sizeof(ImageSpaceModifierInstanceForm));
        if (pMemory) {
            pNew = ::new (pMemory) ImageSpaceModifierInstanceForm(apMod, -10001.0f, afStrength, apTarget);
        }
        else {
            pNew = nullptr;
        }
        pMod = pNew;
        ::new (&spForm) NiPointer<ImageSpaceModifierInstance>(static_cast<ImageSpaceModifierInstance*>(pNew));
        TES::pInstance->listActiveImageSpaceModifiers.AddTail(spForm);
        spForm.~NiPointer<ImageSpaceModifierInstance>();
        if (apMod->pIntroSound) {
            uint32_t uiSoundID = apMod->pIntroSound->iFormID;
            BSAudio* pAudio = &BSAudio::QInstance();
            pAudio->GetSoundHandleByNumericID(uiSoundID, 0x200000u).Play(false);
        }
    }
    return pMod;
}

ImageSpaceModifierInstanceForm* ImageSpaceModifierInstanceForm::TriggerIfNotActive(
    TESImageSpaceModifier* apMod,
    float afStrength,
    NiAVObject* apTarget
) {
    XFNV_INLINED_LOCAL(NiPointer<ImageSpaceModifierInstance>, spForm, std::nullopt, std::nullopt);
    bool bFound = false;
    ImageSpaceModifierInstanceForm* pMod = nullptr;
    if (apMod) {
        for (BSSimpleList<NiPointer<ImageSpaceModifierInstance>>* pkList =
                 &TES::pInstance->listActiveImageSpaceModifiers;
             pkList; pkList = pkList->m_pkNext) {
            ImageSpaceModifierInstance* pInstance = pkList->m_item.m_pObject;
            if (pInstance && (pInstance->IsForm() && pInstance->IsForm()->pImageSpaceMod == apMod)) {
                bFound = true;
                break;
            }
        }
        if (!bFound) {
            ImageSpaceModifierInstanceForm* pNew;
            void* pMemory = NiMemObject::operator new(sizeof(ImageSpaceModifierInstanceForm));
            if (pMemory) {
                pNew = ::new (pMemory) ImageSpaceModifierInstanceForm(apMod, -10001.0f, afStrength, apTarget);
            }
            else {
                pNew = nullptr;
            }
            pMod = pNew;
            ::new (&spForm) NiPointer<ImageSpaceModifierInstance>(static_cast<ImageSpaceModifierInstance*>(pNew));
            TES::pInstance->listActiveImageSpaceModifiers.AddTail(spForm);
            spForm.~NiPointer<ImageSpaceModifierInstance>();
            if (apMod->pIntroSound) {
                uint32_t uiSoundID = apMod->pIntroSound->iFormID;
                BSAudio* pAudio = &BSAudio::QInstance();
                pAudio->GetSoundHandleByNumericID(uiSoundID, 0x200000u).Play(false);
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
    NiAVObject* v1;
    const NiBound* worldBound;
    NiAVObject* v3;
    ImageSpaceManager* imageSpaceManager;
    if (fStrength > 0.0) {
        if (fStartTime < (-10000.0)) {
            fStartTime = TESImageSpaceModifier::GetTickCount();
        }
        imageSpaceManager = BSShaderManager::GetImageSpaceManager();
        NiAVObject* vCondition1;
        vCondition1 = (&spTarget)->operator->();
        if (vCondition1) {
            v3 = (&spTarget)->operator->();
            v3->UpdateWorldBound();
            v1 = (&spTarget)->operator->();
            worldBound = &v1->GetWorldBound();
            imageSpaceManager->ApplyDepthOfFieldParamModifier_0(fStrength, Mode, fMaskConst, &worldBound->m_kCenter);
        }
        else {
            imageSpaceManager->ApplyDepthOfFieldParamModifier(fStrength, Mode, fMaskConst, fDistance, fRange);
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
    ImageSpaceModifierInstanceDOF* pInstance;
    void* pMemory = NiMemObject::operator new(sizeof(ImageSpaceModifierInstanceDOF));
    if (pMemory) {
        pInstance = ::new (pMemory) ImageSpaceModifierInstanceDOF(
            afStrength, -10001.0f, aMode, afDistance, afRange, afMaskConst, nullptr, afDuration
        );
    }
    else {
        pInstance = nullptr;
    }
    NiPointer<ImageSpaceModifierInstance> spInstance(static_cast<ImageSpaceModifierInstance*>(pInstance));
    TES::pInstance->listActiveImageSpaceModifiers.AddTail(spInstance);
    return pInstance;
}

ImageSpaceModifierInstanceDRB::ImageSpaceModifierInstanceDRB(
    float afArg2,
    float afArg3,
    NiAVObject* apArg4,
    float afArg5,
    DepthOfFieldMode aeArg6,
    float afArg7,
    float afArg8,
    float afArg9,
    float afArg10,
    float afArg11,
    float afArg12,
    const NiPoint2& arArg13
)
  : ImageSpaceModifierInstanceTemp(afArg3, afArg2, apArg4, afArg5), fRampup(afArg9), fStart(afArg10),
    fRampdown(afArg11), fDownStart(afArg12), Center(arArg13), fDistance(afArg7), fRange(afArg8), Mode(aeArg6) {}

ImageSpaceModifierInstanceDRB::~ImageSpaceModifierInstanceDRB() {}

void ImageSpaceModifierInstanceDRB::Apply() {
    if (fStrength > 0.0) {
        if (fStartTime < -10000.0) {
            fStartTime = TESImageSpaceModifier::GetTickCount();
        }
        ImageSpaceManager* ImageSpaceManager_ = BSShaderManager::GetImageSpaceManager();
        if (spTarget.m_pObject) {
            NiAVObject* v3 = spTarget.m_pObject;
            v3->UpdateWorldBound();
            NiAVObject* v1 = spTarget.m_pObject;
            const NiBound* WorldBound = &v1->GetWorldBound();
            ImageSpaceManager_->ApplyDepthRadialBlurParamModifier(fStrength, Mode, fRampup, fStart, *WorldBound);
        }
        else {
            ImageSpaceManager_->ApplyDepthRadialBlurParamModifier(
                fStrength, Mode, fDistance, fRange, fRampup, fStart, Center
            );
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
    ImageSpaceModifierInstanceDRB* pInstance;
    void* pMemory = NiMemObject::operator new(sizeof(ImageSpaceModifierInstanceDRB));
    if (pMemory) {
        pInstance = ::new (pMemory) ImageSpaceModifierInstanceDRB(
            afStrength, -10001.0f, nullptr, afDuration, aMode, afDistance, afRange, afRampup, afStart, afRampdown,
            afDownStart, aCenter
        );
    }
    else {
        pInstance = nullptr;
    }
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
        if (spTarget.operator->()) {
            NiAVObject* pTarget = spTarget.operator->();
            pTarget->UpdateWorldBound();
            const NiBound& kBound = spTarget.operator->()->GetWorldBound();
            pManager->ApplyRadialBlurParamModifier(fStrength, fRampup, fStart, fRampdown, fDownStart, kBound);
        }
        else {
            pManager->ApplyRadialBlurParamModifier(fStrength, fRampup, fStart, fRampdown, fDownStart, Center);
        }
    }
}

void ImageSpaceModifierInstanceRB::PrintInfo(char* apBuffer) {
    BSsprintf(
        apBuffer, 0x800, "  RB  Rampup: %f  Start: %f  Rampdown: %f  DownStart: %f  Str: %f", fRampup, fStart,
        fRampdown, fDownStart, fStrength
    );
}

ImageSpaceModifierInstanceRB* ImageSpaceModifierInstanceRB::Trigger_0052A440(
    float afStrength,
    float afRampup,
    float afStart,
    float afRampdown,
    float afDownStart,
    const NiPoint2& aCenter,
    float afDuration
) {
    ImageSpaceModifierInstanceRB* pInstance;
    void* pMemory = NiMemObject::operator new(sizeof(ImageSpaceModifierInstanceRB));
    if (pMemory) {
        pInstance = ::new (pMemory) ImageSpaceModifierInstanceRB(
            afStrength, -10001.0f, nullptr, afDuration, afRampup, afStart, afRampdown, afDownStart, aCenter
        );
    }
    else {
        pInstance = nullptr;
    }
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
    int8_t v2 = 0;
    if (fDuration != 0.0) {
        return (bool)((uint8_t)(fDuration < (TESImageSpaceModifier::GetTickCount() - fStartTime)));
    }
    return (bool)v2;
}

void ImageSpaceModifierInstanceTemp::PrintInfo(char* apBuffer) {
    BSsprintf(apBuffer, 0x800, "  Temp");
}

// The image space modifiers' current time (fCurrentTime).
float TESImageSpaceModifier::GetTickCount() {
    return fCurrentTime;
}
