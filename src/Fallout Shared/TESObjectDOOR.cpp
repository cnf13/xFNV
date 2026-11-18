#include "Fallout Shared/TESObjectDOOR.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/DoorLock.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESKey.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectMISC.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/TeleportData.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BSPlayerDistanceCheckController.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "SDK/MSVC/rtti.h"

SettingT<GameSettingCollection> gs_fRandomDoorDistance("fRandomDoorDistance", 150.0f);
SettingT<GameSettingCollection> gs_fAutoDoorActivateDistance("fAutoDoorActivateDistance", 300.0f);
SettingT<GameSettingCollection>
    gs_sRandomDoorTeleportFailureMessage("sRandomDoorTeleportFailureMessage", "This door leads nowhere.");

TESObjectDOOR::TESObjectDOOR() {
    SetFormType(DOOR_ID);
    TESObjectDOOR::InitializeData();
}

TESObjectDOOR::~TESObjectDOOR() {
    TESObjectDOOR::ClearData();
}

void TESObjectDOOR::InitializeData() {
    pOpenSound = nullptr;
    pCloseSound = nullptr;
    pLoopSound = nullptr;
    cFlags = (uint8_t)0;
}

void TESObjectDOOR::ClearData() {
    ClearRandomTeleportList();
}

// Reads the door's chunks: the name, model and script components, the destructible data and the door's own sounds, flag
// byte and teleport list. There is no handler for a DATA chunk here.
bool TESObjectDOOR::Load(TESFile* apFile) {
    if ((uint8_t)apFile->GetTESForm() != 28) {
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
                // A 16-byte aligned buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
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
                // A 16-byte aligned buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiSize, 128));
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
            case DSTD_ID:
            case DEST_ID: BGSDestructibleObjectForm::LoadChunk(this, apFile); break;
            case FNAM_ID: apFile->GetChunkData(&cFlags, 1u); break;
            case SNAM_ID: {
                uint32_t uiSoundID = 0;
                apFile->GetChunkData(uiSoundID);
                TESForm::AddCompileIndex(uiSoundID, apFile);
                pOpenSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(uiSoundID));
                break;
            }
            case BNAM_ID: {
                uint32_t uiSoundID = 0;
                apFile->GetChunkData(uiSoundID);
                TESForm::AddCompileIndex(uiSoundID, apFile);
                pLoopSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(uiSoundID));
                break;
            }
            case ANAM_ID: {
                uint32_t uiSoundID = 0;
                apFile->GetChunkData(uiSoundID);
                TESForm::AddCompileIndex(uiSoundID, apFile);
                pCloseSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(uiSoundID));
                break;
            }
            case TNAM_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData(uiFormID);
                // the form ID stands in for the form until the door's teleport list is resolved
                TESForm* pForm = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiFormID));
                RandomTeleports.AddHead(pForm);
                break;
            }
            default: break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESObjectDOOR::Save() {
    StartForm();
    TESFullName::Save();
    TESModelTextureSwap::Save(MODL_ID, MODT_ID, MODS_ID);
    TESScriptableForm::Save();
    BGSDestructibleObjectForm::Save();
    if (pOpenSound) {
        TESForm::AddChunk(SNAM_ID, pOpenSound->iFormID);
    }
    if (pCloseSound) {
        TESForm::AddChunk(ANAM_ID, pCloseSound->iFormID);
    }
    if (pLoopSound) {
        TESForm::AddChunk(BNAM_ID, pLoopSound->iFormID);
    }
    TESForm::AddChunk(FNAM_ID, this->TESObjectDOOR::cFlags);
    for (BSSimpleList<TESForm*>* plist = &RandomTeleports; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        TESForm::AddChunk(TNAM_ID, plist->m_item->iFormID);
    }
    CloseForm();
}

