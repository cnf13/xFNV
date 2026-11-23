#include "Fallout Shared/TESObjectACTI.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSTaskManager.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

TESObjectACTI::TESObjectACTI() {
    cFormType = 21;
    TESObjectACTI::InitializeData();
    SetInitialized(true);
}

TESObjectACTI::~TESObjectACTI() {}

void TESObjectACTI::InitializeData() {
    pSoundLoop = nullptr;
    pSoundActivate = nullptr;
    pWaterForm = nullptr;
    pRadioStation = nullptr;
    pRadioTemplate = nullptr;
    ActivateTextOverride = "";
}

// Reads the ACTI record: the name, model, script and destruction components, the sounds, the water type, the radio
// station and the activation prompt.
bool TESObjectACTI::Load(TESFile* apFile) {
    LoadForm(apFile);
    SetInitialized(false);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: static_cast<TESBoundObject*>(this)->LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile); break;
            case MODD_ID:
            case MOSD_ID:
            case MODL_ID:
            case MODT_ID:
                TESModel::LoadModelChunk(static_cast<TESModel*>(static_cast<TESModelTextureSwap*>(this)), apFile);
                break;
            case MODS_ID: {
                const uint32_t uiSize = apFile->GetChunkSize();
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiSize));
                apFile->GetChunkData(pBuffer, uiSize);
                const uint16_t usFormVersion = apFile->GetFormVersion();
                static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(pBuffer, uiSize, usFormVersion);
                break;
            }
            case SCRI_ID: {
                uint32_t uiScript = 0;
                apFile->GetChunkData<uint32_t>(uiScript);
                TESScriptableForm* pScriptableForm = static_cast<TESScriptableForm*>(this);
                // Stores the chunk's value in the pointer field unchanged.
                pScriptableForm->SetFormScript(reinterpret_cast<Script*>(uiScript));
                pScriptableForm->InitItem(this);
                break;
            }
            case DSTD_ID:
            case DEST_ID:
                BGSDestructibleObjectForm::LoadChunk(static_cast<BGSDestructibleObjectForm*>(this), apFile);
                break;
            case INAM_ID: {
                uint32_t uiRadioTemplate = 0;
                apFile->GetChunkData<uint32_t>(uiRadioTemplate);
                // Stores the chunk's value in the pointer field unchanged.
                SetRadioTemplate(reinterpret_cast<TESSound*>(uiRadioTemplate));
                break;
            }
            case RNAM_ID: {
                uint32_t uiRadioStation = 0;
                apFile->GetChunkData<uint32_t>(uiRadioStation);
                // Stores the chunk's value in the pointer field unchanged.
                SetRadioStation(reinterpret_cast<BGSTalkingActivator*>(uiRadioStation));
                break;
            }
            case SNAM_ID: {
                uint32_t uiSoundLoop = 0;
                apFile->GetChunkData<uint32_t>(uiSoundLoop);
                // Stores the chunk's value in the pointer field unchanged.
                SetLoopingSound(reinterpret_cast<TESSound*>(uiSoundLoop));
                break;
            }
            case VNAM_ID: {
                uint32_t uiSoundActivate = 0;
                apFile->GetChunkData<uint32_t>(uiSoundActivate);
                // Stores the chunk's value in the pointer field unchanged.
                SetActivateSound(reinterpret_cast<TESSound*>(uiSoundActivate));
                break;
            }
            case WNAM_ID: {
                uint32_t uiWaterForm = 0;
                apFile->GetChunkData<uint32_t>(uiWaterForm);
                // Stores the chunk's value in the pointer field unchanged.
                SetWaterType(reinterpret_cast<TESWaterForm*>(uiWaterForm));
                break;
            }
            case XATO_ID: {
                // The size compare is signed.
                const int32_t iSize = static_cast<int32_t>(apFile->GetChunkSize());
                if (iSize > 1) {
                    const uint32_t uiSize = static_cast<uint32_t>(iSize);
                    char* pBuffer = static_cast<char*>(ourMalloc(uiSize));
                    apFile->GetChunkData(pBuffer, uiSize);
                    SetActivationString(BSStringT<char>(pBuffer));
                    ourFree(pBuffer);
                }
                break;
            }
            default: break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

