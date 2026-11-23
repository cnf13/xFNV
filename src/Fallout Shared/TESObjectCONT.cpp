#include "Fallout Shared/TESObjectCONT.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSImpactData.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESKey.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectMISC.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

TESObjectCONT::TESObjectCONT() {
    SetFormType(27u);
    TESObjectCONT::InitializeData();
}

TESObjectCONT::~TESObjectCONT() {}

void TESObjectCONT::InitializeData() {
    Data.cContFlags = 0;
    pOpenSound = nullptr;
    pCloseSound = nullptr;
    pLoopingSound = nullptr;
}

// Reads the container's chunks. COED and CNTO share one handler: it reads the chunk code a second time and either gives
// the last entry its extra data (COED) or adds a new entry (CNTO). The editor ID buffer is emptied before the chunk is
// read: GetChunkData writes at most the chunk's bytes, and a zero-size chunk leaves the buffer unwritten, so
// uninitialised data would otherwise be read as the editor ID.
bool TESObjectCONT::Load(TESFile* apFile) {
    // The container entry the last CNTO chunk added; the COED chunk that follows gives it its per-item extra data.
    static ContainerObject* plastloaded;
    if (static_cast<uint8_t>(apFile->GetTESForm()) != CONT_ID) {
        return false;
    }
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
                // the empty string, for a chunk that writes nothing
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case MODL_ID:
            case MODD_ID:
            case MOSD_ID:
            case MODT_ID: TESModel::LoadModelChunk(static_cast<TESModelTextureSwap*>(this), apFile); break;
            case MODS_ID: {
                uint32_t uiSize = apFile->GetChunkSize();
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiSize));
                apFile->GetChunkData(pBuffer, uiSize);
                uint16_t usFormVersion = apFile->GetFormVersion();
                static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(pBuffer, uiSize, usFormVersion);
                break;
            }
            case SCRI_ID: {
                uint32_t uiScriptID = 0;
                apFile->GetChunkData(uiScriptID);
                TESScriptableForm* pScriptableForm = static_cast<TESScriptableForm*>(this);
                pScriptableForm->SetFormScript(reinterpret_cast<Script*>(static_cast<uintptr_t>(uiScriptID)));
                pScriptableForm->InitItem(this);
                break;
            }
            case COED_ID:
            case CNTO_ID: {
                uint32_t uiSubChunk = apFile->GetTESChunk();
                if (uiSubChunk == COED_ID) {
                    if (plastloaded) {
                        plastloaded->pItemExtra = new ContainerItemExtra;
                        plastloaded->pItemExtra->Load(apFile);
                        plastloaded = nullptr;
                    }
                }
                else if (uiSubChunk == CNTO_ID) {
                    CONTAINER_OBJECT_FILE fileEntry;
                    ourMemset(&fileEntry, 0, sizeof(fileEntry));
                    apFile->GetChunkData(&fileEntry, 8u);
                    if (apFile->QEndian()) {
                        fileEntry.Endian();
                    }
                    plastloaded = static_cast<TESContainer*>(this)->AddObject(&fileEntry);
                }
                break;
            }
            case DATA_ID: LoadData(apFile, reinterpret_cast<char*>(&Data), sizeof(Data)); break;
            case DSTD_ID:
            case DEST_ID: BGSDestructibleObjectForm::LoadChunk(this, apFile); break;
            case SNAM_ID: {
                uint32_t uiSoundID = 0;
                apFile->GetChunkData(uiSoundID);
                SetOpenSound(reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID)));
                break;
            }
            case QNAM_ID: {
                uint32_t uiSoundID = 0;
                apFile->GetChunkData(uiSoundID);
                SetCloseSound(reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID)));
                break;
            }
            case RNAM_ID: {
                uint32_t uiSoundID = 0;
                apFile->GetChunkData(uiSoundID);
                SetLoopingSound(reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID)));
                break;
            }
            default: break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESObjectCONT::Save() {
    StartForm();
    TESFullName::Save();
    TESModelTextureSwap::Save(MODL_ID, MODT_ID, MODS_ID);
    TESScriptableForm::Save();
    TESContainer::Save();
    BGSDestructibleObjectForm::Save();
    if (TESForm::QEndianSwapOnSave()) {}
    __SaveData(&Data, sizeof(Data));
    if (TESForm::QEndianSwapOnSave()) {}
    if (this->GetOpenSound()) {
        TESForm::AddChunk(SNAM_ID, this->GetOpenSound()->iFormID);
    }
    if (this->GetCloseSound()) {
        TESForm::AddChunk(QNAM_ID, this->GetCloseSound()->iFormID);
    }
    if (pLoopingSound) {
        TESForm::AddChunk(RNAM_ID, pLoopingSound->iFormID);
    }
    CloseForm();
}