void TESObjectDOOR::InitItem() {
    if (!GetInitialized()) {
        TESScriptableForm::InitItem(this);
        BGSDestructibleObjectForm::InitItem(this);
        TESModelTextureSwap::InitItem(this);
        BSSimpleList<TESForm*>* plist = &RandomTeleports;
        BSSimpleList<TESForm*>* plastlist = nullptr;
        while (plist && !plist->IsEmpty()) {
            uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(plist->m_item));
            TESFile* File = GetFile(-1);
            TESForm::AddCompileIndex(tempint, File);
            TESForm* pform = TESForm::GetFormByNumericID(tempint);
            if (pform) {
                plist->SetItem(pform);
                plastlist = plist;
                plist = plist->m_pkNext;
            }
            else {
                int32_t CastingType = iFormID;
                const char* v2 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find RandomTeleport (%08X) for Door '%s' (%08X).", tempint, v2, CastingType
                );
                if (plastlist) {
                    // The slot holds a raw form ID, which is passed to Remove as the item pointer.
                    TESForm* pRemove = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(tempint));
                    plastlist->Remove(pRemove);
                    plist = plastlist->m_pkNext;
                }
                else {
                    plist->RemoveHead();
                }
            }
        }
        SetInitialized(true);
    }
}

void TESObjectDOOR::Copy(TESForm* apCopy) {
    TESObjectDOOR* pdoor = dynamic_cast<TESObjectDOOR*>(apCopy);
    if (pdoor) {
        ClearRandomTeleportList();
        BSSimpleList<TESForm*>* pcopylist = &pdoor->RandomTeleports;
        BSSimpleList<TESForm*>* plist = &RandomTeleports;
        while (pcopylist && !pcopylist->IsEmpty()) {
            plist->AddTail(pcopylist->m_item);
            if (plist->m_pkNext) {
                plist = plist->m_pkNext;
            }
            pcopylist = pcopylist->m_pkNext;
        }
        this->TESObjectDOOR::cFlags = pdoor->TESObjectDOOR::cFlags;
        pOpenSound = pdoor->pOpenSound;
        pCloseSound = pdoor->pCloseSound;
        pLoopSound = pdoor->pLoopSound;
        CopyAllComponents(apCopy);
    }
}

bool TESObjectDOOR::Compare(TESForm* apCompare) {
    TESObjectDOOR* pcomp = dynamic_cast<TESObjectDOOR*>(apCompare);
    if (!pcomp) {
        return true;
    }
    if (CompareAllComponents(pcomp)) {
        return true;
    }
    if (pOpenSound != pcomp->pOpenSound) {
        return true;
    }
    if (pCloseSound != pcomp->pCloseSound) {
        return true;
    }
    if (pLoopSound != pcomp->pLoopSound) {
        return true;
    }
    if (this->TESObjectDOOR::cFlags != pcomp->TESObjectDOOR::cFlags) {
        return true;
    }
    BSSimpleList<TESForm*>* pcomplist = &pcomp->RandomTeleports;
    int32_t v3 = RandomTeleports.ItemsInList();
    if (v3 != pcomplist->ItemsInList()) {
        return true;
    }
    while (pcomplist && !pcomplist->IsEmpty()) {
        if (!RandomTeleports.IsInList(pcomplist->m_item)) {
            return true;
        }
        pcomplist = pcomplist->m_pkNext;
    }
    return false;
}