// Writes the ACTI record: the name, model, script and destruction components, the sounds, water type and radio station
// as form IDs, and the activation text override (with its terminator) when it is set.
void TESObjectACTI::Save() {
    StartForm();
    TESFullName::Save();
    TESModelTextureSwap::Save(MODL_ID, MODT_ID, MODS_ID);
    TESScriptableForm::Save();
    BGSDestructibleObjectForm::Save();
    if (pSoundLoop) {
        TESForm::AddChunk(SNAM_ID, pSoundLoop->GetFormID());
    }
    if (pSoundActivate) {
        TESForm::AddChunk(VNAM_ID, pSoundActivate->GetFormID());
    }
    if (pRadioTemplate) {
        TESForm::AddChunk(INAM_ID, pRadioTemplate->GetFormID());
    }
    if (pWaterForm) {
        TESForm::AddChunk(WNAM_ID, pWaterForm->GetFormID());
    }
    if (pRadioStation) {
        TESForm::AddChunk(RNAM_ID, pRadioStation->GetFormID());
    }
    if (ActivateTextOverride.GetLength()) {
        BSStringT<char> kLengthCopy = GetActivationString();
        uint32_t uiSize = kLengthCopy.GetLength() + 1;
        BSStringT<char> kTextCopy = GetActivationString();
        TESForm::AddChunkArray(XATO_ID, const_cast<char*>(kTextCopy.String()), uiSize);
    }
    CloseForm();
}

void TESObjectACTI::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm::SaveGame(apSaveGameBuffer);
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_BASE_OBJECT_FULLNAME)) {
        apSaveGameBuffer->BGSSaveGameBuffer::SaveString(static_cast<TESFullName*>(this)->TESFullName::GetFullName(), 0);
    }
}

void TESObjectACTI::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    int8_t aName[268];
    TESForm::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_BASE_OBJECT_FULLNAME)) {
        apLoadGameBuffer->LoadString(static_cast<char*>(static_cast<void*>(aName)));
        static_cast<TESFullName*>(this)->TESFullName::SetFullName(
            static_cast<const char*>(static_cast<const void*>(aName))
        );
    }
}

void TESObjectACTI::InitItem() {
    if (!GetInitialized()) {
        TESScriptableForm::InitItem(this);
        BGSDestructibleObjectForm::InitItem(this);
        TESModelTextureSwap::InitItem(this);
        uint32_t uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSoundLoop));
        if (uiTempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiTempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
            pSoundLoop = dynamic_cast<TESSound*>(pForm);
            if (!pSoundLoop) {
                uint32_t uiFormID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find looping sound (%08X) for activator '%s' (%08X).",
                    uiTempInt,
                    pEditorID,
                    uiFormID
                );
            }
        }
        uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSoundActivate));
        if (uiTempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiTempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
            pSoundActivate = dynamic_cast<TESSound*>(pForm);
            if (!pSoundActivate) {
                uint32_t uiFormID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find activate sound (%08X) for activator '%s' (%08X).",
                    uiTempInt,
                    pEditorID,
                    uiFormID
                );
            }
        }
        uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pRadioTemplate));
        if (uiTempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiTempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
            pRadioTemplate = dynamic_cast<TESSound*>(pForm);
            if (!pRadioTemplate) {
                uint32_t uiFormID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find pRadioTemplate (%08X) for activator '%s' (%08X).",
                    uiTempInt,
                    pEditorID,
                    uiFormID
                );
            }
        }
        uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pWaterForm));
        if (uiTempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiTempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
            pWaterForm = dynamic_cast<TESWaterForm*>(pForm);
            if (!pWaterForm) {
                uint32_t uiFormID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find water form (%08X) for activator '%s' (%08X).",
                    uiTempInt,
                    pEditorID,
                    uiFormID
                );
            }
        }
        uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pRadioStation));
        if (uiTempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiTempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
            pRadioStation = dynamic_cast<BGSTalkingActivator*>(pForm);
            if (!pRadioStation || !pRadioStation->IsRadioStation()) {
                uint32_t uiFormID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find radio station (%08X) for activator '%s' (%08X).",
                    uiTempInt,
                    pEditorID,
                    uiFormID
                );
                pRadioStation = nullptr;
            }
        }
        SetInitialized(true);
    }
}

