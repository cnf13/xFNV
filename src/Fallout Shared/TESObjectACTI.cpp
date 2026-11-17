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

TESObjectACTI::TESObjectACTI() {
    cFormType = 21;
    TESObjectACTI::InitializeData();
    SetInitialized(1);
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
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
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
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiSize, 128));
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
                    alignas(BSStringT<char>) uint8_t aTextStorage[sizeof(BSStringT<char>)];
                    BSStringT<char>* pText = ::new (aTextStorage) BSStringT<char>(pBuffer);
                    SetActivationString(*pText);
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
        alignas(BSStringT<char>) uint8_t aLengthStorage[sizeof(BSStringT<char>)];
        BSStringT<char>* pLengthCopy = GetActivationString(reinterpret_cast<BSStringT<char>*>(aLengthStorage));
        uint32_t uiSize = pLengthCopy->GetLength() + 1;
        alignas(BSStringT<char>) uint8_t aTextStorage[sizeof(BSStringT<char>)];
        BSStringT<char>* pTextCopy = GetActivationString(reinterpret_cast<BSStringT<char>*>(aTextStorage));
        TESForm::AddChunkArray(XATO_ID, const_cast<char*>(pTextCopy->String()), uiSize);
        reinterpret_cast<BSStringT<char>*>(aTextStorage)->~BSStringT<char>();
        reinterpret_cast<BSStringT<char>*>(aLengthStorage)->~BSStringT<char>();
    }
    CloseForm();
}

void TESObjectACTI::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    BGSChangeFlags v4;
    const char* a2;
    TESForm::SaveGame(apSaveGameBuffer);
    BGSChangeFlags* v2 = &(v4 = apSaveGameBuffer->GetChangeFlags());
    if (v2->CheckFlags(4u)) {
        a2 = static_cast<TESFullName*>(this)->TESFullName::GetFullName();
        apSaveGameBuffer->BGSSaveGameBuffer::SaveString(a2, 0);
    }
}

void TESObjectACTI::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    BGSChangeFlags result;
    int8_t apModel[268];
    TESForm::LoadGame(apLoadGameBuffer);
    BGSChangeFlags* v2 = &(result = apLoadGameBuffer->GetChangeFlags());
    if (v2->CheckFlags(4u)) {
        apLoadGameBuffer->LoadString(static_cast<char*>(static_cast<void*>(apModel)));
        static_cast<TESFullName*>(this)->TESFullName::SetFullName(
            static_cast<const char*>(static_cast<const void*>(apModel))
        );
    }
}

void TESObjectACTI::InitItem() {
    if (!GetInitialized()) {
        TESScriptableForm::InitItem(this);
        BGSDestructibleObjectForm::InitItem(this);
        TESModelTextureSwap::InitItem(this);
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSoundLoop));
        if (itempint) {
            TESFile* File = GetFile(-1);
            TESForm::AddCompileIndex(itempint, File);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
            pSoundLoop = dynamic_cast<TESSound*>(FormByNumericID);
            if (!pSoundLoop) {
                int32_t CastingType = iFormID;
                const char* v11 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find looping sound (%08X) for activator '%s' (%08X).", itempint, v11,
                    CastingType
                );
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSoundActivate));
        if (itempint) {
            TESFile* v3 = GetFile(-1);
            TESForm::AddCompileIndex(itempint, v3);
            TESForm* v4 = TESForm::GetFormByNumericID(itempint);
            pSoundActivate = dynamic_cast<TESSound*>(v4);
            if (!pSoundActivate) {
                int32_t v17 = iFormID;
                const char* v12 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find activate sound (%08X) for activator '%s' (%08X).", itempint, v12, v17
                );
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pRadioTemplate));
        if (itempint) {
            TESFile* v5 = GetFile(-1);
            TESForm::AddCompileIndex(itempint, v5);
            TESForm* v6 = TESForm::GetFormByNumericID(itempint);
            pRadioTemplate = dynamic_cast<TESSound*>(v6);
            if (!pRadioTemplate) {
                int32_t v18 = iFormID;
                const char* v13 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find pRadioTemplate (%08X) for activator '%s' (%08X).", itempint, v13, v18
                );
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pWaterForm));
        if (itempint) {
            TESFile* v7 = GetFile(-1);
            TESForm::AddCompileIndex(itempint, v7);
            TESForm* v8 = TESForm::GetFormByNumericID(itempint);
            pWaterForm = dynamic_cast<TESWaterForm*>(v8);
            if (!pWaterForm) {
                int32_t v19 = iFormID;
                const char* v14 = GetFormEditorID();
                Warning("MASTERFILE: Could not find water form (%08X) for activator '%s' (%08X).", itempint, v14, v19);
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pRadioStation));
        if (itempint) {
            TESFile* v9 = GetFile(-1);
            TESForm::AddCompileIndex(itempint, v9);
            TESForm* v10 = TESForm::GetFormByNumericID(itempint);
            pRadioStation = dynamic_cast<BGSTalkingActivator*>(v10);
            if (!pRadioStation || !pRadioStation->IsRadioStation()) {
                int32_t v20 = iFormID;
                const char* v15 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find radio station (%08X) for activator '%s' (%08X).", itempint, v15, v20
                );
                pRadioStation = 0;
            }
        }
        SetInitialized(1);
    }
}