bool TESObjectDOOR::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIdFlag,
    TESBoundObject* apObjecttoGet,
    int32_t aicount
) {
    BaseProcess* pprocess_3;
    BaseProcess* pprocess;
    XFNV_INLINED_LOCAL(BSSoundHandle, th_s, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(BSSoundHandle, apHandle, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    int32_t fdoPunishment_2;
    int32_t fdoPunishment;
    bool blockpick;
    bool bignorelock;
    PACKAGE_OBJECT_TYPE eUseItem;
    if (!apItemActivated || apItemActivated->GetDestroyed()) {
        return false;
    }
    DoorTeleportData* pTel = apItemActivated->GetTeleport();
    LockData* pLock = apItemActivated->GetLock();
    if (!pTel && IsRandomDoor()) {
        TESObjectREFR* pteldoor = FindRandomTeleportTarget(apItemActivated, apActionRef);
        if (!pteldoor) {
            const char* v7 = gs_sRandomDoorTeleportFailureMessage.String();
            Interface::ShowMessage(
                v7, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", nullptr, 2.0f, false
            );
            return false;
        }
        TESObjectDOOR::LinkRandomTeleportDoors(apItemActivated, pteldoor);
        pTel = apItemActivated->GetTeleport();
        if (!pTel) {
            return false;
        }
    }
    Actor* pActionActor = dynamic_cast<Actor*>(apActionRef);
    if (pActionActor) {
        pprocess = pActionActor->pCurrentProcess;
    }
    else {
        pprocess = nullptr;
    }
    if (!pActionActor) {
        goto LABEL_60;
    }
    pprocess_3 = pActionActor->pCurrentProcess;
    if (pprocess_3->GetCurrentFurniture() && pActionActor->GetSitSleepState()) {
        return false;
    }
    if (pActionActor == pPlayer) {
        pPlayer->iCombatPersue = 0;
    }
    bignorelock = TESObjectDOOR::CanActorIgnoreLock(apItemActivated, pActionActor, true, true);
    if (!pLock || (!pLock->IsLocked() || bignorelock)) {
        goto LABEL_60;
    }
    blockpick = false;
    if (pActionActor->IsNpc()) {
        blockpick = pActionActor->HasObjects(pLockpick, POTYPE_NONE, 1, 0, eUseItem);
    }
    if (pLock->pKey) {
        char msg[260];
        if (!pstaticdoor && !pActionActor->HasObjects(pLock->pKey, POTYPE_NONE, 1, 0, eUseItem)) {
            Animation* panim;
            ExtraLock* pextralock_2;
            if (pActionActor == pPlayer) {
                ExtraLock* pextralock = apItemActivated->GetExtraLock();
                *reinterpret_cast<float*>(&fdoPunishment) = 0.0f;
                BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::IGNORE_BROKEN_LOCK, pPlayer, &fdoPunishment);
                if (pextralock &&
                    (pextralock->pLock->GetLockLevel(apItemActivated) == 5 || pextralock->pLock->IsBroken())) {
                    const char* v8 = sImpossibleLock.String();
                    Interface::ShowMessage(
                        v8, 0, "Interface\\Icons\\Message Icons\\glow_message_padlock.dds", nullptr, 2.0f, false
                    );
                    Interface::PlayMenuSound(18);
                }
                else {
                    _ShowLockPickMenu(apItemActivated, 0, false);
                }
                return false;
            }
            if (!blockpick) {
                return false;
            }
            panim = pActionActor->GetAnimation();
            if (panim) {
                panim->SpecialIdleFree(true, false);
            }
            TESIdleManager::SetUsedItem(pLockpick);
            TESIdleManager::SetUsedItemActivate(true);
            TESIdleManager::SetUsedItemLevel(1);
            pprocess->SetupSpecialIdle(pActionActor, nullptr, AnimIdle::PLAY_TYPE_AUTO, true, false, true);
            TESIdleManager::SetUsedItem(nullptr);
            TESIdleManager::SetUsedItemActivate(false);
            TESIdleManager::SetUsedItemLevel(-1);
            pextralock_2 = apItemActivated->GetExtraLock();
            if (pextralock_2) {
                pextralock_2->UnLockDoor();
                apItemActivated->AddLockChange();
            }
        LABEL_60:;
            if (pTel && pActionActor) {
                if (!pPlayer->IsSleepingorResting() &&
                    (!pprocess->eLevel && pprocess->GetFadeState() != BaseProcess::FADE_TELEPORT_OUT)) {
                    if ((pOpenSound) && (apItemActivated->Get3D())) {
                        BSSoundHandle SoundHandle;
                        if (pActionActor == pPlayer) {
                            TESForm* FormTwo = pOpenSound;
                            uint32_t arPos_4 = FormTwo->iFormID;
                            BSAudio* v12 = &BSAudio::QInstance();
                            BSSoundHandle* arHandle = v12->GetSoundHandleByNumericID(
                                &apHandle, arPos_4,
                                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                    BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_UNKBIT31
                            );
                            SoundHandle.operator=(*arHandle);
                        }
                        else {
                            TESForm* v13 = pOpenSound;
                            uint32_t arPos_4a = v13->iFormID;
                            BSAudio* v14 = &BSAudio::QInstance();
                            BSSoundHandle* SoundHandleByNumericID =
                                v14->GetSoundHandleByNumericID(&th_s, arPos_4a, 258u);
                            SoundHandle.operator=(*SoundHandleByNumericID);
                        }
                        if (pActionActor != pPlayer) {
                            NiPoint3 v33 = apItemActivated->GetLocationOnReference();
                            SoundHandle.SetPosition(v33.x, v33.y, v33.z);
                        }
                        SoundHandle.Play(false);
                        if (pActionActor == pPlayer) {
                            BSAudio* v15 = &BSAudio::QInstance();
                            if (!v15->bMultiThread) {
                                for (int32_t i = 0; i < 4; ++i) {
                                    BSAudio* v16 = &BSAudio::QInstance();
                                    v16->Update(false);
                                }
                            }
                        }
                    }

                    static_cast<HighProcess*>(pprocess)->HighProcess::FadeOut(pActionActor, apItemActivated, true);
                    return pActionActor == pPlayer;
                }
                TESObjectCELL* ptelcell = pTel->GetTeleportCell();
                TESWorldSpace* ptelworld = pTel->GetTeleportWorldSpace();
                BaseProcess* CurrentAIProcess = pActionActor->pCurrentProcess;
                CurrentAIProcess->SetDoor(apItemActivated);
                if (pActionActor == pPlayer) {
                    if (!pPlayer->GetInterior()) {
                        pPlayer->pLastExtDoorActivated = apItemActivated;
                    }
                    pPlayer->btransporting = true;
                    pPlayer->HaveFollowersRecheckPackages();
                    HandlePlayerTeleport(apItemActivated);
                    pPlayer->btransporting = false;
                }
                else {
                    apActionRef->SetLocationOnReference(*&pTel->position);
                    if (ptelcell && TES::pInstance->IsCellLoaded(ptelcell, false)) {
                        float* FollowUpInfoList = &(&pTel->rotation)->x;
                        apActionRef->SetAngleOnReferenceZ(FollowUpInfoList[2]);
                        apActionRef->SetAngleOnReferenceX(0.0f);
                    }
                    else {
                        apActionRef->SetAngleOnReferenceX(3.4028235e+38f);
                    }
                    Actor* pac = dynamic_cast<Actor*>(apActionRef);
                    if (pac) {
                        pac->SetForceUpdateQuest(false);
                    }
                    TESObjectREFR::MoveRefToNewSpace(apActionRef, ptelcell, ptelworld);
                }
            }
            else {
                ExtraLock* pextralock_6 = apItemActivated->GetExtraLock();
                if (pextralock_6) {
                    pextralock_6->UnLockDoor();
                    apItemActivated->AddLockChange();
                }
                if (this) {
                    BGSOpenCloseForm::HandleActivate(apItemActivated, apActionRef, this);
                }
                else {
                    BGSOpenCloseForm::HandleActivate(apItemActivated, apActionRef, nullptr);
                }
            }
            return true;
        }
        if (pActionActor != pPlayer || pstaticdoor) {
            goto LABEL_60;
        }
        pstaticdoor = static_cast<MobileObject*>(apItemActivated);
        ExtraLock* pextralock_3 = apItemActivated->GetExtraLock();
        if (pextralock_3) {
            pextralock_3->UnLockDoor();
            apItemActivated->AddLockChange();
        }
        BSAudio* v9 = &BSAudio::QInstance();
        BSSoundHandle* SoundHandleByName = v9->GetSoundHandleByName(&a1, "UILockpickingUnlock", 289u);
        SoundHandleByName->Play(false);
        BSsprintf(msg, 255, sOpenWithKey.String(), pLock->pKey->GetFullName());
        Interface::ShowMessage(msg, 0, "Interface\\Icons\\Message Icons\\glow_message_key.dds", nullptr, 2.0f, false);
        pstaticdoor->Activate(pPlayer, false, nullptr, 1);
        pstaticdoor = nullptr;
        return true;
    }

    if (pActionActor != pPlayer) {
        if (!blockpick) {
            return false;
        }
        Animation* panim_2 = pActionActor->GetAnimation();
        if (panim_2) {
            panim_2->SpecialIdleFree(true, false);
        }
        TESIdleManager::SetUsedItem(pLockpick);
        TESIdleManager::SetUsedItemActivate(true);
        TESIdleManager::SetUsedItemLevel(1);
        pprocess->SetupSpecialIdle(pActionActor, nullptr, AnimIdle::PLAY_TYPE_AUTO, true, false, true);
        TESIdleManager::SetUsedItem(nullptr);
        TESIdleManager::SetUsedItemActivate(false);
        TESIdleManager::SetUsedItemLevel(-1);
        ExtraLock* pextralock_5 = apItemActivated->GetExtraLock();
        if (pextralock_5) {
            pextralock_5->UnLockDoor();
            apItemActivated->AddLockChange();
        }
        goto LABEL_60;
    }
    ExtraLock* pextralock_4 = apItemActivated->GetExtraLock();
    *reinterpret_cast<float*>(&fdoPunishment_2) = 0.0f;
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::IGNORE_BROKEN_LOCK, pPlayer, &fdoPunishment_2);
    if (pextralock_4 && (pextralock_4->pLock->GetLockLevel(apItemActivated) == 5 || pextralock_4->pLock->IsBroken())) {
        const char* v10 = sImpossibleLock.String();
        Interface::ShowMessage(
            v10, 0, "Interface\\Icons\\Message Icons\\glow_message_padlock.dds", nullptr, 2.0f, false
        );
        Interface::PlayMenuSound(18);
    }
    else {
        _ShowLockPickMenu(apItemActivated, 0, false);
    }
    return false;
}