// The trivial setters and getters are written as direct field copies; the water type goes through the virtual
// GetWaterType.
void TESObjectACTI::Copy(TESForm* apCopy) {
    TESObjectACTI* pActi = dynamic_cast<TESObjectACTI*>(apCopy);
    if (pActi) {
        pSoundLoop = pActi->pSoundLoop;
        pRadioTemplate = pActi->pRadioTemplate;
        pSoundActivate = pActi->pSoundActivate;
        pWaterForm = pActi->GetWaterType();
        pRadioStation = pActi->pRadioStation;
        {
            BSStringT<char> sActivation = pActi->GetActivationString();
            SetActivationString(sActivation);
        }
        CopyAllComponents(apCopy);
    }
}

bool TESObjectACTI::Compare(TESForm* apCompare) {
    TESObjectACTI* pComp = dynamic_cast<TESObjectACTI*>(apCompare);
    if (!pComp) {
        return true;
    }
    TESSound* pLoopingSound = GetLoopingSound();
    if (pLoopingSound != pComp->GetLoopingSound()) {
        return true;
    }
    if (pSoundActivate != pComp->pSoundActivate) {
        return true;
    }
    if (pRadioTemplate != pComp->pRadioTemplate) {
        return true;
    }
    TESWaterForm* pWaterType = GetWaterType();
    if (pWaterType != pComp->GetWaterType()) {
        return true;
    }
    if (pRadioStation != pComp->pRadioStation) {
        return true;
    }
    int32_t iCompare;
    {
        BSStringT<char> sCompActivation = pComp->GetActivationString();
        BSStringT<char> sActivation = GetActivationString();
        iCompare = sActivation.StrCmp(sCompActivation, true);
    }
    return iCompare || CompareAllComponents(pComp);
}