void TESObjectCONT::InitItem() {
    if (!GetInitialized()) {
        TESScriptableForm::InitItem(this);
        TESContainer::InitItem(this);
        BGSDestructibleObjectForm::InitItem(this);
        TESModelTextureSwap::InitItem(this);
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this->GetOpenSound()));
        if (itempint) {
            TESForm::AddCompileIndex(itempint, GetFile(-1));
            TESForm* pForm = TESForm::GetFormByNumericID(itempint);
            pOpenSound = dynamic_cast<TESSound*>(pForm);
            if (!this->GetOpenSound()) {
                int32_t iContainerID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find open sound (%08X) for container '%s' (%08X).",
                    itempint,
                    pEditorID,
                    iContainerID
                );
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this->GetCloseSound()));
        if (itempint) {
            TESForm::AddCompileIndex(itempint, GetFile(-1));
            TESForm* pForm = TESForm::GetFormByNumericID(itempint);
            pCloseSound = dynamic_cast<TESSound*>(pForm);
            if (!this->GetCloseSound()) {
                int32_t iContainerID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find open sound (%08X) for container '%s' (%08X).",
                    itempint,
                    pEditorID,
                    iContainerID
                );
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pLoopingSound));
        if (itempint) {
            TESForm::AddCompileIndex(itempint, GetFile(-1));
            TESForm* pForm = TESForm::GetFormByNumericID(itempint);
            pLoopingSound = dynamic_cast<TESSound*>(pForm);
            if (!pLoopingSound) {
                int32_t iContainerID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find pLoopingSound (%08X) for container '%s' (%08X).",
                    itempint,
                    pEditorID,
                    iContainerID
                );
            }
        }
        SetInitialized(true);
    }
}

void TESObjectCONT::Copy(TESForm* apCopy) {
    TESObjectCONT* pCopy = dynamic_cast<TESObjectCONT*>(apCopy);
    if (pCopy) {
        CopyAllComponents(apCopy);
        Data.cContFlags = pCopy->Data.cContFlags;
        SetOpenSound(pCopy->GetOpenSound());
        SetCloseSound(pCopy->GetCloseSound());
        SetLoopingSound(pCopy->GetLoopingSound());
    }
}

bool TESObjectCONT::Compare(TESForm* apCompare) {
    TESObjectCONT* pComp = dynamic_cast<TESObjectCONT*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    int32_t cContFlags = Data.cContFlags;
    if (cContFlags != pComp->Data.cContFlags) {
        return true;
    }
    TESSound* pSound = GetOpenSound();
    if (pSound != pComp->GetOpenSound()) {
        return true;
    }
    pSound = GetCloseSound();
    if (pSound != pComp->GetCloseSound()) {
        return true;
    }
    pSound = GetLoopingSound();
    return pSound != pComp->GetLoopingSound();
}