NiAVObject* TESObjectDOOR::Clone3D(TESObjectREFR* apRequester) {
    NiAVObject* p3D = TESBoundObject::Clone3D(apRequester);
    if (pOpenSound) {
        BSAudio* v2 = &BSAudio::QInstance();
        if (!v2->bMultiThread) {
            TESSound* a2 = pOpenSound;
            TESSound* FormTwo = pOpenSound;
            const char* Filename = FormTwo->GetFilename();
            BSAudio* v4 = &BSAudio::QInstance();
            v4->PrecacheSound(Filename, 289u, a2);
        }
    }
    if (p3D) {
        NiTimeController* pcon = p3D->GetController(&BSPlayerDistanceCheckController::ms_RTTI);
        if (!IsAutomatic() || (apRequester && apRequester->GetDestroyed())) {
            if (pcon) {
                p3D->RemoveController(pcon);
            }
        }
        else {
            if (!pcon) {
                BSPlayerDistanceCheckController* pcontrol;
                BSPlayerDistanceCheckController* v14 = static_cast<BSPlayerDistanceCheckController*>(
                    NiMemObject::operator new(sizeof(BSPlayerDistanceCheckController))
                );
                if (v14) {
                    pcontrol = ::new (v14) BSPlayerDistanceCheckController;
                }
                else {
                    pcontrol = nullptr;
                }
                pcontrol->SetCheckDistance(gs_fAutoDoorActivateDistance.Float());
                pcontrol->SetCallback(&TESObjectDOOR::AutoDoorCallback);
                pcontrol->SetTarget(p3D);
            }
            const NiFixedString* v5 = &BSXFlags::Tag();
            BSXFlags* pextra = static_cast<BSXFlags*>(p3D->GetExtraData(*v5));
            if (!pextra) {
                BSXFlags* pextra_2;
                BSXFlags* apMemory = static_cast<BSXFlags*>(NiMemObject::operator new(sizeof(BSXFlags)));
                if (apMemory) {
                    pextra_2 = ::new (apMemory) BSXFlags;
                }
                else {
                    pextra_2 = nullptr;
                }
                pextra = pextra_2;
                const NiFixedString* v6 = &BSXFlags::Tag();
                p3D->AddExtraData(*v6, pextra_2);
            }
            pextra->SetFlags(1u, true);
        }
    }
    return p3D;
}