bool TESObjectACTI::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIdFlag,
    TESBoundObject* apObjecttoGet,
    int32_t aicount
) {
    Actor* pActionActor = nullptr;
    bool bStoreSoundLoopOnPlayer = false;
    if (apActionRef && apActionRef->IsActor()) {
        pActionActor = static_cast<Actor*>(apActionRef);
    }

    if (pActionActor) {
        TESIdleManager::SetUsedItem(this);
        TESIdleManager::SetUsedItemActivate(true);
        double dEyeLevel = pActionActor->GetEyeLevel();
        double dWorldEyeLevel = pActionActor->GetLocationOnReference().z + dEyeLevel;
        float fActZ = dWorldEyeLevel - pActionActor->GetReach();
        float fItemZ = apItemActivated->GetLocationOnReference().z;
        int32_t iLevel = apItemActivated->IsNpc() || fActZ < static_cast<double>(fItemZ);
        TESIdleManager::SetUsedItemLevel(iLevel);
        bool bAnim = pActionActor->pCurrentProcess->SetupSpecialIdle(
            pActionActor,
            nullptr,
            AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE,
            true,
            false,
            true
        );
        TESIdleManager::SetUsedItem(nullptr);
        TESIdleManager::SetUsedItemActivate(false);
        TESIdleManager::SetUsedItemLevel(-1);
        if (!bAnim && pWaterForm) {
            return false;
        }
        if (apActionRef == pPlayer && bAnim) {
            pCurrentWater = pWaterForm;
            pPlayer->SetIsDrinkingPlacedWater(true);
            bStoreSoundLoopOnPlayer = true;
            pPlayer->GetAnimation(true)->ClearGroup(AGS_MOVEMENT, 0.0f);
            pPlayer->GetAnimation(false)->ClearGroup(AGS_MOVEMENT, 0.0f);
        }
        if (!apItemActivated->IsAnOwner(pActionActor, true)) {
            TESForm* pOwnerFaction = apItemActivated->GetOwner();
            pActionActor->TrespassAlarm(apItemActivated, pOwnerFaction, -1);
        }
    }
    if (pSoundActivate) {
        NiAVObject* pSoundNode = nullptr;
        if (apItemActivated && apItemActivated->Get3D()) {
            pSoundNode = apItemActivated->Get3D()->GetObjectByName(FixedStrings::QAttachSound());
        }

        const NiPoint3* pSoundPos;
        if (pSoundNode) {
            pSoundPos = &pSoundNode->GetWorldTranslate();
        }
        else {
            pSoundPos = &apItemActivated->GetLocationOnReference();
        }
        float fX = pSoundPos->x;
        float fY = pSoundPos->y;
        float fZ = pSoundPos->z;
        uint32_t uiLoopFlags = 0;
        if (bStoreSoundLoopOnPlayer) {
            uiLoopFlags = TESSound::ConvertLoopFlag(pSoundActivate->GetData().iFlags);
        }
        TESForm* pSoundForm = pSoundActivate;
        uint32_t uiSoundFlags = uiLoopFlags |
            (BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
             BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN);
        const char* pFilename = pSoundActivate->GetFilename();
        BSSoundHandle sndHandle = BSAudio::QInstance().GetSoundHandleByFilename(pFilename, uiSoundFlags, pSoundForm);
        if (bStoreSoundLoopOnPlayer) {
            pPlayer->m_Extra.SetActivateLoopSound(sndHandle);
        }
        sndHandle.SetPosition(fX, fY, fZ);
        sndHandle.SetObjectToFollow(apItemActivated->Get3D());
        sndHandle.Play(false);
    }
    if (pWaterForm) {
        pWaterForm->Activate(apItemActivated, apActionRef, abIdFlag, apObjecttoGet, aicount);
    }
    BGSOpenCloseForm::HandleActivate(apItemActivated, apActionRef, nullptr);
    if (pRadioStation) {
        bool bTurnOn = false;
        BGSOpenCloseForm::OPEN_STATE eState = BGSOpenCloseForm::GetOpenState(apItemActivated);
        if (eState == BGSOpenCloseForm::OBJECT_OPEN || eState == BGSOpenCloseForm::OBJECT_OPENING) {
            bTurnOn = true;
        }
        FalloutRadio::ActivateRadio(apItemActivated, bTurnOn);
    }
    return true;
}

// Sets pRadioStation, the RNAM subrecord.
void TESObjectACTI::SetRadioStation(BGSTalkingActivator* apRadio) {
    pRadioStation = apRadio;
}

void TESObjectACTI::SetLoopingSound(TESSound* apSound) {
    pSoundLoop = apSound;
}

void TESObjectACTI::SetActivateSound(TESSound* apSound) {
    pSoundActivate = apSound;
}

void TESObjectACTI::SetActivationString(BSStringT<char> ActivateTextOverrideInput) {
    ActivateTextOverride.Set(ActivateTextOverrideInput);
}

void TESObjectACTI::SetWaterType(TESWaterForm* pWat) {
    pWaterForm = pWat;
}

TESSound* TESObjectACTI::GetLoopingSound() {
    return pSoundLoop;
}

BGSTalkingActivator* TESObjectACTI::GetRadioStation() {
    return pRadioStation;
}

void TESObjectACTI::SetContinuousBroadcast(bool abVal) {
    uint32_t iFlags = TESForm::GetFormFlags();
    if (abVal) {
        iFlags |= kFormFlags_TalkingActivator_IsContinuousBroadcast;
    }
    else {
        iFlags &= ~kFormFlags_TalkingActivator_IsContinuousBroadcast;
    }
    TESForm::SetFormFlags(iFlags);
    TESForm::AddChange(CHANGE_FORM_FLAGS);
}

bool TESObjectACTI::GetNonPipboyRadio() {
    return (TESForm::GetFormFlags() & kFormFlags_NonPipboyRadio) != 0;
}

BSStringT<char> TESObjectACTI::GetActivationString() {
    return ActivateTextOverride;
}

BGSOpenCloseForm::BGSOpenCloseForm() {}

const char* TESSound::GetFilename() {
    return GetSoundFile();
}