// The trivial setters and getters are written as direct field copies; the water type goes through the virtual
// GetWaterType.
void TESObjectACTI::Copy(TESForm* apCopy) {
    TESObjectACTI* acti = dynamic_cast<TESObjectACTI*>(apCopy);
    if (acti) {
        pSoundLoop = acti->pSoundLoop;
        pRadioTemplate = acti->pRadioTemplate;
        pSoundActivate = acti->pSoundActivate;
        pWaterForm = acti->GetWaterType();
        pRadioStation = acti->pRadioStation;
        FNVD_INLINED_LOCAL(BSStringT<char>, sActivation, std::nullopt, std::nullopt);
        acti->GetActivationString(&sActivation);
        SetActivationString(sActivation);
        // SetActivationString releases its argument; natively C++ passes it a copy of this raw-storage local, so the
        // local is destroyed here.
        sActivation.~BSStringT<char>();
        CopyAllComponents(apCopy);
    }
}

bool TESObjectACTI::Compare(TESForm* apCompare) {
    FNVD_INLINED_LOCAL(BSStringT<char>, v11, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSStringT<char>, apString, std::nullopt, std::nullopt);
    TESObjectACTI* pComp = dynamic_cast<TESObjectACTI*>(apCompare);
    if (!pComp) {
        return 1;
    }
    TESSound* LoopingSound = GetLoopingSound();
    if (LoopingSound != pComp->GetLoopingSound()) {
        return 1;
    }
    TESForm* FormTwo = pSoundActivate;
    if (FormTwo != pComp->pSoundActivate) {
        return 1;
    }
    TESSound* RefCount = pRadioTemplate;
    if (RefCount != pComp->pRadioTemplate) {
        return 1;
    }
    TESWaterForm* v6 = GetWaterType();
    if (v6 != pComp->GetWaterType()) {
        return 1;
    }
    BGSTalkingActivator* RadioStation = pRadioStation;
    if (RadioStation != pComp->pRadioStation) {
        return 1;
    }
    BSStringT<char>* arString = pComp->GetActivationString(&v11);
    BSStringT<char>* ActivationString = GetActivationString(&apString);
    int32_t v13 = ActivationString->StrCmp(*arString, 1);
    apString.~BSStringT<char>();
    v11.~BSStringT<char>();
    return v13 || CompareAllComponents(pComp);
}