bool TESObjectDOOR::IsSlidingDoor() {
    return ((cFlags & 16) != 0);
}

TESSound* TESObjectDOOR::GetLoopSound() {
    return pLoopSound;
}

bool TESObjectDOOR::IsRandomDoor() {
    return (bool)((uint8_t)(!(&RandomTeleports)->IsEmpty()));
}

void TESObjectDOOR::ClearRandomTeleportList() {
    (&RandomTeleports)->RemoveAll();
}

bool TESObjectDOOR::IsHidden() {
    return ((cFlags & 4) != 0);
}

bool TESObjectDOOR::IsMinimalUse() {
    return ((cFlags & 8) != 0);
}

void TESObjectDOOR::SetMinimalUse(bool abVal) {
    if (abVal) {
        cFlags |= 8u;
    }
    else {
        cFlags &= (~8u);
    }
}

bool TESObjectDOOR::IsAutomatic() {
    return ((cFlags & 2) != 0);
}

void TESObjectDOOR::HandlePlayerTeleport(TESObjectREFR* apActivatedDoor) {
    if ((apActivatedDoor) && (apActivatedDoor->GetObjectReference() == this)) {
        DoorTeleportData* pTel = apActivatedDoor->GetTeleport();
        if (pTel) {
            bool Interior;
            PLAYER_TARGET_LOC* ploc;
            TESObjectCELL* ptelcell = pTel->GetTeleportCell();
            TESWorldSpace* ptelworld = pTel->GetTeleportWorldSpace();
            PLAYER_TARGET_LOC* apMemory = static_cast<PLAYER_TARGET_LOC*>(ourMalloc(sizeof(PLAYER_TARGET_LOC)));
            if (apMemory) {
                ploc = ::new (apMemory) PLAYER_TARGET_LOC;
            }
            else {
                ploc = nullptr;
            }
            ploc->Location = *&pTel->position;
            ploc->Angle = *&pTel->rotation;
            ploc->bResetWeather = false;
            ploc->pfnArrivalFunc = &TESObjectDOOR::DoorTeleportPlayerArrivalCallback;
            // The arrival data is an integer field; the callback takes the door reference back out of it.
            ploc->iArrivalFuncData = reinterpret_cast<uintptr_t>(apActivatedDoor);
            if (ptelcell && ptelcell->GetInterior()) {
                ploc->pInterior = ptelcell;
            }
            else {
                ploc->pWorld = ptelworld;
            }
            pPlayer->RequestPositionPlayer(ploc);
            TESObjectCELL* poriginalParentCell = pPlayer->GetParentCell();
            if (poriginalParentCell) {
                Interior = poriginalParentCell->GetInterior();
            }
            else {
                Interior = false;
            }
            bStaticWasInInterior = Interior;
        }
    }
}