bool TESObjectCONT::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIdFlag,
    TESBoundObject* apObjecttoGet,
    int32_t aicount
) {
    int32_t fdoPunishment;
    Actor* pActionActor = dynamic_cast<Actor*>(apActionRef);
    TESForm* pownership = nullptr;
    PACKAGE_OBJECT_TYPE eUseItem = POTYPE_NONE;
    if (!pActionActor) {
        return false;
    }
    bool bignorelock = false;
    if (apActionRef != pPlayer) {
        bignorelock = apItemActivated->IsAnOwner(pActionActor, true);
    }
    LockData* pLock = apItemActivated->GetLock();
    if (pLock && pLock->IsLocked() && !bignorelock) {
        pActionActor->HasObjects(pLockpick, POTYPE_NONE, 1, 0, eUseItem);
        if (pLock->pKey && pActionActor->HasObjects(pLock->pKey, POTYPE_NONE, 1, 0, eUseItem)) {
            if (!pstaticcontainer && pActionActor == pPlayer) {
                char cMessage[256];
                pstaticcontainer = static_cast<MobileObject*>(apItemActivated);
                ExtraLock* pextralock = apItemActivated->GetExtraLock();
                if (pextralock) {
                    pextralock->UnLockDoor();
                    apItemActivated->AddLockChange();
                }
                BSAudio::QInstance().GetSoundHandleByName("UILockpickingUnlock", 289u).Play(false);
                BSsprintf(cMessage, 255u, sOpenWithKey.String(), pLock->pKey->GetFullName());
                Interface::ShowMessage(
                    cMessage,
                    0,
                    "Interface\\Icons\\Message Icons\\glow_message_key.dds",
                    nullptr,
                    2.0f,
                    false
                );
                pstaticcontainer->Activate(pPlayer, false, nullptr, 1);
                pstaticcontainer = nullptr;
                return true;
            }
        }
        else if (!pstaticcontainer) {
            if (pActionActor == pPlayer) {
                ExtraLock* pextralock = apItemActivated->GetExtraLock();
                *reinterpret_cast<float*>(&fdoPunishment) = 0.0f;
                BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::IGNORE_BROKEN_LOCK, pPlayer, &fdoPunishment);
                if (pextralock &&
                    (pextralock->pLock->GetLockLevel(apItemActivated) == 5 || pextralock->pLock->IsBroken())) {
                    Interface::ShowMessage(
                        sImpossibleLock.String(),
                        0,
                        "Interface\\Icons\\Message Icons\\glow_message_padlock.dds",
                        nullptr,
                        2.0f,
                        false
                    );
                }
                else {
                    _ShowLockPickMenu(apItemActivated, 0, false);
                }
            }
            else {
                Animation* panim = pActionActor->GetAnimation();
                if (panim) {
                    panim->SpecialIdleFree(true, false);
                }
                TESIdleManager::SetUsedItem(pLockpick);
                TESIdleManager::SetUsedItemActivate(true);
                TESIdleManager::SetUsedItemLevel(1);
                pActionActor->pCurrentProcess
                    ->SetupSpecialIdle(pActionActor, nullptr, AnimIdle::PLAY_TYPE_AUTO, true, false, true);
                TESIdleManager::SetUsedItem(nullptr);
                TESIdleManager::SetUsedItemActivate(false);
                TESIdleManager::SetUsedItemLevel(-1);
                ExtraLock* pextralock = apItemActivated->GetExtraLock();
                if (pextralock) {
                    pextralock->UnLockDoor();
                    apItemActivated->AddLockChange();
                }
            }
        }
        return false;
    }
    if (pActionActor != pPlayer && GetOpenSound() && apItemActivated && apItemActivated->Get3D()) {
        uint32_t uiSoundID = GetOpenSound()->iFormID;
        BSSoundHandle soundHand = BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, 258u);
        NiPoint3 kLocation = apItemActivated->GetLocationOnReference();
        soundHand.SetPosition(kLocation.x, kLocation.y, kLocation.z);
        soundHand.Play(false);
    }

    if (pActionActor == pPlayer) {
        if (BGSOpenCloseForm::GetOpenState(apItemActivated) == BGSOpenCloseForm::OBJECT_OPEN) {
            HandleOpenFinish(apItemActivated, pActionActor);
        }
        else {
            BGSOpenCloseForm::HandleActivate(apItemActivated, pActionActor, nullptr);
        }
        return true;
    }

    if (apObjecttoGet) {
        apItemActivated
            ->RemoveItem(apObjecttoGet, nullptr, aicount, false, false, pActionActor, nullptr, nullptr, true, false);
    }
    if (pActionActor->IsNpc() && !apItemActivated->IsAnOwner(pActionActor, true) && apItemActivated->GetOwner()) {
        pActionActor->StealAlarm(apItemActivated, apObjecttoGet, aicount, 1, pownership);
    }

    return true;
}

// Once the player's open animation finishes, opens the container menu and runs the activation.
void TESObjectCONT::HandleOpenFinish(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef) {
    if (apItemActivated && apActionRef == pPlayer) {
        Interface::QueueMenuCreate(
            Interface::OPEN_CONTAINER,
            apItemActivated,
            nullptr,
            nullptr,
            Interface::CMM_STANDARD,
            false
        );
        BGSOpenCloseForm::HandleActivate(apItemActivated, apActionRef, nullptr);
    }
}

TESSound* TESObjectCONT::GetLoopingSound() {
    return pLoopingSound;
}

void TESObjectCONT::SetOpenSound(TESSound* apSound) {
    pOpenSound = apSound;
}

void TESObjectCONT::SetCloseSound(TESSound* apSound) {
    pCloseSound = apSound;
}

void TESObjectCONT::SetLoopingSound(TESSound* apSound) {
    pLoopingSound = apSound;
}

bool TESObjectCONT::CanActivate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abAllowLockpicking) {
    if (!apActionRef || !apActionRef->IsActor()) {
        return false;
    }
    LockData* pLock = apItemActivated->GetLock();
    if (!pLock || !pLock->IsLocked()) {
        return true;
    }
    if (apActionRef != pPlayer && apItemActivated->IsAnOwner(static_cast<Actor*>(apActionRef), true)) {
        return true;
    }
    PACKAGE_OBJECT_TYPE eMatchId = POTYPE_NONE;
    if (pLock->pKey && static_cast<Actor*>(apActionRef)->Actor::HasObjects(pLock->pKey, POTYPE_NONE, 1, 0, eMatchId)) {
        return true;
    }
    return abAllowLockpicking &&
        static_cast<Actor*>(apActionRef)->Actor::HasObjects(pLockpick, POTYPE_NONE, 1, 0, eMatchId);
}