bool TESObjectACTI::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIdFlag,
    TESBoundObject* apObjecttoGet,
    int32_t aicount
) {
    FNVD_INLINED_LOCAL(BSSoundHandle, sndHandle, std::nullopt, std::nullopt);
    Actor* pActionActor = 0;
    bool bstoreSoundLoopOnPlayer = 0;
    if (apActionRef) {
        if (apActionRef->IsActor()) {
            pActionActor = static_cast<Actor*>(apActionRef);
        }
    }
    if (pActionActor) {
        TESIdleManager::SetUsedItem(this);
        TESIdleManager::SetUsedItemActivate(1);
        double fEyeLevel = pActionActor->GetEyeLevel();
        double fWorldEyeLevel = pActionActor->GetLocationOnReference().z + fEyeLevel;
        float factz = fWorldEyeLevel - pActionActor->GetReach();
        float fitemz = apItemActivated->GetLocationOnReference().z;
        int32_t ilevel = 0;
        ilevel = apItemActivated->IsNpc() || factz < static_cast<double>(fitemz);
        TESIdleManager::SetUsedItemLevel(ilevel);
        BaseProcess* CurrentAIProcess = pActionActor->pCurrentProcess;
        bool banim = CurrentAIProcess->SetupSpecialIdle(pActionActor, 0, AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE, 1, 0, 1);
        TESIdleManager::SetUsedItem(0);
        TESIdleManager::SetUsedItemActivate(0);
        TESIdleManager::SetUsedItemLevel(-1);
        if (!banim && pWaterForm) {
            return 0;
        }
        if (apActionRef == pPlayer && banim) {
            pCurrentWater = pWaterForm;
            pPlayer->SetIsDrinkingPlacedWater(1);
            bstoreSoundLoopOnPlayer = 1;
            Animation* Animation_0 = pPlayer->GetAnimation(static_cast<bool>(1));
            Animation_0->ClearGroup(AGS_MOVEMENT, 0.0f);
            Animation* v8 = pPlayer->GetAnimation(static_cast<bool>(0));
            v8->ClearGroup(AGS_MOVEMENT, 0.0f);
        }
        if (!apItemActivated->IsAnOwner(pActionActor, 1)) {
            TESForm* pOwnerFaction = apItemActivated->GetOwner();
            pActionActor->TrespassAlarm(apItemActivated, pOwnerFaction, -1);
        }
    }
    if (pSoundActivate) {
        const NiPoint3* WorldTranslate;
        NiAVObject* psoundNode = 0;
        if (apItemActivated) {
            if (apItemActivated->Get3D()) {
                NiNode* v20 = apItemActivated->Get3D();
                NiFixedString* v9 = &FixedStrings::QAttachSound();
                psoundNode = v20->GetObjectByName(*v9);
            }
        }
        if (psoundNode) {
            WorldTranslate = &psoundNode->GetWorldTranslate();
        }
        else {
            WorldTranslate = &apItemActivated->GetLocationOnReference();
        }
        float x = WorldTranslate->x;
        float y = WorldTranslate->y;
        float z = WorldTranslate->z;
        uint32_t iloopFlags = 0;
        if (bstoreSoundLoopOnPlayer) {
            TESSound* FormTwo = pSoundActivate;
            iloopFlags = TESSound::ConvertLoopFlag(FormTwo->GetData().iFlags);
        }
        TESForm* blendAmount = pSoundActivate;
        uint32_t pOwnerFactiona = iloopFlags |
            (BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
             BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN);
        TESSound* v12 = pSoundActivate;
        const char* a2 = v12->GetFilename();
        BSAudio* v13 = &BSAudio::QInstance();
        v13->GetSoundHandleByFilename(&sndHandle, a2, pOwnerFactiona, blendAmount);
        if (bstoreSoundLoopOnPlayer) {
            (&pPlayer->m_Extra)->SetActivateLoopSound(sndHandle);
        }
        sndHandle.SetPosition(x, y, z);
        NiNode* v14 = apItemActivated->Get3D();
        sndHandle.SetObjectToFollow(v14);
        sndHandle.Play(0);
    }
    if (pWaterForm) {
        pWaterForm->Activate(apItemActivated, apActionRef, abIdFlag, apObjecttoGet, aicount);
    }
    BGSOpenCloseForm::HandleActivate(apItemActivated, apActionRef, 0);
    if (pRadioStation) {
        bool bturnOn = 0;
        BGSOpenCloseForm::OPEN_STATE estate = BGSOpenCloseForm::GetOpenState(apItemActivated);
        if (estate == BGSOpenCloseForm::OBJECT_OPEN || estate == BGSOpenCloseForm::OBJECT_OPENING) {
            bturnOn = 1;
        }
        FalloutRadio::ActivateRadio(apItemActivated, bturnOn);
    }
    return 1;
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
    uint32_t a2a;
    int32_t a2 = (int32_t)TESForm::GetFormFlags();
    if (abVal) {
        a2a = (uint32_t)(a2 | 1073741824);
    }
    else {
        a2a = a2 & 0xBFFFFFFFu;
    }
    TESForm::SetFormFlags(a2a);
    TESForm::AddChange(1);
}

bool TESObjectACTI::GetNonPipboyRadio() {
    return ((((int32_t)TESForm::GetFormFlags()) & 268435456) != 0);
}

BSStringT<char>* TESObjectACTI::GetActivationString(BSStringT<char>* apResult) {
    ::new (apResult) BSStringT<char>(ActivateTextOverride);
    return apResult;
}

BGSOpenCloseForm::BGSOpenCloseForm() {}

const char* TESSound::GetFilename() {
    return GetSoundFile();
}