bool TESObjectDOOR::RandomlyTeleportsTo(TESForm* apSpace) {
    char v4 = 0;
    if ((apSpace) && (TESForm::IsInteriorCellOrWorldspace(apSpace))) {
        return RandomTeleports.IsInList(apSpace);
    }

    return v4;
}

TESObjectREFR* TESObjectDOOR::FindRandomTeleportTarget(TESObjectREFR* apStartDoor, TESObjectREFR* apActionRef) {
    TESObjectREFR* pbackup;
    TESObjectREFR* pretdoor = nullptr;
    pbackup = nullptr;
    if (apStartDoor) {
        TESForm* pstartspace = (TESForm*)apStartDoor->GetWorldSpace();
        if ((!pstartspace) && (apStartDoor->GetParentCell()) && (apStartDoor->GetParentCell()->GetInterior())) {
            pstartspace = (TESForm*)apStartDoor->GetParentCell();
        }

        if (pstartspace) {
            using checked_t = NiTPointerMap<int32_t, bool>;
            checked_t checked(37u);
            uint32_t ifailures = 0;
            uint32_t icount = RandomTeleports.ItemsInList();
            uint32_t itry = -1;
            uint32_t ibackuptry = -1;
            uint32_t ilasttry = pPlayer->GetLastSpaceForDoor(this);
            while (ifailures < icount && !pretdoor) {
                ++ifailures;
                itry = Random() % icount;
                for (bool bchecked = false; checked.GetAt(itry, bchecked) && bchecked; bchecked = false) {
                    itry = Random() % icount;
                }
                TESForm* ptryspace = nullptr;
                uint32_t iindex = 0;
                for (BSSimpleList<TESForm*>* plist = &RandomTeleports; plist && !plist->IsEmpty();
                     plist = plist->m_pkNext) {
                    if (iindex == itry) {
                        ptryspace = plist->m_item;
                        break;
                    }
                    ++iindex;
                }
                pretdoor = TESObjectCELL::PickRandomTeleportDoorInSpace(ptryspace, pstartspace, &pbackup);
                if ((pbackup) && (ibackuptry == -1)) {
                    ibackuptry = itry;
                }

                if ((pretdoor) && (itry == ilasttry)) {
                    pbackup = pretdoor;
                    ibackuptry = itry;
                    pretdoor = nullptr;
                }

                checked.SetAt(iindex, true);
            }
            if (itry != -1) {
                pPlayer->SetLastSpaceForDoor(this, itry);
            }
        }
    }
    return pretdoor;
}

bool TESObjectDOOR::CanActorIgnoreLock(
    DoorLock* apLock,
    Actor* apActor,
    TESObjectREFR* apDoorRef,
    bool abActivate,
    bool abMovement
) {
    bool bResult = false;
    if (apActor && apLock) {
        if ((apActor->IsFollowing() && apActor->GetCurrentAIProcess()->GetTarget() == pPlayer &&
             apDoorRef->GetTeleport()) ||
            apActor->IsTresPassing()) {
            bResult = true;
        }
        else {
            bResult = apLock->CanActorIgnoreDoorLock(apActor, abActivate, abMovement);
        }
    }
    return bResult;
}

