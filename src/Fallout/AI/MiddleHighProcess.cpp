#include "Fallout/AI/MiddleHighProcess.h"

#include "BSAudio/BSAudio.h"
#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/WeaponObject.h"
#include "BSHavok/bhkBlendController.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCharacterListener.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkRagdollPenetrationUtil.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSBodyPartData.h"
#include "Fallout Shared/BGSIdleCollection.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BSCloneReserver.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/LipSynchAnim.h"
#include "Fallout Shared/Magic/EnchantmentItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageData.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageSchedule.h"
#include "Fallout Shared/PackageTarget.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/PathingRequestFlee.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESClass.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFurniture.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectMISC.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/TeleportData.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ActorPackage.h"
#include "Fallout/AI/ActorPackageData.h"
#include "Fallout/AI/AlarmPackage.h"
#include "Fallout/AI/ArrowProjectile.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CachedValues.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/DialoguePackage.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/FollowerBarks.h"
#include "Fallout/AI/GunWobble.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/LowProcess.h"
#include "Fallout/AI/MiddleLowProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PatrolPackage.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/Procedure.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/AI/QueuedItem.h"
#include "Fallout/AI/SandBoxPackage.h"
#include "Fallout/AI/TrespassPackage.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/DetectLifeEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicShaderHitEffect.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/GarbageCollector.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSMultiTargetTreadTransfController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMultiTargetTransformController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramFactory.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterStateManager.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>
#include <cstdint>
#include <limits>

// The game settings this file defines, with their IDs and values, in initialization order.
BSSimpleList<TESObjectREFR*> MiddleHighTempRefListChairBed;
SettingT<INISettingCollection> fAimUpDegrees("fAimUpDegrees:Combat", 90.0f);
SettingT<INISettingCollection> fAimDownDegrees("fAimDownDegrees:Combat", 90.0f);
SettingT<INISettingCollection> fAimChaseLookingMult("fAimChaseLookingMult:Combat", 3.0f);
BSCriticalSection TESObjectREFR::kFurnitureLock;
BSPerformanceTimer AIProcessing_MiddleHighUpdate("AIProcessing", "MiddleHighUpdate", true, true);

MiddleHighProcess::DeferredHideLimb::DeferredHideLimb() : spDismemberedLimbRoot(nullptr), spReplacementLimb(nullptr) {}

MiddleHighProcess::DeferredHideLimb::~DeferredHideLimb() {}

MiddleHighProcess::MiddleHighProcess()
  : fPursueTimer(0.0f), fEquippedWeight(0.0f), bDoneOnce(false), bAimingTarget(false), bNeedReparentWeapon(false),
    bDrawn(false), eUseItem(POTYPE_NONE), pLastIdlePlayed(nullptr), bPickPackIdle(false), pWeapon(nullptr),
    pAmmo(nullptr), bWeaponGrenade(false), bWeaponMine(false), bWeaponThrown(false), bHeavyArmorWorn(false),
    bPowerArmorWorn(false), bPowerArmorHelmet(false), bHasBackPackWorn(false), pWeaponBone(nullptr), pFireNode(nullptr),
    bWantWeaponDrawn(false), bWeaponDrawn(false), cKnockState(0), cSitSleepState(0), pCurrentFurniture(nullptr),
    cCurrentFurnitureIndex(127), FurnitureMarker(255u), pCommandingActor(nullptr), pLastBoundWeapon(nullptr),
    pCurrentPackageSpell(nullptr), bCheckMagicNode(false), pCurrentWeaponEffect(nullptr), fAlphaMult(1.0f),
    fScriptRefractPower(0.0f), m_pFaceAnimationData(nullptr), bRefreshMagicShaderEffects(false),
    bRefreshMagicShaderEffectBody(false), bRefreshMagicShaderEffectWeapon(false), bRefreshMagicShaderEffectLoading(0),
    bRefreshFlareFlags(false), uiFlareFlags(0), bPickPocketed(false), bDoneClothesChange(false),
    bDetectLifeDetected(false), bSummonedCreature(false), cUpdate3DModel(0), bForceNextUpdate(false),
    iPackageIdleNumber(-1), pCurrentIdle(nullptr), fPackageIdleTimer(-1.0f), bPlayedBeginIdles(false),
    bPlayedEndIdles(false), pCreatureLipsynchAnim(nullptr), iCreatureLipsynchStartTime(0), iReservationSlot(-1),
    pAttachedArrowList(nullptr), pAnimation(nullptr), fAimLooking(0.0f), fAutomaticShotDelay(0.0f), cShotsToFire(1),
    bBeenAttacked(false), pHeadNode(nullptr), pTorsoNode(nullptr), pBSBound(nullptr), bIronSights(false),
    iAnimActionSuccess(2), iMovementAnimations(0), pListItemstoEquipUnequip(nullptr), fRadiationDelta(0.0f),
    fRadiationMagicDelta(0.0f), fRadiationWaterDelta(0.0f), pLastHitData(nullptr), iWeaponConditionStage(-1),
    pFaceNode(nullptr), pFaceNodeSkinned(nullptr), pAnimFace(nullptr), pLastAttackHitData(nullptr),
    nFurnitureListTimer(0) {
    pCachedValues = new CachedValues();
    MiddleHighProcess::SetCurrentSpell(nullptr);
    MiddleHighProcess::SetDesiredTarget(nullptr);
    pActiveEffectList = new BSSimpleList<ActiveEffect*>;
    RunOncePackage.Clear();
    ourMemset(pAimSequence, 0, sizeof(pAimSequence));
    ourMemset(pDamageRootNode, 0, sizeof(pDamageRootNode));
    iNumberItemsActivate = 0;
    FurnitureMarker.SetHeading(0.0f);
    FurnitureMarker.SetType(0);
    FurnitureMarker.SetPosition(NiPoint3::ZERO);
    fEssentialDownTimer = 0.0f;
    eLevel = PROCESS_TYPE_MIDDLE_HIGH;
}

MiddleHighProcess::~MiddleHighProcess() {
    if (pCachedValues) {
        ourFree(pCachedValues);
        pCachedValues = nullptr;
    }
    if (pActiveEffectList) {
        for (ActiveEffect* peffect : pActiveEffectList->ItemsUntilNull()) {
            if (peffect) {
                delete peffect;
            }
        }
        pActiveEffectList->RemoveAll();
        delete pActiveEffectList;
    }
    if (pWeapon) {
        delete pWeapon;
    }
    if (pAmmo) {
        delete pAmmo;
    }
    pFireNode = nullptr;
    pWeapon = nullptr;
    pAmmo = nullptr;
    spCharController = nullptr;
    if (MiddleHighProcess::GetRunOncePackage()) {
        if (pSaveLoadGameOLD->GetLoading()) {
            pSaveLoadGameOLD->DeleteForm(MiddleHighProcess::GetRunOncePackage());
        }
        else {
            MiddleHighProcess::ClearRunOncePackage();
        }
    }
    if (!DeadDetectList.IsEmpty()) {
        DeadDetectList.RemoveAll();
    }
    MiddleHighProcess::SetAnimation(nullptr);
    pLightingProperty = nullptr;
    RefListChairBed.RemoveAll();
    if (pAttachedArrowList) {
        for (BSSimpleList<ArrowProjectile*>* pList = pAttachedArrowList; pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            if (pList->m_item) {
                pList->m_item->ClearAttachedRef();
            }
        }
        pAttachedArrowList->RemoveAll();
        delete pAttachedArrowList;
    }
    pAttachedArrowList = nullptr;
    spWeaponPreload = nullptr;
    spReserveProjectileClones = nullptr;
    pCurrentFurniture = nullptr;
    ClearReservation();
    while (!DeferredHideLimbList.IsEmpty()) {
        delete DeferredHideLimbList.m_item;
        DeferredHideLimbList.RemoveHead();
    }
    if (pCreatureLipsynchAnim) {
        delete pCreatureLipsynchAnim;
    }
    if (pLastHitData) {
        ourFree(pLastHitData);
    }
    pFaceNode = nullptr;
    pFaceNodeSkinned = nullptr;
    pAnimFace = nullptr;
    MiddleHighProcess::RemoveAllQueuedItems();
}

void MiddleHighProcess::HandleDeletedReference(TESObjectREFR* apRef) {
    CurrentPackage.HandleDeletedReference(apRef);
    ActorPackageData* pActorPackageData = BaseProcess::GetInstanceDataThatIsRunning();
    if (pActorPackageData) {
        pActorPackageData->HandleDeletedReference(apRef);
    }
    RunOncePackage.HandleDeletedReference(apRef);
    pActorPackageData = GetRunOncePackageInstanceData();
    if (pActorPackageData) {
        pActorPackageData->HandleDeletedReference(apRef);
    }
}

void MiddleHighProcess::Update(MobileObject* apRef, float fTimeDelta) {
    if (apRef) {
        Actor* pActor = dynamic_cast<Actor*>(apRef);
        bool bEvalFlag = true;
        if (pActor && pActor->GetRefPersists()) {
            CheckforNewPackage(pActor, false);
            TESPackage* pPack = GetPackageThatIsRunning();
            ResetTarget();
            if (pPack && pPack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_NONE && !pPack->GetWaiting()) {
                while (bEvalFlag) {
                    uint32_t uiProcedureType = pPack->ePROCEDURE_TYPE;
                    switch (Procedure::Action[uiProcedureType][GetProcedureIndexRunning()]) {
                        case PROCEDURE_TRAVEL: {
                            TESObjectREFR* pRef = nullptr;
                            if (pPack->pPackLoc) {
                                pRef = pPack->pPackLoc->GetLocReference();
                            }
                            if (pGenericLocation && !GetRunOncePackage()) {
                                pRef = pGenericLocation;
                            }

                            if (pActor->GetSitSleepState()) {
                                TESPackage* pActorPackage = pActor->GetCurrentPackage();
                                if (!pActorPackage->IsActorAtLocation(pActor, false, -1.0f, false)) {
                                    ProcessGetUpOneHour(pActor);
                                }
                            }
                            ProcessTravel(pActor, true, true, 257, false);
                            bEvalFlag = pActor->GetSitSleepState() == SIT_SLEEP_NORMAL &&
                                pActor->GetCurrentPackage() &&
                                pActor->GetCurrentPackage()->GetPackType() != PACKAGE_TYPE_TRAVEL &&
                                pActor->GetCurrentPackage()->IsActorAtLocation(pActor, false, -1.0f, false) &&
                                pActor->pCurrentProcess->eLevel == PROCESS_TYPE_MIDDLE_HIGH &&
                                (!pRef || !pRef->IsFurniture());
                            break;
                        }
                        case PROCEDURE_WANDER: {
                            bEvalFlag = false;
                            break;
                        }
                        case PROCEDURE_ACTIVATE: {
                            bEvalFlag = ProcessActivateOneHour(pActor);
                            break;
                        }
                        case PROCEDURE_SLEEP: {
                            bEvalFlag = false;
                            break;
                        }
                        case PROCEDURE_EAT: {
                            bEvalFlag = ProcessEatOneHour(pActor);
                            break;
                        }
                        case PROCEDURE_FOLLOW: {
                            if (pActor->GetSitSleepState()) {
                                ProcessGetUpOneHour(pActor);
                            }
                            bEvalFlag = ProcessFollowOneHour(pActor, 3600.0f, true);
                            break;
                        }
                        case PROCEDURE_ESCORT: {
                            if (pActor->GetSitSleepState()) {
                                ProcessGetUpOneHour(pActor);
                            }
                            bEvalFlag = ProcessEscortOneHour(pActor);
                            break;
                        }
                        case PROCEDURE_ALARM: {
                            if (pActor->GetSitSleepState()) {
                                ProcessGetUpOneHour(pActor);
                            }
                            ProcessAlarm(pActor);
                            bEvalFlag = false;
                            break;
                        }
                        case PROCEDURE_COMBAT: {
                            bEvalFlag = ProcessCombatOneHour(pActor, fTimeDelta);
                            break;
                        }
                        case PROCEDURE_FLEE: {
                            if (pActor->GetSitSleepState()) {
                                ProcessGetUpOneHour(pActor);
                            }
                            ProcessFlee(pActor);
                            bEvalFlag = false;
                            break;
                        }
                        case PROCEDURE_DIALOGUE: {
                            ProcessDialogue(pActor);
                            bEvalFlag = false;
                            break;
                        }
                        case PROCEDURE_WAIT: {
                            if (!pTarget) {
                                SetTargetForPackage(pActor);
                            }
                            if (!pTarget ||
                                pTarget->GetDelete() ||
                                pPack->IsActorAtRefTarget(
                                    pActor,
                                    GetCurrentPackage()->GetLocationRadius(pActor) == 0 ? 400 : 0
                                )) {
                                AddToProcedureIndexRunning(pActor, 1);
                            }
                            bEvalFlag = false;
                            break;
                        }
                        case PROCEDURE_TRAVEL_TARGET: {
                            if (!pTarget) {
                                SetTargetForPackage(pActor);
                            }
                            if (!pTarget) {
                                AddToProcedureIndexRunning(pActor, 1);
                            }
                            if (pTarget &&
                                pTarget->IsActor() &&
                                pTarget != pPlayer &&
                                pPack->GetPackType() == PACKAGE_TYPE_ESCORT) {
                                CreateFollowforEscort(pActor, GetCurrentPackage(), true);
                            }

                            ProcessActivateOneHour(pActor);
                            bEvalFlag = pActor->GetCurrentPackage() &&
                                pActor->GetCurrentPackage()->IsActorAtRefTarget(pActor, 0) &&
                                pActor->pCurrentProcess->eLevel == PROCESS_TYPE_MIDDLE_HIGH;
                            break;
                        }
                        case PROCEDURE_PURSUE: {
                            if (pActor->GetSitSleepState()) {
                                ProcessGetUpOneHour(pActor);
                            }
                            ProcessPursue(pActor, false, true, 257);
                            bEvalFlag = false;
                            break;
                        }
                        case PROCEDURE_GET_UP: {
                            ProcessGetUpOneHour(pActor);
                            AddToProcedureIndexRunning(pActor, 1);
                            break;
                        }
                        case PROCEDURE_MOUNT_HORSE: {
                            AddToProcedureIndexRunning(pActor, 1);
                            bEvalFlag = false;
                            break;
                        }
                        case PROCEDURE_NOTIFY: {
                            ProcessNotify(pActor);
                            bEvalFlag = false;
                            break;
                        }
                        case PROCEDURE_ACCOMPANY: {
                            ProcessAccompanyOneHour(pActor);
                            bEvalFlag = false;
                            break;
                        }
                        case PROCEDURE_USE_ITEM_AT: {
                            ProcessUseItemAt(pActor);
                            bEvalFlag = false;
                            break;
                        }
                        case PROCEDURE_CHOOSE_CAST: {
                            bEvalFlag = false;
                            break;
                        }
                        case PROCEDURE_REMOVE_WORN_ITEMS: {
                            ProcessRemoveWorn(pActor, true, false);
                            bEvalFlag = false;
                            break;
                        }
                        case PROCEDURE_AVOID_RADIATION: {
                            ProcessAvoidArea(pActor);
                            bEvalFlag = false;
                            break;
                        }
                        case PROCEDURE_TAKE_BACK_ITEM: {
                            AddToProcedureIndexRunning(pActor, 2);
                            break;
                        }
                        case PROCEDURE_GUARD: {
                            bEvalFlag = false;
                            break;
                        }
                        case PROCEDURE_USE_WEAPON: {
                            ProcessUseWeapon(pActor);
                            bEvalFlag = false;
                            break;
                        }
                        case PROCEDURE_OBSERVE_COMBAT: {
                            AddToProcedureIndexRunning(pActor, 1);
                            [[fallthrough]];
                        }
                        case PROCEDURE_DONE: {
                            bEvalFlag = false;
                            break;
                        }
                        default: {
                            bEvalFlag = false;
                            break;
                        }
                    }
                    if (pActor->GetCurrentProcessType() == PROCESS_TYPE_MIDDLE_HIGH) {
                        TESPackage* pRunningPack = GetPackageThatIsRunning();
                        if (pRunningPack && pRunningPack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_NONE) {
                            uint32_t uiRunningType = pRunningPack->ePROCEDURE_TYPE;
                            if (Procedure::Action[uiRunningType][GetProcedureIndexRunning()] == PROCEDURE_DONE) {
                                if (pRunningPack->ePROCEDURE_TYPE
                                        ? pRunningPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE ||
                                            pRunningPack->GetPackType() == PACKAGE_TYPE_EAT ||
                                            pRunningPack->GetPackType() == PACKAGE_TYPE_SLEEP
                                        : !pRunningPack->IsActorAtLocation(pActor, false, -1.0f, false)) {
                                    AddToProcedureIndexRunning(pActor, -1);
                                    return;
                                }
                                EndMoveMessage(pActor);
                                if (pRunningPack && pRunningPack->GetOncePerDay()) {
                                    char cDay = appCalendar.GetDay();
                                    pActor->SetRunOnceExtraPackage(pRunningPack, cDay);
                                }

                                if (!pRunningPack->GetPackageSchedule()->PSData.iDuration) {
                                    SetTarget(nullptr);
                                    if (!GetRunOncePackage() || ContinuingPackageforPC()) {
                                        if (GetCurrentPackage()->GetIsCreated()) {
                                            // Fetches the package once more and does not use it.
                                            GetCurrentPackage();
                                            if (GetCurrentPackage()->IsInterruptPackage()) {
                                                LoadPackageFromExtraData(pActor);
                                            }
                                            else {
                                                ClearCurrentPackage();
                                            }
                                            if (!pActor->IsPathingComplete()) {
                                                EndMoveMessage(pActor);
                                            }
                                        }
                                    }
                                    else {
                                        ClearRunOncePackage();
                                    }
                                    delete pAcquireObject;
                                    pAcquireObject = nullptr;
                                    pItemBeingUsed = nullptr;
                                    while (!ObjectList.IsEmpty()) {
                                        ObjectstoAcquire* pObject = ObjectList.m_item;
                                        delete pObject;
                                        ObjectList.Remove(pObject);
                                    }
                                    pGenericLocation = nullptr;
                                    GenericLocationsList.RemoveAll();
                                }
                            }
                        }
                    }
                    pPack = GetPackageThatIsRunning();
                }
            }
        }

        if (!pActor->IsWaitingOnPath()) {
            apRef->pCurrentProcess->ComputeLastTimeProcessed();
        }
    }
}

void MiddleHighProcess::Update(MobileObject* apRef) {
    AIProcessing_MiddleHighUpdate.Start();
    TESPackage* pPack = nullptr;
    if (apRef) {
        if (!apRef->Get3D() ||
            apRef->GetDelete() ||
            apRef->GetDisabled() ||
            !apRef->GetParentCell() ||
            !apRef->GetParentCell()->IsLoaded()) {
            ComputeLastTimeProcessed();
            return;
        }
        Actor* pActor = apRef->IsActor() ? static_cast<Actor*>(apRef) : nullptr;
        if (pActor) {
            if (bDetectLifeDetected) {
                bDetectLifeDetected = DetectLifeEffect::CheckActorDetected(pActor, bDetectLifeDetected);
            }
            bool bNewPackage = CheckforNewPackage(pActor, false);
            pPack = GetPackageThatIsRunning();
            if (bNewPackage || (!pPack && !pActor->IsPathingComplete())) {
                ClearCurrentDataforProcess(pActor);
            }
            pPack = GetPackageThatIsRunning();
            ResetTarget();
            if (bNewPackage) {
                pActor->AlertAccompanyToPackageChange();
                SetLocationForPackage(pActor, false);
            }
            if (pPack && pPack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_NONE && !pPack->GetWaiting()) {
                uint32_t uiProcedureType = pPack->ePROCEDURE_TYPE;
                switch (Procedure::Action[uiProcedureType][GetProcedureIndexRunning()]) {
                    case PROCEDURE_TRAVEL: {
                        ProcessTravel(pActor, bNewPackage, true, 257, false);
                        break;
                    }
                    case PROCEDURE_ACTIVATE: {
                        ProcessActivate(pActor, true);
                        break;
                    }
                    case PROCEDURE_ACQUIRE: {
                        ProcessAcquire(pActor);
                        break;
                    }
                    case PROCEDURE_SLEEP: {
                        Interface::GetConsoleSelection();
                        break;
                    }
                    case PROCEDURE_EAT: {
                        ProcessEat(pActor, nullptr);
                        break;
                    }
                    case PROCEDURE_FOLLOW: {
                        ProcessFollow(pActor, true, 257, false);
                        break;
                    }
                    case PROCEDURE_ESCORT: {
                        ProcessEscort(pActor);
                        break;
                    }
                    case PROCEDURE_ALARM: {
                        ProcessAlarm(pActor);
                        break;
                    }
                    case PROCEDURE_DIALOGUE: {
                        ProcessDialogue(pActor);
                        break;
                    }
                    case PROCEDURE_WAIT: {
                        if (!pTarget) {
                            SetTargetForPackage(pActor);
                        }
                        if (!pTarget || pTarget->GetDelete()) {
                            AddToProcedureIndexRunning(pActor, 1);
                        }
                        if (pPack->GetWaiting()) {
                            if (!pPlayer->IsPlayerFollower(pActor)) {
                                int32_t iNumFollowers = pPlayer->GetNumberOfFollowers();
                                if (iNumFollowers < iNumberActorsAllowedToFollowPlayer.Int()) {
                                    pPack->SetWaiting(false);
                                }
                            }
                        }
                        else {
                            if (pActor->GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_FOLLOW) {
                                float fWaitDistance = static_cast<float>(
                                    GetCurrentPackage()->GetPackageTarget()->iValue
                                );
                                if (static_cast<double>(fWaitDistance) >=
                                    pActor->GetDistanceFromReference(pTarget, false, false)) {
                                    AddToProcedureIndexRunning(pActor, 1);
                                }
                            }
                            else {
                                float fWaitDistance = 0.0f;
                                int32_t iDistPack = pPack->GetPackageTarget()->iValue;
                                if (iDistPack <= 0) {
                                    iDistPack = 200;
                                }
                                if (fWaitDistance == 0.0 && pActor->GetParentCell()->GetInterior()) {
                                    fWaitDistance = fAIEscortWaitDistanceInterior.Float();
                                }
                                else if (fWaitDistance == 0.0) {
                                    fWaitDistance = fAIEscortWaitDistanceExterior.Float() *
                                        static_cast<double>(iDistPack);
                                }
                                if (fWaitDistance / 2.0 >= pActor->GetDistanceFromReference(pTarget, false, false)) {
                                    AddToProcedureIndexRunning(pActor, 1);
                                }
                            }
                        }
                        break;
                    }
                    case PROCEDURE_TRAVEL_TARGET: {
                        ProcessActivate(pActor, false);
                        break;
                    }
                    case PROCEDURE_PURSUE: {
                        ProcessPursue(pActor, false, true, 257);
                        break;
                    }
                    case PROCEDURE_CREATE_FOLLOW: {
                        if (!pTarget) {
                            SetTargetForPackage(pActor);
                        }
                        if (!pTarget) {
                            AddToProcedureIndexRunning(pActor, 1);
                            if (!pActor->IsPathingComplete()) {
                                EndMoveMessage(pActor);
                            }
                        }
                        if (pTarget != pPlayer) {
                            CreateFollowforEscort(pActor, pPack, true);
                        }
                        AddToProcedureIndexRunning(pActor, 1);
                        break;
                    }
                    case PROCEDURE_GET_UP: {
                        ProcessGetUp(pActor);
                        if (pActor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
                            AddToProcedureIndexRunning(pActor, 1);
                        }
                        break;
                    }
                    case PROCEDURE_NOTIFY: {
                        ProcessNotify(pActor);
                        break;
                    }
                    case PROCEDURE_ACCOMPANY: {
                        ProcessAccompany(pActor);
                        break;
                    }
                    case PROCEDURE_USE_ITEM_AT: {
                        ProcessUseItemAt(pActor);
                        break;
                    }
                    case PROCEDURE_AMBUSH_WAIT: {
                        ProcessAmbushWait(pActor);
                        break;
                    }
                    case PROCEDURE_CHOOSE_CAST: {
                        ProcessChooseSpell(pActor);
                        break;
                    }
                    case PROCEDURE_FLEE_NON_COMBAT: {
                        ProcessFleeNonCombat(pActor);
                        break;
                    }
                    case PROCEDURE_REMOVE_WORN_ITEMS: {
                        ProcessRemoveWorn(pActor, true, false);
                        break;
                    }
                    case PROCEDURE_SUMMON_CREATURE_DEFEND: {
                        ProcessSummonCreatureDefend(pActor);
                        break;
                    }
                    case PROCEDURE_AVOID_RADIATION: {
                        ProcessAvoidArea(pActor);
                        break;
                    }
                    case PROCEDURE_TAKE_BACK_ITEM: {
                        AddToProcedureIndexRunning(pActor, 2);
                        break;
                    }
                    case PROCEDURE_PATROL: {
                        ProcessPatrol(pActor);
                        break;
                    }
                    case PROCEDURE_DIALOGUE_ACTIVATE: {
                        ProcessDialogueActivate(pActor);
                        break;
                    }
                    case PROCEDURE_USE_WEAPON: {
                        ProcessUseWeapon(pActor);
                        break;
                    }
                    case PROCEDURE_FLEE: {
                        ProcessFlee(pActor);
                        [[fallthrough]];
                    }
                    case PROCEDURE_DONE: {
                        if (!IsPackageDoneOnce() && !pPack->GetIsCreated() && pPack->IsInterruptPackage()) {
                            Script::SetActionFlag(pPack, &pActor->m_Extra, SCRIPT_ACTION_ONPACKAGEEND);
                            PackageDone(pActor, pPack);
                            SetPackageDoneOnce(true);
                        }
                        if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_PATROL) {
                            PackageSchedule* pSchedule = pPack->GetPackageSchedule();
                            if (!pSchedule || static_cast<double>(pSchedule->PSData.iDuration) <= 0.0) {
                                const TESPatrolPackageData* pPatrolData = pPack->GetPatrolPackageData();
                                if (pPatrolData && !pPatrolData->bRepeatable) {
                                    SetCurrentActionComplete(true);
                                    ClearCurrentDataforProcess(pActor);
                                    ClearCurrentPackage();
                                    CheckforNewPackage(pActor, true);
                                }
                            }
                            return;
                        }
                        if (pPack->GetPackType() == PACKAGE_TYPE_EAT ||
                            pPack->GetPackType() == PACKAGE_TYPE_SLEEP ||
                            (pPack->GetPackType() == PACKAGE_TYPE_TRAVEL &&
                             !pPack->IsActorAtLocation(pActor, false, -1.0f, false))) {
                            AddToProcedureIndexRunning(pActor, -1);
                            return;
                        }
                        if (pPack->GetOncePerDay()) {
                            char cDay = appCalendar.GetDay();
                            pActor->SetRunOnceExtraPackage(pPack, cDay);
                        }
                        Interface::GetConsoleSelection();
                        if (!pPack->GetPackageSchedule()->PSData.iDuration) {
                            SetTarget(nullptr);
                            if (GetRunOncePackage()) {
                                ClearRunOncePackage();
                            }
                            else {
                                if (GetCurrentPackage()->GetIsCreated()) {
                                    ClearCurrentPackage();
                                    if (!pActor->IsPathingComplete()) {
                                        EndMoveMessage(pActor);
                                    }
                                }
                            }
                            delete pAcquireObject;
                            pAcquireObject = nullptr;
                            while (!ObjectList.IsEmpty()) {
                                ObjectstoAcquire* pObject = ObjectList.m_item;
                                delete pObject;
                                ObjectList.Remove(pObject);
                            }
                            pGenericLocation = nullptr;
                            GenericLocationsList.RemoveAll();
                        }
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
            if (!pActor->IsWaitingOnPath()) {
                apRef->pCurrentProcess->ComputeLastTimeProcessed();
            }
        }
        else {
            apRef->pCurrentProcess->ComputeLastTimeProcessed();
        }
        AIProcessing_MiddleHighUpdate.Stop();
    }
}

bool MiddleHighProcess::CheckforNewPackage(Actor* apActor, bool abFlag) {
    if (apActor->IsInCombat() || apActor->IsinDialogue() || apActor->IsAlarmed() || apActor->IsRunningRunOnce()) {
        return false;
    }
    SIT_SLEEP_STATE eSitSleepState = apActor->GetSitSleepState();
    if (eSitSleepState && eSitSleepState != SIT_SLEEP_IS_SITTING && eSitSleepState != SIT_SLEEP_IS_SLEEPING) {
        return false;
    }
    if (GetRunOncePackage()) {
        ClearRunOncePackage();
    }
    float fHour = appCalendar.GetHour();
    bool bNewPackage = false;
    TESPackage* pPrevPackage = apActor->GetCurrentPackage();
    bool bRedress = false;
    if (pPrevPackage && (pPrevPackage->GetPackType() == PACKAGE_TYPE_SLEEP || pPrevPackage->GetForceUnequipWeapon())) {
        bRedress = true;
    }

    if (abFlag || !GetCurrentPackage() || iHourPackageEvaluated != static_cast<int32_t>(fHour)) {
        bNewPackage = LowProcess::CheckforNewPackage(apActor, abFlag);
        iHourPackageEvaluated = static_cast<int32_t>(fHour);
    }
    if (bNewPackage) {
        iAnimActionSuccess = 2;
        SetContinuingPackage(false);
        SetCurrentActionComplete(false);
        SetPackageIdleDone(false);
        SetTargetActivated(false);
        while (!ObjectList.IsEmpty()) {
            ObjectstoAcquire* pObject = ObjectList.m_item;
            delete pObject;
            ObjectList.Remove(pObject);
        }
        if (bRedress) {
            TESNPC* pNPC = dynamic_cast<TESNPC*>(apActor->GetObjectReference());
            TESCreature* pCreature = dynamic_cast<TESCreature*>(apActor->GetObjectReference());
            TESPackage* pPack = GetCurrentPackage();
            bool bWearWeapon = true;
            if (pPack) {
                bWearWeapon = !pPack->GetForceUnequipWeapon();
            }
            if (pNPC) {
                pNPC->InitDefaultWorn(apActor, true, bWearWeapon, false, true);
            }
            else if (pCreature) {
                pCreature->InitDefaultWorn(apActor, true, bWearWeapon, true);
            }
        }
    }
    return bNewPackage;
}

MagicItem* MiddleHighProcess::GetCurrentPackageSpell() {
    return pCurrentPackageSpell;
}

void MiddleHighProcess::SetCurrentPackageSpell(MagicItem* apSpell) {
    pCurrentPackageSpell = apSpell;
}

void MiddleHighProcess::ClearFaceAnimationData() {
    m_pFaceAnimationData = nullptr;
}

bool MiddleHighProcess::GetIdleDoneOnce() {
    return bDoneOnce;
}

void MiddleHighProcess::SetIdleDoneOnce(bool abflag) {
    bDoneOnce = abflag;
}

void MiddleHighProcess::ClearFurniture(Actor* apActor) {
    if (pCurrentFurniture) {
        pCurrentFurniture->SetMarkerUsed(cCurrentFurnitureIndex, false);
        if (apActor->GetAnimation()) {
            apActor->GetAnimation()->SpecialIdleFree(true, false);
        }
    }
    SetSitSleepState(apActor, SIT_SLEEP_NORMAL, nullptr, 0x7Fu);
    if (static_cast<const void*>(apActor) == static_cast<const void*>(pPlayer)) {
        pPlayer->ClearSitHeadingDelta();
    }
    ClearFurnitureMarker();
}

bool MiddleHighProcess::SetTargetForPackage(Actor* apActor) {
    SetTarget(nullptr);
    TESPackage* pPack = GetPackageThatIsRunning();
    if (!pPack ||
        (!pPack->GetPackageTarget() &&
         pPack->GetPackType() != PACKAGE_TYPE_SANDBOX &&
         pPack->GetPackType() != PACKAGE_TYPE_PATROL)) {
        return false;
    }
    SetTarget(GetTargetForPackage(apActor, pPack));
    if (!pTarget) {
        return false;
    }
    if (pTarget->IsStillLoading()) {
        SetTarget(nullptr);
        return false;
    }

    if (pTarget->GetDelete()) {
        if (pTarget->TESObjectREFR::GetExtra()->GetReferencePointer()) {
            SetTarget(pTarget->TESObjectREFR::GetExtra()->GetReferencePointer());
        }
    }
    lastSeenPostion = pTarget->GetLocationOnReference();
    pTarget->SetTargeted(true);
    return true;
}

TESObjectREFR* MiddleHighProcess::SetLocationForPackage(Actor* apActor, bool bSecondLoc) {
    TESPackage* pPack = GetCurrentPackage();
    if (!pPack) {
        return nullptr;
    }
    TESObjectREFR* pLocation = GetLocationForPackage(apActor, pPack, bSecondLoc);
    if (bSecondLoc) {
        pGenericSecondLocation = pLocation;
    }
    else {
        pGenericLocation = pLocation;
    }
    return pLocation;
}

bool MiddleHighProcess::GetPickpocketed() {
    return bPickPocketed;
}

void MiddleHighProcess::SetPickpocketed(bool abflag) {
    bPickPocketed = abflag;
}

// A new target drops the old target's furniture reservation and becomes the running package's target; a new
// target of a package that reserves one gets the first free marker reserved. A null target ends the move message.
void MiddleHighProcess::SetTarget(TESObjectREFR* apRef) {
    bool bChanged = apRef != pTarget;
    BaseProcess* pProcess = this;
    if (bChanged) {
        if (pTarget) {
            ClearReservation();
        }
        pTarget = apRef;
        pProcess->GetActorPackageThatIsRunning()->SetTarget(apRef);
    }
    if (apRef) {
        apRef->SetTargeted(true);
        if (bChanged) {
            TESPackage* pPackage = pProcess->GetCurrentPackage();
            if (pPackage && pPackage->GetShouldReserveTarget()) {
                iReservationSlot = apRef->GetFirstFreeMarkerIndex(true);
                apRef->SetMarkerReserved(iReservationSlot, true);
            }
        }
    }
    else {
        pProcess->EndMoveMessage(nullptr);
    }
}

void MiddleHighProcess::SetAcquireObject(ObjectstoAcquire* apacquire) {
    pAcquireObject = apacquire;
}

void MiddleHighProcess::SetSavedAcquireObject(ObjectstoAcquire* apobject) {
    pSavedAcquireObject = apobject;
}

BSFaceGenAnimationData* MiddleHighProcess::GetFaceAnimationData(Actor* apActor) {
    if (!apActor->Get3D()) {
        m_pFaceAnimationData = nullptr;
        return nullptr;
    }
    if (!m_pFaceAnimationData) {
        m_pFaceAnimationData = apActor->TESObjectREFR::GetFaceAnimationData(nullptr);
    }
    return m_pFaceAnimationData;
}

ItemChange* MiddleHighProcess::GetCurrentWeapon() {
    return pWeapon;
}

ItemChange* MiddleHighProcess::GetCurrentAmmo() {
    return pAmmo;
}

// Equips apWeapon: clears the old weapon's gun wobble, deletes the old item change, precaches the new weapon's sounds
// (the silenced ones with the silencer mod or WEAPON_MOD_EFFECT_COUNT, re-tested per sound) and queues its models, then
// stores the bone pointers of ap3D. Every getter is re-called as written below.
bool MiddleHighProcess::SetCurrentWeapon(ItemChange* apweap, NiNode* ap3D, bool bForce) {
    uint8_t aMemContext[sizeof(AutoMemContext)];
    AutoMemContext* pMemContext = ::new (aMemContext) AutoMemContext(
        MEM_CONTEXT_TES_ACTOR,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp",
        4273
    );
    BaseProcess* pProcess = this;
    if (pWeapon != apweap || (apweap && bForce)) {
        if (pWeapon && pWeapon->GetContainerObject() && pAnimation) {
            if (pAnimation == pPlayer->GetAnimation(false)) {
                NiNode* pRoot = static_cast<NiNode*>(pPlayer->Get3D(true));
                ClearGunWobble(
                    weaponToAnimGroupWeapon[static_cast<TESObjectWEAP*>(pWeapon->GetContainerObject())
                                                ->GetWeaponType()],
                    pRoot
                );
                pRoot = static_cast<NiNode*>(pPlayer->Get3D(false));
                ClearGunWobble(
                    weaponToAnimGroupWeapon[static_cast<TESObjectWEAP*>(pWeapon->GetContainerObject())
                                                ->GetWeaponType()],
                    pRoot
                );
            }
            else if (pAnimation->GetSceneRoot()) {
                NiNode* pRoot = pAnimation->GetSceneRoot();
                ClearGunWobble(
                    weaponToAnimGroupWeapon[static_cast<TESObjectWEAP*>(pWeapon->GetContainerObject())
                                                ->GetWeaponType()],
                    pRoot
                );
            }
            else {
                ClearGunWobble(
                    weaponToAnimGroupWeapon[static_cast<TESObjectWEAP*>(pWeapon->GetContainerObject())
                                                ->GetWeaponType()],
                    ap3D
                );
            }
        }
        if (pWeapon != apweap) {
            ItemChange* pOldWeapon = pWeapon;
            if (pOldWeapon) {
                delete pOldWeapon;
            }
        }
        pWeapon = apweap;
        pFireNode = nullptr;
        SetWeaponFlags();
        TESObjectWEAP* pWeaponObject = pWeapon ? static_cast<TESObjectWEAP*>(pWeapon->GetContainerObject()) : nullptr;
        if (pWeaponObject) {
            bool bSilenced = pWeapon->HasModEffectActive(WEAPON_MOD_SILENCE) ||
                pWeapon->HasModEffectActive(WEAPON_MOD_EFFECT_COUNT);
            if (pWeaponObject->GetAttackSound(bSilenced)) {
                bSilenced = pWeapon->HasModEffectActive(WEAPON_MOD_SILENCE) ||
                    pWeapon->HasModEffectActive(WEAPON_MOD_EFFECT_COUNT);
                TESSound* pSettings = pWeaponObject->GetAttackSound(false);
                const char* pFilename = pWeaponObject->GetAttackSound(bSilenced)->GetFilename();
                BSAudio::QInstance().PrecacheSound(
                    pFilename,
                    BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT,
                    pSettings
                );
            }
            bSilenced = pWeapon->HasModEffectActive(WEAPON_MOD_SILENCE) ||
                pWeapon->HasModEffectActive(WEAPON_MOD_EFFECT_COUNT);
            if (pWeaponObject->GetAttackSound2(bSilenced)) {
                bSilenced = pWeapon->HasModEffectActive(WEAPON_MOD_SILENCE) ||
                    pWeapon->HasModEffectActive(WEAPON_MOD_EFFECT_COUNT);
                TESSound* pSettings = pWeaponObject->GetAttackSound2(false);
                const char* pFilename = pWeaponObject->GetAttackSound2(bSilenced)->GetFilename();
                BSAudio::QInstance().PrecacheSound(
                    pFilename,
                    BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT,
                    pSettings
                );
            }
            bSilenced = pWeapon->HasModEffectActive(WEAPON_MOD_SILENCE) ||
                pWeapon->HasModEffectActive(WEAPON_MOD_EFFECT_COUNT);
            if (pWeaponObject->GetAttackSound2D(bSilenced)) {
                bSilenced = pWeapon->HasModEffectActive(WEAPON_MOD_SILENCE) ||
                    pWeapon->HasModEffectActive(WEAPON_MOD_EFFECT_COUNT);
                TESSound* pSettings = pWeaponObject->GetAttackSound2D(false);
                const char* pFilename = pWeaponObject->GetAttackSound2D(bSilenced)->GetFilename();
                BSAudio::QInstance().PrecacheSound(
                    pFilename,
                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT,
                    pSettings
                );
            }
            if (pWeaponObject->GetAttackFailSound()) {
                TESSound* pSettings = pWeaponObject->GetAttackFailSound();
                const char* pFilename = pWeaponObject->GetAttackFailSound()->GetFilename();
                BSAudio::QInstance().PrecacheSound(
                    pFilename,
                    BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT,
                    pSettings
                );
            }
            QueuedFile* pPreload = new QueuedFile(IO_TASK_PRIORITY_VERY_LOW);
            spWeaponPreload = pPreload;
            bool bPlayerProcess = false;
            if (pPlayer && pPlayer->GetCurrentAIProcess() == pProcess) {
                bPlayerProcess = true;
            }
            IOTask* pParent = spWeaponPreload;
            pWeaponObject->QueueFiles(IO_TASK_PRIORITY_VERY_LOW, static_cast<QueuedFile*>(pParent), bPlayerProcess);
        }
        else {
            spWeaponPreload = nullptr;
        }
        spReserveProjectileClones = nullptr;
        if (pPlayer && pPlayer->GetCurrentAIProcess() == pProcess) {
            pPlayer->SetShotsSinceLastAmmoRegenTick(0);
        }
    }
    if (!pAnimation || !ap3D) {
        pWeaponBone = nullptr;
        pFireNode = nullptr;
        if (pPlayer && pPlayer->GetCurrentAIProcess() == pProcess) {
            pThirdPersonPlayerWeaponNode = nullptr;
        }
    }
    else {
        if (pAnimation->GetSceneRoot()) {
            NiNode* pRoot = pAnimation->GetSceneRoot();
            NiControllerManager* pManager = pAnimation->GetControllerManager();
            pProcess->StoreBonePointers(pManager, pRoot);
        }
        else {
            pProcess->StoreBonePointers(nullptr, ap3D);
        }
        if (pAnimation == pPlayer->GetAnimation(false)) {
            NiNode* pRoot = pPlayer->GetAnimation(true)->GetSceneRoot();
            NiControllerManager* pManager = pPlayer->GetAnimation(true)->GetControllerManager();
            pProcess->StoreBonePointers(pManager, pRoot);
        }
    }
    pProcess->SetWeaponConditionStage(static_cast<uint32_t>(-1));
    pMemContext->~AutoMemContext();
    return true;
}

bool MiddleHighProcess::SetCurrentAmmo(ItemChange* apammo) {
    if (pAmmo) {
        delete pAmmo;
    }
    pAmmo = apammo;
    return true;
}

// Runs the queued equips and unequips whose model has finished loading, in list order; a done item is freed and
// leaves the list. Equipping a grenade, mine or thrown weapon equips it a second time as ammo. Unequipping loads
// the hand-to-hand idles when the animation lacks them. An emptied list is freed.
void MiddleHighProcess::ResolveEquipmentQueue(Actor* apActor) {
    if (!pListItemstoEquipUnequip) {
        return;
    }
    BSSimpleList<QueuedItem*>* pIter = pListItemstoEquipUnequip;
    while (pIter && pIter->GetItem()) {
        QueuedItem* pQueued = pIter->GetItem();
        bool bDone = false;
        if (pQueued && (!pQueued->spQueuedFiles || pQueued->spQueuedFiles->GetAllChildrenFinished())) {
            bDone = true;
            if (pQueued->bEquip) {
                if (apActor->GetAnimAction() == ANIM_ACTION_BLOCK && !apActor->GetCurrentWeapon()) {
                    apActor->SetBlock(false);
                }
                apActor->EquipObject(
                    pQueued->pObject,
                    pQueued->iNumber,
                    pQueued->pExtra,
                    pQueued->bNow,
                    pQueued->bForceEquip,
                    pQueued->bPlayPickUpSounds
                );
                if (pQueued->pObject->GetFormType() == WEAP_ID) {
                    TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(pQueued->pObject);
                    if (pWeapon->GetWeaponType() == WEAPON_ONE_HAND_GRENADE ||
                        pWeapon->GetWeaponType() == WEAPON_ONE_HAND_MINE ||
                        pWeapon->GetWeaponType() == WEAPON_ONE_HAND_LUNCHBOX_MINE ||
                        pWeapon->GetWeaponType() == WEAPON_ONE_HAND_THROWN) {
                        pQueued->pObject->SetFormType(AMMO_ID);
                        apActor->EquipObject(
                            pQueued->pObject,
                            pQueued->iNumber,
                            pQueued->pExtra,
                            pQueued->bNow,
                            pQueued->bForceEquip,
                            false
                        );
                        pQueued->pObject->SetFormType(WEAP_ID);
                    }
                }
            }
            else {
                apActor->UnEquipObject(
                    pQueued->pObject,
                    pQueued->iNumber,
                    pQueued->pExtra,
                    pQueued->bForceUnequip,
                    pQueued->bEquipCalling,
                    pQueued->bPlayPickUpSounds
                );
                Animation* pAnim = GetAnimation();
                if (pAnim &&
                    !pAnim->GroupLoaded(
                        TESAnimGroup::AnimGroup(AGM_NONE, AGW_HAND_TO_HAND, ANIM_GROUP_WEAPONS_START, false)
                    )) {
                    pAnim->AddAnimationsFromList(
                        pModelLoader->BuildKFFileList(apActor->GetModel(), true, false, AGW_HAND_TO_HAND)
                    );
                }
            }
        }
        if (bDone) {
            if (pQueued) {
                delete pQueued;
            }
            pIter->RemoveHead();
        }
        else {
            pIter = pIter->GetNext();
        }
    }
    if (!pListItemstoEquipUnequip->GetItem()) {
        pListItemstoEquipUnequip->RemoveAll();
        if (pListItemstoEquipUnequip) {
            delete pListItemstoEquipUnequip;
        }
        pListItemstoEquipUnequip = nullptr;
    }
}

bool MiddleHighProcess::IsItemQueued(TESBoundObject* apObject, ExtraDataList* apExtra, bool abEquipped) {
    bool bFound = false;
    for (BSSimpleList<QueuedItem*>* pList = pListItemstoEquipUnequip; pList && pList->m_item && !bFound;
         pList = pList->m_pkNext) {
        QueuedItem* pItem = pList->m_item;
        if (pItem && pItem->bEquip == abEquipped && pItem->pObject == apObject) {
            bFound = apExtra == pItem->pExtra;
        }
    }
    return bFound;
}

void MiddleHighProcess::AddQueuedItem(
    Actor* apActor,
    bool abEquip,
    TESBoundObject* apObject,
    int32_t aiNumber,
    ExtraDataList* apExtra,
    bool abNow,
    bool abForceEquip,
    bool abFromMenus,
    bool abForceUnequip,
    bool abEquipCalling,
    bool abPlayPickUpSounds
) {
    AutoMemContext kMemContext(
        MEM_CONTEXT_TES_ACTOR,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp",
        835
    );
    if (!IsItemQueued(apObject, apExtra, abEquip)) {
        QueuedItem* pItem = new QueuedItem;
        pItem->pObject = apObject;
        pItem->bEquip = abEquip;
        pItem->iNumber = aiNumber;
        pItem->pExtra = apExtra;
        pItem->bNow = abNow;
        pItem->bForceEquip = abForceEquip;
        pItem->bForceUnequip = abForceUnequip;
        pItem->bEquipCalling = abEquipCalling;
        pItem->bPlayPickUpSounds = abPlayPickUpSounds;
        pItem->QueueFiles(apActor);
        if (!pListItemstoEquipUnequip) {
            pListItemstoEquipUnequip = new BSSimpleList<QueuedItem*>;
        }
        pListItemstoEquipUnequip->AddTail(pItem);
    }
}

void MiddleHighProcess::RemoveQueuedItem(TESBoundObject* apObject, ExtraDataList* apExtra) {
    for (QueuedItem* pItem : pListItemstoEquipUnequip->ItemsUntilNull()) {
        if (pItem && pItem->pObject == apObject && apExtra == pItem->pExtra) {
            if (pItem->spQueuedFiles) {
                pIOManager->TryCancelTask(pItem->spQueuedFiles, nullptr);
            }
            delete pItem;
            pListItemstoEquipUnequip->Remove(pItem);
            return;
        }
    }
}

void MiddleHighProcess::RemoveAllQueuedItems() {
    for (BSSimpleList<QueuedItem*>* pList = pListItemstoEquipUnequip; pList && pList->m_item; pList->RemoveHead()) {
        QueuedItem* pItem = pList->m_item;
        if (pItem) {
            if (pItem->spQueuedFiles) {
                pIOManager->TryCancelTask(pItem->spQueuedFiles, nullptr);
            }
            delete pItem;
        }
    }
    if (pListItemstoEquipUnequip) {
        delete pListItemstoEquipUnequip;
    }
    pListItemstoEquipUnequip = nullptr;
}

void MiddleHighProcess::SetFireNode(NiAVObject* apFireNode) {
    pFireNode = apFireNode;
}

void MiddleHighProcess::StoreBonePointers(NiControllerManager* apManager, NiNode* ap3D) {
    bool b3rdPerson = false;
    if (apManager) {
        if (pPlayer->GetAnimation(true) && apManager == pPlayer->GetAnimation(true)->GetControllerManager()) {
            b3rdPerson = true;
            NiAVObjectPalette* pPalette = apManager->GetObjectPalette();
            NiFixedString kBoneName(bipedBoneName[1]);
            MiddleHighProcess::pThirdPersonPlayerWeaponNode = static_cast<NiNode*>(pPalette->GetAVObject(kBoneName));
        }
        else {
            NiAVObjectPalette* pPalette = apManager->GetObjectPalette();
            NiFixedString kBoneName(bipedBoneName[1]);
            pWeaponBone = static_cast<NiNode*>(pPalette->GetAVObject(kBoneName));
        }
    }
    else if (ap3D) {
        if (pPlayer->GetAnimation(true) && ap3D == pPlayer->GetAnimation(true)->GetSceneRoot()) {
            b3rdPerson = true;
            MiddleHighProcess::pThirdPersonPlayerWeaponNode = static_cast<NiNode*>(
                BSUtilities::GetObjectByName(static_cast<NiAVObject*>(ap3D), bipedBoneName[1])
            );
        }
        else {
            pWeaponBone = static_cast<NiNode*>(
                BSUtilities::GetObjectByName(static_cast<NiAVObject*>(ap3D), bipedBoneName[1])
            );
        }
    }
    if (b3rdPerson) {
        RemoveScabard(MiddleHighProcess::pThirdPersonPlayerWeaponNode);
    }
    else {
        RemoveScabard(pWeaponBone);
    }
}

NiNode* MiddleHighProcess::GetWeaponBone(BipedAnim* apBiped) {
    if (apBiped) {
        return apBiped->GetParentBone(BIPED_BONE_WEAPON);
    }
    return pWeaponBone;
}

NiNode* MiddleHighProcess::GetRightWeaponBone(BipedAnim* apBiped) {
    if (apBiped) {
        return nullptr;
    }
    return pWeaponBone;
}

bool MiddleHighProcess::IsCurrentWeaponGrenade() {
    return bWeaponGrenade;
}

bool MiddleHighProcess::IsCurrentWeaponMine() {
    return bWeaponMine;
}

bool MiddleHighProcess::IsCurrentWeaponGrenadeOrMine() {
    return bWeaponGrenade | bWeaponMine | bWeaponThrown;
}

bool MiddleHighProcess::IsCurrentWeaponThrown() {
    return bWeaponThrown;
}

Animation* MiddleHighProcess::GetAnimation() {
    return pAnimation;
}

void MiddleHighProcess::ForceWeaponDrawnSheathed(bool abDrawn, BipedAnim* apBiped, Animation* apAnim, Actor* apActor) {
    ForceBoneScale();
    NiNode* pBone = GetWeaponBone(apBiped);
    // The biped's root, else the actor's 3D.
    NiNode* p3D = nullptr;
    if (apBiped) {
        p3D = apBiped->root;
    }
    if (!p3D) {
        p3D = apActor->Get3D();
    }
    if (pBone) {
        NiAVObject* pObject = pBone->GetAtChecked(0);
        if (pObject && abDrawn) {
            pObject->SetAppCulled(false);
        }

        if (apActor == pPlayer) {
            ReparentWeapon(abDrawn, apBiped, apAnim, apActor);
        }
        else if (GetWantWeaponDrawn() || GetCurrentWeapon() || GetWeaponDrawn()) {
            bNeedReparentWeapon = true;
            bDrawn = abDrawn;
        }
        else if (bDrawn != abDrawn && !pSaveLoadGame->GetSaveGameLoading()) {
            // pweap is never set, so the reparent below never runs.
            TESObjectWEAP* pweap = nullptr;
            if (pweap && !pweap->IsEmbeddedWeapon()) {
                bNeedReparentWeapon = true;
                bDrawn = abDrawn;
            }
        }
        if (p3D) {
            NiAVObject* pRound = BSUtilities::GetObjectByName(p3D, "##NifRound");
            if (pRound) {
                if (pAmmo && pAmmo->iNumber) {
                    pRound->SetAppCulled(false);
                }
                else {
                    pRound->SetAppCulled(true);
                }
            }
            NiAVObject* pLight = BSUtilities::GetObjectByName(p3D, "##LightOn");
            if (pLight) {
                pLight->SetAppCulled(true);
            }
        }
    }
    else if (this->pWeapon && pSaveLoadGame->GetSaveGameLoading()) {
        TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(this->pWeapon->pContainerObj);
        if (pWeapon && !pWeapon->IsEmbeddedWeapon()) {
            bNeedReparentWeapon = true;
            bDrawn = abDrawn;
        }
    }
    else if (this->pWeapon) {
        TESObjectWEAP* pWeaponForm = static_cast<TESObjectWEAP*>(this->pWeapon->pContainerObj);
        if (pWeaponForm && !pWeaponForm->IsEmbeddedWeapon()) {
            bNeedReparentWeapon = true;
            bDrawn = abDrawn;
        }
    }
    if (apAnim && apAnim->pAccumRoot && !Interface::IsInPipboyMenu() && apAnim->GetGroup(AGS_IDLE) != 1) {
        if (apActor == pPlayer) {
            ItemChange* pWeaponItem;
            bool bNoWeapon = abDrawn && this->pWeapon == nullptr;
            if (abDrawn) {
                pWeaponItem = this->pWeapon;
            }
            else {
                // -1 is GetAnimGroup's "no weapon" value.
                pWeaponItem = reinterpret_cast<ItemChange*>(static_cast<uintptr_t>(-1));
            }
            uint16_t usAnimGroup = apActor->GetAnimGroup(ANIM_GROUP_IDLE, pWeaponItem, bNoWeapon, apAnim);
            apAnim->PlayGroup(usAnimGroup, ACTION_START, -1, AGS_NONE);
        }
        else {
            AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_EQUIP_UNEQUIP_WEAPON);
        }
    }

    if (abDrawn &&
        GetCurrentWeapon() &&
        apAnim &&
        apAnim->pAccumRoot &&
        (!Interface::IsInPipboyMenu() || apAnim == pPlayer->GetAnimation(false))) {
        ItemChange* pCurrentWeapon = GetCurrentWeapon();
        ANIM_GROUP_ENUM eHandGripAnim = static_cast<ANIM_GROUP_ENUM>(
            static_cast<TESObjectWEAP*>(pCurrentWeapon->pContainerObj)->TESObjectWEAP::GetHandGripAnim()
        );
        if (eHandGripAnim != ANIM_GROUP_NONE) {
            if (apActor == pPlayer) {
                uint16_t usAnimGroup = apActor->GetAnimGroup(eHandGripAnim, this->pWeapon, false, apAnim);
                apAnim->PlayGroup(usAnimGroup, ACTION_START, -1, AGS_NONE);
            }
            else {
                AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_HAND_GRIP_WEAPON);
            }
        }
    }
    else if ((!abDrawn || !GetCurrentWeapon()) && apAnim && apAnim->GetCurrentSequence(AGS_LEFT_ARM)) {
        ANIM_GROUP_ENUM eType = apAnim->GetCurrentSequence(AGS_LEFT_ARM)->GetAnimGroup()->GetType();
        if (eType >= ANIM_GROUP_HAND_GRIP_1 && eType <= ANIM_GROUP_HAND_GRIP_6) {
            apAnim->BlendOut(AGS_LEFT_ARM, false);
        }
    }
    if (pSaveLoadGame->GetSaveGameLoading() || apActor == pPlayer) {
        apActor->SetHavokWeapon();
    }
    else {
        AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_HAVOKWEAPON);
    }
    TESObjectWEAP* pWeaponForm = nullptr;
    if (this->pWeapon) {
        pWeaponForm = static_cast<TESObjectWEAP*>(this->pWeapon->pContainerObj);
    }
    if (pWeaponForm && pWeaponForm->GetIdleSound()) {
        if (pWeaponForm->GetIdleSound()->GetFilename()) {
            BSSoundHandle hWeapIdle;
            apActor->m_Extra.GetWeaponIdleSound(hWeapIdle);
            if (!hWeapIdle.IsValid()) {
                uint32_t uiLoopFlags = TESSound::ConvertLoopFlag(pWeaponForm->GetIdleSound()->GetData().iFlags);
                uint32_t uiPlayerFlag = pPlayer != apActor
                    ? 0
                    : static_cast<BSGameSound::SoundFlags>(
                          BSGameSound::SOUND_FLAG_FIRST_PERSON | BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE
                      );
                bool bActually3rdPerson = pPlayer->IsActually3rdPerson();
                uiPlayerFlag ^= bActually3rdPerson ? BSGameSound::SOUND_FLAG_FIRST_PERSON : 0;
                TESSound* pSettings = pWeaponForm->GetIdleSound();
                uint32_t uiFlags = uiPlayerFlag | (uiLoopFlags | BSGameSound::SOUND_FLAG_IS_3D);
                const char* pFilename = pWeaponForm->GetIdleSound()->GetFilename();
                hWeapIdle = BSAudio::QInstance().GetSoundHandleByFilename(pFilename, uiFlags, pSettings);
                hWeapIdle.bAssumeSuccess = true;
                hWeapIdle.SetObjectToFollow(apActor->Get3D());
                hWeapIdle.SetPosition(apActor->GetLocationOnReference());
            }
            if (abDrawn && !hWeapIdle.IsPlaying()) {
                hWeapIdle.Play(false);
            }
            else if (!abDrawn && hWeapIdle.IsPlaying()) {
                hWeapIdle.Stop();
            }
            apActor->m_Extra.SetWeaponIdleSound(hWeapIdle);
        }
    }

    apActor->SetWalkRunFlagDirty();
}

void MiddleHighProcess::UpdateReparentWeapon(Actor* apActor) {
    if (bNeedReparentWeapon && apActor->Get3D()) {
        if (pAnimation && pWeapon) {
            TESObjectWEAP* pWeaponForm = static_cast<TESObjectWEAP*>(pWeapon->ItemChange::GetItemChangeObject());
            uint32_t uiWeaponGroup = weaponToAnimGroupWeapon[pWeaponForm->GetWeaponType()];
            if (!pAnimation->GetAnimsLoading() && pAnimation->GetControllerManager()) {
                uint16_t usAnimGroup = TESAnimGroup::AnimGroup(
                    AGM_NONE,
                    static_cast<ANIM_GROUP_WEAPON_ENUM>(uiWeaponGroup),
                    ANIM_GROUP_WEAPONS_START,
                    false
                );
                if (!pAnimation->GroupLoaded(usAnimGroup) && !apActor->IsDead(true)) {
                    const char* pModel = apActor->GetModel();
                    BSSimpleList<const char*>* pFileList = pModelLoader->BuildKFFileList(
                        pModel,
                        true,
                        false,
                        static_cast<ANIM_GROUP_WEAPON_ENUM>(uiWeaponGroup)
                    );
                    pAnimation->AddAnimationsFromList(pFileList);
                }
            }
        }

        if (!pAnimation || !pAnimation->GetAnimsLoading()) {
            if (apActor->TESObjectREFR::IsDead(false)) {
                apActor->RemoveWeapon();
            }
            else {
                Animation* pAnim = apActor->GetAnimation();
                BipedAnim* pBiped = apActor->GetBiped();
                ReparentWeapon(bWeaponDrawn, pBiped, pAnim, apActor);
            }
            bNeedReparentWeapon = false;
            bDrawn = false;
        }
    }
}

TESPackage* MiddleHighProcess::GetRunOncePackage() {
    return RunOncePackage.pPackage;
}

void MiddleHighProcess::SetRunOncePackage(TESPackage* apPack, MobileObject* apActor) {
    if (apPack) {
        TESObjectREFR* pInitialTarget = apPack->ComputeInitialTarget(apActor);
        RunOncePackage.InitFromPackage(apPack, pInitialTarget);
    }
    else {
        ClearRunOncePackage();
        SetTarget(nullptr);
    }
    ClearMovementStoped();
}

void MiddleHighProcess::ClearRunOncePackage() {
    uint8_t ucHadPackage = GetRunOncePackage() != nullptr;
    RunOncePackage.Clear();
    if (ucHadPackage) {
        SetupNewPackage();
    }
}

void MiddleHighProcess::SetRunOnceProcedureIndex(int32_t aiNewIndex) {
    RunOncePackage.iCurrentProcedureIndex = aiNewIndex;
}

int32_t MiddleHighProcess::GetRunOnceProcedureIndex() {
    return RunOncePackage.iCurrentProcedureIndex;
}

void MiddleHighProcess::AddToRunOnceProcedureIndex(int32_t aiOffset) {
    RunOncePackage.AddToCurrentProcedureIndex(aiOffset);
}

// Done without a running package; never for a package without a procedure; else when the running step is
// PROCEDURE_DONE.
bool MiddleHighProcess::IsPackageDone(Actor*) {
    BaseProcess* pProcess = this;
    TESPackage* pPackage = pProcess->GetPackageThatIsRunning();
    if (!pPackage) {
        return true;
    }
    if (pPackage->GetProcedureType() == PACKAGE_PROCEDURE_NONE) {
        return false;
    }
    PACKAGE_PROCEDURE_TYPE eType = pPackage->GetProcedureType();
    return Procedure::Action[eType][pProcess->GetProcedureIndexRunning()] == PROCEDURE_DONE;
}

void MiddleHighProcess::SetupNewPackage() {
    MiddleHighProcess::ForceBoneScale();
    pItemBeingUsed = nullptr;
    iAnimActionSuccess = 2;
    SetContinuingPackage(false);
    SetCurrentActionComplete(false);
    SetTargetActivated(false);
    LowProcess::SetPackageIdleDone(false);
    SetPackageDoneOnce(false);
    SetTarget(nullptr);
    ClearAcquireList();
    ClearReservation();
}

ActorPackageData* MiddleHighProcess::GetRunOncePackageInstanceData() {
    return RunOncePackage.pData;
}

ActorPackageData* MiddleHighProcess::GetInstanceDataThatIsRunning() {
    if (GetRunOncePackage()) {
        return GetRunOncePackageInstanceData();
    }

    return BaseProcess::GetInstanceDataThatIsRunning();
}

ActorPackage* MiddleHighProcess::GetActorPackageThatIsRunning() {
    if (GetRunOncePackage()) {
        return &RunOncePackage;
    }

    return &CurrentPackage;
}

TESPackage* MiddleHighProcess::GetPackageThatIsRunning() {
    if (GetRunOncePackage()) {
        return GetRunOncePackage();
    }
    return GetCurrentPackage();
}

int32_t MiddleHighProcess::GetProcedureIndexRunning() {
    if (GetRunOncePackage()) {
        return GetRunOnceProcedureIndex();
    }
    return GetCurrentProcedureIndex();
}

// Sets the run-once or the current procedure index, then clamps the running index to the last step of the running
// package's procedure (a signed compare).
void MiddleHighProcess::SetProcedureIndexRunning(int32_t ainumb) {
    BaseProcess* pProcess = this;
    TESPackage* pPackage = pProcess->GetPackageThatIsRunning();
    if (pProcess->GetRunOncePackage()) {
        pProcess->SetRunOnceProcedureIndex(ainumb);
    }
    else {
        pProcess->SetCurrentProcedureIndex(ainumb);
    }
    if (pPackage) {
        int32_t iCount = Procedure::GetProcedureCount(pPackage->GetProcedureType());
        if (pProcess->GetProcedureIndexRunning() >= iCount) {
            pProcess->SetProcedureIndexRunning(iCount - 1);
        }
    }
}

void MiddleHighProcess::AddToProcedureIndexRunning(MobileObject* apActor, int32_t ainumb) {
    TESPackage* pPack = GetPackageThatIsRunning();
    SetAnimationActiveFlag(false);
    if (GetRunOncePackage()) {
        AddToRunOnceProcedureIndex(ainumb);
    }
    else {
        AddToCurrentProcedureIndex(ainumb);
    }
    if (GetProcedureIndexRunning() < 0) {
        SetProcedureIndexRunning(0);
    }
    if (pPack && pPack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_NONE) {
        int32_t iRange = Procedure::GetProcedureCount(pPack->ePROCEDURE_TYPE);
        if (GetProcedureIndexRunning() >= iRange) {
            SetProcedureIndexRunning(iRange - 1);
        }
        uint32_t uiProcedureType = pPack->ePROCEDURE_TYPE;
        if (Procedure::Action[uiProcedureType][GetProcedureIndexRunning()] == PROCEDURE_DONE && !IsPackageDoneOnce()) {
            Script::SetActionFlag(pPack, &apActor->m_Extra, SCRIPT_ACTION_ONPACKAGEEND);
            PackageDone(static_cast<Actor*>(apActor), pPack);
            SetPackageDoneOnce(true);
        }
    }
}

bhkCharacterController* MiddleHighProcess::GetCharController() {
    return spCharController.m_pObject;
}

void MiddleHighProcess::EndMoveMessage(Actor* apActor) {
    if (apActor) {
        apActor->StopMoving();
    }
}

void MiddleHighProcess::ProcessFollow(Actor* apActor, bool abflag, int32_t emovemode, bool abpursuit) {
    TESPackage* pPack = GetPackageThatIsRunning();
    if (pPack && pPack->GetWaiting()) {
        if (pPlayer->IsPlayerFollower(apActor)) {
            return;
        }
        int32_t iNumFollowers = pPlayer->GetNumberOfFollowers();
        if (iNumFollowers >= iNumberActorsAllowedToFollowPlayer.Int()) {
            return;
        }
        pPack->SetWaiting(false);
    }
    if (!pTarget || pTarget->GetDelete()) {
        SetTargetForPackage(apActor);
        if (pTarget) {
            lastSeenPostion = pTarget->GetLocationOnReference();
        }
    }
    if (!pTarget) {
        if (abflag) {
            AddToProcedureIndexRunning(apActor, 1);
        }
        return;
    }
    float fCurrentDistance = apActor->GetDistanceFromReference(pTarget, false, false);
    apActor->GetWorldSpace();
    pTarget->GetWorldSpace();
    float fDistance = pPack->GetPackageRadiusActorToRefTarget(apActor, false);
    TESObjectREFR* pFollowTarget = pTarget;
    if (!pTarget ||
        (pPack->GetPackType() == PACKAGE_TYPE_FOLLOW &&
         pPack->pPackLoc &&
         pPack->pPackLoc->GetLocType() < PLT_CURRLOC &&
         pPack->IsActorAtLocation(apActor, false, -1.0f, false))) {
        if (abflag) {
            AddToProcedureIndexRunning(apActor, 1);
            if (pPack->GetIsCreated()) {
                if (GetRunOncePackage()) {
                    ClearRunOncePackage();
                }
                else {
                    ClearCurrentPackage();
                }
                CheckforNewPackage(apActor, false);
                return;
            }
            if (!apActor->IsPathingComplete()) {
                EndMoveMessage(apActor);
            }
        }
    }
    else {
        float fDistanceMoved = (lastSeenPostion - pTarget->GetLocationOnReference()).Length();
        if (fAIMoveDistanceToRecalcFollowPath.Float() < static_cast<double>(fDistanceMoved) ||
            (fDistance < static_cast<double>(fCurrentDistance) && apActor->IsPathingComplete())) {
            apActor->SetMoveMode(static_cast<uint32_t>(emovemode));
            TESWorldSpace* pWorld = pTarget->GetWorldSpace();
            TESObjectCELL* pCell = pTarget->GetParentCell();
            if (!SetupPathFinding(apActor, pTarget->GetLocationOnReference(), pCell, pWorld)) {
                return;
            }
            lastSeenPostion = pTarget->GetLocationOnReference();
        }
    }
    fCurrentDistance = apActor->GetDistanceFromReference(pFollowTarget, false, false);
    if (fDistance > static_cast<double>(fCurrentDistance) && !apActor->IsPathingComplete()) {
        apActor->EndMovement();
    }
}

void MiddleHighProcess::ProcessPursue(Actor* apActor, bool abNewpackage, bool abInterateIndex, int32_t emovemode) {
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    if (pTarget == pPlayer && pPlayer->GetControlsDisabled(PlayerCharacter::DPC_FIGHTING)) {
        EndMoveMessage(apActor);
        return;
    }
    TrespassPackage* pTresPack = dynamic_cast<TrespassPackage*>(GetPackageThatIsRunning());
    Actor* pTargetAsActor = static_cast<Actor*>(pTarget);
    if (pTresPack && pTargetAsActor && !pTargetAsActor->IsTresPassing()) {
        AddToProcedureIndexRunning(pTargetAsActor, 2);
        return;
    }
    if (apActor->GetDistanceFromReference(pTarget, false, false) > 200.0) {
        ProcessFollow(apActor, false, emovemode, true);
        return;
    }
    if (!apActor->IsPathingComplete()) {
        EndMoveMessage(apActor);
    }
    if (pTarget && dynamic_cast<HighProcess*>(apActor->pCurrentProcess) && apActor->pCurrentProcess) {
        if (apActor->pCurrentProcess->GetSitSleepState() == SIT_SLEEP_NORMAL) {
            NiPoint3 kTargetPos = pTarget->GetLocationOnReference();
            apActor->RequestRotate(kTargetPos, false);
        }
    }

    if (pTresPack) {
        Actor* pAct = dynamic_cast<Actor*>(pTarget);
        if (pAct && !pAct->IsTresPassing()) {
            AddToProcedureIndexRunning(apActor, 2);
            return;
        }
        if ((!pTresPack->iwarninglevel ||
             (pTresPack->iwarning > pTresPack->iwarninglevel && pTresPack->fwarningtimer <= 0.0)) &&
            pTargetAsActor) {
            pTargetAsActor->TrespassAlarm(apActor, nullptr, -1);
            return;
        }

        if (pTresPack->fwarningtimer <= 0.0) {
            AddToProcedureIndexRunning(apActor, 1);
            return;
        }
        float fMod = appTimer.fDelta * -1.0;
        pTresPack->ModWarningTimer(fMod);
        double dWarningTimer = pTresPack->fwarningtimer;
        if (fAITrespassWarningTimer.Float() < dWarningTimer) {
            fMod = pTresPack->fwarningtimer * -1.0;
            pTresPack->ModWarningTimer(fMod);
        }
    }
    Character* pChar = dynamic_cast<Character*>(pTarget);
    Actor* pTargetActor = dynamic_cast<Actor*>(pTarget);
    if (apActor->IsAlarmed() && pTargetActor) {
        if (pChar == pPlayer) {
            if (pTarget == pPlayer && pPlayer->IsSleepingorResting() && !pPlayer->btransporting) {
                pPlayer->SetSleeping(0, true);
            }
            else if (GetTalkingtoPC() || pPlayer->IsPlayerResistingArrest()) {
                if (pPlayer->IsPlayerResistingArrest()) {
                    EnterCombat(
                        apActor,
                        pPlayer,
                        false,
                        false,
                        nullptr,
                        false,
                        false,
                        false,
                        0,
                        false,
                        false,
                        true,
                        nullptr
                    );
                }
            }
            else if (pPlayer->Activate(apActor, false, nullptr, 1)) {
                AddToProcedureIndexRunning(apActor, 1);
            }
        }
        else {
            AddToProcedureIndexRunning(apActor, 2);
            EnterCombat(
                apActor,
                pTargetActor,
                false,
                false,
                nullptr,
                false,
                false,
                false,
                0,
                false,
                false,
                true,
                nullptr
            );
        }
    }
}

// Flee procedure: casts the package spell once its magic item is loaded, then, while the package still has a target,
// refreshes the avoided refs and either paths to the ref to flee to or finds a teleport door, ending the package once
// the flee timer has run out.
void MiddleHighProcess::ProcessFlee(Actor* apActor) {
    FleePackage* pFleePackage = dynamic_cast<FleePackage*>(GetPackageThatIsRunning());
    TESObjectREFR* pTargReference = nullptr;
    if (GetCurrentPackageSpell()) {
        MagicItem* pLoaded = GetCurrentPackageSpell();
        if (!pLoaded->IsLoaded()) {
            MagicItem* pLoading = GetCurrentPackageSpell();
            if (!pLoading->IsLoading()) {
                MagicItem* pPreload = GetCurrentPackageSpell();
                pPreload->Preload(false);
            }
            return;
        }
        MagicItem* pSpell = GetCurrentPackageSpell();
        // apActor is adjusted to its MagicCaster without a null test (only the MagicTarget argument has one).
        static_cast<MagicCaster&>(*apActor).CastSpell(pSpell, static_cast<MagicTarget*>(apActor), false);
        MagicItem* pUnload = GetCurrentPackageSpell();
        pUnload->Unload(false);
        SetCurrentPackageSpell(nullptr);
    }
    if (pFleePackage) {
        pFleePackage->UpdateAvoidedRefList(this);
        if (pFleePackage->GetPackageTarget()) {
            PackageTarget* pTarget = pFleePackage->GetPackageTarget();
            if (pTarget->GetTargReference()) {
                PackageTarget* pTargetWithRef = pFleePackage->GetPackageTarget();
                pTargReference = pTargetWithRef->GetTargReference();
            }
        }
        if (pTargReference) {
            float fFleeTimer = pFleePackage->GetFleeTimer();
            TESObjectREFR* pClosestAvoidedRef = pFleePackage->GetClosestAvoidedRef(apActor);
            if (pClosestAvoidedRef) {
                if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
                    apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                    apActor->InitiateGetUpPackage();
                    return;
                }
                TESObjectCELL* pParentCell = apActor->GetParentCell();
                if (pParentCell) {
                    pFleePackage->ModFleeTimer(appTimer.GetSeconds() * -1.0f);
                    float fDistance = apActor->GetDistanceFromReference(pClosestAvoidedRef, false, false);
                    float fFleeDistance = 0.0f;
                    if (pParentCell->GetInterior()) {
                        fFleeDistance = fFleeDistanceInterior.Float();
                    }
                    else {
                        fFleeDistance = fFleeDistanceExterior.Float();
                    }
                    if (fFleeDistance > fDistance && apActor->IsPathingComplete()) {
                        TESObjectREFR* pRefFleeTo = pFleePackage->GetRefFleeTo();
                        if (fEssentialDownTimer > 0.0f) {
                            fEssentialDownTimer = fEssentialDownTimer - appTimer.GetSeconds();
                        }
                        if (pRefFleeTo) {
                            TESWorldSpace* pWorld = pRefFleeTo->GetWorldSpace();
                            TESObjectCELL* pCell = pRefFleeTo->GetParentCell();
                            if (!SetupPathFinding(apActor, pRefFleeTo->GetLocationOnReference(), pCell, pWorld)) {
                                return;
                            }
                        }
                        else {
                            pFleePackage->FindTeleportDoor(apActor, pClosestAvoidedRef);
                            alignas(PathingRequestFlee) uint8_t aRequestStorage[sizeof(PathingRequestFlee)];
                            PathingRequestFlee* pRequest = ::new (aRequestStorage) PathingRequestFlee;
                            pRequest->Initialize(apActor);
                            pRequest->SetInitialPathHeading(apActor->GetHeading(false));
                            if (apActor->IsCreature()) {
                                TESActorBaseData* pBaseData = static_cast<TESActorBaseData*>(apActor->GetActorBase());
                                pRequest->SetCantOpenDoors(pBaseData->GetCantOpenDoors());
                            }
                            pRequest->SetTargetRadius(static_cast<float>(pFleePackage->GetLocationRadius(apActor)));
                            pRequest->SetAvoidNodeArray(pFleePackage->CreateAvoidNodeArray());
                            pRequest->SetMinFleeDistance(1000.0f);
                            if (pRefFleeTo) {
                                alignas(PathingLocation) uint8_t aLocationStorage[sizeof(PathingLocation)];
                                PathingLocation* pLocation = ::new (aLocationStorage) PathingLocation(pRefFleeTo);
                                pRequest->SetDestination(*pLocation);
                                pLocation->~PathingLocation();
                            }
                            else {
                                pRequest->SetNoDestination();
                            }
                            apActor->SetPathfindingHide(pRequest);
                            pRequest->~PathingRequestFlee();
                        }
                        pFleePackage->SetFleeSucceeded(false);
                        pFleePackage->ModFleeTimer(pFleePackage->GetFleeTimer() * -1.0f);
                    }
                    if (apActor->IsPathingComplete()) {
                        pFleePackage->SetFleeSucceeded(true);
                        pFleePackage->ModFleeTimer(appTimer.GetSeconds());
                        if (fFleeIsSafeTimer.Float() <= fFleeTimer) {
                            AddToProcedureIndexRunning(apActor, 1);
                        }
                    }
                }
            }
            else {
                pFleePackage->SetFleeSucceeded(true);
                pFleePackage->ModFleeTimer(appTimer.GetSeconds());
                if (fFleeIsSafeTimer.Float() <= fFleeTimer) {
                    AddToProcedureIndexRunning(apActor, 1);
                }
            }
        }
    }
}

void MiddleHighProcess::ProcessGetUpOneHour(Actor* apActor) {
    apActor->GetOutofFurnitureQuick();
}

bool MiddleHighProcess::SitOrSleep(Actor* apActor) {
    bool bSleeping;
    TESPackage* pPack = GetPackageThatIsRunning();
    if ((!pCurrentFurniture || !pCurrentFurniture->Get3D()) &&
        (!pPack || pPack->GetPackType() != PACKAGE_TYPE_SANDMAN)) {
        return false;
    }
    if (GetWeaponDrawn() && !GetAlert()) {
        apActor->SetWantWeaponDrawn(false);
        return true;
    }
    pGenericLocation = nullptr;
    Animation* pAnim = apActor->GetAnimation();
    switch (apActor->GetAnimAction()) {
        case ANIM_ACTION_EQUIP_WEAPON:
        case ANIM_ACTION_UNEQUIP_WEAPON:
        case ANIM_ACTION_ATTACK:
        case ANIM_ACTION_ATTACK_FOLLOW_THROUGH:
        case ANIM_ACTION_BLOCK:
        case ANIM_ACTION_RECOIL:
        case ANIM_ACTION_STAGGER:
        case ANIM_ACTION_DODGE: {
            return true;
        }
        default: {
            bSleeping = false;
            if (pCurrentFurniture) {
                bSleeping = TESFurniture::IsSleepMarker(FurnitureMarker.GetType());
            }
            if (cSitSleepState) {
                break;
            }
            HighProcess* pHigh = dynamic_cast<HighProcess*>(this);
            if (pHigh &&
                (pHigh->GetCurrentProcessIdle() ||
                 apActor->IsAnimIdleQuequed() ||
                 pAnim->SpecialIdleWorking() ||
                 pAnim->SpecialIdlePlaying())) {
                pHigh->SetCurrentProcessIdle(nullptr);
                pHigh->ClearPostAnimationActions();
                pAnim->SpecialIdleFree(true, true);
            }

            if (apActor->IsPc()) {
                HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_SIT_SLEEP_ANIM);
            }
            if (bSleeping && !GetCurrentPackage()) {
                ProcessRemoveWorn(apActor, false, false);
            }
            if (apActor->IsSneaking()) {
                apActor->SetWalkRunFlagDirty();
            }
            apActor->ClearMoveMode(1024u);
            if (apActor != pPlayer) {
                EndMoveMessage(apActor);
            }
            if (pCurrentFurniture->GetMarkerUsed(cCurrentFurnitureIndex, true) &&
                pPack &&
                pPack->GetPackType() != PACKAGE_TYPE_SANDMAN) {
                SetSitSleepState(apActor, SIT_SLEEP_NORMAL, nullptr, 127);
                return true;
            }
            if (pPack && pPack->GetPackType() != PACKAGE_TYPE_SANDMAN) {
                if (bSleeping) {
                    SetSitSleepState(apActor, SIT_SLEEP_LOAD_SLEEP_IDLE, pCurrentFurniture, cCurrentFurnitureIndex);
                }
                else {
                    SetSitSleepState(apActor, SIT_SLEEP_LOAD_SIT_IDLE, pCurrentFurniture, cCurrentFurnitureIndex);
                }
                if ((bSleeping || TESFurniture::IsSitMarker(FurnitureMarker.GetType())) && !LoadDynamicIdle(apActor)) {
                    int32_t iIndex = cCurrentFurnitureIndex;
                    const char* pEditorID = pCurrentFurniture->GetFormEditorID();
                    Warning(
                        "AI: Missing furniture dynamic idle for marker %d in the editor's idle manager. '%s' "
                        "index %d.",
                        FurnitureMarker.GetType(),
                        pEditorID,
                        iIndex
                    );
                }
            }

            SetCurrentProcessAnimIdleLoaded(nullptr, nullptr);
            RemovePostAnimationAction(BaseProcess::POSTANIM_ACTION_IDLE_LOADED);
            RemovePostAnimationAction(BaseProcess::POSTANIM_ACTION_PLAY_IDLE);
            if (bSleeping) {
                SetSitSleepState(apActor, SIT_SLEEP_WANT_TO_SLEEP, pCurrentFurniture, cCurrentFurnitureIndex);
            }
            else {
                SetSitSleepState(apActor, SIT_SLEEP_WANT_TO_SIT, pCurrentFurniture, cCurrentFurnitureIndex);
            }
            TESIdleForm* pIdle = pIdleManager->GetIdleToPlay(apActor, pCurrentFurniture);
            if (!pIdle) {
                int32_t iIndex = cCurrentFurnitureIndex;
                const char* pEditorID = pCurrentFurniture->GetFormEditorID();
                Warning(
                    "AI: Missing furniture entry animation for marker %d in the editor's idle manager. '%s' index %d.",
                    FurnitureMarker.GetType(),
                    pEditorID,
                    iIndex
                );
                cSitSleepState = 0;
                return false;
            }
            pAnim->SpecialIdleQueue(pIdle, pIdle->GetIdleAnimGroupSection());
            apActor->SetPosition(FurnitureMarker.Position);
            pAnim->ClearDelta();
            pCurrentFurniture->SetMarkerUsed(cCurrentFurnitureIndex, true);
            break;
        }
    }
    switch (cSitSleepState) {
        case 2:
        case 7: {
            float fTargetAngle = FurnitureMarker.GetHeading();
            apActor->StopMoving();
            apActor->SetHeading(fTargetAngle);
            apActor->SetPosition(FurnitureMarker.Position);
            pAnim->ClearDelta();
            if (bSleeping) {
                SetSitSleepState(apActor, SIT_SLEEP_WAITING_FOR_SLEEP_ANIM, pCurrentFurniture, cCurrentFurnitureIndex);
            }
            else {
                SetSitSleepState(apActor, SIT_SLEEP_WAITING_FOR_SIT_ANIM, pCurrentFurniture, cCurrentFurnitureIndex);
            }
            return true;
        }
        case 3:
        case 8: {
            if (pAnim->SpecialIdleLoaded() && GetAnimAction() == ANIM_ACTION_NONE) {
                if (!pAnim->GetCurrentSequence(AGS_MOVEMENT) ||
                    pAnim->GetCurrentSequence(AGS_MOVEMENT)->GetState() == NiControllerSequence::kAnimating) {
                    pAnim->ClearDelta();
                    apActor->SetPosition(FurnitureMarker.Position);
                    pAnim->SpecialIdlePlay(apActor);
                    apActor->PickAnimations(1.0f, 1.0f);
                    return true;
                }
            }
            if (!pAnim->SpecialIdleDonePlaying()) {
                if (!pAnim->GetSpecialIdleSequence() &&
                    pAnim->spAnimIdle.m_pObject &&
                    pAnim->spAnimIdle->eFlags == AnimIdle::ANIM_IDLE_DONE_PLAYING) {
                    apActor->GetOutofFurnitureQuick();
                    pAnim->SpecialIdleFree(true, false);
                }

                return true;
            }
            if (!apActor->GetSandman()) {
                if (GetFurnitureMarkerID() <= 20) {
                    pCurrentFurniture->SetMarkerReserved(cCurrentFurnitureIndex, false);
                    uint32_t uiType = FurnitureMarker.GetType();
                    float fHeadingOffset = static_cast<TESFurniture*>(pCurrentFurniture->GetObjectReference())
                                               ->TESFurniture::GetMarkerHeadingOffset(uiType);
                    apActor->ModHeading(fHeadingOffset);
                    pAnim->SkipNextBlend();
                    if (pAnim->spAnimIdle.m_pObject) {
                        pAnim->BlendOut(pAnim->spAnimIdle->eSection, false);
                    }
                    if (bSleeping) {
                        BSFaceGenAnimationData* pAnimData = apActor->GetFaceAnimationData();
                        if (pAnimData) {
                            pAnimData->SetDead(true, false);
                        }
                        SetSitSleepState(apActor, SIT_SLEEP_IS_SLEEPING, pCurrentFurniture, cCurrentFurnitureIndex);
                    }
                    else {
                        SetSitSleepState(apActor, SIT_SLEEP_IS_SITTING, pCurrentFurniture, cCurrentFurnitureIndex);
                    }
                    if (apActor->IsPc()) {
                        HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_SIT_SLEEP);
                    }
                }
                else {
                    pCurrentFurniture->SetMarkerUsed(cCurrentFurnitureIndex, false);
                    pCurrentFurniture->SetMarkerReserved(cCurrentFurnitureIndex, false);
                    SetSitSleepState(apActor, SIT_SLEEP_NORMAL, nullptr, 127);
                    FurnitureMarker.SetHeading(0.0f);
                    FurnitureMarker.SetType(0);
                    FurnitureMarker.SetPosition(NiPoint3::ZERO);
                    pCurrentFurniture = nullptr;
                    apActor->SetChaseBip(false);
                    apActor->SetAnimAction(ANIM_ACTION_NONE, nullptr);
                    if (pPack) {
                        uint32_t uiProcedureType = pPack->ePROCEDURE_TYPE;
                        if (Procedure::Action[uiProcedureType][GetProcedureIndexRunning()] != PROCEDURE_PATROL) {
                            AddToProcedureIndexRunning(apActor, 1);
                        }
                    }
                    if (apActor == pPlayer) {
                        HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_RUN_AND_GUN);
                    }
                }
                pAnim->SpecialIdleFree(true, false);
                return true;
            }
            SetSitSleepState(apActor, SIT_SLEEP_NORMAL, nullptr, 127);
            apActor->SetChaseBip(false);
            apActor->SetAnimAction(ANIM_ACTION_NONE, nullptr);
            pAnim->SpecialIdleFree(true, false);
            if (apActor == pPlayer) {
                HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_RUN_AND_GUN);
            }
            return false;
        }
        default: {
            return true;
        }
    }
}

// True when marker aiFurnitureNum of apFurnitureRef is a sit or sleep marker and the furniture's 3D has collision.
bool MiddleHighProcess::ShouldDisableCharController(TESObjectREFR* apFurnitureRef, uint32_t aiFurnitureNUm) {
    bool bResult = false;
    bool bSitMarker = false;
    if (apFurnitureRef) {
        apFurnitureRef->GetObjectReference();
        bSitMarker = TESFurniture::IsSitMarker(aiFurnitureNUm);
    }
    bool bSleepMarker = false;
    if (apFurnitureRef) {
        apFurnitureRef->GetObjectReference();
        bSleepMarker = TESFurniture::IsSleepMarker(aiFurnitureNUm);
    }
    if (bSitMarker || bSleepMarker) {
        bResult = GetCollisionObjectCountInSceneGraph(apFurnitureRef->Get3D(), false, false, false) > 0;
    }
    return bResult;
}

bool MiddleHighProcess::ProcessGetUp(Actor* apActor) {
    bool bResult;
    Animation* pAnim = apActor->GetAnimation();
    if (!pAnim) {
        return false;
    }
    if (pPlayer->IsPipboyActive()) {
        return false;
    }
    if (pCurrentFurniture) {
        bool bSleeping = TESFurniture::IsSleepMarker(FurnitureMarker.GetType());
        if (cSitSleepState == 4 && bSleeping) {
            return false;
        }

        if (cSitSleepState != 9 || bSleeping) {
            switch (cSitSleepState) {
                case 3: {
                    if (GetFurnitureMarkerID() > 20) {
                        pAnim->SetLoopCount(AGS_MOVEMENT, 0);
                        if (GetCurrentFurniture()) {
                            uint32_t uiIndex = GetCurrentFurnitureIndex();
                            TESObjectREFR* pFurniture = GetCurrentFurniture();
                            pFurniture->SetMarkerUsed(uiIndex, false);
                        }
                        SetSitSleepState(apActor, SIT_SLEEP_WANT_TO_STAND, pCurrentFurniture, cCurrentFurnitureIndex);
                    }
                    bResult = true;
                    break;
                }
                case 4:
                case 9: {
                    if (apActor->IsPc()) {
                        HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_SIT_SLEEP_ANIM);
                    }
                    if (pAnim->SpecialIdleWorking()) {
                        bResult = true;
                    }
                    else {
                        TESIdleForm* pIdle;
                        if (bSleeping) {
                            BSFaceGenAnimationData* pAnimData = apActor->GetFaceAnimationData();
                            if (pAnimData) {
                                pAnimData->SetDead(false, false);
                            }
                            SetSitSleepState(
                                apActor,
                                SIT_SLEEP_WANT_TO_WAKE,
                                pCurrentFurniture,
                                cCurrentFurnitureIndex
                            );
                        }
                        else {
                            SetSitSleepState(
                                apActor,
                                SIT_SLEEP_WANT_TO_STAND,
                                pCurrentFurniture,
                                cCurrentFurnitureIndex
                            );
                        }
                        pIdle = pIdleManager->GetIdleToPlay(apActor, pCurrentFurniture);
                        if (pIdle) {
                            pAnim->SpecialIdleQueue(pIdle, pIdle->GetIdleAnimGroupSection());
                            bResult = true;
                        }
                        else {
                            int32_t iIndex = cCurrentFurnitureIndex;
                            const char* pEditorID = pCurrentFurniture->GetFormEditorID();
                            Warning(
                                "AI: Missing furniture exit animation (index %d) for marker %d in the editor's "
                                "idle manager. '%s' index %d.",
                                cCurrentFurnitureIndex,
                                FurnitureMarker.GetType(),
                                pEditorID,
                                iIndex
                            );
                            pCurrentFurniture->SetMarkerUsed(cCurrentFurnitureIndex, false);
                            pAnim->SkipNextBlend();
                            pAnim->ClearGroup(AGS_IDLE, 0.0f);
                            SetSitSleepState(apActor, SIT_SLEEP_NORMAL, pCurrentFurniture, 127);
                            uint32_t uiType = FurnitureMarker.GetType();
                            float fHeadingOffset = -static_cast<TESFurniture*>(pCurrentFurniture->GetObjectReference())
                                                        ->TESFurniture::GetMarkerHeadingOffset(uiType);
                            apActor->ModHeading(fHeadingOffset);
                            float fHeading = apActor->GetHeading(false) + 3.1415927410125732;
                            apActor->SetHeading(fHeading);
                            ClearFurniture(apActor);
                            apActor->SetChaseBip(false);
                            if (bSleeping) {
                                bool bWearWeapon = true;
                                TESPackage* pPack = GetPackageThatIsRunning();
                                if (pPack) {
                                    bWearWeapon = !pPack->GetForceUnequipWeapon();
                                }
                                if (apActor->IsNpc()) {
                                    TESNPC* pNPC = dynamic_cast<TESNPC*>(apActor->GetObjectReference());
                                    if (pNPC) {
                                        pNPC->InitDefaultWorn(apActor, true, bWearWeapon, false, true);
                                    }
                                }
                                else {
                                    TESCreature* pCreature = dynamic_cast<TESCreature*>(apActor->GetObjectReference());
                                    if (pCreature) {
                                        pCreature->InitDefaultWorn(apActor, true, bWearWeapon, true);
                                    }
                                }
                            }
                            float fDamage = (apActor->GetFatigue() + 10.0) * -1.0;
                            apActor->DamageModActorValue(ActorValue::FATIGUE, fDamage, nullptr);
                            AddToProcedureIndexRunning(apActor, 1);
                            if (GetNeedTalkPlayer()) {
                                apActor->InitiateDialogue(
                                    pPlayer,
                                    nullptr,
                                    nullptr,
                                    true,
                                    false,
                                    false,
                                    nullptr,
                                    false,
                                    false
                                );
                            }
                            bResult = false;
                        }
                    }
                    break;
                }
                case 5:
                case 10: {
                    if (pAnim->SpecialIdleLoaded() &&
                        (!pAnim->GetCurrentSequence(AGS_MOVEMENT) ||
                         pAnim->GetCurrentSequence(AGS_MOVEMENT)->GetState() == NiControllerSequence::kAnimating)) {
                        SIT_SLEEP_STATE eCurrentState = static_cast<SIT_SLEEP_STATE>(cSitSleepState);
                        SetSitSleepState(apActor, SIT_SLEEP_NORMAL, pCurrentFurniture, cCurrentFurnitureIndex);
                        uint32_t uiType = FurnitureMarker.GetType();
                        float fHeadingOffset = -static_cast<TESFurniture*>(pCurrentFurniture->GetObjectReference())
                                                    ->TESFurniture::GetMarkerHeadingOffset(uiType);
                        apActor->ModHeading(fHeadingOffset);
                        SetSitSleepState(apActor, eCurrentState, pCurrentFurniture, cCurrentFurnitureIndex);
                        pAnim->SkipNextBlend();
                        pAnim->SpecialIdlePlay(apActor);
                    }
                    else if (pAnim->SpecialIdleDonePlaying()) {
                        uint32_t uiMarkerID = GetFurnitureMarkerID();
                        if (uiMarkerID <= 20) {
                            pAnim->SkipNextBlend();
                        }
                        ClearFurniture(apActor);
                        if (apActor->IsPc()) {
                            HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_RUN_AND_GUN);
                        }
                        if (uiMarkerID <= 20) {
                            float fHeading = apActor->GetHeading(false) + 3.1415927410125732;
                            apActor->SetHeading(fHeading);
                        }
                        apActor->PickAnimations(1.0f, 1.0f);
                        apActor->SetChaseBip(false);
                        bhkCharacterController* pCharController = GetCharController();
                        if (pCharController) {
                            pCharController->SetPosition(*apActor->QLocationOnReference());
                        }
                        if (GetNeedTalkPlayer()) {
                            apActor->InitiateDialogue(
                                pPlayer,
                                nullptr,
                                nullptr,
                                true,
                                false,
                                false,
                                nullptr,
                                false,
                                false
                            );
                        }
                    }
                    bResult = true;
                    break;
                }
                default: {
                    if (bSleeping) {
                        cSitSleepState = 9;
                    }
                    else {
                        cSitSleepState = 4;
                    }
                    bResult = true;
                    break;
                }
            }
        }
        else {
            return false;
        }
    }
    else {
        SetSitSleepState(apActor, SIT_SLEEP_NORMAL, nullptr, 127);
        AddToProcedureIndexRunning(apActor, 1);
        if (GetNeedTalkPlayer()) {
            apActor->InitiateDialogue(pPlayer, nullptr, nullptr, true, false, false, nullptr, false, false);
        }
        return false;
    }
    return bResult;
}

void MiddleHighProcess::ProcessTravel(
    Actor* apActor,
    bool abNewpackage,
    bool abInterateindex,
    int32_t aemovemode,
    bool abSecondLoc
) {
    TESObjectCELL* pLocCell;
    TESWorldSpace* pLocWorld;
    NiPoint3 kLocCoord;
    TESPackage* pPack = nullptr;
    pPack = GetPackageThatIsRunning();
    if ((abNewpackage && !pPack->IsActorAtLocation(apActor, false, -1.0f, false)) ||
        (apActor->IsPathingComplete() &&
         !pPack->IsActorAtLocation(apActor, false, -1.0f, false) &&
         !DidPathingFail(apActor))) {
        apActor->SetMoveMode(static_cast<uint32_t>(aemovemode));
        pLocWorld = pPack->GetLocationWorld(apActor);
        pLocCell = pPack->GetLocationCell(apActor);
        kLocCoord = pPack->GetLocationCoord(apActor);
        if (!SetupPathFinding(apActor, kLocCoord, pLocCell, pLocWorld)) {
            return;
        }
    }
    float fCurrentDistance = apActor->GetDistanceFromPoint(pPack->GetLocationCoord(apActor));
    float fDistance = pPack->GetPackageRadiusActorToLocation(apActor, false);
    bool bAtRef = false;
    TESObjectREFR* pRef = nullptr;
    if (pPack->pPackLoc) {
        pRef = pPack->pPackLoc->GetLocReference();
    }
    if (pGenericLocation && !GetRunOncePackage()) {
        pRef = pGenericLocation;
    }

    if (pPack->GetPackType() == PACKAGE_TYPE_WANDER) {
        fDistance = 20.0f;
    }
    if (pRef && pRef->IsFurniture() && pPack->GetPackType() != PACKAGE_TYPE_WANDER) {
        if (!pCurrentFurniture) {
            pCurrentFurniture = pRef;
            ClearFurnitureMarker();
        }
        if (apActor->GetSitSleepState()) {
            bAtRef = true;
        }
        else if (
            apActor->GetSitSleepState() == SIT_SLEEP_NORMAL &&
            pCurrentFurniture->GetMarkerUsed(cCurrentFurnitureIndex, true) &&
            !apActor->IsPathingComplete()
        ) {
            ClearFurniture(apActor);
            AddToProcedureIndexRunning(apActor, 1);
            return;
        }
        else {
            bAtRef = pPack->IsActorAtLocation(apActor, false, -1.0f, false);
        }
    }
    else {
        bAtRef = pPack->IsActorAtLocation(apActor, false, -1.0f, false);
    }
    if (bAtRef || DidPathingFail(apActor)) {
        if (!GetLockedLocation()) {
            if (pPack->GetLockDoorsAtLocation()) {
                const NiPoint3* pSecondPos = &apActor->GetLocationOnReference();
                const NiPoint3* pFirstPos = &apActor->GetLocationOnReference();
                TESObjectCELL* pCell = apActor->GetParentCell();
                pDataHandler->EnumReferencesCloseToPoint(
                    pCell,
                    *pFirstPos,
                    5000.0f,
                    *pSecondPos,
                    5000.0f,
                    &LowProcess::LockDoorsatLocation,
                    apActor
                );
            }
            SetLockedLocation(true);
        }
        if (pPack->GetUnlockDoorsAtLocation()) {
            const NiPoint3* pSecondPos = &apActor->GetLocationOnReference();
            const NiPoint3* pFirstPos = &apActor->GetLocationOnReference();
            TESObjectCELL* pCell = apActor->GetParentCell();
            pDataHandler->EnumReferencesCloseToPoint(
                pCell,
                *pFirstPos,
                5000.0f,
                *pSecondPos,
                5000.0f,
                &LowProcess::UnLockDoorsatLocation,
                apActor
            );
        }
        if (pPack && !bDoneClothesChange && pPack->GetForceUnequipWeapon()) {
            bDoneClothesChange = true;
            ItemChange* pWeaponItem = GetCurrentWeapon();
            if (pWeaponItem) {
                apActor->QueueUnEquipObject(pWeaponItem->pContainerObj, 1, nullptr, false, false, true);
            }
        }

        if (pRef && pRef->IsFurniture()) {
            if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
                apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                if (abInterateindex) {
                    AddToProcedureIndexRunning(apActor, 1);
                }
                EndMoveMessage(apActor);
            }
            else {
                if (pPlayer->IsSleepingorResting()) {
                    NiMatrix3 kRot;
                    TESFurniture* pFurniture = dynamic_cast<TESFurniture*>(pRef->GetObjectReference());
                    FurnitureMark* pMarker = GetFurnitureMarker();
                    apActor->SetHeading(pMarker->GetHeading());
                    float fScale = apActor->GetScale();
                    uint32_t uiType = pMarker->GetType();
                    NiPoint3 kMarkerOffset = pFurniture->GetMarkerTargetOffset(uiType, fScale);
                    kRot.MakeZRotation(pMarker->GetHeading());
                    kMarkerOffset = kRot * kMarkerOffset;
                    pRef->SetMarkerUsed(cCurrentFurnitureIndex, true);
                    apActor->SetPosition(pMarker->Position + kMarkerOffset);
                    uiType = pMarker->GetType();
                    apActor->ModHeading(pFurniture->GetMarkerHeadingOffset(uiType));
                    bool bSleep = pPack->GetPackType() == PACKAGE_TYPE_SLEEP;
                    if (bSleep) {
                        SetSitSleepState(apActor, SIT_SLEEP_LOAD_SLEEP_IDLE, pCurrentFurniture, cCurrentFurnitureIndex);
                        apActor->pCurrentProcess->SetProcedureIndexRunning(1);
                        apActor->pCurrentProcess->ProcessRemoveWorn(apActor, false);
                    }
                    else {
                        SetSitSleepState(apActor, SIT_SLEEP_LOAD_SIT_IDLE, pCurrentFurniture, cCurrentFurnitureIndex);
                    }
                    if (LoadDynamicIdle(apActor)) {
                        if (bSleep) {
                            BSFaceGenAnimationData* pAnimData = apActor->GetFaceAnimationData();
                            if (pAnimData) {
                                pAnimData->SetDead(true, false);
                            }
                            SetSitSleepState(apActor, SIT_SLEEP_IS_SLEEPING, pCurrentFurniture, cCurrentFurnitureIndex);
                        }
                        else {
                            SetSitSleepState(apActor, SIT_SLEEP_IS_SITTING, pCurrentFurniture, cCurrentFurnitureIndex);
                        }
                    }
                }
                else {
                    AlignToFurnitureAndAddPostAnimSitSleep(apActor);
                }
            }
        }
        else {
            if (((pRef && pRef->GetObjectReference() == pXMarkerHeading) ||
                 (pPack->pPackLoc && pPack->pPackLoc->GetLocType() == PLT_EDITORLOC)) &&
                !DidPathingFail(apActor) &&
                apActor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
                float fTargetAngle = 0.0f;
                if (pRef) {
                    fTargetAngle = pRef->GetAngleOnReference().z;
                }
                else {
                    fTargetAngle = apActor->GetStartingAngle().z;
                }
                apActor->SetHeading(fTargetAngle);
            }

            EndMoveMessage(apActor);
            if (abInterateindex && (!DidPathingFail(apActor) || !pPack->GetMustComplete())) {
                AddToProcedureIndexRunning(apActor, 1);
            }
        }
    }
    else if (apActor) {
        if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
            apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
            apActor->InitiateGetUpPackage();
        }
        else {
            Actor* pLocActor = nullptr;
            if (pRef && pRef->IsFurniture()) {
                NiPoint3 kPoint = apActor->GetLocationOnReference();
                uint32_t uiNum = 0;
                if (!pCurrentFurniture) {
                    pCurrentFurniture = pRef;
                    ClearFurnitureMarker();
                }
                if (pCurrentFurniture->GetClosestFreeMarker(kPoint, true, true, FurnitureMarker, uiNum, true)) {
                    TESWorldSpace* pWorld = pPack->GetLocationWorld(apActor);
                    TESObjectCELL* pCell = pPack->GetLocationCell(apActor);
                    if (SetupPathFinding(apActor, FurnitureMarker.Position, pCell, pWorld)) {
                        pCurrentFurniture = pRef;
                        cCurrentFurnitureIndex = uiNum;
                    }
                }
                else {
                    EndMoveMessage(apActor);
                    SetProcedureIndexRunning(Procedure::GetProcedureCount(pPack->ePROCEDURE_TYPE) - 1);
                }
            }
            else {
                if (pPack->pPackLoc && pPack->pPackLoc->GetLocReference()) {
                    pLocActor = dynamic_cast<Actor*>(pPack->pPackLoc->GetLocReference());
                }

                if (pLocActor) {
                    float fDistanceMoved = (lastSeenPostion - pLocActor->GetLocationOnReference()).Length();
                    if (fAIMoveDistanceToRecalcFollowPath.Float() < static_cast<double>(fDistanceMoved) ||
                        (fDistance < static_cast<double>(fCurrentDistance) && apActor->IsPathingComplete())) {
                        TESWorldSpace* pWorld = pLocActor->GetWorldSpace();
                        TESObjectCELL* pCell = pLocActor->GetParentCell();
                        if (SetupPathFinding(apActor, pLocActor->GetLocationOnReference(), pCell, pWorld)) {
                            lastSeenPostion = pLocActor->GetLocationOnReference();
                        }
                    }
                }
            }
        }
    }
}

bool MiddleHighProcess::EnterCombat(
    Actor* apActor,
    Actor* apTarget,
    bool abDontCheckForAmbush,
    bool absendalarm,
    CombatGroup* apGroup,
    bool abfleeing,
    bool abForceFlee,
    bool abIsAggressor,
    char acTeam,
    bool bforcecombat,
    bool bIgnoreDefensivePackage,
    bool abCastSpell,
    TESPackage* apPackage
) {
    if (GetFinishingCombatPackage()) {
        return false;
    }
    TESPackage* pCurrentPack = GetCurrentPackage();
    if (!abDontCheckForAmbush &&
        pCurrentPack &&
        pCurrentPack->GetPackType() == PACKAGE_TYPE_AMBUSH &&
        !pCurrentPack->IsActorAtSecondLocation(apTarget, apActor, false, -1.0f, false)) {
        return false;
    }
    if (!apTarget && !apGroup) {
        apActor->StartCombat(nullptr, nullptr, false, false, false, 0, false, apPackage);
        return true;
    }
    if (apTarget) {
        PROCESS_TYPE eTargetProcessType = apTarget->GetCurrentProcessType();
        if (eTargetProcessType != apActor->GetCurrentProcessType() && !apActor->Get3D()) {
            LowProcess::EnterCombat(
                apActor,
                apTarget,
                abDontCheckForAmbush,
                absendalarm,
                apGroup,
                abfleeing,
                abForceFlee,
                abIsAggressor,
                acTeam,
                bIgnoreDefensivePackage,
                false,
                true,
                nullptr
            );
        }
    }
    if (apActor->IsRestrained() || apActor->IsUnConscious()) {
        return false;
    }
    if (!apTarget ||
        apActor->GetCombatController() ||
        bforcecombat ||
        !pCombatManager->CheckShouldFlee(apActor, apTarget)) {
        if (apActor->GetCombatController() && apTarget && !apGroup) {
            if (apActor->GetCombatController()->IsActoraCombatTarget(apTarget)) {
                return false;
            }
        }

        if (apTarget == pPlayer && !bforcecombat) {
            int32_t iNumInCombat = pPlayer->GetNumberActorsInCombat();
            if (iNumInCombat >= iNumberActorsInCombatPlayer.Int()) {
                return false;
            }
        }
        if (apTarget) {
            bool bTargetSwimming = apTarget->IsSwimming();
            if (!apActor->FightsInWater() && bTargetSwimming) {
                return false;
            }
            if (apActor->SwimsOnly() && !bTargetSwimming) {
                return false;
            }
        }
        ClearMovementStoped();
        TESPackage* pPack = GetPackageThatIsRunning();
        if (pPack && pPack->IsInterruptPackage()) {
            pPack = apActor->m_Extra.GetPackageExtra();
        }

        if (apTarget && pPack && pPack->GetDefensive() && !bforcecombat && !bIgnoreDefensivePackage) {
            NiPoint3 kDest = GetCurrentDestinationCoordinate(apActor, false);
            // The package's radius, 600 when below 1.
            float fDistancePack = static_cast<float>(pPack->GetLocationRadius(apActor));
            if (fDistancePack < 1.0) {
                fDistancePack = 600.0f;
            }
            double dDistance = apTarget->GetDistanceFromPoint(kDest);
            if (fDistancePack < dDistance && apTarget->GetCombatController()) {
                if (!apTarget->GetCombatController()->IsActoraCombatTarget(apActor)) {
                    SetDetectAlert(true);
                    return false;
                }
            }
        }

        if (!apActor->GetCombatController()) {
            SetDetectAlert(false);
            apActor->EndDialogue();
            pSavedAcquireObject = pAcquireObject;
            pAcquireObject = nullptr;
        }
        bool bAggressor = abIsAggressor;
        if (apActor->IsActoraGuard()) {
            bAggressor = false;
        }
        if (apTarget && apTarget->IsInCombat() && apTarget->GetCombatController()) {
            if (apTarget->GetCombatController()->IsActoraCombatTarget(apActor)) {
                bAggressor = false;
            }
        }

        SetIsAggressor(bAggressor);
        int32_t iPriority = 0;
        if (bforcecombat) {
            iPriority = 200;
        }
        apActor->StartCombat(apTarget, apGroup, bforcecombat, bAggressor, abfleeing, iPriority, abCastSpell, apPackage);
        if (apTarget == pPlayer && apActor->IsInCombat() && pPlayer->GetDetectionLevel(apActor) > 0) {
            pPlayer->AddActorToPlayerCombatList(apActor);
        }
        return true;
    }
    apActor->InitiateFlee(apTarget, true, true, true, nullptr, nullptr, -1.0f, -1.0f);
    if (apTarget == pPlayer && pPlayer->GetDetectionLevel(apActor) > 0) {
        pPlayer->AddActorToPlayerCombatList(apActor);
    }
    return true;
}

// The weapon's condition stage: a tenth of its health (an embedded weapon's is the body-part condition actor value),
// rounded up, less one above 1. Multiplies by the float 0.1 widened to a double and stores the product as a float.
int32_t MiddleHighProcess::GetWeaponCondition(Actor* apActor) {
    if (!pWeapon) {
        return 0;
    }
    TESObjectWEAP* pWeaponForm = static_cast<TESObjectWEAP*>(pWeapon->GetContainerObject());
    float fHealth = pWeapon->GetItemHealth(true);
    if (pWeaponForm && pWeaponForm->IsEmbeddedWeapon()) {
        ActorValue::Index iValue = pWeaponForm->GetEmbeddedWeaponConditionValue();
        TESActorBase* pBase = apActor->GetActorBase();
        BaseProcess* pProcess = this;
        fHealth = pProcess->GetActorFloatValue(pBase, iValue, apActor);
    }
    const float fTenth = fHealth * 0.10000000149011612;
    int32_t iStage = static_cast<int32_t>(std::ceil(fTenth));
    if (iStage > 1) {
        --iStage;
    }
    return iStage;
}

TESIdleForm* MiddleHighProcess::GetLastIdlePlayed() {
    return pLastIdlePlayed;
}

void MiddleHighProcess::SetLastIdlePlayed(TESIdleForm* apIdle) {
    pLastIdlePlayed = apIdle;
}

BSSimpleList<ActiveEffect*>* MiddleHighProcess::GetActiveEffectList() {
    return pActiveEffectList;
}

// Calls MagicCaster::CastAbility directly, not through the vtable.
void MiddleHighProcess::CastAbility(MagicCaster* apCaster, SpellItem* apAbility, bool abLoadCast) {
    apCaster->MagicCaster::CastAbility(apAbility, abLoadCast);
}

// Calls MagicCaster::TransferDisease directly, not through the vtable.
void MiddleHighProcess::TransferDisease(
    MagicCaster* apCaster,
    SpellItem* apDisease,
    MagicTarget* apTarget,
    bool abLoadCast
) {
    apCaster->MagicCaster::TransferDisease(apDisease, apTarget, abLoadCast);
}

MagicItem* MiddleHighProcess::GetCurrentSpell() {
    return pCurrentSpell;
}

void MiddleHighProcess::SetCurrentSpell(MagicItem* apCurrentSpell) {
    pCurrentSpell = apCurrentSpell;
}

MagicTarget* MiddleHighProcess::GetDesiredTarget() {
    return pDesiredTarget;
}

void MiddleHighProcess::SetDesiredTarget(MagicTarget* apDesiredTarget) {
    pDesiredTarget = apDesiredTarget;
}

bool MiddleHighProcess::GetShouldCheckMagicNode() {
    return bCheckMagicNode;
}

void MiddleHighProcess::SetShouldCheckMagicNode(bool abCheck) {
    bCheckMagicNode = abCheck;
}

void MiddleHighProcess::SetIronSights(bool abIronSights) {
    bIronSights = abIronSights;
}

bool MiddleHighProcess::GetIronSights() {
    return bIronSights;
}

bool MiddleHighProcess::HasMovementAnim(ANIM_GROUP_ENUM aeMovementAnim) {
    if (!pAnimation || pAnimation->GetAnimsLoading()) {
        return true;
    }
    uint32_t uiBit = static_cast<uint32_t>(2 * aeMovementAnim - 6);
    if (uiBit > 32) {
        return true;
    }
    uint32_t uiFlags = iMovementAnimations >> uiBit;
    if ((uiFlags & MOVEMENT_ANIM_LOADED) != 0) {
        return true;
    }
    if ((uiFlags & MOVEMENT_ANIM_NOT_LOADED) != 0) {
        return false;
    }
    uint8_t ucLoaded = pAnimation->GroupLoaded(aeMovementAnim);
    iMovementAnimations |= (2 - ucLoaded) << uiBit;
    return ucLoaded;
}

KNOCK_STATE_ENUM MiddleHighProcess::GetKnockState() {
    return static_cast<KNOCK_STATE_ENUM>(cKnockState);
}

void MiddleHighProcess::SetKnockState(int8_t aeKnockState) {
    cKnockState = aeKnockState;
}

// Knocks apActor back from an explosion at aSourceLocation: queued for the task threads when 3D tasks are queued;
// an actor with a ragdoll goes limp and takes the explosion impulse, one without has its character controller pushed
// away for a while.
void MiddleHighProcess::KnockExplosion(Actor* apActor, NiPoint3 aSourceLocation, float afMagnitude) {
    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
        TaskQueueInterface::GetInstance()
            ->QueueActorKnockExplosion(apActor, aSourceLocation.x, aSourceLocation.y, aSourceLocation.z, afMagnitude);
        return;
    }
    if (!apActor->CanKnockDown()) {
        return;
    }
    ClearMuzzleFlash();
    if (!apActor->HasRagDoll()) {
        bhkCharacterController* pController = static_cast<bhkCharacterController*>(apActor->GetCharController());
        NiPoint3 kPosition;
        pController->GetPosition(kPosition);
        NiPoint3 kVelocity = kPosition - aSourceLocation;
        kVelocity.z = 0.0f;
        kVelocity.Unitize();
        float fScale = afMagnitude / fMagicExplosionPowerMax.Float() * fMagicExplosionIncorporealMult.Float();
        kVelocity *= fScale;
        float fTime = afMagnitude / fMagicExplosionPowerMax.Float() * fMagicExplosionIncorporealTime.Float();
        pController->SetVelocityModifier(kVelocity, fTime);
        return;
    }
    if (apActor->GetSitSleepState()) {
        apActor->GetOutofFurnitureQuick();
    }
    // KNOCK_STATE 2: the explosion lead-in.
    cKnockState = 2;
    ComputeLastTimeProcessed();
    NiAVObject* p3D;
    if (apActor == pPlayer) {
        p3D = pPlayer->Get3D(false);
    }
    else {
        p3D = apActor->TESObjectREFR::Get3D();
    }
    if (apActor->pRagdollController) {
        apActor->pRagdollController->EnableRagdollFeedback(false);
        apActor->pRagdollController->DisableRagdollAnim(true);
    }
    CFilter kFilter;
    apActor->GetCollisionFilter(&kFilter.iFilter);
    uint32_t iGroup = kFilter.GetGroup();
    apActor->pPenetrationDetection->Initialize(p3D, iGroup);
    apActor->SetHavokWeapon();
    bhkWorld::SetMotion(p3D, hkpMotion::MOTION_DYNAMIC, true, false, true);
    NiUpdateData kData(0.0f, false, false);
    p3D->Update(kData);
    hkVector4 kLocation;
    NI2HK(kLocation, aSourceLocation);
    TESHavokUtilities::AddExplosionImpulse(p3D, kLocation, afMagnitude, nullptr);
}

void MiddleHighProcess::UpdateKnockState(Actor* apActor) {
    NiPoint3 kDelta;
    Animation* pAnim = apActor->GetAnimation();
    if (pAnim && (apActor != pPlayer || pAnim != pPlayer->GetAnimation(true))) {
        if (apActor->CanKnockDown()) {
            bool bUnconscious = false;
            if (GetCachedFlag(CachedValues::CVF_FATIGUE_DAMAGED)) {
                bUnconscious = apActor->GetFatigue() < 0.0;
            }
            if ((bUnconscious ||
                 apActor->IsParalyzed() ||
                 apActor->GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN) &&
                !cKnockState) {
                if (apActor->GetSitSleepState() && apActor->GetSitSleepState() != SIT_SLEEP_WANT_TO_STAND) {
                    apActor->GetOutofFurnitureQuick();
                }

                apActor->InterruptCast();
                ClearMuzzleFlash();
                apActor->AddChange(CHANGE_REFR_HAVOK_MOVE);
                if (apActor->IsParalyzed()) {
                    cKnockState = 3;
                    if (apActor->pRagdollController) {
                        apActor->pRagdollController->EnableRagdollFeedback(false);
                        apActor->pRagdollController->SyncRagdollTo3D();
                        apActor->pRagdollController->DisableRagdollAnim(false);
                    }
                    bhkWorld::SetMotion(apActor->TESObjectREFR::Get3D(), hkpMotion::MOTION_DYNAMIC, true, false, true);
                    bhkRigidBody::SetFixed(apActor->TESObjectREFR::Get3D(), 1, 1);
                    pAnim->ClearGroup(AGS_WHOLE_BODY, 0.0f);
                    pAnim->ZeroGlobalTransform();
                    EndMoveMessage(apActor);
                }
                else {
                    if (apActor->IsPlayerTeammate()) {
                        FollowerBarks::TriggerFollowerBark(apActor, FollowerBarks::BARK_KNOCKED_OUT);
                    }
                    cKnockState = 4;
                    pAnim->GetDelta(&kDelta, apActor, true, false);
                    float fScale = appTimer.fDelta * 50.0;
                    kDelta = -kDelta / fScale;
                    if (apActor->pRagdollController) {
                        apActor->pRagdollController->EnableRagdollFeedback(false);
                        apActor->pRagdollController->DisableRagdollAnim(true);
                    }
                    apActor->SetHavokWeapon();
                    bhkBlendController::DoKnockDown(apActor->TESObjectREFR::Get3D(), kDelta, 0, 0.0f, 1);
                    bhkRigidBody::SetFixed(apActor->TESObjectREFR::Get3D(), 0, 1);
                }
            }
            switch (cKnockState) {
                case 1: {
                    double dDiff;
                    float fTime = fHourLastProcessed;
                    float fCurrent = appCalendar.GetHour();
                    float fElapsedHours = 0.0f;
                    if (fTime > static_cast<double>(fCurrent)) {
                        dDiff = fTime + 23.0 - fCurrent;
                    }
                    else {
                        dDiff = fCurrent - fTime;
                    }
                    fElapsedHours = dDiff;
                    double dTimeScale = appCalendar.GetTimeScale();
                    bool bMin = fElapsedHours > dTimeScale * 0.0006600000197067857;
                    dTimeScale = appCalendar.GetTimeScale();
                    bool bMax = fElapsedHours > dTimeScale * 0.001500000013038516;
                    if (apActor->GetFatigue() >= 0.0 &&
                        !apActor->IsParalyzed() &&
                        bMin &&
                        (bMax || apActor->IsNonAccumNearController(16.0f))) {
                        char cKnockStateSave = cKnockState;
                        cKnockState = 3;
                        TESIdleForm* pIdle = pIdleManager->GetIdleToPlay(apActor, nullptr);
                        cKnockState = cKnockStateSave;
                        BSFaceGenAnimationData* pAnimData = apActor->GetFaceAnimationData();
                        if (pAnimData) {
                            pAnimData->SetDead(false, false);
                        }
                        cKnockState = 5;
                        if (pIdle) {
                            pAnim->SpecialIdleQueue(pIdle, AGS_WHOLE_BODY);
                        }
                        else {
                            Warning("AI: No 'GetUp' animation found for actor '%s'.", apActor->GetFullName());
                            ClearKnockState(apActor);
                        }
                    }
                    break;
                }
                case 2:
                case 4: {
                    NiNode* pRoot = pAnim->pAccumRoot;
                    if (!pRoot || bhkUtilFunctions::GetHeirGain(pRoot) <= 0.0) {
                        apActor->StopCurrentDialogue();
                        if (apActor == pPlayer && pPlayer->GetIronSights()) {
                            pPlayer->SetIronSights(false, false, false);
                        }
                        if (cKnockState == 4) {
                            cKnockState = 3;
                        }
                        else {
                            cKnockState = 1;
                        }
                        BSFaceGenAnimationData* pAnimData = apActor->GetFaceAnimationData();
                        if (pAnimData) {
                            pAnimData->SetDead(true, false);
                        }
                        pAnim->ClearGroup(AGS_WHOLE_BODY, 0.0f);
                        pAnim->ZeroGlobalTransform();
                        EndMoveMessage(apActor);
                    }
                    break;
                }
                case 3: {
                    if (apActor->GetFatigue() >= 0.0 &&
                        !apActor->IsParalyzed() &&
                        apActor->GetLifeState() != ACTOR_LIFE_STATE_ESSENTIAL_DOWN) {
                        TESIdleForm* pIdle = pIdleManager->GetIdleToPlay(apActor, nullptr);
                        if (pIdle) {
                            pAnim->SpecialIdleQueue(pIdle, AGS_WHOLE_BODY);
                            BSFaceGenAnimationData* pAnimData = apActor->GetFaceAnimationData();
                            if (pAnimData) {
                                pAnimData->SetDead(false, false);
                            }
                            cKnockState = 5;
                        }
                        else {
                            Warning("AI: No 'GetUp' animation found for actor '%s'.", apActor->GetFullName());
                            ClearKnockState(apActor);
                        }
                    }
                    break;
                }
                case 5: {
                    if (pAnim->SpecialIdleLoaded() &&
                        (!pAnim->GetCurrentSequence(AGS_MOVEMENT) ||
                         pAnim->GetCurrentSequence(AGS_MOVEMENT)->GetState() == NiControllerSequence::kAnimating)) {
                        if (apActor->IsSwimming() || !pAnim->SpecialIdlePlay(apActor)) {
                            pAnim->SpecialIdleFree(true, false);
                        }
                        NiAVObject* p3D = apActor->TESObjectREFR::Get3D();
                        NiAVObject* pSpine = pAnim->pAccumRoot;
                        if (pSpine) {
                            pSpine = static_cast<NiNode*>(pSpine)->NiNode::GetAtChecked(0);
                            NiAVObject* pHead = apActor->GetActorBone(pSpine, *FixedStrings::GetBip01_Head());
                            if (!pHead) {
                                pHead = BSUtilities::GetObjectByName(pSpine, FixedStrings::QBip01Neck());
                            }
                            if (!pHead) {
                                pHead = BSUtilities::GetObjectByName(pSpine, FixedStrings::QBip01Spine1());
                            }
                            if (pHead) {
                                NiPoint3 kMoveDir;
                                const NiPoint3* pSpinePos = &pSpine->GetWorldTranslate();
                                kMoveDir = pHead->GetWorldTranslate() - *pSpinePos;
                                if (!GetCharController()->QQuadruped()) {
                                    NiAVObject* p180Test = apActor->GetActorBone(p3D, *FixedStrings::GetBip01_Spine());
                                    if (p180Test &&
                                        bhkNiCollisionObject::GetFaceUp(static_cast<NiNode*>(p180Test), nullptr)) {
                                        kMoveDir *= -1.0f;
                                    }
                                }
                                float fTargetAngle = GetZAngleFromVector(kMoveDir);
                                cKnockState = 0;
                                apActor->SetHeading(fTargetAngle);
                            }
                        }
                        bhkRigidBody::SetFixed(p3D, 0, 0);
                        bhkBlendController::DoGetUp(p3D, 0.0f);
                        apActor->RemoveChange(CHANGE_REFR_HAVOK_MOVE);
                        apActor->m_Extra.SetRagDollData(nullptr);
                        cKnockState = 6;
                    }
                    else if (!pAnim->spAnimIdle.m_pObject && !pAnim->spAnimIdleQueued.m_pObject) {
                        cKnockState = 3;
                    }
                    break;
                }
                case 6: {
                    if (pAnim->SpecialIdleDonePlaying()) {
                        cKnockState = 0;
                        pAnim->SpecialIdleFree(true, false);
                        if (apActor->pRagdollController) {
                            apActor->pRagdollController->EnableRagdollFeedback(true);
                            apActor->pRagdollController->EnableRagdollAnim();
                        }
                        apActor->SetHavokWeapon();
                        if (pWeapon &&
                            pWeaponBone &&
                            (bWeaponGrenade || bWeaponThrown) &&
                            pWeaponBone->GetAtChecked(0)) {
                            pWeaponBone->GetAt(0)->SetAppCulled(false);
                            if (apActor == pPlayer &&
                                MiddleHighProcess::pThirdPersonPlayerWeaponNode &&
                                MiddleHighProcess::pThirdPersonPlayerWeaponNode->GetAtChecked(0)) {
                                MiddleHighProcess::pThirdPersonPlayerWeaponNode->GetAt(0)->SetAppCulled(false);
                            }
                        }
                    }
                    break;
                }
                default: {
                    return;
                }
            }
        }
        else {
            cKnockState = 0;
        }
    }
}

void MiddleHighProcess::ClearKnockState(Actor* apActor) {
    if (cKnockState && !(apActor->GetFatigue() < 0.0) && !apActor->IsParalyzed()) {
        NiNode* p3D = apActor->TESObjectREFR::Get3D();
        bhkRigidBody::SetFixed(p3D, 0, 1);
        bhkBlendController::DoGetUp(p3D, 1.0);
        apActor->GetExtra()->SetRagDollData(nullptr);
        apActor->RemoveChange(CHANGE_REFR_HAVOK_MOVE);
        BSFaceGenAnimationData* pAnimData = apActor->GetFaceAnimationData();
        if (pAnimData) {
            pAnimData->SetDead(false, false);
        }
        cKnockState = 0;
        if (pWeapon && pWeaponBone && (bWeaponGrenade || bWeaponThrown) && pWeaponBone->GetAtChecked(0)) {
            pWeaponBone->GetAt(0)->SetAppCulled(false);
            if (apActor == pPlayer &&
                MiddleHighProcess::pThirdPersonPlayerWeaponNode &&
                MiddleHighProcess::pThirdPersonPlayerWeaponNode->GetAtChecked(0)) {
                MiddleHighProcess::pThirdPersonPlayerWeaponNode->GetAt(0)->SetAppCulled(false);
            }
        }
    }
}

void MiddleHighProcess::SetAimSequence(AIM_ENUM aeDirection, BSAnimGroupSequence* apSeq) {
    pAimSequence[aeDirection] = apSeq;
    if (apSeq) {
        if (aeDirection) {
            apSeq->NiControllerSequence::SetSequenceWeight(0.0);
        }
        else {
            apSeq->NiControllerSequence::SetSequenceWeight(1.0);
        }
    }
}

void MiddleHighProcess::UpdateAimSequences(Actor* apActor) {
    float fUpWeight = 0.0;
    float fDownWeight = 0.0;
    double dLooking = appTimer.GetSeconds() * fAimLooking;
    double dChase = fAimChaseLookingMult.Float() * dLooking;
    float fNewLooking = fAimLooking - fAnimationMult.Float() * dChase;
    if (fAimLooking > 0.0 && fNewLooking < 0.0) {
        fAimLooking = 0.0;
    }
    else if (fAimLooking < 0.0 && fNewLooking > 0.0) {
        fAimLooking = 0.0;
    }
    else if (fNewLooking == 0.0 || !(fNewLooking > -9.999999974752427e-07) || !(fNewLooking < 9.999999974752427e-07)) {
        fAimLooking = fNewLooking;
    }
    else {
        fAimLooking = 0.0;
    }
    float fTotalLooking = apActor->GetLooking() + fAimLooking;
    if (fTotalLooking < 0.0 && pAimSequence[1]) {
        float fRatio = fTotalLooking / (fAimUpDegrees.Float() * -0.01745329238474369);
        fUpWeight = BSMIN(1.0f, fRatio);
    }
    else if (fTotalLooking > 0.0 && pAimSequence[2]) {
        float fRatio = fTotalLooking / (fAimDownDegrees.Float() * 0.01745329238474369);
        fDownWeight = BSMIN(1.0f, fRatio);
    }
    float fZero = 0.0f + 0.0f;
    float fSum = fUpWeight + fDownWeight;
    float fCenterWeight = 1.0 - BSMAX(fSum, fZero);
    if (pAimSequence[0]) {
        pAimSequence[0]->SetSequenceWeight(fCenterWeight);
    }
    if (pAimSequence[1]) {
        pAimSequence[1]->SetSequenceWeight(fUpWeight);
    }
    if (pAimSequence[2]) {
        pAimSequence[2]->SetSequenceWeight(fDownWeight);
    }
}

float MiddleHighProcess::GetAimLooking() {
    return fAimLooking;
}

void MiddleHighProcess::AddGunDrift(float afDegrees) {
    fAimLooking = static_cast<float>(fAimLooking - (afDegrees * 0.01745329238474369));
}

void MiddleHighProcess::ClearAutomaticShotsToFire(bool abStartShooting) {
    if (pWeapon && pAmmo) {
        TESObjectWEAP* pWeaponForm = static_cast<TESObjectWEAP*>(pWeapon->pContainerObj);
        if (pWeaponForm->IsAutomaticWeapon() && abStartShooting) {
            double dWeapFireRate = pWeaponForm->data.fShotsPerSec;
            bool bModActive = pWeapon->HasModEffectActive(8);
            float fSPS = pWeaponForm->GetAttackSpeed(bModActive) * dWeapFireRate;
            fAutomaticShotDelay = 1.0 / fSPS;
        }
        else {
            fAutomaticShotDelay = 0.0f;
        }
    }
    else {
        fAutomaticShotDelay = 0.0f;
    }
}

uint8_t MiddleHighProcess::GetAutomaticShotsToFire(Actor* apActor) {
    float fCharacterTime;
    if (!pWeapon) {
        return 0;
    }
    TESObjectWEAP* pWeaponForm = static_cast<TESObjectWEAP*>(pWeapon->pContainerObj);
    if (!pWeaponForm->IsAutomaticWeapon()) {
        return 0;
    }
    double dSeconds = appTimer.fDelta;
    float fModifiedTime = fAnimationMult.Float() * dSeconds;
    if (GetAnimation()) {
        fCharacterTime = GetAnimation()->GetGlobalTimeMultiplier();
    }
    else {
        fCharacterTime = 1.0f;
    }
    if (apActor == pPlayer) {
        fAutomaticShotDelay = fAutomaticShotDelay - pVATS.GetPlayerUpdateMult() * fModifiedTime * fCharacterTime;
    }
    else if (apActor == pVATSTarget) {
        double dDelta = appTimer.fDelta;
        double dScaled = fAnimationMult.Float() * dDelta * fCharacterTime;
        fAutomaticShotDelay = fAutomaticShotDelay - pVATS.GetTargetUpdateMult() * dScaled;
    }
    else {
        double dDelta = appTimer.fDelta;
        fAutomaticShotDelay = fAutomaticShotDelay - fAnimationMult.Float() * dDelta * fCharacterTime;
    }
    double dFireRate = pWeaponForm->data.fShotsPerSec;
    bool bModActive = pWeapon->HasModEffectActive(8);
    float fSPS = pWeaponForm->GetAttackSpeed(bModActive) * dFireRate;
    uint8_t ucShots = 0;
    while (fAutomaticShotDelay < 0.0) {
        float fOneShot = 1.0 / fSPS;
        fAutomaticShotDelay = fAutomaticShotDelay + fOneShot;
        ++ucShots;
    }
    return ucShots;
}

void MiddleHighProcess::SetShotsToFire(uint8_t acShots) {
    cShotsToFire = acShots;
}

uint8_t MiddleHighProcess::GetShotsToFire() {
    return cShotsToFire;
}

float MiddleHighProcess::GetAutomaticShotDelay() {
    return fAutomaticShotDelay;
}

bool MiddleHighProcess::GetWantWeaponDrawn() {
    return bWantWeaponDrawn;
}

void MiddleHighProcess::SetWantWeaponDrawn(bool abWantDrawn) {
    bWantWeaponDrawn = abWantDrawn;
}

bool MiddleHighProcess::GetWeaponDrawn() {
    return bWeaponDrawn;
}

// Sets the weapon drawn flag, sheathing the weapon on the actor's animation when it goes down, then reloads the
// drawn weapon of an NPC (with the clip size mod applied) and marks the walk/run state dirty.
void MiddleHighProcess::SetWeaponDrawn(Actor* apActor, bool abDrawn) {
    if (bWeaponDrawn != abDrawn) {
        bWeaponDrawn = abDrawn;
        if (apActor && !bWeaponDrawn) {
            Animation* pAnim = apActor->GetAnimation();
            BipedAnim* pBiped = apActor->GetBiped();
            ForceWeaponDrawnSheathed(bWeaponDrawn, pBiped, pAnim, apActor);
        }
    }
    if (apActor && apActor != pPlayer && bWeaponDrawn && pWeapon) {
        bool bUseModValue = pWeapon->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE);
        TESObjectWEAP* pWeap = static_cast<TESObjectWEAP*>(pWeapon->GetContainerObject());
        apActor->ReloadWeaponNV(pWeap, 0, bUseModValue, false);
    }
    apActor->SetWalkRunFlagDirty();
}

void MiddleHighProcess::Set3DUpdateFlag(RESET_3D_FLAGS aeFlag) {
    cUpdate3DModel |= aeFlag;
}

// Departure: the exe multiplies the flags by ~aeFlag, which sets unrelated bits instead of clearing aeFlag.
void MiddleHighProcess::Clear3DUpdateFlag(RESET_3D_FLAGS aeFlag) {
    cUpdate3DModel &= ~static_cast<int8_t>(aeFlag);
}

void MiddleHighProcess::ClearAll3DUpdateFlags() {
    cUpdate3DModel = 0;
}

bool MiddleHighProcess::Get3DUpdateFlag(RESET_3D_FLAGS aeFlag) {
    return (cUpdate3DModel & aeFlag) != 0;
}

SIT_SLEEP_STATE MiddleHighProcess::GetSitSleepState() {
    if ((GetPostAnimationActions() & kPostAnimationSitSleepActive) != 0 &&
        (cSitSleepState == 4 || cSitSleepState == 9)) {
        return static_cast<SIT_SLEEP_STATE>(cSitSleepState + 1);
    }
    return static_cast<SIT_SLEEP_STATE>(cSitSleepState);
}

// Entering a sit or sleep state counts the actor's Havok forced adds up and marks the furniture marker used; leaving
// one counts them down (queued to the 3D task thread when it runs), never for the player. Sitting or sleeping down
// shrinks the character controller and drops its character collisions; standing or waking restores them.
void MiddleHighProcess::SetSitSleepState(
    Actor* apActor,
    SIT_SLEEP_STATE aeState,
    TESObjectREFR* apFurnitureRef,
    uint8_t acFurnitureIndex
) {
    NiNode* p3D = apActor ? apActor->Get3D() : nullptr;
    if (p3D && apActor != pPlayer) {
        bool bWasSitSleep = cSitSleepState == SIT_SLEEP_IS_SLEEPING || cSitSleepState == SIT_SLEEP_IS_SITTING;
        bool bIsSitSleep = aeState == SIT_SLEEP_IS_SLEEPING || aeState == SIT_SLEEP_IS_SITTING;
        if (bWasSitSleep && !bIsSitSleep) {
            if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                TaskQueueInterface::GetInstance()->QueueHavokAdjustForceAddCount(p3D, false, true, false);
            }
            else {
                bhkWorld::AdjustForceAddCount(p3D, false, true, false);
            }
        }
        else if (bIsSitSleep && !bWasSitSleep) {
            if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                TaskQueueInterface::GetInstance()->QueueHavokAdjustForceAddCount(p3D, true, true, false);
            }
            else {
                bhkWorld::AdjustForceAddCount(p3D, true, true, false);
            }
            if (apFurnitureRef) {
                apFurnitureRef->SetMarkerUsed(acFurnitureIndex, true);
            }
        }
    }
    if (apActor) {
        apActor->SetLooking(apActor->GetLooking());
        bhkCharacterController* pController = static_cast<bhkCharacterController*>(apActor->GetCharController());
        if (pController) {
            BaseProcess* pProcess = this;
            int32_t iMarker = FurnitureMarker.cNumber;
            bool bDisableController = pProcess->ShouldDisableCharController(apFurnitureRef, iMarker);
            if (aeState == SIT_SLEEP_WANT_TO_SIT ||
                aeState == SIT_SLEEP_WANT_TO_SLEEP ||
                (cSitSleepState == SIT_SLEEP_LOAD_SIT_IDLE && aeState == SIT_SLEEP_IS_SITTING) ||
                (cSitSleepState == SIT_SLEEP_LOAD_SLEEP_IDLE && aeState == SIT_SLEEP_IS_SLEEPING)) {
                if (bDisableController) {
                    apActor->SetSize(0.1f);
                    pController->SetFlag(bhkCharacterController::NO_CHARACTER_COLLISIONS, true);
                }
                apActor->SetChaseBip(true);
            }
            else if (
                aeState == SIT_SLEEP_WANT_TO_STAND || aeState == SIT_SLEEP_WANT_TO_WAKE || aeState == SIT_SLEEP_NORMAL
            ) {
                apActor->SetSize(0.0f);
                pController->SetFlag(bhkCharacterController::NO_CHARACTER_COLLISIONS, false);
            }
        }
    }
    cSitSleepState = static_cast<uint8_t>(aeState);
    pCurrentFurniture = apFurnitureRef;
    cCurrentFurnitureIndex = acFurnitureIndex;
    apActor->SetCachedSitSleepState(aeState);
}

// The number of the furniture marker the actor uses, 0 without furniture, marker (index 127) or marker data.
uint32_t MiddleHighProcess::GetFurnitureMarkerID() {
    if (!pCurrentFurniture || cCurrentFurnitureIndex == 127) {
        return 0;
    }
    BSFurnitureMarker* pMarker = BSFurnitureMarker::Find(pCurrentFurniture->Get3D());
    if (!pMarker) {
        return 0;
    }
    FurnitureMark kMark = pMarker->QMark(cCurrentFurnitureIndex);
    return kMark.cNumber;
}

TESObjectREFR* MiddleHighProcess::GetCurrentFurniture() {
    return pCurrentFurniture;
}

void MiddleHighProcess::RemoveFurnitureRef(TESObjectREFR* apFurnitureRef) {
    RefListChairBed.Remove(*reinterpret_cast<void**>(&apFurnitureRef));
    if (pCurrentFurniture == apFurnitureRef) {
        pCurrentFurniture = nullptr;
    }
}

uint32_t MiddleHighProcess::GetCurrentFurnitureIndex() {
    return cCurrentFurnitureIndex;
}

FurnitureMark* MiddleHighProcess::GetFurnitureMarker() {
    return &FurnitureMarker;
}

// Plays the idle the actor uses at its furniture; for the player in third person the first-person animation gets the
// first-person idle too, picked while third person is forced off.
bool MiddleHighProcess::LoadDynamicIdle(Actor* apActor) {
    TESIdleForm* pIdle = pIdleManager->GetIdleToPlay(apActor, pCurrentFurniture);
    Animation* pAnimation;
    if (apActor == pPlayer) {
        pAnimation = pPlayer->GetAnimation(!pPlayer->Is3rdPerson());
    }
    else {
        pAnimation = apActor->GetAnimation();
    }
    if (!pIdle || !pAnimation) {
        return false;
    }
    pAnimation->SpecialIdleReplace(pIdle, apActor, pIdle->GetIdleAnimGroupSection());
    if (apActor != pPlayer || !pPlayer->Is3rdPerson()) {
        return true;
    }
    pPlayer->Force3rdPerson(false);
    TESIdleForm* pFirstPersonIdle = pIdleManager->GetIdleToPlay(apActor, pCurrentFurniture);
    if (pFirstPersonIdle) {
        Animation* pFirstPersonAnimation = pPlayer->GetAnimation(true);
        if (pFirstPersonAnimation) {
            pFirstPersonAnimation
                ->SpecialIdleReplace(pFirstPersonIdle, apActor, pFirstPersonIdle->GetIdleAnimGroupSection());
        }
    }
    pPlayer->Force3rdPerson(true);
    return true;
}

void MiddleHighProcess::LoadSitSleepIdle(Actor* apActor, Animation* apAnimation) {
    if (pCurrentFurniture) {
        BSFurnitureMarker* pMarkData = BSFurnitureMarker::Find(pCurrentFurniture->Get3D());
        if (pMarkData && static_cast<uint32_t>(this->cCurrentFurnitureIndex) < pMarkData->QNumMarks()) {
            char cRestoreSitSleepState = cSitSleepState;
            switch (cRestoreSitSleepState) {
                case 1:
                case 2:
                case 3:
                case 4: {
                    cSitSleepState = 1;
                    break;
                }
                case 6:
                case 7:
                case 8:
                case 9: {
                    cSitSleepState = 6;
                    break;
                }
                default: {
                    break;
                }
            }
            apActor->SetCachedSitSleepState(static_cast<SIT_SLEEP_STATE>(cSitSleepState));
            if (cSitSleepState) {
                pCurrentFurniture->SetMarkerUsed(this->cCurrentFurnitureIndex, true);
                bool b3rdPerson = false;
                if (apActor == pPlayer) {
                    b3rdPerson = pPlayer->Is3rdPerson();
                    if (apAnimation != pPlayer->GetAnimation(!b3rdPerson)) {
                        pPlayer->Force3rdPerson(!b3rdPerson);
                    }
                }
                TESIdleForm* pIdle = pIdleManager->GetIdleToPlay(apActor, pCurrentFurniture);
                if (pIdle) {
                    apAnimation->SpecialIdleReplace(pIdle, apActor, pIdle->GetIdleAnimGroupSection());
                }
                if (apActor == pPlayer) {
                    pPlayer->Force3rdPerson(b3rdPerson);
                }
            }
            cSitSleepState = cRestoreSitSleepState;
            uint32_t uiType = FurnitureMarker.GetType();
            bool bDisableCharController = ShouldDisableCharController(pCurrentFurniture, uiType);
            bhkCharacterController* pCharController = GetCharController();
            if (bDisableCharController && cSitSleepState) {
                apActor->SetSize(0.1f);
                apActor->SetChaseBip(true);
                if (pCharController) {
                    pCharController->SetFlag(bhkCharacterController::NO_CHARACTER_COLLISIONS, true);
                }
            }
            else {
                apActor->SetSize(0.0f);
                apActor->SetChaseBip(false);
                if (pCharController) {
                    pCharController->SetFlag(bhkCharacterController::NO_CHARACTER_COLLISIONS, false);
                }
            }
            apActor->SetCachedSitSleepState(static_cast<SIT_SLEEP_STATE>(cSitSleepState));
        }
        else {
            ClearFurniture(apActor);
        }
    }
}

bool MiddleHighProcess::HasBeenAttacked() {
    return bBeenAttacked;
}

void MiddleHighProcess::SetBeenAttacked(bool abFlag) {
    bBeenAttacked = abFlag;
}

BSShaderPPLightingProperty* MiddleHighProcess::GetLightingShaderProperty() {
    return pLightingProperty;
}

void MiddleHighProcess::SetLightingShaderProperty(BSShaderPPLightingProperty* apProperty) {
    pLightingProperty = apProperty;
}

void MiddleHighProcess::CalculateLightValue(Actor* apActor, bool abIgnoreTorch) {
    float fLight = 0.0f;
    if (GetLightLevelTimer() <= 0.0) {
        if (pLightingProperty.m_pObject) {
            NiPointer<NiLight> pLight(static_cast<NiLight*>(nullptr));
            float fLightPoint = 0.0f;
            float fAmbientPoint = 0.0f;
            float fDirPoint = 0.0f;
            NiTListIterator kIter = nullptr;
            ShadowSceneLight* pShadowLight = pLightingProperty->GetNextLight(kIter);
            NiLight* pTorch = nullptr;
            if (abIgnoreTorch) {
                LightRefData* pRefLight = apActor->GetLight();
                if (pRefLight) {
                    pTorch = pRefLight->spLight.m_pObject;
                }
            }
            if (!pShadowLight && apActor == pPlayer) {
                // Fetches the first-person 3D and does not use it.
                pPlayer->Get3D(true);
                pShadowLight = pLightingProperty->GetNextLight(kIter);
            }
            ShadowSceneNode* pShadowNode = BSShaderManager::GetShadowSceneNode(0);
            if (pShadowNode) {
                ShadowSceneLight* pSunLight = pShadowNode->pSunLight;
                int32_t iNumberLights = 0;
                NiPoint3 kPoint = apActor->GetLocationOnReference();
                NiPoint3 kBoundMax = apActor->GetBoundMax();
                float fTargetHeight = kBoundMax.z - apActor->GetBoundMin().z - 1.0;
                fTargetHeight = fTargetHeight / 2.0;
                kPoint.z = kPoint.z + fTargetHeight;
                if (apActor == pPlayer) {
                    fLightPoint = pShadowNode
                                      ->GetLuminanceAtPoint(kPoint, iNumberLights, fAmbientPoint, fDirPoint, pTorch);
                }
                else {
                    while (pShadowLight) {
                        fLightPoint = pShadowLight->GetLuminanceAtPoint(kPoint, pTorch) + fLightPoint;
                        pShadowLight = static_cast<ShadowSceneLight*>(GetNext(kIter));
                        ++iNumberLights;
                    }
                    if (pSunLight) {
                        NiColor kColor = pSunLight->GetLight()->m_kAmb;
                        float fAmbientLum = BSMAX(kColor.r, BSMAX(kColor.g, kColor.b));
                        kColor = pSunLight->GetLight()->m_kDiff;
                        float fDirectionalLum = BSMAX(kColor.r, BSMAX(kColor.g, kColor.b));
                        fAmbientPoint = fAmbientLum + fDirectionalLum;
                    }
                }
                if (apActor->GetParentCell() && apActor->GetParentCell()->GetInterior()) {
                    fLightPoint = fLightPoint + fAmbientPoint;
                }
                else if (pSunLight) {
                    fLightPoint = pSunLight->GetLuminanceAtPoint(kPoint, nullptr) + fLightPoint;
                }
                fLight = fLightPoint * 100.0;
            }
            else {
                NiNode* p3D = apActor->TESObjectREFR::Get3D();
                apActor->InitLightingPropertyPtr(p3D);
                fLight = 0.0f;
            }
        }
        if (fLight > 100.0) {
            fLight = 100.0f;
        }
        else if (fLight < 0.0) {
            fLight = 0.0f;
        }
        SetActorLightLevel(fLight);
        float fTimer = fLight * 0.01 + 3.0;
        SetLightLevelTimer(fTimer);
    }
    else {
        // Keeps the timer as a double and subtracts the BSTimer delta.
        double fTimer = GetLightLevelTimer();
        float fNewTimer = fTimer - appTimer.fDelta;
        SetLightLevelTimer(fNewTimer);
    }
}

void MiddleHighProcess::ClearCurrentDataforProcess(Actor* apActor) {
    iAnimActionSuccess = 2;
    SetCurrentActionComplete(false);
    SetCurrentMovementComplete(true);
    SetTarget(nullptr);
    ClearMovementStoped();
    ClearReservation();
    SetPackageIdleDone(false);
    pCurrentPackageSpell = nullptr;
    delete pAcquireObject;
    pAcquireObject = nullptr;
    apActor->ClearLookAtTarget();
    apActor->SetForceSneak(false);
    ClearAutomaticShotsToFire(false);
    bDoneClothesChange = false;
    while (!ObjectList.IsEmpty()) {
        ObjectstoAcquire* pObj = ObjectList.m_item;
        delete pObj;
        ObjectList.Remove(pObj);
    }
    pCurrentIdle = nullptr;
    bPlayedBeginIdles = false;
    bPlayedEndIdles = false;
    RefListChairBed.RemoveAll();
    GenericLocationsList.RemoveAll();
    pGenericLocation = nullptr;
    if (apActor->GetSitSleepState() == SIT_SLEEP_NORMAL && GetCurrentFurniture()) {
        ClearFurniture(apActor);
    }

    ClearUseWeaponHeadTrackTarget(true);
}

void MiddleHighProcess::SetCommandingActor(Actor* apActor) {
    pCommandingActor = apActor;
}

Actor* MiddleHighProcess::GetCommandingActor() {
    return pCommandingActor;
}

void MiddleHighProcess::SetLastBoundWeapon(TESObjectWEAP* apBoundWeapon) {
    pLastBoundWeapon = apBoundWeapon;
}

void MiddleHighProcess::SetFurniture(TESObjectREFR* pref, uint8_t aiindex, FurnitureMark* apMarker) {
    pCurrentFurniture = pref;
    cCurrentFurnitureIndex = aiindex;
    if (apMarker) {
        FurnitureMarker.cNumber = apMarker->cNumber;
        FurnitureMarker.Position = apMarker->Position;
        FurnitureMarker.sHeading = apMarker->sHeading;
        if (pCurrentFurniture && !pSaveLoadGame->GetSaveGameLoading()) {
            pCurrentFurniture->SetMarkerReserved(aiindex, true);
        }
    }
}

// Re-equips the actor's worn weapon and reattaches the weapon and ammo 3D. On PC the ammo calls do nothing; they are
// made in the order below.
void MiddleHighProcess::RemoveAndAttachItems(MobileObject* apMob) {
    Actor* pActor = dynamic_cast<Actor*>(apMob);
    if (!pActor) {
        return;
    }
    InventoryChanges* pChanges = apMob->GetExtra()->GetContainerChanges();
    if (!pChanges) {
        return;
    }
    pActor->ReloadTargets(false);
    ItemChange* pWornWeapon = pChanges->GetWornItem(BO_WEAPON, false);
    BaseProcess* pProcess = this;
    pProcess->SetCurrentWeapon(pWornWeapon, nullptr, false);
    apMob->RemoveWeapon();
    if (pWeapon) {
        apMob->AttachWeapon(static_cast<TESObjectWEAP*>(pWeapon->GetContainerObject()));
    }
    apMob->RemoveAmmo();
    if (pAmmo) {
        apMob->AttachAmmo(static_cast<TESAmmo*>(pAmmo->GetContainerObject()));
    }
    apMob->AttachAmmo(nullptr);
    apMob->RemoveAmmo();
}

uint16_t MiddleHighProcess::GetSaveSize(uint32_t aiFlags, MobileObject* pmob) {
    Actor* pActor = dynamic_cast<Actor*>(pmob);
    uint16_t usSaveSize = MiddleLowProcess::GetSaveSize(aiFlags, pmob);
    uint16_t usBaseSize = usSaveSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        usSaveSize += 6;
    }
    int16_t sSize = usSaveSize + 2;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 52u) {
        sSize += 5;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 77u) {
        sSize += 4;
    }
    sSize += 49;
    if ((aiFlags & CHANGE_REFR_ANIMATION) != 0) {
        sSize += ComputeAnimationSaveSize(static_cast<PlayerCharacter*>(pmob), pAnimation);
    }
    uint16_t usSize = ActiveEffect::GetActiveEffectListSaveSize(pActiveEffectList, pActor) + (sSize + 8);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 69u) {
        ++usSize;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 73u) {
        usSize += 2;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 101u) {
        usSize += 6 + 4 * DeadDetectList.ItemsInList();
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 109u) {
        ++usSize;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        usSize += 4;
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                usSize - usBaseSize,
                pHeader->iFormID,
                pForm->GetFormEditorID(),
                pHeader->iFlags,
                0x1B89,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                usSize - usBaseSize,
                0x1B89,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp"
            );
        }
    }
    return usSize;
}

// Saves the BGS-format data of the process: the low process's own, then the weapon, the furniture, the magic, the
// dead-detect list and the queued items. The animation is written as a variable-sized value between two buffer
// positions, and only when the form's change flags ask for it.
void MiddleHighProcess::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    Actor* pActor = apSaveGameBuffer->GetActor();
    MiddleLowProcess::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bWantWeaponDrawn), sizeof(bWantWeaponDrawn), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bWeaponDrawn), sizeof(bWeaponDrawn), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bCheckMagicNode), sizeof(bCheckMagicNode), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fAlphaMult), sizeof(fAlphaMult), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fScriptRefractPower), sizeof(fScriptRefractPower), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eUseItem), sizeof(eUseItem), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bBeenAttacked), sizeof(bBeenAttacked), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&lastSeenPostion), sizeof(lastSeenPostion), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fEquippedWeight), sizeof(fEquippedWeight), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cSitSleepState), sizeof(cSitSleepState), 0);
    apSaveGameBuffer
        ->SaveDataEndian(reinterpret_cast<char*>(&cCurrentFurnitureIndex), sizeof(cCurrentFurnitureIndex), 0);
    apSaveGameBuffer
        ->SaveDataEndian(reinterpret_cast<char*>(&FurnitureMarker.cNumber), sizeof(FurnitureMarker.cNumber), 0);
    apSaveGameBuffer
        ->SaveDataEndian(reinterpret_cast<char*>(&FurnitureMarker.sHeading), sizeof(FurnitureMarker.sHeading), 0);
    apSaveGameBuffer
        ->SaveDataEndian(reinterpret_cast<char*>(&FurnitureMarker.Position), sizeof(FurnitureMarker.Position), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cKnockState), sizeof(cKnockState), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bDoneOnce), sizeof(bDoneOnce), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bPickPocketed), sizeof(bPickPocketed), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bDoneClothesChange), sizeof(bDoneClothesChange), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fPursueTimer), sizeof(fPursueTimer), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bSummonedCreature), sizeof(bSummonedCreature), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fAimLooking), sizeof(fAimLooking), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fAutomaticShotDelay), sizeof(fAutomaticShotDelay), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cShotsToFire), sizeof(cShotsToFire), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bSustainedFire), sizeof(bSustainedFire), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bIronSights), sizeof(bIronSights), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iAnimActionSuccess), sizeof(iAnimActionSuccess), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iReservationSlot), sizeof(iReservationSlot), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bAimingTarget), sizeof(bAimingTarget), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iPackageIdleNumber), sizeof(iPackageIdleNumber), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fPackageIdleTimer), sizeof(fPackageIdleTimer), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bPlayedBeginIdles), sizeof(bPlayedBeginIdles), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bPlayedEndIdles), sizeof(bPlayedEndIdles), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fRadiationDelta), sizeof(fRadiationDelta), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fRadiationMagicDelta), sizeof(fRadiationMagicDelta), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fRadiationWaterDelta), sizeof(fRadiationWaterDelta), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iWeaponConditionStage), sizeof(iWeaponConditionStage), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bPickPackIdle), sizeof(bPickPackIdle), 0);
    apSaveGameBuffer->SaveFormID(pLastIdlePlayed, 0);
    apSaveGameBuffer->SaveFormID(pCurrentIdle, 0);
    apSaveGameBuffer->SaveFormID(pCommandingActor, 0);
    apSaveGameBuffer->SaveFormID(pCurrentFurniture, 0);
    int32_t iAmmoCount = 0;
    if (pAmmo) {
        iAmmoCount = pAmmo->GetCount();
    }
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iAmmoCount), sizeof(iAmmoCount), 0);
    int32_t iDeadCount = 0;
    uint32_t uiDeadPosition = apSaveGameBuffer->StartVariableSizedValue();
    BSSimpleList<void*>* pNode = &DeadDetectList;
    while (pNode) {
        void* pItem = pNode->GetItem();
        if (pItem) {
            apSaveGameBuffer->SaveFormID(static_cast<TESForm*>(pItem), 0);
            ++iDeadCount;
        }
        pNode = pNode->GetNext();
    }
    apSaveGameBuffer->SaveVariableSizedValue(iDeadCount, uiDeadPosition);
    RunOncePackage.SaveGame(apSaveGameBuffer);
    if (pActor && apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_ANIMATION)) {
        uint32_t uiAnimPosition = apSaveGameBuffer->StartVariableSizedValue();
        uint32_t uiAnimStart = apSaveGameBuffer->GetBufferPosition();
        pAnimation->SaveGameBGS(apSaveGameBuffer);
        uint32_t uiAnimEnd = apSaveGameBuffer->GetBufferPosition();
        apSaveGameBuffer->SaveVariableSizedValue(uiAnimEnd - uiAnimStart, uiAnimPosition);
    }

    ActiveEffect::SaveActiveEffectList(
        apSaveGameBuffer,
        static_cast<BSSimpleList<ActiveEffect*>*>(static_cast<void*>(pActiveEffectList))
    );
    uint32_t uiPackageSpellID = pCurrentPackageSpell ? pCurrentPackageSpell->GetMagicItemFormID() : 0;
    apSaveGameBuffer->SaveFormID(uiPackageSpellID, 0);
    uint32_t uiSpellID = pCurrentSpell ? pCurrentSpell->GetMagicItemFormID() : 0;
    apSaveGameBuffer->SaveFormID(uiSpellID, 0);
    uint32_t uiTargetID = pDesiredTarget ? pDesiredTarget->GetMagicTargetFormID() : 0;
    apSaveGameBuffer->SaveFormID(uiTargetID, 0);
    int32_t iQueuedCount = 0;
    uint32_t uiQueuedPosition = apSaveGameBuffer->StartVariableSizedValue();
    BSSimpleList<QueuedItem*>* pQueuedNode = pListItemstoEquipUnequip;
    while (pQueuedNode) {
        QueuedItem* pQueuedItem = pQueuedNode->GetItem();
        if (pQueuedItem) {
            pQueuedItem->SaveGame(apSaveGameBuffer);
            ++iQueuedCount;
        }
        pQueuedNode = pQueuedNode->GetNext();
    }
    apSaveGameBuffer->SaveVariableSizedValue(iQueuedCount, uiQueuedPosition);
}

void MiddleHighProcess::SaveGame(uint32_t aiFlags, MobileObject* pmob) {
    uint32_t iCurrentPackageSpellID;
    uint32_t iRefID;
    uint32_t iBlockHeader;
    float fHeading;
    uint16_t iBlockSize;
    uint8_t cNumber;
    uint32_t iDesiredTargetID;
    uint32_t iCommandingActorID;
    uint32_t iCurrentSpellID;
    NiPoint3 kPosition;
    uint32_t iFurnitureID;
    Actor* pActor = dynamic_cast<Actor*>(pmob);
    MiddleLowProcess::SaveGame(aiFlags, pmob);
    iBlockSize = 0;
    // The block's size placeholder in the save buffer, patched at the end.
    void* pBlockSize = nullptr;
    uint8_t* iTempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        iTempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        iBlockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iBlockHeader, sizeof(iBlockHeader));
        pBlockSize = pSaveLoadGameOLD->m_pBuffer;
        pSaveLoadGameOLD->SaveGameDataOLD(&iBlockSize, sizeof(iBlockSize));
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&bWantWeaponDrawn, sizeof(bWantWeaponDrawn));
    pSaveLoadGameOLD->SaveGameDataOLD(&bWeaponDrawn, sizeof(bWeaponDrawn));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 52u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&bCheckMagicNode, sizeof(bCheckMagicNode));
        pSaveLoadGameOLD->SaveGameDataOLD(&fAlphaMult, sizeof(fAlphaMult));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 77u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&fScriptRefractPower, sizeof(fScriptRefractPower));
    }
    iCommandingActorID = 0;
    if (pCommandingActor) {
        iCommandingActorID = pCommandingActor->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&iCommandingActorID, 4u);
    pSaveLoadGameOLD->SaveGameDataOLD(&eUseItem, sizeof(eUseItem));
    pSaveLoadGameOLD->SaveGameDataOLD(&bBeenAttacked, sizeof(bBeenAttacked));
    pSaveLoadGameOLD->SaveGameDataOLD(&lastSeenPostion, sizeof(lastSeenPostion));
    pSaveLoadGameOLD->SaveGameDataOLD(&fEquippedWeight, sizeof(fEquippedWeight));
    pSaveLoadGameOLD->SaveGameDataOLD(&cSitSleepState, sizeof(cSitSleepState));
    pSaveLoadGameOLD->SaveGameDataOLD(&cCurrentFurnitureIndex, sizeof(cCurrentFurnitureIndex));
    iFurnitureID = 0;
    if (pCurrentFurniture) {
        iFurnitureID = pCurrentFurniture->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&iFurnitureID, 4u);
    fHeading = FurnitureMarker.GetHeading();
    cNumber = FurnitureMarker.cNumber;
    kPosition = FurnitureMarker.Position;
    pSaveLoadGameOLD->SaveGameDataOLD(&fHeading, sizeof(fHeading));
    pSaveLoadGameOLD->SaveGameDataOLD(&cNumber, sizeof(cNumber));
    pSaveLoadGameOLD->SaveGameDataOLD(&kPosition, sizeof(kPosition));
    pSaveLoadGameOLD->SaveGameDataOLD(&cKnockState, sizeof(cKnockState));
    if ((aiFlags & CHANGE_REFR_ANIMATION) != 0) {
        Animation::SaveAnimation(pmob, pAnimation);
    }
    iCurrentSpellID = 0;
    if (pCurrentSpell) {
        iCurrentSpellID = pCurrentSpell->GetMagicItemFormID();
    }
    pSaveLoadGameOLD->SaveNumericID(&iCurrentSpellID, 4u);
    iDesiredTargetID = 0;
    if (pDesiredTarget) {
        iDesiredTargetID = pDesiredTarget->GetMagicTargetFormID();
    }
    pSaveLoadGameOLD->SaveNumericID(&iDesiredTargetID, 4u);
    ActiveEffect::SaveActiveEffectList(pActiveEffectList, pActor);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 69u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&bDoneOnce, sizeof(bDoneOnce));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 73u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&bPickPocketed, sizeof(bPickPocketed));
        pSaveLoadGameOLD->SaveGameDataOLD(&bDoneClothesChange, sizeof(bDoneClothesChange));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 101u) {
        uint16_t iCount;
        pSaveLoadGameOLD->SaveGameDataOLD(&fPursueTimer, sizeof(fPursueTimer));
        iCount = 0;
        uint16_t* pCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&iCount, sizeof(iCount));
        for (BSSimpleList<void*>* pList = &DeadDetectList; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
            TESObjectREFR* pRef = static_cast<TESObjectREFR*>(pList->m_item);
            iRefID = 0;
            if (pRef) {
                iRefID = pRef->iFormID;
            }
            pSaveLoadGameOLD->SaveNumericID(&iRefID, 4u);
            ++iCount;
        }
        *pCount = iCount;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 109u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&bSummonedCreature, sizeof(bSummonedCreature));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        iCurrentPackageSpellID = 0;
        if (pCurrentPackageSpell) {
            iCurrentPackageSpellID = pCurrentPackageSpell->GetMagicItemFormID();
        }
        pSaveLoadGameOLD->SaveNumericID(&iCurrentPackageSpellID, 4u);
    }
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            uint32_t iFlags = pHeader->iFlags;
            const char* pEditorID = pForm->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - iTempStartPosition),
                pHeader->iFormID,
                pEditorID,
                iFlags,
                7185,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - iTempStartPosition),
                7185,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        char* pBlockEnd = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
        char* pBlockStart = static_cast<char*>(pBlockSize);
        if (pBlockEnd > pBlockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp",
                7185
            );
        }
        *static_cast<uint16_t*>(pBlockSize) = static_cast<uint16_t>(pBlockEnd - pBlockStart);
    }
}

// Loads what SaveGame wrote: the low process's data, the weapon, furniture, knock, idle and radiation state, the
// dead-detect list and the run-once package. When the change flags carry the inventory, the worn weapon is made current
// again and attached, its clip refilled from the player's ammo count (Lily's weapon instead re-inits her default outfit
// and animation). Then the magic state and, from version 15, the queued equip items.
// LoadData fills raw bytes, and LoadFormID writes a bare form ID into the pointer fields (InitLoadGame resolves them
// later): both destinations are treated as untyped memory, hence the reinterpret_casts.
void MiddleHighProcess::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Actor* pActor = apLoadGameBuffer->GetActor();
    MiddleLowProcess::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bWantWeaponDrawn), sizeof(bWantWeaponDrawn));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bWeaponDrawn), sizeof(bWeaponDrawn));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bCheckMagicNode), sizeof(bCheckMagicNode));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fAlphaMult), sizeof(fAlphaMult));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fScriptRefractPower), sizeof(fScriptRefractPower));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eUseItem), sizeof(eUseItem));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bBeenAttacked), sizeof(bBeenAttacked));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&lastSeenPostion), sizeof(lastSeenPostion));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fEquippedWeight), sizeof(fEquippedWeight));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cSitSleepState), sizeof(cSitSleepState));
    if (pActor) {
        pActor->SetCachedSitSleepState(static_cast<SIT_SLEEP_STATE>(cSitSleepState));
    }
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cCurrentFurnitureIndex), sizeof(cCurrentFurnitureIndex));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&FurnitureMarker.cNumber), sizeof(FurnitureMarker.cNumber));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&FurnitureMarker.sHeading), sizeof(FurnitureMarker.sHeading));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&FurnitureMarker.Position), sizeof(FurnitureMarker.Position));
    int8_t cLoadedKnockState = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cLoadedKnockState), sizeof(cLoadedKnockState));
    if (pActor && cLoadedKnockState == 0 && cKnockState != 0) {
        ClearKnockState(pActor);
    }
    cKnockState = cLoadedKnockState;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bDoneOnce), sizeof(bDoneOnce));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bPickPocketed), sizeof(bPickPocketed));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bDoneClothesChange), sizeof(bDoneClothesChange));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fPursueTimer), sizeof(fPursueTimer));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bSummonedCreature), sizeof(bSummonedCreature));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fAimLooking), sizeof(fAimLooking));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fAutomaticShotDelay), sizeof(fAutomaticShotDelay));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cShotsToFire), sizeof(cShotsToFire));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bSustainedFire), sizeof(bSustainedFire));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bIronSights), sizeof(bIronSights));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iAnimActionSuccess), sizeof(iAnimActionSuccess));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iReservationSlot), sizeof(iReservationSlot));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bAimingTarget), sizeof(bAimingTarget));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iPackageIdleNumber), sizeof(iPackageIdleNumber));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fPackageIdleTimer), sizeof(fPackageIdleTimer));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bPlayedBeginIdles), sizeof(bPlayedBeginIdles));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bPlayedEndIdles), sizeof(bPlayedEndIdles));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fRadiationDelta), sizeof(fRadiationDelta));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fRadiationMagicDelta), sizeof(fRadiationMagicDelta));
    if (apLoadGameBuffer->GetVersion() >= 10) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fRadiationWaterDelta), sizeof(fRadiationWaterDelta));
    }
    if (apLoadGameBuffer->GetVersion() >= 3) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iWeaponConditionStage), sizeof(iWeaponConditionStage));
    }
    if (apLoadGameBuffer->GetVersion() >= 7) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bPickPackIdle), sizeof(bPickPackIdle));
    }
    pLastIdlePlayed = dynamic_cast<TESIdleForm*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
    pCurrentIdle = dynamic_cast<TESIdleForm*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
    apLoadGameBuffer->LoadFormID(pCommandingActor);
    apLoadGameBuffer->LoadFormID(pCurrentFurniture);
    uint32_t uiAmmoCount = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&uiAmmoCount), sizeof(uiAmmoCount));
    uint32_t uiDeadCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < uiDeadCount; ++i) {
        void* pDead = nullptr;
        apLoadGameBuffer->LoadFormID(pDead);
        DeadDetectList.AddHead(pDead);
    }
    RunOncePackage.LoadGame(apLoadGameBuffer);
    if (pActor && apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_ANIMATION)) {
        pSaveLoadGame->QueueSubBuffer(QUEUED_SUB_BUFFER_ANIMATION, apLoadGameBuffer, nullptr);
    }

    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_INVENTORY | CHANGE_REFR_LEVELED_INVENTORY)) {
        InventoryChanges* pChanges = pActor->GetExtra()->GetContainerChanges();
        if (pChanges) {
            ItemChange* pWornWeapon = pChanges->GetWornItem(BO_WEAPON, false);
            SetCurrentWeapon(pWornWeapon, pActor->Get3D(), false);
            TESObjectWEAP* pWeaponObject = pWeapon ? static_cast<TESObjectWEAP*>(pWeapon->GetItemChangeObject())
                                                   : nullptr;
            if (pWeapon && pActor->Get3D() && !(pActor->IsCreature() && std::strstr(pActor->GetFullName(), "Lily"))) {
                pActor->AttachWeapon(pWeaponObject);
                pActor->GetCurrentAIProcess()->SetCurrentAmmo(nullptr);
                pActor->ReloadWeaponNV(
                    pWeaponObject,
                    0,
                    pWeapon->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE),
                    true
                );
                if (pAmmo) {
                    uint32_t uiHeldCount = uiAmmoCount;
                    InventoryChanges* pPlayerChanges = InventoryChanges::GetInventoryChanges(pPlayer);
                    if (pPlayerChanges) {
                        uiHeldCount = pPlayerChanges->GetObjectCount(pAmmo->GetItemChangeObject());
                    }
                    if (uiHeldCount < uiAmmoCount) {
                        pAmmo->SetCount(uiHeldCount);
                        uiAmmoCount = uiHeldCount;
                    }
                    else {
                        pAmmo->SetCount(uiAmmoCount);
                    }
                }
            }
            else if (pWeapon && pActor->Get3D() && pActor->IsCreature() && std::strstr(pActor->GetFullName(), "Lily")) {
                static_cast<TESCreature*>(pActor->GetObjectReference())
                    ->TESCreature::InitDefaultWorn(pActor, true, true, false);
                BaseProcess* pProcess = pActor->GetCurrentAIProcess();
                pProcess
                    ->ForceWeaponDrawnSheathed(pActor->IsWeaponDrawn(), nullptr, pActor->Actor::GetAnimation(), pActor);
                if (ReferenceProcessLists.kUpdateLock.TryLock()) {
                    pActor->InitAnimation();
                    ReferenceProcessLists.kUpdateLock.Unlock();
                }
            }
        }
    }
    ActiveEffect::LoadActiveEffectList(
        apLoadGameBuffer,
        static_cast<BSSimpleList<ActiveEffect*>*>(static_cast<void*>(pActiveEffectList))
    );
    apLoadGameBuffer->LoadFormID(pCurrentPackageSpell);
    apLoadGameBuffer->LoadFormID(pCurrentSpell);
    apLoadGameBuffer->LoadFormID(pDesiredTarget);
    if (apLoadGameBuffer->GetVersion() >= 15) {
        uint32_t uiQueuedCount = apLoadGameBuffer->LoadVariableSizedValue();
        if (uiQueuedCount) {
            BSSimpleList<QueuedItem*>* pList = new BSSimpleList<QueuedItem*>;
            pListItemstoEquipUnequip = pList;
            for (uint32_t i = 0; i < uiQueuedCount; ++i) {
                QueuedItem* pItem = new QueuedItem;
                pItem->LoadGame(apLoadGameBuffer);
                pListItemstoEquipUnequip->AddHead(pItem);
            }
        }
    }
}

void MiddleHighProcess::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags, MobileObject* pmob) {
    uint32_t iCurrentPackageSpellID;
    uint32_t iRefID;
    bool bRagdollSaved;
    bool bTemp;
    uint32_t iBlockHeader;
    float fHeading;
    uint16_t iBlockSize;
    uint8_t cNumber;
    uint32_t iDesiredTargetID;
    uint32_t iCommandingActorID;
    char cTempKnockState;
    uint32_t iCurrentSpellID;
    NiPoint3 kPosition;
    uint32_t iFurnitureID;
    Actor* pActor = dynamic_cast<Actor*>(pmob);
    MiddleLowProcess::LoadGame(aiFlags, aiCurrentFlags, pmob);
    iBlockSize = 0;
    uint8_t* iStartBufferPosition = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iBlockHeader, sizeof(iBlockHeader));
        if (iBlockHeader != 0x424C4F4B) {
            LoadFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp",
                    7200,
                    pHeader->iFormID,
                    pForm->GetFormEditorID(),
                    pHeader->cVersion,
                    pHeader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp",
                    7200,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        iStartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iBlockSize, sizeof(iBlockSize));
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&bWantWeaponDrawn, sizeof(bWantWeaponDrawn));
    pSaveLoadGameOLD->LoadGameDataOLD(&bWeaponDrawn, sizeof(bWeaponDrawn));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 52u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&bCheckMagicNode, sizeof(bCheckMagicNode));
        pSaveLoadGameOLD->LoadGameDataOLD(&fAlphaMult, sizeof(fAlphaMult));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 77u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&fScriptRefractPower, sizeof(fScriptRefractPower));
    }
    if ((aiFlags & CHANGE_REFR_INVENTORY) != 0 || (aiCurrentFlags & CHANGE_REFR_INVENTORY) != 0) {
        RemoveAndAttachItems(pmob);
        pActor->ReloadTargets(false);
        NiAVObject* pObject = pmob->TESObjectREFR::Get3D();
        if (pObject) {
            NiNode* pNode = static_cast<NiNode*>(pObject);
            if (pmob == pPlayer && !pPlayer->Is3rdPerson()) {
                pObject = pNode->GetAtChecked(0);
                pNode = static_cast<NiNode*>(pObject);
            }
            pActor->InitLightingPropertyPtr(pNode);
        }
    }
    pSaveLoadGameOLD->LoadNumericID(&iCommandingActorID, 4u);
    pCommandingActor = reinterpret_cast<Actor*>(static_cast<uintptr_t>(iCommandingActorID));
    pSaveLoadGameOLD->LoadGameDataOLD(&eUseItem, sizeof(eUseItem));
    pSaveLoadGameOLD->LoadGameDataOLD(&bBeenAttacked, sizeof(bBeenAttacked));
    if (pSaveLoadGameOLD->GetCurrentVersion() < 24u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&bTemp, sizeof(bTemp));
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&lastSeenPostion, sizeof(lastSeenPostion));
    pSaveLoadGameOLD->LoadGameDataOLD(&fEquippedWeight, sizeof(fEquippedWeight));
    pSaveLoadGameOLD->LoadGameDataOLD(&cSitSleepState, sizeof(cSitSleepState));
    pSaveLoadGameOLD->LoadGameDataOLD(&cCurrentFurnitureIndex, sizeof(cCurrentFurnitureIndex));
    pSaveLoadGameOLD->LoadNumericID(&iFurnitureID, 4u);
    pCurrentFurniture = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(iFurnitureID));
    pSaveLoadGameOLD->LoadGameDataOLD(&fHeading, sizeof(fHeading));
    pSaveLoadGameOLD->LoadGameDataOLD(&cNumber, sizeof(cNumber));
    pSaveLoadGameOLD->LoadGameDataOLD(&kPosition, sizeof(kPosition));
    FurnitureMarker.SetHeading(fHeading);
    FurnitureMarker.SetType(cNumber);
    FurnitureMarker.SetPosition(kPosition);
    pSaveLoadGameOLD->LoadGameDataOLD(&cTempKnockState, sizeof(cTempKnockState));
    if (cTempKnockState) {
        if ((aiFlags & CHANGE_REFR_HAVOK_MOVE) == 0 && pSaveLoadGameOLD->GetCurrentVersion() < 47u) {
            pSaveLoadGameOLD->LoadGameDataOLD(&bRagdollSaved, sizeof(bRagdollSaved));
            if (bRagdollSaved) {
                RagDollData* pRagdoll = new RagDollData;
                pRagdoll->LoadGame();
                if (pRagdoll) {
                    delete pRagdoll;
                }
            }
        }
    }
    else {
        ClearKnockState(dynamic_cast<Actor*>(pmob));
    }
    cKnockState = cTempKnockState;
    if ((aiFlags & CHANGE_REFR_ANIMATION) != 0) {
        Animation::LoadAnimation(pmob, pAnimation);
    }
    pSaveLoadGameOLD->LoadNumericID(&iCurrentSpellID, 4u);
    pCurrentSpell = reinterpret_cast<MagicItem*>(static_cast<uintptr_t>(iCurrentSpellID));
    pSaveLoadGameOLD->LoadNumericID(&iDesiredTargetID, 4u);
    pDesiredTarget = reinterpret_cast<MagicTarget*>(static_cast<uintptr_t>(iDesiredTargetID));
    ActiveEffect::LoadActiveEffectListTES(pActiveEffectList, pActor);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 69u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&bDoneOnce, sizeof(bDoneOnce));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 73u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&bPickPocketed, sizeof(bPickPocketed));
        pSaveLoadGameOLD->LoadGameDataOLD(&bDoneClothesChange, sizeof(bDoneClothesChange));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 101u) {
        uint16_t iCount;
        if (pSaveLoadGameOLD->GetCurrentVersion() < 104u) {
            pSaveLoadGameOLD->AdvanceBuffer(4u);
        }
        pSaveLoadGameOLD->LoadGameDataOLD(&fPursueTimer, sizeof(fPursueTimer));
        if (pSaveLoadGameOLD->GetCurrentVersion() < 104u) {
            pSaveLoadGameOLD->AdvanceBuffer(4u);
        }
        pSaveLoadGameOLD->LoadGameDataOLD(&iCount, sizeof(iCount));
        for (uint32_t i = 0; i < iCount; ++i) {
            pSaveLoadGameOLD->LoadNumericID(&iRefID, 4u);
            // The list keeps the form ID until InitLoadGame resolves it.
            void* pDead = reinterpret_cast<void*>(static_cast<uintptr_t>(iRefID));
            DeadDetectList.AddHead(pDead);
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 109u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&bSummonedCreature, sizeof(bSummonedCreature));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        pSaveLoadGameOLD->LoadNumericID(&iCurrentPackageSpellID, 4u);
        pCurrentPackageSpell = reinterpret_cast<MagicItem*>(static_cast<uintptr_t>(iCurrentPackageSpellID));
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* iCurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            if (iCurrentBufferPosition <= &iStartBufferPosition[iBlockSize]) {
                if (iCurrentBufferPosition < &iStartBufferPosition[iBlockSize]) {
                    uint32_t uiFlags = pHeader->iFlags;
                    int32_t iVersion = pHeader->cVersion;
                    const char* pEditorID = pForm->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(&iStartBufferPosition[iBlockSize] - iCurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp",
                        7373,
                        pHeader->iFormID,
                        pEditorID,
                        iVersion,
                        uiFlags
                    );
                }
            }
            else {
                uint32_t uiFlags = pHeader->iFlags;
                int32_t iVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(iCurrentBufferPosition - &iStartBufferPosition[iBlockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp",
                    7373,
                    pHeader->iFormID,
                    pEditorID,
                    iVersion,
                    uiFlags
                );
            }
        }
        else if (iCurrentBufferPosition <= &iStartBufferPosition[iBlockSize]) {
            if (iCurrentBufferPosition < &iStartBufferPosition[iBlockSize]) {
                int32_t iVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                    "%i",
                    static_cast<int32_t>(&iStartBufferPosition[iBlockSize] - iCurrentBufferPosition),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp",
                    7373,
                    iVersion
                );
            }
        }
        else {
            int32_t iVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(iCurrentBufferPosition - &iStartBufferPosition[iBlockSize]),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp",
                7373,
                iVersion
            );
        }
    }
}

void MiddleHighProcess::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Actor* pActor = apLoadGameBuffer->GetActor();
    LowProcess::InitLoadGame(apLoadGameBuffer);
    Actor* pCommander;
    if (pCommandingActor) {
        TESForm* pForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCommandingActor))
        );
        pCommander = dynamic_cast<Actor*>(pForm);
    }
    else {
        pCommander = nullptr;
    }
    pCommandingActor = pCommander;
    TESObjectREFR* pFurniture;
    if (pCurrentFurniture) {
        TESForm* pForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCurrentFurniture))
        );
        pFurniture = dynamic_cast<TESObjectREFR*>(pForm);
    }
    else {
        pFurniture = nullptr;
    }
    pCurrentFurniture = pFurniture;
    // The dead-detect list holds form IDs since LoadGame: each becomes its reference, or its node is removed.
    BSSimpleList<void*>* pList = &DeadDetectList;
    BSSimpleList<void*>* pLastList = nullptr;
    while (pList && !pList->IsEmpty()) {
        void* pForm = pList->m_item;
        bool bRemove = false;
        if (pForm) {
            TESForm* pResolved = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pForm)));
            pForm = dynamic_cast<TESObjectREFR*>(pResolved);
            if (pForm) {
                pList->SetItem(pForm);
            }
            else {
                bRemove = true;
            }
        }
        else {
            bRemove = true;
        }
        if (bRemove) {
            if (pLastList) {
                pLastList->Remove(pList->m_item);
                pList = pLastList->m_pkNext;
            }
            else {
                pList->RemoveHead();
            }
        }
        else {
            pLastList = pList;
            pList = pList->m_pkNext;
        }
    }
    RunOncePackage.InitLoadGameBGS(apLoadGameBuffer);
    ActiveEffect::InitLoadActiveEffectList(apLoadGameBuffer, pActiveEffectList);
    if (pCurrentFurniture) {
        if (pCurrentFurniture->GetObjectReference()->GetFormType() == FURN_ID) {
            if (pActor) {
                if (pActor->IsDead(false) || pActor->GetParentCell() != pCurrentFurniture->GetParentCell()) {
                    ClearFurniture(pActor);
                }
            }
        }
        else {
            ClearFurniture(pActor);
        }
    }
    else if (cSitSleepState) {
        cSitSleepState = 0;
        if (pActor == pPlayer) {
            pPlayer->ClearSitHeadingDelta();
        }
    }
    if ((iReservationSlot & 0x80000000u) == 0 && pTarget) {
        pTarget->SetMarkerReserved(iReservationSlot, true);
    }

    if (pActor) {
        SetWeaponFlags();
        UpdateArmorWorn(pActor);
    }
    if (pActor) {
        CheckCachedValueFlags(pActor);
    }
    MagicItem* pPackageSpell;
    if (pCurrentPackageSpell) {
        pPackageSpell = MagicItem::GetMagicItemByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCurrentPackageSpell))
        );
    }
    else {
        pPackageSpell = nullptr;
    }
    pCurrentPackageSpell = pPackageSpell;
    MagicItem* pSpell;
    if (pCurrentSpell) {
        pSpell = MagicItem::GetMagicItemByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCurrentSpell)));
    }
    else {
        pSpell = nullptr;
    }
    pCurrentSpell = pSpell;
    MagicTarget* pMagicTarget;
    if (pDesiredTarget) {
        pMagicTarget = MagicTarget::GetMagicTargetByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDesiredTarget))
        );
    }
    else {
        pMagicTarget = nullptr;
    }
    pDesiredTarget = pMagicTarget;
    if (apLoadGameBuffer->GetVersion() >= 15u) {
        BSSimpleList<QueuedItem*>* pItemList = pListItemstoEquipUnequip;
        BSSimpleList<QueuedItem*>* pLastItemList = nullptr;
        while (pItemList && !pItemList->IsEmpty()) {
            QueuedItem* pQueuedItem = pItemList->m_item;
            bool bRemove = false;
            if (pQueuedItem) {
                if (pQueuedItem->pObject) {
                    pQueuedItem->InitLoadGame(apLoadGameBuffer);
                }
                else {
                    delete pQueuedItem;
                    bRemove = true;
                }
            }
            else {
                bRemove = true;
            }
            if (bRemove) {
                if (pLastItemList) {
                    pLastItemList->Remove(pItemList->m_item);
                    pItemList = pLastItemList->m_pkNext;
                }
                else {
                    pItemList->RemoveHead();
                }
            }
            else {
                pLastItemList = pItemList;
                pItemList = pItemList->m_pkNext;
            }
        }
    }
}

// The TES-format init of a load: the low process's own, then the form IDs the process kept are resolved back to
// forms. The dead-detect list is walked once: an entry that names no reference is unlinked, and a kept entry is
// re-pointed at the form it resolved to.
void MiddleHighProcess::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags, MobileObject* pmob) {
    Actor* pActor = dynamic_cast<Actor*>(pmob);
    LowProcess::InitLoadGame(aiFlags, aiOldFlags, pmob);
    BaseProcess* pProcess = this;
    if (pProcess->GetRunOncePackage() && (aiFlags & 0) != 0)
    // The second test masks with 0, so the package's TES init never runs.
    {
        RunOncePackage.InitLoadGameTES(aiFlags, aiOldFlags);
    }

    // Each stored pointer's value is passed as the numeric form ID to resolve: the field holds the ID as untyped
    // memory, so the pointer is read as an integer here.
    if (pCommandingActor) {
        TESForm* pForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCommandingActor))
        );
        pCommandingActor = dynamic_cast<Actor*>(pForm);
    }
    if (pCurrentFurniture) {
        TESForm* pForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCurrentFurniture))
        );
        pCurrentFurniture = dynamic_cast<TESObjectREFR*>(pForm);
    }
    if (pCurrentSpell) {
        pCurrentSpell = MagicItem::GetMagicItemByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCurrentSpell))
        );
    }
    if (pDesiredTarget) {
        pDesiredTarget = MagicTarget::GetMagicTargetByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDesiredTarget))
        );
    }
    ActiveEffect::InitLoadGameActiveEffectList(pActiveEffectList, pActor);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x65) {
        BSSimpleList<void*>* pNode = &DeadDetectList;
        BSSimpleList<void*>* pLastGood = nullptr;
        while (pNode && !pNode->IsEmpty()) {
            void* pItem = pNode->GetItem();
            TESObjectREFR* pRef = nullptr;
            if (pItem) {
                TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pItem)));
                pRef = dynamic_cast<TESObjectREFR*>(pForm);
                pNode->SetItem(pRef);
            }
            if (pRef) {
                pLastGood = pNode;
                pNode = pNode->GetNext();
            }
            else if (pLastGood) {
                pLastGood->Remove(pItem);
                pNode = pLastGood->GetNext();
            }
            else {
                pNode->RemoveHead();
            }
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x71 && pCurrentPackageSpell) {
        pCurrentPackageSpell = MagicItem::GetMagicItemByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCurrentPackageSpell))
        );
    }
}

// Finishes the init-load of the process: the base's, then the dead state of the face, the weapon reattached to the
// 3D and the furniture: the actor is put back on its marker, or the furniture is dropped when it or its marker is
// gone. A knocked-out actor's 3D goes dynamic, then the active effects finish loading. A summoned creature without a
// commander is queued for deletion.
void MiddleHighProcess::FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags, MobileObject* pmob) {
    BaseProcess::FinishInitLoadGame(aiFlags, aiOldFlags, pmob);
    Actor* pActor = dynamic_cast<Actor*>(pmob);
    BSFaceGenAnimationData* pFaceAnimationData = pmob->GetFaceAnimationData(nullptr);
    if (pFaceAnimationData && cKnockState) {
        // Knock states 1-4 (explode, explode lead-in, knocked down and knock-out lead-in) are dead faces.
        int8_t cKnock = cKnockState;
        if (cKnock > 0 && cKnock <= 4) {
            pFaceAnimationData->SetDead(true, false);
        }
        else {
            pFaceAnimationData->SetDead(false, false);
        }
    }
    if (pActor) {
        if (pActor == pPlayer) {
            Animation* pAnimation1st = pPlayer->GetAnimation(true);
            BipedAnim* pBiped1st = pPlayer->GetBiped(true);
            ForceWeaponDrawnSheathed(bWeaponDrawn, pBiped1st, pAnimation1st, pPlayer);
            Animation* pAnimation3rd = pPlayer->GetAnimation(false);
            BipedAnim* pBiped3rd = pPlayer->GetBiped(false);
            ForceWeaponDrawnSheathed(bWeaponDrawn, pBiped3rd, pAnimation3rd, pPlayer);
        }
        else if (GetWeaponBone(pActor->GetBiped())) {
            Animation* pActorAnimation = pActor->GetAnimation();
            BipedAnim* pActorBiped = pActor->GetBiped();
            ForceWeaponDrawnSheathed(bWeaponDrawn, pActorBiped, pActorAnimation, pActor);
        }
    }
    NiAVObject* p3D;
    if (pmob == pPlayer) {
        p3D = pPlayer->Get3D(false);
    }
    else {
        p3D = pmob->TESObjectREFR::Get3D();
    }
    if (pActor && pActor->TESObjectREFR::Get3D() && pCurrentFurniture && !pActor->IsDead(false)) {
        bool bValid = true;
        if (pActor->GetParentCell() == pCurrentFurniture->GetParentCell()) {
            TESFurniture* pFurniture = dynamic_cast<TESFurniture*>(pCurrentFurniture->GetObjectReference());
            BSFurnitureMarker* pMarker = nullptr;
            if (pFurniture) {
                pMarker = BSFurnitureMarker::Find(pCurrentFurniture->Get3D());
                if (!pMarker || cCurrentFurnitureIndex >= pMarker->QNumMarks()) {
                    bValid = false;
                }
            }
            if (bValid) {
                uint8_t cSavedSitSleepState = cSitSleepState;
                // The wanting and getting-on states (1-4, 6-9) load the idle of their first state.
                switch (cSitSleepState) {
                    case 1:
                    case 2:
                    case 3:
                    case 4: cSitSleepState = 1; break;
                    case 6:
                    case 7:
                    case 8:
                    case 9: cSitSleepState = 6; break;
                    default: break;
                }
                if (cSitSleepState) {
                    pCurrentFurniture->SetMarkerUsed(cCurrentFurnitureIndex, true);
                    if (pActor == pPlayer) {
                        LoadDynamicIdle(pPlayer);
                        pPlayer->SetFirstPerson(pPlayer->Is3rdPerson());
                        LoadDynamicIdle(pPlayer);
                        pPlayer->SetFirstPerson(pPlayer->Is3rdPerson());
                    }
                    else {
                        LoadDynamicIdle(pActor);
                    }
                }
                cSitSleepState = cSavedSitSleepState;
                if ((cSitSleepState == SIT_SLEEP_IS_SITTING || cSitSleepState == SIT_SLEEP_IS_SLEEPING) && pFurniture) {
                    if (pMarker && cCurrentFurnitureIndex < pMarker->QNumMarks()) {
                        FurnitureMark kMark = pMarker->QMark(cCurrentFurnitureIndex);
                        NiPoint3 kMarkPosition = kMark.QPosition();
                        NiPoint3 kWorldPosition;
                        const NiPoint3* pLocation = &pCurrentFurniture->GetLocationOnReference();
                        NiMatrix3 kRotation = pCurrentFurniture->GetOrientation();
                        NiMatrix3::TransformVertices(kRotation, *pLocation, 1, &kMarkPosition, &kWorldPosition);
                        FurnitureMarker.SetType(kMark.GetType());
                        FurnitureMarker.SetPosition(kWorldPosition);
                        float fMarkHeading = kMark.GetHeading();
                        float fHeading = pCurrentFurniture->GetAngleOnReference().z + fMarkHeading;
                        FurnitureMarker.SetHeading(fHeading);
                    }
                    float fScale = pActor->GetScale();
                    NiPoint3 kDelta = pFurniture->GetMarkerTargetOffset(FurnitureMarker.GetType(), fScale);
                    NiMatrix3 kRotZ;
                    kRotZ.MakeZRotation(FurnitureMarker.GetHeading());
                    kDelta = kRotZ * kDelta;
                    pActor->SetPosition(FurnitureMarker.QPosition() + kDelta);
                    uint8_t cSitSleepStateBeforeTurn = cSitSleepState;
                    cSitSleepState = SIT_SLEEP_NORMAL;
                    if (pActor != pPlayer) {
                        pActor->SetHeading(FurnitureMarker.GetHeading());
                        pActor->ModHeading(pFurniture->GetMarkerHeadingOffset(FurnitureMarker.GetType()));
                    }
                    cSitSleepState = cSitSleepStateBeforeTurn;
                    if (pActor->GetCharController()) {
                        const NiPoint3& rPosition = FurnitureMarker.QPosition();
                        static_cast<bhkCharacterController*>(pActor->GetCharController())->SetPosition(rPosition);
                    }
                }
            }
        }
        else {
            bValid = false;
        }
        if (!bValid) {
            pCurrentFurniture = nullptr;
            cSitSleepState = SIT_SLEEP_NORMAL;
            if (pActor == pPlayer) {
                pPlayer->ClearSitHeadingDelta();
            }
        }
        else if (cSitSleepState == SIT_SLEEP_IS_SLEEPING || cSitSleepState == SIT_SLEEP_IS_SITTING) {
            bhkWorld::AdjustForceAddCount(p3D, true, true, false);
        }
    }
    if (pActor && cSitSleepState && !pCurrentFurniture) {
        cSitSleepState = SIT_SLEEP_NORMAL;
        if (pActor == pPlayer) {
            pPlayer->ClearSitHeadingDelta();
        }
    }
    // Knock states 2 and 1: explode lead-in and explode.
    if ((p3D && cKnockState == 2) || cKnockState == 1) {
        bhkWorld::SetMotion(p3D, hkpMotion::MOTION_DYNAMIC, true, false, true);
        NiUpdateData kUpdateData(0.0f, false, false);
        p3D->Update(kUpdateData);
    }
    ActiveEffect::FinishInitLoadGameActiveEffectList(
        static_cast<BSSimpleList<ActiveEffect*>*>(static_cast<void*>(pActiveEffectList)),
        pActor
    );
    bRefreshFlareFlags = true;
    if (pActor) {
        SetShouldRefreshMagicShadersEffects(true, false, false);
    }
    if (bSummonedCreature && !pCommandingActor) {
        pSaveLoadGameOLD->AddFormToDeferredDeletionsList(pmob);
    }
    if (pActor) {
        CheckCachedValueFlags(pActor);
    }
}

// Reverts a BGS-format load: the low process's own, then the package, the furniture, the magic and every timer and
// flag the process keeps. The deferred limbs are freed, the queued items dropped and, for a revert-only load, the
// knock state cleared.
void MiddleHighProcess::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    MiddleLowProcess::Revert(apLoadGameBuffer);
    BaseProcess* pProcess = this;
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_REFR_INVENTORY | CHANGE_REFR_LEVELED_INVENTORY)) {
        pProcess->SetCurrentWeapon(nullptr, nullptr, false);
        pProcess->SetCurrentAmmo(nullptr);
        pProcess->SetCurrentShield(nullptr);
        pProcess->SetCurrentTorch(nullptr);
        pCurrentWeaponEffect = nullptr;
    }
    RunOncePackage.RevertBGS(apLoadGameBuffer);
    Actor* pActor = apLoadGameBuffer->GetActor();
    if (pActor) {
        pProcess->SetSitSleepState(pActor, SIT_SLEEP_NORMAL, nullptr, 0x7F);
    }
    pCurrentFurniture = nullptr;
    cCurrentFurnitureIndex = 0x7F;
    DeadDetectList.RemoveAll();
    bWeaponGrenade = false;
    bWeaponMine = false;
    bWeaponThrown = false;
    pCommandingActor = nullptr;
    pLastBoundWeapon = nullptr;
    bWeaponDrawn = false;
    bWantWeaponDrawn = false;
    fPursueTimer = 0.0f;
    bBeenAttacked = false;
    eUseItem = POTYPE_NONE;
    fEquippedWeight = 0.0f;
    bCheckMagicNode = false;
    pCurrentWeaponEffect = nullptr;
    fAlphaMult = 1.0f;
    fScriptRefractPower = 0.0f;
    uiFlareFlags = 0;
    bRefreshFlareFlags = false;
    bDoneOnce = false;
    bPickPocketed = false;
    bDoneClothesChange = false;
    bRefreshMagicShaderEffects = false;
    bRefreshMagicShaderEffectBody = false;
    bRefreshMagicShaderEffectWeapon = false;
    bRefreshMagicShaderEffectLoading = 0;
    pSavedAcquireObject = nullptr;
    bDetectLifeDetected = false;
    RefListChairBed.RemoveAll();
    iAnimActionSuccess = 2;
    iReservationSlot = 0xFFFFFFFF;
    iMovementAnimations = 0;
    bAimingTarget = false;
    iPackageIdleNumber = 0xFFFFFFFF;
    fPackageIdleTimer = -1.0f;
    bPlayedBeginIdles = false;
    bPlayedEndIdles = false;
    iCreatureLipsynchStartTime = 0;
    fRadiationDelta = 0.0f;
    fRadiationMagicDelta = 0.0f;
    fRadiationWaterDelta = 0.0f;
    iWeaponConditionStage = 0xFFFFFFFF;
    bPickPackIdle = false;
    pFaceNode = nullptr;
    pFaceNodeSkinned = nullptr;
    pAnimFace = nullptr;
    pLastIdlePlayed = nullptr;
    pCurrentIdle = nullptr;
    for (BSAnimGroupSequence*& pSequence : pAimSequence) {
        pSequence = nullptr;
    }
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_REFR_ANIMATION) && pAnimation) {
        pAnimation->Revert(apLoadGameBuffer);
    }

    ActiveEffect::RevertActiveEffectList(
        apLoadGameBuffer,
        static_cast<BSSimpleList<ActiveEffect*>*>(static_cast<void*>(pActiveEffectList))
    );
    while (!DeferredHideLimbList.IsEmpty()) {
        DeferredHideLimb* pItem = DeferredHideLimbList.GetItem();
        if (pItem) {
            delete pItem;
        }
        DeferredHideLimbList.RemoveHead();
    }
    pProcess->RemoveAllQueuedItems();
    if (pActor && apLoadGameBuffer->GetRevertOnly()) {
        pProcess->ClearKnockState(pActor);
    }
}

// Reverts a TES-format load: the low process's own, then the weapon, the furniture, the magic and the timers the
// process keeps.
void MiddleHighProcess::Revert(uint32_t aiFlags, MobileObject* pmob) {
    Actor* pActor = dynamic_cast<Actor*>(pmob);
    MiddleLowProcess::Revert(aiFlags, pmob);
    BaseProcess* pProcess = this;
    if ((aiFlags & CHANGE_REFR_INVENTORY) != 0) {
        if (pWeapon) {
            pActor->ReloadTargets(false);
        }
        pProcess->SetCurrentWeapon(nullptr, nullptr, false);
        pProcess->SetCurrentAmmo(nullptr);
        pProcess->SetCurrentShield(nullptr);
        pProcess->SetCurrentTorch(nullptr);
        pCurrentWeaponEffect = nullptr;
    }
    // The test masks with 0, so the package's TES revert never runs.
    if ((aiFlags & 0) != 0) {
        RunOncePackage.RevertTES(aiFlags);
    }
    if (pSaveLoadGameOLD->IsUnk048Zero()) {
        if ((aiFlags & CHANGE_REFR_ANIMATION) != 0 && pAnimation) {
            pAnimation->RevertTES(pmob);
        }
        ActiveEffect::RevertActiveEffectList(
            static_cast<BSSimpleList<ActiveEffect*>*>(static_cast<void*>(pActiveEffectList)),
            pActor
        );
        if (pActor) {
            pProcess->EndMoveMessage(pActor);
        }
        bWantWeaponDrawn = false;
        pProcess->SetSitSleepState(pActor, SIT_SLEEP_NORMAL, nullptr, 0x7F);
        pCurrentFurniture = nullptr;
        cCurrentFurnitureIndex = 0x7F;
        DeadDetectList.RemoveAll();
        bWeaponGrenade = false;
        bWeaponMine = false;
        bWeaponThrown = false;
        pCommandingActor = nullptr;
        pLastBoundWeapon = nullptr;
        pProcess->SetRunOnceProcedureIndex(0);
        bWeaponDrawn = false;
        iNumberItemsActivate = 0;
        fPursueTimer = 0.0f;
        bBeenAttacked = false;
        eUseItem = POTYPE_NONE;
        fEquippedWeight = 0.0f;
        bCheckMagicNode = false;
        pCurrentWeaponEffect = nullptr;
        fAlphaMult = 1.0f;
        fScriptRefractPower = 0.0f;
        uiFlareFlags = 0;
        bRefreshFlareFlags = false;
        bDoneOnce = false;
        fEssentialDownTimer = 0.0f;
        bPickPocketed = false;
        bDoneClothesChange = false;
        bRefreshMagicShaderEffects = false;
        bRefreshMagicShaderEffectBody = false;
        bRefreshMagicShaderEffectWeapon = false;
        bRefreshMagicShaderEffectLoading = 0;
        pSavedAcquireObject = nullptr;
        bDetectLifeDetected = false;
        RefListChairBed.RemoveAll();
        if (pAttachedArrowList) {
            pAttachedArrowList->RemoveAll();
            BSSimpleList<ArrowProjectile*>* pArrowProjectiles = pAttachedArrowList;
            if (pArrowProjectiles) {
                delete pArrowProjectiles;
            }
            pAttachedArrowList = nullptr;
        }
    }
    pFaceNode = nullptr;
    pFaceNodeSkinned = nullptr;
    pAnimFace = nullptr;
}

// Finishes the load of the process: the base's, the run-once package's and the active effects'. An actor that sits
// or sleeps is put back on its furniture marker, or the furniture is cleared when the model has no markers.
void MiddleHighProcess::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Actor* pActor = apLoadGameBuffer->GetActor();
    BaseProcess::FinishLoadGame(apLoadGameBuffer);
    RunOncePackage.FinishLoadGame(apLoadGameBuffer);
    ActiveEffect::FinishLoadActiveEffectList(
        apLoadGameBuffer,
        static_cast<BSSimpleList<ActiveEffect*>*>(static_cast<void*>(pActiveEffectList))
    );
    if (!pCurrentFurniture) {
        return;
    }
    if (cSitSleepState != SIT_SLEEP_IS_SITTING && cSitSleepState != SIT_SLEEP_IS_SLEEPING) {
        return;
    }
    TESFurniture* pFurniture = static_cast<TESFurniture*>(pCurrentFurniture->GetObjectReference());
    BSFurnitureMarker* pMarker = BSFurnitureMarker::Find(pCurrentFurniture->Get3D());
    if (!pMarker) {
        ClearFurniture(pActor);
        return;
    }
    FurnitureMark kMark = pMarker->QMark(cCurrentFurnitureIndex);
    NiPoint3 kMarkPosition = kMark.QPosition();
    NiPoint3 kWorldPosition;
    const NiPoint3* pLocation = &pCurrentFurniture->GetLocationOnReference();
    NiMatrix3 kRotation = pCurrentFurniture->GetOrientation();
    NiMatrix3::TransformVertices(kRotation, *pLocation, 1, &kMarkPosition, &kWorldPosition);
    FurnitureMarker.SetType(kMark.GetType());
    FurnitureMarker.SetPosition(kWorldPosition);
    float fMarkHeading = kMark.GetHeading();
    float fHeading = pCurrentFurniture->GetAngleOnReference().z + fMarkHeading;
    FurnitureMarker.SetHeading(fHeading);
    float fScale = pActor->GetScale();
    NiPoint3 kDelta = pFurniture->GetMarkerTargetOffset(FurnitureMarker.GetType(), fScale);
    NiMatrix3 kRotZ;
    kRotZ.MakeZRotation(FurnitureMarker.GetHeading());
    kDelta = kRotZ * kDelta;
    pActor->SetPosition(FurnitureMarker.QPosition() + kDelta);
    uint8_t cSavedSitSleepState = cSitSleepState;
    cSitSleepState = SIT_SLEEP_NORMAL;
    if (pActor != pPlayer) {
        pActor->SetHeading(FurnitureMarker.GetHeading());
        pActor->ModHeading(pFurniture->GetMarkerHeadingOffset(FurnitureMarker.GetType()));
    }
    cSitSleepState = cSavedSitSleepState;
    if (pActor->GetCharController()) {
        const NiPoint3& rPosition = FurnitureMarker.QPosition();
        static_cast<bhkCharacterController*>(pActor->GetCharController())->SetPosition(rPosition);
    }
}

TESEffectShader* MiddleHighProcess::GetWeaponEnchantmentVisuals() {
    return pCurrentWeaponEffect;
}

void MiddleHighProcess::SetWeaponEnchantmentVisuals(TESEffectShader* apEffect) {
    pCurrentWeaponEffect = apEffect;
}

bool MiddleHighProcess::GetShouldRefreshMagicShadersEffects() {
    return bRefreshMagicShaderEffects;
}

void MiddleHighProcess::SetShouldRefreshMagicShadersEffects(bool abWeapon, bool abBody, bool abLoading) {
    bRefreshMagicShaderEffects = true;
    bRefreshMagicShaderEffectBody |= abBody;
    bRefreshMagicShaderEffectWeapon |= abWeapon;
    bRefreshMagicShaderEffectLoading |= abLoading;
}

void MiddleHighProcess::RefreshMagicShaderEffects(Actor* apActor) {
    if (bRefreshMagicShaderEffectBody) {
        MagicShaderHitEffect* pEffect = ReferenceProcessLists.FindActiveShaderHitEffect(apActor);
        if (pEffect) {
            pEffect->Init();
        }
    }
    if (bRefreshMagicShaderEffectWeapon) {
        SetWeaponEnchantmentVisuals(nullptr);
        if (apActor->pCurrentProcess) {
            BaseProcess* pProcess = apActor->pCurrentProcess;
            if (pProcess->GetCurrentWeapon()) {
                EnchantmentItem* pEnchant = TESEnchantableForm::GetFormEnchanting(GetCurrentWeapon()->pContainerObj);
                if (pEnchant) {
                    double dItemCharge = apActor->pCurrentProcess->GetCurrentWeapon()->GetItemCharge();
                    if (pEnchant->GetCost(apActor) <= dItemCharge) {
                        EffectSetting* pEffectSetting = pEnchant->GetAVEffect(MagicSystem::SELF);
                        if (pEffectSetting && pEffectSetting->data.pEnchantEffect) {
                            SetWeaponEnchantmentVisuals(pEffectSetting->data.pEnchantEffect);
                        }
                    }
                }
            }
        }
        bool bCreated = false;
        TESEffectShader* pVisuals = GetWeaponEnchantmentVisuals();
        MagicShaderHitEffect* pWeapEffect = ReferenceProcessLists.FindAndCleanupWeaponShaderHitEffect(
            apActor,
            pVisuals
        );
        if (pWeapEffect) {
            pWeapEffect->Init();
        }
        else if (GetWeaponEnchantmentVisuals()) {
            MagicShaderHitEffect* pNewEffect = new MagicShaderHitEffect(apActor, GetWeaponEnchantmentVisuals(), -1.0f);
            pNewEffect->bWeaponEnchantment = true;
            if (bRefreshMagicShaderEffectLoading) {
                pNewEffect->SetLifeTime(100.0f);
            }
            if (pNewEffect->Init()) {
                ReferenceProcessLists.AddTempEffect(pNewEffect);
                if (!bRefreshMagicShaderEffectLoading) {
                    bCreated = true;
                }
            }
        }
        if (apActor == pPlayer && pPlayer->GetInventory3D()) {
            TESEffectShader* pInventoryVisuals = GetWeaponEnchantmentVisuals();
            MagicShaderHitEffect* pInventory = pPlayer->GetMagicShaderHitEffect();
            if (pInventoryVisuals) {
                if (pInventory && pInventory->pEffectData != pInventoryVisuals) {
                    delete pInventory;
                    pInventory = nullptr;
                }

                if (!pInventory) {
                    MagicShaderHitEffect* pNewInventory = new MagicShaderHitEffect(pPlayer, pInventoryVisuals, -1.0f);
                    pInventory = pNewInventory;
                    pNewInventory->bWeaponEnchantment = true;
                    pPlayer->SetInventoryWeaponEffect(pNewInventory);
                    if (!bCreated) {
                        pNewInventory->SetLifeTime(100.0f);
                    }
                }
            }
            else if (pInventory) {
                delete pInventory;
                pInventory = nullptr;
                pPlayer->SetInventoryWeaponEffect(nullptr);
            }
            if (pInventory && !pInventory->Init()) {
                delete pInventory;
                pPlayer->SetInventoryWeaponEffect(nullptr);
            }
        }
    }
    bRefreshMagicShaderEffects = false;
    bRefreshMagicShaderEffectBody = false;
    bRefreshMagicShaderEffectWeapon = false;
    bRefreshMagicShaderEffectLoading = 0;
}

void MiddleHighProcess::SetBeginIdlesPlayed(bool abFlag) {
    bPlayedBeginIdles = abFlag;
}

bool MiddleHighProcess::GetBeginIdlesPlayed() {
    return bPlayedBeginIdles;
}

void MiddleHighProcess::SetEndIdlesPlayed(bool abFlag) {
    bPlayedEndIdles = abFlag;
}

bool MiddleHighProcess::GetEndIdlesPlayed() {
    return bPlayedEndIdles;
}

void MiddleHighProcess::SetAlphaMult(float afAlpha) {
    fAlphaMult = afAlpha;
}

void MiddleHighProcess::SetScriptRefractionPower(float afRefractPower) {
    fScriptRefractPower = afRefractPower;
}

// The flare flags are rebuilt from the active effects when asked for; CheckFlareFlag's result is returned as it is.
bool MiddleHighProcess::ShouldCheckFlare(BSSimpleList<ActiveEffect*>* apList, MagicSystem::FlareType aeFlareType) {
    if (bRefreshFlareFlags) {
        uiFlareFlags = 0;
        while (apList && !apList->IsEmpty()) {
            uiFlareFlags |= MagicSystem::CheckEffectFlares(apList->m_item->GetEffectSetting());
            apList = apList->m_pkNext;
        }
        bRefreshFlareFlags = false;
    }
    return MagicSystem::CheckFlareFlag(aeFlareType, uiFlareFlags);
}

void MiddleHighProcess::SetRefreshFlareFlags() {
    bRefreshFlareFlags = true;
}

BSBound* MiddleHighProcess::GetBSBound() {
    return pBSBound;
}

void MiddleHighProcess::SetBSBound(BSBound* apBound) {
    pBSBound = apBound;
}

void MiddleHighProcess::SetForceNextUpdate(bool abForce) {
    bForceNextUpdate = abForce;
}

void MiddleHighProcess::SetIsSummonedCreature(bool abValue) {
    bSummonedCreature = abValue;
}

void MiddleHighProcess::AddAttachedArrow(ArrowProjectile* apArrow) {
    if (apArrow) {
        if (!pAttachedArrowList) {
            pAttachedArrowList = new BSSimpleList<ArrowProjectile*>;
        }
        if (!pAttachedArrowList->IsInList(apArrow)) {
            pAttachedArrowList->AddHead(apArrow);
        }
    }
}

void MiddleHighProcess::RemoveAttachedArrow(ArrowProjectile* apArrow) {
    if (apArrow && pAttachedArrowList) {
        pAttachedArrowList->Remove(apArrow);
    }
}

// Moves each arrow stuck in the actor whose cell differs from apAttachedRef's into that cell and the temporary
// change list.
void MiddleHighProcess::MoveAttachedArrowsToParentCell(TESObjectREFR* apAttachedRef) {
    if (!apAttachedRef || !pAttachedArrowList || pAttachedArrowList->IsEmpty()) {
        return;
    }
    TESObjectCELL* pCell = apAttachedRef->GetParentCell();
    for (BSSimpleList<ArrowProjectile*>* pNode = pAttachedArrowList; pNode && !pNode->IsEmpty();
         pNode = pNode->GetNext()) {
        TESObjectREFR* pArrow = static_cast<TESObjectREFR*>(pNode->GetItem());
        if (pArrow && pArrow->GetParentCell() != pCell && pCell) {
            pCell->AddReferenceSimple(pArrow);
            ReferenceProcessLists.AddActorToTempChangeList(static_cast<Actor*>(pArrow));
        }
    }
}

void MiddleHighProcess::StoreDamageNodes(NiNode* apNode, BGSBodyPartData* apBodyPartData) {
    BGSBodyPart* pTorsoPart;
    BGSBodyPart* pHeadPart;
    BGSBodyPart** pBodyParts = apBodyPartData->pPartArray;
    for (int32_t i = 0; i < 15; ++i) {
        if (pBodyParts[i]) {
            NiNode* pLimbRoot;
            BGSBodyPart* pBP = pBodyParts[i];
            const char* pName = pBP->GetNodeName();
            if (pName) {
                pLimbRoot = static_cast<NiNode*>(BSUtilities::GetObjectByName(apNode, pName));
            }
            else {
                pLimbRoot = nullptr;
            }
            pDamageRootNode[pBP->GetPartType()] = pLimbRoot;
        }
    }
    if (apBodyPartData) {
        pHeadPart = apBodyPartData->GetBodyPart(BGSBodyPart::LIMB_HEAD_1);
    }
    else {
        pHeadPart = nullptr;
    }
    if (pHeadPart && pHeadPart->GetTargetName()) {
        pHeadNode = BSUtilities::GetObjectByName(apNode, pHeadPart->GetTargetName());
    }

    if (apBodyPartData) {
        pTorsoPart = apBodyPartData->GetBodyPart(BGSBodyPart::LIMB_TORSO);
    }
    else {
        pTorsoPart = nullptr;
    }
    if (pTorsoPart && pTorsoPart->GetTargetName()) {
        pTorsoNode = BSUtilities::GetObjectByName(apNode, pTorsoPart->GetTargetName());
    }
}

void MiddleHighProcess::StoreDismemberedLimbNode(BGSBodyPart::LIMB_ENUM aeLimb, NiNode* apNode) {
    pDamageRootNode[aeLimb] = apNode;
    if (aeLimb == 1) {
        pHeadNode = nullptr;
    }
    if (aeLimb == 0) {
        pTorsoNode = nullptr;
    }
}

NiNode* MiddleHighProcess::GetDamageNode(BGSBodyPart::LIMB_ENUM aeIndex) {
    return pDamageRootNode[aeIndex];
}

NiAVObject* MiddleHighProcess::GetTorsoNode() {
    return pTorsoNode;
}

bool MiddleHighProcess::IsSustainedFire() {
    return bSustainedFire;
}

void MiddleHighProcess::SetSustainedFire(bool abValue) {
    bSustainedFire = abValue;
}

void MiddleHighProcess::UpdateArmorWorn(Actor* apActor) {
    bool bWasWearingPA = bPowerArmorWorn;
    bool bWasWearingBP = bHasBackPackWorn;
    bHeavyArmorWorn = false;
    bPowerArmorWorn = false;
    bPowerArmorHelmet = false;
    bHasBackPackWorn = false;
    for (BSSimpleList<ItemChange*>* pList = InventoryChanges::GetInventoryChanges(apActor)->pListofChanges;
         pList && !pList->IsEmpty() && (!bPowerArmorWorn || !bPowerArmorHelmet || !bHasBackPackWorn);
         pList = pList->m_pkNext) {
        ItemChange* pItem = pList->m_item;
        TESBoundObject* pObj = pItem->pContainerObj;
        if (pItem->GetWorn(false) && pObj->GetFormType() == ARMO_ID) {
            // The armor's TESBipedModelForm base, taken through a reference: there is deliberately no null test.
            TESBipedModelForm& rBiped = static_cast<TESObjectARMO&>(*pObj);
            if (rBiped.TESBipedModelForm::HasBackPack()) {
                bHasBackPackWorn = true;
            }
            if (rBiped.TESBipedModelForm::IsPowerArmor()) {
                if (rBiped.TESBipedModelForm::FillsBipedSlot(BO_HAIR, false, nullptr) ||
                    rBiped.TESBipedModelForm::FillsBipedSlot(BO_HEAD, false, nullptr)) {
                    bPowerArmorHelmet = true;
                }
                else {
                    bPowerArmorWorn = true;
                }
            }
            if (rBiped.TESBipedModelForm::IsHeavy() &&
                !rBiped.TESBipedModelForm::FillsBipedSlot(BO_HAIR, false, nullptr) &&
                !rBiped.TESBipedModelForm::FillsBipedSlot(BO_HEAD, false, nullptr)) {
                bHeavyArmorWorn = true;
            }
        }
    }
    if (!pSaveLoadGame->GetSaveGameLoading() &&
        (bWasWearingBP != bHasBackPackWorn || bWasWearingPA != bPowerArmorWorn) &&
        !bWeaponDrawn &&
        pWeapon &&
        apActor->IsNpc()) {
        Animation* pAnimation = apActor->Actor::GetAnimation();
        // Calls Character's biped getter directly on the actor, not through the virtual.
        ForceWeaponDrawnSheathed(false, static_cast<Character*>(apActor)->Character::GetBiped(), pAnimation, apActor);
    }
}

bool MiddleHighProcess::IsHeavyBodyArmorWorn() {
    return bHeavyArmorWorn;
}

bool MiddleHighProcess::IsPowerBodyArmorWorn() {
    return bPowerArmorWorn;
}

bool MiddleHighProcess::IsPowerArmorHelmetWorn() {
    return bPowerArmorHelmet;
}

bool MiddleHighProcess::HasBackPackWorn() {
    return bHasBackPackWorn;
}

void MiddleHighProcess::TryReserveProjectileClones(Actor* apActor) {
    if (!spReserveProjectileClones.m_pObject &&
        spWeaponPreload.m_pObject &&
        static_cast<QueuedFile*>(spWeaponPreload.m_pObject)->QueuedFile::GetAllChildrenFinished()) {
        TESBoundObject* pAmmoForm;
        TESObjectWEAP* pCurrentWeapon;
        ItemChange* pWeaponItem = GetCurrentWeapon();
        ItemChange* pAmmoItem = GetCurrentAmmo();
        if (pWeaponItem) {
            pCurrentWeapon = static_cast<TESObjectWEAP*>(GetCurrentWeapon()->pContainerObj);
        }
        else {
            pCurrentWeapon = nullptr;
        }
        if (pAmmoItem) {
            pAmmoForm = GetCurrentAmmo()->pContainerObj;
        }
        else {
            pAmmoForm = nullptr;
        }
        if (pCurrentWeapon && pAmmoForm && pCurrentWeapon->GetProjectile(apActor)) {
            int32_t iCloneCount = pCurrentWeapon->GetProjectile(apActor)->GetPreCloneCount(pCurrentWeapon);
            if (iCloneCount > 0) {
                NiPointer<Model> spModel;
                const char* pModel = static_cast<TESModel*>(pCurrentWeapon->GetProjectile(apActor))->GetModel();
                pModelLoader->FindModel(pModel, spModel);
                if (spModel.m_pObject && spModel->GetModel()) {
                    BSCloneReserver* pReserver = new BSCloneReserver(spModel->GetModel(), iCloneCount);
                    spReserveProjectileClones = pReserver;
                }
            }
        }
    }
}

void MiddleHighProcess::SetAnimActionSuccess(int32_t aivalue) {
    iAnimActionSuccess = aivalue;
}

void MiddleHighProcess::DeferHideDismemberedLimb(
    BGSBodyPart::LIMB_ENUM aeLimbIndex,
    NiNode* apDismemberedLimbRoot,
    NiAVObject* apReplacementLimb,
    bool abExplosion
) {
    MiddleHighProcess::DeferredHideLimb* pLimb = new MiddleHighProcess::DeferredHideLimb;
    pLimb->fDismemberTimer = 0.0f;
    pLimb->bExplosion = abExplosion;
    pLimb->eLimbIndex = aeLimbIndex;
    pLimb->spDismemberedLimbRoot = apDismemberedLimbRoot;
    pLimb->spReplacementLimb = static_cast<NiNode*>(apReplacementLimb);
    DeferredHideLimbList.AddHead(pLimb);
}

void MiddleHighProcess::UpdateDeferredHideDismemberedLimbs(Actor* apActor, float afDeltaTime) {
    float fTimer;
    if (pVATS.eMode == VATS::VATS_PLAYBACK) {
        fTimer = fExplodeLimbRemovalDelayVATS.Float();
    }
    else {
        fTimer = fExplodeLimbRemovalDelay.Float();
    }
    BSSimpleList<MiddleHighProcess::DeferredHideLimb*>* pList = &DeferredHideLimbList;
    BSSimpleList<MiddleHighProcess::DeferredHideLimb*>* pLastList = nullptr;
    while (pList && !pList->IsEmpty()) {
        MiddleHighProcess::DeferredHideLimb* pLimb = pList->m_item;
        bool bRemove = false;
        if (!pLimb->spDismemberedLimbRoot.m_pObject ||
            pLimb->spDismemberedLimbRoot->m_uiRefCount <= 1 ||
            (pLimb->spReplacementLimb.m_pObject && pLimb->spReplacementLimb->m_uiRefCount <= 1)) {
            bRemove = true;
        }
        pLimb->fDismemberTimer = pLimb->fDismemberTimer + afDeltaTime;
        if (!bRemove && fTimer < static_cast<double>(pLimb->fDismemberTimer)) {
            apActor->FillBoneArray(pLimb->spDismemberedLimbRoot.m_pObject, false);
            bool bExplosion = pLimb->bExplosion;
            apActor->HideDismemberedLimb(
                pLimb->eLimbIndex,
                pLimb->spDismemberedLimbRoot.m_pObject,
                pLimb->spReplacementLimb.m_pObject,
                bExplosion
            );
            bRemove = true;
        }
        if (bRemove) {
            if (pLimb) {
                delete pLimb;
            }
            if (pLastList) {
                // Remove is passed the limb local, which holds the freed item's pointer.
                MiddleHighProcess::DeferredHideLimb* pItem = pLimb;
                pLastList->Remove(pItem);
                pList = pLastList->m_pkNext;
            }
            else {
                pList->RemoveHead();
            }
        }
        else {
            pLastList = pList;
            pList = pList->m_pkNext;
        }
    }
}

void MiddleHighProcess::SetCreatureLipSynchAnim(LipSynchAnim* apCreatureLipsynchAnim) {
    if (pCreatureLipsynchAnim) {
        delete pCreatureLipsynchAnim;
    }
    pCreatureLipsynchAnim = apCreatureLipsynchAnim;
}

LipSynchAnim* MiddleHighProcess::GetCreatureLipSynchAnim() {
    return pCreatureLipsynchAnim;
}

void MiddleHighProcess::AddRadiationMagicDelta(float afValue) {
    fRadiationMagicDelta += afValue;
}

void MiddleHighProcess::RemoveRadiationMagicDelta(float afValue) {
    fRadiationMagicDelta -= afValue;
}

float MiddleHighProcess::GetRadiationMagicDelta() {
    return fRadiationMagicDelta;
}

void MiddleHighProcess::SetRadiationWaterDelta(float afValue) {
    fRadiationWaterDelta = afValue;
}

float MiddleHighProcess::GetRadiationWaterDelta() {
    return fRadiationWaterDelta;
}

void MiddleHighProcess::SetRadiationDelta(float afValue) {
    fRadiationDelta = afValue;
}

float MiddleHighProcess::GetRadiationDelta() {
    return fRadiationDelta;
}

HitData* MiddleHighProcess::GetLastHitData() {
    return pLastHitData;
}

void MiddleHighProcess::SetLastHitData(HitData* apData) {
    if (!pLastHitData) {
        pLastHitData = new HitData;
    }
    pLastHitData->Copy(apData);
}

void MiddleHighProcess::ClearLastHitData() {
    if (pLastHitData) {
        pLastHitData->ClearData();
    }
    else {
        pLastHitData = new HitData;
    }
}

HitData* MiddleHighProcess::GetLastAttackHitData() {
    return pLastAttackHitData;
}

void MiddleHighProcess::SetLastAttackHitData(HitData* apData) {
    if (!pLastAttackHitData) {
        pLastAttackHitData = new HitData;
    }
    pLastAttackHitData->Copy(apData);
}

void MiddleHighProcess::ClearLastAttackHitData() {
    if (pLastAttackHitData) {
        pLastAttackHitData->ClearData();
    }
    else {
        pLastAttackHitData = new HitData;
    }
}

int32_t MiddleHighProcess::GetWeaponConditionStage() {
    return iWeaponConditionStage;
}

void MiddleHighProcess::SetWeaponConditionStage(int32_t auiWeaponHealth) {
    iWeaponConditionStage = auiWeaponHealth;
}

void MiddleHighProcess::SetFaceNode(BSFaceGenNiNode* apNode) {
    pFaceNode = apNode;
}

NiAVObject* MiddleHighProcess::GetHeadAnims() {
    return pAnimFace;
}

// Caches whether health and fatigue are below their permanent values (an unordered compare counts as not below) and
// whether the actor is a ghost. Each current value is kept as a double.
void MiddleHighProcess::CheckCachedValueFlags(Actor* apActor) {
    ActorValueOwner* pOwner = apActor;
    const double dHealth = pOwner->GetActorFloatValue(ActorValue::HEALTH);
    bool bHealthDamaged = pOwner->GetPermanentActorFloatValue(ActorValue::HEALTH) > dHealth;
    const double dFatigue = pOwner->GetActorFloatValue(ActorValue::FATIGUE);
    bool bFatigueDamaged = pOwner->GetPermanentActorFloatValue(ActorValue::FATIGUE) > dFatigue;
    SetCachedValueFlag(CachedValues::CVF_HEALTH_DAMAGED, bHealthDamaged);
    SetCachedValueFlag(CachedValues::CVF_FATIGUE_DAMAGED, bFatigueDamaged);
    SetCachedValueFlag(CachedValues::CVF_ACTOR_IS_GHOST, apActor->GetExtra()->GetGhost());
}

void MiddleHighProcess::ProcessEscort(Actor* apActor) {
    NiPoint3 kTargLocCoord;
    NiPoint3 kMyLocCoord;
    TESPackage* pPack = GetPackageThatIsRunning();
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    if (!apActor->GetItemBeingEscorted()) {
        if (!pTarget || (!pTarget->IsActor() && !apActor->HasObjectId(pTarget->iFormID))) {
            AddToProcedureIndexRunning(apActor, 1);
            if (!apActor->IsPathingComplete()) {
                EndMoveMessage(apActor);
            }
            return;
        }
    }
    bool bIsActor = false;
    if (pTarget && pTarget->IsActor() && pTarget != apActor) {
        bIsActor = true;
        Actor* pAct = dynamic_cast<Actor*>(pTarget);
        TESPackage* pRunning = pAct->pCurrentProcess->GetCurrentPackage();
        if (pTarget != pPlayer &&
            (!pRunning || (pRunning->GetPackType() != PACKAGE_TYPE_FOLLOW && !pRunning->GetIsCreated()))) {
            SetProcedureIndexRunning(0);
            return;
        }
    }

    const NiPoint3* pOrigin = &apActor->GetLocationOnReference();
    const NiPoint3* pPos = &apActor->GetLocationOnReference();
    NiPoint3 kDistance = *pPos - *pOrigin;
    float fWaitDistance = 0.0f;
    Actor* pTargActor = nullptr;
    if (bIsActor) {
        const NiPoint3* pTargetPos = &pTarget->GetLocationOnReference();
        const NiPoint3* pActorPos = &apActor->GetLocationOnReference();
        kDistance = *pActorPos - *pTargetPos;
        pTargActor = static_cast<Actor*>(pTarget);
        fWaitDistance = 0.0f;
        int32_t iDistPack = pPack->GetPackageTarget()->iValue;
        if (iDistPack <= 0) {
            iDistPack = 200;
        }
        if (pTarget == pPlayer && fWaitDistance == 0.0) {
            fWaitDistance = static_cast<float>(iDistPack);
        }
        else if (fWaitDistance == 0.0 && apActor->GetParentCell()->GetInterior()) {
            fWaitDistance = fAIEscortWaitDistanceInterior.Float();
        }
        else if (fWaitDistance == 0.0) {
            fWaitDistance = fAIEscortWaitDistanceExterior.Float() * static_cast<double>(iDistPack);
        }
    }
    if (pPack->pPackLoc && pPack->IsActorAtLocation(apActor, false, -1.0f, false)) {
        if (!bIsActor) {
            ItemChange* pItem;
            pItem = apActor->GetItemBeingEscorted();
            if (pItem) {
                ExtraDataList* pExtraList = nullptr;
                if (pItem->pExtraObjectList) {
                    pExtraList = pItem->pExtraObjectList->m_item;
                }
                uint32_t uiID = 0;
                if (pExtraList && pExtraList->GetReferencePointer()) {
                    uiID = pExtraList->GetReferencePointer()->iFormID;
                }

                TESObjectREFR* pContainer = pPack->pPackLoc->GetLocReference();
                if (!pContainer) {
                    pContainer = pGenericLocation;
                }
                if (pContainer && pContainer->HasContainer()) {
                    int32_t iCount = pPack->GetInitialTargetCount();
                    apActor->PutItemInReference(pItem->pContainerObj, pExtraList, pContainer, iCount, uiID);
                }
                else {
                    if (pItem->pExtraObjectList) {
                        pExtraList = pItem->pExtraObjectList->m_item;
                    }
                    const NiPoint3* pPoint = nullptr;
                    PackageLocation* pPackLoc = pPack->pPackLoc;
                    if (pPackLoc) {
                        TESObjectREFR* pRef = pPackLoc->GetLocReference();
                        if (!pRef) {
                            pRef = pGenericLocation;
                        }
                        if (pRef) {
                            TESBoundObject* pBaseObject = pRef->GetObjectReference();
                            if (pBaseObject == pXMarker || pRef->GetObjectReference() == pXMarkerHeading) {
                                pPoint = &pRef->GetLocationOnReference();
                            }
                        }
                    }
                    int32_t iCount = pPack->GetInitialTargetCount();
                    apActor->DropObject(pItem->pContainerObj, pExtraList, iCount, pPoint, nullptr);
                }
            }
            if (pItem) {
                delete pItem;
            }
            AddToProcedureIndexRunning(apActor, 1);
        }
        else {
            double dDistance = kDistance.Length();
            if (fWaitDistance >= dDistance) {
                AddToProcedureIndexRunning(apActor, 1);
                Actor* pTargetActor = static_cast<Actor*>(pTarget);
                if (pTargetActor->pCurrentProcess->GetPackageThatIsRunning()) {
                    pTargetActor->pCurrentProcess->AddToProcedureIndexRunning(apActor, 1);
                    if (pTargetActor->pCurrentProcess->GetPackageThatIsRunning()->GetIsCreated()) {
                        TESPackage* pFollow = pTargetActor->pCurrentProcess->GetPackageThatIsRunning();
                        if (pFollow) {
                            delete pFollow;
                        }
                        pTargetActor->pCurrentProcess->ClearCurrentPackage();
                        if (pTargetActor->IsPathingComplete()) {
                            pTargetActor->EndMovement();
                        }
                        CheckforNewPackage(apActor, false);
                        return;
                    }
                }
            }
        }
        if (!apActor->IsPathingComplete()) {
            EndMoveMessage(apActor);
        }
    }
    else {
        if (bIsActor) {
            double dMyDistance;
            NiPoint3* pMyLocCoord = &(kMyLocCoord = pPack->GetLocationCoord(apActor));
            const NiPoint3* pMyPos = &apActor->GetLocationOnReference();
            NiPoint3 kMyDistance = *pMyPos - *pMyLocCoord;
            NiPoint3* pTargLocCoord = &(kTargLocCoord = pPack->GetLocationCoord(apActor));
            const NiPoint3* pTargPos = &pTargActor->GetLocationOnReference();
            NiPoint3 kTargDistance = *pTargPos - *pTargLocCoord;
            double dDistance = kDistance.Length();
            if (((fWaitDistance < dDistance &&
                  (dMyDistance = kMyDistance.Length(), kTargDistance.Length() >= dMyDistance)) ||
                 (pTargActor != pPlayer &&
                  pTargActor->GetCurrentPackage() &&
                  pTargActor->GetCurrentPackage()->IsInterruptPackage())) &&
                !apActor->IsPathingComplete()) {
                SetCurrentMovementComplete(true);
                AddToProcedureIndexRunning(apActor, -1);
                return;
            }
        }
        TESObjectCELL* pLocCell;
        TESWorldSpace* pLocWorld;
        NiPoint3 kLocCoord;
        if (apActor->IsPathingComplete() && (!bIsActor || fWaitDistance >= static_cast<double>(kDistance.Length()))) {
            pLocWorld = pPack->GetLocationWorld(apActor);
            pLocCell = pPack->GetLocationCell(apActor);
            kLocCoord = pPack->GetLocationCoord(apActor);
            if (!SetupPathFinding(apActor, kLocCoord, pLocCell, pLocWorld)) {
                return;
            }
        }
    }
}

void MiddleHighProcess::ProcessSleep(Actor* apActor) {
    if (apActor->IsNpc() &&
        apActor->GetSitSleepState() == SIT_SLEEP_NORMAL &&
        !pCurrentFurniture &&
        RefListChairBed.IsEmpty()) {
        FindBedChairs(apActor, false, nullptr);
    }
    if (apActor->GetSitSleepState() != SIT_SLEEP_IS_SLEEPING && apActor->IsNpc()) {
        if (!pCurrentFurniture && RefListChairBed.ItemsInList() && TESObjectREFR::kFurnitureLock.TryLock()) {
            pCurrentFurniture = static_cast<TESObjectREFR*>(RefListChairBed.m_item);
            ClearFurnitureMarker();
            if (pCurrentFurniture->GetOwner()) {
                RefListChairBed.Remove(*reinterpret_cast<void**>(&pCurrentFurniture));
            }
            else {
                int32_t iNumber = RefListChairBed.ItemsInList();
                uint32_t uiRandom = Random();
                int32_t iRandom = uiRandom % iNumber;
                if (iRandom >= iNumber) {
                    iRandom = iNumber;
                }
                BSSimpleList<void*>* pList = &RefListChairBed;
                for (int32_t i = 0; i < iRandom; ++i) {
                    pList = pList->m_pkNext;
                }
                pCurrentFurniture = static_cast<TESObjectREFR*>(pList->m_item);
                ClearFurnitureMarker();
            }
            TESObjectREFR::kFurnitureLock.Unlock();
        }

        SetTarget(pCurrentFurniture);
        if (pCurrentFurniture) {
            ProcessActivate(apActor, false);
        }
        if (DidPathingFail(apActor)) {
            pCurrentFurniture = nullptr;
            ClearFurnitureMarker();
        }
    }
    if ((apActor->IsNpc() && apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) ||
        (!pCurrentFurniture && RefListChairBed.IsEmpty())) {
        SetCurrentActionComplete(true);
        RefListChairBed.RemoveAll();
    }
    SetupSpecialIdle(apActor, nullptr, AnimIdle::PLAY_TYPE_AUTO, true, false, true);
}

void MiddleHighProcess::ProcessEat(Actor* apActor, TESObjectREFR* apLocRef) {
    GetPackageThatIsRunning();
    ItemChange* pFood = apActor->GetBestFoodItem();
    if (GetCurrentActionComplete()) {
        SetupSpecialIdle(apActor, nullptr, AnimIdle::PLAY_TYPE_AUTO, true, false, true);
    }
    else {
        if (apActor->IsNpc() &&
            apActor->GetSitSleepState() == SIT_SLEEP_NORMAL &&
            !pCurrentFurniture &&
            RefListChairBed.IsEmpty()) {
            FindBedChairs(apActor, true, apLocRef);
        }
        if (apActor->IsNpc() && apActor->GetSitSleepState() != SIT_SLEEP_IS_SITTING) {
            if (!pCurrentFurniture && RefListChairBed.ItemsInList() && TESObjectREFR::kFurnitureLock.TryLock()) {
                pCurrentFurniture = static_cast<TESObjectREFR*>(RefListChairBed.m_item);
                ClearFurnitureMarker();
                if (pCurrentFurniture->GetOwner()) {
                    RefListChairBed.Remove(*reinterpret_cast<void**>(&pCurrentFurniture));
                }
                else {
                    int32_t iNumber = RefListChairBed.ItemsInList();
                    uint32_t uiRandom = Random();
                    int32_t iRandom = uiRandom % iNumber;
                    if (iRandom >= iNumber) {
                        iRandom = iNumber;
                    }
                    BSSimpleList<void*>* pList = &RefListChairBed;
                    for (int32_t i = 0; i < iRandom; ++i) {
                        pList = pList->m_pkNext;
                    }
                    pCurrentFurniture = static_cast<TESObjectREFR*>(pList->m_item);
                    ClearFurnitureMarker();
                    RefListChairBed.Remove(*reinterpret_cast<void**>(&pCurrentFurniture));
                }
                TESObjectREFR::kFurnitureLock.Unlock();
            }

            SetTarget(pCurrentFurniture);
            if (pCurrentFurniture) {
                ProcessActivate(apActor, false);
            }
            if (DidPathingFail(apActor)) {
                ClearFurniture(apActor);
            }
        }
        if ((apActor->IsNpc() && apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING) ||
            (!pCurrentFurniture && RefListChairBed.IsEmpty())) {
            ExtraDataList* pExtra = nullptr;
            if (pFood) {
                if (pFood->pExtraObjectList) {
                    pExtra = pFood->pExtraObjectList->m_item;
                }
                apActor->Eat(pFood->pContainerObj, pExtra, true);
            }
            SetCurrentActionComplete(true);
            RefListChairBed.RemoveAll();
        }
        SetupSpecialIdle(apActor, nullptr, AnimIdle::PLAY_TYPE_AUTO, true, false, true);
        if ((apActor->GetMoveMode() & 1024) != 0) {
            apActor->SetWalkRunFlagDirty();
            apActor->ClearMoveMode(1024u);
        }
    }
}

void MiddleHighProcess::ProcessCombat(Actor* apActor) {
    int32_t iRoundAmound = static_cast<int32_t>(fCombatRoundAmount.Float());
    Actor* pTargetActor = dynamic_cast<Actor*>(pTarget);
    bool bRoundDone = false;
    if (pTargetActor && !pTargetActor->GetDisabled()) {
        if (((apActor->GetParentCell() && apActor->GetParentCell() == pTargetActor->GetParentCell()) ||
             apActor->GetDistanceFromReference(pTargetActor, false, false) < 300.0) &&
            pTargetActor->GetCurrentAIProcess() &&
            pTargetActor->GetCurrentAIProcess()->GetProcessLevel() == PROCESS_TYPE_MIDDLE_HIGH) {
            while (iRoundAmound && !apActor->IsDead(false)) {
                apActor->PerformOneRoundofCombat();
                pTargetActor = dynamic_cast<Actor*>(pTarget);
                if (pTargetActor && !pTargetActor->IsDead(false)) {
                    if (pTargetActor->GetCurrentAIProcess()->GetProcessLevel() == PROCESS_TYPE_MIDDLE_HIGH) {
                        pTargetActor->PerformOneRoundofCombat();
                        bRoundDone = true;
                    }
                }

                --iRoundAmound;
            }
            if (bRoundDone && pTargetActor) {
                pTargetActor->GetCurrentAIProcess()->ComputeLastTimeProcessed();
            }
        }
        else {
            ProcessFollow(apActor, true, 257, false);
        }
    }
    else {
        apActor->StopCombat(pTargetActor);
    }
}

void MiddleHighProcess::ProcessAlarm(Actor* apActor) {
    AlarmPackage* pPack = static_cast<AlarmPackage*>(GetCurrentPackage());
    Actor* pTargetActor = static_cast<Actor*>(pPack->GetPackageTarget()->GetTargReference());
    bool bCombatFlag = false;
    if (pPack) {
        if (apActor->GetClass()->IsGuard()) {
            AddToProcedureIndexRunning(apActor, 1);
        }
        else {
            for (Crime* pCrime : pPack->Crimes.ItemsUntilNull()) {
                Actor* pCrimeAct = nullptr;
                TESObjectREFR* pCrimeRef = pCrime->pcrimetarget;
                if (pCrimeRef && pCrimeRef->IsActor()) {
                    pCrimeAct = dynamic_cast<Actor*>(pCrimeRef);
                }

                if (!pCrime->breported) {
                    float fRadius2 = static_cast<float>(iCrimeAlarmRecDistance.Int());
                    const NiPoint3* pPos2 = &apActor->GetLocationOnReference();
                    float fRadius1 = static_cast<float>(iCrimeAlarmRecDistance.Int());
                    const NiPoint3* pPos1 = &apActor->GetLocationOnReference();
                    TESObjectCELL* pCell = apActor->GetParentCell();
                    pDataHandler->EnumReferencesCloseToPoint(
                        pCell,
                        *pPos1,
                        fRadius1,
                        *pPos2,
                        fRadius2,
                        &HighProcess::FindDoorsToSendYellAlarm,
                        pCrime
                    );
                }
                if (!bCombatFlag) {
                    FIGHT_REACTION eReaction = FIGHT_REACTION_NEUTRAL;
                    if (pCrime->ecrime <= PackageNS::CRIME_TYPE_TRESPASS) {
                        if (apActor->GetShouldAttackActor(pTargetActor, false, eReaction, false)) {
                            bCombatFlag = true;
                        }
                    }
                    else if (pCrimeAct && apActor->GetShouldAttackActor(pTargetActor, false, eReaction, false)) {
                        bCombatFlag = true;
                    }
                }
            }
            if (bCombatFlag) {
                EnterCombat(
                    apActor,
                    pTargetActor,
                    false,
                    false,
                    nullptr,
                    false,
                    false,
                    false,
                    0,
                    false,
                    false,
                    true,
                    nullptr
                );
            }
            else {
                AddToProcedureIndexRunning(apActor, 3);
            }
        }
    }
}

bool MiddleHighProcess::ProcessActivate(Actor* apActor, bool abInterateindex) {
    TESPackage* pPack = nullptr;
    pPack = GetPackageThatIsRunning();
    if (!pTarget || pTarget->GetDelete()) {
        SetTargetForPackage(apActor);
    }
    if (!pTarget) {
        AddToProcedureIndexRunning(apActor, 1);
        return false;
    }
    TESObjectREFR* pRef = pPack->GetLocationReference(apActor);
    if (pRef && pRef->IsFurniture()) {
        TESBoundObject* pBaseObject = pTarget->GetObjectReference();
        if (pBaseObject == pXMarkerHeading || pTarget->GetObjectReference() == pXMarker) {
            SetActionHeadTrackTarget(pRef);
            return true;
        }
    }

    if (pPack->GetPackType() == PACKAGE_TYPE_AMBUSH && !pPack->IsTargetAtLocation(apActor, 300)) {
        AddToProcedureIndexRunning(apActor, -1);
    }
    DoorTeleportData* pTel = pTarget->GetTeleport();
    bool bAtRef = false;
    if (pTel) {
        const NiPoint3* pActorPos = &apActor->GetLocationOnReference();
        const NiPoint3* pDoorPos = &pTarget->GetLinkedDoorTeleportPosition();
        NiPoint3 kDist = *pDoorPos - *pActorPos;
        double dLength = kDist.Length();
        if (static_cast<double>(iAIDistanceRadiusMinLocation.Int()) >= dLength) {
            bAtRef = true;
        }
    }
    else if (pTarget->IsFurniture() && apActor->GetSitSleepState()) {
        bAtRef = true;
    }
    else {
        if (pTarget->IsFurniture() &&
            apActor->GetSitSleepState() == SIT_SLEEP_NORMAL &&
            pTarget->GetMarkerUsed(cCurrentFurnitureIndex, true) &&
            !apActor->IsPathingComplete()) {
            ClearFurniture(apActor);
            EndMoveMessage(apActor);
            SetTarget(nullptr);
            return false;
        }
        bAtRef = pPack->IsActorAtRefTarget(apActor, 0);
    }
    if (!bAtRef && !DidPathingFail(apActor)) {
        if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
            apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
            apActor->InitiateGetUpPackage();
            return true;
        }
        if (apActor->IsPathingComplete()) {
            if (pTarget->GetTeleport()) {
                if (pTarget->IsFurniture()) {
                    NiPoint3 kPoint = apActor->GetLocationOnReference();
                    uint32_t uiIndex = 0;
                    if (!pTarget->GetClosestFreeMarker(kPoint, true, true, FurnitureMarker, uiIndex, true)) {
                        ClearFurniture(apActor);
                        EndMoveMessage(apActor);
                        SetTarget(nullptr);
                        return false;
                    }
                    TESWorldSpace* pWorld = pTarget->GetWorldSpace();
                    if (!SetupPathFinding(apActor, FurnitureMarker.Position, pTarget->GetParentCell(), pWorld)) {
                        return false;
                    }
                    cCurrentFurnitureIndex = uiIndex;
                }
                else {
                    TESWorldSpace* pWorld = pTarget->GetWorldSpace();
                    TESObjectCELL* pCell = pTarget->GetParentCell();
                    NiPoint3 kDoorPos = pTarget->GetLinkedDoorTeleportPosition();
                    if (!SetupPathFinding(apActor, kDoorPos, pCell, pWorld)) {
                        return false;
                    }
                }
            }
            else {
                TESWorldSpace* pWorld = pTarget->GetWorldSpace();
                TESObjectCELL* pCell = pTarget->GetParentCell();
                if (!SetupPathFinding(apActor, pTarget->GetLocationOnReference(), pCell, pWorld)) {
                    return false;
                }
            }
        }
        if (!apActor->IsPathingComplete()) {
            if (apActor->GetCurrentProcessType() != PROCESS_TYPE_MIDDLE_HIGH ||
                apActor->GetDesiredProcessLevel() != PROCESS_TYPE_MIDDLE_HIGH) {
                return false;
            }
            bAtRef = pPack->IsActorAtRefTarget(apActor, 0);
        }
    }
    if (!bAtRef && !DidPathingFail(apActor)) {
        return false;
    }
    if (!apActor->IsPathingComplete()) {
        EndMoveMessage(apActor);
    }
    if (GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE && DidPathingFail(apActor)) {
        apActor->EndDialogue();
        return false;
    }
    if (abInterateindex) {
        TESObjectREFR* pObject = pTarget;
        if (!DidPathingFail(apActor) && !pPack->GetMustComplete()) {
            AddToProcedureIndexRunning(apActor, 1);
        }
        if (!DidPathingFail(apActor)) {
            if (pTarget->IsFurniture()) {
                if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
                    apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                    AddToProcedureIndexRunning(apActor, 1);
                    SetTargetActivated(true);
                    return true;
                }
                if (!iAnimActionSuccess) {
                    AddToProcedureIndexRunning(apActor, 1);
                    pGenericLocation = nullptr;
                    EndMoveMessage(apActor);
                    iAnimActionSuccess = 1;
                    return true;
                }
                AlignToFurnitureAndAddPostAnimSitSleep(apActor);
            }
            else if (pAcquireObject) {
                pTarget->Activate(apActor, true, pAcquireObject->pObject, 1);
            }
            else if (!pTarget->IsActor() && !pTarget->Activate(apActor, true, nullptr, 1) && pPack->GetMustComplete()) {
                AddToProcedureIndexRunning(apActor, -1);
            }
        }
        if (pObject) {
            pObject->RunScript();
        }
    }
    else {
        TESObjectREFR* pObject;
        pObject = pTarget;
        if (!DidPathingFail(apActor)) {
            if (pTarget->IsFurniture()) {
                if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
                    apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                    SetTargetActivated(true);
                    return true;
                }
                if (!iAnimActionSuccess) {
                    AddToProcedureIndexRunning(apActor, 1);
                    pGenericLocation = nullptr;
                    EndMoveMessage(apActor);
                    iAnimActionSuccess = 1;
                    return true;
                }
                AlignToFurnitureAndAddPostAnimSitSleep(apActor);
            }
            else if (pAcquireObject) {
                pTarget->Activate(apActor, false, pAcquireObject->pObject, 1);
            }
            else {
                pTarget->Activate(apActor, false, nullptr, 1);
            }
        }
        if (pObject) {
            pObject->RunScript();
        }
    }
    delete pAcquireObject;
    pAcquireObject = nullptr;
    SetTarget(nullptr);
    return DidPathingFail(apActor) == 0;
}

void MiddleHighProcess::ProcessAcquire(Actor* apActor) {
    TESPackage* pPack = GetPackageThatIsRunning();
    if (pPack) {
        int32_t iNumber = 1;
        if (pPack->GetPackageTarget()) {
            iNumber = pPack->GetInitialTargetCount();
        }
        PackageTarget* pPackTarget = pPack->GetPackageTarget();
        if (pPack->GetPackType() != PACKAGE_TYPE_EXPLORE ||
            !pPackTarget ||
            (pPackTarget->GetTargObjectType() != POTYPE_COMBATWEAR &&
             pPackTarget->GetTargObjectType() != POTYPE_WEAR)) {
            if (CheckIfHasObject(apActor, iNumber)) {
                EndMoveMessage(apActor);
                if (GetRunOncePackage()) {
                    if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC) {
                        AddToProcedureIndexRunning(apActor, 2);
                    }
                    else {
                        AddToProcedureIndexRunning(apActor, 1);
                    }
                }
                else if (
                    pPack->IsActorAtLocation(apActor, false, -1.0f, false) ||
                    pPack->GetPackType() == PACKAGE_TYPE_ESCORT
                ) {
                    if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC) {
                        AddToProcedureIndexRunning(apActor, 2);
                    }
                    else {
                        AddToProcedureIndexRunning(apActor, 1);
                    }
                }
                else {
                    SetProcedureIndexRunning(0);
                }
                return;
            }
            if (!pTarget || pTarget->GetDelete() || pTarget->GetDisabled()) {
                SetTargetForPackage(apActor);
            }
            if (!pTarget) {
                AddToProcedureIndexRunning(apActor, 1);
                return;
            }
            if (pTarget->IsActor()) {
                if (pAcquireObject) {
                    if (pAcquireObject->eType == ACQUIRE_TYPE_PICKPOCKET) {
                        ProcessPickPocket(apActor);
                    }
                    else {
                        ProcessBuyObject(apActor);
                    }
                }
            }
            else if (pAcquireObject) {
                if (pAcquireObject->eType == ACQUIRE_TYPE_STEALGROUND) {
                    ProcessSteal(apActor);
                }
                else {
                    ProcessActivate(apActor, false);
                }
            }
            else {
                if (!pPack->GetPackageTarget() || pPack->GetPackageTarget()->GetTargReference() != pTarget) {
                    AddToProcedureIndexRunning(apActor, 1);
                    return;
                }
                ProcessActivate(apActor, true);
            }
            if (apActor->GetCurrentProcessType() == PROCESS_TYPE_MIDDLE_HIGH &&
                apActor->GetDesiredProcessLevel() == PROCESS_TYPE_MIDDLE_HIGH &&
                CheckIfHasObject(apActor, iNumber)) {
                EndMoveMessage(apActor);
                if (GetRunOncePackage()) {
                    if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC) {
                        AddToProcedureIndexRunning(apActor, 2);
                    }
                    else {
                        AddToProcedureIndexRunning(apActor, 1);
                    }
                }
                else if (
                    pPack->IsActorAtLocation(apActor, false, -1.0f, false) ||
                    pPack->GetPackType() == PACKAGE_TYPE_ESCORT
                ) {
                    if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC) {
                        AddToProcedureIndexRunning(apActor, 2);
                    }
                    else {
                        AddToProcedureIndexRunning(apActor, 1);
                    }
                }
                else {
                    SetProcedureIndexRunning(0);
                }
            }
        }
        else {
            ProcessActivate(apActor, true);
            if (GetProcedureIndexRunning() > 1) {
                if (ObjectList.IsEmpty()) {
                    AddToProcedureIndexRunning(apActor, 1);
                    if (apActor->IsNpc()) {
                        TESNPC* pNPC = dynamic_cast<TESNPC*>(apActor->GetObjectReference());
                        if (pNPC) {
                            pNPC->InitDefaultWorn(apActor, true, true, false, true);
                        }
                    }
                    else {
                        TESCreature* pCreature = dynamic_cast<TESCreature*>(apActor->GetObjectReference());
                        if (pCreature) {
                            pCreature->InitDefaultWorn(apActor, true, true, true);
                        }
                    }
                }
                else {
                    AddToProcedureIndexRunning(apActor, -1);
                    pAcquireObject = ObjectList.m_item;
                    ObjectList.Remove(pAcquireObject);
                    SetTarget(pAcquireObject->pRef);
                }
            }
        }
    }
}

void MiddleHighProcess::ProcessFleeNonCombat(Actor* apActor) {
    TESPackage* pPackage = GetPackageThatIsRunning();
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    SetLocationForPackage(apActor, false);
    float fRadius = 0.0f;
    if (pPackage->GetPackageTarget()) {
        fRadius = static_cast<float>(pPackage->GetPackageTarget()->iValue);
    }
    else {
        fRadius = static_cast<float>(pPackage->GetLocationRadius(apActor));
    }
    if (pTarget && static_cast<double>(fRadius) < apActor->GetDistanceFromReference(pTarget, false, false)) {
        apActor->InitiateFlee(apActor, true, true, true, nullptr, nullptr, -1.0f, -1.0f);
        FleePackage* pPack = dynamic_cast<FleePackage*>(GetPackageThatIsRunning());
        Actor* pAct = dynamic_cast<Actor*>(pTarget);
        if (pPack && pAct) {
            pPack->AddAvoidedRef(pAct);
        }
    }
    else if (pTarget || pPackage->IsActorAtLocation(apActor, false, -1.0f, false)) {
        if (!apActor->IsPathingComplete()) {
            EndMoveMessage(apActor);
        }
    }
    else {
        NiPoint3 kPoint;
        TESObjectCELL* pCell = pPackage->GetLocationCell(apActor);
        kPoint = pPackage->GetLocationCoord(apActor);
        if (apActor->IsPathingComplete()) {
            TESWorldSpace* pWorld = apActor->GetWorldSpace();
            SetupPathFinding(apActor, kPoint, pCell, pWorld);
        }
        else {
            SetActorsAnimation(apActor, 513, true);
        }
    }
}

void MiddleHighProcess::ProcessSandBox(Actor* apActor) {
    NiPoint3 kDestination;
    SandBoxActorPackageData* pData = static_cast<SandBoxActorPackageData*>(BaseProcess::GetInstanceDataThatIsRunning());
    if (pData) {
        TESPackage* pPack = GetCurrentPackage();
        float fRadius = static_cast<float>(pPack->GetLocationRadius(apActor));
        bool bAtLoc = pData->UpdateIsActorAtLocation(apActor, fRadius);
        TESObjectREFR* pLoc = pData->pLocation;
        float fArbitraryRadiusPadding = 150.0f;
        bool bNeedToRepath = false;
        if (pLoc && pData->pCurrentTargetObject) {
            bNeedToRepath = fRadius + fArbitraryRadiusPadding <
                pLoc->GetDistanceFromReference(pData->pCurrentTargetObject, false, false);
            if (bNeedToRepath) {
                pData->ForgetTargetAndPickProcedure(apActor, pPack);
            }
        }
        else if (apActor->IsPathing()) {
            if (pLoc) {
                NiPointer<PathingRequest> spRequest(apActor->pActorMover->GetCurrentRequest());
                if (spRequest.m_pObject) {
                    const PathingLocation* pPathLoc = &spRequest->Destination;
                    TESWorldSpace* pLocWorld = pPathLoc->pWorldSpace;
                    TESObjectCELL* pLocIntCell = pPathLoc->GetCell();
                    if (pLocIntCell && !pLocIntCell->GetInterior()) {
                        pLocIntCell = nullptr;
                    }

                    TESWorldSpace* pSandboxWorld = pLoc->GetWorldSpace();
                    TESObjectCELL* pSandboxIntCell = pLoc->GetParentCell();
                    if (pSandboxIntCell && !pSandboxIntCell->GetInterior()) {
                        pSandboxIntCell = nullptr;
                    }

                    bool bSameSpace = false;
                    if (pSandboxIntCell || pLocIntCell) {
                        if (pSandboxIntCell == pLocIntCell) {
                            bSameSpace = true;
                        }
                    }
                    else {
                        bSameSpace = pSandboxWorld == pLocWorld;
                    }
                    float fPaddedRadius = fRadius + fArbitraryRadiusPadding;
                    if (bSameSpace && fRadius >= 0.0) {
                        NiPoint3* pDestination = ::new (&kDestination) NiPoint3(pPathLoc->GetLocation());
                        const NiPoint3* pLocPos = &pLoc->GetLocationOnReference();
                        NiPoint3 kDelta = *pLocPos - *pDestination;
                        double dSqrLength = kDelta.SqrLength();
                        bNeedToRepath = fPaddedRadius * fPaddedRadius < dSqrLength;
                    }
                }
            }
        }
        else {
            bNeedToRepath = !bAtLoc;
        }
        if (bNeedToRepath) {
            TESPackage* pRunningPack = GetPackageThatIsRunning();
            float fPackRadius = static_cast<float>(pRunningPack->GetLocationRadius(apActor));
            float fDefaultRadius = PathingRequest::GetDefaultRadius();
            float fRequestRadius = BSMAX(fPackRadius, fDefaultRadius);
            PathingRequest kRequest;
            if (pLoc) {
                TESWorldSpace* pWorld = pLoc->GetWorldSpace();
                TESObjectCELL* pCell = pLoc->GetParentCell();
                const NiPoint3* pPos = &pLoc->GetLocationOnReference();
                apActor->BuildRequest(&kRequest, *pPos, pCell, pWorld, fRequestRadius, nullptr);
            }
            else {
                TESWorldSpace* pWorld = pPack->GetLocationWorld(apActor);
                TESObjectCELL* pCell = pPack->GetLocationCell(apActor);
                apActor
                    ->BuildRequest(&kRequest, pPack->GetLocationCoord(apActor), pCell, pWorld, fRequestRadius, nullptr);
            }
            kRequest.SetSoftGoalRadius(false);
            apActor->SetPathfindingGoal(&kRequest);
            pData->eState = SandBoxActorPackageData::SANDBOX_STATE_GETTING_UP;
        }
        else {
            switch (pData->eState) {
                case SandBoxActorPackageData::SANDBOX_STATE_INIT: {
                    pData->eState = SandBoxActorPackageData::SANDBOX_STATE_GETTING_UP;
                    pData->InitLocation(apActor, pPack->pPackLoc);
                    [[fallthrough]];
                }
                case SandBoxActorPackageData::SANDBOX_STATE_GETTING_UP: {
                    if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING ||
                        apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING) {
                        apActor->InitiateGetUpPackage();
                    }
                    else if (pData->IsDoneWithIdleAnims(apActor, true)) {
                        pData->eState = SandBoxActorPackageData::SANDBOX_STATE_MOVING;
                    }
                    return;
                }
                case SandBoxActorPackageData::SANDBOX_STATE_MOVING: {
                    switch (pData->eProcedure) {
                        case SANDBOX_PROCEDURE_SIT: {
                            if (apActor->GetSitSleepState() >= SIT_SLEEP_LOAD_SIT_IDLE &&
                                apActor->GetSitSleepState() <= SIT_SLEEP_IS_SITTING) {
                                pData->eState = SandBoxActorPackageData::SANDBOX_STATE_DOINGSTUFF;
                            }
                            break;
                        }
                        case SANDBOX_PROCEDURE_SLEEP: {
                            if (apActor->GetSitSleepState() >= SIT_SLEEP_LOAD_SLEEP_IDLE &&
                                apActor->GetSitSleepState() <= SIT_SLEEP_IS_SLEEPING) {
                                pData->eState = SandBoxActorPackageData::SANDBOX_STATE_DOINGSTUFF;
                            }
                            break;
                        }
                        case SANDBOX_PROCEDURE_EAT: {
                            if (apActor->GetSitSleepState() >= SIT_SLEEP_LOAD_SIT_IDLE &&
                                apActor->GetSitSleepState() <= SIT_SLEEP_IS_SITTING) {
                                pData->eState = SandBoxActorPackageData::SANDBOX_STATE_DOINGSTUFF;
                            }
                            break;
                        }
                        case SANDBOX_PROCEDURE_WANDER: {
                            pData->eState = SandBoxActorPackageData::SANDBOX_STATE_DOINGSTUFF;
                            break;
                        }
                        case SANDBOX_PROCEDURE_USE_IDLE_MARKER: {
                            if (!apActor->IsPathing() && pCurrentIdle) {
                                pData->eState = SandBoxActorPackageData::SANDBOX_STATE_DOINGSTUFF;
                            }

                            break;
                        }
                        default: {
                            break;
                        }
                    }
                    [[fallthrough]];
                }
                case SandBoxActorPackageData::SANDBOX_STATE_DOINGSTUFF: {
                    if (pData->ShouldPickNewProcedure(apActor)) {
                        if (pData->MoveToNextLocation()) {
                            TESObjectREFR* pNextLoc = pData->pLocation;
                            if (pNextLoc) {
                                TESPackage* pRunningPack = GetPackageThatIsRunning();
                                float fPackRadius = static_cast<float>(pRunningPack->GetLocationRadius(apActor));
                                float fDefaultRadius = PathingRequest::GetDefaultRadius();
                                float fRequestRadius = BSMAX(fPackRadius, fDefaultRadius);
                                PathingRequest kRequest;
                                TESWorldSpace* pWorld = pNextLoc->GetWorldSpace();
                                TESObjectCELL* pCell = pNextLoc->GetParentCell();
                                const NiPoint3* pPos = &pNextLoc->GetLocationOnReference();
                                apActor->BuildRequest(&kRequest, *pPos, pCell, pWorld, fRequestRadius, nullptr);
                                kRequest.SetSoftGoalRadius(false);
                                apActor->SetPathfindingGoal(&kRequest);
                                pData->eState = SandBoxActorPackageData::SANDBOX_STATE_GETTING_UP;
                            }
                        }
                        SetupNewPackage();
                        apActor->ClearLookAtTarget();
                        pData->PickProcedure(apActor, GetCurrentPackage());
                        pData->eState = SandBoxActorPackageData::SANDBOX_STATE_GETTING_UP;
                    }
                    TESObjectREFR* pSandboxTarget = pData->pCurrentTargetObject;
                    bool bTargetOK = true;
                    if (!pSandboxTarget) {
                        if (SandBoxActorPackageData::IsTargetNeededForProcedure(pData->eProcedure)) {
                            bTargetOK = false;
                        }
                    }
                    bool bProcedureValid = bTargetOK;
                    if (bTargetOK) {
                        switch (pData->eProcedure) {
                            case SANDBOX_PROCEDURE_SIT:
                            case SANDBOX_PROCEDURE_SLEEP: {
                                bProcedureValid = ProcessUseFurniture(apActor, pSandboxTarget);
                                break;
                            }
                            case SANDBOX_PROCEDURE_EAT: {
                                ProcessEat(apActor, pData->pLocation);
                                break;
                            }
                            case SANDBOX_PROCEDURE_WANDER: {
                                ProcessWander(apActor, nullptr, -1.0f);
                                break;
                            }
                            case SANDBOX_PROCEDURE_USE_IDLE_MARKER: {
                                ProcessUseIdleMarker(apActor, pSandboxTarget);
                                break;
                            }
                            case SANDBOX_PROCEDURE_DIALOGUE: {
                                if (ProcessSandBoxDialogue(apActor, pSandboxTarget)) {
                                    pData->PickProcedure(apActor, GetCurrentPackage());
                                    pData->eState = SandBoxActorPackageData::SANDBOX_STATE_MOVING;
                                }
                                break;
                            }
                            default: {
                                bProcedureValid = false;
                                break;
                            }
                        }
                    }
                    ActorMover* pMover = apActor->pActorMover;
                    if (!bProcedureValid || (pMover && pMover->IsPathIncomplete())) {
                        pData->ForgetTargetAndPickProcedure(apActor, GetCurrentPackage());
                        pData->eState = SandBoxActorPackageData::SANDBOX_STATE_GETTING_UP;
                    }
                    if (apActor->IsPathing()) {
                        SetActorsAnimation(apActor, 257, true);
                    }
                    break;
                }
                default: {
                    return;
                }
            }
        }
    }
    else {
        AddToProcedureIndexRunning(apActor, 1);
    }
}

void MiddleHighProcess::ProcessMovementBlocked(Actor* apActor) {
    ActorPackage* pPackage;
    if (GetRunOncePackage()) {
        pPackage = &RunOncePackage;
    }
    else {
        pPackage = &CurrentPackage;
    }
    double dHour = appCalendar.GetHour();
    float fTimeDiff = dHour - pPackage->GetPackageStartTime();
    float fAbsTimeDiff = std::fabs(fTimeDiff);
    if (!(fAbsTimeDiff >= 0.05000000074505806)) {
        apActor->StopMoving();
    }
    else {
        AddToProcedureIndexRunning(apActor, 1);
        TESPackage* pCurrPackage = CurrentPackage.pPackage;
        pCurrPackage->SetIsCreated(true);
    }
}

void MiddleHighProcess::ProcessPatrol(Actor* apActor) {
    uint32_t eState;
    PatrolActorPackageData* pCurrentPackageInstanceData = static_cast<PatrolActorPackageData*>(
        BaseProcess::GetInstanceDataThatIsRunning()
    );
    if (pCurrentPackageInstanceData) {
        if (apActor->GetSitSleepState() == SIT_SLEEP_NORMAL ||
            pCurrentPackageInstanceData->GetCurrentReference() == pCurrentFurniture) {
            eState = pCurrentPackageInstanceData->eState;
            if (eState) {
                if (eState == 1) {
                    ProcessPatrolTravelingToNextRef(apActor, pCurrentPackageInstanceData);
                }
                else {
                    if (eState == 2) {
                        ProcessPatrolAtRef(apActor, pCurrentPackageInstanceData);
                    }
                }
            }
            else {
                ProcessPatrolInit(apActor, pCurrentPackageInstanceData);
            }
        }
        else {
            apActor->InitiateGetUpPackage();
        }
    }
    else {
        SetProcedureIndexRunning(0xFFFFFFFFu);
    }
}

void MiddleHighProcess::ProcessNotify(Actor* apActor) {
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    AddToProcedureIndexRunning(apActor, 1u);
}

bool MiddleHighProcess::CheckIfHasObject(Actor* apActor, int32_t ainumb) {
    PackageTarget* pPackTarget = GetPackageThatIsRunning()->GetPackageTarget();
    if (!pPackTarget) {
        return false;
    }
    PACK_TARG_TYPE eType = pPackTarget->GetTargType();
    bool bDone = false;
    if (pPackTarget->GetTargType() && pPackTarget->GetTargType() != PTT_LINKEDREF) {
        if (eType == PTT_OBJECT) {
            TESBoundObject* pTargObject = static_cast<TESBoundObject*>(pPackTarget->GetTargObject());
            return apActor->HasObjects(pTargObject, POTYPE_NONE, ainumb, 0, eUseItem);
        }

        PACKAGE_OBJECT_TYPE eTargObjectType = pPackTarget->GetTargObjectType();
        return apActor->HasObjects(nullptr, eTargObjectType, ainumb, 0, eUseItem);
    }
    else {
        ActorPackage* pActorPack = GetActorPackageThatIsRunning();
        if (pActorPack) {
            TESObjectREFR* pRef = pActorPack->pTarget;
            if (pRef) {
                uint32_t uiFormID = pRef->iFormID;
                return apActor->HasObjects(pRef->GetObjectReference(), POTYPE_NONE, 1, uiFormID, eUseItem);
            }
        }
    }
    return bDone;
}

TESObjectREFR* MiddleHighProcess::GetTargetForPackage(Actor* apActor, TESPackage* apPackage) {
    TESObjectREFR* pReturnTarget = nullptr;
    bool bCurrentPackage = apPackage == GetCurrentPackage();
    switch (apPackage->GetPackType()) {
        case PACKAGE_TYPE_FOLLOW:
        case PACKAGE_TYPE_ESCORT:
        case PACKAGE_TYPE_ACCOMPANY: {
            if (apPackage->IsTargetAnActor(apActor)) {
                pReturnTarget = apPackage->GetEscortTargetForPackage(apActor, false);
            }
            break;
        }
        case PACKAGE_TYPE_PATROL: {
            if (bCurrentPackage) {
                PatrolActorPackageData* pData = static_cast<PatrolActorPackageData*>(
                    BaseProcess::GetInstanceDataThatIsRunning()
                );
                if (pData) {
                    pReturnTarget = pData->GetCurrentReference();
                }
            }
            if (!pTarget) {
                pReturnTarget = PatrolActorPackageData::GetInitialReferenceForSetup(apActor, apPackage);
            }
            break;
        }
        case PACKAGE_TYPE_IN_GAME_DIALOGUE: {
            pReturnTarget = static_cast<DialoguePackage*>(apPackage)->pTargetActor;
            break;
        }
        case PACKAGE_TYPE_DIALOGUE: {
            if (apPackage->GetPackageTarget()->GetTargReference() == apActor) {
                pReturnTarget = apActor;
                break;
            }
            [[fallthrough]];
        }
        default: {
            pReturnTarget = SetTargetData(apActor, apPackage, apPackage->GetPackageTarget());
            break;
        }
    }
    return pReturnTarget;
}

TESObjectREFR* MiddleHighProcess::GetLocationForPackage(Actor* apActor, TESPackage* apPackage, bool bSecondLoc) {
    if (!apPackage) {
        return nullptr;
    }
    PackageLocation* pLoc = nullptr;
    TESObjectREFR* pReturnLocationRef = nullptr;
    if (bSecondLoc) {
        pLoc = apPackage->GetPackageSecondLocation();
    }
    else {
        pLoc = apPackage->pPackLoc;
    }
    if (pLoc) {
        if (pLoc->GetLocType() == PLT_REFERENCE) {
            pReturnLocationRef = pLoc->GetLocReference();
        }
        if (pLoc->GetLocType() == PLT_LINKEDREF) {
            return apActor->GetLinkedRef();
        }

        if (pLoc->IsLocationGeneric()) {
            if (GenericLocationsList.IsEmpty()) {
                TESObjectCELL* pCell = apActor->GetParentCell();
                NiPoint3 kPointB = apActor->GetLocationOnReference();
                PACK_LOC_TYPE eType = pLoc->GetLocType();
                if (eType == PLT_OBJECT) {
                    pObjectForLocation = static_cast<TESBoundObject*>(pLoc->GetLocObject());
                    eFormType = POTYPE_NONE;
                }
                else if (eType == PLT_TYPE) {
                    pObjectForLocation = nullptr;
                    eFormType = pLoc->GetLocObjectType();
                }
                float fAcquireDist = 0.0f;
                if (!apPackage->GetAcquireRadius(apActor, fAcquireDist)) {
                    fAcquireDist = fAIAcquireObjectDistance.Float();
                }
                float fRadius2 = fAcquireDist;
                const NiPoint3* pPos = &apActor->GetLocationOnReference();
                pDataHandler->EnumReferencesCloseToPoint(
                    pCell,
                    kPointB,
                    fAcquireDist,
                    *pPos,
                    fRadius2,
                    &EvaluateGenericLocation,
                    apActor
                );
                eFormType = POTYPE_NONE;
                pObjectForLocation = nullptr;
                for (BSSimpleList<TESObjectREFR*>* pList = &TempLocList; pList->m_item; pList->RemoveHead()) {
                    GenericLocationsList.AddHead(pList->m_item);
                }
                TempLocList.RemoveAll();
            }
            if (!GenericLocationsList.IsEmpty()) {
                pReturnLocationRef = GenericLocationsList.m_item;
                GenericLocationsList.RemoveHead();
            }
        }
    }
    return pReturnLocationRef;
}

void MiddleHighProcess::EvaluateOrderAcquireList(Actor* apActor) {
    bool bAllowKilling;
    bool bAllowStealing;
    bool bAllowBuying;
    BSSimpleList<ObjectstoAcquire*>* pRefItem = &TempObjectList;
    TESPackage* pPack = GetPackageThatIsRunning();
    if (pPack) {
        bAllowBuying = pPack->SearchGetAllowBuying();
    }
    else {
        bAllowBuying = false;
    }
    if (pPack) {
        bAllowStealing = pPack->SearchGetAllowStealing();
    }
    else {
        bAllowStealing = false;
    }
    if (pPack) {
        bAllowKilling = pPack->SearchGetAllowKilling();
    }
    else {
        bAllowKilling = false;
    }
    while (pRefItem && pRefItem->m_item) {
        ObjectstoAcquire* pObject = pRefItem->m_item;
        TESObjectREFR* pRef = pObject->pRef;
        bool bIsActor = pRef->IsActor();
        Actor* pBuyIt = bIsActor ? static_cast<Actor*>(pRef) : nullptr;
        TESBoundObject* pObj = pObject->pObject;
        char cBought = 0;
        bool bAdd = true;
        if (pBuyIt) {
            bAdd = !pBuyIt->IsPlayerTeammate();
        }
        if (pRef->GetOwner() || pRef->IsActor()) {
            bool bSteal = false;
            bool bKill = false;
            int32_t iValue = TESValueForm::GetFormValue(pObj);
            TESNPC* pNPC = nullptr;
            if (!pRef->IsDead(false)) {
                int32_t iGoldAmount = apActor->GetGoldAmount();
                if (iGoldAmount >= iValue) {
                    Actor* pBuyActor = nullptr;
                    if (pRef->IsNpc()) {
                        pBuyActor = static_cast<Actor*>(pRef);
                    }
                    else {
                        pNPC = dynamic_cast<TESNPC*>(pRef->GetOwner());
                        if (pNPC) {
                            pBuyActor = ReferenceProcessLists.GetActorRefInHigh(pNPC, false);
                        }
                    }
                    if (pBuyActor && bAllowBuying && pBuyActor->GetActorBase()->SellBuysItem(pObj)) {
                        int32_t iGold = apActor->GetGoldAmount();
                        if (iGold >= TESValueForm::GetFormValue(pObject->pObject)) {
                            if (apActor->IsInCombat()) {
                                bAdd = false;
                            }
                            else {
                                pObject->eType = ACQUIRE_TYPE_BUY_NPC;
                                uint32_t uiDistance = static_cast<int32_t>(
                                    apActor->GetDistanceFromReference(pRef, false, false)
                                );
                                pObject->iWantValue = static_cast<int32_t>(uiDistance);
                                cBought = 1;
                            }
                        }
                    }
                }
            }
            if (!cBought && (!pBuyIt || !pBuyIt->IsPlayerTeammate())) {
                if (pRef->IsActor()) {
                    if (pRef->IsDead(false)) {
                        bAdd = true;
                    }
                    else if (pRef->IsNpc()) {
                        if (bAllowStealing) {
                            bSteal = true;
                        }
                        if (bAllowKilling) {
                            bKill = true;
                        }
                    }
                }
                else if (bAllowStealing) {
                    bSteal = true;
                }
                if ((!bSteal && !bKill) || (pBuyIt && pBuyIt->IsPlayerTeammate())) {
                    if (apActor->IsCreature()) {
                        bAdd = false;
                    }
                    else {
                        TESBoundObject* pOwner;
                        if (!pRef->IsActor() ||
                            pRef->IsNpc() ||
                            (pOwner = static_cast<TESBoundObject*>(pRef->GetOwner()),
                             pOwner == apActor->GetObjectReference()) ||
                            apActor->bPlayerTeammate) {
                            bAdd = false;
                        }
                        else if (pBuyIt && !pBuyIt->IsPlayerTeammate()) {
                            pObject->eType = ACQUIRE_TYPE_KILL;
                            bAdd = true;
                        }
                    }
                }
                else {
                    if (pRef->IsActor()) {
                        if (bSteal) {
                            if (apActor->IsInCombat()) {
                                bAdd = false;
                            }
                            else {
                                pObject->eType = ACQUIRE_TYPE_PICKPOCKET;
                            }
                        }
                        else if (bKill) {
                            if (apActor->IsInCombat() || apActor->bPlayerTeammate) {
                                bAdd = false;
                            }
                            else if (!pBuyIt || !pBuyIt->IsPlayerTeammate()) {
                                pObject->eType = ACQUIRE_TYPE_KILL;
                            }
                        }
                    }
                    else if (pObject->eType == ACQUIRE_TYPE_NOOWNED_CONTAINER) {
                        PACKAGE_OBJECT_TYPE eUseItem = POTYPE_NONE;
                        pObject->eType = ACQUIRE_TYPE_PICKPOCKET;
                        if (!apActor->HasObjects(pLockpick, POTYPE_NONE, 1, 0, eUseItem) && pRef->GetLock()) {
                            bAdd = false;
                        }
                    }
                    else {
                        pObject->eType = ACQUIRE_TYPE_STEALGROUND;
                    }
                    pObject->bSteal = bSteal;
                    pObject->bKill = bKill;
                }
            }
        }
        else {
            double dDistance = apActor->GetDistanceFromReference(pRef, false, false);
            pObject->iWantValue = static_cast<int32_t>(dDistance);
        }
        if (bAdd) {
            pObject->pRef->SetTargeted(true);
            ObjectList.AddTail(pObject);
        }
        else {
            delete pObject;
        }
        pRefItem = pRefItem->m_pkNext;
    }
    static_cast<HighProcess*>(this)->HighProcess::SortObjectsListByType();
    static_cast<HighProcess*>(this)->HighProcess::SortObjectsListByValue();
    TempObjectList.RemoveAll();
}

bool MiddleHighProcess::ProcessCombatOneHour(Actor* apActor, float afTimeDelta) {
    int32_t iNumber = static_cast<int32_t>(fCombatRoundAmount.Float() * 4.0);
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    Actor* pTargetActor = dynamic_cast<Actor*>(pTarget);
    if (pTarget && !pTarget->GetDisabled()) {
        if (pTargetActor &&
            apActor->GetDistanceFromReference(pTargetActor, false, false) < 250.0 &&
            pTargetActor->pCurrentProcess) {
            while (iNumber && !apActor->IsDead(false)) {
                apActor->PerformOneRoundofCombat();
                if (!pTargetActor->IsDead(false)) {
                    pTargetActor->PerformOneRoundofCombat();
                }
                --iNumber;
            }
        }
        else {
            ProcessFollowOneHour(apActor, 3600.0f, false);
        }
        if (pTargetActor && !pTargetActor->IsDead(false)) {
            pTargetActor->pCurrentProcess->ComputeLastTimeProcessed();
        }

        return false;
    }

    apActor->StopCombat(pTargetActor);
    apActor->EndInterruptPackage(false);
    return false;
}

void MiddleHighProcess::SetAnimation(Animation* apAnimation) {
    if (pAnimation) {
        if (pAnimation == apAnimation) {
            return;
        }
        ourMemset(pAimSequence, 0, sizeof(pAimSequence));
        GarbageCollector::Add(pAnimation);
    }
    pAnimation = apAnimation;
}

bool MiddleHighProcess::SetupPathFinding(
    Actor* apActor,
    NiPoint3 aDestPoint,
    TESObjectCELL* apDestIntCell,
    TESWorldSpace* apDestWorld
) {
    if (apActor->GetSitSleepState() != SIT_SLEEP_IS_SITTING && apActor->GetSitSleepState() != SIT_SLEEP_IS_SLEEPING) {
        return apActor->SetPathfindingGoal(aDestPoint, apDestIntCell, apDestWorld, -1.0f, nullptr);
    }
    apActor->InitiateGetUpPackage();
    return false;
}

void MiddleHighProcess::ResetTarget() {
    BaseProcess* pProcess = this;
    TESPackage* pPackage = pProcess->GetPackageThatIsRunning();
    if (pPackage && pPackage->GetPackageTarget() && pPackage->GetPackageTarget()->GetTargReference()) {
        TESObjectREFR* pRef = pPackage->GetPackageTarget()->GetTargReference();
        if (!pRef->GetDelete() && pTarget && pRef != pTarget) {
            pProcess->SetTarget(pRef);
        }
    }
}

// Buy procedure: walks to the seller (through a load door when the target is one), then takes the object off the seller
// (or activates the container for its owner), pays the seller and fires OnSell.
void MiddleHighProcess::ProcessBuyObject(Actor* apActor) {
    GetPackageThatIsRunning();
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    if (!pTarget || pTarget->GetDelete() || pTarget->GetDisabled()) {
        AddToProcedureIndexRunning(apActor, 1);
        return;
    }
    if (!pAcquireObject) {
        return;
    }
    ObjectstoAcquire* pAcquire = pAcquireObject;
    if (pAcquire->pObject == pAcquireObject->pRef->GetObjectReference()) {
        SetTarget(pAcquireObject->pRef);
        return;
    }
    uint8_t bArrived = 0;
    if (pTarget->GetTeleport()) {
        const NiPoint3* pActorPos = &apActor->GetLocationOnReference();
        NiPoint3 kOffset = pTarget->GetLinkedDoorTeleportPosition() - *pActorPos;
        if (kOffset.Length() <= 128.0) {
            bArrived = 1;
        }
    }
    else {
        bArrived = GetCurrentPackage()->IsActorAtRefTarget(apActor, 0);
    }
    if (!bArrived && !DidPathingFail(apActor)) {
        if (apActor->IsPathingComplete()) {
            if (!pTarget->GetTeleport()) {
                TESWorldSpace* pWorld = pTarget->GetWorldSpace();
                TESObjectCELL* pCell = pTarget->GetParentCell();
                if (!SetupPathFinding(apActor, pTarget->GetLocationOnReference(), pCell, pWorld)) {
                    return;
                }
            }
            else {
                TESWorldSpace* pWorld = pTarget->GetWorldSpace();
                TESObjectCELL* pCell = pTarget->GetParentCell();
                if (!SetupPathFinding(apActor, pTarget->GetLinkedDoorTeleportPosition(), pCell, pWorld)) {
                    return;
                }
            }
        }
        return;
    }
    if (!apActor->IsPathingComplete()) {
        EndMoveMessage(apActor);
    }
    if (!DidPathingFail(apActor)) {
        TESObjectREFR* pSeller = nullptr;
        if (!pAcquireObject->pRef->IsActor()) {
            TESNPC* pOwner = dynamic_cast<TESNPC*>(pAcquireObject->pRef->GetOwner());
            if (pOwner) {
                pSeller = ReferenceProcessLists.GetActorRefInHigh(pOwner, false);
            }
            pAcquireObject->pRef->Activate(apActor, false, static_cast<TESBoundObject*>(pAcquireObject->pObject), 1);
        }
        else {
            pSeller = pTarget;
            pSeller->RemoveItem(
                static_cast<TESBoundObject*>(pAcquireObject->pObject),
                nullptr,
                1,
                false,
                false,
                apActor,
                nullptr,
                nullptr,
                true,
                false
            );
        }
        if (pSeller) {
            int32_t iValue = TESValueForm::GetFormValue(pAcquireObject->pObject);
            apActor->PayGoldToActor(apActor, (pAcquireObject->iamount * iValue));
        }
        ExtraDataList* pExtra = nullptr;
        pExtra = pAcquireObject->pExtra;
        Script::SetActionFlag(pSeller, pExtra, SCRIPT_ACTION_ONSELL);
    }
    delete pAcquireObject;
    SetTarget(nullptr);
    pAcquireObject = nullptr;
}

void MiddleHighProcess::ProcessSteal(Actor* apActor) {
    ProcessActivate(apActor, false);
}

void MiddleHighProcess::ProcessPickPocket(Actor* apActor) {
    ProcessActivate(apActor, false);
}

void MiddleHighProcess::ProcessDialogue(MobileObject* apActor) {
    DialoguePackage* pDialoguePack = static_cast<DialoguePackage*>(GetPackageThatIsRunning());
    MobileObject* pConversationTarget = pDialoguePack->GetTargetOfConversation();
    MobileObject* pOther = pDialoguePack->GetActorStartedConversation();
    if (apActor == pOther) {
        pOther = pConversationTarget;
    }
    if (pDialoguePack->flistentimer > 0.0) {
        pDialoguePack->flistentimer = pDialoguePack->flistentimer - appTimer.GetSeconds();
    }
    else if (GetCurrentActionComplete()) {
        apActor->EndDialogue();
        TESPackage* pOtherPackage = pOther->GetCurrentPackage();
        if (pOtherPackage == apActor->GetCurrentPackage()) {
            pOther->EndDialogue();
        }
        SetCurrentActionComplete(false);
    }
    else {
        pDialoguePack->Speak(false);
    }
}

bool MiddleHighProcess::ProcessDialogueActivate(Actor* apActor) {
    TESPackage* pPackage = GetPackageThatIsRunning();
    if (pPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        if (pTarget || SetTargetForPackage(apActor)) {
            DialoguePackage* pDialoguePack = static_cast<DialoguePackage*>(GetPackageThatIsRunning());
            PlayerCharacter* pTargetActor = static_cast<PlayerCharacter*>(pDialoguePack->pTargetActor);
            MobileObject* pStarter = pDialoguePack->GetActorStartedConversation();
            if (pDialoguePack->IsCreated() || !pDialoguePack->GetPackageSecondLocation()) {
                if (apActor == pTargetActor) {
                    AddToProcedureIndexRunning(apActor, 1);
                }
                else {
                    ProcessActivate(apActor, true);
                }
                return true;
            }

            if (pDialoguePack->IsActorAtLocation(pStarter, false, -1.0f, true) &&
                pDialoguePack->IsActorAtSecondLocation(pTargetActor, apActor, false, -1.0f, true)) {
                if (pTargetActor == pPlayer) {
                    apActor->Activate(pTargetActor, false, nullptr, 1);
                }
                else {
                    AddToProcedureIndexRunning(apActor, 2);
                }
            }
            else if (pTargetActor == apActor && pTargetActor->IsActor()) {
                if (pTargetActor != pPlayer && !pDialoguePack->GetDialogueDoNotControlTarget()) {
                    pTargetActor->pCurrentProcess->ProcessTravel(pTargetActor, false, false, -1, true);
                }
            }
            else {
                ProcessTravel(apActor, false, false, 257, false);
            }
            return true;
        }

        AddToProcedureIndexRunning(apActor, 3);
        return false;
    }

    SetTargetForPackage(apActor);
    AddToProcedureIndexRunning(apActor, 3);
    apActor->InitiateDialogue(pPackage);
    return true;
}

bool MiddleHighProcess::ProcessUseFurniture(Actor* apActor, TESObjectREFR* apFurniture) {
    GetPackageThatIsRunning();
    if (!apFurniture || !apFurniture->IsFurniture()) {
        return false;
    }
    if (pCurrentFurniture != apFurniture) {
        if (apActor->GetSitSleepState()) {
            if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
                apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                apActor->InitiateGetUpPackage();
            }
            return true;
        }
        pCurrentFurniture = apFurniture;
        ClearFurnitureMarker();
    }
    if (!pCurrentFurniture) {
        ClearFurnitureMarker();
    }
    TESFurniture* pFurnitureForm = static_cast<TESFurniture*>(pCurrentFurniture->TESObjectREFR::GetObjectReference());
    int32_t iSitState = pFurnitureForm->CanSitOn() ? 4 : 9;
    if (static_cast<int32_t>(apActor->GetSitSleepState()) != iSitState && apActor->IsNpc()) {
        if (!pTarget) {
            SetTarget(pCurrentFurniture);
        }
        ProcessActivate(apActor, false);
    }
    if (static_cast<int32_t>(apActor->GetSitSleepState()) == iSitState) {
        SetCurrentActionComplete(true);
    }
    if (DidPathingFail(apActor)) {
        pCurrentFurniture = nullptr;
        FurnitureMarker.SetHeading(0.0);
        FurnitureMarker.SetType(0);
        FurnitureMarker.SetPosition(NiPoint3::ZERO);
        EndMoveMessage(apActor);
        AddToProcedureIndexRunning(apActor, 1);
        return false;
    }

    if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
        BSFaceGenAnimationData* pFaceAnimData = apActor->GetFaceAnimationData();
        if (pFaceAnimData && !pFaceAnimData->GetDead()) {
            pFaceAnimData->SetDead(true, false);
        }
    }
    return true;
}

void MiddleHighProcess::ProcessRemoveWorn(Actor* apActor, bool bInterateindex, bool bOnlyifClothes) {
    TESNPC* pNPC = dynamic_cast<TESNPC*>(apActor->TESObjectREFR::GetObjectReference());
    TESCreature* pCreature = dynamic_cast<TESCreature*>(apActor->TESObjectREFR::GetObjectReference());
    TESPackage* pPackage = GetCurrentPackage();
    uint8_t ucWorn = 1;
    uint8_t ucWearWeapon = 1;
    if (pPackage) {
        if (pPackage->GetPackType() == PACKAGE_TYPE_SLEEP) {
            bOnlyifClothes = true;
            ucWorn = 0;
            ucWearWeapon = 0;
        }
        else {
            ucWearWeapon = !pPackage->GetForceUnequipWeapon();
        }
    }
    if (pNPC) {
        pNPC->InitDefaultWorn(apActor, ucWorn, ucWearWeapon, bOnlyifClothes, true);
    }
    else if (pCreature) {
        pCreature->InitDefaultWorn(apActor, ucWorn, ucWearWeapon, true);
    }
    if (apActor && apActor->MobileObject::GetCurrentAIProcess()) {
        apActor->MobileObject::GetCurrentAIProcess()->Set3DUpdateFlag(RESET_MODEL);
    }

    if (bInterateindex) {
        AddToProcedureIndexRunning(apActor, 1u);
    }
}

// Starts a patrol: sets apData up for the current package and heads for the first point. When that fails, no
// procedure runs.
bool MiddleHighProcess::ProcessPatrolInit(Actor* apActor, PatrolActorPackageData* apData) {
    if (!apData->Init(apActor, GetCurrentPackage())) {
        SetProcedureIndexRunning(0xFFFFFFFF);
        return false;
    }
    ProcessPatrolStartTravelToNextRef(apActor, apData);
    return true;
}

void MiddleHighProcess::ProcessPatrolStartTravelToNextRef(Actor* apActor, PatrolActorPackageData* apData) {
    TESObjectREFR* pRef = apData->GetCurrentReference();
    if (pRef) {
        SetTarget(pRef);
        float fRadius = PathingRequest::GetDefaultRadius();
        TESPackage* pPackage = GetPackageThatIsRunning();
        if (pPackage) {
            float fPackRadius = static_cast<float>(pPackage->GetLocationRadius(apActor));
            fRadius = BSMAX(fRadius, fPackRadius);
        }
        TESBoundObject* pLocBound = pRef->GetObjectReference();
        int32_t iFormType = pLocBound->GetFormType();
        bool bIsIdleMarker = iFormType == 48;
        if (iFormType == 48) {
            fRadius = 10.0f;
        }
        PathingRequest kRequest;
        TESWorldSpace* pWorld = pRef->GetWorldSpace();
        TESObjectCELL* pCell = pRef->GetParentCell();
        const NiPoint3* pPos = &pRef->GetLocationOnReference();
        apActor->BuildRequest(&kRequest, *pPos, pCell, pWorld, fRadius, nullptr);
        kRequest.SetSoftGoalRadius(false);
        if (bIsIdleMarker || pLocBound == pXMarkerHeading) {
            kRequest.SetGoalAngleToFace(pRef->GetAngleOnReference().z);
        }
        apActor->SetPathfindingGoal(&kRequest);
        apData->eState = PatrolActorPackageData::PATROL_STATE_TRAVELING_TO_NEXT_REF;
        if (apActor->IsPathingComplete()) {
            ProcessPatrolStartAtRefBehavior(apActor, apData);
        }
    }
    else {
        SetProcedureIndexRunning(-1);
    }
}

void MiddleHighProcess::ProcessPatrolTravelingToNextRef(Actor* apActor, PatrolActorPackageData* apData) {
    if (apActor->DidPathingFail()) {
        ProcessPatrolStartTravelToNextRef(apActor, apData);
        return;
    }
    TESObjectREFR* pRef = apData->GetCurrentReference();
    if (pRef) {
        int32_t eMoveMode = 257;
        TESPackage* pPack = GetPackageThatIsRunning();
        bool bAlwaysRun = pPack->GetAlwaysRun();
        bool bCouldRun = false;
        if (bAlwaysRun) {
            float fCurrentDistance = apActor->GetDistanceFromReference(pRef, false, false);
            bCouldRun = 64.0f < static_cast<double>(fCurrentDistance);
            if (64.0f < static_cast<double>(fCurrentDistance)) {
                float fDistance = pPack->GetPackageRadiusActorToLocation(apActor, false);
                float fDoubleDistance = fDistance + fDistance;
                eMoveMode = CalculateMoveMode(apActor, fCurrentDistance, fDistance, fDoubleDistance, false, false);
            }
        }
        SetActorsAnimation(apActor, eMoveMode, bCouldRun);
    }
    if (pRef && pRef->GetObjectReference()->GetFormType() == FURN_ID) {
        ProcessUseFurniture(apActor, pRef);
        if (pRef == pCurrentFurniture &&
            (GetSitSleepState() == SIT_SLEEP_WAITING_FOR_SIT_ANIM ||
             GetSitSleepState() == SIT_SLEEP_WAITING_FOR_SLEEP_ANIM)) {
            ProcessPatrolStartAtRefBehavior(apActor, apData);
        }
        return;
    }
    bool bPauseAtWaypoint = false;
    TESPackage* pRunningPack = GetPackageThatIsRunning();
    if (pRunningPack) {
        BGSIdleCollection* pIdleCollection = pRunningPack->pIdleCollection;
        if (pIdleCollection) {
            bPauseAtWaypoint = pIdleCollection->GetIdleCount() > 0;
        }
    }
    bool bFinished = apActor->IsPathingComplete();
    float fCurrentPercent = apActor->pActorMover->GetCurrentPercent();
    float fTestValue = 0.925f;
    if (bFinished || (!bPauseAtWaypoint && fTestValue < static_cast<double>(fCurrentPercent))) {
        float fMinLength;
        float fMinLengthSqr;
        const NiPoint3* pRefPos = &pRef->GetLocationOnReference();
        const NiPoint3* pActorPos = &apActor->GetLocationOnReference();
        NiPoint3 kDiff = *pActorPos - *pRefPos;
        float fSqrLength = kDiff.SqrXYLength();
        float fRadius = 32.0f;
        if (pRef->GetObjectReference()->GetFormType() == IDLM_ID) {
            fRadius = PathingRequest::GetDefaultRadius();
        }
        else {
            TESBoundObject* pBaseObject = pRef->GetObjectReference();
            if (pBaseObject == pXMarkerHeading) {
                fRadius = PathingRequest::GetDefaultRadius();
            }
            TESPackage* pPackage = GetCurrentPackage();
            if (pPackage) {
                float fPackRadius = static_cast<float>(pPackage->GetLocationRadius(apActor));
                fRadius = BSMAX(fRadius, fPackRadius);
            }
        }
        float fRadiusSqr = fRadius * fRadius;
        bool bGoToNext = false;
        if (!bFinished) {
            fMinLength = fRadius + 50.0;
            fMinLengthSqr = fMinLength * fMinLength;
            if (fMinLengthSqr < static_cast<double>(fSqrLength) || kDiff.z * kDiff.z > 32400.0) {
                return;
            }
            float fTimeAtRef;
            fTimeAtRef = 0.0f;
            if (pRef->m_Extra.GetPatrolRefData()) {
                fTimeAtRef = pRef->m_Extra.GetPatrolRefData()->fTimeAtRef;
            }
            if (fTimeAtRef != 0.0) {
                return;
            }
            bGoToNext = true;
        }
        if (bGoToNext || !(fRadiusSqr < static_cast<double>(fSqrLength)) || !(kDiff.z * kDiff.z < 32400.0)) {
            ProcessPatrolStartAtRefBehavior(apActor, apData);
        }
        else {
            ProcessPatrolStartTravelToNextRef(apActor, apData);
        }
    }
}

void MiddleHighProcess::ProcessPatrolStartAtRefBehavior(Actor* apActor, PatrolActorPackageData* apData) {
    StartAtPatrolRefBehaviorLow(apActor, apData);
    apActor->ClearLookAtTarget();
    TESObjectREFR* pRef = apData->GetCurrentReference();
    if (!pRef || pRef->GetObjectReference()->GetFormType() != IDLM_ID) {
        fPackageIdleTimer = 0.0f;
    }
    PatrolRefData* pPatrolRefData = pRef->m_Extra.GetPatrolRefData();
    if (pPatrolRefData) {
        TESTopic* pTopic = pPatrolRefData->GetOnPatrolTopic();
        if (pTopic) {
            BaseProcess* pProcess = apActor->pCurrentProcess;
            if (!pProcess->GetGreetingFlag()) {
                ProcessGreet(apActor, pTopic, false, false, true, false);
            }
        }
        Script* pScript = pPatrolRefData->GetOnPatrolScript();
        if (pScript) {
            pScript->Run(apActor, nullptr, nullptr, false);
        }
    }
}

void MiddleHighProcess::ProcessPatrolAtRef(Actor* apActor, PatrolActorPackageData* apData) {
    NiPoint3 kDiff;
    TESObjectREFR* pCurrentRef = apData->GetCurrentReference();
    if (!pCurrentRef) {
        if (apData->SetNextReferenceForPatrol(true)) {
            ProcessPatrolStartTravelToNextRef(apActor, apData);
        }
        else {
            SetProcedureIndexRunning(-1);
        }
        return;
    }
    if (apActor->IsPathingComplete()) {
        const NiPoint3* pRefPos;
        pRefPos = &pCurrentRef->GetLocationOnReference();
        const NiPoint3* pActorPos;
        pActorPos = &apActor->GetLocationOnReference();
        kDiff = *pActorPos - *pRefPos;
        float fSqrLength;
        fSqrLength = kDiff.SqrXYLength();
        float fRadius;
        fRadius = 32.0f;
        if (pCurrentRef->GetObjectReference()->GetFormType() == 48) {
            fRadius = PathingRequest::GetDefaultRadius();
        }
        else {
            if (pCurrentRef->GetObjectReference() == pXMarkerHeading) {
                fRadius = PathingRequest::GetDefaultRadius();
            }
            TESPackage* pPackage = GetCurrentPackage();
            if (pPackage) {
                float fPackRadius = static_cast<float>(pPackage->GetLocationRadius(apActor));
                fRadius = BSMAX(fRadius, fPackRadius);
            }
        }
        fRadius = fRadius * fRadius;
        if (fRadius < static_cast<double>(fSqrLength) || std::fabsf(kDiff.z) > 180.0) {
            ProcessPatrolStartTravelToNextRef(apActor, apData);
            return;
        }
    }
    float fTimeAtRef = 0.0;
    if (pCurrentRef->GetExtra()->GetPatrolRefData()) {
        fTimeAtRef = pCurrentRef->GetExtra()->GetPatrolRefData()->GetTimeAtRef();
    }
    if (fTimeAtRef > 0.0) {
        if (pCurrentRef->GetObjectReference()->GetFormType() == 48) {
            ProcessUseIdleMarker(apActor, pCurrentRef);
        }
    }
    apData->fCurrentTimeAtReference = appTimer.GetSeconds() + apData->fCurrentTimeAtReference;
    if (fTimeAtRef <= static_cast<double>(apData->fCurrentTimeAtReference) &&
        !apActor->IsPlayingLowerBodySpecialIdle()) {
        if (!apData->SetNextReferenceForPatrol(true)) {
            AddToProcedureIndexRunning(apActor, 1);
        }
        else {
            ProcessPatrolStartTravelToNextRef(apActor, apData);
        }
    }
}

void MiddleHighProcess::ProcessAccompanyOneHour(Actor* apActor) {
    ProcessFollowOneHour(apActor, 3600.0f, true);
}

void MiddleHighProcess::SetWeaponFlags() {
    bWeaponGrenade = false;
    bWeaponMine = false;
    bWeaponThrown = false;
    if (pWeapon) {
        TESObjectWEAP* pWeaponObject = static_cast<TESObjectWEAP*>(pWeapon->GetContainerObject());
        if (pWeaponObject) {
            switch (pWeaponObject->GetWeaponType()) {
                case WEAPON_ONE_HAND_GRENADE: bWeaponGrenade = true; break;
                case WEAPON_ONE_HAND_MINE:
                case WEAPON_ONE_HAND_LUNCHBOX_MINE: bWeaponMine = true; break;
                case WEAPON_ONE_HAND_THROWN: bWeaponThrown = true; break;
            }
        }
    }
}

void MiddleHighProcess::SaveCharController(BGSSaveFormBuffer* apSaveGameBuffer) {
    NiPoint3 kPosition;
    char cWantState;
    uint32_t uiFlags;
    NiPoint3 kVelocity;
    char cState;
    Actor* pActor = apSaveGameBuffer->GetActor();
    bhkCharacterController* pCharController = GetCharController();
    if (pCharController && (!pActor || !pActor->IsDead(false))) {
        cState = static_cast<char>(pCharController->getState());
        apSaveGameBuffer->SaveDataEndian(&cState, sizeof(cState), 0);
        cWantState = pCharController->eWantState;
        apSaveGameBuffer->SaveDataEndian(&cWantState, sizeof(cWantState), 0);
        uiFlags = pCharController->GetFlag(0xFFFFFFFFu);
        apSaveGameBuffer->SaveDataEndian(&uiFlags, sizeof(uiFlags), 0);
        if ((uiFlags & bhkCharacterController::CHASEBIP) != 0) {
            pCharController->GetPosition(kPosition);
            apSaveGameBuffer->SaveDataEndian(&kPosition, sizeof(kPosition), 0);
        }
        apSaveGameBuffer->SaveDataEndian(&pCharController->OutVelocity, sizeof(pCharController->OutVelocity), 0);
        apSaveGameBuffer->SaveDataEndian(&pCharController->VelocityMod, sizeof(pCharController->VelocityMod), 0);
        pCharController->GetLinearVelocity(kVelocity);
        apSaveGameBuffer->SaveDataEndian(&kVelocity, sizeof(kVelocity), 0);
        if (pActor) {
            apSaveGameBuffer->SaveDataEndian(&pCharController->fJumpHeight, sizeof(pCharController->fJumpHeight), 0);
            apSaveGameBuffer
                ->SaveDataEndian(&pCharController->fFallStartHeight, sizeof(pCharController->fFallStartHeight), 0);
        }
        else {
            apSaveGameBuffer->SaveDataEndian(&pCharController->fGravity, sizeof(pCharController->fGravity), 0);
        }
        apSaveGameBuffer->SaveDataEndian(&pCharController->fFallTime, sizeof(pCharController->fFallTime), 0);
        apSaveGameBuffer->SaveDataEndian(&pCharController->fPitchAngle, sizeof(pCharController->fPitchAngle), 0);
        apSaveGameBuffer->SaveDataEndian(&pCharController->fPitchMult, sizeof(pCharController->fPitchMult), 0);
    }
}

// Reads the character controller's saved state: the true and wanted states, the listener flags (and the position when
// the collision was disabled), the velocities and the fall and pitch values. A sitting or sleeping actor is put on the
// ground with no velocity, without character collisions when its furniture asks for it.
void MiddleHighProcess::LoadCharController(BGSLoadFormBuffer* apLoadGameBuffer) {
    Actor* pActor = apLoadGameBuffer->GetActor();
    bhkCharacterController* pController = GetCharController();
    if (!pController) {
        return;
    }
    uint8_t ucTrueState = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucTrueState), sizeof(ucTrueState));
    pController->SetTrueState(static_cast<hkpCharacterStateType>(ucTrueState));
    uint8_t ucWantState = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucWantState), sizeof(ucWantState));
    pController->eWantState = static_cast<hkpCharacterStateType>(ucWantState);
    uint32_t uiFlags = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&uiFlags), sizeof(uiFlags));
    pController->bhkCharacterListener::ClearFlag(0xFFFFFFFF);
    pController->bhkCharacterListener::SetFlag(uiFlags, true);
    if (uiFlags & bhkCharacterController::CHASEBIP) {
        NiPoint3 kPosition;
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&kPosition), sizeof(kPosition));
        pController->SetPosition(kPosition);
    }
    if (pActor && pActor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
        pActor->SetChaseBip(false);
    }
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pController->OutVelocity), sizeof(pController->OutVelocity));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pController->VelocityMod), sizeof(pController->VelocityMod));
    alignas(16) NiPoint3 kVelocity;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&kVelocity), sizeof(kVelocity));
    pController->SetLinearVelocityNi(kVelocity);
    if (pActor) {
        apLoadGameBuffer->LoadData(
            reinterpret_cast<char*>(&pController->fJumpHeight),
            sizeof(pController->fJumpHeight)
        );
        apLoadGameBuffer->LoadData(
            reinterpret_cast<char*>(&pController->fFallStartHeight),
            sizeof(pController->fFallStartHeight)
        );
    }
    else {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pController->fGravity), sizeof(pController->fGravity));
    }
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pController->fFallTime), sizeof(pController->fFallTime));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pController->fPitchAngle), sizeof(pController->fPitchAngle));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pController->fPitchMult), sizeof(pController->fPitchMult));
    SIT_SLEEP_STATE eSitSleepState = pActor ? pActor->GetSitSleepState() : SIT_SLEEP_NORMAL;
    if (eSitSleepState == SIT_SLEEP_IS_SITTING || eSitSleepState == SIT_SLEEP_IS_SLEEPING) {
        if (ShouldDisableCharController(pCurrentFurniture, FurnitureMarker.GetType())) {
            pController->bhkCharacterListener::SetFlag(bhkCharacterController::NO_CHARACTER_COLLISIONS, true);
        }
        pController->SetTrueState(HK_CHARACTER_ON_GROUND);
        pController->SetLinearVelocity(hkVector4::getZero());
        alignas(16) hkVector4 kNormal(0.0f, 0.0f, 1.0f, 0.0f);
        pController->SetSupportNormal(kNormal);
    }
}

void MiddleHighProcess::LoadCharController(MobileObject* pmob) {
    NiPoint3 kPos;
    uint32_t iFlags;
    hkpCharacterStateType eCharState;
    NiPoint3 kLinearVelocity;
    bhkCharacterController* pCon = GetCharController();
    if (pCon && !pmob->IsDead(false)) {
        pSaveLoadGameOLD->LoadGameDataOLD(&eCharState, sizeof(eCharState));
        pCon->SetTrueState(eCharState);
        pSaveLoadGameOLD->LoadGameDataOLD(&pCon->eWantState, sizeof(pCon->eWantState));
        pSaveLoadGameOLD->LoadGameDataOLD(&iFlags, sizeof(iFlags));
        pCon->ClearFlag(0xFFFFFFFFu);
        pCon->SetFlag(iFlags, true);
        if ((iFlags & bhkCharacterController::CHASEBIP) != 0 && pSaveLoadGameOLD->GetCurrentVersion() >= 35u) {
            pSaveLoadGameOLD->LoadGameDataOLD(&kPos, sizeof(kPos));
            if (GetSitSleepState() && GetFurnitureMarker()) {
                MobileObject* pActor = nullptr;
                if (pmob->IsActor()) {
                    pActor = pmob;
                }
                if (!pActor) {
                    kPos = GetFurnitureMarker()->Position;
                }
            }

            pCon->SetPosition(kPos);
        }
        if (GetSitSleepState() == SIT_SLEEP_NORMAL) {
            pmob->SetChaseBip(false);
        }
        pSaveLoadGameOLD->LoadGameDataOLD(&pCon->OutVelocity, sizeof(pCon->OutVelocity));
        pSaveLoadGameOLD->LoadGameDataOLD(&pCon->VelocityMod, sizeof(pCon->VelocityMod));
        pSaveLoadGameOLD->LoadGameDataOLD(&kLinearVelocity, sizeof(kLinearVelocity));
        pCon->SetLinearVelocityNi(kLinearVelocity);
        if (pmob->IsActor()) {
            pSaveLoadGameOLD->LoadGameDataOLD(&pCon->fJumpHeight, sizeof(pCon->fJumpHeight));
            pSaveLoadGameOLD->LoadGameDataOLD(&pCon->fFallStartHeight, sizeof(pCon->fFallStartHeight));
            pSaveLoadGameOLD->LoadGameDataOLD(&pCon->fFallTime, sizeof(pCon->fFallTime));
        }
        else {
            if (pSaveLoadGameOLD->GetCurrentVersion() >= 32u) {
                pSaveLoadGameOLD->LoadGameDataOLD(&pCon->fGravity, sizeof(pCon->fGravity));
                pSaveLoadGameOLD->LoadGameDataOLD(&pCon->fFallTime, sizeof(pCon->fFallTime));
            }
        }
        if (eCharState == HK_CHARACTER_IN_AIR && (pCon->QSupported() || pCon->QSupported2())) {
            pCon->fFallStartHeight = 0.0f;
        }
        if (pSaveLoadGameOLD->GetCurrentVersion() >= 119u) {
            pSaveLoadGameOLD->LoadGameDataOLD(&pCon->fPitchAngle, sizeof(pCon->fPitchAngle));
            pSaveLoadGameOLD->LoadGameDataOLD(&pCon->fPitchMult, sizeof(pCon->fPitchMult));
        }
    }
}

void MiddleHighProcess::SaveCharController(MobileObject* pmob) {
    NiPoint3 kPos;
    uint32_t iFlags;
    hkpCharacterStateType eCharState;
    NiPoint3 kLinearVelocity;
    bhkCharacterController* pCon = GetCharController();
    if (pCon && !pmob->IsDead(false)) {
        eCharState = pCon->getState();
        pSaveLoadGameOLD->SaveGameDataOLD(&eCharState, sizeof(eCharState));
        pSaveLoadGameOLD->SaveGameDataOLD(&pCon->eWantState, sizeof(pCon->eWantState));
        iFlags = pCon->GetFlag(0xFFFFFFFFu);
        pSaveLoadGameOLD->SaveGameDataOLD(&iFlags, sizeof(iFlags));
        if ((iFlags & bhkCharacterController::CHASEBIP) != 0) {
            pCon->GetPosition(kPos);
            pSaveLoadGameOLD->SaveGameDataOLD(&kPos, sizeof(kPos));
        }
        pSaveLoadGameOLD->SaveGameDataOLD(&pCon->OutVelocity, sizeof(pCon->OutVelocity));
        pSaveLoadGameOLD->SaveGameDataOLD(&pCon->VelocityMod, sizeof(pCon->VelocityMod));
        pCon->GetLinearVelocity(kLinearVelocity);
        pSaveLoadGameOLD->SaveGameDataOLD(&kLinearVelocity, sizeof(kLinearVelocity));
        if (pmob->IsActor()) {
            pSaveLoadGameOLD->SaveGameDataOLD(&pCon->fJumpHeight, sizeof(pCon->fJumpHeight));
            pSaveLoadGameOLD->SaveGameDataOLD(&pCon->fFallStartHeight, sizeof(pCon->fFallStartHeight));
        }
        else {
            pSaveLoadGameOLD->SaveGameDataOLD(&pCon->fGravity, sizeof(pCon->fGravity));
        }
        pSaveLoadGameOLD->SaveGameDataOLD(&pCon->fFallTime, sizeof(pCon->fFallTime));
        if (pSaveLoadGameOLD->GetCurrentVersion() >= 119u) {
            pSaveLoadGameOLD->SaveGameDataOLD(&pCon->fPitchAngle, sizeof(pCon->fPitchAngle));
            pSaveLoadGameOLD->SaveGameDataOLD(&pCon->fPitchMult, sizeof(pCon->fPitchMult));
        }
    }
}

// Returns true on every path.
bool MiddleHighProcess::ReloadDynamicIdleOn1stPerson(Actor* apActor) {
    int32_t iIdleAnimGroupSection;
    Animation* pAnimation;
    TESIdleForm* pIdleForm;
    uint8_t bThirdPerson = pPlayer->Is3rdPerson();
    if (static_cast<const void*>(apActor) == static_cast<const void*>(pPlayer)) {
        if (bThirdPerson) {
            pPlayer->Force3rdPerson(false);
        }
        SetSitSleepState(apActor, SIT_SLEEP_LOAD_SIT_IDLE, pCurrentFurniture, cCurrentFurnitureIndex);
        pIdleForm = pIdleManager->GetIdleToPlay(apActor, pCurrentFurniture);
        SetSitSleepState(apActor, SIT_SLEEP_IS_SITTING, pCurrentFurniture, cCurrentFurnitureIndex);
        if (pIdleForm) {
            pAnimation = pPlayer->GetAnimation(true);
            if (pAnimation) {
                iIdleAnimGroupSection = pIdleForm->GetIdleAnimGroupSection();
                pAnimation
                    ->SpecialIdleReplace(pIdleForm, apActor, static_cast<ANIM_GROUP_SECTION>(iIdleAnimGroupSection));
            }
        }
        if (bThirdPerson) {
            pPlayer->Force3rdPerson(true);
        }
    }
    return true;
}

void MiddleHighProcess::ForceBoneScale() {}

void MiddleHighProcess::ClearReservation() {
    if (!pDataHandler->GetClearingData()) {
        if (pTarget && static_cast<int32_t>(iReservationSlot) >= 0) {
            pTarget->SetMarkerReserved(iReservationSlot, false);
            iReservationSlot = static_cast<uint32_t>(-1);
        }

        if (pCurrentFurniture && !pSaveLoadGame->GetSaveGameLoading()) {
            pCurrentFurniture->SetMarkerReserved(cCurrentFurnitureIndex, false);
        }
    }
}

void MiddleHighProcess::ClearDamageNodes() {
    ourMemset(pDamageRootNode, 0, sizeof(pDamageRootNode));
    pHeadNode = nullptr;
    pTorsoNode = nullptr;
}

void MiddleHighProcess::MiddleHighCopy(MiddleHighProcess* apProcess) {
    apProcess->RunOncePackage.Copy(&RunOncePackage);
    lastSeenPostion = apProcess->lastSeenPostion;
    pAnimation = apProcess->pAnimation;
    apProcess->pAnimation = nullptr;
    if (pAnimation) {
        pAnimation->ClearGroup(AGS_MOVEMENT, 0.0f);
    }
    pWeapon = apProcess->pWeapon;
    pAmmo = apProcess->pAmmo;
    bWeaponGrenade = apProcess->bWeaponGrenade;
    bWeaponMine = apProcess->bWeaponMine;
    bWeaponThrown = apProcess->bWeaponThrown;
    bHeavyArmorWorn = apProcess->bHeavyArmorWorn;
    bPowerArmorWorn = apProcess->bPowerArmorWorn;
    bPowerArmorHelmet = apProcess->bPowerArmorHelmet;
    bHasBackPackWorn = apProcess->bHasBackPackWorn;
    bWantWeaponDrawn = apProcess->GetWantWeaponDrawn();
    bWeaponDrawn = apProcess->GetWeaponDrawn();
    cKnockState = apProcess->cKnockState;
    pCurrentPackageSpell = apProcess->pCurrentPackageSpell;
    bNeedReparentWeapon = apProcess->bNeedReparentWeapon;
    pFireNode = apProcess->pFireNode;
    pWeaponBone = apProcess->pWeaponBone;
    bPickPocketed = apProcess->bPickPocketed;
    bDoneClothesChange = apProcess->bDoneClothesChange;
    apProcess->pFireNode = nullptr;
    apProcess->pWeapon = nullptr;
    apProcess->pAmmo = nullptr;
    apProcess->bWeaponGrenade = false;
    apProcess->bWeaponThrown = false;
    apProcess->bWeaponMine = false;
    eUseItem = apProcess->eUseItem;
    fEquippedWeight = apProcess->fEquippedWeight;
    pLastIdlePlayed = apProcess->pLastIdlePlayed;
    for (void*& rpItem : apProcess->DeadDetectList.ItemsUntilNull()) {
        DeadDetectList.AddHead(*(&rpItem));
    }
    for (QueuedItem*& rpItem : apProcess->pListItemstoEquipUnequip->ItemsUntilNull()) {
        if (!pListItemstoEquipUnequip) {
            pListItemstoEquipUnequip = new BSSimpleList<QueuedItem*>;
        }
        QueuedItem* pCopy = rpItem;
        QueuedItem* pItem = new QueuedItem;
        pItem->pObject = pCopy->pObject;
        pItem->bEquip = pCopy->bEquip;
        pItem->iNumber = pCopy->iNumber;
        pItem->pExtra = pCopy->pExtra;
        pItem->bNow = pCopy->bNow;
        pItem->bForceEquip = pCopy->bForceEquip;
        pItem->bForceUnequip = pCopy->bForceUnequip;
        pItem->bEquipCalling = pCopy->bEquipCalling;
        pItem->bPlayPickUpSounds = pCopy->bPlayPickUpSounds;
        pItem->spQueuedFiles = pCopy->spQueuedFiles;
        pListItemstoEquipUnequip->AddHead(pItem);
    }
    bBeenAttacked = apProcess->HasBeenAttacked();
    cSitSleepState = apProcess->cSitSleepState;
    pCurrentFurniture = apProcess->pCurrentFurniture;
    if (pCurrentFurniture) {
        pCurrentFurniture->SetTargeted(true);
    }
    cCurrentFurnitureIndex = apProcess->cCurrentFurnitureIndex;
    ourMemcpy(&FurnitureMarker, &apProcess->FurnitureMarker, sizeof(FurnitureMarker));
    ourMemcpy(pDamageRootNode, apProcess->pDamageRootNode, sizeof(pDamageRootNode));
    pHeadNode = apProcess->pHeadNode;
    pTorsoNode = apProcess->pTorsoNode;
    pLightingProperty = apProcess->GetLightingShaderProperty();
    pActiveEffectList->RemoveAll();
    for (BSSimpleList<ActiveEffect*>* plist = apProcess->GetActiveEffectList(); plist && !plist->IsEmpty();
         plist = plist->m_pkNext) {
        ActiveEffect* m_item = plist->m_item;
        ActiveEffect* pItem = m_item->Copy();
        pActiveEffectList->AddHead(pItem);
    }
    pCommandingActor = apProcess->pCommandingActor;
    pLastBoundWeapon = apProcess->pLastBoundWeapon;
    spCharController = apProcess->spCharController;
    bCheckMagicNode = apProcess->bCheckMagicNode;
    pCurrentWeaponEffect = apProcess->pCurrentWeaponEffect;
    fAlphaMult = apProcess->fAlphaMult;
    fScriptRefractPower = apProcess->fScriptRefractPower;
    pBSBound = apProcess->pBSBound;
    bDetectLifeDetected = apProcess->bDetectLifeDetected;
    cUpdate3DModel = apProcess->cUpdate3DModel;
    bForceNextUpdate = apProcess->bForceNextUpdate;
    iPackageIdleNumber = apProcess->iPackageIdleNumber;
    pCurrentIdle = apProcess->pCurrentIdle;
    fPackageIdleTimer = apProcess->fPackageIdleTimer;
    pAttachedArrowList = apProcess->pAttachedArrowList;
    apProcess->pAttachedArrowList = nullptr;
    spWeaponPreload = apProcess->spWeaponPreload;
    spReserveProjectileClones.SetPointer_(&apProcess->spReserveProjectileClones);
    bIronSights = apProcess->bIronSights;
    iAnimActionSuccess = apProcess->iAnimActionSuccess;
    fAimLooking = apProcess->fAimLooking;
    iMovementAnimations = apProcess->iMovementAnimations;
    fRadiationMagicDelta = apProcess->fRadiationMagicDelta;
    fRadiationWaterDelta = apProcess->fRadiationWaterDelta;
    fRadiationDelta = apProcess->fRadiationDelta;
    pFaceNode = apProcess->pFaceNode;
    pFaceNodeSkinned = apProcess->pFaceNodeSkinned;
    pAnimFace = apProcess->pAnimFace;
}

void MiddleHighProcess::ReparentWeapon(bool abDrawn, BipedAnim* apBiped, Animation* apAnim, Actor* apActor) {
    if (apAnim && apAnim->GetControllerManager()) {
        NiNode* pBone = GetWeaponBone(apBiped);
        BSAnimGroupSequence* pSeq = apAnim->GetCurrentSequence(AGS_WEAPON);
        ANIM_GROUP_ENUM eGroup = static_cast<ANIM_GROUP_ENUM>(abDrawn + 23);
        if (!pSeq ||
            (animGroupInfo[pSeq->GetAnimGroup()->GetType()].eAction != AGAT_ATTACK_MINE &&
             animGroupInfo[pSeq->GetAnimGroup()->GetType()].eAction != AGAT_ATTACK_THROW)) {
            if (!pSeq ||
                (pSeq->GetAnimGroup()->GetType() != ANIM_GROUP_EQUIP &&
                 pSeq->GetAnimGroup()->GetType() != ANIM_GROUP_UNEQUIP)) {
                uint16_t usGroup = apActor->GetAnimGroup(eGroup, pWeapon, pWeapon == nullptr, apAnim);
                if (TESAnimGroup::GetType(usGroup) == eGroup) {
                    pSeq = apAnim->PlayGroup(usGroup, ACTION_START, -1, AGS_NONE);
                    if (abDrawn) {
                        SetAnimAction(ANIM_ACTION_EQUIP_WEAPON, pSeq);
                    }
                }
            }
            if (pSeq) {
                NiNode* p3D;
                if (apActor->TESObjectREFR::IsDead(false)) {
                    pSeq->SetOffset(pSeq->m_fEndKeyTime);
                    apAnim->Update(apActor, 0.0f, pSeq->m_fEndKeyTime);
                    apAnim->UpdateMovement(apActor);
                }
                if (apBiped) {
                    p3D = apBiped->root;
                }
                else {
                    p3D = nullptr;
                }
                if (!p3D) {
                    p3D = apActor->Get3D();
                }
                if (p3D && !pBone && pWeapon) {
                    SetCurrentWeapon(pWeapon, p3D, false);
                    pBone = GetWeaponBone(apBiped);
                }

                NiNode* pNewParent = static_cast<NiNode*>(
                    BSUtilities::GetObjectByName(p3D, pSeq->GetAnimGroup()->pParentName)
                );
                if (pNewParent) {
                    pNewParent->AttachChild(pBone, 1);
                    apActor->SetHavokWeapon();
                }
                else {
                    Warning(
                        "ANIMATION: Animation '%s' doesn't have a 'prn: ...' to reparent the weapon bone.",
                        pSeq->m_kName.GetHandleString()
                    );
                }
            }
            else {
                const char* pAction;
                const char* pWeaponName;
                if (pWeapon) {
                    pWeaponName = pWeapon->GetFullName();
                }
                else {
                    pWeaponName = "NONE";
                }
                if (abDrawn) {
                    pAction = "Equip";
                }
                else {
                    pAction = "Holster";
                }
                int32_t iFormID = apActor->iFormID;
                Warning(
                    "AI: Missing %s animation for '%s' (%08X) weapon '%s'.",
                    pAction,
                    apActor->GetFormEditorID(),
                    iFormID,
                    pWeaponName
                );
            }
        }
    }
}

void MiddleHighProcess::ClearFurnitureMarker() {
    if (!pSaveLoadGame->GetSaveGameLoading() && pCurrentFurniture && !pDataHandler->GetClearingData()) {
        pCurrentFurniture->SetMarkerReserved(cCurrentFurnitureIndex, false);
    }

    cCurrentFurnitureIndex = 127;
    FurnitureMarker.SetHeading(0.0);
    FurnitureMarker.SetType(0);
    FurnitureMarker.SetPosition(NiPoint3::ZERO);
}

uint16_t MiddleHighProcess::GetCharControllerSaveSize(MobileObject* pmob) {
    int16_t sStateSize;
    int16_t sVelocitySize;
    int16_t sFallSize;
    int16_t sSize = 0;
    bhkCharacterController* pController = GetCharController();
    if (pController && !pmob->IsDead(false)) {
        sStateSize = 12;
        if ((static_cast<bhkCharacterListener*>(pController)->GetFlag(0xFFFFFFFFu) &
             bhkCharacterController::CHASEBIP) != 0) {
            sStateSize = 24;
        }
        sVelocitySize = static_cast<int16_t>(sStateSize + 44);
        if (pmob->IsActor()) {
            sFallSize = static_cast<int16_t>(sVelocitySize + 8);
        }
        else {
            sFallSize = static_cast<int16_t>(sVelocitySize + 4);
        }
        sSize = static_cast<int16_t>(sFallSize + 4);
        if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x77u) {
            sSize += 8;
        }
    }

    return static_cast<uint16_t>(sSize);
}

bool MiddleHighProcess::ProcessEscortOneHour(Actor* apActor) {
    TESPackage* pPack = GetPackageThatIsRunning();
    if (!pPack) {
        return false;
    }
    ItemChange* pItem = apActor->GetItemBeingEscorted();
    if (!pItem) {
        if (!pTarget || (!pTarget->IsActor() && !apActor->HasObjectId(pTarget->iFormID))) {
            SetTargetForPackage(apActor);
            if (!pTarget || (!pTarget->IsActor() && !apActor->HasObjectId(pTarget->iFormID))) {
                AddToProcedureIndexRunning(apActor, 1);
                if (!apActor->IsPathingComplete()) {
                    EndMoveMessage(apActor);
                }
                return false;
            }
        }
    }
    if (!pItem && this->pAcquireObject && this->pAcquireObject->pRef == apActor) {
        pItem = apActor->GetInventoryItem(this->pAcquireObject->pObject, 0);
    }

    bool bIsActor = false;
    if (pTarget && pTarget->IsActor() && pTarget != apActor) {
        bIsActor = true;
        Actor* pAct = dynamic_cast<Actor*>(pTarget);
        TESPackage* pRunning = pAct->pCurrentProcess->GetCurrentPackage();
        if (pTarget != pPlayer &&
            (!pRunning || (pRunning->GetPackType() != PACKAGE_TYPE_FOLLOW && !pRunning->GetIsCreated()))) {
            SetProcedureIndexRunning(0);
            return false;
        }
    }

    NiPoint3 kDistance = apActor->GetLocationOnReference();
    if (bIsActor) {
        const NiPoint3* pTargetPos = &pTarget->GetLocationOnReference();
        const NiPoint3* pActorPos = &apActor->GetLocationOnReference();
        kDistance = *pActorPos - *pTargetPos;
    }
    float fWaitDistance = 0.0f;
    if (bIsActor) {
        fWaitDistance = 0.0f;
        int32_t iPackDist = pPack->GetPackageTarget()->iValue;
        if (iPackDist <= 0) {
            iPackDist = 200;
        }
        if (pTarget == pPlayer && fWaitDistance == 0.0) {
            fWaitDistance = static_cast<float>(iPackDist);
        }
        else if (fWaitDistance == 0.0 && apActor->GetParentCell()->GetInterior()) {
            fWaitDistance = fAIEscortWaitDistanceInterior.Float();
        }
        else if (fWaitDistance == 0.0) {
            fWaitDistance = fAIEscortWaitDistanceExterior.Float() * static_cast<double>(iPackDist);
        }
    }
    if (pPack->pPackLoc && pPack->IsActorAtLocation(apActor, false, -1.0f, false)) {
        NiPoint3 kTargDistance = NiPoint3::ZERO;
        NiPoint3 kMyDistance = NiPoint3::ZERO;
        if (!bIsActor) {
            PackageLocation* pPackLoc;
            pPackLoc = pPack->pPackLoc;
            PackageTarget* pPackTarget;
            pPackTarget = pPack->GetPackageTarget();
            if (pItem) {
                ExtraDataList* pExtraList = nullptr;
                if (pItem->pExtraObjectList) {
                    pExtraList = pItem->pExtraObjectList->m_item;
                }
                uint32_t uiID = 0;
                if (pExtraList && pExtraList->GetReferencePointer()) {
                    uiID = pExtraList->GetReferencePointer()->iFormID;
                }

                TESObjectREFR* pContainer = pPack->pPackLoc->GetLocReference();
                if (!pContainer) {
                    pContainer = pGenericLocation;
                }
                if (pContainer && pContainer->HasContainer()) {
                    int32_t iCount = pPack->GetInitialTargetCount();
                    apActor->PutItemInReference(pItem->pContainerObj, pExtraList, pContainer, iCount, uiID);
                }
                else {
                    if (pItem->pExtraObjectList) {
                        pExtraList = pItem->pExtraObjectList->m_item;
                    }
                    const NiPoint3* pPoint = nullptr;
                    if (pPackLoc) {
                        TESObjectREFR* pRef = pPackLoc->GetLocReference();
                        if (!pRef) {
                            pRef = pGenericLocation;
                        }
                        if (pRef) {
                            TESBoundObject* pBaseObject = pRef->GetObjectReference();
                            if (pBaseObject == pXMarker || pRef->GetObjectReference() == pXMarkerHeading) {
                                pPoint = &pRef->GetLocationOnReference();
                            }
                        }
                    }
                    int32_t iNumber = 1;
                    if (!pPackTarget->GetTargReference()) {
                        iNumber = pPack->GetInitialTargetCount();
                    }
                    apActor->DropObject(pItem->pContainerObj, pExtraList, iNumber, pPoint, nullptr);
                }
                if (pItem) {
                    delete pItem;
                }
                if (pPackTarget->GetTargObjectType() == POTYPE_WEAP ||
                    (pPackTarget->GetTargObjectType() >= POTYPE_COMBATWEAR &&
                     pPackTarget->GetTargObjectType() <= POTYPE_WEAP_RANGED)) {
                    while (!ObjectList.IsEmpty()) {
                        this->pAcquireObject = ObjectList.m_item;
                        ObjectList.Remove(this->pAcquireObject);
                        SetTarget(this->pAcquireObject->pRef);
                        pItem = apActor->GetInventoryItem(this->pAcquireObject->pObject, 0);
                        ObjectstoAcquire* pAcquireObject = this->pAcquireObject;
                        delete pAcquireObject;
                        this->pAcquireObject = nullptr;
                        if (pContainer && pContainer->HasContainer()) {
                            int32_t iCount = pPack->GetInitialTargetCount();
                            apActor->PutItemInReference(pItem->pContainerObj, pExtraList, pContainer, iCount, uiID);
                        }
                        else {
                            if (pItem->pExtraObjectList) {
                                pExtraList = pItem->pExtraObjectList->m_item;
                            }
                            const NiPoint3* pPoint = nullptr;
                            if (pPackLoc) {
                                TESObjectREFR* pRef = pPackLoc->GetLocReference();
                                if (!pRef) {
                                    pRef = pGenericLocation;
                                }
                                if (pRef) {
                                    TESBoundObject* pBaseObject = pRef->GetObjectReference();
                                    if (pBaseObject == pXMarker || pRef->GetObjectReference() == pXMarkerHeading) {
                                        pPoint = &pRef->GetLocationOnReference();
                                    }
                                }
                            }
                            int32_t iNumber = 1;
                            if (!pPackTarget->GetTargReference()) {
                                iNumber = pPack->GetInitialTargetCount();
                            }
                            apActor->DropObject(pItem->pContainerObj, pExtraList, iNumber, pPoint, nullptr);
                        }
                        if (pItem) {
                            delete pItem;
                        }
                        pItem = nullptr;
                    }
                    bool bWearArmor = true;
                    bool bWearWeapon = true;
                    if (GetCurrentPackage()) {
                        if (GetCurrentPackage()->GetForceUnequipWeapon()) {
                            bWearWeapon = false;
                        }
                    }
                    if (apActor->IsNpc()) {
                        TESNPC* pNPC = dynamic_cast<TESNPC*>(apActor->GetObjectReference());
                        if (pNPC) {
                            pNPC->InitDefaultWorn(apActor, bWearArmor, bWearWeapon, false, true);
                        }
                    }
                    else {
                        TESCreature* pCreature = dynamic_cast<TESCreature*>(apActor->GetObjectReference());
                        if (pCreature) {
                            pCreature->InitDefaultWorn(apActor, bWearArmor, bWearWeapon, true);
                        }
                    }
                }
                SetCurrentActionComplete(true);
            }
            AddToProcedureIndexRunning(apActor, 1);
        }
        else {
            double dDistance = kDistance.Length();
            if (fWaitDistance >= dDistance) {
                double dMyDistance = kMyDistance.Length();
                if (kTargDistance.Length() >= dMyDistance) {
                    AddToProcedureIndexRunning(apActor, 1);
                    Actor* pTargetActor = static_cast<Actor*>(pTarget);
                    if (pTargetActor->pCurrentProcess->GetPackageThatIsRunning()) {
                        pTargetActor->pCurrentProcess->AddToProcedureIndexRunning(apActor, 1);
                        if (pTargetActor->pCurrentProcess->GetPackageThatIsRunning()->GetIsCreated()) {
                            pTargetActor->pCurrentProcess->ClearCurrentPackage();
                            if (pTargetActor->IsPathingComplete()) {
                                pTargetActor->EndMovement();
                            }
                            SetCurrentActionComplete(true);
                            CheckforNewPackage(apActor, false);
                            return false;
                        }
                    }
                }
            }
        }
        if (!apActor->IsPathingComplete()) {
            EndMoveMessage(apActor);
        }
        return false;
    }

    GetCurrentPackage()->GetPackageRadiusActorToRefTarget(apActor, false);
    if (pTarget && pTarget->IsActor()) {
        apActor->GetDistanceFromReference(pTarget, false, false);
    }

    return false;
}

bool MiddleHighProcess::ProcessEatOneHour(Actor* apActor) {
    ItemChange* pFood = apActor->GetBestFoodItem();
    if (pFood) {
        ExtraDataList* pExtraList = nullptr;
        if (pFood->GetExtraDataList()) {
            pExtraList = pFood->GetExtraDataList()->m_item;
        }
        apActor->Eat(pFood->pContainerObj, pExtraList, true);
        AddToProcedureIndexRunning(apActor, 1);
    }
    return false;
}

bool MiddleHighProcess::ProcessActivateOneHour(Actor* apActor) {
    TESPackage* pPack = GetPackageThatIsRunning();
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    if (!pTarget || pTarget->GetDelete()) {
        AddToProcedureIndexRunning(apActor, 1);
        return false;
    }
    if (pPack->GetPackType() == PACKAGE_TYPE_AMBUSH && !pPack->IsTargetAtLocation(apActor, 300)) {
        AddToProcedureIndexRunning(apActor, -1);
    }
    DoorTeleportData* pTel = pTarget->GetTeleport();
    bool bAtRef = false;
    if (pTel) {
        NiPoint3 kDist;
        const NiPoint3* pActorPos = &apActor->GetLocationOnReference();
        const NiPoint3* pDoorPos = &pTarget->GetLinkedDoorTeleportPosition();
        kDist = *pDoorPos - *pActorPos;
        if (kDist.Length() <= 128.0) {
            bAtRef = true;
        }
    }
    else {
        bAtRef = GetCurrentPackage()->IsActorAtRefTarget(apActor, 0);
    }
    if (!bAtRef) {
        if (apActor->GetSitSleepState()) {
            ProcessGetUpOneHour(apActor);
        }
        // The destination is never used, but the teleport lookup caches the extra data and an actor's location query
        // records its swim history.
        if (pTarget->GetTeleport()) {
            pTarget->GetLinkedDoorTeleportPosition();
        }
        else {
            pTarget->GetLocationOnReference();
        }
        return false;
    }
    if (pTarget == pPlayer && pPlayer->IsSleepingorResting() && !pPlayer->btransporting) {
        pPlayer->SetSleeping(0, true);
        AddToProcedureIndexRunning(apActor, -2);
        return false;
    }

    if (pTarget->IsActor()) {
        if (pTarget == pPlayer && pPlayer->IsSleepingorResting() && !pPlayer->btransporting) {
            pPlayer->SetSleeping(0, true);
            AddToProcedureIndexRunning(apActor, -1);
            return false;
        }

        SetTargetActivated(true);
        AddToProcedureIndexRunning(apActor, 1);
        return true;
    }

    SetTargetActivated(true);
    pTarget->Activate(apActor, false, nullptr, 1);
    AddToProcedureIndexRunning(apActor, 1);
    return true;
}

void MiddleHighProcess::AlignToFurnitureAndAddPostAnimSitSleep(Actor* apActor) {
    bool bAligned;
    if (apActor->CanMove()) {
        bAligned = !apActor->RequestRotate(FurnitureMarker.GetHeading(), false);
    }
    else {
        apActor->SetHeading(FurnitureMarker.GetHeading());
        bAligned = true;
    }
    if (bAligned) {
        AddPostAnimationAction(POSTANIM_ACTION_SIT_SLEEP);
    }
}

bool MiddleHighProcess::ShouldPickPackIdle(Actor* apActor) {
    if (GetPackageIdleDone()) {
        return false;
    }
    TESPackage* pPackage = GetPackageThatIsRunning();
    bool bPick = false;
    uint32_t uiProcedureType = pPackage->ePROCEDURE_TYPE;
    PROCEDURE eProcedure = Procedure::Action[uiProcedureType][GetProcedureIndexRunning()];
    if (eProcedure == PROCEDURE_SANDBOX) {
        SandBoxActorPackageData* pData = static_cast<SandBoxActorPackageData*>(GetInstanceDataThatIsRunning());
        if (pData) {
            eProcedure = pData->GetProcedure();
        }
    }
    switch (eProcedure) {
        case PROCEDURE_TRAVEL:
        case PROCEDURE_WANDER:
        case PROCEDURE_SLEEP:
        case PROCEDURE_EAT:
        case PROCEDURE_FOLLOW:
        case PROCEDURE_ESCORT:
        case PROCEDURE_ACCOMPANY:
        case PROCEDURE_USE_ITEM_AT:
        case PROCEDURE_AMBUSH_WAIT:
        case PROCEDURE_FLEE_NON_COMBAT:
        case PROCEDURE_PATROL:
        case PROCEDURE_GUARD:
        case PROCEDURE_DIALOGUE_ACTIVATE:
        case PROCEDURE_DONE: {
            if (pPackage->ePROCEDURE_TYPE) {
                if (apActor->IsPathingComplete()) {
                    bPick = true;
                }
            }
            else if (pPackage->IsActorAtLocation(apActor, false, -1.0f, false)) {
                bPick = true;
            }
            break;
        }
        default: {
            break;
        }
    }
    return bPick;
}

// The running package and the package extra share one stack slot: a package that is not in-game dialogue is used
// itself.
void MiddleHighProcess::PickPackageIdles(Actor* apActor, bool abForceIdle) {
    using kArray_t = BSSimpleArray<TESIdleForm*, 1024>;
    TESPackage* pPackage = GetPackageThatIsRunning();
    if (pPackage && pPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        pPackage = apActor->m_Extra.GetPackageExtra();
    }

    Animation* pAnimData = apActor->GetAnimation();
    if (pPackage) {
        if (bPickPackIdle) {
            abForceIdle = true;
            fPackageIdleTimer = 0.0f;
        }
        if (ShouldPickPackIdle(apActor)) {
            BGSIdleCollection* pIdleCollection = pPackage->pIdleCollection;
            if (pIdleCollection &&
                pAnimData &&
                (pAnimData->SpecialIdleDonePlaying() || abForceIdle) &&
                !apActor->IsParalyzed()) {
                if (!(fPackageIdleTimer <= 0.0)) {
                    fPackageIdleTimer = fPackageIdleTimer - appTimer.fDelta;
                }
                else {
                    bPickPackIdle = false;
                    TESIdleForm* pLastIdle = pCurrentIdle;
                    pCurrentIdle = nullptr;
                    BGSIdleCollection::PICK_TYPE ePick = pIdleCollection->GetPickType();
                    kArray_t kArray(10u);
                    uint32_t uiIdleCount = pIdleCollection->GetIdlesForActor(apActor, pTarget, &kArray);
                    if (uiIdleCount) {
                        if (ePick) {
                            if (ePick == BGSIdleCollection::SEQUENCE) {
                                if (iPackageIdleNumber == -1 || iPackageIdleNumber + 1 >= uiIdleCount) {
                                    iPackageIdleNumber = 0;
                                }
                                else {
                                    ++iPackageIdleNumber;
                                }
                                pCurrentIdle = kArray.GetAt(static_cast<int32_t>(iPackageIdleNumber));
                                if (pPackage->GetOnBeginIdle() == pCurrentIdle) {
                                    pCurrentIdle = nullptr;
                                }
                                if (iPackageIdleNumber + 1 >= uiIdleCount && pIdleCollection->GetDoOnce()) {
                                    SetPackageIdleDone(true);
                                }
                                fPackageIdleTimer = -1.0f;
                            }
                        }
                        else {
                            pCurrentIdle = pLastIdle;
                            if (pLastIdle) {
                                while (pLastIdle == pCurrentIdle && uiIdleCount > 1) {
                                    uint32_t uiIndex = static_cast<uint32_t>(rt::rand()) % uiIdleCount;
                                    pCurrentIdle = kArray.GetAt(static_cast<int32_t>(uiIndex));
                                    iPackageIdleNumber = uiIndex;
                                }
                            }
                            else {
                                uint32_t uiIndex = static_cast<uint32_t>(rt::rand()) % uiIdleCount;
                                pCurrentIdle = kArray.GetAt(static_cast<int32_t>(uiIndex));
                                iPackageIdleNumber = uiIndex;
                            }
                            if (pPackage->GetOnBeginIdle() == pCurrentIdle) {
                                pCurrentIdle = nullptr;
                            }
                            if (pCurrentIdle) {
                                fPackageIdleTimer = 0.8f;
                                if (pIdleCollection->GetDoOnce()) {
                                    SetPackageIdleDone(true);
                                }
                            }
                        }
                        if (pCurrentIdle) {
                            SetupSpecialIdle(apActor, pCurrentIdle, AnimIdle::PLAY_TYPE_AUTO, true, false, true);
                        }
                    }
                }
            }
        }
    }
}

// One-hour step of the follow procedure: a waiting follow package resumes once there is room among the player's
// followers; the actor repaths when the target has moved far enough or is out of range, and the procedure advances
// once a follow package with a location has the actor there. afDelta is unused.
bool MiddleHighProcess::ProcessFollowOneHour(Actor* apActor, float afDelta, bool abInterateIndex) {
    TESPackage* pPackage = GetPackageThatIsRunning();
    if (pPackage && pPackage->GetWaiting()) {
        if (pPlayer->IsPlayerFollower(apActor)) {
            return false;
        }
        int32_t iFollowers = pPlayer->GetNumberOfFollowers();
        if (iFollowers >= static_cast<int32_t>(iNumberActorsAllowedToFollowPlayer.UInt())) {
            return false;
        }
        pPackage->SetWaiting(false);
    }
    if (!pTarget || pTarget->GetDelete()) {
        SetTargetForPackage(apActor);
        if (pTarget) {
            lastSeenPostion = pTarget->GetLocationOnReference();
        }
    }
    if (!pTarget) {
        if (abInterateIndex) {
            AddToProcedureIndexRunning(apActor, 1);
        }
        return false;
    }
    float fDistance = apActor->GetDistanceFromReference(pTarget, false, false);
    apActor->GetWorldSpace();
    pTarget->GetWorldSpace();
    float fRadius = static_cast<float>(pPackage->GetPackageRadiusActorToRefTarget(apActor, false));
    TESObjectREFR* pFollowTarget = pTarget;
    if (!pTarget ||
        (pPackage->GetPackType() == PACKAGE_TYPE_FOLLOW &&
         pPackage->GetLocation() &&
         static_cast<int32_t>(pPackage->GetLocation()->GetLocType()) < PLT_CURRLOC &&
         pPackage->IsActorAtLocation(apActor, false, -1.0f, false))) {
        if (abInterateIndex) {
            AddToProcedureIndexRunning(apActor, 1);
            if (pPackage->GetIsCreated()) {
                if (GetRunOncePackage()) {
                    ClearRunOncePackage();
                }
                else {
                    ClearCurrentPackage();
                }
                CheckforNewPackage(apActor, false);
                return false;
            }
            if (!apActor->IsPathingComplete()) {
                EndMoveMessage(apActor);
            }
        }
    }
    else {
        float fMoved = (lastSeenPostion - pTarget->GetLocationOnReference()).Length();
        if (fAIMoveDistanceToRecalcFollowPath.Float() < fMoved ||
            (fRadius < fDistance && apActor->IsPathingComplete())) {
            TESWorldSpace* pWorld = pTarget->GetWorldSpace();
            TESObjectCELL* pCell = pTarget->GetParentCell();
            if (!SetupPathFinding(apActor, pTarget->GetLocationOnReference(), pCell, pWorld)) {
                return true;
            }
            lastSeenPostion = pTarget->GetLocationOnReference();
        }
    }
    fDistance = apActor->GetDistanceFromReference(pFollowTarget, false, false);
    if (fRadius > fDistance && !apActor->IsPathingComplete()) {
        apActor->EndMovement();
    }
    return false;
}

uint8_t MiddleHighProcess::DidPathingFail(Actor* apActor) {
    return apActor->DidPathingFail();
}

TESObjectREFR* MiddleHighProcess::SetTargetData(Actor* apActor, TESPackage* apPackage, PackageTarget* ptarg) {
    if (!apPackage || !ptarg) {
        return nullptr;
    }
    TESObjectREFR* pLocationRef = nullptr;
    if (apPackage->GetPackType() != PACKAGE_TYPE_ESCORT && apPackage->pPackLoc) {
        pLocationRef = apPackage->pPackLoc->GetLocReference();
    }
    else {
        pLocationRef = apActor;
    }
    if (ptarg->GetTargType() == PTT_REFERENCE) {
        TESObjectREFR* pRef = ptarg->GetTargReference();
        if (!pRef || !pRef->IsDead(true) || apPackage->GetIsCreated()) {
            return pRef;
        }
        apPackage->SetNeverRun(pRef, true);
        return nullptr;
    }
    if (ptarg->GetTargType() != PTT_LINKEDREF) {
        bool bRebuiltList = false;
        while (true) {
            if (ObjectList.IsEmpty()) {
                bRebuiltList = true;
                bool bEvalFlag = true;
                TESObjectCELL* pCell = apActor->GetParentCell();
                NiPoint3 kPointB = apActor->GetLocationOnReference();
                PACK_TARG_TYPE eType = ptarg->GetTargType();
                if (eType == PTT_OBJECT) {
                    TESForm* pForm = ptarg->GetTargObject();
                    if (pForm->GetFormType() == FLST_ID) {
                        if (pTargetFormList) {
                            ++sFormListNumb;
                        }
                        else {
                            pTargetFormList = static_cast<BGSListForm*>(pForm);
                            sFormListNumb = 0;
                        }
                        BSSimpleList<TESForm*>* pList = &pTargetFormList->ListOfForms;
                        if (!pList->IsEmpty()) {
                            for (int32_t iNum = 0; pList && !pList->IsEmpty() && sFormListNumb <= iNum; ++iNum) {
                                pList = pList->m_pkNext;
                            }
                        }
                        if (pForm) {
                            pObjecttoAcquire = static_cast<TESBoundObject*>(pForm);
                        }
                        eFormType = POTYPE_NONE;
                    }
                    else if (pForm->GetFormType() == FACT_ID) {
                        pFactiontoAquire = static_cast<TESFaction*>(pForm);
                        eFormType = POTYPE_ACTOR;
                    }
                    else if (ptarg->GetTargObject()) {
                        if (ptarg->GetTargObject()->IsBoundObject()) {
                            pObjecttoAcquire = static_cast<TESBoundObject*>(ptarg->GetTargObject());
                            eFormType = POTYPE_NONE;
                        }
                    }
                }
                else if (eType == PTT_TYPE) {
                    pObjecttoAcquire = nullptr;
                    eFormType = ptarg->GetTargObjectType();
                    eUseItem = eFormType;
                }
                if (!pObjecttoAcquire) {
                    switch (eFormType) {
                        case POTYPE_NONE: {
                            MagicItem* pSpell = dynamic_cast<MagicItem*>(ptarg->GetTargObject());
                            if (pSpell) {
                                pCurrentPackageSpell = pSpell;
                            }
                            bEvalFlag = false;
                            break;
                        }
                        case POTYPE_WEAP:
                        case POTYPE_COMBATWEAR:
                        case POTYPE_WEAR:
                        case POTYPE_WEAP_MELEE:
                        case POTYPE_WEAP_RANGED: {
                            if (!apActor->IsInCombat()) {
                                BuildMulitTargetObjectList(apActor, pLocationRef, eFormType);
                                eUseItem = eFormType;
                                bEvalFlag = false;
                            }
                            break;
                        }
                        case POTYPE_SPELL:
                        case POTYPE_SPELL_RANGED:
                        case POTYPE_SPELL_TOUCH:
                        case POTYPE_SPELL_SELF: {
                            eUseItem = eFormType;
                            bEvalFlag = false;
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
                if (bEvalFlag) {
                    float fAcquireDist = 0.0f;
                    if (apPackage->GetPackageSearchLocation()) {
                        uint32_t uiRadius = apPackage->GetSearchLocationRadius(apActor);
                        TESObjectCELL* pSearchCell = apPackage->GetSearchLocationCell(apActor);
                        pCell = pSearchCell ? pSearchCell : pCell;
                        kPointB = apPackage->GetSearchLocationCoord(apActor);
                        if (uiRadius) {
                            fAcquireDist = static_cast<float>(uiRadius);
                        }
                        else if (pCell && pCell->GetInterior()) {
                            fAcquireDist = std::numeric_limits<float>::max();
                        }
                        else {
                            fAcquireDist = fAIAcquireObjectDistance.Float();
                        }
                        pDataHandler->EnumReferencesCloseToPoint(
                            pCell,
                            kPointB,
                            fAcquireDist,
                            kPointB,
                            fAcquireDist,
                            &LowProcess::EvaluateReferenceforAcquire,
                            apActor
                        );
                    }
                    else {
                        if (!GetCurrentPackage() || !GetCurrentPackage()->GetAcquireRadius(apActor, fAcquireDist)) {
                            fAcquireDist = fAIAcquireObjectDistance.Float();
                        }
                        const NiPoint3* pActorPos = &apActor->GetLocationOnReference();
                        pDataHandler->EnumReferencesCloseToPoint(
                            pCell,
                            kPointB,
                            fAcquireDist,
                            *pActorPos,
                            fAcquireDist,
                            &LowProcess::EvaluateReferenceforAcquire,
                            apActor
                        );
                    }
                    eFormType = POTYPE_NONE;
                    pObjecttoAcquire = nullptr;
                    pFactiontoAquire = nullptr;
                    EvaluateOrderAcquireList(apActor);
                }
                iNumberItemsActivate = apPackage->GetInitialTargetCount();
            }
            bool bReserveRef = apActor != pPlayer && apPackage->GetShouldReserveTarget();
            while (!ObjectList.IsEmpty()) {
                TESNPC* pNPC;
                TESForm* pOwnerForm;
                pAcquireObject = ObjectList.m_item;
                ObjectList.Remove(pAcquireObject);
                TESObjectREFR* pAcquireRef = pAcquireObject->pRef;
                if (pAcquireObject->eType == ACQUIRE_TYPE_BUY_NPC && !pAcquireRef->IsActor()) {
                    pOwnerForm = pAcquireObject->pRef->GetOwner();
                    pNPC = nullptr;
                    if (pOwnerForm && pOwnerForm->GetFormType() == NPC__ID) {
                        pNPC = static_cast<TESNPC*>(pOwnerForm);
                    }

                    if (!pNPC) {
                        continue;
                    }
                    pAcquireRef = ReferenceProcessLists.GetActorRefInHigh(pNPC, false);
                }
                if (!bReserveRef) {
                    return pAcquireRef;
                }
                if (!pAcquireRef) {
                    return pAcquireRef;
                }
                if (GetCurrentFurniture() == pAcquireRef) {
                    return pAcquireRef;
                }
                if (!bReserveRef) {
                    return pAcquireRef;
                }
                if (!pAcquireRef) {
                    return pAcquireRef;
                }
                if (pTarget == pAcquireRef) {
                    return pAcquireRef;
                }
                int32_t iFreeMarker = pAcquireRef->GetFirstFreeMarkerIndex(false);
                if (iFreeMarker >= 0) {
                    return pAcquireRef;
                }
            }
            if (pTarget || bRebuiltList) {
                return nullptr;
            }
        }
    }
    TESObjectREFR* pLinkedRef = apActor->GetLinkedRef();
    if (!pLinkedRef || !pLinkedRef->IsDead(true) || apPackage->GetIsCreated()) {
        return pLinkedRef;
    }
    apPackage->SetNeverRun(pLinkedRef, true);
    return nullptr;
}

void MiddleHighProcess::FindBedChairs(Actor* apActor, bool abSit, TESObjectREFR* apRefLoc) {
    HighProcess::bWantsToSit = abSit;
    TESObjectCELL* pCell = apActor->GetParentCell();
    NiPoint3 kPointB = apActor->GetLocationOnReference();
    TESPackage* pPack = GetPackageThatIsRunning();
    if (pPack && apActor) {
        PackageLocation* pLocation = pPack->pPackLoc;
        float fAcquireDist = 0.0f;
        if (pPack->GetPackType() == PACKAGE_TYPE_EAT) {
            if (pLocation && pLocation->GetLocType() == PLT_CELL && pLocation->GetLocCell()->GetInterior()) {
                fAcquireDist = std::numeric_limits<float>::max();
            }
            else {
                fAcquireDist = static_cast<float>(pPack->GetLocationRadius(apActor));
            }
        }
        else if (!pPack->GetAcquireRadius(apActor, fAcquireDist)) {
            if (pLocation && pLocation->GetLocType() == PLT_CELL && pLocation->GetLocCell()->GetInterior()) {
                fAcquireDist = std::numeric_limits<float>::max();
            }
            else {
                fAcquireDist = fAIFindBedChairsDistance.Float();
            }
        }
        if (apRefLoc) {
            const NiPoint3* pLoc = &apRefLoc->GetLocationOnReference();
            TESObjectCELL* pRefCell = apRefLoc->GetParentCell();
            pDataHandler->EnumReferencesCloseToPoint(
                pRefCell,
                *pLoc,
                fAcquireDist,
                *pLoc,
                fAcquireDist,
                &MiddleHighProcess::EvaluateReferenceforSitSleep,
                apActor
            );
        }
        else {
            pDataHandler->EnumReferencesCloseToPoint(
                pCell,
                kPointB,
                fAcquireDist,
                pPack->GetLocationCoord(apActor),
                fAcquireDist,
                &MiddleHighProcess::EvaluateReferenceforSitSleep,
                apActor
            );
        }
        RefListChairBed.RemoveAll();
        for (TESObjectREFR*& rpItem : MiddleHighTempRefListChairBed.ItemsUntilNull()) {
            RefListChairBed.AddHead(*reinterpret_cast<void**>(&rpItem));
        }
        MiddleHighTempRefListChairBed.RemoveAll();
        BSSimpleList<void*>* pFirst = &RefListChairBed;
        int32_t iCount = pFirst->ItemsInList();
        bool bSwappedItems = true;
        while (iCount && bSwappedItems) {
            bSwappedItems = false;
            for (BSSimpleList<void*>* pSecond = pFirst; pSecond; pSecond = pSecond->m_pkNext) {
                void* pFirstData = pFirst->m_item;
                void* pSecondData = pSecond->m_item;
                if (!static_cast<TESObjectREFR*>(pFirstData)->GetOwner() &&
                    static_cast<TESObjectREFR*>(pSecondData)->GetOwner()) {
                    pFirst->SetItem(pSecondData);
                    pSecond->SetItem(pFirstData);
                    bSwappedItems = true;
                }
            }
            pFirst = pFirst->m_pkNext;
            --iCount;
        }
        nFurnitureListTimer = appTimer.iLastTime;
    }
}

bool MiddleHighProcess::EvaluateReferenceforSitSleep(TESObjectREFR* apRef, void* aiFilterData) {
    Actor* pActor = static_cast<Actor*>(aiFilterData);
    if (!aiFilterData) {
        return false;
    }
    if (!apRef ||
        apRef->GetDelete() ||
        apRef->GetTemporary() ||
        apRef->GetDisabled() ||
        (pActor->IsChild(false) && !pActor->CanChildUseObject(apRef))) {
        return false;
    }
    if (MiddleHighTempRefListChairBed.IsInList(apRef)) {
        return false;
    }
    if (!apRef->IsFurniture() || !apRef->HasFreeMarker(false)) {
        return false;
    }
    if (apRef->GetOwner() && !apRef->IsAnOwner(pActor, true)) {
        return false;
    }
    TESForm* pRefSpace = apRef->GetSpace();
    if (pRefSpace != pActor->GetSpace()) {
        return false;
    }
    TESFurniture* pFurniture = static_cast<TESFurniture*>(apRef->GetObjectReference());
    if ((HighProcess::bWantsToSit && pFurniture->CanSitOn()) ||
        (!HighProcess::bWantsToSit && pFurniture->CanSleepOn())) {
        MiddleHighTempRefListChairBed.AddHead(apRef);
    }
    return false;
}

void bhkCharacterController::SetSupportNormal(const hkVector4& arNormal) {
    hkSupportNorm = arNormal;
}

bool MagicSystem::CheckFlareFlag(FlareType aeFlareType, uint32_t aiFlareFlags) {
    return (aiFlareFlags & aeFlareType) != 0;
}

NiFixedString& FixedStrings::QBip01Neck() {
    return *FixedStrings::pBip01_Neck;
}

NiFixedString& FixedStrings::QBip01Spine1() {
    return *FixedStrings::Bip01_Spine1;
}

QueuedItem::QueuedItem()
  : pObject(nullptr), pExtra(nullptr), iNumber(0), bEquip(false), bNow(false), bForceEquip(false),
    bPlayPickUpSounds(false), bForceUnequip(false), bEquipCalling(false), spQueuedFiles(nullptr) {}

QueuedItem::~QueuedItem() {}

void QueuedItem::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    int32_t iExtraIndex;
    Actor* pActor = apSaveGameBuffer->GetActor();
    apSaveGameBuffer->SaveFormID(pObject, 0);
    iExtraIndex = -1;
    if (pExtra) {
        InventoryChanges* pInventory;
        if (pActor) {
            pInventory = pActor->m_Extra.GetContainerChanges();
        }
        else {
            pInventory = nullptr;
        }
        if (pInventory) {
            iExtraIndex = pInventory->GetExtraDataListIndexForItem(pObject, pExtra);
        }
    }
    apSaveGameBuffer->SaveDataEndian(&iExtraIndex, sizeof(iExtraIndex), 0);
    apSaveGameBuffer->SaveDataEndian(&iNumber, sizeof(iNumber), 0);
    apSaveGameBuffer->SaveDataEndian(&bEquip, sizeof(bEquip), 0);
    apSaveGameBuffer->SaveDataEndian(&bNow, sizeof(bNow), 0);
    apSaveGameBuffer->SaveDataEndian(&bForceEquip, sizeof(bForceEquip), 0);
    apSaveGameBuffer->SaveDataEndian(&bPlayPickUpSounds, sizeof(bPlayPickUpSounds), 0);
    apSaveGameBuffer->SaveDataEndian(&bForceUnequip, sizeof(bForceUnequip), 0);
    apSaveGameBuffer->SaveDataEndian(&bEquipCalling, sizeof(bEquipCalling), 0);
}

void QueuedItem::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    int32_t iExtraIndex;
    Actor* pActor = apLoadGameBuffer->GetActor();
    TESForm* pForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
    pObject = dynamic_cast<TESBoundObject*>(pForm);
    iExtraIndex = -1;
    apLoadGameBuffer->LoadData(&iExtraIndex, sizeof(iExtraIndex));
    if (iExtraIndex != -1) {
        InventoryChanges* pInventory;
        if (pActor) {
            pInventory = pActor->m_Extra.GetContainerChanges();
        }
        else {
            pInventory = nullptr;
        }
        if (pInventory) {
            pExtra = pInventory->GetExtraDataListFromIndexForItem(pObject, iExtraIndex);
        }
    }
    apLoadGameBuffer->LoadData(&iNumber, sizeof(iNumber));
    apLoadGameBuffer->LoadData(&bEquip, sizeof(bEquip));
    apLoadGameBuffer->LoadData(&bNow, sizeof(bNow));
    apLoadGameBuffer->LoadData(&bForceEquip, sizeof(bForceEquip));
    apLoadGameBuffer->LoadData(&bPlayPickUpSounds, sizeof(bPlayPickUpSounds));
    apLoadGameBuffer->LoadData(&bForceUnequip, sizeof(bForceUnequip));
    apLoadGameBuffer->LoadData(&bEquipCalling, sizeof(bEquipCalling));
}

bool QueuedItem::QueueFiles(Actor* apActor) {
    if (!apActor || !pObject) {
        return false;
    }
    if (!bEquip) {
        if (apActor->IsCreature() && pObject->GetFormType() == WEAP_ID) {
            Animation* pAnim = apActor->pCurrentProcess->GetAnimation();
            if (!pAnim ||
                !pAnim->GetControllerManager() ||
                !pAnim->GroupLoaded(TESAnimGroup::AnimGroup(AGM_NONE, AGW_HAND_TO_HAND, ANIM_GROUP_EQUIP, false))) {
                QueuedFile* pFile = new QueuedFile(IO_TASK_PRIORITY_VERY_HIGH);
                spQueuedFiles = pFile;
                const char* pModel = apActor->GetModel();
                BSSimpleList<const char*>* pFileList = pModelLoader
                                                           ->BuildKFFileList(pModel, true, false, AGW_HAND_TO_HAND);
                pModelLoader->QueueKFList(pFileList, IO_TASK_PRIORITY_VERY_HIGH, spQueuedFiles.m_pObject, nullptr);
                delete pFileList;
            }
        }
        return false;
    }
    if (pObject->GetFormType() != WEAP_ID && pObject->GetFormType() != ARMO_ID) {
        return false;
    }
    QueuedFile* pFile = new QueuedFile(IO_TASK_PRIORITY_VERY_HIGH);
    spQueuedFiles = pFile;
    pModelLoader->QueueBoundObject(pObject, IO_TASK_PRIORITY_VERY_HIGH, spQueuedFiles.m_pObject, apActor);
    if (apActor->IsCreature() && pObject->GetFormType() == WEAP_ID) {
        TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(pObject);
        Animation* pAnim = apActor->pCurrentProcess->GetAnimation();
        if (!pAnim ||
            !pAnim->GetControllerManager() ||
            !pAnim->GroupLoaded(
                TESAnimGroup::AnimGroup(
                    AGM_NONE,
                    weaponToAnimGroupWeapon[pWeapon->GetWeaponType()],
                    ANIM_GROUP_EQUIP,
                    false
                )
            )) {
            ANIM_GROUP_WEAPON_ENUM eAnimWeapon = weaponToAnimGroupWeapon[pWeapon->GetWeaponType()];
            const char* pModel = apActor->GetModel();
            BSSimpleList<const char*>* pFileList = pModelLoader->BuildKFFileList(pModel, true, false, eAnimWeapon);
            pModelLoader->QueueKFList(pFileList, IO_TASK_PRIORITY_VERY_HIGH, spQueuedFiles.m_pObject, nullptr);
            delete pFileList;
        }
    }
    return true;
}

void QueuedItem::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    QueueFiles(apLoadGameBuffer->GetActor());
}

void QueuedItem::operator delete(void* apMemory) {
    ourFree(apMemory);
}

bool SandBoxActorPackageData::IsTargetNeededForProcedure(SANDBOX_PROCEDURE aeProcedure) {
    return aeProcedure < SANDBOX_PROCEDURE_EAT || aeProcedure > SANDBOX_PROCEDURE_WANDER;
}
