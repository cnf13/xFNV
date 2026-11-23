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
#include "xfnv/Compiler.h"

#include <limits>

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
    cFlags = 0;
}

void TESObjectDOOR::ClearData() {
    ClearRandomTeleportList();
}

// Reads the door's chunks: the name, model and script components, the destructible data and the door's own sounds, flag
// byte and teleport list. There is no handler for a DATA chunk here.
bool TESObjectDOOR::Load(TESFile* apFile) {
    if (static_cast<uint8_t>(apFile->GetTESForm()) != DOOR_ID) {
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
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
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
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(tempint, pFile);
            TESForm* pform = TESForm::GetFormByNumericID(tempint);
            if (pform) {
                plist->SetItem(pform);
                plastlist = plist;
                plist = plist->m_pkNext;
            }
            else {
                int32_t iDoorFormID = iFormID;
                const char* pcEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find RandomTeleport (%08X) for Door '%s' (%08X).",
                    tempint,
                    pcEditorID,
                    iDoorFormID
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
    int32_t iCount = RandomTeleports.ItemsInList();
    if (iCount != pcomplist->ItemsInList()) {
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
            Interface::ShowMessage(
                gs_sRandomDoorTeleportFailureMessage.String(),
                0,
                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                nullptr,
                2.0f,
                false
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
    BaseProcess* pprocess = pActionActor ? pActionActor->pCurrentProcess : nullptr;
    if (pActionActor) {
        if (pActionActor->pCurrentProcess->GetCurrentFurniture() && pActionActor->GetSitSleepState()) {
            return false;
        }
        if (pActionActor == pPlayer) {
            pPlayer->iCombatPersue = 0;
        }
        bignorelock = TESObjectDOOR::CanActorIgnoreLock(apItemActivated, pActionActor, true, true);
        if (pLock && pLock->IsLocked() && !bignorelock) {
            blockpick = false;
            if (pActionActor->IsNpc()) {
                blockpick = pActionActor->HasObjects(pLockpick, POTYPE_NONE, 1, 0, eUseItem);
            }
            if (pLock->pKey) {
                char msg[260];
                if (!pstaticdoor && !pActionActor->HasObjects(pLock->pKey, POTYPE_NONE, 1, 0, eUseItem)) {
                    Animation* panim;
                    ExtraLock* pextralock;
                    if (pActionActor == pPlayer) {
                        pextralock = apItemActivated->GetExtraLock();
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
                    pextralock = apItemActivated->GetExtraLock();
                    if (pextralock) {
                        pextralock->UnLockDoor();
                        apItemActivated->AddLockChange();
                    }
                }
                else if (pActionActor == pPlayer && !pstaticdoor) {
                    pstaticdoor = static_cast<MobileObject*>(apItemActivated);
                    ExtraLock* pextralock = apItemActivated->GetExtraLock();
                    if (pextralock) {
                        pextralock->UnLockDoor();
                        apItemActivated->AddLockChange();
                    }
                    BSAudio::QInstance().GetSoundHandleByName("UILockpickingUnlock", 289u).Play(false);
                    BSsprintf(msg, 255, sOpenWithKey.String(), pLock->pKey->GetFullName());
                    Interface::ShowMessage(
                        msg,
                        0,
                        "Interface\\Icons\\Message Icons\\glow_message_key.dds",
                        nullptr,
                        2.0f,
                        false
                    );
                    pstaticdoor->Activate(pPlayer, false, nullptr, 1);
                    pstaticdoor = nullptr;
                    return true;
                }
            }
            else {
                if (pActionActor != pPlayer) {
                    if (!blockpick) {
                        return false;
                    }
                    Animation* panim = pActionActor->GetAnimation();
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
                    ExtraLock* pextralock = apItemActivated->GetExtraLock();
                    if (pextralock) {
                        pextralock->UnLockDoor();
                        apItemActivated->AddLockChange();
                    }
                }
                else {
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
                        Interface::PlayMenuSound(18);
                    }
                    else {
                        _ShowLockPickMenu(apItemActivated, 0, false);
                    }
                    return false;
                }
            }
        }
    }
    if (pTel && pActionActor) {
        if (!pPlayer->IsSleepingorResting() &&
            !pprocess->eLevel &&
            pprocess->GetFadeState() != BaseProcess::FADE_TELEPORT_OUT) {
            if (pOpenSound && apItemActivated->Get3D()) {
                BSSoundHandle kSoundHandle;
                if (pActionActor == pPlayer) {
                    uint32_t uiSoundID = pOpenSound->iFormID;
                    kSoundHandle = BSAudio::QInstance().GetSoundHandleByNumericID(
                        uiSoundID,
                        BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                            BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_UNKBIT31
                    );
                }
                else {
                    uint32_t uiSoundID = pOpenSound->iFormID;
                    kSoundHandle = BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, 258u);
                }
                if (pActionActor != pPlayer) {
                    NiPoint3 kLocation = apItemActivated->GetLocationOnReference();
                    kSoundHandle.SetPosition(kLocation.x, kLocation.y, kLocation.z);
                }
                kSoundHandle.Play(false);
                if (pActionActor == pPlayer) {
                    if (!BSAudio::QInstance().bMultiThread) {
                        for (int32_t i = 0; i < 4; ++i) {
                            BSAudio::QInstance().Update(false);
                        }
                    }
                }
            }

            static_cast<HighProcess*>(pprocess)->HighProcess::FadeOut(pActionActor, apItemActivated, true);
            return pActionActor == pPlayer;
        }
        TESObjectCELL* ptelcell = pTel->GetTeleportCell();
        TESWorldSpace* ptelworld = pTel->GetTeleportWorldSpace();
        pActionActor->pCurrentProcess->SetDoor(apItemActivated);
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
                apActionRef->SetAngleOnReferenceZ(pTel->rotation.z);
                apActionRef->SetAngleOnReferenceX(0.0f);
            }
            else {
                apActionRef->SetAngleOnReferenceX(std::numeric_limits<float>::max());
            }
            Actor* pac = dynamic_cast<Actor*>(apActionRef);
            if (pac) {
                pac->SetForceUpdateQuest(false);
            }
            TESObjectREFR::MoveRefToNewSpace(apActionRef, ptelcell, ptelworld);
        }
    }
    else {
        ExtraLock* pextralock = apItemActivated->GetExtraLock();
        if (pextralock) {
            pextralock->UnLockDoor();
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

NiAVObject* TESObjectDOOR::Clone3D(TESObjectREFR* apRequester) {
    NiAVObject* p3D = TESBoundObject::Clone3D(apRequester);
    if (pOpenSound) {
        if (!BSAudio::QInstance().bMultiThread) {
            TESSound* pSound = pOpenSound;
            const char* pcFilename = pSound->GetFilename();
            BSAudio::QInstance().PrecacheSound(pcFilename, 289u, pSound);
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
                BSPlayerDistanceCheckController* pcontrol = new BSPlayerDistanceCheckController;
                pcontrol->SetCheckDistance(gs_fAutoDoorActivateDistance.Float());
                pcontrol->SetCallback(&TESObjectDOOR::AutoDoorCallback);
                pcontrol->SetTarget(p3D);
            }
            NiExtraData* pExtraData = p3D->GetExtraData(BSXFlags::Tag());
            BSXFlags* pextra = static_cast<BSXFlags*>(pExtraData);
            if (!pextra) {
                pextra = new BSXFlags;
                p3D->AddExtraData(BSXFlags::Tag(), pextra);
            }
            pextra->SetFlags(BSXFlags::ANIMATED, true);
        }
    }
    return p3D;
}

bool TESObjectDOOR::IsSlidingDoor() {
    return (cFlags & FLAG_SLIDING) != 0;
}

TESSound* TESObjectDOOR::GetLoopSound() {
    return pLoopSound;
}

bool TESObjectDOOR::IsRandomDoor() {
    return !RandomTeleports.IsEmpty();
}

void TESObjectDOOR::ClearRandomTeleportList() {
    RandomTeleports.RemoveAll();
}

bool TESObjectDOOR::IsHidden() {
    return (cFlags & FLAG_HIDDEN) != 0;
}

bool TESObjectDOOR::IsMinimalUse() {
    return (cFlags & FLAG_MINIMAL_USE) != 0;
}

void TESObjectDOOR::SetMinimalUse(bool abVal) {
    if (abVal) {
        cFlags |= FLAG_MINIMAL_USE;
    }
    else {
        cFlags &= ~FLAG_MINIMAL_USE;
    }
}

bool TESObjectDOOR::IsAutomatic() {
    return (cFlags & FLAG_AUTOMATIC) != 0;
}

void TESObjectDOOR::HandlePlayerTeleport(TESObjectREFR* apActivatedDoor) {
    if (apActivatedDoor && apActivatedDoor->GetObjectReference() == this) {
        DoorTeleportData* pTel = apActivatedDoor->GetTeleport();
        if (pTel) {
            PLAYER_TARGET_LOC* ploc;
            TESObjectCELL* ptelcell = pTel->GetTeleportCell();
            TESWorldSpace* ptelworld = pTel->GetTeleportWorldSpace();
            ploc = new PLAYER_TARGET_LOC;
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
            bool bInterior = poriginalParentCell ? poriginalParentCell->GetInterior() : false;
            bStaticWasInInterior = bInterior;
        }
    }
}

bool TESObjectDOOR::RandomlyTeleportsTo(TESForm* apSpace) {
    if (apSpace && TESForm::IsInteriorCellOrWorldspace(apSpace)) {
        return RandomTeleports.IsInList(apSpace);
    }

    return false;
}

TESObjectREFR* TESObjectDOOR::FindRandomTeleportTarget(TESObjectREFR* apStartDoor, TESObjectREFR* apActionRef) {
    TESObjectREFR* pretdoor = nullptr;
    TESObjectREFR* pbackup = nullptr;
    if (apStartDoor) {
        TESForm* pstartspace = apStartDoor->GetWorldSpace();
        if (!pstartspace && apStartDoor->GetParentCell() && apStartDoor->GetParentCell()->GetInterior()) {
            pstartspace = apStartDoor->GetParentCell();
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
                if (pbackup && ibackuptry == -1) {
                    ibackuptry = itry;
                }

                if (pretdoor && itry == ilasttry) {
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
        if ((apActor->IsFollowing() &&
             apActor->GetCurrentAIProcess()->GetTarget() == pPlayer &&
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
    DoorLock kLock(apDoorRef);
    return TESObjectDOOR::CanActorIgnoreLock(&kLock, apActor, apDoorRef, abActivate, abMovement);
}

void TESObjectDOOR::AutoDoorCallback(NiAVObject* apDoor3D) {
    TESBoundObject* pBaseForm;
    BaseProcess* pBaseProc;
    HighProcess* pHighProc;
    TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(apDoor3D);
    if (pRef) {
        pBaseForm = pRef->TESObjectREFR::GetObjectReference();
        if (pBaseForm->TESForm::GetFormType() == DOOR_ID && !pRef->TESForm::GetDestroyed()) {
            pBaseProc = pPlayer->MobileObject::GetCurrentAIProcess();
            pHighProc = dynamic_cast<HighProcess*>(pBaseProc);
            if (pHighProc && pHighProc->GetFadeState() != BaseProcess::FADE_TELEPORT_OUT) {
                pPlayer->SetForceActivateRef(pRef);
            }
        }
    }
}

void TESObjectDOOR::DoorTeleportPlayerArrivalCallback(uintptr_t aiArrivalData) {
    // The arrival data is the integer field PLAYER_TARGET_LOC::iArrivalFuncData; Activate stored the door reference in
    // it.
    TESObjectREFR* pDoorRef = reinterpret_cast<TESObjectREFR*>(aiArrivalData);
    if (aiArrivalData) {
        TESObjectDOOR::PlayerPullFollower();
        if (pPlayer->GetParentCell() && pPlayer->GetParentCell()->GetInterior()) {
            pPlayer->GetParentCell()->ProcessActors();
        }

        TESObjectDOOR* pDoor = static_cast<TESObjectDOOR*>(pDoorRef->GetObjectReference());
        if (pDoor->pCloseSound && pDoorRef->GetTeleport()->pLinkedDoor) {
            TESObjectREFR* pLinkedDoor = pDoorRef->GetTeleport()->pLinkedDoor;
            if (pLinkedDoor->Get3D()) {
                uint32_t uiSoundID = pDoor->pCloseSound->iFormID;
                BSSoundHandle kSoundHandle = BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, 289u);
                kSoundHandle.Play(false);
            }
        }
    }
    TESObjectCELL* pNewParentCell = pPlayer->GetParentCell();
    if (pNewParentCell && !pNewParentCell->GetInterior()) {
        pNewParentCell->ForceLODFadeInLoadedCells();
    }

    if (gs_bSaveOnTravel_GamePlay.Bool() && pNewParentCell) {
        pSaveLoadManager->SetAutoSaveTimerTo1();
    }
}

void TESObjectDOOR::PlayerPullFollower() {
    Actor* ptarget;
    ExtraFollower* pfollow = pPlayer->m_Extra.GetFollowerExtra();
    if (pfollow) {
        BSSimpleList<Actor*>* plist = new BSSimpleList<Actor*>;
        BSSimpleList<Actor*>* pcopyList = new BSSimpleList<Actor*>;
        BSSimpleList<Actor*>* pnode = pcopyList;
        for (Actor*& rpItem : pfollow->pActorlist->ItemsUntilNull()) {
            pnode->AddTail(rpItem);
        }
        while (pnode && pnode->m_item) {
            ptarget = pnode->m_item;
            if (ptarget) {
                if (ptarget->IsFollowing(pPlayer)) {
                    ptarget->ForceStopMoving();
                    float fTime = appCalendar.GetHour() - 1.0;
                    ptarget->pCurrentProcess->SetLastTimeProcessed(fTime);
                }
                else {
                    plist->AddHead(ptarget);
                }
            }
            pnode = pnode->m_pkNext;
        }
        for (pnode = plist; pnode && pnode->m_item; pnode = pnode->m_pkNext) {
            Actor* pActor = pnode->m_item;
            pPlayer->m_Extra.RemoveFollower(pActor);
        }
        plist->RemoveAll();
        if (plist) {
            delete plist;
        }
        if (pfollow->pActorlist->IsEmpty()) {
            pPlayer->m_Extra.RemoveFollowerExtra();
        }
    }
}

void TESObjectDOOR::LinkRandomTeleportDoors(TESObjectREFR* apDoor1, TESObjectREFR* apDoor2) {
    NiPoint3 kRotation2;
    NiPoint3 kRotation1;
    if (apDoor1 && apDoor2 && apDoor1 != apDoor2) {
        TESObjectDOOR* pbasedoor1 = dynamic_cast<TESObjectDOOR*>(apDoor1->GetObjectReference());
        TESObjectDOOR* pbasedoor2 = dynamic_cast<TESObjectDOOR*>(apDoor2->GetObjectReference());
        if (pbasedoor1 && pbasedoor2 && !apDoor1->GetTeleport() && !apDoor2->GetTeleport()) {
            DoorTeleportData* ptel1 = apDoor1->AddTeleport();
            DoorTeleportData* ptel2 = apDoor2->AddTeleport();
            if (ptel1 && ptel2) {
                float fdefdist = gs_fRandomDoorDistance.Float();
                float fAutoDoorActivateDistance = gs_fAutoDoorActivateDistance.Float() + 100.0;
                float fautodist = BSMAX(fdefdist, fAutoDoorActivateDistance);
                float f180 = 3.1415927f;
                ptel1->pLinkedDoor = apDoor2;
                TESObjectREFR* pmark = apDoor2->GetRandomTeleportMarker();
                if (pmark) {
                    ptel1->SetTeleportPosition(pmark->GetLocationOnReference());
                    ptel1->SetTeleportRotation(pmark->GetAngleOnReference());
                }
                else {
                    NiPoint3 delta = apDoor2->GetDeltaForAngle(f180);
                    if (pbasedoor2->IsAutomatic()) {
                        delta *= fautodist;
                    }
                    else {
                        delta *= fdefdist;
                    }
                    ptel1->SetTeleportPosition(apDoor2->GetLocationOnReference() + delta);
                    float fAngle = apDoor2->GetAngleOnReference().z + f180;
                    ::new (&kRotation1) NiPoint3(0.0f, 0.0f, fAngle);
                    ptel1->SetTeleportRotation(kRotation1);
                }
                ptel2->pLinkedDoor = apDoor1;
                pmark = apDoor1->GetRandomTeleportMarker();
                if (pmark) {
                    ptel2->SetTeleportPosition(pmark->GetLocationOnReference());
                    ptel2->SetTeleportRotation(pmark->GetAngleOnReference());
                }
                else {
                    NiPoint3 delta = apDoor1->GetDeltaForAngle(f180);
                    if (pbasedoor1->IsAutomatic()) {
                        delta *= fautodist;
                    }
                    else {
                        delta *= fdefdist;
                    }
                    ptel2->SetTeleportPosition(apDoor1->GetLocationOnReference() + delta);
                    float fAngle = apDoor1->GetAngleOnReference().z + f180;
                    ::new (&kRotation2) NiPoint3(0.0f, 0.0f, fAngle);
                    ptel2->SetTeleportRotation(kRotation2);
                }
            }
        }
    }
}

hkpMotion::MotionType hkpRigidBody::getMotionType() {
    return getMotion()->getType();
}