bool TESObjectDOOR::CanActorIgnoreLock(TESObjectREFR* apDoorRef, Actor* apActor, bool abActivate, bool abMovement) {
    DoorLock apMemory(apDoorRef);
    bool CanActorIgnoreLock_0 =
        TESObjectDOOR::CanActorIgnoreLock(&apMemory, apActor, apDoorRef, abActivate, abMovement);
    return CanActorIgnoreLock_0;
}

void TESObjectDOOR::AutoDoorCallback(NiAVObject* apDoor3D) {
    TESBoundObject* pBaseForm;
    BaseProcess* pBaseProc;
    HighProcess* pHighProc;
    TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(apDoor3D);
    if (pRef) {
        pBaseForm = pRef->TESObjectREFR::GetObjectReference();
        if ((((int32_t)pBaseForm->TESForm::GetFormType()) == 28) && (!pRef->TESForm::GetDestroyed())) {
            pBaseProc = pPlayer->MobileObject::GetCurrentAIProcess();
            pHighProc = dynamic_cast<HighProcess*>(pBaseProc);
            if ((pHighProc) && (((int32_t)pHighProc->GetFadeState()) != 4)) {
                pPlayer->SetForceActivateRef(pRef);
            }
        }
    }
}

void TESObjectDOOR::DoorTeleportPlayerArrivalCallback(uintptr_t aiArrivalData) {
    XFNV_INLINED_LOCAL(BSSoundHandle, SoundHandle, std::nullopt, std::nullopt);
    // The arrival data is the integer field PLAYER_TARGET_LOC::iArrivalFuncData; Activate stored the door reference in
    // it.
    TESObjectREFR* pdoorref = reinterpret_cast<TESObjectREFR*>(aiArrivalData);
    if (aiArrivalData) {
        TESObjectDOOR::PlayerPullFollower();
        if ((pPlayer->GetParentCell()) && (pPlayer->GetParentCell()->GetInterior())) {
            pPlayer->GetParentCell()->ProcessActors();
        }

        TESObjectDOOR* pdoor = static_cast<TESObjectDOOR*>(pdoorref->GetObjectReference());
        if ((pdoor->pCloseSound) && (pdoorref->GetTeleport()->pLinkedDoor)) {
            TESObjectREFR* LinkedDoor = pdoorref->GetTeleport()->pLinkedDoor;
            if (LinkedDoor->Get3D()) {
                uint32_t CastingType = pdoor->pCloseSound->iFormID;
                BSAudio* v1 = &BSAudio::QInstance();
                v1->GetSoundHandleByNumericID(&SoundHandle, CastingType, 289u);
                SoundHandle.Play(false);
            }
        }
    }
    TESObjectCELL* pnewParentCell_3 = pPlayer->GetParentCell();
    if ((pnewParentCell_3) && (!pnewParentCell_3->GetInterior())) {
        pnewParentCell_3->ForceLODFadeInLoadedCells();
    }

    if ((gs_bSaveOnTravel_GamePlay.Bool()) && (pnewParentCell_3)) {
        pSaveLoadManager->SetAutoSaveTimerTo1();
    }
}

void TESObjectDOOR::PlayerPullFollower() {
    Actor* ptarget;
    ExtraFollower* pfollow = (&pPlayer->m_Extra)->GetFollowerExtra();
    if (pfollow) {
        BSSimpleList<Actor*>* v1;
        BSSimpleList<Actor*>* pTempList;
        BSSimpleList<Actor*>* v4 = static_cast<BSSimpleList<Actor*>*>(ourMalloc(sizeof(BSSimpleList<Actor*>)));
        if (v4) {
            pTempList = ::new (v4) BSSimpleList<Actor*>;
        }
        else {
            pTempList = nullptr;
        }
        BSSimpleList<Actor*>* plist = pTempList;
        BSSimpleList<Actor*>* v3 = static_cast<BSSimpleList<Actor*>*>(ourMalloc(sizeof(BSSimpleList<Actor*>)));
        if (v3) {
            v1 = ::new (v3) BSSimpleList<Actor*>;
        }
        else {
            v1 = nullptr;
        }
        BSSimpleList<Actor*>* plist_2 = v1;
        for (Actor*& rpItem : pfollow->pActorlist->ItemsUntilNull()) {
            plist_2->AddTail(rpItem);
        }
        while (plist_2 && plist_2->m_item) {
            ptarget = plist_2->m_item;
            if (ptarget) {
                if (ptarget->IsFollowing(pPlayer)) {
                    ptarget->ForceStopMoving();
                    float afTime = appCalendar.GetHour() - 1.0;
                    ptarget->pCurrentProcess->SetLastTimeProcessed(afTime);
                }
                else {
                    plist->AddHead(ptarget);
                }
            }
            plist_2 = plist_2->m_pkNext;
        }
        for (plist_2 = plist; plist_2 && plist_2->m_item; plist_2 = plist_2->m_pkNext) {
            Actor* apActor = plist_2->m_item;
            (&pPlayer->m_Extra)->RemoveFollower(apActor);
        }
        plist->RemoveAll();
        if (plist) {
            delete plist;
        }
        if (pfollow->pActorlist->IsEmpty()) {
            (&pPlayer->m_Extra)->RemoveFollowerExtra();
        }
    }
}

void TESObjectDOOR::LinkRandomTeleportDoors(TESObjectREFR* apDoor1, TESObjectREFR* apDoor2) {
    NiPoint3 v16;
    NiPoint3 dst;
    NiPoint3 delta_2;
    NiPoint3 delta;
    if ((apDoor1) && (apDoor2) && (apDoor1 != apDoor2)) {
        TESObjectDOOR* pbasedoor1 = dynamic_cast<TESObjectDOOR*>(apDoor1->GetObjectReference());
        TESObjectDOOR* pbasedoor2 = dynamic_cast<TESObjectDOOR*>(apDoor2->GetObjectReference());
        if ((pbasedoor1) && (pbasedoor2) && (!apDoor1->GetTeleport() && !apDoor2->GetTeleport())) {
            DoorTeleportData* ptel1 = apDoor1->AddTeleport();
            DoorTeleportData* ptel2 = apDoor2->AddTeleport();
            if ((ptel1) && (ptel2)) {
                float fdefdist = gs_fRandomDoorDistance.Float();
                float fAutoDoorActivateDistance = gs_fAutoDoorActivateDistance.Float() + 100.0;
                float fautodist = BSMAX(fdefdist, fAutoDoorActivateDistance);
                float f180 = 3.1415927f;
                ptel1->pLinkedDoor = apDoor2;
                TESObjectREFR* pmark = apDoor2->GetRandomTeleportMarker();
                if (pmark) {
                    const NiPoint3* v2 = &pmark->GetLocationOnReference();
                    ptel1->SetTeleportPosition(*v2);
                    ptel1->SetTeleportRotation(pmark->GetAngleOnReference());
                }
                else {
                    apDoor2->GetDeltaForAngle(&delta, f180);
                    if (pbasedoor2->IsAutomatic()) {
                        delta.operator*=(fautodist);
                    }
                    else {
                        delta.operator*=(fdefdist);
                    }
                    const NiPoint3* v3 = &apDoor2->GetLocationOnReference();
                    ptel1->SetTeleportPosition(*v3 + delta);
                    const NiPoint3* AngleOnReference = &apDoor2->GetAngleOnReference();
                    float v14 = AngleOnReference->z + f180;
                    NiPoint3* v6 = ::new (&dst) NiPoint3(0.0f, 0.0f, v14);
                    ptel1->SetTeleportRotation(*v6);
                }
                ptel2->pLinkedDoor = apDoor1;
                pmark = apDoor1->GetRandomTeleportMarker();
                if (pmark) {
                    const NiPoint3* v7 = &pmark->GetLocationOnReference();
                    ptel2->SetTeleportPosition(*v7);
                    const NiPoint3* a2 = &pmark->GetAngleOnReference();
                    ptel2->SetTeleportRotation(*a2);
                }
                else {
                    apDoor1->GetDeltaForAngle(&delta_2, f180);
                    if (pbasedoor1->IsAutomatic()) {
                        delta_2.operator*=(fautodist);
                    }
                    else {
                        delta_2.operator*=(fdefdist);
                    }
                    const NiPoint3* v8 = &apDoor1->GetLocationOnReference();
                    ptel2->SetTeleportPosition(*v8 + delta_2);
                    const NiPoint3* v10 = &apDoor1->GetAngleOnReference();
                    float v13 = v10->z + f180;
                    NiPoint3* v11 = ::new (&v16) NiPoint3(0.0f, 0.0f, v13);
                    ptel2->SetTeleportRotation(*v11);
                }
            }
        }
    }
}

hkpMotion::MotionType hkpRigidBody::getMotionType() {
    return getMotion()->getType();
}
