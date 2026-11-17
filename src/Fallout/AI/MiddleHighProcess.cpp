#include "Fallout/AI/MiddleHighProcess.h"

#include "BSAudio/BSAudio.h"
#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
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
#include "Reconstruction.hpp"
#include "SDK/MSVC/rtti.h"

#include <cmath>
#include <stdint.h>

// The game settings this file defines, with their IDs and values, in initialization order.
BSSimpleList<TESObjectREFR*> MiddleHighTempRefListChairBed;
SettingT<INISettingCollection> fAimUpDegrees("fAimUpDegrees:Combat", 90.0f);
SettingT<INISettingCollection> fAimDownDegrees("fAimDownDegrees:Combat", 90.0f);
SettingT<INISettingCollection> fAimChaseLookingMult("fAimChaseLookingMult:Combat", 3.0f);
BSCriticalSection TESObjectREFR::kFurnitureLock;
BSPerformanceTimer AIProcessing_MiddleHighUpdate("AIProcessing", "MiddleHighUpdate", true, true);

// GarbageCollector::Add for animations.
void GarbageCollector_AddAnimation(Animation* apAnimation) {
    GarbageCollector v5;
    v5.EnterCS(8, "GarbageCollector::Add(Animation)");
    apAnimation->ShutdownAllAnimIdles();
    if (apAnimation->GetControllerManager()) {
        NiControllerManager* ControllerManager = apAnimation->GetControllerManager();
        ControllerManager->DeactivateAll(0.0f);
    }
    NiNode* SceneRoot = apAnimation->GetSceneRoot();
    if (SceneRoot) {
        NiTimeController* pkControl = SceneRoot->GetController(&NiMultiTargetTransformController::ms_RTTI);
        if (pkControl) {
            SceneRoot->RemoveController(pkControl);
        }
        NiTimeController* pkControla = SceneRoot->GetController(&BSMultiTargetTreadTransfController::ms_RTTI);
        if (pkControla) {
            SceneRoot->RemoveController(pkControla);
        }
    }
    pModelLoader->CancelReplacementKFList(apAnimation);
    if (!GetThreadTLSData()->bCollectGarbage || (pSaveLoadGame && pSaveLoadGame->GetSaveGameLoading())) {
        if (apAnimation) {
            delete apAnimation;
        }
    }
    else {
        GarbageCollector::AnimGarbageA.Add(apAnimation);
    }
    v5.LeaveCS();
}

MiddleHighProcess::DeferredHideLimb::DeferredHideLimb() : spDismemberedLimbRoot(nullptr), spReplacementLimb(nullptr) {}

MiddleHighProcess::DeferredHideLimb::~DeferredHideLimb() {}

MiddleHighProcess::MiddleHighProcess()
  : fPursueTimer(0.0f), fEquippedWeight(0.0f), bDoneOnce(0), bAimingTarget(0), bNeedReparentWeapon(0), bDrawn(0),
    eUseItem(POTYPE_NONE), pLastIdlePlayed(0), bPickPackIdle(0), pWeapon(0), pAmmo(0), bWeaponGrenade(0),
    bWeaponMine(0), bWeaponThrown(0), bHeavyArmorWorn(0), bPowerArmorWorn(0), bPowerArmorHelmet(0), bHasBackPackWorn(0),
    pWeaponBone(0), pFireNode(0), bWantWeaponDrawn(0), bWeaponDrawn(0), cKnockState(0), cSitSleepState(0),
    pCurrentFurniture(0), cCurrentFurnitureIndex(127), FurnitureMarker(255u), pCommandingActor(0), pLastBoundWeapon(0),
    pCurrentPackageSpell(0), bCheckMagicNode(0), pCurrentWeaponEffect(0), fAlphaMult(1.0f), fScriptRefractPower(0.0f),
    m_pFaceAnimationData(0), bRefreshMagicShaderEffects(0), bRefreshMagicShaderEffectBody(0),
    bRefreshMagicShaderEffectWeapon(0), bRefreshMagicShaderEffectLoading(0), bRefreshFlareFlags(0), uiFlareFlags(0),
    bPickPocketed(0), bDoneClothesChange(0), bDetectLifeDetected(0), bSummonedCreature(0), cUpdate3DModel(0),
    bForceNextUpdate(0), iPackageIdleNumber(-1), pCurrentIdle(0), fPackageIdleTimer(-1.0f), bPlayedBeginIdles(0),
    bPlayedEndIdles(0), pCreatureLipsynchAnim(0), iCreatureLipsynchStartTime(0), iReservationSlot(-1),
    pAttachedArrowList(0), pAnimation(0), fAimLooking(0.0f), fAutomaticShotDelay(0.0f), cShotsToFire(1),
    bBeenAttacked(0), pHeadNode(0), pTorsoNode(0), pBSBound(0), bIronSights(0), iAnimActionSuccess(2),
    iMovementAnimations(0), pListItemstoEquipUnequip(0), fRadiationDelta(0.0f), fRadiationMagicDelta(0.0f),
    fRadiationWaterDelta(0.0f), pLastHitData(0), iWeaponConditionStage(-1), pFaceNode(0), pFaceNodeSkinned(0),
    pAnimFace(0), pLastAttackHitData(0), nFurnitureListTimer(0) {
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
    FurnitureMarker.SetPosition(NiPoint3_ZERO);
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
        delete static_cast<DeferredHideLimb*>(DeferredHideLimbList.m_item);
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
    ActorPackageData* pactorPackageData = BaseProcess::GetInstanceDataThatIsRunning();
    if (pactorPackageData) {
        pactorPackageData->HandleDeletedReference(apRef);
    }
    RunOncePackage.HandleDeletedReference(apRef);
    ActorPackageData* pactorPackageData_2 = GetRunOncePackageInstanceData();
    if (pactorPackageData_2) {
        pactorPackageData_2->HandleDeletedReference(apRef);
    }
}

void MiddleHighProcess::Update(MobileObject* apRef, float fTimeDelta) {
    if (apRef) {
        Actor* apActor_2 = dynamic_cast<Actor*>(apRef);
        bool bevalflag = 1;
        if (apActor_2) {
            if (apActor_2->GetRefPersists()) {
                CheckforNewPackage(apActor_2, 0);
                TESPackage* pPack = GetPackageThatIsRunning();
                ResetTarget();
                if (pPack) {
                    if (pPack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_NONE && !pPack->GetWaiting()) {
                        while (bevalflag) {
                            uint32_t Count = pPack->ePROCEDURE_TYPE;
                            switch (Procedure_Action[Count][GetProcedureIndexRunning()]) {
                                case PROCEDURE_TRAVEL: {
                                    TESPackage* v4;
                                    TESPackage* v14;
                                    TESObjectREFR* pRef = 0;
                                    if (pPack->pPackLoc) {
                                        pRef = pPack->pPackLoc->GetLocReference();
                                    }
                                    if (pGenericLocation) {
                                        if (!GetRunOncePackage()) {
                                            pRef = pGenericLocation;
                                        }
                                    }
                                    if (apActor_2->GetSitSleepState()) {
                                        TESPackage* CurrentPackage = apActor_2->GetCurrentPackage();
                                        if (!CurrentPackage->IsActorAtLocation(apActor_2, 0, -1.0f, 0)) {
                                            ProcessGetUpOneHour(apActor_2);
                                        }
                                    }
                                    ProcessTravel(apActor_2, 1, 1, 257, 0);
                                    bevalflag = apActor_2->GetSitSleepState() == SIT_SLEEP_NORMAL &&
                                        (apActor_2->GetCurrentPackage() &&
                                         ((v4 = apActor_2->GetCurrentPackage(),
                                           v4->GetPackType() != PACKAGE_TYPE_TRAVEL) &&
                                          ((v14 = apActor_2->GetCurrentPackage(),
                                            v14->IsActorAtLocation(apActor_2, 0, -1.0f, 0)) &&
                                           (apActor_2->pCurrentProcess->eLevel == PROCESS_TYPE_MIDDLE_HIGH &&
                                            (!pRef || !pRef->IsFurniture())))));
                                    break;
                                }
                                case PROCEDURE_WANDER: {
                                    bevalflag = 0;
                                    break;
                                }
                                case PROCEDURE_ACTIVATE: {
                                    bevalflag = ProcessActivateOneHour(apActor_2);
                                    break;
                                }
                                case PROCEDURE_SLEEP: {
                                    bevalflag = 0;
                                    break;
                                }
                                case PROCEDURE_EAT: {
                                    bevalflag = ProcessEatOneHour(apActor_2);
                                    break;
                                }
                                case PROCEDURE_FOLLOW: {
                                    if (apActor_2->GetSitSleepState()) {
                                        ProcessGetUpOneHour(apActor_2);
                                    }
                                    bevalflag = ProcessFollowOneHour(apActor_2, 3600.0f, 1);
                                    break;
                                }
                                case PROCEDURE_ESCORT: {
                                    if (apActor_2->GetSitSleepState()) {
                                        ProcessGetUpOneHour(apActor_2);
                                    }
                                    bevalflag = ProcessEscortOneHour(apActor_2);
                                    break;
                                }
                                case PROCEDURE_ALARM: {
                                    if (apActor_2->GetSitSleepState()) {
                                        ProcessGetUpOneHour(apActor_2);
                                    }
                                    ProcessAlarm(apActor_2);
                                    bevalflag = 0;
                                    break;
                                }
                                case PROCEDURE_COMBAT: {
                                    bevalflag = ProcessCombatOneHour(apActor_2, fTimeDelta);
                                    break;
                                }
                                case PROCEDURE_FLEE: {
                                    if (apActor_2->GetSitSleepState()) {
                                        ProcessGetUpOneHour(apActor_2);
                                    }
                                    ProcessFlee(apActor_2);
                                    bevalflag = 0;
                                    break;
                                }
                                case PROCEDURE_DIALOGUE: {
                                    ProcessDialogue(apActor_2);
                                    bevalflag = 0;
                                    break;
                                }
                                case PROCEDURE_WAIT: {
                                    TESPackage* v5;
                                    int32_t ipadding;
                                    if (!pTarget) {
                                        SetTargetForPackage(apActor_2);
                                    }
                                    if (!pTarget ||
                                        (pTarget->GetDelete() ||
                                         (v5 = GetCurrentPackage(),
                                          (ipadding = v5->GetLocationRadius(apActor_2) == 0 ? 400 : 0,
                                           pPack->IsActorAtRefTarget(apActor_2, ipadding))))) {
                                        AddToProcedureIndexRunning(apActor_2, 1);
                                    }
                                    bevalflag = 0;
                                    break;
                                }
                                case PROCEDURE_TRAVEL_TARGET: {
                                    TESPackage* v13;
                                    if (!pTarget) {
                                        SetTargetForPackage(apActor_2);
                                    }
                                    if (!pTarget) {
                                        AddToProcedureIndexRunning(apActor_2, 1);
                                    }
                                    if (pTarget) {
                                        if (pTarget->IsActor() && pTarget != pPlayer &&
                                            pPack->GetPackType() == PACKAGE_TYPE_ESCORT) {
                                            TESPackage* v6 = GetCurrentPackage();
                                            CreateFollowforEscort(apActor_2, v6, true);
                                        }
                                    }
                                    ProcessActivateOneHour(apActor_2);
                                    bevalflag = apActor_2->GetCurrentPackage() &&
                                        ((v13 = apActor_2->GetCurrentPackage(),
                                          v13->IsActorAtRefTarget(apActor_2, 0)) &&
                                         apActor_2->pCurrentProcess->eLevel == PROCESS_TYPE_MIDDLE_HIGH);
                                    break;
                                }
                                case PROCEDURE_PURSUE: {
                                    if (apActor_2->GetSitSleepState()) {
                                        ProcessGetUpOneHour(apActor_2);
                                    }
                                    ProcessPursue(apActor_2, 0, 1, 257);
                                    bevalflag = 0;
                                    break;
                                }
                                case PROCEDURE_OBSERVE_COMBAT: {
                                    AddToProcedureIndexRunning(apActor_2, 1);
                                    goto LABEL_81;
                                }
                                case PROCEDURE_GET_UP: {
                                    ProcessGetUpOneHour(apActor_2);
                                    AddToProcedureIndexRunning(apActor_2, 1);
                                    break;
                                }
                                case PROCEDURE_MOUNT_HORSE: {
                                    AddToProcedureIndexRunning(apActor_2, 1);
                                    bevalflag = 0;
                                    break;
                                }
                                case PROCEDURE_NOTIFY: {
                                    ProcessNotify(apActor_2);
                                    bevalflag = 0;
                                    break;
                                }
                                case PROCEDURE_ACCOMPANY: {
                                    ProcessAccompanyOneHour(apActor_2);
                                    bevalflag = 0;
                                    break;
                                }
                                case PROCEDURE_USE_ITEM_AT: {
                                    ProcessUseItemAt(apActor_2);
                                    bevalflag = 0;
                                    break;
                                }
                                case PROCEDURE_CHOOSE_CAST: {
                                    bevalflag = 0;
                                    break;
                                }
                                case PROCEDURE_REMOVE_WORN_ITEMS: {
                                    ProcessRemoveWorn(apActor_2, 1, 0);
                                    bevalflag = 0;
                                    break;
                                }
                                case PROCEDURE_AVOID_RADIATION: {
                                    ProcessAvoidArea(apActor_2);
                                    bevalflag = 0;
                                    break;
                                }
                                case PROCEDURE_TAKE_BACK_ITEM: {
                                    AddToProcedureIndexRunning(apActor_2, 2);
                                    break;
                                }
                                case PROCEDURE_GUARD: {
                                    bevalflag = 0;
                                    break;
                                }
                                case PROCEDURE_USE_WEAPON: {
                                    ProcessUseWeapon(apActor_2);
                                    bevalflag = 0;
                                    break;
                                }
                                case PROCEDURE_DONE: {
                                LABEL_81:;
                                    bevalflag = 0;
                                    break;
                                }
                                default: {
                                    bevalflag = 0;
                                    break;
                                }
                            }
                            if (apActor_2->GetCurrentProcessType() == PROCESS_TYPE_MIDDLE_HIGH) {
                                TESPackage* pPack_2 = GetPackageThatIsRunning();
                                if (pPack_2) {
                                    if (pPack_2->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_NONE) {
                                        uint32_t v7 = pPack_2->ePROCEDURE_TYPE;
                                        if (Procedure_Action[v7][GetProcedureIndexRunning()] == PROCEDURE_DONE) {
                                            if (pPack_2->ePROCEDURE_TYPE) {
                                                if (pPack_2->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE ||
                                                    (pPack_2->GetPackType() == PACKAGE_TYPE_EAT ||
                                                     pPack_2->GetPackType() == PACKAGE_TYPE_SLEEP)) {
                                                LABEL_89:;
                                                    AddToProcedureIndexRunning(apActor_2, -1);
                                                    return;
                                                }
                                            }
                                            else {
                                                if (!pPack_2->IsActorAtLocation(apActor_2, 0, -1.0f, 0)) {
                                                    goto LABEL_89;
                                                }
                                            }
                                            EndMoveMessage(apActor_2);
                                            if (pPack_2) {
                                                if (pPack_2->GetOncePerDay()) {
                                                    char Day = appCalendar.GetDay();
                                                    apActor_2->SetRunOnceExtraPackage(pPack_2, Day);
                                                }
                                            }
                                            if (!pPack_2->GetPackageSchedule()->PSData.iDuration) {
                                                SetTarget(0);
                                                if (!GetRunOncePackage() || ContinuingPackageforPC()) {
                                                    TESPackage* v9 = GetCurrentPackage();
                                                    if (v9->GetIsCreated()) {
                                                        // Fetches the package once more and does not use it, as the
                                                        // game does.
                                                        GetCurrentPackage();
                                                        TESPackage* v10 = GetCurrentPackage();
                                                        if (v10->IsInterruptPackage()) {
                                                            LoadPackageFromExtraData(apActor_2);
                                                        }
                                                        else {
                                                            ClearCurrentPackage();
                                                        }
                                                        if (!apActor_2->IsPathingComplete()) {
                                                            EndMoveMessage(apActor_2);
                                                        }
                                                    }
                                                }
                                                else {
                                                    ClearRunOncePackage();
                                                }
                                                if (pAcquireObject) {
                                                    pAcquireObject->Delete(1u);
                                                }
                                                pAcquireObject = 0;
                                                pItemBeingUsed = 0;
                                                while (!ObjectList.IsEmpty()) {
                                                    ObjectstoAcquire* pObject = ObjectList.m_item;
                                                    if (pObject) {
                                                        pObject->Delete(1u);
                                                    }
                                                    ObjectList.Remove(pObject);
                                                }
                                                pGenericLocation = 0;
                                                GenericLocationsList.RemoveAll();
                                            }
                                        }
                                    }
                                }
                            }
                            pPack = GetPackageThatIsRunning();
                        }
                    }
                }
            }
        }
        if (!apActor_2->IsWaitingOnPath()) {
            BaseProcess* CurrentAIProcess = apRef->pCurrentProcess;
            CurrentAIProcess->ComputeLastTimeProcessed();
        }
    }
}

void MiddleHighProcess::Update(MobileObject* apRef) {
    AIProcessing_MiddleHighUpdate.Start();
    TESPackage* pPack = 0;
    if (apRef) {
        if (!apRef->Get3D() ||
            (apRef->GetDelete() ||
             (apRef->GetDisabled() || (!apRef->GetParentCell() || !apRef->GetParentCell()->IsLoaded())))) {
            ComputeLastTimeProcessed();
            return;
        }
        Actor* apActor = apRef->IsActor() ? static_cast<Actor*>(apRef) : 0;
        if (apActor) {
            if (bDetectLifeDetected) {
                bDetectLifeDetected = DetectLifeEffect::CheckActorDetected(apActor, bDetectLifeDetected);
            }
            bool bNewPackage = CheckforNewPackage(apActor, 0);
            pPack = GetPackageThatIsRunning();
            if (bNewPackage || (!pPack && !apActor->IsPathingComplete())) {
                ClearCurrentDataforProcess(apActor);
            }
            pPack = GetPackageThatIsRunning();
            ResetTarget();
            if (bNewPackage) {
                apActor->AlertAccompanyToPackageChange();
                SetLocationForPackage(apActor, 0);
            }
            if (pPack && (pPack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_NONE && !pPack->GetWaiting())) {
                uint32_t Count = pPack->ePROCEDURE_TYPE;
                switch (Procedure_Action[Count][GetProcedureIndexRunning()]) {
                    case PROCEDURE_TRAVEL: {
                        ProcessTravel(apActor, bNewPackage, 1, 257, 0);
                        goto LABEL_114;
                    }
                    case PROCEDURE_ACTIVATE: {
                        ProcessActivate(apActor, 1);
                        goto LABEL_114;
                    }
                    case PROCEDURE_ACQUIRE: {
                        ProcessAcquire(apActor);
                        goto LABEL_114;
                    }
                    case PROCEDURE_SLEEP: {
                        Interface::GetConsoleSelection();
                        goto LABEL_114;
                    }
                    case PROCEDURE_EAT: {
                        ProcessEat(apActor, 0);
                        goto LABEL_114;
                    }
                    case PROCEDURE_FOLLOW: {
                        ProcessFollow(apActor, 1, 257, 0);
                        goto LABEL_114;
                    }
                    case PROCEDURE_ESCORT: {
                        ProcessEscort(apActor);
                        goto LABEL_114;
                    }
                    case PROCEDURE_ALARM: {
                        ProcessAlarm(apActor);
                        goto LABEL_114;
                    }
                    case PROCEDURE_FLEE: {
                        ProcessFlee(apActor);
                        break;
                    }
                    case PROCEDURE_DIALOGUE: {
                        ProcessDialogue(apActor);
                        goto LABEL_114;
                    }
                    case PROCEDURE_WAIT: {
                        if (!pTarget) {
                            SetTargetForPackage(apActor);
                        }
                        if (!pTarget || pTarget->GetDelete()) {
                            AddToProcedureIndexRunning(apActor, 1);
                        }
                        if (pPack->GetWaiting()) {
                            if (!pPlayer->IsPlayerFollower(apActor)) {
                                int32_t NumberOfFollowers = pPlayer->GetNumberOfFollowers();
                                if (NumberOfFollowers < iNumberActorsAllowedToFollowPlayer.Int()) {
                                    pPack->SetWaiting(0);
                                }
                            }
                        }
                        else {
                            TESPackage* CurrentPackage = apActor->GetCurrentPackage();
                            if (CurrentPackage->GetPackType() == PACKAGE_TYPE_FOLLOW) {
                                TESPackage* pPack_2 = GetCurrentPackage();
                                float fwaitdistance = static_cast<float>(pPack_2->GetPackageTarget()->iValue);
                                if (static_cast<double>(fwaitdistance) >=
                                    apActor->GetDistanceFromReference(pTarget, 0, 0)) {
                                    AddToProcedureIndexRunning(apActor, 1);
                                }
                            }
                            else {
                                float fwaitdistance_2 = 0.0f;
                                int32_t idistpack = pPack->GetPackageTarget()->iValue;
                                if (idistpack <= 0) {
                                    idistpack = 200;
                                }
                                if (fwaitdistance_2 == 0.0 && apActor->GetParentCell()->GetInterior()) {
                                    fwaitdistance_2 = fAIEscortWaitDistanceInterior.Float();
                                }
                                else {
                                    if (fwaitdistance_2 == 0.0) {
                                        fwaitdistance_2 =
                                            fAIEscortWaitDistanceExterior.Float() * static_cast<double>(idistpack);
                                    }
                                }
                                if (fwaitdistance_2 / 2.0 >= apActor->GetDistanceFromReference(pTarget, 0, 0)) {
                                    AddToProcedureIndexRunning(apActor, 1);
                                }
                            }
                        }
                        goto LABEL_114;
                    }
                    case PROCEDURE_TRAVEL_TARGET: {
                        ProcessActivate(apActor, 0);
                        goto LABEL_114;
                    }
                    case PROCEDURE_PURSUE: {
                        ProcessPursue(apActor, 0, 1, 257);
                        goto LABEL_114;
                    }
                    case PROCEDURE_CREATE_FOLLOW: {
                        if (!pTarget) {
                            SetTargetForPackage(apActor);
                        }
                        if (!pTarget) {
                            AddToProcedureIndexRunning(apActor, 1);
                            if (!apActor->IsPathingComplete()) {
                                EndMoveMessage(apActor);
                            }
                        }
                        if (pTarget != pPlayer) {
                            CreateFollowforEscort(apActor, pPack, 1);
                        }
                        AddToProcedureIndexRunning(apActor, 1);
                        goto LABEL_114;
                    }
                    case PROCEDURE_GET_UP: {
                        ProcessGetUp(apActor);
                        if (apActor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
                            AddToProcedureIndexRunning(apActor, 1);
                        }
                        goto LABEL_114;
                    }
                    case PROCEDURE_NOTIFY: {
                        ProcessNotify(apActor);
                        goto LABEL_114;
                    }
                    case PROCEDURE_ACCOMPANY: {
                        ProcessAccompany(apActor);
                        goto LABEL_114;
                    }
                    case PROCEDURE_USE_ITEM_AT: {
                        ProcessUseItemAt(apActor);
                        goto LABEL_114;
                    }
                    case PROCEDURE_AMBUSH_WAIT: {
                        ProcessAmbushWait(apActor);
                        goto LABEL_114;
                    }
                    case PROCEDURE_CHOOSE_CAST: {
                        ProcessChooseSpell(apActor);
                        goto LABEL_114;
                    }
                    case PROCEDURE_FLEE_NON_COMBAT: {
                        ProcessFleeNonCombat(apActor);
                        goto LABEL_114;
                    }
                    case PROCEDURE_REMOVE_WORN_ITEMS: {
                        ProcessRemoveWorn(apActor, 1, 0);
                        goto LABEL_114;
                    }
                    case PROCEDURE_SUMMON_CREATURE_DEFEND: {
                        ProcessSummonCreatureDefend(apActor);
                        goto LABEL_114;
                    }
                    case PROCEDURE_AVOID_RADIATION: {
                        ProcessAvoidArea(apActor);
                        goto LABEL_114;
                    }
                    case PROCEDURE_TAKE_BACK_ITEM: {
                        AddToProcedureIndexRunning(apActor, 2);
                        goto LABEL_114;
                    }
                    case PROCEDURE_PATROL: {
                        ProcessPatrol(apActor);
                        goto LABEL_114;
                    }
                    case PROCEDURE_DIALOGUE_ACTIVATE: {
                        ProcessDialogueActivate(apActor);
                        goto LABEL_114;
                    }
                    case PROCEDURE_USE_WEAPON: {
                        ProcessUseWeapon(apActor);
                        goto LABEL_114;
                    }
                    case PROCEDURE_DONE: {
                        break;
                    }
                    default: {
                        goto LABEL_114;
                    }
                }
                if (!IsPackageDoneOnce() && !pPack->GetIsCreated() && pPack->IsInterruptPackage()) {
                    Script::SetActionFlag(pPack, &apActor->m_Extra, SCRIPT_ACTION_ONPACKAGEEND);
                    PackageDone(apActor, pPack);
                    SetPackageDoneOnce(1);
                }
                if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_PATROL) {
                    PackageSchedule* psched = pPack->GetPackageSchedule();
                    if (!psched || static_cast<double>(psched->PSData.iDuration) <= 0.0) {
                        const TESPatrolPackageData* ppatrolData = pPack->GetPatrolPackageData();
                        if (ppatrolData) {
                            if (!ppatrolData->bRepeatable) {
                                SetCurrentActionComplete(1);
                                ClearCurrentDataforProcess(apActor);
                                ClearCurrentPackage();
                                CheckforNewPackage(apActor, 1);
                            }
                        }
                    }
                    return;
                }
                if (pPack->GetPackType() == PACKAGE_TYPE_EAT ||
                    (pPack->GetPackType() == PACKAGE_TYPE_SLEEP ||
                     (pPack->GetPackType() == PACKAGE_TYPE_TRAVEL &&
                      !pPack->IsActorAtLocation(apActor, 0, -1.0f, 0)))) {
                    AddToProcedureIndexRunning(apActor, -1);
                    return;
                }
                if (pPack->GetOncePerDay()) {
                    char Day = appCalendar.GetDay();
                    apActor->SetRunOnceExtraPackage(pPack, Day);
                }
                Interface::GetConsoleSelection();
                if (!pPack->GetPackageSchedule()->PSData.iDuration) {
                    SetTarget(0);
                    if (GetRunOncePackage()) {
                        ClearRunOncePackage();
                    }
                    else {
                        TESPackage* v7 = GetCurrentPackage();
                        if (v7->GetIsCreated()) {
                            ClearCurrentPackage();
                            if (!apActor->IsPathingComplete()) {
                                EndMoveMessage(apActor);
                            }
                        }
                    }
                    ObjectstoAcquire* apThis = pAcquireObject;
                    if (apThis) {
                        apThis->Delete(1u);
                    }
                    pAcquireObject = 0;
                    while (!ObjectList.IsEmpty()) {
                        ObjectstoAcquire* pobj = ObjectList.m_item;
                        if (pobj) {
                            pobj->Delete(1u);
                        }
                        ObjectList.Remove(pobj);
                    }
                    pGenericLocation = 0;
                    GenericLocationsList.RemoveAll();
                }
            }
        LABEL_114:;
            if (!apActor->IsWaitingOnPath()) {
                BaseProcess* CurrentAIProcess = apRef->pCurrentProcess;
                CurrentAIProcess->ComputeLastTimeProcessed();
            }
        }
        else {
            BaseProcess* v8 = apRef->pCurrentProcess;
            v8->ComputeLastTimeProcessed();
        }
        AIProcessing_MiddleHighUpdate.Stop();
    }
}

bool MiddleHighProcess::CheckforNewPackage(Actor* apActor, bool abFlag) {
    if (apActor->IsInCombat() || (apActor->IsinDialogue() || (apActor->IsAlarmed() || apActor->IsRunningRunOnce()))) {
        return 0;
    }
    SIT_SLEEP_STATE v4 = apActor->GetSitSleepState();
    if (v4 && (v4 != SIT_SLEEP_IS_SITTING && v4 != SIT_SLEEP_IS_SLEEPING)) {
        return 0;
    }
    if (GetRunOncePackage()) {
        ClearRunOncePackage();
    }
    float fhour = appCalendar.GetHour();
    bool bnewPackage = 0;
    TESPackage* prev = apActor->GetCurrentPackage();
    bool bRedress = 0;
    if (prev) {
        if (prev->GetPackType() == PACKAGE_TYPE_SLEEP || prev->GetForceUnequipWeapon()) {
            bRedress = 1;
        }
    }
    if (abFlag || (!GetCurrentPackage() || iHourPackageEvaluated != static_cast<int32_t>(fhour))) {
        bnewPackage = LowProcess::CheckforNewPackage(apActor, abFlag);
        iHourPackageEvaluated = static_cast<int32_t>(fhour);
    }
    if (bnewPackage) {
        iAnimActionSuccess = 2;
        SetContinuingPackage(0);
        SetCurrentActionComplete(0);
        SetPackageIdleDone(0);
        SetTargetActivated(0);
        while (!ObjectList.IsEmpty()) {
            ObjectstoAcquire* pobj = ObjectList.m_item;
            if (pobj) {
                pobj->Delete(1u);
            }
            ObjectList.Remove(pobj);
        }
        if (bRedress) {
            TESNPC* pnpc = dynamic_cast<TESNPC*>(apActor->GetObjectReference());
            TESCreature* pcreature = dynamic_cast<TESCreature*>(apActor->GetObjectReference());
            TESPackage* pPack = GetCurrentPackage();
            bool abwearweapon = 1;
            if (pPack) {
                abwearweapon = !pPack->GetForceUnequipWeapon();
            }
            if (pnpc) {
                pnpc->InitDefaultWorn(apActor, 1, abwearweapon, 0, 1);
            }
            else {
                if (pcreature) {
                    pcreature->InitDefaultWorn(apActor, 1, abwearweapon, 1);
                }
            }
        }
    }
    return bnewPackage;
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
    Animation* v2;
    if (pCurrentFurniture) {
        pCurrentFurniture->SetMarkerUsed(cCurrentFurnitureIndex, 0);
        if (apActor->GetAnimation()) {
            v2 = apActor->GetAnimation();
            v2->SpecialIdleFree(1, 0);
        }
    }
    SetSitSleepState(apActor, SIT_SLEEP_NORMAL, nullptr, (uint8_t)0x7Fu);
    if (static_cast<const void*>(apActor) == static_cast<const void*>(pPlayer)) {
        pPlayer->ClearSitHeadingDelta();
    }
    ClearFurnitureMarker();
}

bool MiddleHighProcess::SetTargetForPackage(Actor* apActor) {
    ExtraDataList* extra;
    ExtraDataList* v5;
    TESObjectREFR* referencePointer;
    SetTarget(nullptr);
    TESPackage* v8 = GetPackageThatIsRunning();
    if ((!v8) ||
        ((!v8->GetPackageTarget()) && ((((int32_t)v8->GetPackType()) != 12) && (((int32_t)v8->GetPackType()) != 13)))) {
        return (bool)0;
    }
    TESObjectREFR* v3 = GetTargetForPackage(apActor, v8);
    SetTarget(v3);
    if (!pTarget) {
        return (bool)0;
    }
    if (pTarget->IsStillLoading()) {
        SetTarget(nullptr);
        return (bool)0;
    }
    else {
        if (pTarget->GetDelete()) {
            extra = pTarget->TESObjectREFR::GetExtra();
            if (extra->GetReferencePointer()) {
                v5 = pTarget->TESObjectREFR::GetExtra();
                referencePointer = v5->GetReferencePointer();
                SetTarget(referencePointer);
            }
        }
        lastSeenPostion = pTarget->GetLocationOnReference();
        pTarget->SetTargeted(1);
        return (bool)1;
    }
}

TESObjectREFR* MiddleHighProcess::SetLocationForPackage(Actor* apActor, bool bSecondLoc) {
    TESPackage* v6 = GetCurrentPackage();
    if (!v6) {
        return 0;
    }
    TESObjectREFR* v5 = GetLocationForPackage(apActor, v6, bSecondLoc);
    if (bSecondLoc) {
        pGenericSecondLocation = v5;
    }
    else {
        pGenericLocation = v5;
    }
    return v5;
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
// stores the bone pointers of ap3D. Every getter is re-called as written below, as the game does.
bool MiddleHighProcess::SetCurrentWeapon(ItemChange* apweap, NiNode* ap3D, bool bForce) {
    uint8_t aMemContext[sizeof(AutoMemContext)];
    AutoMemContext* pMemContext = ::new (aMemContext) AutoMemContext(
        MEM_CONTEXT_TES_ACTOR, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp", 4273
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
            bool bSilenced =
                pWeapon->HasModEffectActive(WEAPON_MOD_SILENCE) || pWeapon->HasModEffectActive(WEAPON_MOD_EFFECT_COUNT);
            if (pWeaponObject->GetAttackSound(bSilenced)) {
                bSilenced = pWeapon->HasModEffectActive(WEAPON_MOD_SILENCE) ||
                    pWeapon->HasModEffectActive(WEAPON_MOD_EFFECT_COUNT);
                TESSound* pSettings = pWeaponObject->GetAttackSound(false);
                const char* pFilename = pWeaponObject->GetAttackSound(bSilenced)->GetFilename();
                BSAudio::QInstance().PrecacheSound(
                    pFilename, BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT, pSettings
                );
            }
            bSilenced =
                pWeapon->HasModEffectActive(WEAPON_MOD_SILENCE) || pWeapon->HasModEffectActive(WEAPON_MOD_EFFECT_COUNT);
            if (pWeaponObject->GetAttackSound2(bSilenced)) {
                bSilenced = pWeapon->HasModEffectActive(WEAPON_MOD_SILENCE) ||
                    pWeapon->HasModEffectActive(WEAPON_MOD_EFFECT_COUNT);
                TESSound* pSettings = pWeaponObject->GetAttackSound2(false);
                const char* pFilename = pWeaponObject->GetAttackSound2(bSilenced)->GetFilename();
                BSAudio::QInstance().PrecacheSound(
                    pFilename, BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT, pSettings
                );
            }
            bSilenced =
                pWeapon->HasModEffectActive(WEAPON_MOD_SILENCE) || pWeapon->HasModEffectActive(WEAPON_MOD_EFFECT_COUNT);
            if (pWeaponObject->GetAttackSound2D(bSilenced)) {
                bSilenced = pWeapon->HasModEffectActive(WEAPON_MOD_SILENCE) ||
                    pWeapon->HasModEffectActive(WEAPON_MOD_EFFECT_COUNT);
                TESSound* pSettings = pWeaponObject->GetAttackSound2D(false);
                const char* pFilename = pWeaponObject->GetAttackSound2D(bSilenced)->GetFilename();
                BSAudio::QInstance().PrecacheSound(
                    pFilename, BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT, pSettings
                );
            }
            if (pWeaponObject->GetAttackFailSound()) {
                TESSound* pSettings = pWeaponObject->GetAttackFailSound();
                const char* pFilename = pWeaponObject->GetAttackFailSound()->GetFilename();
                BSAudio::QInstance().PrecacheSound(
                    pFilename, BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT, pSettings
                );
            }
            void* pMemory = ourMalloc(sizeof(QueuedFile));
            QueuedFile* pPreload = pMemory ? ::new (pMemory) QueuedFile(IO_TASK_PRIORITY_VERY_LOW) : nullptr;
            spWeaponPreload = pPreload;
            bool bPlayerProcess = false;
            if (pPlayer && pPlayer->GetCurrentAIProcess() == pProcess) {
                bPlayerProcess = true;
            }
            IOTask* pParent = spWeaponPreload.operator->();
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
        if (pQueued &&
            (!pQueued->spQueuedFiles.operator->() || pQueued->spQueuedFiles.operator->()->GetAllChildrenFinished())) {
            bDone = true;
            if (pQueued->bEquip) {
                if (apActor->GetAnimAction() == ANIM_ACTION_BLOCK && !apActor->GetCurrentWeapon()) {
                    apActor->SetBlock(false);
                }
                apActor->EquipObject(
                    pQueued->pObject, pQueued->iNumber, pQueued->pExtra, pQueued->bNow, pQueued->bForceEquip,
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
                            pQueued->pObject, pQueued->iNumber, pQueued->pExtra, pQueued->bNow, pQueued->bForceEquip,
                            false
                        );
                        pQueued->pObject->SetFormType(WEAP_ID);
                    }
                }
            }
            else {
                apActor->UnEquipObject(
                    pQueued->pObject, pQueued->iNumber, pQueued->pExtra, pQueued->bForceUnequip, pQueued->bEquipCalling,
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
    bool bflag = 0;
    for (BSSimpleList<QueuedItem*>* plist = pListItemstoEquipUnequip; plist && (plist->m_item && !bflag);
         plist = plist->m_pkNext) {
        QueuedItem* pitem = plist->m_item;
        if (pitem) {
            if (pitem->bEquip == abEquipped && pitem->pObject == apObject) {
                bflag = apExtra == pitem->pExtra;
            }
        }
    }
    return bflag;
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
    AutoMemContext apMemory(
        MEM_CONTEXT_TES_ACTOR, 1, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp", 835
    );
    if (!IsItemQueued(apObject, apExtra, abEquip)) {
        QueuedItem* v16 = static_cast<QueuedItem*>(ourMalloc(sizeof(QueuedItem)));
        if (v16) {
            v16 = ::new (v16) QueuedItem;
        }
        else {
            v16 = 0;
        }
        v16->pObject = apObject;
        v16->bEquip = abEquip;
        v16->iNumber = aiNumber;
        v16->pExtra = apExtra;
        v16->bNow = abNow;
        v16->bForceEquip = abForceEquip;
        v16->bForceUnequip = abForceUnequip;
        v16->bEquipCalling = abEquipCalling;
        v16->bPlayPickUpSounds = abPlayPickUpSounds;
        v16->QueueFiles(apActor);
        if (!pListItemstoEquipUnequip) {
            BSSimpleList<QueuedItem*>* v12;
            BSSimpleList<QueuedItem*>* v15 =
                static_cast<BSSimpleList<QueuedItem*>*>(ourMalloc(sizeof(BSSimpleList<QueuedItem*>)));
            if (v15) {
                v12 = ::new (v15) BSSimpleList<QueuedItem*>;
            }
            else {
                v12 = 0;
            }
            pListItemstoEquipUnequip = v12;
        }
        pListItemstoEquipUnequip->AddTail(v16);
    }
}

void MiddleHighProcess::RemoveQueuedItem(TESBoundObject* apObject, ExtraDataList* apExtra) {
    for (QueuedItem* pitem : pListItemstoEquipUnequip->ItemsUntilNull()) {
        if (pitem && (pitem->pObject == apObject && apExtra == pitem->pExtra)) {
            if (pitem->spQueuedFiles) {
                pIOManager->TryCancelTask(pitem->spQueuedFiles, 0);
            }
            delete pitem;
            pListItemstoEquipUnequip->Remove(pitem);
            return;
        }
    }
}

void MiddleHighProcess::RemoveAllQueuedItems() {
    for (BSSimpleList<QueuedItem*>* plist = pListItemstoEquipUnequip; plist && plist->m_item; plist->RemoveHead()) {
        QueuedItem* pitem = plist->m_item;
        if (pitem) {
            if (pitem->spQueuedFiles) {
                pIOManager->TryCancelTask(pitem->spQueuedFiles, 0);
            }
            delete pitem;
        }
    }
    if (pListItemstoEquipUnequip) {
        delete pListItemstoEquipUnequip;
    }
    pListItemstoEquipUnequip = 0;
}

void MiddleHighProcess::SetFireNode(NiAVObject* apFireNode) {
    pFireNode = apFireNode;
}

void MiddleHighProcess::StoreBonePointers(NiControllerManager* apManager, NiNode* ap3D) {
    int32_t v12 = 1;
    bool b3rdPerson = 0;
    if (apManager) {
        Animation* Animation_0;
        if (pPlayer->GetAnimation(1) &&
            (Animation_0 = pPlayer->GetAnimation(1), apManager == Animation_0->GetControllerManager())) {
            b3rdPerson = 1;
            NiAVObjectPalette* ObjectPalette = apManager->GetObjectPalette();
            NiFixedString v10(static_cast<const char*>(bipedBoneName[1]));
            NiFixedString* v6 = &v10;
            MiddleHighProcess::pThirdPersonPlayerWeaponNode = static_cast<NiNode*>(ObjectPalette->GetAVObject(*v6));
        }
        else {
            NiAVObjectPalette* v5 = apManager->GetObjectPalette();
            NiFixedString apString(static_cast<const char*>(bipedBoneName[1]));
            NiFixedString* v4 = &apString;
            pWeaponBone = static_cast<NiNode*>(v5->GetAVObject(*v4));
        }
    }
    else {
        if (ap3D) {
            if (pPlayer->GetAnimation(1) && ap3D == pPlayer->GetAnimation(1)->GetSceneRoot()) {
                b3rdPerson = 1;
                MiddleHighProcess::pThirdPersonPlayerWeaponNode = static_cast<NiNode*>(BSUtilities::GetObjectByName(
                    static_cast<NiAVObject*>(ap3D), static_cast<const char*>(bipedBoneName[1])
                ));
            }
            else {
                pWeaponBone = static_cast<NiNode*>(BSUtilities::GetObjectByName(
                    static_cast<NiAVObject*>(ap3D), static_cast<const char*>(bipedBoneName[1])
                ));
            }
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
    FNVD_INLINED_LOCAL(BSSoundHandle, out, std::nullopt, std::nullopt);
    ForceBoneScale();
    NiNode* pweaponBone = GetWeaponBone(apBiped);
    // The biped's root, else the actor's 3D.
    NiNode* p3d = 0;
    if (apBiped) {
        p3d = apBiped->root;
    }
    if (!p3d) {
        p3d = apActor->Get3D();
    }
    if (pweaponBone) {
        NiAVObject* pobj = pweaponBone->GetAtChecked(0);
        if (pobj) {
            if (abDrawn) {
                pobj->SetAppCulled(0);
            }
        }
        if (apActor == pPlayer) {
            ReparentWeapon(abDrawn, apBiped, apAnim, apActor);
        }
        else {
            if (GetWantWeaponDrawn() || (GetCurrentWeapon() || GetWeaponDrawn())) {
                bNeedReparentWeapon = 1;
                bDrawn = abDrawn;
            }
            else {
                if (bDrawn != abDrawn && !pSaveLoadGame->GetSaveGameLoading()) {
                    // pweap is tested but never set, because the weapon's form goes to a second local, so the reparent
                    // below never runs, as the game does.
                    TESObjectWEAP* pweap = 0;
                    if (this->pWeapon) {
                        TESObjectWEAP* pformWeapon = static_cast<TESObjectWEAP*>(this->pWeapon->pContainerObj);
                        (void)pformWeapon;
                    }
                    if (pweap && !pweap->IsEmbeddedWeapon()) {
                        bNeedReparentWeapon = 1;
                        bDrawn = abDrawn;
                    }
                }
            }
        }
        if (p3d) {
            NiAVObject* pobj_2 = BSUtilities::GetObjectByName(p3d, "##NifRound");
            if (pobj_2) {
                if (pAmmo && pAmmo->iNumber) {
                    pobj_2->SetAppCulled(0);
                }
                else {
                    pobj_2->SetAppCulled(1);
                }
            }
            NiAVObject* pobj_3 = BSUtilities::GetObjectByName(p3d, "##LightOn");
            if (pobj_3) {
                pobj_3->SetAppCulled(1);
            }
        }
    }
    else {
        if (this->pWeapon && pSaveLoadGame->GetSaveGameLoading()) {
            TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(this->pWeapon->pContainerObj);
            if (pWeapon) {
                if (!pWeapon->IsEmbeddedWeapon()) {
                    bNeedReparentWeapon = 1;
                    bDrawn = abDrawn;
                }
            }
        }
        else {
            if (this->pWeapon) {
                TESObjectWEAP* v32 = static_cast<TESObjectWEAP*>(this->pWeapon->pContainerObj);
                if (v32) {
                    if (!v32->IsEmbeddedWeapon()) {
                        bNeedReparentWeapon = 1;
                        bDrawn = abDrawn;
                    }
                }
            }
        }
    }
    if (apAnim) {
        if (apAnim->pAccumRoot) {
            if (!Interface::IsInPipboyMenu() && apAnim->GetGroup(AGS_IDLE) != 1) {
                if (apActor == pPlayer) {
                    ItemChange* apWeaponItem;
                    bool v24 = abDrawn && this->pWeapon == 0;
                    if (abDrawn) {
                        apWeaponItem = this->pWeapon;
                    }
                    else {
                        // -1 is GetAnimGroup's "no weapon" value.
                        apWeaponItem = reinterpret_cast<ItemChange*>(static_cast<uintptr_t>(-1));
                    }
                    uint16_t AnimGroup = apActor->GetAnimGroup(ANIM_GROUP_IDLE, apWeaponItem, v24, apAnim);
                    apAnim->PlayGroup(AnimGroup, ACTION_START, -1, AGS_NONE);
                }
                else {
                    AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_EQUIP_UNEQUIP_WEAPON);
                }
            }
        }
    }
    if (abDrawn &&
        (GetCurrentWeapon() &&
         (apAnim && (apAnim->pAccumRoot && (!Interface::IsInPipboyMenu() || apAnim == pPlayer->GetAnimation(0)))))) {
        ItemChange* v6 = GetCurrentWeapon();
        ANIM_GROUP_ENUM ehandGripAnim = static_cast<ANIM_GROUP_ENUM>(
            static_cast<TESObjectWEAP*>(v6->pContainerObj)->TESObjectWEAP::GetHandGripAnim()
        );
        if (ehandGripAnim != ANIM_GROUP_NONE) {
            if (apActor == pPlayer) {
                uint16_t v8 = apActor->GetAnimGroup(ehandGripAnim, this->pWeapon, 0, apAnim);
                apAnim->PlayGroup(v8, ACTION_START, -1, AGS_NONE);
            }
            else {
                AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_HAND_GRIP_WEAPON);
            }
        }
    }
    else {
        if (!abDrawn || !GetCurrentWeapon()) {
            if (apAnim) {
                if (apAnim->GetCurrentSequence(AGS_LEFT_ARM)) {
                    BSAnimGroupSequence* CurrentSequence = apAnim->GetCurrentSequence(AGS_LEFT_ARM);
                    ANIM_GROUP_ENUM Type = CurrentSequence->GetAnimGroup()->GetType();
                    if (Type >= ANIM_GROUP_HAND_GRIP_1 && Type <= ANIM_GROUP_HAND_GRIP_6) {
                        apAnim->BlendOut(AGS_LEFT_ARM, 0);
                    }
                }
            }
        }
    }
    if (pSaveLoadGame->GetSaveGameLoading() || apActor == pPlayer) {
        apActor->SetHavokWeapon();
    }
    else {
        AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_HAVOKWEAPON);
    }
    TESObjectWEAP* pweapon = 0;
    if (this->pWeapon) {
        pweapon = static_cast<TESObjectWEAP*>(this->pWeapon->pContainerObj);
    }
    if (pweapon) {
        if (pweapon->GetIdleSound()) {
            TESSound* IdleSound = pweapon->GetIdleSound();
            if (IdleSound->GetFilename()) {
                BSSoundHandle hweapIdle;
                (&apActor->m_Extra)->GetWeaponIdleSound(hweapIdle);
                if (!hweapIdle.IsValid()) {
                    TESSound* v11 = pweapon->GetIdleSound();
                    uint32_t iloopFlags = TESSound::ConvertLoopFlag(v11->GetData().iFlags);
                    uint32_t iplayerFlag = pPlayer != apActor
                        ? 0
                        : static_cast<BSGameSound::SoundFlags>(
                              BSGameSound::SOUND_FLAG_FIRST_PERSON | BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE
                          );
                    bool IsActually3rdPerson = pPlayer->IsActually3rdPerson();
                    iplayerFlag ^= IsActually3rdPerson ? BSGameSound::SOUND_FLAG_FIRST_PERSON : 0;
                    TESSound* v20 = pweapon->GetIdleSound();
                    uint32_t v19 = iplayerFlag | (iloopFlags | BSGameSound::SOUND_FLAG_IS_3D);
                    TESSound* v14 = pweapon->GetIdleSound();
                    const char* Filename = v14->GetFilename();
                    BSAudio* v15 = &BSAudio::QInstance();
                    BSSoundHandle* arHandle = v15->GetSoundHandleByFilename(&out, Filename, v19, v20);
                    hweapIdle.operator=(*arHandle);
                    hweapIdle.bAssumeSuccess = 1;
                    NiNode* v16 = apActor->Get3D();
                    hweapIdle.SetObjectToFollow(v16);
                    const NiPoint3* v17 = &apActor->GetLocationOnReference();
                    hweapIdle.SetPosition(*v17);
                }
                if (!abDrawn || hweapIdle.IsPlaying()) {
                    if (!abDrawn && hweapIdle.IsPlaying()) {
                        hweapIdle.Stop();
                    }
                }
                else {
                    hweapIdle.Play(0);
                }
                (&apActor->m_Extra)->SetWeaponIdleSound(hweapIdle);
            }
        }
    }
    apActor->SetWalkRunFlagDirty();
}

void MiddleHighProcess::UpdateReparentWeapon(Actor* apActor) {
    uint16_t v2;
    const char* model;
    BipedAnim* v4;
    Animation* v5;
    BSSimpleList<const char*>* apFileList;
    uint32_t a5;
    TESObjectWEAP* formFlags;
    if (bNeedReparentWeapon) {
        if (apActor->Get3D()) {
            if (pAnimation) {
                if (pWeapon) {
                    formFlags = static_cast<TESObjectWEAP*>(pWeapon->ItemChange::GetItemChangeObject());
                    a5 = weaponToAnimGroupWeapon[formFlags->GetWeaponType()];
                    if (!pAnimation->GetAnimsLoading()) {
                        if (pAnimation->GetControllerManager()) {
                            v2 = TESAnimGroup::AnimGroup(
                                AGM_NONE, static_cast<ANIM_GROUP_WEAPON_ENUM>(a5), ANIM_GROUP_WEAPONS_START, 0
                            );
                            if ((!pAnimation->GroupLoaded(v2)) && (!apActor->IsDead((bool)1))) {
                                model = apActor->GetModel();
                                apFileList =
                                    pModelLoader->BuildKFFileList(model, 1, 0, static_cast<ANIM_GROUP_WEAPON_ENUM>(a5));
                                pAnimation->AddAnimationsFromList(apFileList);
                            }
                        }
                    }
                }
            }
            if ((!pAnimation) || (!pAnimation->GetAnimsLoading())) {
                if (apActor->TESObjectREFR::IsDead(0)) {
                    apActor->RemoveWeapon();
                }
                else {
                    v5 = apActor->GetAnimation();
                    v4 = apActor->GetBiped();
                    ReparentWeapon(bWeaponDrawn, v4, v5, apActor);
                }
                bNeedReparentWeapon = (bool)0;
                bDrawn = (bool)0;
            }
        }
    }
}

TESPackage* MiddleHighProcess::GetRunOncePackage() {
    return RunOncePackage.pPackage;
}

void MiddleHighProcess::SetRunOncePackage(TESPackage* apPack, MobileObject* apActor) {
    TESObjectREFR* v3;
    if (apPack) {
        v3 = apPack->ComputeInitialTarget(apActor);
        (&RunOncePackage)->InitFromPackage(apPack, v3);
    }
    else {
        ClearRunOncePackage();
        SetTarget(nullptr);
    }
    ClearMovementStoped();
}

void MiddleHighProcess::ClearRunOncePackage() {
    uint8_t v2 = (uint8_t)(GetRunOncePackage() != 0);
    (&RunOncePackage)->Clear();
    if (v2) {
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
    return Procedure_Action[eType][pProcess->GetProcedureIndexRunning()] == PROCEDURE_DONE;
}

void MiddleHighProcess::SetupNewPackage() {
    MiddleHighProcess::ForceBoneScale();
    pItemBeingUsed = nullptr;
    iAnimActionSuccess = (uint16_t)2;
    SetContinuingPackage((bool)0);
    SetCurrentActionComplete((uint8_t)0);
    SetTargetActivated((uint8_t)0);
    LowProcess::SetPackageIdleDone(0);
    SetPackageDoneOnce((uint8_t)0);
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
    else {
        return BaseProcess::GetInstanceDataThatIsRunning();
    }
}

ActorPackage* MiddleHighProcess::GetActorPackageThatIsRunning() {
    if (GetRunOncePackage()) {
        return &RunOncePackage;
    }
    else {
        return &CurrentPackage;
    }
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
    SetAnimationActiveFlag(0);
    if (GetRunOncePackage()) {
        AddToRunOnceProcedureIndex(ainumb);
    }
    else {
        AddToCurrentProcedureIndex(ainumb);
    }
    if (GetProcedureIndexRunning() < 0) {
        SetProcedureIndexRunning(0);
    }
    if (pPack) {
        if (pPack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_NONE) {
            PACKAGE_PROCEDURE_TYPE Count = pPack->ePROCEDURE_TYPE;
            int32_t irange = Procedure::GetProcedureCount(Count);
            if (GetProcedureIndexRunning() >= irange) {
                SetProcedureIndexRunning(irange - 1);
            }
            uint32_t v4 = pPack->ePROCEDURE_TYPE;
            if (Procedure_Action[v4][GetProcedureIndexRunning()] == PROCEDURE_DONE && !IsPackageDoneOnce()) {
                Script::SetActionFlag(pPack, &apActor->m_Extra, SCRIPT_ACTION_ONPACKAGEEND);
                PackageDone(static_cast<Actor*>(apActor), pPack);
                SetPackageDoneOnce(1);
            }
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
    MiddleHighProcess* v13 = this;
    TESPackage* v21 = 0;
    TESPackage* pPack = GetPackageThatIsRunning();
    if (pPack && pPack->GetWaiting()) {
        if (pPlayer->IsPlayerFollower(apActor)) {
            return;
        }
        int32_t NumberOfFollowers = pPlayer->GetNumberOfFollowers();
        if (NumberOfFollowers >= iNumberActorsAllowedToFollowPlayer.Int()) {
            return;
        }
        pPack->SetWaiting(0);
    }
    TESObjectREFR* apRef = 0;
    if (!v13->pTarget || v13->pTarget->GetDelete()) {
        v13->SetTargetForPackage(apActor);
        if (v13->pTarget) {
            v13->lastSeenPostion = v13->pTarget->GetLocationOnReference();
        }
    }
    if (!v13->pTarget) {
        if (abflag) {
            v13->AddToProcedureIndexRunning(apActor, 1);
        }
        return;
    }
    float fcurrentdistance = apActor->GetDistanceFromReference(v13->pTarget, 0, 0);
    apActor->GetWorldSpace();
    v13->pTarget->GetWorldSpace();
    float fdistance = pPack->GetPackageRadiusActorToRefTarget(apActor, 0);
    TESObjectREFR* ptarg = v13->pTarget;
    if (!v13->pTarget ||
        (pPack->GetPackType() == PACKAGE_TYPE_FOLLOW &&
         (pPack->pPackLoc &&
          (pPack->pPackLoc->GetLocType() < PLT_CURRLOC && pPack->IsActorAtLocation(apActor, 0, -1.0f, 0))))) {
        if (abflag) {
            v13->AddToProcedureIndexRunning(apActor, 1);
            if (pPack->GetIsCreated()) {
                if (v13->GetRunOncePackage()) {
                    v13->ClearRunOncePackage();
                }
                else {
                    v13->ClearCurrentPackage();
                }
                v13->CheckforNewPackage(apActor, 0);
                return;
            }
            if (!apActor->IsPathingComplete()) {
                v13->EndMoveMessage(apActor);
            }
        }
    }
    else {
        const NiPoint3* v6 = &v13->pTarget->GetLocationOnReference();
        float fdistancemoved = (v13->lastSeenPostion - *v6).Length();
        float* v8 = &fAIMoveDistanceToRecalcFollowPath.Float();
        if (*v8 < static_cast<double>(fdistancemoved) ||
            (fdistance < static_cast<double>(fcurrentdistance) && apActor->IsPathingComplete())) {
            apActor->SetMoveMode(static_cast<uint32_t>(emovemode));
            TESWorldSpace* pworld = v13->pTarget->GetWorldSpace();
            TESObjectCELL* pcell = v13->pTarget->GetParentCell();
            if (!v13->SetupPathFinding(apActor, v13->pTarget->GetLocationOnReference(), pcell, pworld)) {
                return;
            }
            v13->lastSeenPostion = v13->pTarget->GetLocationOnReference();
        }
        if (!apActor->IsPathingComplete()) {
            TESGlobal* ptimescale = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(58u));
            float fhour = appCalendar.GetHour();
            double LastTimeProcessed = v13->fHourLastProcessed;
            if (fhour < LastTimeProcessed) {
                fhour = fhour + 24.0;
            }
            double v10 = v13->fHourLastProcessed;
            float ftimedelta = fhour - v10;
            double v11 = ptimescale->fValue;
            ftimedelta = 3600.0 / v11 * ftimedelta;
            (void)ftimedelta;
        }
    }
    fcurrentdistance = apActor->GetDistanceFromReference(ptarg, 0, 0);
    if (fdistance > static_cast<double>(fcurrentdistance) && !apActor->IsPathingComplete()) {
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
    TESPackage* v5 = GetPackageThatIsRunning();
    TrespassPackage* pTresPack = dynamic_cast<TrespassPackage*>(v5);
    Actor* apact = static_cast<Actor*>(pTarget);
    if (pTresPack && (apact && !apact->IsTresPassing())) {
        AddToProcedureIndexRunning(apact, 2);
        return;
    }
    if (apActor->GetDistanceFromReference(pTarget, 0, 0) > 200.0) {
        ProcessFollow(apActor, 0, emovemode, 1);
        return;
    }
    if (!apActor->IsPathingComplete()) {
        EndMoveMessage(apActor);
    }
    if (pTarget) {
        if (dynamic_cast<HighProcess*>(apActor->pCurrentProcess)) {
            if (apActor->pCurrentProcess) {
                BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
                if (CurrentAIProcess->GetSitSleepState() == SIT_SLEEP_NORMAL) {
                    NiPoint3 v17 = pTarget->GetLocationOnReference();
                    apActor->RequestRotate(v17, 0);
                }
            }
        }
    }
    if (pTresPack) {
        int32_t iwarning;
        Actor* pact = dynamic_cast<Actor*>(pTarget);
        if (pact && !pact->IsTresPassing()) {
            AddToProcedureIndexRunning(apActor, 2);
            return;
        }
        if (!pTresPack->iwarninglevel ||
            ((iwarning = pTresPack->iwarning, iwarning > pTresPack->iwarninglevel) &&
             pTresPack->fwarningtimer <= 0.0)) {
            if (apact) {
                apact->TrespassAlarm(apActor, 0, -1);
                return;
            }
        }
        if (pTresPack->fwarningtimer <= 0.0) {
            AddToProcedureIndexRunning(apActor, 1);
            return;
        }
        float v9 = appTimer.fDelta * -1.0;
        pTresPack->ModWarningTimer(v9);
        double AltTimer = pTresPack->fwarningtimer;
        if (fAITrespassWarningTimer.Float() < AltTimer) {
            float v7 = pTresPack->fwarningtimer * -1.0;
            pTresPack->ModWarningTimer(v7);
        }
    }
    Character* pchar = dynamic_cast<Character*>(pTarget);
    Actor* ptargetactor = dynamic_cast<Actor*>(pTarget);
    if (apActor->IsAlarmed()) {
        if (ptargetactor) {
            if (pchar == pPlayer) {
                if (pTarget == pPlayer && (pPlayer->IsSleepingorResting() && !pPlayer->btransporting)) {
                    pPlayer->SetSleeping(0, 1);
                }
                else {
                    if (GetTalkingtoPC() || pPlayer->IsPlayerResistingArrest()) {
                        if (pPlayer->IsPlayerResistingArrest()) {
                            EnterCombat(apActor, pPlayer, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0);
                        }
                    }
                    else {
                        if (pPlayer->Activate(apActor, 0, 0, 1)) {
                            AddToProcedureIndexRunning(apActor, 1);
                        }
                    }
                }
            }
            else {
                AddToProcedureIndexRunning(apActor, 2);
                EnterCombat(apActor, ptargetactor, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0);
            }
        }
    }
}

// Flee procedure: casts the package spell once its magic item is loaded, then, while the package still has a target,
// refreshes the avoided refs and either paths to the ref to flee to or finds a teleport door, ending the package once
// the flee timer has run out. An interior flag and an "essential down" flag are also stored that nothing reads, as the
// game does.
void MiddleHighProcess::ProcessFlee(Actor* apActor) {
    FleePackage* pFleePackage = dynamic_cast<FleePackage*>(GetPackageThatIsRunning());
    TESObjectREFR* pTargReference = nullptr;
    bool bInterior = false;
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
        // apActor is adjusted to its MagicCaster without a null test (only the MagicTarget argument has one), as the
        // game does.
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
                        bInterior = true;
                    }
                    else {
                        fFleeDistance = fFleeDistanceExterior.Float();
                    }
                    if (fFleeDistance > fDistance && apActor->IsPathingComplete()) {
                        TESObjectREFR* pRefFleeTo = pFleePackage->GetRefFleeTo();
                        bool bEssentialDown = false;
                        if (fEssentialDownTimer > 0.0f) {
                            fEssentialDownTimer = fEssentialDownTimer - appTimer.GetSeconds();
                        }
                        else {
                            bEssentialDown = true;
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
                    else {
                        TESGlobal* pTimeScale = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(0x3A));
                        float fHour = appCalendar.GetHour();
                        if (fHour < GetLastTimeProcessed()) {
                            fHour = static_cast<float>(fHour + 24.0);
                        }
                        float fElapsed = fHour - GetLastTimeProcessed();
                        fElapsed = static_cast<float>(3600.0 / pTimeScale->GetValue() * fElapsed);
                        static_cast<void>(fElapsed);
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
    static_cast<void>(bInterior);
}

void MiddleHighProcess::ProcessGetUpOneHour(Actor* apActor) {
    apActor->GetOutofFurnitureQuick();
}

bool MiddleHighProcess::SitOrSleep(Actor* apActor) {
    bool bsleeping;
    TESPackage* ppack = GetPackageThatIsRunning();
    if ((!pCurrentFurniture || !pCurrentFurniture->Get3D()) &&
        (!ppack || ppack->GetPackType() != PACKAGE_TYPE_SANDMAN)) {
        return 0;
    }
    if (GetWeaponDrawn() && !GetAlert()) {
        apActor->SetWantWeaponDrawn(0);
        return 1;
    }
    pGenericLocation = 0;
    Animation* panim = apActor->GetAnimation();
    switch (apActor->GetAnimAction()) {
        case ANIM_ACTION_EQUIP_WEAPON:
        case ANIM_ACTION_UNEQUIP_WEAPON:
        case ANIM_ACTION_ATTACK:
        case ANIM_ACTION_ATTACK_FOLLOW_THROUGH:
        case ANIM_ACTION_BLOCK:
        case ANIM_ACTION_RECOIL:
        case ANIM_ACTION_STAGGER:
        case ANIM_ACTION_DODGE: {
            return 1;
        }
        default: {
            bsleeping = 0;
            if (pCurrentFurniture) {
                uint32_t Type = FurnitureMarker.GetType();
                bsleeping = TESFurniture::IsSleepMarker(Type);
            }
            if (cSitSleepState) {
                break;
            }
            HighProcess* phigh = dynamic_cast<HighProcess*>(this);
            if (phigh) {
                if (phigh->GetCurrentProcessIdle() ||
                    (apActor->IsAnimIdleQuequed() || (panim->SpecialIdleWorking() || panim->SpecialIdlePlaying()))) {
                    phigh->SetCurrentProcessIdle(0);
                    phigh->ClearPostAnimationActions();
                    panim->SpecialIdleFree(1, 1);
                }
            }
            if (apActor->IsPc()) {
                HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_SIT_SLEEP_ANIM);
            }
            if (bsleeping && !GetCurrentPackage()) {
                ProcessRemoveWorn(apActor, 0, 0);
            }
            if (apActor->IsSneaking()) {
                apActor->SetWalkRunFlagDirty();
            }
            apActor->ClearMoveMode(1024u);
            if (apActor != pPlayer) {
                EndMoveMessage(apActor);
            }
            if (pCurrentFurniture->GetMarkerUsed(cCurrentFurnitureIndex, 1) &&
                (ppack && ppack->GetPackType() != PACKAGE_TYPE_SANDMAN)) {
                SetSitSleepState(apActor, SIT_SLEEP_NORMAL, 0, 127);
                return 1;
            }
            if (ppack) {
                if (ppack->GetPackType() != PACKAGE_TYPE_SANDMAN) {
                    uint32_t v4;
                    if (bsleeping) {
                        SetSitSleepState(apActor, SIT_SLEEP_LOAD_SLEEP_IDLE, pCurrentFurniture, cCurrentFurnitureIndex);
                    }
                    else {
                        SetSitSleepState(apActor, SIT_SLEEP_LOAD_SIT_IDLE, pCurrentFurniture, cCurrentFurnitureIndex);
                    }
                    if (bsleeping || (v4 = FurnitureMarker.GetType(), TESFurniture::IsSitMarker(v4))) {
                        if (!LoadDynamicIdle(apActor)) {
                            int32_t a2 = cCurrentFurnitureIndex;
                            const char* afWalkSpeed = pCurrentFurniture->GetFormEditorID();
                            Warning(
                                "AI: Missing furniture dynamic idle for marker %d in the editor's idle manager. '%s' "
                                "index %d.",
                                FurnitureMarker.GetType(), afWalkSpeed, a2
                            );
                        }
                    }
                }
            }
            SetCurrentProcessAnimIdleLoaded(0, 0);
            RemovePostAnimationAction(BaseProcess::POSTANIM_ACTION_IDLE_LOADED);
            RemovePostAnimationAction(BaseProcess::POSTANIM_ACTION_PLAY_IDLE);
            if (bsleeping) {
                SetSitSleepState(apActor, SIT_SLEEP_WANT_TO_SLEEP, pCurrentFurniture, cCurrentFurnitureIndex);
            }
            else {
                SetSitSleepState(apActor, SIT_SLEEP_WANT_TO_SIT, pCurrentFurniture, cCurrentFurnitureIndex);
            }
            TESIdleForm* pidle = pIdleManager->GetIdleToPlay(apActor, pCurrentFurniture);
            if (!pidle) {
                int32_t a2a = cCurrentFurnitureIndex;
                const char* afWalkSpeeda = pCurrentFurniture->GetFormEditorID();
                Warning(
                    "AI: Missing furniture entry animation for marker %d in the editor's idle manager. '%s' index %d.",
                    FurnitureMarker.GetType(), afWalkSpeeda, a2a
                );
                cSitSleepState = 0;
                return 0;
            }
            ANIM_GROUP_SECTION IdleAnimGroupSection = pidle->GetIdleAnimGroupSection();
            panim->SpecialIdleQueue(pidle, IdleAnimGroupSection);
            apActor->SetPosition(FurnitureMarker.Position);
            panim->ClearDelta();
            pCurrentFurniture->SetMarkerUsed(cCurrentFurnitureIndex, 1);
            break;
        }
    }
    switch (cSitSleepState) {
        case 2:
        case 7: {
            float ftargetangle = FurnitureMarker.GetHeading();
            apActor->StopMoving();
            apActor->SetHeading(ftargetangle);
            apActor->SetPosition(FurnitureMarker.Position);
            panim->ClearDelta();
            if (bsleeping) {
                SetSitSleepState(apActor, SIT_SLEEP_WAITING_FOR_SLEEP_ANIM, pCurrentFurniture, cCurrentFurnitureIndex);
            }
            else {
                SetSitSleepState(apActor, SIT_SLEEP_WAITING_FOR_SIT_ANIM, pCurrentFurniture, cCurrentFurnitureIndex);
            }
            return 1;
        }
        case 3:
        case 8: {
            if (panim->SpecialIdleLoaded() && GetAnimAction() == ANIM_ACTION_NONE) {
                BSAnimGroupSequence* CurrentSequence;
                if (!panim->GetCurrentSequence(AGS_MOVEMENT) ||
                    (CurrentSequence = panim->GetCurrentSequence(AGS_MOVEMENT),
                     CurrentSequence->GetState() == NiControllerSequence::kAnimating)) {
                    panim->ClearDelta();
                    apActor->SetPosition(FurnitureMarker.Position);
                    panim->SpecialIdlePlay(apActor);
                    apActor->PickAnimations(1.0f, 1.0f);
                    return 1;
                }
            }
            if (!panim->SpecialIdleDonePlaying()) {
                if (!panim->GetSpecialIdleSequence()) {
                    if (panim->spAnimIdle.m_pObject) {
                        if (panim->spAnimIdle.m_pObject->eFlags == AnimIdle::ANIM_IDLE_DONE_PLAYING) {
                            apActor->GetOutofFurnitureQuick();
                            panim->SpecialIdleFree(1, 0);
                        }
                    }
                }
                return 1;
            }
            if (!apActor->GetSandman()) {
                if (GetFurnitureMarkerID() <= 20) {
                    pCurrentFurniture->SetMarkerReserved(cCurrentFurnitureIndex, 0);
                    uint32_t v8 = FurnitureMarker.GetType();
                    float a2b = static_cast<TESFurniture*>(pCurrentFurniture->GetObjectReference())
                                    ->TESFurniture::GetMarkerHeadingOffset(v8);
                    apActor->ModHeading(a2b);
                    panim->SkipNextBlend();
                    if (panim->spAnimIdle.m_pObject) {
                        ANIM_GROUP_SECTION Time = panim->spAnimIdle.m_pObject->eSection;
                        panim->BlendOut(Time, 0);
                    }
                    if (bsleeping) {
                        BSFaceGenAnimationData* panimData = apActor->GetFaceAnimationData();
                        if (panimData) {
                            panimData->SetDead(1, 0);
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
                    pCurrentFurniture->SetMarkerUsed(cCurrentFurnitureIndex, 0);
                    pCurrentFurniture->SetMarkerReserved(cCurrentFurnitureIndex, 0);
                    SetSitSleepState(apActor, SIT_SLEEP_NORMAL, 0, 127);
                    FurnitureMarker.SetHeading(0.0f);
                    FurnitureMarker.SetType(0);
                    FurnitureMarker.SetPosition(NiPoint3_ZERO);
                    pCurrentFurniture = 0;
                    apActor->SetChaseBip(0);
                    apActor->SetAnimAction(ANIM_ACTION_NONE, 0);
                    if (ppack) {
                        uint32_t Count = ppack->ePROCEDURE_TYPE;
                        if (Procedure_Action[Count][GetProcedureIndexRunning()] != PROCEDURE_PATROL) {
                            AddToProcedureIndexRunning(apActor, 1);
                        }
                    }
                    if (apActor == pPlayer) {
                        HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_RUN_AND_GUN);
                    }
                }
                panim->SpecialIdleFree(1, 0);
                return 1;
            }
            SetSitSleepState(apActor, SIT_SLEEP_NORMAL, 0, 127);
            apActor->SetChaseBip(0);
            apActor->SetAnimAction(ANIM_ACTION_NONE, 0);
            panim->SpecialIdleFree(1, 0);
            if (apActor == pPlayer) {
                HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_RUN_AND_GUN);
            }
            return 0;
        }
        default: {
            return 1;
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
    bool result;
    Animation* panim = apActor->GetAnimation();
    if (!panim) {
        return 0;
    }
    if (pPlayer->IsPipboyActive()) {
        return 0;
    }
    if (pCurrentFurniture) {
        uint32_t Type = FurnitureMarker.GetType();
        bool bsleeping = TESFurniture::IsSleepMarker(Type);
        if (cSitSleepState == 4 && bsleeping) {
            return 0;
        }
        else {
            if (cSitSleepState != 9 || bsleeping) {
                switch (cSitSleepState) {
                    case 3: {
                        if (GetFurnitureMarkerID() > 20) {
                            panim->SetLoopCount(AGS_MOVEMENT, 0);
                            if (GetCurrentFurniture()) {
                                uint32_t afWalkSpeed = GetCurrentFurnitureIndex();
                                TESObjectREFR* v4 = GetCurrentFurniture();
                                v4->SetMarkerUsed(afWalkSpeed, 0);
                            }
                            SetSitSleepState(
                                apActor, SIT_SLEEP_WANT_TO_STAND, pCurrentFurniture, cCurrentFurnitureIndex
                            );
                        }
                        goto LABEL_63;
                    }
                    case 4:
                    case 9: {
                        if (apActor->IsPc()) {
                            HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_SIT_SLEEP_ANIM);
                        }
                        if (panim->SpecialIdleWorking()) {
                            result = 1;
                        }
                        else {
                            TESIdleForm* pidle;
                            if (bsleeping) {
                                BSFaceGenAnimationData* panimData = apActor->GetFaceAnimationData();
                                if (panimData) {
                                    panimData->SetDead(0, 0);
                                }
                                SetSitSleepState(
                                    apActor, SIT_SLEEP_WANT_TO_WAKE, pCurrentFurniture, cCurrentFurnitureIndex
                                );
                            }
                            else {
                                SetSitSleepState(
                                    apActor, SIT_SLEEP_WANT_TO_STAND, pCurrentFurniture, cCurrentFurnitureIndex
                                );
                            }
                            pidle = pIdleManager->GetIdleToPlay(apActor, pCurrentFurniture);
                            if (pidle) {
                                ANIM_GROUP_SECTION IdleAnimGroupSection;
                                IdleAnimGroupSection = pidle->GetIdleAnimGroupSection();
                                panim->SpecialIdleQueue(pidle, IdleAnimGroupSection);
                            LABEL_63:;
                                result = 1;
                            }
                            else {
                                int32_t blendAmount = cCurrentFurnitureIndex;
                                const char* afWalkSpeeda = pCurrentFurniture->GetFormEditorID();
                                Warning(
                                    "AI: Missing furniture exit animation (index %d) for marker %d in the editor's "
                                    "idle manager. '%s' index %d.",
                                    cCurrentFurnitureIndex, FurnitureMarker.GetType(), afWalkSpeeda, blendAmount
                                );
                                pCurrentFurniture->SetMarkerUsed(cCurrentFurnitureIndex, 0);
                                panim->SkipNextBlend();
                                panim->ClearGroup(AGS_IDLE, 0.0f);
                                SetSitSleepState(apActor, SIT_SLEEP_NORMAL, pCurrentFurniture, 127);
                                uint32_t v5 = FurnitureMarker.GetType();
                                float blendAmounta =
                                    -static_cast<TESFurniture*>(pCurrentFurniture->GetObjectReference())
                                         ->TESFurniture::GetMarkerHeadingOffset(v5);
                                apActor->ModHeading(blendAmounta);
                                float fHeading = apActor->GetHeading(0) + 3.1415927410125732;
                                apActor->SetHeading(fHeading);
                                ClearFurniture(apActor);
                                apActor->SetChaseBip(0);
                                if (bsleeping) {
                                    bool abwearweapon = 1;
                                    TESPackage* ppack = GetPackageThatIsRunning();
                                    if (ppack) {
                                        abwearweapon = !ppack->GetForceUnequipWeapon();
                                    }
                                    if (apActor->IsNpc()) {
                                        TESNPC* pnpc = dynamic_cast<TESNPC*>(apActor->GetObjectReference());
                                        if (pnpc) {
                                            pnpc->InitDefaultWorn(apActor, 1, abwearweapon, 0, 1);
                                        }
                                    }
                                    else {
                                        TESCreature* pcrea = dynamic_cast<TESCreature*>(apActor->GetObjectReference());
                                        if (pcrea) {
                                            pcrea->InitDefaultWorn(apActor, 1, abwearweapon, 1);
                                        }
                                    }
                                }
                                float v15 = (apActor->GetFatigue() + 10.0) * -1.0;
                                apActor->DamageModActorValue(ActorValue::FATIGUE, v15, 0);
                                AddToProcedureIndexRunning(apActor, 1);
                                if (GetNeedTalkPlayer()) {
                                    apActor->InitiateDialogue(pPlayer, 0, 0, 1, 0, 0, 0, 0, 0);
                                }
                                result = 0;
                            }
                        }
                        break;
                    }
                    case 5:
                    case 10: {
                        BSAnimGroupSequence* CurrentSequence;
                        if (panim->SpecialIdleLoaded() &&
                            (!panim->GetCurrentSequence(AGS_MOVEMENT) ||
                             (CurrentSequence = panim->GetCurrentSequence(AGS_MOVEMENT),
                              CurrentSequence->GetState() == NiControllerSequence::kAnimating))) {
                            SIT_SLEEP_STATE ecurrentState = static_cast<SIT_SLEEP_STATE>(cSitSleepState);
                            SetSitSleepState(apActor, SIT_SLEEP_NORMAL, pCurrentFurniture, cCurrentFurnitureIndex);
                            uint32_t v8 = FurnitureMarker.GetType();
                            float blendAmountb = -static_cast<TESFurniture*>(pCurrentFurniture->GetObjectReference())
                                                      ->TESFurniture::GetMarkerHeadingOffset(v8);
                            apActor->ModHeading(blendAmountb);
                            SetSitSleepState(apActor, ecurrentState, pCurrentFurniture, cCurrentFurnitureIndex);
                            panim->SkipNextBlend();
                            panim->SpecialIdlePlay(apActor);
                        }
                        else {
                            if (panim->SpecialIdleDonePlaying()) {
                                uint32_t imarkerID = GetFurnitureMarkerID();
                                if (imarkerID <= 20) {
                                    panim->SkipNextBlend();
                                }
                                ClearFurniture(apActor);
                                if (apActor->IsPc()) {
                                    HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_RUN_AND_GUN);
                                }
                                if (imarkerID <= 20) {
                                    float v14 = apActor->GetHeading(0) + 3.1415927410125732;
                                    apActor->SetHeading(v14);
                                }
                                apActor->PickAnimations(1.0f, 1.0f);
                                apActor->SetChaseBip(0);
                                bhkCharacterController* pcon = GetCharController();
                                if (pcon) {
                                    pcon->SetPosition(*apActor->QLocationOnReference());
                                }
                                if (GetNeedTalkPlayer()) {
                                    apActor->InitiateDialogue(pPlayer, 0, 0, 1, 0, 0, 0, 0, 0);
                                }
                            }
                        }
                        goto LABEL_63;
                    }
                    default: {
                        if (bsleeping) {
                            cSitSleepState = 9;
                        }
                        else {
                            cSitSleepState = 4;
                        }
                        goto LABEL_63;
                    }
                }
            }
            else {
                return 0;
            }
        }
    }
    else {
        SetSitSleepState(apActor, SIT_SLEEP_NORMAL, 0, 127);
        AddToProcedureIndexRunning(apActor, 1);
        if (GetNeedTalkPlayer()) {
            apActor->InitiateDialogue(pPlayer, 0, 0, 1, 0, 0, 0, 0, 0);
        }
        return 0;
    }
    return result;
}

void MiddleHighProcess::ProcessTravel(
    Actor* apActor,
    bool abNewpackage,
    bool abInterateindex,
    int32_t aemovemode,
    bool abSecondLoc
) {
    TESObjectCELL* v24;
    TESWorldSpace* a2;
    NiPoint3 afRadius1;
    TESPackage* pPack = 0;
    pPack = GetPackageThatIsRunning();
    if (((!abNewpackage || pPack->IsActorAtLocation(apActor, 0, -1.0f, 0)) &&
         (!apActor->IsPathingComplete() ||
          (pPack->IsActorAtLocation(apActor, 0, -1.0f, 0) || DidPathingFail(apActor)))) ||
        (apActor->SetMoveMode(static_cast<uint32_t>(aemovemode)),
         (a2 = pPack->GetLocationWorld(apActor),
          (v24 = pPack->GetLocationCell(apActor),
           (afRadius1 = pPack->GetLocationCoord(apActor), SetupPathFinding(apActor, afRadius1, v24, a2)))))) {
        float fcurrentdistance = apActor->GetDistanceFromPoint(pPack->GetLocationCoord(apActor));
        float fdistance = pPack->GetPackageRadiusActorToLocation(apActor, 0);
        bool batRef = 0;
        TESObjectREFR* pRef = 0;
        if (pPack->pPackLoc) {
            pRef = pPack->pPackLoc->GetLocReference();
        }
        if (pGenericLocation) {
            if (!GetRunOncePackage()) {
                pRef = pGenericLocation;
            }
        }
        if (pPack->GetPackType() == PACKAGE_TYPE_WANDER) {
            fdistance = 20.0f;
        }
        if (pRef && (pRef->IsFurniture() && pPack->GetPackType() != PACKAGE_TYPE_WANDER)) {
            if (!pCurrentFurniture) {
                pCurrentFurniture = pRef;
                ClearFurnitureMarker();
            }
            if (apActor->GetSitSleepState()) {
                batRef = 1;
            LABEL_26:;
                if (batRef || DidPathingFail(apActor)) {
                    if (!GetLockedLocation()) {
                        if (pPack->GetLockDoorsAtLocation()) {
                            const NiPoint3* v22 = &apActor->GetLocationOnReference();
                            const NiPoint3* v20 = &apActor->GetLocationOnReference();
                            TESObjectCELL* ParentCell = apActor->GetParentCell();
                            pDataHandler->EnumReferencesCloseToPoint(
                                ParentCell, *v20, 5000.0f, *v22, 5000.0f, &LowProcess::LockDoorsatLocation, apActor
                            );
                        }
                        SetLockedLocation(1);
                    }
                    if (pPack->GetUnlockDoorsAtLocation()) {
                        const NiPoint3* v23 = &apActor->GetLocationOnReference();
                        const NiPoint3* v21 = &apActor->GetLocationOnReference();
                        TESObjectCELL* v19 = apActor->GetParentCell();
                        pDataHandler->EnumReferencesCloseToPoint(
                            v19, *v21, 5000.0f, *v23, 5000.0f, &LowProcess::UnLockDoorsatLocation, apActor
                        );
                    }
                    if (pPack) {
                        if (!bDoneClothesChange && pPack->GetForceUnequipWeapon()) {
                            bDoneClothesChange = 1;
                            ItemChange* pweapon = GetCurrentWeapon();
                            if (pweapon) {
                                apActor->QueueUnEquipObject(pweapon->pContainerObj, 1, 0, 0, 0, 1);
                            }
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
                                NiMatrix3 rot;
                                TESFurniture* pFurniture = dynamic_cast<TESFurniture*>(pRef->GetObjectReference());
                                FurnitureMark* pmarker = GetFurnitureMarker();
                                float a2a = pmarker->GetHeading();
                                apActor->SetHeading(a2a);
                                float a2b = apActor->GetScale();
                                uint32_t Type = pmarker->GetType();
                                NiPoint3 markerOffset_2 = pFurniture->GetMarkerTargetOffset(Type, a2b);
                                float a2c = pmarker->GetHeading();
                                rot.MakeZRotation(a2c);
                                markerOffset_2 = rot * markerOffset_2;
                                pRef->SetMarkerUsed(cCurrentFurnitureIndex, 1);
                                apActor->SetPosition(pmarker->Position + markerOffset_2);
                                uint32_t v9 = pmarker->GetType();
                                float a2d = pFurniture->GetMarkerHeadingOffset(v9);
                                apActor->ModHeading(a2d);
                                bool absleep = pPack->GetPackType() == PACKAGE_TYPE_SLEEP;
                                if (absleep) {
                                    SetSitSleepState(
                                        apActor, SIT_SLEEP_LOAD_SLEEP_IDLE, pCurrentFurniture, cCurrentFurnitureIndex
                                    );
                                    BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
                                    CurrentAIProcess->SetProcedureIndexRunning(1);
                                    BaseProcess* v32 = apActor->pCurrentProcess;
                                    v32->ProcessRemoveWorn(apActor, 0);
                                }
                                else {
                                    SetSitSleepState(
                                        apActor, SIT_SLEEP_LOAD_SIT_IDLE, pCurrentFurniture, cCurrentFurnitureIndex
                                    );
                                }
                                if (LoadDynamicIdle(apActor)) {
                                    if (absleep) {
                                        BSFaceGenAnimationData* panimData = apActor->GetFaceAnimationData();
                                        if (panimData) {
                                            panimData->SetDead(1, 0);
                                        }
                                        SetSitSleepState(
                                            apActor, SIT_SLEEP_IS_SLEEPING, pCurrentFurniture, cCurrentFurnitureIndex
                                        );
                                    }
                                    else {
                                        SetSitSleepState(
                                            apActor, SIT_SLEEP_IS_SITTING, pCurrentFurniture, cCurrentFurnitureIndex
                                        );
                                    }
                                }
                            }
                            else {
                                AlignToFurnitureAndAddPostAnimSitSleep(apActor);
                            }
                        }
                    }
                    else {
                        TESBoundObject* ObjectReference;
                        if ((pRef &&
                             (ObjectReference = pRef->GetObjectReference(), ObjectReference == pXMarkerHeading)) ||
                            (pPack->pPackLoc && pPack->pPackLoc->GetLocType() == PLT_EDITORLOC)) {
                            if (!DidPathingFail(apActor) && apActor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
                                float ftargetangle = 0.0f;
                                if (pRef) {
                                    ftargetangle = (&pRef->GetAngleOnReference())->z;
                                }
                                else {
                                    ftargetangle = apActor->GetStartingAngle().z;
                                }
                                apActor->SetHeading(ftargetangle);
                            }
                        }
                        EndMoveMessage(apActor);
                        if (abInterateindex && (!DidPathingFail(apActor) || !pPack->GetMustComplete())) {
                            AddToProcedureIndexRunning(apActor, 1);
                        }
                    }
                }
                else {
                    if (apActor) {
                        if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
                            apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                            apActor->InitiateGetUpPackage();
                        }
                        else {
                            float v44 = 0.0f;
                            Actor* plocactor = 0;
                            if (pRef && pRef->IsFurniture()) {
                                NiPoint3 point = apActor->GetLocationOnReference();
                                uint32_t inum = 0;
                                if (!pCurrentFurniture) {
                                    pCurrentFurniture = pRef;
                                    ClearFurnitureMarker();
                                }
                                if (pCurrentFurniture->GetClosestFreeMarker(point, 1, 1, FurnitureMarker, inum, 1)) {
                                    TESWorldSpace* a2e = pPack->GetLocationWorld(apActor);
                                    TESObjectCELL* LocationCell = pPack->GetLocationCell(apActor);
                                    if (SetupPathFinding(apActor, FurnitureMarker.Position, LocationCell, a2e)) {
                                        pCurrentFurniture = pRef;
                                        cCurrentFurnitureIndex = inum;
                                    }
                                }
                                else {
                                    EndMoveMessage(apActor);
                                    PACKAGE_PROCEDURE_TYPE Count = pPack->ePROCEDURE_TYPE;
                                    int32_t ProcedureCount = Procedure::GetProcedureCount(Count);
                                    SetProcedureIndexRunning(ProcedureCount - 1);
                                }
                            }
                            else {
                                if (pPack->pPackLoc) {
                                    if (pPack->pPackLoc->GetLocReference()) {
                                        TESObjectREFR* LocReference = pPack->pPackLoc->GetLocReference();
                                        plocactor = dynamic_cast<Actor*>(LocReference);
                                    }
                                }
                                if (plocactor) {
                                    const NiPoint3* v15 = &plocactor->GetLocationOnReference();
                                    float fdistancemoved = (lastSeenPostion - *v15).Length();
                                    float* v17 = &fAIMoveDistanceToRecalcFollowPath.Float();
                                    if (*v17 < static_cast<double>(fdistancemoved) ||
                                        (fdistance < static_cast<double>(fcurrentdistance) &&
                                         apActor->IsPathingComplete())) {
                                        TESWorldSpace* pworld = plocactor->GetWorldSpace();
                                        TESObjectCELL* pcell = plocactor->GetParentCell();
                                        if (SetupPathFinding(
                                                apActor, plocactor->GetLocationOnReference(), pcell, pworld
                                            )) {
                                            lastSeenPostion = plocactor->GetLocationOnReference();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                return;
            }
            if (apActor->GetSitSleepState() == SIT_SLEEP_NORMAL &&
                (pCurrentFurniture->GetMarkerUsed(cCurrentFurnitureIndex, 1) && !apActor->IsPathingComplete())) {
                ClearFurniture(apActor);
                AddToProcedureIndexRunning(apActor, 1);
                return;
            }
        }
        batRef = pPack->IsActorAtLocation(apActor, 0, -1.0f, 0);
        goto LABEL_26;
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
        return 0;
    }
    TESPackage* ppack = GetCurrentPackage();
    if (!abDontCheckForAmbush &&
        (ppack &&
         (ppack->GetPackType() == PACKAGE_TYPE_AMBUSH &&
          !ppack->IsActorAtSecondLocation(apTarget, apActor, 0, -1.0f, 0)))) {
        return 0;
    }
    if (!apTarget && !apGroup) {
        apActor->StartCombat(0, 0, 0, 0, 0, 0, 0, apPackage);
        return 1;
    }
    if (apTarget) {
        PROCESS_TYPE CurrentProcessType = apTarget->GetCurrentProcessType();
        if (CurrentProcessType != apActor->GetCurrentProcessType() && !apActor->Get3D()) {
            LowProcess::EnterCombat(
                apActor, apTarget, abDontCheckForAmbush, absendalarm, apGroup, abfleeing, abForceFlee, abIsAggressor,
                acTeam, bIgnoreDefensivePackage, 0, 1, 0
            );
        }
    }
    if (apActor->IsRestrained() || apActor->IsUnConscious()) {
        return 0;
    }
    if (!apTarget ||
        (apActor->GetCombatController() || (bforcecombat || !pCombatManager->CheckShouldFlee(apActor, apTarget)))) {
        if (apActor->GetCombatController()) {
            if (apTarget) {
                if (!apGroup) {
                    CombatController* v16 = apActor->GetCombatController();
                    if (v16->IsActoraCombatTarget(apTarget)) {
                        return 0;
                    }
                }
            }
        }
        if (apTarget == pPlayer && !bforcecombat) {
            int32_t NumberActorsInCombat = pPlayer->GetNumberActorsInCombat();
            if (NumberActorsInCombat >= iNumberActorsInCombatPlayer.Int()) {
                return 0;
            }
        }
        if (apTarget) {
            bool btargetSwimming = apTarget->IsSwimming();
            if (!apActor->FightsInWater() && btargetSwimming) {
                return 0;
            }
            if (apActor->SwimsOnly() && !btargetSwimming) {
                return 0;
            }
        }
        ClearMovementStoped();
        TESPackage* pPack = GetPackageThatIsRunning();
        if (pPack) {
            if (pPack->IsInterruptPackage()) {
                pPack = (&apActor->m_Extra)->GetPackageExtra();
            }
        }
        if (apTarget) {
            if (pPack) {
                if (pPack->GetDefensive() && !bforcecombat && !bIgnoreDefensivePackage) {
                    NiPoint3 dest = GetCurrentDestinationCoordinate(apActor, 0);
                    // The package's radius, 600 when below 1.
                    float fdistancepack = static_cast<float>(pPack->GetLocationRadius(apActor));
                    if (fdistancepack < 1.0) {
                        fdistancepack = 600.0f;
                    }
                    double DistanceFromPoint = apTarget->GetDistanceFromPoint(dest);
                    if (fdistancepack < DistanceFromPoint) {
                        if (apTarget->GetCombatController()) {
                            CombatController* v19 = apTarget->GetCombatController();
                            if (!v19->IsActoraCombatTarget(apActor)) {
                                SetDetectAlert(1);
                                return 0;
                            }
                        }
                    }
                }
            }
        }
        if (!apActor->GetCombatController()) {
            SetDetectAlert(0);
            apActor->EndDialogue();
            pSavedAcquireObject = pAcquireObject;
            pAcquireObject = 0;
        }
        bool baggressor = abIsAggressor;
        if (apActor->IsActoraGuard()) {
            baggressor = 0;
        }
        if (apTarget) {
            if (apTarget->IsInCombat()) {
                if (apTarget->GetCombatController()) {
                    CombatController* v20 = apTarget->GetCombatController();
                    if (v20->IsActoraCombatTarget(apActor)) {
                        baggressor = 0;
                    }
                }
            }
        }
        SetIsAggressor(baggressor);
        int32_t iPriority = 0;
        if (bforcecombat) {
            iPriority = 200;
        }
        apActor->StartCombat(apTarget, apGroup, bforcecombat, baggressor, abfleeing, iPriority, abCastSpell, apPackage);
        if (apTarget == pPlayer && apActor->IsInCombat() && pPlayer->GetDetectionLevel(apActor) > 0) {
            pPlayer->AddActorToPlayerCombatList(apActor);
        }
        return 1;
    }
    apActor->InitiateFlee(apTarget, 1, 1, 1, 0, 0, -1.0f, -1.0f);
    if (apTarget == pPlayer && pPlayer->GetDetectionLevel(apActor) > 0) {
        pPlayer->AddActorToPlayerCombatList(apActor);
    }
    return 1;
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
    if ((!pAnimation) || pAnimation->GetAnimsLoading()) {
        return (bool)1;
    }
    uint32_t uiBit = (uint32_t)((2 * aeMovementAnim) - 6);
    if (uiBit > 32) {
        return (bool)1;
    }
    uint32_t uiFlags = iMovementAnimations >> uiBit;
    if ((uiFlags & 1) != 0) {
        return (bool)1;
    }
    if ((uiFlags & 2) != 0) {
        return (bool)0;
    }
    uint8_t result = (uint8_t)pAnimation->GroupLoaded(aeMovementAnim);
    iMovementAnimations |= ((int32_t)((2 - result) << uiBit));
    return (bool)result;
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
        TaskQueueInterface::GetInstance()->QueueActorKnockExplosion(
            apActor, aSourceLocation.x, aSourceLocation.y, aSourceLocation.z, afMagnitude
        );
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
        float* pMult = &fMagicExplosionIncorporealMult.Float();
        float fScale = afMagnitude / fMagicExplosionPowerMax.Float() * *pMult;
        kVelocity *= fScale;
        float* pTime = &fMagicExplosionIncorporealTime.Float();
        float fTime = afMagnitude / fMagicExplosionPowerMax.Float() * *pTime;
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
    NiUpdateData kData;
    kData.~NiUpdateData();
    ::new (&kData) NiUpdateData(0.0f, false, false);
    p3D->Update(kData);
    hkVector4 kLocation;
    NI2HK(kLocation, aSourceLocation);
    TESHavokUtilities::AddExplosionImpulse(p3D, kLocation, afMagnitude, nullptr);
}

void MiddleHighProcess::UpdateKnockState(Actor* apActor) {
    NiPoint3 dr;
    Animation* panim = apActor->GetAnimation();
    if (panim) {
        if (apActor != pPlayer || panim != pPlayer->GetAnimation(static_cast<bool>(1))) {
            if (apActor->CanKnockDown()) {
                bool bunconscious = 0;
                if (GetCachedFlag(CachedValues::CVF_FATIGUE_DAMAGED)) {
                    bunconscious = apActor->GetFatigue() < 0.0;
                }
                if ((bunconscious ||
                     (apActor->IsParalyzed() || apActor->GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN)) &&
                    !cKnockState) {
                    if (apActor->GetSitSleepState()) {
                        if (apActor->GetSitSleepState() != SIT_SLEEP_WANT_TO_STAND) {
                            apActor->GetOutofFurnitureQuick();
                        }
                    }
                    apActor->InterruptCast();
                    ClearMuzzleFlash();
                    apActor->AddChange(4);
                    if (apActor->IsParalyzed()) {
                        cKnockState = 3;
                        if (apActor->pRagdollController) {
                            apActor->pRagdollController->EnableRagdollFeedback(0);
                            apActor->pRagdollController->SyncRagdollTo3D();
                            apActor->pRagdollController->DisableRagdollAnim(0);
                        }
                        NiNode* v2 = apActor->TESObjectREFR::Get3D();
                        bhkWorld::SetMotion(v2, hkpMotion::MOTION_DYNAMIC, 1, 0, 1);
                        NiAVObject* pav = apActor->TESObjectREFR::Get3D();
                        bhkRigidBody::SetFixed(pav, 1, 1);
                        panim->ClearGroup(AGS_WHOLE_BODY, 0.0f);
                        panim->ZeroGlobalTransform();
                        EndMoveMessage(apActor);
                    }
                    else {
                        if (apActor->IsPlayerTeammate()) {
                            FollowerBarks::TriggerFollowerBark(apActor, FollowerBarks::BARK_KNOCKED_OUT);
                        }
                        cKnockState = 4;
                        panim->GetDelta(&dr, apActor, 1, 0);
                        float v20 = appTimer.fDelta * 50.0;
                        dr = -dr / v20;
                        if (apActor->pRagdollController) {
                            apActor->pRagdollController->EnableRagdollFeedback(0);
                            apActor->pRagdollController->DisableRagdollAnim(1);
                        }
                        apActor->SetHavokWeapon();
                        NiNode* v5 = apActor->TESObjectREFR::Get3D();
                        bhkBlendController::DoKnockDown(v5, dr, 0, 0.0f, 1);
                        NiAVObject* pav_2 = apActor->TESObjectREFR::Get3D();
                        bhkRigidBody::SetFixed(pav_2, 0, 1);
                    }
                }
                switch (cKnockState) {
                    case 1: {
                        double fdif_2;
                        float ftime = fHourLastProcessed;
                        float fcurrent = appCalendar.GetHour();
                        float fdif = 0.0f;
                        if (!(ftime > static_cast<double>(fcurrent))) {
                            fdif_2 = fcurrent - ftime;
                        }
                        else {
                            fdif_2 = ftime + 23.0 - fcurrent;
                        }
                        fdif = fdif_2;
                        double TimeScale = appCalendar.GetTimeScale();
                        bool bmin = fdif > TimeScale * 0.0006600000197067857;
                        double v10 = appCalendar.GetTimeScale();
                        bool bmax = fdif > v10 * 0.001500000013038516;
                        if (apActor->GetFatigue() >= 0.0 && !apActor->IsParalyzed() && bmin &&
                            (bmax || apActor->IsNonAccumNearController(16.0f))) {
                            char cKnockStateSave = cKnockState;
                            cKnockState = 3;
                            TESIdleForm* pidle_2 = pIdleManager->GetIdleToPlay(apActor, 0);
                            cKnockState = cKnockStateSave;
                            BSFaceGenAnimationData* panimData_4 = apActor->GetFaceAnimationData();
                            if (panimData_4) {
                                panimData_4->SetDead(0, 0);
                            }
                            cKnockState = 5;
                            if (pidle_2) {
                                panim->SpecialIdleQueue(pidle_2, AGS_WHOLE_BODY);
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
                        NiNode* proot = panim->pAccumRoot;
                        if (!proot || bhkUtilFunctions::GetHeirGain(proot) <= 0.0) {
                            apActor->StopCurrentDialogue();
                            if (apActor == pPlayer && pPlayer->GetIronSights()) {
                                pPlayer->SetIronSights(0, 0, 0);
                            }
                            if (cKnockState == 4) {
                                cKnockState = 3;
                            }
                            else {
                                cKnockState = 1;
                            }
                            BSFaceGenAnimationData* panimData = apActor->GetFaceAnimationData();
                            if (panimData) {
                                panimData->SetDead(1, 0);
                            }
                            panim->ClearGroup(AGS_WHOLE_BODY, 0.0f);
                            panim->ZeroGlobalTransform();
                            EndMoveMessage(apActor);
                        }
                        break;
                    }
                    case 3: {
                        if (!(apActor->GetFatigue() >= 0.0) ||
                            (apActor->IsParalyzed() || apActor->GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN)) {}
                        else {
                            TESIdleForm* pidle = pIdleManager->GetIdleToPlay(apActor, 0);
                            if (pidle) {
                                panim->SpecialIdleQueue(pidle, AGS_WHOLE_BODY);
                                BSFaceGenAnimationData* panimData_2 = apActor->GetFaceAnimationData();
                                if (panimData_2) {
                                    panimData_2->SetDead(0, 0);
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
                        BSAnimGroupSequence* CurrentSequence;
                        if (panim->SpecialIdleLoaded() &&
                            (!panim->GetCurrentSequence(AGS_MOVEMENT) ||
                             (CurrentSequence = panim->GetCurrentSequence(AGS_MOVEMENT),
                              CurrentSequence->GetState() == NiControllerSequence::kAnimating))) {
                            if (apActor->IsSwimming() || !panim->SpecialIdlePlay(apActor)) {
                                panim->SpecialIdleFree(1, 0);
                            }
                            NiAVObject* pav_3 = apActor->TESObjectREFR::Get3D();
                            NiAVObject* pspine = panim->pAccumRoot;
                            if (pspine) {
                                pspine = static_cast<NiNode*>(pspine)->NiNode::GetAtChecked(0);
                                NiFixedString* Bip01_Head = FixedStrings::GetBip01_Head();
                                NiAVObject* phead = apActor->GetActorBone(pspine, *Bip01_Head);
                                if (!phead) {
                                    NiFixedString* v13 = &FixedStrings::QBip01Neck();
                                    phead = BSUtilities::GetObjectByName(pspine, *v13);
                                }
                                if (!phead) {
                                    NiFixedString* v14 = &FixedStrings::QBip01Spine1();
                                    phead = BSUtilities::GetObjectByName(pspine, *v14);
                                }
                                if (phead) {
                                    NiPoint3 movedir;
                                    const NiPoint3* a2 = &pspine->GetWorldTranslate();
                                    movedir = phead->GetWorldTranslate() - *a2;
                                    bhkCharacterController* v15 = GetCharController();
                                    if (!v15->QQuadruped()) {
                                        NiFixedString* Bip01_Spine = FixedStrings::GetBip01_Spine();
                                        NiAVObject* p180Test = apActor->GetActorBone(pav_3, *Bip01_Spine);
                                        if (p180Test) {
                                            if (bhkNiCollisionObject::GetFaceUp(static_cast<NiNode*>(p180Test), 0)) {
                                                movedir.operator*=(-1.0f);
                                            }
                                        }
                                    }
                                    float ftargetAngle = GetZAngleFromVector(movedir);
                                    cKnockState = 0;
                                    apActor->SetHeading(ftargetAngle);
                                }
                            }
                            bhkRigidBody::SetFixed(pav_3, 0, 0);
                            bhkBlendController::DoGetUp(pav_3, 0.0f);
                            apActor->RemoveChange(4);
                            (&apActor->m_Extra)->SetRagDollData(0);
                            cKnockState = 6;
                        }
                        else {
                            if (!panim->spAnimIdle.m_pObject && !panim->spAnimIdleQueued.m_pObject) {
                                cKnockState = 3;
                            }
                        }
                        break;
                    }
                    case 6: {
                        if (panim->SpecialIdleDonePlaying()) {
                            cKnockState = 0;
                            panim->SpecialIdleFree(1, 0);
                            if (apActor->pRagdollController) {
                                apActor->pRagdollController->EnableRagdollFeedback(1);
                                apActor->pRagdollController->EnableRagdollAnim();
                            }
                            apActor->SetHavokWeapon();
                            if (pWeapon) {
                                if (pWeaponBone) {
                                    if (bWeaponGrenade || bWeaponThrown) {
                                        if (pWeaponBone->GetAtChecked(0)) {
                                            NiAVObject* v17 = pWeaponBone->GetAt(0);
                                            v17->SetAppCulled(0);
                                            if (apActor == pPlayer) {
                                                if (MiddleHighProcess::pThirdPersonPlayerWeaponNode) {
                                                    if (MiddleHighProcess::pThirdPersonPlayerWeaponNode->GetAtChecked(
                                                            0
                                                        )) {
                                                        NiAVObject* v18 =
                                                            MiddleHighProcess::pThirdPersonPlayerWeaponNode->GetAt(0);
                                                        v18->SetAppCulled(0);
                                                    }
                                                }
                                            }
                                        }
                                    }
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
}

void MiddleHighProcess::ClearKnockState(Actor* apActor) {
    if (cKnockState) {
        if (!(apActor->GetFatigue() < 0.0) && !apActor->IsParalyzed()) {
            NiNode* apObject = apActor->TESObjectREFR::Get3D();
            bhkRigidBody::SetFixed(apObject, 0, 1);
            bhkBlendController::DoGetUp(apObject, 1.0);
            ExtraDataList* Extra = apActor->GetExtra();
            Extra->SetRagDollData(0);
            apActor->RemoveChange(4);
            BSFaceGenAnimationData* FaceAnimationData = apActor->GetFaceAnimationData();
            if (FaceAnimationData) {
                FaceAnimationData->SetDead(0, 0);
            }
            cKnockState = 0;
            if (pWeapon) {
                if (pWeaponBone) {
                    if (bWeaponGrenade || bWeaponThrown) {
                        if (pWeaponBone->GetAtChecked(0)) {
                            NiAVObject* v3 = pWeaponBone->GetAt(0);
                            v3->SetAppCulled(0);
                            if (apActor == pPlayer) {
                                if (MiddleHighProcess::pThirdPersonPlayerWeaponNode) {
                                    if (MiddleHighProcess::pThirdPersonPlayerWeaponNode->GetAtChecked(0)) {
                                        NiAVObject* v4 = MiddleHighProcess::pThirdPersonPlayerWeaponNode->GetAt(0);
                                        v4->SetAppCulled(0);
                                    }
                                }
                            }
                        }
                    }
                }
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
    float v10 = 0.0;
    float v9 = 0.0;
    double v7 = appTimer.GetSeconds() * fAimLooking;
    double v6 = fAimChaseLookingMult.Float() * v7;
    float v13 = fAimLooking - fAnimationMult.Float() * v6;
    if (!(fAimLooking > 0.0) || !(v13 < 0.0)) {
        if (!(fAimLooking < 0.0) || !(v13 > 0.0)) {
            if (v13 == 0.0 || (!(v13 > -9.999999974752427e-07) || !(v13 < 9.999999974752427e-07))) {
                fAimLooking = v13;
            }
            else {
                fAimLooking = 0.0;
            }
        }
        else {
            fAimLooking = 0.0;
        }
    }
    else {
        fAimLooking = 0.0;
    }
    float v11 = apActor->GetLooking() + fAimLooking;
    if (v11 < 0.0 && pAimSequence[1]) {
        float v5 = v11 / (fAimUpDegrees.Float() * -0.01745329238474369);
        v10 = BSMIN(static_cast<float>(1.0), v5);
    }
    else {
        if (v11 > 0.0) {
            if (pAimSequence[2]) {
                float v4 = v11 / (fAimDownDegrees.Float() * 0.01745329238474369);
                v9 = BSMIN(static_cast<float>(1.0), v4);
            }
        }
    }
    float v3 = static_cast<float>(0.0) + static_cast<float>(0.0);
    float v2 = v10 + v9;
    float v12 = 1.0 - BSMAX(v2, v3);
    if (pAimSequence[0]) {
        pAimSequence[0]->SetSequenceWeight(v12);
    }
    if (pAimSequence[1]) {
        pAimSequence[1]->SetSequenceWeight(v10);
    }
    if (pAimSequence[2]) {
        pAimSequence[2]->SetSequenceWeight(v9);
    }
}

float MiddleHighProcess::GetAimLooking() {
    return fAimLooking;
}

void MiddleHighProcess::AddGunDrift(float afDegrees) {
    fAimLooking = (float)(fAimLooking - (afDegrees * 0.01745329238474369));
}

void MiddleHighProcess::ClearAutomaticShotsToFire(bool abStartShooting) {
    if (pWeapon && pAmmo) {
        TESObjectWEAP* pweapon = static_cast<TESObjectWEAP*>(pWeapon->pContainerObj);
        if (pweapon->IsAutomaticWeapon() && abStartShooting) {
            double fWeapFireRate = pweapon->data.fShotsPerSec;
            bool HasModEffectActive_0 = pWeapon->HasModEffectActive(8);
            float fSPS = pweapon->GetAttackSpeed(HasModEffectActive_0) * fWeapFireRate;
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
    TESObjectWEAP* pweapon = static_cast<TESObjectWEAP*>(pWeapon->pContainerObj);
    if (!pweapon->IsAutomaticWeapon()) {
        return 0;
    }
    double Seconds = appTimer.fDelta;
    float fModifiedTime = fAnimationMult.Float() * Seconds;
    if (GetAnimation()) {
        Animation* v3 = GetAnimation();
        fCharacterTime = v3->GetGlobalTimeMultiplier();
    }
    else {
        fCharacterTime = 1.0f;
    }
    if (apActor == pPlayer) {
        fAutomaticShotDelay = fAutomaticShotDelay - pVATS.GetPlayerUpdateMult() * fModifiedTime * fCharacterTime;
    }
    else {
        if (apActor == pVATSTarget) {
            double v8 = appTimer.fDelta;
            double v7 = fAnimationMult.Float() * v8 * fCharacterTime;
            fAutomaticShotDelay = fAutomaticShotDelay - pVATS.GetTargetUpdateMult() * v7;
        }
        else {
            double v6 = appTimer.fDelta;
            fAutomaticShotDelay = fAutomaticShotDelay - fAnimationMult.Float() * v6 * fCharacterTime;
        }
    }
    double TurnSpeed = pweapon->data.fShotsPerSec;
    bool HasModEffectActive_0 = pWeapon->HasModEffectActive(8);
    float fSPS = pweapon->GetAttackSpeed(HasModEffectActive_0) * TurnSpeed;
    uint8_t cshots = 0;
    while (fAutomaticShotDelay < 0.0) {
        float foneShot = 1.0 / fSPS;
        fAutomaticShotDelay = fAutomaticShotDelay + foneShot;
        ++cshots;
    }
    return cshots;
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
    cUpdate3DModel = static_cast<uint8_t>(cUpdate3DModel | aeFlag);
}

void MiddleHighProcess::Clear3DUpdateFlag(RESET_3D_FLAGS aeFlag) {
    cUpdate3DModel *= (~((int8_t)aeFlag));
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
            if (aeState == SIT_SLEEP_WANT_TO_SIT || aeState == SIT_SLEEP_WANT_TO_SLEEP ||
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
        pCurrentFurniture = 0;
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
            pFirstPersonAnimation->SpecialIdleReplace(
                pFirstPersonIdle, apActor, pFirstPersonIdle->GetIdleAnimGroupSection()
            );
        }
    }
    pPlayer->Force3rdPerson(true);
    return true;
}

void MiddleHighProcess::LoadSitSleepIdle(Actor* apActor, Animation* apAnimation) {
    if (pCurrentFurniture) {
        uint32_t cCurrentFurnitureIndex;
        NiNode* v3 = pCurrentFurniture->Get3D();
        BSFurnitureMarker* pmarkdata = BSFurnitureMarker::Find(v3);
        if (pmarkdata &&
            (cCurrentFurnitureIndex = this->cCurrentFurnitureIndex, cCurrentFurnitureIndex < pmarkdata->QNumMarks())) {
            char crestoreSitSleepState = cSitSleepState;
            switch (crestoreSitSleepState) {
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
                pCurrentFurniture->SetMarkerUsed(this->cCurrentFurnitureIndex, 1);
                bool b3rdPerson = 0;
                if (apActor == pPlayer) {
                    b3rdPerson = pPlayer->Is3rdPerson();
                    if (apAnimation != pPlayer->GetAnimation(!b3rdPerson)) {
                        pPlayer->Force3rdPerson(!b3rdPerson);
                    }
                }
                TESIdleForm* pidle = pIdleManager->GetIdleToPlay(apActor, pCurrentFurniture);
                if (pidle) {
                    ANIM_GROUP_SECTION IdleAnimGroupSection = pidle->GetIdleAnimGroupSection();
                    apAnimation->SpecialIdleReplace(pidle, apActor, IdleAnimGroupSection);
                }
                if (apActor == pPlayer) {
                    pPlayer->Force3rdPerson(b3rdPerson);
                }
            }
            cSitSleepState = crestoreSitSleepState;
            uint32_t Type = FurnitureMarker.GetType();
            bool bdisableCharController = ShouldDisableCharController(pCurrentFurniture, Type);
            bhkCharacterController* pcon = GetCharController();
            if (bdisableCharController && cSitSleepState) {
                apActor->SetSize(0.1f);
                apActor->SetChaseBip(1);
                if (pcon) {
                    pcon->SetFlag(bhkCharacterController::NO_CHARACTER_COLLISIONS, 1);
                }
            }
            else {
                apActor->SetSize(0.0f);
                apActor->SetChaseBip(0);
                if (pcon) {
                    pcon->SetFlag(bhkCharacterController::NO_CHARACTER_COLLISIONS, 0);
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
    float flight = 0.0f;
    if (!(GetLightLevelTimer() <= 0.0)) {
        // Keeps the timer as a double and subtracts the BSTimer delta.
        double fTimer = GetLightLevelTimer();
        float fNewTimer = fTimer - appTimer.fDelta;
        SetLightLevelTimer(fNewTimer);
    }
    else {
        if (pLightingProperty.m_pObject) {
            NiPointer<NiLight> plight(static_cast<NiLight*>(0));
            float flightpoint = 0.0f;
            float fambientpoint = 0.0f;
            float fdirpoint = 0.0f;
            NiTListIterator kIter = nullptr;
            ShadowSceneLight* pshadowlight = pLightingProperty.m_pObject->GetNextLight(kIter);
            NiLight* ptorch = nullptr;
            if (abIgnoreTorch) {
                LightRefData* prefLight = apActor->GetLight();
                if (prefLight) {
                    ptorch = prefLight->spLight.m_pObject;
                }
            }
            if (!pshadowlight && apActor == pPlayer) {
                // Fetches the first-person 3D and does not use it, as the game does.
                pPlayer->Get3D(true);
                pshadowlight = pLightingProperty.m_pObject->GetNextLight(kIter);
            }
            ShadowSceneNode* pshadowNode = BSShaderManager::GetShadowSceneNode(0);
            if (pshadowNode) {
                ShadowSceneLight* psunlight = pshadowNode->pSunLight;
                int32_t inumberlights = 0;
                NiPoint3 point = apActor->GetLocationOnReference();
                NiPoint3 kBoundMax = apActor->GetBoundMax();
                float fTargetHeight = kBoundMax.z - apActor->GetBoundMin().z - 1.0;
                fTargetHeight = fTargetHeight / 2.0;
                point.z = point.z + fTargetHeight;
                if (apActor == pPlayer) {
                    flightpoint =
                        pshadowNode->GetLuminanceAtPoint(point, inumberlights, fambientpoint, fdirpoint, ptorch);
                }
                else {
                    while (pshadowlight) {
                        flightpoint = pshadowlight->GetLuminanceAtPoint(point, ptorch) + flightpoint;
                        pshadowlight = static_cast<ShadowSceneLight*>(GetNext(kIter));
                        ++inumberlights;
                    }
                    if (psunlight) {
                        NiColor kColor = psunlight->GetLight()->m_kAmb;
                        float fAmbientLum = BSMAX(kColor.r, BSMAX(kColor.g, kColor.b));
                        kColor = psunlight->GetLight()->m_kDiff;
                        float fDirectionalLum = BSMAX(kColor.r, BSMAX(kColor.g, kColor.b));
                        fambientpoint = fAmbientLum + fDirectionalLum;
                    }
                }
                if (apActor->GetParentCell() && apActor->GetParentCell()->GetInterior()) {
                    flightpoint = flightpoint + fambientpoint;
                }
                else if (psunlight) {
                    flightpoint = psunlight->GetLuminanceAtPoint(point, nullptr) + flightpoint;
                }
                flight = flightpoint * 100.0;
            }
            else {
                NiNode* p3D = apActor->TESObjectREFR::Get3D();
                apActor->InitLightingPropertyPtr(p3D);
                flight = 0.0f;
            }
        }
        if (!(flight > 100.0)) {
            if (flight < 0.0) {
                flight = 0.0f;
            }
        }
        else {
            flight = 100.0f;
        }
        SetActorLightLevel(flight);
        float fTimer = flight * 0.01 + 3.0;
        SetLightLevelTimer(fTimer);
    }
}

void MiddleHighProcess::ClearCurrentDataforProcess(Actor* apActor) {
    iAnimActionSuccess = 2;
    SetCurrentActionComplete(0);
    SetCurrentMovementComplete(1);
    SetTarget(0);
    ClearMovementStoped();
    ClearReservation();
    SetPackageIdleDone(0);
    pCurrentPackageSpell = 0;
    if (pAcquireObject) {
        pAcquireObject->Delete(1u);
    }
    pAcquireObject = 0;
    apActor->ClearLookAtTarget();
    apActor->SetForceSneak(0);
    ClearAutomaticShotsToFire(0);
    bDoneClothesChange = 0;
    while (!ObjectList.IsEmpty()) {
        ObjectstoAcquire* pobj = ObjectList.m_item;
        if (pobj) {
            pobj->Delete(1u);
        }
        ObjectList.Remove(pobj);
    }
    pCurrentIdle = 0;
    bPlayedBeginIdles = 0;
    bPlayedEndIdles = 0;
    RefListChairBed.RemoveAll();
    GenericLocationsList.RemoveAll();
    pGenericLocation = 0;
    if (apActor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
        if (GetCurrentFurniture()) {
            ClearFurniture(apActor);
        }
    }
    ClearUseWeaponHeadTrackTarget(1);
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
        FurnitureMarker.Position.x = apMarker->Position.x;
        FurnitureMarker.Position.y = apMarker->Position.y;
        FurnitureMarker.Position.z = apMarker->Position.z;
        FurnitureMarker.sHeading = apMarker->sHeading;
        if (pCurrentFurniture) {
            if (!pSaveLoadGame->GetSaveGameLoading()) {
                pCurrentFurniture->SetMarkerReserved(aiindex, 1);
            }
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
    FNVD_DYNAMIC_CAST_VOID(Actor)(apMob, RTTI_MobileObject, RTTI_Actor);
}

uint16_t MiddleHighProcess::GetSaveSize(uint32_t aiFlags, MobileObject* pmob) {
    Actor* pactor = dynamic_cast<Actor*>(pmob);
    uint16_t SaveSize = MiddleLowProcess::GetSaveSize(aiFlags, pmob);
    uint16_t v7 = SaveSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        SaveSize += 6;
    }
    int16_t v9 = SaveSize + 2;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 52u) {
        v9 += 5;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 77u) {
        v9 += 4;
    }
    int16_t v10 = v9 + 49;
    if ((aiFlags & CHANGE_REFR_ANIMATION) != 0) {
        v10 += ComputeAnimationSaveSize(static_cast<PlayerCharacter*>(pmob), pAnimation);
    }
    uint16_t isize = ActiveEffect::GetActiveEffectListSaveSize(pActiveEffectList, pactor) + (v10 + 8);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 69u) {
        ++isize;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 73u) {
        isize += 2;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 101u) {
        isize += 6 + 4 * DeadDetectList.ItemsInList();
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 109u) {
        ++isize;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        isize += 4;
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", isize - v7,
                pheader->iFormID, pform->GetFormEditorID(), pheader->iFlags, 0x1B89,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", isize - v7, 0x1B89,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp"
            );
        }
    }
    return isize;
}

// Saves the BGS-format data of the process: the low process's own, then the weapon, the furniture, the magic, the
// dead-detect list and the queued items. The animation is written as a variable-sized value between two buffer
// positions, and only when the form's change flags ask for it.
void MiddleHighProcess::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    Actor* pActor = apSaveGameBuffer->GetActor();
    MiddleLowProcess::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bWantWeaponDrawn), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bWeaponDrawn), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bCheckMagicNode), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fAlphaMult), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fScriptRefractPower), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eUseItem), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bBeenAttacked), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&lastSeenPostion), 0xC, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fEquippedWeight), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cSitSleepState), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cCurrentFurnitureIndex), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&FurnitureMarker.cNumber), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&FurnitureMarker.sHeading), 2, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&FurnitureMarker.Position), 0xC, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cKnockState), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bDoneOnce), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bPickPocketed), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bDoneClothesChange), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fPursueTimer), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bSummonedCreature), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fAimLooking), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fAutomaticShotDelay), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cShotsToFire), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bSustainedFire), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bIronSights), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iAnimActionSuccess), 2, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iReservationSlot), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bAimingTarget), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iPackageIdleNumber), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fPackageIdleTimer), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bPlayedBeginIdles), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bPlayedEndIdles), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fRadiationDelta), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fRadiationMagicDelta), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fRadiationWaterDelta), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iWeaponConditionStage), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bPickPackIdle), 1, 0);
    apSaveGameBuffer->SaveFormID(pLastIdlePlayed, 0);
    apSaveGameBuffer->SaveFormID(pCurrentIdle, 0);
    apSaveGameBuffer->SaveFormID(pCommandingActor, 0);
    apSaveGameBuffer->SaveFormID(pCurrentFurniture, 0);
    int32_t iAmmoCount = 0;
    if (pAmmo) {
        iAmmoCount = pAmmo->GetCount();
    }
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iAmmoCount), 4, 0);
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
    if (pActor) {
        if (apSaveGameBuffer->GetChangeFlags().CheckFlags(0x10000000)) {
            uint32_t uiAnimPosition = apSaveGameBuffer->StartVariableSizedValue();
            uint32_t uiAnimStart = apSaveGameBuffer->GetBufferPosition();
            pAnimation->SaveGameBGS(apSaveGameBuffer);
            uint32_t uiAnimEnd = apSaveGameBuffer->GetBufferPosition();
            apSaveGameBuffer->SaveVariableSizedValue(uiAnimEnd - uiAnimStart, uiAnimPosition);
        }
    }
    ActiveEffect::SaveActiveEffectList(
        apSaveGameBuffer, static_cast<BSSimpleList<ActiveEffect*>*>(static_cast<void*>(pActiveEffectList))
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
    uint32_t icurrentPackageSpellID;
    uint32_t irefID;
    uint32_t iblockHeader;
    float fheading;
    uint16_t iblockSize;
    uint8_t cnumber;
    uint32_t idesiredTargetID;
    uint32_t icommandingActorID;
    uint32_t icurrentSpellID;
    NiPoint3 position;
    uint32_t ifurnitureID;
    Actor* pactor = dynamic_cast<Actor*>(pmob);
    MiddleLowProcess::SaveGame(aiFlags, pmob);
    iblockSize = 0;
    // The block's size placeholder in the save buffer, patched at the end.
    void* pblockSize = 0;
    uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        iblockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
        pblockSize = pSaveLoadGameOLD->m_pBuffer;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&bWantWeaponDrawn, 1u);
    pSaveLoadGameOLD->SaveGameDataOLD(&bWeaponDrawn, 1u);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 52u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&bCheckMagicNode, 1u);
        pSaveLoadGameOLD->SaveGameDataOLD(&fAlphaMult, 4u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 77u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&fScriptRefractPower, 4u);
    }
    icommandingActorID = 0;
    if (pCommandingActor) {
        icommandingActorID = pCommandingActor->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&icommandingActorID, 4u);
    pSaveLoadGameOLD->SaveGameDataOLD(&eUseItem, 4u);
    pSaveLoadGameOLD->SaveGameDataOLD(&bBeenAttacked, 1u);
    pSaveLoadGameOLD->SaveGameDataOLD(&lastSeenPostion, 12u);
    pSaveLoadGameOLD->SaveGameDataOLD(&fEquippedWeight, 4u);
    pSaveLoadGameOLD->SaveGameDataOLD(&cSitSleepState, 1u);
    pSaveLoadGameOLD->SaveGameDataOLD(&cCurrentFurnitureIndex, 1u);
    ifurnitureID = 0;
    if (pCurrentFurniture) {
        ifurnitureID = pCurrentFurniture->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&ifurnitureID, 4u);
    fheading = FurnitureMarker.GetHeading();
    cnumber = FurnitureMarker.cNumber;
    position = FurnitureMarker.Position;
    pSaveLoadGameOLD->SaveGameDataOLD(&fheading, 4u);
    pSaveLoadGameOLD->SaveGameDataOLD(&cnumber, 1u);
    pSaveLoadGameOLD->SaveGameDataOLD(&position, 12u);
    pSaveLoadGameOLD->SaveGameDataOLD(&cKnockState, 1u);
    if ((aiFlags & 0x10000000) != 0) {
        Animation::SaveAnimation(pmob, pAnimation);
    }
    icurrentSpellID = 0;
    if (pCurrentSpell) {
        icurrentSpellID = pCurrentSpell->GetMagicItemFormID();
    }
    pSaveLoadGameOLD->SaveNumericID(&icurrentSpellID, 4u);
    idesiredTargetID = 0;
    if (pDesiredTarget) {
        idesiredTargetID = pDesiredTarget->GetMagicTargetFormID();
    }
    pSaveLoadGameOLD->SaveNumericID(&idesiredTargetID, 4u);
    ActiveEffect::SaveActiveEffectList(pActiveEffectList, pactor);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 69u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&bDoneOnce, 1u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 73u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&bPickPocketed, 1u);
        pSaveLoadGameOLD->SaveGameDataOLD(&bDoneClothesChange, 1u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 101u) {
        uint16_t icount;
        pSaveLoadGameOLD->SaveGameDataOLD(&fPursueTimer, 4u);
        icount = 0;
        uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&icount, 2u);
        for (BSSimpleList<void*>* plist = &DeadDetectList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
            TESObjectREFR* pref = static_cast<TESObjectREFR*>(plist->m_item);
            irefID = 0;
            if (pref) {
                irefID = pref->iFormID;
            }
            pSaveLoadGameOLD->SaveNumericID(&irefID, 4u);
            ++icount;
        }
        *pcount = icount;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 109u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&bSummonedCreature, 1u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        icurrentPackageSpellID = 0;
        if (pCurrentPackageSpell) {
            icurrentPackageSpellID = pCurrentPackageSpell->GetMagicItemFormID();
        }
        pSaveLoadGameOLD->SaveNumericID(&icurrentPackageSpellID, 4u);
    }
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* v3 = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), pheader->iFormID, v3, iFlags, 7185,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), 7185,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        char* pblockEnd = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
        char* pblockStart = static_cast<char*>(pblockSize);
        if (pblockEnd > pblockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp", 7185
            );
        }
        *static_cast<uint16_t*>(pblockSize) = static_cast<uint16_t>(pblockEnd - pblockStart);
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
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bWantWeaponDrawn), 1);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bWeaponDrawn), 1);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bCheckMagicNode), 1);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fAlphaMult), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fScriptRefractPower), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eUseItem), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bBeenAttacked), 1);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&lastSeenPostion), 0xC);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fEquippedWeight), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cSitSleepState), 1);
    if (pActor) {
        pActor->SetCachedSitSleepState(static_cast<SIT_SLEEP_STATE>(cSitSleepState));
    }
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cCurrentFurnitureIndex), 1);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&FurnitureMarker.cNumber), 1);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&FurnitureMarker.sHeading), 2);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&FurnitureMarker.Position), 0xC);
    int8_t cLoadedKnockState = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cLoadedKnockState), 1);
    if (pActor && cLoadedKnockState == 0 && cKnockState != 0) {
        ClearKnockState(pActor);
    }
    cKnockState = cLoadedKnockState;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bDoneOnce), 1);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bPickPocketed), 1);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bDoneClothesChange), 1);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fPursueTimer), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bSummonedCreature), 1);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fAimLooking), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fAutomaticShotDelay), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cShotsToFire), 1);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bSustainedFire), 1);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bIronSights), 1);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iAnimActionSuccess), 2);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iReservationSlot), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bAimingTarget), 1);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iPackageIdleNumber), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fPackageIdleTimer), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bPlayedBeginIdles), 1);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bPlayedEndIdles), 1);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fRadiationDelta), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fRadiationMagicDelta), 4);
    if (apLoadGameBuffer->GetVersion() >= 10) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fRadiationWaterDelta), 4);
    }
    if (apLoadGameBuffer->GetVersion() >= 3) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iWeaponConditionStage), 4);
    }
    if (apLoadGameBuffer->GetVersion() >= 7) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bPickPackIdle), 1);
    }
    pLastIdlePlayed = dynamic_cast<TESIdleForm*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
    pCurrentIdle = dynamic_cast<TESIdleForm*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
    apLoadGameBuffer->LoadFormID(pCommandingActor);
    apLoadGameBuffer->LoadFormID(pCurrentFurniture);
    uint32_t uiAmmoCount = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&uiAmmoCount), 4);
    uint32_t uiDeadCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < uiDeadCount; ++i) {
        void* pDead = nullptr;
        apLoadGameBuffer->LoadFormID(pDead);
        DeadDetectList.AddHead(pDead);
    }
    RunOncePackage.LoadGame(apLoadGameBuffer);
    if (pActor) {
        if (apLoadGameBuffer->GetChangeFlags().CheckFlags(0x10000000)) {
            pSaveLoadGame->QueueSubBuffer(QUEUED_SUB_BUFFER_ANIMATION, apLoadGameBuffer, nullptr);
        }
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(0x8000020)) {
        InventoryChanges* pChanges = pActor->GetExtra()->GetContainerChanges();
        if (pChanges) {
            ItemChange* pWornWeapon = pChanges->GetWornItem(BO_WEAPON, false);
            SetCurrentWeapon(pWornWeapon, pActor->Get3D(), false);
            TESObjectWEAP* pWeaponObject =
                pWeapon ? static_cast<TESObjectWEAP*>(pWeapon->GetItemChangeObject()) : nullptr;
            if (pWeapon && pActor->Get3D() && !(pActor->IsCreature() && std::strstr(pActor->GetFullName(), "Lily"))) {
                pActor->AttachWeapon(pWeaponObject);
                pActor->GetCurrentAIProcess()->SetCurrentAmmo(nullptr);
                pActor->ReloadWeaponNV(
                    pWeaponObject, 0, pWeapon->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE), true
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
                pProcess->ForceWeaponDrawnSheathed(
                    pActor->IsWeaponDrawn(), nullptr, pActor->Actor::GetAnimation(), pActor
                );
                if (ReferenceProcessLists.kUpdateLock.TryLock()) {
                    pActor->InitAnimation();
                    ReferenceProcessLists.kUpdateLock.Unlock();
                }
            }
        }
    }
    ActiveEffect::LoadActiveEffectList(
        apLoadGameBuffer, static_cast<BSSimpleList<ActiveEffect*>*>(static_cast<void*>(pActiveEffectList))
    );
    apLoadGameBuffer->LoadFormID(pCurrentPackageSpell);
    apLoadGameBuffer->LoadFormID(pCurrentSpell);
    apLoadGameBuffer->LoadFormID(pDesiredTarget);
    if (apLoadGameBuffer->GetVersion() >= 15) {
        uint32_t uiQueuedCount = apLoadGameBuffer->LoadVariableSizedValue();
        if (uiQueuedCount) {
            void* pListMemory = ourMalloc(sizeof(BSSimpleList<QueuedItem*>));
            BSSimpleList<QueuedItem*>* pList = pListMemory ? ::new (pListMemory) BSSimpleList<QueuedItem*> : nullptr;
            pListItemstoEquipUnequip = pList;
            for (uint32_t i = 0; i < uiQueuedCount; ++i) {
                void* pItemMemory = ourMalloc(sizeof(QueuedItem));
                QueuedItem* pItem = pItemMemory ? ::new (pItemMemory) QueuedItem : nullptr;
                pItem->LoadGame(apLoadGameBuffer);
                pListItemstoEquipUnequip->AddHead(pItem);
            }
        }
    }
}

void MiddleHighProcess::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags, MobileObject* pmob) {
    uint32_t icurrentPackageSpellID;
    uint32_t irefID;
    bool bragdollSaved;
    bool btemp;
    uint32_t iblockHeader;
    float fheading;
    uint16_t iblockSize;
    uint8_t cnumber;
    uint32_t idesiredTargetID;
    uint32_t icommandingActorID;
    char ctempKnockState;
    uint32_t icurrentSpellID;
    NiPoint3 position;
    uint32_t ifurnitureID;
    Actor* pactor = dynamic_cast<Actor*>(pmob);
    MiddleLowProcess::LoadGame(aiFlags, aiCurrentFlags, pmob);
    iblockSize = 0;
    uint8_t* istartBufferPosition = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, 4u);
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp", 7200,
                    pheader->iFormID, pform->GetFormEditorID(), pheader->cVersion, pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp", 7200,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, 2u);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&bWantWeaponDrawn, 1u);
    pSaveLoadGameOLD->LoadGameDataOLD(&bWeaponDrawn, 1u);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 52u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&bCheckMagicNode, 1u);
        pSaveLoadGameOLD->LoadGameDataOLD(&fAlphaMult, 4u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 77u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&fScriptRefractPower, 4u);
    }
    if ((aiFlags & 32) != 0 || (aiCurrentFlags & 32) != 0) {
        RemoveAndAttachItems(pmob);
        pactor->ReloadTargets(0);
        NiAVObject* pobj = (NiAVObject*)pmob->TESObjectREFR::Get3D();
        if (pobj) {
            NiNode* pnode = static_cast<NiNode*>(pobj);
            if (pmob == pPlayer && !pPlayer->Is3rdPerson()) {
                pobj = pnode->GetAtChecked(0);
                pnode = static_cast<NiNode*>(pobj);
            }
            pactor->InitLightingPropertyPtr(pnode);
        }
    }
    pSaveLoadGameOLD->LoadNumericID(&icommandingActorID, 4u);
    pCommandingActor = reinterpret_cast<Actor*>(static_cast<uintptr_t>(icommandingActorID));
    pSaveLoadGameOLD->LoadGameDataOLD(&eUseItem, 4u);
    pSaveLoadGameOLD->LoadGameDataOLD(&bBeenAttacked, 1u);
    if (pSaveLoadGameOLD->GetCurrentVersion() < 24u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&btemp, 1u);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&lastSeenPostion, 12u);
    pSaveLoadGameOLD->LoadGameDataOLD(&fEquippedWeight, 4u);
    pSaveLoadGameOLD->LoadGameDataOLD(&cSitSleepState, 1u);
    pSaveLoadGameOLD->LoadGameDataOLD(&cCurrentFurnitureIndex, 1u);
    pSaveLoadGameOLD->LoadNumericID(&ifurnitureID, 4u);
    pCurrentFurniture = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(ifurnitureID));
    pSaveLoadGameOLD->LoadGameDataOLD(&fheading, 4u);
    pSaveLoadGameOLD->LoadGameDataOLD(&cnumber, 1u);
    pSaveLoadGameOLD->LoadGameDataOLD(&position, 12u);
    FurnitureMarker.SetHeading(fheading);
    FurnitureMarker.SetType(cnumber);
    FurnitureMarker.SetPosition(position);
    pSaveLoadGameOLD->LoadGameDataOLD(&ctempKnockState, 1u);
    if (ctempKnockState) {
        if ((aiFlags & 4) == 0 && pSaveLoadGameOLD->GetCurrentVersion() < 47u) {
            pSaveLoadGameOLD->LoadGameDataOLD(&bragdollSaved, 1u);
            if (bragdollSaved) {
                RagDollData* pragdoll;
                void* apMemory = ourMalloc(sizeof(RagDollData));
                if (apMemory) {
                    pragdoll = ::new (apMemory) RagDollData;
                }
                else {
                    pragdoll = 0;
                }
                pragdoll->LoadGame();
                if (pragdoll) {
                    delete pragdoll;
                }
            }
        }
    }
    else {
        Actor* pactor_2 = dynamic_cast<Actor*>(pmob);
        ClearKnockState(pactor_2);
    }
    cKnockState = ctempKnockState;
    if ((aiFlags & 0x10000000) != 0) {
        Animation::LoadAnimation(pmob, pAnimation);
    }
    pSaveLoadGameOLD->LoadNumericID(&icurrentSpellID, 4u);
    pCurrentSpell = reinterpret_cast<MagicItem*>(static_cast<uintptr_t>(icurrentSpellID));
    pSaveLoadGameOLD->LoadNumericID(&idesiredTargetID, 4u);
    pDesiredTarget = reinterpret_cast<MagicTarget*>(static_cast<uintptr_t>(idesiredTargetID));
    ActiveEffect::LoadActiveEffectListTES(pActiveEffectList, pactor);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 69u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&bDoneOnce, 1u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 73u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&bPickPocketed, 1u);
        pSaveLoadGameOLD->LoadGameDataOLD(&bDoneClothesChange, 1u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 101u) {
        uint16_t icount;
        if (pSaveLoadGameOLD->GetCurrentVersion() < 104u) {
            pSaveLoadGameOLD->AdvanceBuffer(4u);
        }
        pSaveLoadGameOLD->LoadGameDataOLD(&fPursueTimer, 4u);
        if (pSaveLoadGameOLD->GetCurrentVersion() < 104u) {
            pSaveLoadGameOLD->AdvanceBuffer(4u);
        }
        pSaveLoadGameOLD->LoadGameDataOLD(&icount, 2u);
        for (uint32_t i = 0; i < icount; ++i) {
            pSaveLoadGameOLD->LoadNumericID(&irefID, 4u);
            // The list keeps the form ID until InitLoadGame resolves it.
            void* pdead = reinterpret_cast<void*>(static_cast<uintptr_t>(irefID));
            DeadDetectList.AddHead(pdead);
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 109u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&bSummonedCreature, 1u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        pSaveLoadGameOLD->LoadNumericID(&icurrentPackageSpellID, 4u);
        pCurrentPackageSpell = reinterpret_cast<MagicItem*>(static_cast<uintptr_t>(icurrentPackageSpellID));
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pheader_2 = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader_2) {
            TESForm* pform_2 = TESForm::GetFormByNumericID(pheader_2->iFormID);
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    uint32_t afHeadinga = pheader_2->iFlags;
                    int32_t cVersion = pheader_2->cVersion;
                    const char* v5 = pform_2->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(&istartBufferPosition[iblockSize] - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp", 7373,
                        pheader_2->iFormID, v5, cVersion, afHeadinga
                    );
                }
            }
            else {
                uint32_t afHeading = pheader_2->iFlags;
                int32_t v6 = pheader_2->cVersion;
                const char* v4 = pform_2->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(icurrentBufferPosition - &istartBufferPosition[iblockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp", 7373,
                    pheader_2->iFormID, v4, v6, afHeading
                );
            }
        }
        else {
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    int32_t afHeadingc = pSaveLoadGameOLD->GetCurrentVersion();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                        "%i",
                        static_cast<int32_t>(&istartBufferPosition[iblockSize] - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp", 7373, afHeadingc
                    );
                }
            }
            else {
                int32_t afHeadingb = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                    static_cast<int32_t>(icurrentBufferPosition - &istartBufferPosition[iblockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleHighProcess.cpp", 7373, afHeadingb
                );
            }
        }
    }
}

void MiddleHighProcess::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    MagicTarget* MagicTargetByNumericID;
    MagicItem* v8;
    MagicItem* MagicItemByNumericID;
    TESObjectREFR* v10;
    Actor* v11;
    Actor* pactor = apLoadGameBuffer->GetActor();
    LowProcess::InitLoadGame(apLoadGameBuffer);
    if (pCommandingActor) {
        TESForm* FormByNumericID =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCommandingActor)));
        v11 = dynamic_cast<Actor*>(FormByNumericID);
    }
    else {
        v11 = 0;
    }
    pCommandingActor = v11;
    if (pCurrentFurniture) {
        TESForm* v3 =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCurrentFurniture)));
        v10 = dynamic_cast<TESObjectREFR*>(v3);
    }
    else {
        v10 = 0;
    }
    pCurrentFurniture = v10;
    // The dead-detect list holds form IDs since LoadGame: each becomes its reference, or its node is removed.
    BSSimpleList<void*>* plist = &DeadDetectList;
    BSSimpleList<void*>* plastList = 0;
    while (plist && !plist->IsEmpty()) {
        void* pForm = plist->m_item;
        bool bremove = 0;
        if (pForm) {
            TESForm* v4 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pForm)));
            pForm = dynamic_cast<TESObjectREFR*>(v4);
            if (pForm) {
                plist->SetItem(pForm);
            }
            else {
                bremove = 1;
            }
        }
        else {
            bremove = 1;
        }
        if (bremove) {
            if (plastList) {
                BSSimpleList<void*>* v6 = plist;
                plastList->Remove(v6->m_item);
                plist = plastList->m_pkNext;
            }
            else {
                plist->RemoveHead();
            }
        }
        else {
            plastList = plist;
            plist = plist->m_pkNext;
        }
    }
    RunOncePackage.InitLoadGameBGS(apLoadGameBuffer);
    ActiveEffect::InitLoadActiveEffectList(apLoadGameBuffer, pActiveEffectList);
    if (pCurrentFurniture) {
        if (pCurrentFurniture->GetObjectReference()->GetFormType() == FURN_ID) {
            if (pactor) {
                TESObjectCELL* ParentCell;
                if (pactor->IsDead(0) ||
                    (ParentCell = pactor->GetParentCell(), ParentCell != pCurrentFurniture->GetParentCell())) {
                    ClearFurniture(pactor);
                }
            }
        }
        else {
            ClearFurniture(pactor);
        }
    }
    else {
        if (cSitSleepState) {
            cSitSleepState = 0;
            if (pactor == pPlayer) {
                pPlayer->ClearSitHeadingDelta();
            }
        }
    }
    if ((iReservationSlot & 0x80000000u) == 0) {
        if (pTarget) {
            pTarget->SetMarkerReserved(iReservationSlot, 1);
        }
    }
    if (pactor) {
        SetWeaponFlags();
        UpdateArmorWorn(pactor);
    }
    if (pactor) {
        CheckCachedValueFlags(pactor);
    }
    if (pCurrentPackageSpell) {
        MagicItemByNumericID = MagicItem::GetMagicItemByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCurrentPackageSpell))
        );
    }
    else {
        MagicItemByNumericID = 0;
    }
    pCurrentPackageSpell = MagicItemByNumericID;
    if (pCurrentSpell) {
        v8 = MagicItem::GetMagicItemByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCurrentSpell)));
    }
    else {
        v8 = 0;
    }
    pCurrentSpell = v8;
    if (pDesiredTarget) {
        MagicTargetByNumericID =
            MagicTarget::GetMagicTargetByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDesiredTarget)));
    }
    else {
        MagicTargetByNumericID = 0;
    }
    pDesiredTarget = MagicTargetByNumericID;
    if (apLoadGameBuffer->GetVersion() >= 15u) {
        BSSimpleList<QueuedItem*>* plist_2 = pListItemstoEquipUnequip;
        BSSimpleList<QueuedItem*>* plastList_2 = 0;
        while (plist_2 && !plist_2->IsEmpty()) {
            QueuedItem* pqueuedItem = plist_2->m_item;
            bool bremove_2 = 0;
            if (pqueuedItem) {
                if (pqueuedItem->pObject) {
                    pqueuedItem->InitLoadGame(apLoadGameBuffer);
                }
                else {
                    delete pqueuedItem;
                    bremove_2 = 1;
                }
            }
            else {
                bremove_2 = 1;
            }
            if (bremove_2) {
                if (plastList_2) {
                    plastList_2->Remove(plist_2->m_item);
                    plist_2 = plastList_2->m_pkNext;
                }
                else {
                    plist_2->RemoveHead();
                }
            }
            else {
                plastList_2 = plist_2;
                plist_2 = plist_2->m_pkNext;
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
    if (pProcess->GetRunOncePackage()) {
        // The second test masks with 0, so the package's TES init never runs, as the game does.
        if ((aiFlags & 0) != 0) {
            RunOncePackage.InitLoadGameTES(aiFlags, aiOldFlags);
        }
    }
    // Each stored pointer's value is passed as the numeric form ID to resolve: the field holds the ID as untyped
    // memory, so the pointer is read as an integer here.
    if (pCommandingActor) {
        TESForm* pForm =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCommandingActor)));
        pCommandingActor = dynamic_cast<Actor*>(pForm);
    }
    if (pCurrentFurniture) {
        TESForm* pForm =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCurrentFurniture)));
        pCurrentFurniture = dynamic_cast<TESObjectREFR*>(pForm);
    }
    if (pCurrentSpell) {
        pCurrentSpell =
            MagicItem::GetMagicItemByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCurrentSpell)));
    }
    if (pDesiredTarget) {
        pDesiredTarget =
            MagicTarget::GetMagicTargetByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDesiredTarget)));
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
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x71) {
        if (pCurrentPackageSpell) {
            pCurrentPackageSpell = MagicItem::GetMagicItemByNumericID(
                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCurrentPackageSpell))
            );
        }
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
                        kWorldPosition.~NiPoint3();
                        ::new (&kWorldPosition) NiPoint3;
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
                    kRotZ.~NiMatrix3();
                    ::new (&kRotZ) NiMatrix3;
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
        NiUpdateData kUpdateData;
        kUpdateData.~NiUpdateData();
        ::new (&kUpdateData) NiUpdateData(0.0f, false, false);
        p3D->Update(kUpdateData);
    }
    ActiveEffect::FinishInitLoadGameActiveEffectList(
        static_cast<BSSimpleList<ActiveEffect*>*>(static_cast<void*>(pActiveEffectList)), pActor
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
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(0x8000020)) {
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
    bCheckMagicNode = 0;
    pCurrentWeaponEffect = nullptr;
    fAlphaMult = 1.0f;
    fScriptRefractPower = 0.0f;
    uiFlareFlags = 0;
    bRefreshFlareFlags = false;
    bDoneOnce = false;
    bPickPocketed = false;
    bDoneClothesChange = false;
    bRefreshMagicShaderEffects = 0;
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
    bPlayedBeginIdles = 0;
    bPlayedEndIdles = false;
    iCreatureLipsynchStartTime = 0;
    fRadiationDelta = 0.0f;
    fRadiationMagicDelta = 0.0f;
    fRadiationWaterDelta = 0.0f;
    iWeaponConditionStage = 0xFFFFFFFF;
    bPickPackIdle = 0;
    pFaceNode = nullptr;
    pFaceNodeSkinned = nullptr;
    pAnimFace = nullptr;
    pLastIdlePlayed = nullptr;
    pCurrentIdle = nullptr;
    for (int32_t i = 0; i < 3; ++i) {
        pAimSequence[i] = nullptr;
    }
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(0x10000000)) {
        if (pAnimation) {
            pAnimation->Revert(apLoadGameBuffer);
        }
    }
    ActiveEffect::RevertActiveEffectList(
        apLoadGameBuffer, static_cast<BSSimpleList<ActiveEffect*>*>(static_cast<void*>(pActiveEffectList))
    );
    while (!DeferredHideLimbList.IsEmpty()) {
        DeferredHideLimb* pItem = static_cast<DeferredHideLimb*>(DeferredHideLimbList.GetItem());
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
    if ((aiFlags & 0x20) != 0) {
        if (pWeapon) {
            pActor->ReloadTargets(false);
        }
        pProcess->SetCurrentWeapon(nullptr, nullptr, false);
        pProcess->SetCurrentAmmo(nullptr);
        pProcess->SetCurrentShield(nullptr);
        pProcess->SetCurrentTorch(nullptr);
        pCurrentWeaponEffect = nullptr;
    }
    // The test masks with 0, so the package's TES revert never runs, as the game does.
    if ((aiFlags & 0) != 0) {
        RunOncePackage.RevertTES(aiFlags);
    }
    if (pSaveLoadGameOLD->IsUnk048Zero()) {
        if ((aiFlags & 0x10000000) != 0 && pAnimation) {
            pAnimation->RevertTES(pmob);
        }
        ActiveEffect::RevertActiveEffectList(
            static_cast<BSSimpleList<ActiveEffect*>*>(static_cast<void*>(pActiveEffectList)), pActor
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
        bCheckMagicNode = 0;
        pCurrentWeaponEffect = nullptr;
        fAlphaMult = 1.0f;
        fScriptRefractPower = 0.0f;
        uiFlareFlags = 0;
        bRefreshFlareFlags = false;
        bDoneOnce = false;
        fEssentialDownTimer = 0.0f;
        bPickPocketed = false;
        bDoneClothesChange = false;
        bRefreshMagicShaderEffects = 0;
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
        apLoadGameBuffer, static_cast<BSSimpleList<ActiveEffect*>*>(static_cast<void*>(pActiveEffectList))
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
    kWorldPosition.~NiPoint3();
    ::new (&kWorldPosition) NiPoint3;
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
    kRotZ.~NiMatrix3();
    ::new (&kRotZ) NiMatrix3;
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
    bRefreshMagicShaderEffects = (uint8_t)1;
    bRefreshMagicShaderEffectBody |= ((uint8_t)abBody);
    bRefreshMagicShaderEffectWeapon |= ((uint8_t)abWeapon);
    bRefreshMagicShaderEffectLoading |= ((uint8_t)abLoading);
}

void MiddleHighProcess::RefreshMagicShaderEffects(Actor* apActor) {
    if (bRefreshMagicShaderEffectBody) {
        MagicShaderHitEffect* peffect = ReferenceProcessLists.FindActiveShaderHitEffect(apActor);
        if (peffect) {
            peffect->Init();
        }
    }
    if (bRefreshMagicShaderEffectWeapon) {
        SetWeaponEnchantmentVisuals(0);
        if (apActor->pCurrentProcess) {
            BaseProcess* pProcess = apActor->pCurrentProcess;
            if (pProcess->GetCurrentWeapon()) {
                ItemChange* v2 = GetCurrentWeapon();
                EnchantmentItem* penchant = TESEnchantableForm::GetFormEnchanting(v2->pContainerObj);
                if (penchant) {
                    BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
                    ItemChange* v3 = CurrentAIProcess->GetCurrentWeapon();
                    double ItemCharge = v3->GetItemCharge();
                    if (penchant->GetCost(apActor) <= ItemCharge) {
                        EffectSetting* pes = penchant->GetAVEffect(MagicSystem::SELF);
                        if (pes) {
                            if (pes->data.pEnchantEffect) {
                                TESEffectShader* peffect_2 = pes->data.pEnchantEffect;
                                SetWeaponEnchantmentVisuals(peffect_2);
                            }
                        }
                    }
                }
            }
        }
        bool bcreated = 0;
        TESEffectShader* v4 = GetWeaponEnchantmentVisuals();
        MagicShaderHitEffect* pweapEffect = ReferenceProcessLists.FindAndCleanupWeaponShaderHitEffect(apActor, v4);
        if (pweapEffect) {
            pweapEffect->Init();
        }
        else {
            if (GetWeaponEnchantmentVisuals()) {
                MagicShaderHitEffect* pweapEffect_2;
                MagicShaderHitEffect* apMemory =
                    static_cast<MagicShaderHitEffect*>(NiMemObject::operator new(sizeof(MagicShaderHitEffect)));
                if (apMemory) {
                    TESEffectShader* v5 = GetWeaponEnchantmentVisuals();
                    pweapEffect_2 = ::new (apMemory) MagicShaderHitEffect(apActor, v5, -1.0f);
                }
                else {
                    pweapEffect_2 = 0;
                }
                pweapEffect_2->bWeaponEnchantment = 1;
                if (bRefreshMagicShaderEffectLoading) {
                    pweapEffect_2->SetLifeTime(100.0f);
                }
                if (pweapEffect_2->Init()) {
                    ReferenceProcessLists.AddTempEffect(pweapEffect_2);
                    if (!bRefreshMagicShaderEffectLoading) {
                        bcreated = 1;
                    }
                }
            }
        }
        if (apActor == pPlayer) {
            if (pPlayer->GetInventory3D()) {
                TESEffectShader* peffect_3 = GetWeaponEnchantmentVisuals();
                MagicShaderHitEffect* pinventory = pPlayer->GetMagicShaderHitEffect();
                if (peffect_3) {
                    if (pinventory) {
                        if (pinventory->pEffectData != peffect_3) {
                            delete pinventory;
                            pinventory = 0;
                        }
                    }
                    if (!pinventory) {
                        MagicShaderHitEffect* pinventory_2;
                        MagicShaderHitEffect* v12 =
                            static_cast<MagicShaderHitEffect*>(NiMemObject::operator new(sizeof(MagicShaderHitEffect)));
                        if (v12) {
                            pinventory_2 = ::new (v12) MagicShaderHitEffect(pPlayer, peffect_3, -1.0f);
                        }
                        else {
                            pinventory_2 = 0;
                        }
                        pinventory = pinventory_2;
                        pinventory_2->bWeaponEnchantment = 1;
                        pPlayer->SetInventoryWeaponEffect(pinventory_2);
                        if (!bcreated) {
                            pinventory_2->SetLifeTime(100.0f);
                        }
                    }
                }
                else {
                    if (pinventory) {
                        delete pinventory;
                        pinventory = 0;
                        pPlayer->SetInventoryWeaponEffect(0);
                    }
                }
                if (pinventory) {
                    if (!pinventory->Init()) {
                        delete pinventory;
                        pPlayer->SetInventoryWeaponEffect(0);
                    }
                }
            }
        }
    }
    bRefreshMagicShaderEffects = 0;
    bRefreshMagicShaderEffectBody = 0;
    bRefreshMagicShaderEffectWeapon = 0;
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
        bRefreshFlareFlags = 0;
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
            BSSimpleList<ArrowProjectile*>* v2;
            BSSimpleList<ArrowProjectile*>* v4 =
                static_cast<BSSimpleList<ArrowProjectile*>*>(ourMalloc(sizeof(BSSimpleList<ArrowProjectile*>)));
            if (v4) {
                v2 = ::new (v4) BSSimpleList<ArrowProjectile*>;
            }
            else {
                v2 = 0;
            }
            pAttachedArrowList = v2;
        }
        if (!pAttachedArrowList->IsInList(apArrow)) {
            pAttachedArrowList->AddHead(apArrow);
        }
    }
}

void MiddleHighProcess::RemoveAttachedArrow(ArrowProjectile* apArrow) {
    if (apArrow) {
        if (pAttachedArrowList) {
            pAttachedArrowList->Remove(apArrow);
        }
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
    BGSBodyPart* ptorsoPart;
    BGSBodyPart* pheadPart;
    BGSBodyPart** pBodyParts = apBodyPartData->pPartArray;
    for (int32_t i = 0; i < 15; ++i) {
        if (pBodyParts[i]) {
            NiNode* plimbRoot;
            BGSBodyPart* pbp = pBodyParts[i];
            const char* pname = pbp->GetNodeName();
            if (pname) {
                plimbRoot = static_cast<NiNode*>(BSUtilities::GetObjectByName(apNode, pname));
            }
            else {
                plimbRoot = 0;
            }
            pDamageRootNode[pbp->GetPartType()] = plimbRoot;
        }
    }
    if (apBodyPartData) {
        pheadPart = apBodyPartData->GetBodyPart(BGSBodyPart::LIMB_HEAD_1);
    }
    else {
        pheadPart = 0;
    }
    if (pheadPart) {
        if (pheadPart->GetTargetName()) {
            pHeadNode = BSUtilities::GetObjectByName(apNode, pheadPart->GetTargetName());
        }
    }
    if (apBodyPartData) {
        ptorsoPart = apBodyPartData->GetBodyPart(BGSBodyPart::LIMB_TORSO);
    }
    else {
        ptorsoPart = 0;
    }
    if (ptorsoPart) {
        if (ptorsoPart->GetTargetName()) {
            pTorsoNode = BSUtilities::GetObjectByName(apNode, ptorsoPart->GetTargetName());
        }
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
    bool bwasWearingPA = bPowerArmorWorn;
    bool bwasWearingBP = bHasBackPackWorn;
    bHeavyArmorWorn = 0;
    bPowerArmorWorn = 0;
    bPowerArmorHelmet = 0;
    bHasBackPackWorn = 0;
    for (BSSimpleList<ItemChange*>* plist__pinventory = InventoryChanges::GetInventoryChanges(apActor)->pListofChanges;
         plist__pinventory &&
         (!plist__pinventory->IsEmpty() && (!bPowerArmorWorn || (!bPowerArmorHelmet || !bHasBackPackWorn)));
         plist__pinventory = plist__pinventory->m_pkNext) {
        ItemChange* pitem = plist__pinventory->m_item;
        TESBoundObject* pobj = pitem->pContainerObj;
        if (pitem->GetWorn(0) && pobj->GetFormType() == ARMO_ID) {
            // The armor's TESBipedModelForm base, taken through a reference: there is deliberately no null test, as the
            // game does.
            TESBipedModelForm& rBiped = static_cast<TESObjectARMO&>(*pobj);
            if (rBiped.TESBipedModelForm::HasBackPack()) {
                bHasBackPackWorn = 1;
            }
            if (rBiped.TESBipedModelForm::IsPowerArmor()) {
                if (rBiped.TESBipedModelForm::FillsBipedSlot(BO_HAIR, 0, 0) ||
                    rBiped.TESBipedModelForm::FillsBipedSlot(BO_HEAD, 0, 0)) {
                    bPowerArmorHelmet = 1;
                }
                else {
                    bPowerArmorWorn = 1;
                }
            }
            if (rBiped.TESBipedModelForm::IsHeavy() && !rBiped.TESBipedModelForm::FillsBipedSlot(BO_HAIR, 0, 0) &&
                !rBiped.TESBipedModelForm::FillsBipedSlot(BO_HEAD, 0, 0)) {
                bHeavyArmorWorn = 1;
            }
        }
    }
    if (!pSaveLoadGame->GetSaveGameLoading() &&
        (bwasWearingBP != bHasBackPackWorn || bwasWearingPA != bPowerArmorWorn) && !bWeaponDrawn) {
        if (pWeapon) {
            if (apActor->IsNpc()) {
                Animation* Animation_ = apActor->Actor::GetAnimation();
                // Calls Character's biped getter directly on the actor, not through the virtual.
                ForceWeaponDrawnSheathed(
                    0, static_cast<Character*>(apActor)->Character::GetBiped(), Animation_, apActor
                );
            }
        }
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
    FNVD_INLINED_LOCAL(NiPointer<Model>, spmodel, std::nullopt, std::nullopt);
    if (!spReserveProjectileClones.m_pObject) {
        if (spWeaponPreload.m_pObject) {
            if (static_cast<QueuedFile*>(spWeaponPreload.m_pObject)->QueuedFile::GetAllChildrenFinished()) {
                TESBoundObject* FormFlags;
                TESObjectWEAP* pcurrentWeapon;
                ItemChange* pweap = GetCurrentWeapon();
                ItemChange* pammo = GetCurrentAmmo();
                if (pweap) {
                    pcurrentWeapon = static_cast<TESObjectWEAP*>(GetCurrentWeapon()->pContainerObj);
                }
                else {
                    pcurrentWeapon = 0;
                }
                if (pammo) {
                    FormFlags = GetCurrentAmmo()->pContainerObj;
                }
                else {
                    FormFlags = 0;
                }
                if (pcurrentWeapon) {
                    if (FormFlags) {
                        if (pcurrentWeapon->GetProjectile(apActor)) {
                            BGSProjectile* Projectile_ = pcurrentWeapon->GetProjectile(apActor);
                            int32_t icloneCount = Projectile_->GetPreCloneCount(pcurrentWeapon);
                            if (icloneCount > 0) {
                                ::new (&spmodel) NiPointer<Model>(static_cast<Model*>(0));
                                TESModel* v5 = static_cast<TESModel*>(pcurrentWeapon->GetProjectile(apActor));
                                const char* v3 = v5->GetModel();
                                pModelLoader->FindModel(v3, spmodel);
                                if (spmodel.m_pObject) {
                                    if (spmodel.m_pObject->GetModel()) {
                                        BSCloneReserver* v4;
                                        BSCloneReserver* v9 =
                                            static_cast<BSCloneReserver*>(ourMalloc(sizeof(BSCloneReserver)));
                                        if (v9) {
                                            v4 = ::new (v9) BSCloneReserver(spmodel.m_pObject->GetModel(), icloneCount);
                                        }
                                        else {
                                            v4 = 0;
                                        }
                                        spReserveProjectileClones.operator=(v4);
                                    }
                                }
                                NiPointer<Model>::Destroy(&spmodel);
                            }
                        }
                    }
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
    MiddleHighProcess::DeferredHideLimb* plimb;
    MiddleHighProcess::DeferredHideLimb* plimb_2;
    MiddleHighProcess::DeferredHideLimb* apMemory =
        static_cast<MiddleHighProcess::DeferredHideLimb*>(ourMalloc(sizeof(MiddleHighProcess::DeferredHideLimb)));
    if (apMemory) {
        plimb = ::new (apMemory) MiddleHighProcess::DeferredHideLimb;
    }
    else {
        plimb = 0;
    }
    plimb_2 = plimb;
    plimb->fDismemberTimer = 0.0f;
    plimb_2->bExplosion = abExplosion;
    plimb_2->eLimbIndex = aeLimbIndex;
    plimb_2->spDismemberedLimbRoot.operator=(apDismemberedLimbRoot);
    plimb_2->spReplacementLimb.operator=(static_cast<NiNode*>(apReplacementLimb));
    DeferredHideLimbList.AddHead(*reinterpret_cast<void**>(&plimb_2));
}

void MiddleHighProcess::UpdateDeferredHideDismemberedLimbs(Actor* apActor, float afDeltaTime) {
    float ftimer;
    if (pVATS.eMode == VATS::VATS_PLAYBACK) {
        ftimer = fExplodeLimbRemovalDelayVATS.Float();
    }
    else {
        ftimer = fExplodeLimbRemovalDelay.Float();
    }
    BSSimpleList<void*>* plist = &DeferredHideLimbList;
    BSSimpleList<void*>* plastlist = 0;
    while (plist && !plist->IsEmpty()) {
        MiddleHighProcess::DeferredHideLimb* plimb = static_cast<MiddleHighProcess::DeferredHideLimb*>(plist->m_item);
        bool bremove = 0;
        if (!plimb->spDismemberedLimbRoot.m_pObject ||
            (plimb->spDismemberedLimbRoot.m_pObject->m_uiRefCount <= 1 ||
             (plimb->spReplacementLimb.m_pObject && plimb->spReplacementLimb.m_pObject->m_uiRefCount <= 1))) {
            bremove = 1;
        }
        plimb->fDismemberTimer = plimb->fDismemberTimer + afDeltaTime;
        if (!bremove && ftimer < static_cast<double>(plimb->fDismemberTimer)) {
            apActor->FillBoneArray(plimb->spDismemberedLimbRoot.m_pObject, 0);
            bool bExplosion = plimb->bExplosion;
            NiNode* v4 = plimb->spReplacementLimb.m_pObject;
            NiNode* v3 = plimb->spDismemberedLimbRoot.m_pObject;
            apActor->HideDismemberedLimb(plimb->eLimbIndex, v3, v4, bExplosion);
            bremove = 1;
        }
        if (bremove) {
            if (plimb) {
                delete plimb;
            }
            if (plastlist) {
                // Remove is passed the limb local, which holds the freed item's pointer.
                void* pItem = plimb;
                plastlist->Remove(pItem);
                plist = plastlist->m_pkNext;
            }
            else {
                plist->RemoveHead();
            }
        }
        else {
            plastlist = plist;
            plist = plist->m_pkNext;
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
    HitData* v2;
    void* v4;
    if (!pLastHitData) {
        v4 = ourMalloc(sizeof(HitData));
        if (v4) {
            v2 = ::new (v4) HitData;
        }
        else {
            v2 = nullptr;
        }
        pLastHitData = v2;
    }
    pLastHitData->Copy(apData);
}

void MiddleHighProcess::ClearLastHitData() {
    HitData* v1;
    void* v3;
    if (pLastHitData) {
        pLastHitData->ClearData();
    }
    else {
        v3 = ourMalloc(sizeof(HitData));
        if (v3) {
            v1 = ::new (v3) HitData;
        }
        else {
            v1 = nullptr;
        }
        pLastHitData = v1;
    }
}

HitData* MiddleHighProcess::GetLastAttackHitData() {
    return pLastAttackHitData;
}

void MiddleHighProcess::SetLastAttackHitData(HitData* apData) {
    HitData* v2;
    void* v4;
    if (!pLastAttackHitData) {
        v4 = ourMalloc(sizeof(HitData));
        if (v4) {
            v2 = ::new (v4) HitData;
        }
        else {
            v2 = nullptr;
        }
        pLastAttackHitData = v2;
    }
    pLastAttackHitData->Copy(apData);
}

void MiddleHighProcess::ClearLastAttackHitData() {
    HitData* v1;
    void* v3;
    if (pLastAttackHitData) {
        pLastAttackHitData->ClearData();
    }
    else {
        v3 = ourMalloc(sizeof(HitData));
        if (v3) {
            v1 = ::new (v3) HitData;
        }
        else {
            v1 = nullptr;
        }
        pLastAttackHitData = v1;
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
    NiPoint3 v38;
    NiPoint3 v40;
    Script* v65 = 0;
    TESPackage* pPack = GetPackageThatIsRunning();
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    if (!apActor->GetItemBeingEscorted()) {
        uint32_t CastingType;
        if (!pTarget || (!pTarget->IsActor() && (CastingType = pTarget->iFormID, !apActor->HasObjectId(CastingType)))) {
            AddToProcedureIndexRunning(apActor, 1);
            if (!apActor->IsPathingComplete()) {
                EndMoveMessage(apActor);
            }
            return;
        }
    }
    bool bIsActor = 0;
    if (pTarget) {
        if (pTarget->IsActor() && pTarget != apActor) {
            bIsActor = 1;
            Actor* pact = dynamic_cast<Actor*>(pTarget);
            BaseProcess* CurrentAIProcess = pact->pCurrentProcess;
            TESPackage* prun = CurrentAIProcess->GetCurrentPackage();
            if (pTarget != pPlayer &&
                (!prun || (prun->GetPackType() != PACKAGE_TYPE_FOLLOW && !prun->GetIsCreated()))) {
                SetProcedureIndexRunning(0);
                return;
            }
        }
    }
    const NiPoint3* v21 = &apActor->GetLocationOnReference();
    const NiPoint3* v3 = &apActor->GetLocationOnReference();
    NiPoint3 distance = *v3 - *v21;
    float fwaitdistance = 0.0f;
    Actor* ptarg = 0;
    if (bIsActor) {
        const NiPoint3* v22 = &pTarget->GetLocationOnReference();
        const NiPoint3* v4 = &apActor->GetLocationOnReference();
        distance = *v4 - *v22;
        ptarg = static_cast<Actor*>(pTarget);
        fwaitdistance = 0.0f;
        int32_t idistpack = pPack->GetPackageTarget()->iValue;
        if (idistpack <= 0) {
            idistpack = 200;
        }
        if (pTarget == pPlayer && fwaitdistance == 0.0) {
            fwaitdistance = static_cast<float>(idistpack);
        }
        else {
            if (fwaitdistance == 0.0 && apActor->GetParentCell()->GetInterior()) {
                fwaitdistance = fAIEscortWaitDistanceInterior.Float();
            }
            else {
                if (fwaitdistance == 0.0) {
                    double v33 = static_cast<double>(idistpack);
                    fwaitdistance = fAIEscortWaitDistanceExterior.Float() * v33;
                }
            }
        }
    }
    if (pPack->pPackLoc && pPack->IsActorAtLocation(apActor, 0, -1.0f, 0)) {
        if (!bIsActor) {
            ItemChange* pitem;
            pitem = apActor->GetItemBeingEscorted();
            if (pitem) {
                ExtraDataList* plist = 0;
                if (pitem->pExtraObjectList) {
                    plist = pitem->pExtraObjectList->m_item;
                }
                uint32_t id = 0;
                if (plist) {
                    if (plist->GetReferencePointer()) {
                        TESObjectREFR* ReferencePointer = plist->GetReferencePointer();
                        id = ReferencePointer->iFormID;
                    }
                }
                TESObjectREFR* pcont = pPack->pPackLoc->GetLocReference();
                if (!pcont) {
                    pcont = pGenericLocation;
                }
                if (pcont && pcont->HasContainer()) {
                    int32_t InitialTargetCount = pPack->GetInitialTargetCount();
                    apActor->PutItemInReference(pitem->pContainerObj, plist, pcont, InitialTargetCount, id);
                }
                else {
                    if (pitem->pExtraObjectList) {
                        plist = pitem->pExtraObjectList->m_item;
                    }
                    const NiPoint3* ppoint = 0;
                    PackageLocation* ppackloc = pPack->pPackLoc;
                    if (ppackloc) {
                        TESObjectREFR* pref = ppackloc->GetLocReference();
                        if (!pref) {
                            pref = pGenericLocation;
                        }
                        if (pref) {
                            TESBoundObject* v8;
                            TESBoundObject* ObjectReference = pref->GetObjectReference();
                            if (ObjectReference == pXMarker ||
                                (v8 = pref->GetObjectReference(), v8 == pXMarkerHeading)) {
                                ppoint = &pref->GetLocationOnReference();
                            }
                        }
                    }
                    int32_t v9 = pPack->GetInitialTargetCount();
                    apActor->DropObject(pitem->pContainerObj, plist, v9, ppoint, 0);
                }
            }
            if (pitem) {
                delete pitem;
            }
            AddToProcedureIndexRunning(apActor, 1);
        LABEL_64:;
            if (!apActor->IsPathingComplete()) {
                EndMoveMessage(apActor);
            }
            return;
        }
        double v10 = distance.Length();
        if (!(fwaitdistance >= v10)) {
            goto LABEL_64;
        }
        AddToProcedureIndexRunning(apActor, 1);
        Actor* ptarg_2 = static_cast<Actor*>(pTarget);
        BaseProcess* v32 = ptarg_2->pCurrentProcess;
        if (!v32->GetPackageThatIsRunning()) {
            goto LABEL_64;
        }
        BaseProcess* v31 = ptarg_2->pCurrentProcess;
        v31->AddToProcedureIndexRunning(apActor, 1);
        BaseProcess* v30 = ptarg_2->pCurrentProcess;
        TESPackage* v11 = v30->GetPackageThatIsRunning();
        if (!v11->GetIsCreated()) {
            goto LABEL_64;
        }
        BaseProcess* v29 = ptarg_2->pCurrentProcess;
        TESPackage* pfollow = v29->GetPackageThatIsRunning();
        if (pfollow) {
            delete pfollow;
        }
        BaseProcess* v28 = ptarg_2->pCurrentProcess;
        v28->ClearCurrentPackage();
        if (ptarg_2->IsPathingComplete()) {
            ptarg_2->EndMovement();
        }
        CheckforNewPackage(apActor, 0);
    }
    else {
        const NiPoint3* v12;
        const NiPoint3* v13;
        double v14;
        TESPackage* CurrentPackage;
        NiPoint3* LocationCoord;
        NiPoint3* v25;
        double v27;
        NiPoint3 ftargdistance;
        NiPoint3 fMydistance;
        if (bIsActor &&
            ((((LocationCoord = &(v40 = pPack->GetLocationCoord(apActor)),
                (v12 = &apActor->GetLocationOnReference(),
                 (fMydistance = *v12 - *LocationCoord,
                  (v25 = &(v38 = pPack->GetLocationCoord(apActor)),
                   (v13 = &ptarg->GetLocationOnReference(),
                    (ftargdistance = *v13 - *v25, (v14 = distance.Length(), fwaitdistance < v14))))))) &&
               (v27 = fMydistance.Length(), ftargdistance.Length() >= v27)) ||
              (ptarg != pPlayer &&
               (ptarg->GetCurrentPackage() &&
                (CurrentPackage = ptarg->GetCurrentPackage(), CurrentPackage->IsInterruptPackage())))) &&
             !apActor->IsPathingComplete())) {
            SetCurrentMovementComplete(1);
            AddToProcedureIndexRunning(apActor, -1);
        }
        else {
            double v16;
            TESObjectCELL* LocationCell;
            TESWorldSpace* LocationWorld;
            NiPoint3 v70;
            if (!apActor->IsPathingComplete() ||
                ((bIsActor && (v16 = distance.Length(), !(fwaitdistance >= v16))) ||
                 (LocationWorld = pPack->GetLocationWorld(apActor),
                  (LocationCell = pPack->GetLocationCell(apActor),
                   (v70 = pPack->GetLocationCoord(apActor),
                    SetupPathFinding(apActor, v70, LocationCell, LocationWorld)))))) {
                if (!apActor->IsPathingComplete()) {
                    TESGlobal* ptimescale = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(58u));
                    float fhour = appCalendar.GetHour();
                    if (fhour < fHourLastProcessed) {
                        fhour = fhour + 24.0;
                    }
                    // Scales the elapsed hours to seconds of game time and never uses the result, as the game does.
                    float ftimedelta = fhour - fHourLastProcessed;
                    ftimedelta = 3600.0 / ptimescale->fValue * ftimedelta;
                    (void)ftimedelta;
                }
            }
        }
    }
}

void MiddleHighProcess::ProcessSleep(Actor* apActor) {
    if (apActor->IsNpc() && apActor->GetSitSleepState() == SIT_SLEEP_NORMAL && !pCurrentFurniture &&
        RefListChairBed.IsEmpty()) {
        FindBedChairs(apActor, 0, 0);
    }
    if (apActor->GetSitSleepState() != SIT_SLEEP_IS_SLEEPING && apActor->IsNpc()) {
        if (!pCurrentFurniture) {
            if (RefListChairBed.ItemsInList()) {
                if (TESObjectREFR::kFurnitureLock.TryLock()) {
                    pCurrentFurniture = static_cast<TESObjectREFR*>((&RefListChairBed)->m_item);
                    ClearFurnitureMarker();
                    if (pCurrentFurniture->GetOwner()) {
                        RefListChairBed.Remove(*reinterpret_cast<void**>(&pCurrentFurniture));
                    }
                    else {
                        int32_t inumber = RefListChairBed.ItemsInList();
                        uint32_t v2 = Random();
                        int32_t irandom = v2 % inumber;
                        if (irandom >= inumber) {
                            irandom = inumber;
                        }
                        BSSimpleList<void*>* plist = &RefListChairBed;
                        for (int32_t i = 0; i < irandom; ++i) {
                            plist = plist->m_pkNext;
                        }
                        pCurrentFurniture = static_cast<TESObjectREFR*>(plist->m_item);
                        ClearFurnitureMarker();
                    }
                    TESObjectREFR::kFurnitureLock.Unlock();
                }
            }
        }
        SetTarget(pCurrentFurniture);
        if (pCurrentFurniture) {
            ProcessActivate(apActor, 0);
        }
        if (DidPathingFail(apActor)) {
            pCurrentFurniture = 0;
            ClearFurnitureMarker();
        }
    }
    if ((apActor->IsNpc() && apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) ||
        (!pCurrentFurniture && RefListChairBed.IsEmpty())) {
        SetCurrentActionComplete(1);
        RefListChairBed.RemoveAll();
    }
    SetupSpecialIdle(apActor, 0, AnimIdle::PLAY_TYPE_AUTO, 1, 0, 1);
}

void MiddleHighProcess::ProcessEat(Actor* apActor, TESObjectREFR* apLocRef) {
    GetPackageThatIsRunning();
    ItemChange* pfood = apActor->GetBestFoodItem();
    if (GetCurrentActionComplete()) {
        SetupSpecialIdle(apActor, 0, AnimIdle::PLAY_TYPE_AUTO, 1, 0, 1);
    }
    else {
        if (apActor->IsNpc() && apActor->GetSitSleepState() == SIT_SLEEP_NORMAL && !pCurrentFurniture &&
            RefListChairBed.IsEmpty()) {
            FindBedChairs(apActor, 1, apLocRef);
        }
        if (apActor->IsNpc() && apActor->GetSitSleepState() != SIT_SLEEP_IS_SITTING) {
            if (!pCurrentFurniture) {
                if (RefListChairBed.ItemsInList()) {
                    if (TESObjectREFR::kFurnitureLock.TryLock()) {
                        pCurrentFurniture = static_cast<TESObjectREFR*>((&RefListChairBed)->m_item);
                        ClearFurnitureMarker();
                        if (pCurrentFurniture->GetOwner()) {
                            RefListChairBed.Remove(*reinterpret_cast<void**>(&pCurrentFurniture));
                        }
                        else {
                            int32_t inumber = RefListChairBed.ItemsInList();
                            uint32_t v3 = Random();
                            int32_t irandom = v3 % inumber;
                            if (irandom >= inumber) {
                                irandom = inumber;
                            }
                            BSSimpleList<void*>* plist = &RefListChairBed;
                            for (int32_t i = 0; i < irandom; ++i) {
                                plist = plist->m_pkNext;
                            }
                            pCurrentFurniture = static_cast<TESObjectREFR*>(plist->m_item);
                            ClearFurnitureMarker();
                            RefListChairBed.Remove(*reinterpret_cast<void**>(&pCurrentFurniture));
                        }
                        TESObjectREFR::kFurnitureLock.Unlock();
                    }
                }
            }
            SetTarget(pCurrentFurniture);
            if (pCurrentFurniture) {
                ProcessActivate(apActor, 0);
            }
            if (DidPathingFail(apActor)) {
                ClearFurniture(apActor);
            }
        }
        if ((apActor->IsNpc() && apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING) ||
            (!pCurrentFurniture && RefListChairBed.IsEmpty())) {
            ExtraDataList* pextra = 0;
            if (pfood) {
                if (pfood->pExtraObjectList) {
                    pextra = pfood->pExtraObjectList->m_item;
                }
                apActor->Eat(pfood->pContainerObj, pextra, 1);
            }
            SetCurrentActionComplete(1);
            RefListChairBed.RemoveAll();
        }
        SetupSpecialIdle(apActor, 0, AnimIdle::PLAY_TYPE_AUTO, 1, 0, 1);
        if ((apActor->GetMoveMode() & 1024) != 0) {
            apActor->SetWalkRunFlagDirty();
            apActor->ClearMoveMode(1024u);
        }
    }
}

void MiddleHighProcess::ProcessCombat(Actor* apActor) {
    int32_t iRoundAmound = static_cast<int32_t>(fCombatRoundAmount.Float());
    Actor* pTargetActor = dynamic_cast<Actor*>(pTarget);
    bool bRoundDone = 0;
    if (pTargetActor && !pTargetActor->GetDisabled()) {
        void* pCell;
        BaseProcess* pProcess;
        if (((apActor->GetParentCell() && (pCell = apActor->GetParentCell(), pCell == pTargetActor->GetParentCell())) ||
             apActor->GetDistanceFromReference(pTargetActor, 0, 0) < 300.0) &&
            (pTargetActor->GetCurrentAIProcess() &&
             (pProcess = pTargetActor->GetCurrentAIProcess(),
              pProcess->GetProcessLevel() == PROCESS_TYPE_MIDDLE_HIGH))) {
            while (iRoundAmound && !apActor->IsDead(0)) {
                apActor->PerformOneRoundofCombat();
                pTargetActor = dynamic_cast<Actor*>(pTarget);
                if (pTargetActor) {
                    if (!pTargetActor->IsDead(0)) {
                        BaseProcess* CurrentAIProcess = pTargetActor->GetCurrentAIProcess();
                        if (CurrentAIProcess->GetProcessLevel() == PROCESS_TYPE_MIDDLE_HIGH) {
                            pTargetActor->PerformOneRoundofCombat();
                            bRoundDone = 1;
                        }
                    }
                }
                --iRoundAmound;
            }
            if (bRoundDone) {
                if (pTargetActor) {
                    pProcess = pTargetActor->GetCurrentAIProcess();
                    pProcess->ComputeLastTimeProcessed();
                }
            }
        }
        else {
            ProcessFollow(apActor, 1, 257, 0);
        }
    }
    else {
        apActor->StopCombat(pTargetActor);
    }
}

void MiddleHighProcess::ProcessAlarm(Actor* apActor) {
    AlarmPackage* ppack = static_cast<AlarmPackage*>(GetCurrentPackage());
    Actor* pactor = static_cast<Actor*>(ppack->GetPackageTarget()->GetTargReference());
    bool bcombatflag = 0;
    if (ppack) {
        TESClass* Class = apActor->GetClass();
        if (Class->IsGuard()) {
            AddToProcedureIndexRunning(apActor, 1);
        }
        else {
            for (Crime* pcrime : (&ppack->Crimes)->ItemsUntilNull()) {
                uint32_t* v12 = 0;
                Actor* pcrimeAct = 0;
                TESObjectREFR* pcrimeRef = pcrime->pcrimetarget;
                if (pcrimeRef) {
                    if (pcrimeRef->IsActor()) {
                        pcrimeAct = dynamic_cast<Actor*>(pcrimeRef);
                    }
                }
                if (!pcrime->breported) {
                    Crime* v8 = pcrime;
                    float afRadius2 = static_cast<float>(iCrimeAlarmRecDistance.Int());
                    const NiPoint3* v6 = &apActor->GetLocationOnReference();
                    float afRadius1 = static_cast<float>(iCrimeAlarmRecDistance.Int());
                    const NiPoint3* v4 = &apActor->GetLocationOnReference();
                    TESObjectCELL* ParentCell = apActor->GetParentCell();
                    pDataHandler->EnumReferencesCloseToPoint(
                        ParentCell, *v4, afRadius1, *v6, afRadius2, &HighProcess::FindDoorsToSendYellAlarm, v8
                    );
                }
                if (!bcombatflag) {
                    FIGHT_REACTION ereaction = FIGHT_REACTION_NEUTRAL;
                    if (pcrime->ecrime <= PackageNS::CRIME_TYPE_TRESPASS) {
                        if (apActor->GetShouldAttackActor(pactor, 0, ereaction, 0)) {
                            bcombatflag = 1;
                        }
                    }
                    else {
                        if (pcrimeAct) {
                            if (apActor->GetShouldAttackActor(pactor, 0, ereaction, 0)) {
                                bcombatflag = 1;
                            }
                        }
                    }
                }
            }
            if (bcombatflag) {
                EnterCombat(apActor, pactor, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0);
            }
            else {
                AddToProcedureIndexRunning(apActor, 3);
            }
        }
    }
}

bool MiddleHighProcess::ProcessActivate(Actor* apActor, bool abInterateindex) {
    TESPackage* pPack = 0;
    pPack = GetPackageThatIsRunning();
    if (!pTarget || pTarget->GetDelete()) {
        SetTargetForPackage(apActor);
    }
    if (!pTarget) {
        AddToProcedureIndexRunning(apActor, 1);
        return 0;
    }
    TESObjectREFR* pref = pPack->GetLocationReference(apActor);
    if (pref) {
        if (pref->IsFurniture()) {
            TESBoundObject* v4;
            TESBoundObject* ObjectReference = pTarget->GetObjectReference();
            if (ObjectReference == pXMarkerHeading || (v4 = pTarget->GetObjectReference(), v4 == pXMarker)) {
                SetActionHeadTrackTarget(pref);
                return 1;
            }
        }
    }
    if (pPack->GetPackType() == PACKAGE_TYPE_AMBUSH && !pPack->IsTargetAtLocation(apActor, 300)) {
        AddToProcedureIndexRunning(apActor, -1);
    }
    DoorTeleportData* pTel = pTarget->GetTeleport();
    bool batRef = 0;
    if (pTel) {
        const NiPoint3* v9 = &apActor->GetLocationOnReference();
        const NiPoint3* LinkedDoorTeleportPosition = &pTarget->GetLinkedDoorTeleportPosition();
        NiPoint3 dist = *LinkedDoorTeleportPosition - *v9;
        double flength = dist.Length();
        if (static_cast<double>(iAIDistanceRadiusMinLocation.Int()) >= flength) {
            batRef = 1;
        }
    }
    else {
        if (pTarget->IsFurniture() && apActor->GetSitSleepState()) {
            batRef = 1;
        }
        else {
            if (pTarget->IsFurniture() &&
                (apActor->GetSitSleepState() == SIT_SLEEP_NORMAL &&
                 (pTarget->GetMarkerUsed(cCurrentFurnitureIndex, 1) && !apActor->IsPathingComplete()))) {
                ClearFurniture(apActor);
                EndMoveMessage(apActor);
                SetTarget(0);
                return 0;
            }
            batRef = pPack->IsActorAtRefTarget(apActor, 0);
        }
    }
    if (!batRef && !DidPathingFail(apActor)) {
        if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
            apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
            apActor->InitiateGetUpPackage();
            return 1;
        }
        if (apActor->IsPathingComplete()) {
            if (pTarget->GetTeleport()) {
                if (pTarget->IsFurniture()) {
                    NiPoint3 point = apActor->GetLocationOnReference();
                    uint32_t iindex = 0;
                    if (!pTarget->GetClosestFreeMarker(point, 1, 1, FurnitureMarker, iindex, 1)) {
                        ClearFurniture(apActor);
                        EndMoveMessage(apActor);
                        SetTarget(0);
                        return 0;
                    }
                    TESWorldSpace* WorldSpace = pTarget->GetWorldSpace();
                    if (!SetupPathFinding(apActor, FurnitureMarker.Position, pTarget->GetParentCell(), WorldSpace)) {
                        return 0;
                    }
                    cCurrentFurnitureIndex = iindex;
                }
                else {
                    TESWorldSpace* v11 = pTarget->GetWorldSpace();
                    TESObjectCELL* ParentCell = pTarget->GetParentCell();
                    NiPoint3 v27 = pTarget->GetLinkedDoorTeleportPosition();
                    if (!SetupPathFinding(apActor, v27, ParentCell, v11)) {
                        return 0;
                    }
                }
            }
            else {
                TESWorldSpace* pworld = pTarget->GetWorldSpace();
                TESObjectCELL* pcell = pTarget->GetParentCell();
                if (!SetupPathFinding(apActor, pTarget->GetLocationOnReference(), pcell, pworld)) {
                    return 0;
                }
            }
        }
        if (!apActor->IsPathingComplete()) {
            TESGlobal* ptimescale = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(58u));
            float fhour = appCalendar.GetHour();
            if (fhour < fHourLastProcessed) {
                fhour = fhour + 24.0;
            }
            // Scales the elapsed hours to seconds of game time and never uses the result, as the game does.
            float ftimedelta = fhour - fHourLastProcessed;
            ftimedelta = 3600.0 / ptimescale->fValue * ftimedelta;
            (void)ftimedelta;
            if (apActor->GetCurrentProcessType() != PROCESS_TYPE_MIDDLE_HIGH ||
                apActor->GetDesiredProcessLevel() != PROCESS_TYPE_MIDDLE_HIGH) {
                return 0;
            }
            batRef = pPack->IsActorAtRefTarget(apActor, 0);
        }
    }
    if (!batRef && !DidPathingFail(apActor)) {
        return 0;
    }
    if (!apActor->IsPathingComplete()) {
        EndMoveMessage(apActor);
    }
    TESPackage* v7 = GetCurrentPackage();
    if (v7->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE && DidPathingFail(apActor)) {
        apActor->EndDialogue();
        return 0;
    }
    if (abInterateindex) {
        TESObjectREFR* pobj = pTarget;
        if (!DidPathingFail(apActor) && !pPack->GetMustComplete()) {
            AddToProcedureIndexRunning(apActor, 1);
        }
        if (!DidPathingFail(apActor)) {
            if (pTarget->IsFurniture()) {
                if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
                    apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                    AddToProcedureIndexRunning(apActor, 1);
                    SetTargetActivated(1);
                    return 1;
                }
                if (!iAnimActionSuccess) {
                    goto LABEL_66;
                }
                AlignToFurnitureAndAddPostAnimSitSleep(apActor);
            }
            else {
                if (pAcquireObject) {
                    pTarget->Activate(apActor, 1, pAcquireObject->pObject, 1);
                }
                else {
                    if (!pTarget->IsActor() && !pTarget->Activate(apActor, 1, 0, 1) && pPack->GetMustComplete()) {
                        AddToProcedureIndexRunning(apActor, -1);
                    }
                }
            }
        }
        if (pobj) {
            pobj->RunScript();
        }
    }
    else {
        TESObjectREFR* pobj_2;
        pobj_2 = pTarget;
        if (!DidPathingFail(apActor)) {
            if (pTarget->IsFurniture()) {
                if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
                    apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                    SetTargetActivated(1);
                    return 1;
                }
                if (!iAnimActionSuccess) {
                LABEL_66:;
                    AddToProcedureIndexRunning(apActor, 1);
                    pGenericLocation = 0;
                    EndMoveMessage(apActor);
                    iAnimActionSuccess = 1;
                    return 1;
                }
                AlignToFurnitureAndAddPostAnimSitSleep(apActor);
            }
            else {
                if (pAcquireObject) {
                    pTarget->Activate(apActor, 0, pAcquireObject->pObject, 1);
                }
                else {
                    pTarget->Activate(apActor, 0, 0, 1);
                }
            }
        }
        if (pobj_2) {
            pobj_2->RunScript();
        }
    }
    ObjectstoAcquire* apThis = pAcquireObject;
    if (apThis) {
        apThis->Delete(1u);
    }
    pAcquireObject = 0;
    SetTarget(0);
    return DidPathingFail(apActor) == 0;
}

void MiddleHighProcess::ProcessAcquire(Actor* apActor) {
    TESPackage* pPack = GetPackageThatIsRunning();
    if (pPack) {
        int32_t ainumb = 1;
        if (pPack->GetPackageTarget()) {
            ainumb = pPack->GetInitialTargetCount();
        }
        PackageTarget* ptarg = pPack->GetPackageTarget();
        if (pPack->GetPackType() == PACKAGE_TYPE_EXPLORE &&
            (ptarg && (ptarg->GetTargObjectType() == POTYPE_COMBATWEAR || ptarg->GetTargObjectType() == POTYPE_WEAR))) {
            ProcessActivate(apActor, 1);
            if (GetProcedureIndexRunning() > 1) {
                if (ObjectList.IsEmpty()) {
                    AddToProcedureIndexRunning(apActor, 1);
                    if (apActor->IsNpc()) {
                        TESNPC* pnpc = dynamic_cast<TESNPC*>(apActor->GetObjectReference());
                        if (pnpc) {
                            pnpc->InitDefaultWorn(apActor, 1, 1, 0, 1);
                        }
                    }
                    else {
                        TESCreature* pcrea = dynamic_cast<TESCreature*>(apActor->GetObjectReference());
                        if (pcrea) {
                            pcrea->InitDefaultWorn(apActor, 1, 1, 1);
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
            return;
        }
        if (CheckIfHasObject(apActor, ainumb)) {
            EndMoveMessage(apActor);
            if (GetRunOncePackage()) {
                if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC) {
                    goto LABEL_57;
                }
            LABEL_22:;
                AddToProcedureIndexRunning(apActor, 1);
                return;
            }
            if (pPack->IsActorAtLocation(apActor, 0, -1.0f, 0) || pPack->GetPackType() == PACKAGE_TYPE_ESCORT) {
                if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC) {
                    goto LABEL_57;
                }
            LABEL_26:;
                AddToProcedureIndexRunning(apActor, 1);
                return;
            }
        LABEL_59:;
            SetProcedureIndexRunning(0);
            return;
        }
        if (!pTarget || (pTarget->GetDelete() || pTarget->GetDisabled())) {
            SetTargetForPackage(apActor);
        }
        if (!pTarget) {
            goto LABEL_26;
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
        else {
            if (pAcquireObject) {
                if (pAcquireObject->eType == ACQUIRE_TYPE_STEALGROUND) {
                    ProcessSteal(apActor);
                }
                else {
                    ProcessActivate(apActor, 0);
                }
            }
            else {
                if (!pPack->GetPackageTarget() || pPack->GetPackageTarget()->GetTargReference() != pTarget) {
                    goto LABEL_22;
                }
                ProcessActivate(apActor, 1);
            }
        }
        if (apActor->GetCurrentProcessType() == PROCESS_TYPE_MIDDLE_HIGH &&
            (apActor->GetDesiredProcessLevel() == PROCESS_TYPE_MIDDLE_HIGH && CheckIfHasObject(apActor, ainumb))) {
            EndMoveMessage(apActor);
            if (GetRunOncePackage()) {
                if (pPack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC) {
                    goto LABEL_26;
                }
            }
            else {
                if (!pPack->IsActorAtLocation(apActor, 0, -1.0f, 0) && pPack->GetPackType() != PACKAGE_TYPE_ESCORT) {
                    goto LABEL_59;
                }
                if (pPack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC) {
                    goto LABEL_26;
                }
            }
        LABEL_57:;
            AddToProcedureIndexRunning(apActor, 2);
        }
    }
}

void MiddleHighProcess::ProcessFleeNonCombat(Actor* apActor) {
    TESPackage* ppackage = GetPackageThatIsRunning();
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    SetLocationForPackage(apActor, 0);
    float fradius = 0.0f;
    if (ppackage->GetPackageTarget()) {
        fradius = static_cast<float>(ppackage->GetPackageTarget()->iValue);
    }
    else {
        fradius = static_cast<float>(ppackage->GetLocationRadius(apActor));
    }
    if (pTarget && static_cast<double>(fradius) < apActor->GetDistanceFromReference(pTarget, 0, 0)) {
        apActor->InitiateFlee(apActor, 1, 1, 1, 0, 0, -1.0f, -1.0f);
        FleePackage* v8 = 0;
        TESPackage* ppackage_2 = GetPackageThatIsRunning();
        FleePackage* pPack = dynamic_cast<FleePackage*>(ppackage_2);
        Actor* pact = dynamic_cast<Actor*>(pTarget);
        if (pPack) {
            if (pact) {
                pPack->AddAvoidedRef(pact);
            }
        }
    }
    else {
        if (pTarget || ppackage->IsActorAtLocation(apActor, 0, -1.0f, 0)) {
            if (!apActor->IsPathingComplete()) {
                EndMoveMessage(apActor);
            }
        }
        else {
            NiPoint3 point;
            TESObjectCELL* pcell = ppackage->GetLocationCell(apActor);
            point = ppackage->GetLocationCoord(apActor);
            if (apActor->IsPathingComplete()) {
                TESWorldSpace* WorldSpace = apActor->GetWorldSpace();
                SetupPathFinding(apActor, point, pcell, WorldSpace);
            }
            else {
                SetActorsAnimation(apActor, 513, 1);
            }
        }
    }
}

void MiddleHighProcess::ProcessSandBox(Actor* apActor) {
    NiPoint3 result;
    SandBoxActorPackageData* pdata = static_cast<SandBoxActorPackageData*>(BaseProcess::GetInstanceDataThatIsRunning());
    if (pdata) {
        TESPackage* ppack = GetCurrentPackage();
        float fradius = static_cast<float>(ppack->GetLocationRadius(apActor));
        bool batloc = pdata->UpdateIsActorAtLocation(apActor, fradius);
        TESObjectREFR* ploc = pdata->pLocation;
        float farbitraryRadiusPadding = 150.0f;
        bool bneedToRepath = 0;
        if (ploc && pdata->pCurrentTargetObject) {
            bneedToRepath =
                fradius + farbitraryRadiusPadding < ploc->GetDistanceFromReference(pdata->pCurrentTargetObject, 0, 0);
            if (bneedToRepath) {
                pdata->ForgetTargetAndPickProcedure(apActor, ppack);
            }
        }
        else {
            if (apActor->IsPathing()) {
                if (ploc) {
                    NiPointer<PathingRequest> preq(apActor->pActorMover->GetCurrentRequest());
                    if (preq.m_pObject) {
                        const PathingLocation* pathLoc = &preq.m_pObject->Destination;
                        TESWorldSpace* plocworld = pathLoc->pWorldSpace;
                        TESObjectCELL* plocintcell = pathLoc->GetCell();
                        if (plocintcell) {
                            if (!plocintcell->GetInterior()) {
                                plocintcell = 0;
                            }
                        }
                        TESWorldSpace* psandboxWorld = ploc->GetWorldSpace();
                        TESObjectCELL* psandboxIntCell = ploc->GetParentCell();
                        if (psandboxIntCell) {
                            if (!psandboxIntCell->GetInterior()) {
                                psandboxIntCell = 0;
                            }
                        }
                        bool bsamespace = 0;
                        if (psandboxIntCell || plocintcell) {
                            if (psandboxIntCell == plocintcell) {
                                bsamespace = 1;
                            }
                        }
                        else {
                            bsamespace = psandboxWorld == plocworld;
                        }
                        float fpaddedRadius = fradius + farbitraryRadiusPadding;
                        if (bsamespace && fradius >= 0.0) {
                            NiPoint3* a2 = ::new (&result) NiPoint3(pathLoc->GetLocation());
                            const NiPoint3* v2 = &ploc->GetLocationOnReference();
                            NiPoint3 delta = *v2 - *a2;
                            double v3 = delta.SqrLength();
                            bneedToRepath = fpaddedRadius * fpaddedRadius < v3;
                        }
                    }
                }
            }
            else {
                bneedToRepath = !batloc;
            }
        }
        if (bneedToRepath) {
            TESPackage* ppack_2 = GetPackageThatIsRunning();
            float fradius_3 = static_cast<float>(ppack_2->GetLocationRadius(apActor));
            float a2a = PathingRequest::GetDefaultRadius();
            float fradius_4 = BSMAX(fradius_3, a2a);
            PathingRequest request;
            if (ploc) {
                TESWorldSpace* WorldSpace = ploc->GetWorldSpace();
                TESObjectCELL* ParentCell = ploc->GetParentCell();
                const NiPoint3* v5 = &ploc->GetLocationOnReference();
                apActor->BuildRequest(&request, *v5, ParentCell, WorldSpace, fradius_4, 0);
            }
            else {
                TESWorldSpace* LocationWorld = ppack->GetLocationWorld(apActor);
                TESObjectCELL* LocationCell = ppack->GetLocationCell(apActor);
                apActor->BuildRequest(
                    &request, ppack->GetLocationCoord(apActor), LocationCell, LocationWorld, fradius_4, 0
                );
            }
            request.SetSoftGoalRadius(0);
            apActor->SetPathfindingGoal(&request);
            pdata->eState = SandBoxActorPackageData::SANDBOX_STATE_GETTING_UP;
        }
        else {
            switch (pdata->eState) {
                case SandBoxActorPackageData::SANDBOX_STATE_INIT: {
                    pdata->eState = SandBoxActorPackageData::SANDBOX_STATE_GETTING_UP;
                    PackageLocation* a2b = ppack->pPackLoc;
                    pdata->InitLocation(apActor, a2b);
                    goto LABEL_35;
                }
                case SandBoxActorPackageData::SANDBOX_STATE_GETTING_UP: {
                LABEL_35:;
                    if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING ||
                        apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING) {
                        apActor->InitiateGetUpPackage();
                    }
                    else {
                        if (pdata->IsDoneWithIdleAnims(apActor, 1)) {
                            pdata->eState = SandBoxActorPackageData::SANDBOX_STATE_MOVING;
                        }
                    }
                    return;
                }
                case SandBoxActorPackageData::SANDBOX_STATE_MOVING: {
                    switch (pdata->eProcedure) {
                        case SANDBOX_PROCEDURE_SIT: {
                            if (apActor->GetSitSleepState() >= SIT_SLEEP_LOAD_SIT_IDLE &&
                                apActor->GetSitSleepState() <= SIT_SLEEP_IS_SITTING) {
                                pdata->eState = SandBoxActorPackageData::SANDBOX_STATE_DOINGSTUFF;
                            }
                            break;
                        }
                        case SANDBOX_PROCEDURE_SLEEP: {
                            if (apActor->GetSitSleepState() >= SIT_SLEEP_LOAD_SLEEP_IDLE &&
                                apActor->GetSitSleepState() <= SIT_SLEEP_IS_SLEEPING) {
                                pdata->eState = SandBoxActorPackageData::SANDBOX_STATE_DOINGSTUFF;
                            }
                            break;
                        }
                        case SANDBOX_PROCEDURE_EAT: {
                            if (apActor->GetSitSleepState() >= SIT_SLEEP_LOAD_SIT_IDLE &&
                                apActor->GetSitSleepState() <= SIT_SLEEP_IS_SITTING) {
                                pdata->eState = SandBoxActorPackageData::SANDBOX_STATE_DOINGSTUFF;
                            }
                            break;
                        }
                        case SANDBOX_PROCEDURE_WANDER: {
                            pdata->eState = SandBoxActorPackageData::SANDBOX_STATE_DOINGSTUFF;
                            break;
                        }
                        case SANDBOX_PROCEDURE_USE_IDLE_MARKER: {
                            if (!apActor->IsPathing()) {
                                if (pCurrentIdle) {
                                    pdata->eState = SandBoxActorPackageData::SANDBOX_STATE_DOINGSTUFF;
                                }
                            }
                            break;
                        }
                        default: {
                            goto LABEL_58;
                        }
                    }
                    goto LABEL_58;
                }
                case SandBoxActorPackageData::SANDBOX_STATE_DOINGSTUFF: {
                LABEL_58:;
                    if (pdata->ShouldPickNewProcedure(apActor)) {
                        if (pdata->MoveToNextLocation()) {
                            TESObjectREFR* ploc_2 = pdata->pLocation;
                            if (ploc_2) {
                                TESPackage* ppack_3 = GetPackageThatIsRunning();
                                float fradius_5 = static_cast<float>(ppack_3->GetLocationRadius(apActor));
                                float a2c = PathingRequest::GetDefaultRadius();
                                float fradius_6 = BSMAX(fradius_5, a2c);
                                PathingRequest request_2;
                                TESWorldSpace* v16 = ploc_2->GetWorldSpace();
                                TESObjectCELL* v13 = ploc_2->GetParentCell();
                                const NiPoint3* v8 = &ploc_2->GetLocationOnReference();
                                apActor->BuildRequest(&request_2, *v8, v13, v16, fradius_6, 0);
                                request_2.SetSoftGoalRadius(0);
                                apActor->SetPathfindingGoal(&request_2);
                                pdata->eState = SandBoxActorPackageData::SANDBOX_STATE_GETTING_UP;
                            }
                        }
                        SetupNewPackage();
                        apActor->ClearLookAtTarget();
                        TESPackage* v9 = GetCurrentPackage();
                        pdata->PickProcedure(apActor, v9);
                        pdata->eState = SandBoxActorPackageData::SANDBOX_STATE_GETTING_UP;
                    }
                    TESObjectREFR* psandboxTarget = pdata->pCurrentTargetObject;
                    bool v25 = true;
                    if (!psandboxTarget) {
                        SANDBOX_PROCEDURE FormFlags = pdata->eProcedure;
                        if (SandBoxActorPackageData::IsTargetNeededForProcedure(FormFlags)) {
                            v25 = false;
                        }
                    }
                    bool bprocedureValid = v25;
                    if (v25) {
                        switch (pdata->eProcedure) {
                            case SANDBOX_PROCEDURE_SIT:
                            case SANDBOX_PROCEDURE_SLEEP: {
                                bprocedureValid = ProcessUseFurniture(apActor, psandboxTarget);
                                break;
                            }
                            case SANDBOX_PROCEDURE_EAT: {
                                ProcessEat(apActor, pdata->pLocation);
                                break;
                            }
                            case SANDBOX_PROCEDURE_WANDER: {
                                ProcessWander(apActor, 0, -1.0f);
                                break;
                            }
                            case SANDBOX_PROCEDURE_USE_IDLE_MARKER: {
                                ProcessUseIdleMarker(apActor, psandboxTarget);
                                break;
                            }
                            case SANDBOX_PROCEDURE_DIALOGUE: {
                                if (ProcessSandBoxDialogue(apActor, psandboxTarget)) {
                                    TESPackage* a2d = GetCurrentPackage();
                                    pdata->PickProcedure(apActor, a2d);
                                    pdata->eState = SandBoxActorPackageData::SANDBOX_STATE_MOVING;
                                }
                                break;
                            }
                            default: {
                                bprocedureValid = 0;
                                break;
                            }
                        }
                    }
                    ActorMover* pmover = apActor->pActorMover;
                    if (!bprocedureValid || (pmover && pmover->IsPathIncomplete())) {
                        TESPackage* a2e = GetCurrentPackage();
                        pdata->ForgetTargetAndPickProcedure(apActor, a2e);
                        pdata->eState = SandBoxActorPackageData::SANDBOX_STATE_GETTING_UP;
                    }
                    if (apActor->IsPathing()) {
                        SetActorsAnimation(apActor, 257, 1);
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
        pCurrPackage->SetIsCreated(1);
    }
}

void MiddleHighProcess::ProcessPatrol(Actor* apActor) {
    uint32_t eState;
    PatrolActorPackageData* currentPackageInstanceData =
        static_cast<PatrolActorPackageData*>(BaseProcess::GetInstanceDataThatIsRunning());
    if (currentPackageInstanceData) {
        if ((((int32_t)apActor->GetSitSleepState()) == 0) ||
            (currentPackageInstanceData->GetCurrentReference() == pCurrentFurniture)) {
            eState = currentPackageInstanceData->eState;
            if (eState) {
                if (eState == 1) {
                    ProcessPatrolTravelingToNextRef(apActor, currentPackageInstanceData);
                }
                else {
                    if (eState == 2) {
                        ProcessPatrolAtRef(apActor, currentPackageInstanceData);
                    }
                }
            }
            else {
                ProcessPatrolInit(apActor, currentPackageInstanceData);
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
    PackageTarget* ptarg = GetPackageThatIsRunning()->GetPackageTarget();
    if (!ptarg) {
        return 0;
    }
    PACK_TARG_TYPE etype = ptarg->GetTargType();
    bool bdone = 0;
    if (ptarg->GetTargType() && ptarg->GetTargType() != PTT_LINKEDREF) {
        if (etype == PTT_OBJECT) {
            TESBoundObject* TargObject = static_cast<TESBoundObject*>(ptarg->GetTargObject());
            return apActor->HasObjects(TargObject, POTYPE_NONE, ainumb, 0, eUseItem);
        }
        else {
            PACKAGE_OBJECT_TYPE TargObjectType = ptarg->GetTargObjectType();
            return apActor->HasObjects(0, TargObjectType, ainumb, 0, eUseItem);
        }
    }
    else {
        ActorPackage* pactorPack = GetActorPackageThatIsRunning();
        if (pactorPack) {
            TESObjectREFR* pref = pactorPack->pTarget;
            if (pref) {
                uint32_t CastingType = pref->iFormID;
                return apActor->HasObjects(pref->GetObjectReference(), POTYPE_NONE, 1, CastingType, eUseItem);
            }
        }
    }
    return bdone;
}

TESObjectREFR* MiddleHighProcess::GetTargetForPackage(Actor* apActor, TESPackage* apPackage) {
    TESObjectREFR* preturntarget = 0;
    bool v7 = apPackage == GetCurrentPackage();
    switch (apPackage->GetPackType()) {
        case PACKAGE_TYPE_FOLLOW:
        case PACKAGE_TYPE_ESCORT:
        case PACKAGE_TYPE_ACCOMPANY: {
            if (apPackage->IsTargetAnActor(apActor)) {
                preturntarget = apPackage->GetEscortTargetForPackage(apActor, 0);
            }
            break;
        }
        case PACKAGE_TYPE_PATROL: {
            if (v7) {
                PatrolActorPackageData* pdata =
                    static_cast<PatrolActorPackageData*>(BaseProcess::GetInstanceDataThatIsRunning());
                if (pdata) {
                    preturntarget = pdata->GetCurrentReference();
                }
            }
            if (!pTarget) {
                preturntarget = PatrolActorPackageData::GetInitialReferenceForSetup(apActor, apPackage);
            }
            break;
        }
        case PACKAGE_TYPE_DIALOGUE: {
            if (apPackage->GetPackageTarget()->GetTargReference() != apActor) {
                goto LABEL_15;
            }
            preturntarget = (TESObjectREFR*)apActor;
            break;
        }
        case PACKAGE_TYPE_IN_GAME_DIALOGUE: {
            preturntarget = static_cast<DialoguePackage*>(apPackage)->pTargetActor;
            break;
        }
        default: {
        LABEL_15:;
            preturntarget = SetTargetData(apActor, apPackage, apPackage->GetPackageTarget());
            break;
        }
    }
    return preturntarget;
}

TESObjectREFR* MiddleHighProcess::GetLocationForPackage(Actor* apActor, TESPackage* apPackage, bool bSecondLoc) {
    if (!apPackage) {
        return 0;
    }
    PackageLocation* pLoc = 0;
    TESObjectREFR* preturnlocationref = 0;
    if (bSecondLoc) {
        pLoc = apPackage->GetPackageSecondLocation();
    }
    else {
        pLoc = apPackage->pPackLoc;
    }
    if (pLoc) {
        if (pLoc->GetLocType() == PLT_REFERENCE) {
            preturnlocationref = pLoc->GetLocReference();
        }
        if (pLoc->GetLocType() == PLT_LINKEDREF) {
            return apActor->GetLinkedRef();
        }
        else {
            if (pLoc->IsLocationGeneric()) {
                if (GenericLocationsList.IsEmpty()) {
                    TESObjectCELL* pCell = apActor->GetParentCell();
                    NiPoint3 pointB = apActor->GetLocationOnReference();
                    PACK_LOC_TYPE etype = pLoc->GetLocType();
                    if (etype == PLT_OBJECT) {
                        pObjectForLocation = static_cast<TESBoundObject*>(pLoc->GetLocObject());
                        eFormType = POTYPE_NONE;
                    }
                    else {
                        if (etype == PLT_TYPE) {
                            pObjectForLocation = 0;
                            eFormType = pLoc->GetLocObjectType();
                        }
                    }
                    float facquireDist = 0.0f;
                    if (!apPackage->GetAcquireRadius(apActor, facquireDist)) {
                        facquireDist = fAIAcquireObjectDistance.Float();
                    }
                    float afRadius2 = facquireDist;
                    const NiPoint3* v5 = &apActor->GetLocationOnReference();
                    pDataHandler->EnumReferencesCloseToPoint(
                        pCell, pointB, facquireDist, *v5, afRadius2, &EvaluateGenericLocation, apActor
                    );
                    eFormType = POTYPE_NONE;
                    pObjectForLocation = 0;
                    for (BSSimpleList<TESObjectREFR*>* plist = &TempLocList; plist->m_item; plist->RemoveHead()) {
                        GenericLocationsList.AddHead(plist->m_item);
                    }
                    TempLocList.RemoveAll();
                }
                if (!GenericLocationsList.IsEmpty()) {
                    preturnlocationref = GenericLocationsList.m_item;
                    GenericLocationsList.RemoveHead();
                }
            }
        }
    }
    return preturnlocationref;
}

void MiddleHighProcess::EvaluateOrderAcquireList(Actor* apActor) {
    bool ballowKilling;
    bool ballowStealing;
    bool ballowBuying;
    BSSimpleList<ObjectstoAcquire*>* pRefItem = &TempObjectList;
    TESPackage* ppack = GetPackageThatIsRunning();
    if (ppack) {
        ballowBuying = ppack->SearchGetAllowBuying();
    }
    else {
        ballowBuying = 0;
    }
    if (ppack) {
        ballowStealing = ppack->SearchGetAllowStealing();
    }
    else {
        ballowStealing = 0;
    }
    if (ppack) {
        ballowKilling = ppack->SearchGetAllowKilling();
    }
    else {
        ballowKilling = 0;
    }
    while (pRefItem && pRefItem->m_item) {
        ObjectstoAcquire* pObject = pRefItem->m_item;
        TESObjectREFR* pRef = pObject->pRef;
        TESObjectREFR* pactor = 0;
        bool v2 = pRef->IsActor();
        Actor* bbuyit = v2 ? static_cast<Actor*>(pRef) : 0;
        TESBoundObject* pObj = pObject->pObject;
        char v26 = 0;
        bool badd = 1;
        if (bbuyit) {
            badd = !bbuyit->IsPlayerTeammate();
        }
        if (pRef->GetOwner() || pRef->IsActor()) {
            bool bsteal = 0;
            bool bkill = 0;
            int32_t ivalue = TESValueForm::GetFormValue(pObj);
            TESNPC* pnpc = 0;
            if (!pRef->IsDead(0)) {
                int32_t GoldAmount = apActor->GetGoldAmount();
                if (GoldAmount >= ivalue) {
                    Actor* pbuyactor = 0;
                    if (pRef->IsNpc()) {
                        pbuyactor = static_cast<Actor*>(pRef);
                    }
                    else {
                        TESForm* Owner = pRef->GetOwner();
                        pnpc = dynamic_cast<TESNPC*>(Owner);
                        if (pnpc) {
                            pbuyactor = ReferenceProcessLists.GetActorRefInHigh(pnpc, 0);
                        }
                    }
                    if (pbuyactor) {
                        if (ballowBuying && pbuyactor->GetActorBase()->SellBuysItem(pObj)) {
                            int32_t v5 = apActor->GetGoldAmount();
                            if (v5 >= TESValueForm::GetFormValue(pObject->pObject)) {
                                if (apActor->IsInCombat()) {
                                    badd = 0;
                                }
                                else {
                                    pObject->eType = ACQUIRE_TYPE_BUY_NPC;
                                    uint32_t DistanceFromReference =
                                        static_cast<int32_t>(apActor->GetDistanceFromReference(pRef, 0, 0));
                                    pObject->iWantValue = DistanceFromReference;
                                    v26 = 1;
                                }
                            }
                        }
                    }
                }
            }
            if (!v26 && (!bbuyit || !bbuyit->IsPlayerTeammate())) {
                if (pRef->IsActor()) {
                    if (pRef->IsDead(0)) {
                        badd = 1;
                    }
                    else {
                        if (pRef->IsNpc()) {
                            pactor = pRef;
                            if (ballowStealing) {
                                bsteal = 1;
                            }
                            if (ballowKilling) {
                                bkill = 1;
                            }
                        }
                    }
                }
                else {
                    if (ballowStealing) {
                        bsteal = 1;
                    }
                }
                if ((!bsteal && !bkill) || (bbuyit && bbuyit->IsPlayerTeammate())) {
                    if (apActor->IsCreature()) {
                        badd = 0;
                    }
                    else {
                        TESBoundObject* v7;
                        if (!pRef->IsActor() ||
                            (pRef->IsNpc() ||
                             ((v7 = static_cast<TESBoundObject*>(pRef->GetOwner()),
                               v7 == apActor->GetObjectReference()) ||
                              apActor->bPlayerTeammate))) {
                            badd = 0;
                        }
                        else {
                            if (bbuyit) {
                                if (!bbuyit->IsPlayerTeammate()) {
                                    pObject->eType = ACQUIRE_TYPE_KILL;
                                    badd = 1;
                                }
                            }
                        }
                    }
                }
                else {
                    if (pRef->IsActor()) {
                        if (bsteal) {
                            if (apActor->IsInCombat()) {
                                badd = 0;
                            }
                            else {
                                pObject->eType = ACQUIRE_TYPE_PICKPOCKET;
                            }
                        }
                        else {
                            if (bkill) {
                                if (apActor->IsInCombat() || apActor->bPlayerTeammate) {
                                    badd = 0;
                                }
                                else {
                                    if (!bbuyit || !bbuyit->IsPlayerTeammate()) {
                                        pObject->eType = ACQUIRE_TYPE_KILL;
                                    }
                                }
                            }
                        }
                    }
                    else {
                        if (pObject->eType == ACQUIRE_TYPE_NOOWNED_CONTAINER) {
                            PACKAGE_OBJECT_TYPE eUseItem = POTYPE_NONE;
                            pObject->eType = ACQUIRE_TYPE_PICKPOCKET;
                            if (!apActor->HasObjects(pLockpick, POTYPE_NONE, 1, 0, eUseItem)) {
                                if (pRef->GetLock()) {
                                    badd = 0;
                                }
                            }
                        }
                        else {
                            pObject->eType = ACQUIRE_TYPE_STEALGROUND;
                        }
                    }
                    pObject->bSteal = bsteal;
                    pObject->bKill = bkill;
                }
            }
        }
        else {
            double v8 = apActor->GetDistanceFromReference(pRef, 0, 0);
            pObject->iWantValue = static_cast<int32_t>(v8);
        }
        if (badd) {
            pObject->pRef->SetTargeted(1);
            ObjectList.AddTail(pObject);
        }
        else {
            if (pObject) {
                pObject->Delete(1u);
            }
        }
        pRefItem = pRefItem->m_pkNext;
    }
    static_cast<HighProcess*>(this)->HighProcess::SortObjectsListByType();
    static_cast<HighProcess*>(this)->HighProcess::SortObjectsListByValue();
    TempObjectList.RemoveAll();
}

bool MiddleHighProcess::ProcessCombatOneHour(Actor* apActor, float afTimeDelta) {
    int32_t inumber = static_cast<int32_t>(fCombatRoundAmount.Float() * 4.0);
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    Actor* ptarget = dynamic_cast<Actor*>(pTarget);
    if (pTarget && !pTarget->GetDisabled()) {
        if (ptarget && (apActor->GetDistanceFromReference(ptarget, 0, 0) < 250.0 && ptarget->pCurrentProcess)) {
            while (inumber && !apActor->IsDead(0)) {
                apActor->PerformOneRoundofCombat();
                if (!ptarget->IsDead(0)) {
                    ptarget->PerformOneRoundofCombat();
                }
                --inumber;
            }
        }
        else {
            ProcessFollowOneHour(apActor, 3600.0f, 0);
        }
        if (ptarget) {
            if (!ptarget->IsDead(0)) {
                BaseProcess* CurrentAIProcess = ptarget->pCurrentProcess;
                CurrentAIProcess->ComputeLastTimeProcessed();
            }
        }
        return 0;
    }
    else {
        apActor->StopCombat(ptarget);
        apActor->EndInterruptPackage(0);
        return 0;
    }
}

void MiddleHighProcess::SetAnimation(Animation* apAnimation) {
    if (pAnimation) {
        if (pAnimation == apAnimation) {
            return;
        }
        ourMemset(pAimSequence, 0, sizeof(pAimSequence));
        GarbageCollector_AddAnimation(pAnimation);
    }
    pAnimation = apAnimation;
}

bool MiddleHighProcess::SetupPathFinding(
    Actor* apActor,
    NiPoint3 aDestPoint,
    TESObjectCELL* apDestIntCell,
    TESWorldSpace* apDestWorld
) {
    if ((((int32_t)apActor->GetSitSleepState()) != 4) && (((int32_t)apActor->GetSitSleepState()) != 9)) {
        return (bool)((uint8_t)
                          apActor->SetPathfindingGoal(aDestPoint, apDestIntCell, apDestWorld, (-0x1p+0f), nullptr));
    }
    apActor->InitiateGetUpPackage();
    return (bool)0;
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
// (or activates the container for its owner), pays the seller and fires OnSell. While still pathing it works out the
// game time since the last update and then drops it, as the game does.
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
        if (!apActor->IsPathingComplete()) {
            TESGlobal* pTimeScale = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(0x3A));
            float fHour = appCalendar.GetHour();
            if (fHour < GetLastTimeProcessed()) {
                fHour = static_cast<float>(fHour + 24.0);
            }
            float fElapsed = fHour - GetLastTimeProcessed();
            fElapsed = static_cast<float>(3600.0 / pTimeScale->GetValue() * fElapsed);
            static_cast<void>(fElapsed);
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
                static_cast<TESBoundObject*>(pAcquireObject->pObject), nullptr, 1, false, false, apActor, nullptr,
                nullptr, true, false
            );
        }
        if (pSeller) {
            int32_t iValue = TESValueForm::GetFormValue(pAcquireObject->pObject);
            apActor->PayGoldToActor(apActor, static_cast<int32_t>(pAcquireObject->iamount * iValue));
        }
        ExtraDataList* pExtra = nullptr;
        pExtra = pAcquireObject->pExtra;
        Script::SetActionFlag(pSeller, pExtra, SCRIPT_ACTION_ONSELL);
    }
    if (pAcquireObject) {
        pAcquireObject->Delete(1);
    }
    SetTarget(nullptr);
    pAcquireObject = nullptr;
}

void MiddleHighProcess::ProcessSteal(Actor* apActor) {
    ProcessActivate(apActor, (bool)0);
}

void MiddleHighProcess::ProcessPickPocket(Actor* apActor) {
    ProcessActivate(apActor, (bool)0);
}

void MiddleHighProcess::ProcessDialogue(MobileObject* apActor) {
    DialoguePackage* v5 = static_cast<DialoguePackage*>(GetPackageThatIsRunning());
    MobileObject* PackageEvaluateHour = v5->GetTargetOfConversation();
    MobileObject* ActorStartedConversation = v5->GetActorStartedConversation();
    if (apActor == ActorStartedConversation) {
        ActorStartedConversation = PackageEvaluateHour;
    }
    if (!(v5->flistentimer > 0.0)) {
        if (GetCurrentActionComplete()) {
            apActor->EndDialogue();
            TESPackage* CurrentPackage = ActorStartedConversation->GetCurrentPackage();
            if (CurrentPackage == apActor->GetCurrentPackage()) {
                ActorStartedConversation->EndDialogue();
            }
            SetCurrentActionComplete(0);
        }
        else {
            v5->Speak(0);
        }
    }
    else {
        v5->flistentimer = v5->flistentimer - appTimer.GetSeconds();
    }
}

bool MiddleHighProcess::ProcessDialogueActivate(Actor* apActor) {
    TESPackage* pPackage = GetPackageThatIsRunning();
    if (pPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        if (pTarget || SetTargetForPackage(apActor)) {
            DialoguePackage* th_s = static_cast<DialoguePackage*>(GetPackageThatIsRunning());
            PlayerCharacter* a2 = static_cast<PlayerCharacter*>(th_s->pTargetActor);
            MobileObject* ActorStartedConversation = th_s->GetActorStartedConversation();
            if (th_s->IsCreated() || !th_s->GetPackageSecondLocation()) {
                if (apActor == a2) {
                    AddToProcedureIndexRunning(apActor, 1);
                }
                else {
                    ProcessActivate(apActor, 1);
                }
                return 1;
            }
            else {
                if (th_s->IsActorAtLocation(ActorStartedConversation, 0, -1.0f, 1) &&
                    th_s->IsActorAtSecondLocation(a2, apActor, 0, -1.0f, 1)) {
                    if (a2 == pPlayer) {
                        apActor->Activate(a2, 0, 0, 1);
                    }
                    else {
                        AddToProcedureIndexRunning(apActor, 2);
                    }
                }
                else {
                    if (a2 == apActor && a2->IsActor()) {
                        if (a2 != pPlayer && !th_s->GetDialogueDoNotControlTarget()) {
                            BaseProcess* CurrentAIProcess = a2->pCurrentProcess;
                            CurrentAIProcess->ProcessTravel(a2, 0, 0, -1, 1);
                        }
                    }
                    else {
                        ProcessTravel(apActor, 0, 0, 257, 0);
                    }
                }
                return 1;
            }
        }
        else {
            AddToProcedureIndexRunning(apActor, 3);
            return 0;
        }
    }
    else {
        SetTargetForPackage(apActor);
        AddToProcedureIndexRunning(apActor, 3);
        apActor->InitiateDialogue(pPackage);
        return 1;
    }
}

bool MiddleHighProcess::ProcessUseFurniture(Actor* apActor, TESObjectREFR* apFurniture) {
    BSFaceGenAnimationData* faceAnimationData;
    GetPackageThatIsRunning();
    if ((!apFurniture) || (!apFurniture->IsFurniture())) {
        return (bool)0;
    }
    if (pCurrentFurniture != apFurniture) {
        if (apActor->GetSitSleepState()) {
            if ((((int32_t)apActor->GetSitSleepState()) == 4) || (((int32_t)apActor->GetSitSleepState()) == 9)) {
                apActor->InitiateGetUpPackage();
            }
            return (bool)1;
        }
        pCurrentFurniture = apFurniture;
        ClearFurnitureMarker();
    }
    if (!pCurrentFurniture) {
        ClearFurnitureMarker();
    }
    TESFurniture* objectReference = static_cast<TESFurniture*>(pCurrentFurniture->TESObjectREFR::GetObjectReference());
    int32_t v6 = objectReference->CanSitOn() ? 4 : 9;
    if ((((int32_t)apActor->GetSitSleepState()) != v6) && apActor->IsNpc()) {
        if (!pTarget) {
            SetTarget(pCurrentFurniture);
        }
        ProcessActivate(apActor, (bool)0);
    }
    if (((int32_t)apActor->GetSitSleepState()) == v6) {
        SetCurrentActionComplete((bool)1u);
    }
    if (DidPathingFail(apActor)) {
        pCurrentFurniture = nullptr;
        (&FurnitureMarker)->SetHeading(0.0);
        (&FurnitureMarker)->SetType(0);
        (&FurnitureMarker)->SetPosition(NiPoint3_ZERO);
        EndMoveMessage(apActor);
        AddToProcedureIndexRunning(apActor, (int32_t)1u);
        return (bool)0;
    }
    else {
        if (((int32_t)apActor->GetSitSleepState()) == 9) {
            faceAnimationData = apActor->GetFaceAnimationData();
            if (faceAnimationData) {
                if (!faceAnimationData->GetDead()) {
                    faceAnimationData->SetDead((bool)1, (bool)0);
                }
            }
        }
        return (bool)1;
    }
}

void MiddleHighProcess::ProcessRemoveWorn(Actor* apActor, bool bInterateindex, bool bOnlyifClothes) {
    BaseProcess* pBaseProcess;
    TESBoundObject* objectReference = apActor->TESObjectREFR::GetObjectReference();
    TESNPC* bIsNPC = dynamic_cast<TESNPC*>(objectReference);
    TESBoundObject* v5 = apActor->TESObjectREFR::GetObjectReference();
    TESCreature* bIsCreature = dynamic_cast<TESCreature*>(v5);
    TESPackage* pPackage = GetCurrentPackage();
    uint8_t v12 = 1;
    uint8_t v10 = 1;
    if (pPackage) {
        if (((int32_t)pPackage->GetPackType()) == 4) {
            bOnlyifClothes = (bool)1;
            v12 = 0;
            v10 = 0;
        }
        else {
            v10 = (uint8_t)(!pPackage->GetForceUnequipWeapon());
        }
    }
    if (bIsNPC) {
        bIsNPC->InitDefaultWorn(apActor, v12, v10, bOnlyifClothes, 1);
    }
    else {
        if (bIsCreature) {
            bIsCreature->InitDefaultWorn(apActor, v12, v10, 1);
        }
    }
    if (apActor) {
        if (apActor->MobileObject::GetCurrentAIProcess()) {
            pBaseProcess = apActor->MobileObject::GetCurrentAIProcess();
            pBaseProcess->Set3DUpdateFlag(RESET_MODEL);
        }
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
    TESObjectREFR* pref = apData->GetCurrentReference();
    if (pref) {
        SetTarget(pref);
        float fradius = PathingRequest::GetDefaultRadius();
        TESPackage* ppackage = GetPackageThatIsRunning();
        if (ppackage) {
            float a2 = static_cast<float>(ppackage->GetLocationRadius(apActor));
            fradius = BSMAX(fradius, a2);
        }
        TESBoundObject* plocBound = pref->GetObjectReference();
        int32_t FormType = plocBound->GetFormType();
        bool bisIdleMarker = FormType == 48;
        if (FormType == 48) {
            fradius = 10.0f;
        }
        PathingRequest request;
        TESWorldSpace* WorldSpace = pref->GetWorldSpace();
        TESObjectCELL* ParentCell = pref->GetParentCell();
        const NiPoint3* v4 = &pref->GetLocationOnReference();
        apActor->BuildRequest(&request, *v4, ParentCell, WorldSpace, fradius, 0);
        request.SetSoftGoalRadius(0);
        if (bisIdleMarker || plocBound == pXMarkerHeading) {
            request.SetGoalAngleToFace((&pref->GetAngleOnReference())->z);
        }
        apActor->SetPathfindingGoal(&request);
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
    TESObjectREFR* pref = apData->GetCurrentReference();
    if (pref) {
        int32_t emoveMode = 257;
        TESPackage* pPack = GetPackageThatIsRunning();
        bool balwaysRun = pPack->GetAlwaysRun();
        bool bcouldRun = 0;
        if (balwaysRun) {
            float fcurrentDistance = apActor->GetDistanceFromReference(pref, 0, 0);
            float ftooCloseToRun = 64.0f;
            bcouldRun = 64.0f < static_cast<double>(fcurrentDistance);
            if (64.0f < static_cast<double>(fcurrentDistance)) {
                float fdistance = pPack->GetPackageRadiusActorToLocation(apActor, 0);
                float v9 = fdistance + fdistance;
                emoveMode = CalculateMoveMode(apActor, fcurrentDistance, fdistance, v9, 0, 0);
            }
        }
        SetActorsAnimation(apActor, emoveMode, bcouldRun);
    }
    if (pref && pref->GetObjectReference()->GetFormType() == FURN_ID) {
        ProcessUseFurniture(apActor, pref);
        if (pref == pCurrentFurniture &&
            (GetSitSleepState() == SIT_SLEEP_WAITING_FOR_SIT_ANIM ||
             GetSitSleepState() == SIT_SLEEP_WAITING_FOR_SLEEP_ANIM)) {
            ProcessPatrolStartAtRefBehavior(apActor, apData);
        }
        return;
    }
    bool bpauseAtWaypoint = 0;
    TESPackage* pPack_2 = GetPackageThatIsRunning();
    if (pPack_2) {
        BGSIdleCollection* pidleCollection = pPack_2->pIdleCollection;
        if (pidleCollection) {
            bpauseAtWaypoint = pidleCollection->GetIdleCount() > 0;
        }
    }
    bool bfinished = apActor->IsPathingComplete();
    float fcurrentpercent = apActor->pActorMover->GetCurrentPercent();
    float ftestvalue = 0.925f;
    if (bfinished || (!bpauseAtWaypoint && ftestvalue < static_cast<double>(fcurrentpercent))) {
        float fminlength;
        float v12;
        const NiPoint3* a2 = &pref->GetLocationOnReference();
        const NiPoint3* v4 = &apActor->GetLocationOnReference();
        NiPoint3 diff = *v4 - *a2;
        float fsqrLength = diff.SqrXYLength();
        float fradius = 32.0f;
        if (pref->GetObjectReference()->GetFormType() == IDLM_ID) {
            fradius = PathingRequest::GetDefaultRadius();
        }
        else {
            TESBoundObject* ObjectReference = pref->GetObjectReference();
            if (ObjectReference == pXMarkerHeading) {
                fradius = PathingRequest::GetDefaultRadius();
            }
            TESPackage* ppackage = GetCurrentPackage();
            if (ppackage) {
                float a2a = static_cast<float>(ppackage->GetLocationRadius(apActor));
                fradius = BSMAX(fradius, a2a);
            }
        }
        float fradiusSrq = fradius * fradius;
        bool bgoToNext = 0;
        if (bfinished) {
            goto LABEL_35;
        }
        fminlength = fradius + 50.0;
        v12 = fminlength * fminlength;
        if (!(v12 < static_cast<double>(fsqrLength)) && !(diff.z * diff.z > 32400.0)) {
            float ftimeAtRef;
            ftimeAtRef = 0.0f;
            if ((&pref->m_Extra)->GetPatrolRefData()) {
                PatrolRefData* PatrolRefData_ = (&pref->m_Extra)->GetPatrolRefData();
                ftimeAtRef = PatrolRefData_->fTimeAtRef;
            }
            if (ftimeAtRef == 0.0) {
                bgoToNext = 1;
            LABEL_35:;
                if (bgoToNext || (!(fradiusSrq < static_cast<double>(fsqrLength)) || !(diff.z * diff.z < 32400.0))) {
                    ProcessPatrolStartAtRefBehavior(apActor, apData);
                }
                else {
                    ProcessPatrolStartTravelToNextRef(apActor, apData);
                }
            }
        }
    }
}

void MiddleHighProcess::ProcessPatrolStartAtRefBehavior(Actor* apActor, PatrolActorPackageData* apData) {
    StartAtPatrolRefBehaviorLow(apActor, apData);
    apActor->ClearLookAtTarget();
    TESObjectREFR* pref = apData->GetCurrentReference();
    if (!pref || pref->GetObjectReference()->GetFormType() != IDLM_ID) {
        fPackageIdleTimer = 0.0f;
    }
    PatrolRefData* ppatrolRefData = (&pref->m_Extra)->GetPatrolRefData();
    if (ppatrolRefData) {
        TESTopic* ptopic = ppatrolRefData->GetOnPatrolTopic();
        if (ptopic) {
            BaseProcess* pProcess = apActor->pCurrentProcess;
            if (!pProcess->GetGreetingFlag()) {
                ProcessGreet(apActor, ptopic, 0, 0, 1, 0);
            }
        }
        Script* pscript = ppatrolRefData->GetOnPatrolScript();
        if (pscript) {
            pscript->Run(apActor, 0, 0, 0);
        }
    }
}

void MiddleHighProcess::ProcessPatrolAtRef(Actor* apActor, PatrolActorPackageData* apData) {
    NiPoint3 result;
    TESObjectREFR* CurrentReference = apData->GetCurrentReference();
    if (!CurrentReference) {
        if (apData->SetNextReferenceForPatrol(1)) {
            ProcessPatrolStartTravelToNextRef(apActor, apData);
        }
        else {
            SetProcedureIndexRunning(-1);
        }
        return;
    }
    if (apActor->IsPathingComplete()) {
        const NiPoint3* v3;
        TESBoundObject* ObjectReference;
        const NiPoint3* a2;
        float DefaultRadius;
        float v15;
        a2 = &CurrentReference->GetLocationOnReference();
        v3 = &apActor->GetLocationOnReference();
        result = *v3 - *a2;
        v15 = result.SqrXYLength();
        DefaultRadius = 32.0f;
        ObjectReference = CurrentReference->GetObjectReference();
        if (ObjectReference->GetFormType() == 48) {
            DefaultRadius = PathingRequest::GetDefaultRadius();
        }
        else {
            if (CurrentReference->GetObjectReference() == pXMarkerHeading) {
                DefaultRadius = PathingRequest::GetDefaultRadius();
            }
            TESPackage* v12 = GetCurrentPackage();
            if (v12) {
                float a2a = static_cast<float>(v12->GetLocationRadius(apActor));
                DefaultRadius = BSMAX(DefaultRadius, a2a);
            }
        }
        DefaultRadius = DefaultRadius * DefaultRadius;
        if (DefaultRadius < static_cast<double>(v15) || std::fabsf(result.z) > 180.0) {
        LABEL_24:;
            ProcessPatrolStartTravelToNextRef(apActor, apData);
            return;
        }
    }
    float DecalMinWidth = 0.0;
    ExtraDataList* Extra = CurrentReference->GetExtra();
    if (Extra->GetPatrolRefData()) {
        ExtraDataList* v6 = CurrentReference->GetExtra();
        PatrolRefData* PatrolRefData_ = v6->GetPatrolRefData();
        DecalMinWidth = PatrolRefData_->GetTimeAtRef();
    }
    if (DecalMinWidth > 0.0) {
        TESBoundObject* v8 = CurrentReference->GetObjectReference();
        if (v8->GetFormType() == 48) {
            ProcessUseIdleMarker(apActor, CurrentReference);
        }
    }
    apData->fCurrentTimeAtReference = appTimer.GetSeconds() + apData->fCurrentTimeAtReference;
    if (DecalMinWidth <= static_cast<double>(apData->fCurrentTimeAtReference) &&
        !apActor->IsPlayingLowerBodySpecialIdle()) {
        if (!apData->SetNextReferenceForPatrol(1)) {
            AddToProcedureIndexRunning(apActor, 1);
            return;
        }
        goto LABEL_24;
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
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucTrueState), 1);
    pController->SetTrueState(static_cast<hkpCharacterStateType>(ucTrueState));
    uint8_t ucWantState = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucWantState), 1);
    pController->eWantState = static_cast<hkpCharacterStateType>(ucWantState);
    uint32_t uiFlags = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&uiFlags), 4);
    pController->bhkCharacterListener::ClearFlag(0xFFFFFFFF);
    pController->bhkCharacterListener::SetFlag(uiFlags, true);
    // 0x800: the character's collision was disabled.
    if (uiFlags & 0x800) {
        NiPoint3 kPosition;
        kPosition.~NiPoint3();
        ::new (&kPosition) NiPoint3;
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&kPosition), 0xC);
        pController->SetPosition(kPosition);
    }
    if (pActor && pActor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
        pActor->SetChaseBip(false);
    }
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pController->OutVelocity), 0x10);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pController->VelocityMod), 0x10);
    alignas(16) NiPoint3 kVelocity;
    kVelocity.~NiPoint3();
    ::new (&kVelocity) NiPoint3;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&kVelocity), 0xC);
    pController->SetLinearVelocityNi(kVelocity);
    if (pActor) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pController->fJumpHeight), 4);
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pController->fFallStartHeight), 4);
    }
    else {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pController->fGravity), 4);
    }
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pController->fFallTime), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pController->fPitchAngle), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pController->fPitchMult), 4);
    SIT_SLEEP_STATE eSitSleepState = pActor ? pActor->GetSitSleepState() : SIT_SLEEP_NORMAL;
    if (eSitSleepState == SIT_SLEEP_IS_SITTING || eSitSleepState == SIT_SLEEP_IS_SLEEPING) {
        if (ShouldDisableCharController(pCurrentFurniture, FurnitureMarker.GetType())) {
            // 0x8000000: no character collisions (CL_NO_CHARACTER_COLLISIONS).
            pController->bhkCharacterListener::SetFlag(0x8000000, true);
        }
        pController->SetTrueState(HK_CHARACTER_ON_GROUND);
        pController->SetLinearVelocity(hkVector4::getZero());
        alignas(16) hkVector4 kNormal(0.0f, 0.0f, 1.0f, 0.0f);
        pController->SetSupportNormal(kNormal);
    }
}

void MiddleHighProcess::LoadCharController(MobileObject* pmob) {
    NiPoint3 pos;
    uint32_t iflags;
    hkpCharacterStateType charState;
    NiPoint3 linearVelocity;
    bhkCharacterController* pcon = GetCharController();
    if (pcon) {
        if (!pmob->IsDead(0)) {
            pSaveLoadGameOLD->LoadGameDataOLD(&charState, 4u);
            pcon->SetTrueState(charState);
            pSaveLoadGameOLD->LoadGameDataOLD(&pcon->eWantState, 4u);
            pSaveLoadGameOLD->LoadGameDataOLD(&iflags, 4u);
            pcon->ClearFlag(0xFFFFFFFFu);
            pcon->SetFlag(iflags, 1);
            if ((iflags & 2048) != 0 && pSaveLoadGameOLD->GetCurrentVersion() >= 35u) {
                pSaveLoadGameOLD->LoadGameDataOLD(&pos, 12u);
                if (GetSitSleepState()) {
                    if (GetFurnitureMarker()) {
                        MobileObject* pactor = 0;
                        if (pmob->IsActor()) {
                            pactor = pmob;
                        }
                        if (!pactor) {
                            pos = GetFurnitureMarker()->Position;
                        }
                    }
                }
                pcon->SetPosition(pos);
            }
            if (GetSitSleepState() == SIT_SLEEP_NORMAL) {
                pmob->SetChaseBip(0);
            }
            pSaveLoadGameOLD->LoadGameDataOLD(&pcon->OutVelocity, 16u);
            pSaveLoadGameOLD->LoadGameDataOLD(&pcon->VelocityMod, 16u);
            pSaveLoadGameOLD->LoadGameDataOLD(&linearVelocity, 12u);
            pcon->SetLinearVelocityNi(linearVelocity);
            if (pmob->IsActor()) {
                pSaveLoadGameOLD->LoadGameDataOLD(&pcon->fJumpHeight, 4u);
                pSaveLoadGameOLD->LoadGameDataOLD(&pcon->fFallStartHeight, 4u);
                pSaveLoadGameOLD->LoadGameDataOLD(&pcon->fFallTime, 4u);
            }
            else {
                if (pSaveLoadGameOLD->GetCurrentVersion() >= 32u) {
                    pSaveLoadGameOLD->LoadGameDataOLD(&pcon->fGravity, 4u);
                    pSaveLoadGameOLD->LoadGameDataOLD(&pcon->fFallTime, 4u);
                }
            }
            if (charState == HK_CHARACTER_IN_AIR && (pcon->QSupported() || pcon->QSupported2())) {
                pcon->fFallStartHeight = 0.0f;
            }
            if (pSaveLoadGameOLD->GetCurrentVersion() >= 119u) {
                pSaveLoadGameOLD->LoadGameDataOLD(&pcon->fPitchAngle, 4u);
                pSaveLoadGameOLD->LoadGameDataOLD(&pcon->fPitchMult, 4u);
            }
        }
    }
}

void MiddleHighProcess::SaveCharController(MobileObject* pmob) {
    NiPoint3 pos;
    uint32_t iflags;
    hkpCharacterStateType charState;
    NiPoint3 linearVelocity;
    bhkCharacterController* pcon = GetCharController();
    if (pcon) {
        if (!pmob->IsDead(0)) {
            charState = pcon->getState();
            pSaveLoadGameOLD->SaveGameDataOLD(&charState, 4u);
            pSaveLoadGameOLD->SaveGameDataOLD(&pcon->eWantState, 4u);
            iflags = pcon->GetFlag(0xFFFFFFFFu);
            pSaveLoadGameOLD->SaveGameDataOLD(&iflags, 4u);
            if ((iflags & 2048) != 0) {
                pcon->GetPosition(pos);
                pSaveLoadGameOLD->SaveGameDataOLD(&pos, 12u);
            }
            pSaveLoadGameOLD->SaveGameDataOLD(&pcon->OutVelocity, 16u);
            pSaveLoadGameOLD->SaveGameDataOLD(&pcon->VelocityMod, 16u);
            pcon->GetLinearVelocity(linearVelocity);
            pSaveLoadGameOLD->SaveGameDataOLD(&linearVelocity, 12u);
            if (pmob->IsActor()) {
                pSaveLoadGameOLD->SaveGameDataOLD(&pcon->fJumpHeight, 4u);
                pSaveLoadGameOLD->SaveGameDataOLD(&pcon->fFallStartHeight, 4u);
            }
            else {
                pSaveLoadGameOLD->SaveGameDataOLD(&pcon->fGravity, 4u);
            }
            pSaveLoadGameOLD->SaveGameDataOLD(&pcon->fFallTime, 4u);
            if (pSaveLoadGameOLD->GetCurrentVersion() >= 119u) {
                pSaveLoadGameOLD->SaveGameDataOLD(&pcon->fPitchAngle, 4u);
                pSaveLoadGameOLD->SaveGameDataOLD(&pcon->fPitchMult, 4u);
            }
        }
    }
}

// Returns true on every path.
bool MiddleHighProcess::ReloadDynamicIdleOn1stPerson(Actor* apActor) {
    int32_t idleAnimGroupSection;
    Animation* animation;
    TESIdleForm* apIdleForm;
    uint8_t bThirdPerson = (uint8_t)pPlayer->Is3rdPerson();
    if (static_cast<const void*>(apActor) == static_cast<const void*>(pPlayer)) {
        if (bThirdPerson) {
            pPlayer->Force3rdPerson(0);
        }
        SetSitSleepState(apActor, SIT_SLEEP_LOAD_SIT_IDLE, pCurrentFurniture, cCurrentFurnitureIndex);
        apIdleForm = pIdleManager->GetIdleToPlay(apActor, pCurrentFurniture);
        SetSitSleepState(apActor, SIT_SLEEP_IS_SITTING, pCurrentFurniture, cCurrentFurnitureIndex);
        if (apIdleForm) {
            animation = pPlayer->GetAnimation((bool)1);
            if (animation) {
                idleAnimGroupSection = apIdleForm->GetIdleAnimGroupSection();
                animation->SpecialIdleReplace(
                    apIdleForm, apActor, static_cast<ANIM_GROUP_SECTION>(idleAnimGroupSection)
                );
            }
        }
        if (bThirdPerson) {
            pPlayer->Force3rdPerson(1);
        }
    }
    return true;
}

void MiddleHighProcess::ForceBoneScale() {}

void MiddleHighProcess::ClearReservation() {
    if (!pDataHandler->GetClearingData()) {
        if (pTarget) {
            if (((int32_t)iReservationSlot) >= 0) {
                pTarget->SetMarkerReserved(iReservationSlot, 0);
                iReservationSlot = (uint32_t)((-1));
            }
        }
        if (pCurrentFurniture) {
            if (!pSaveLoadGame->GetSaveGameLoading()) {
                pCurrentFurniture->SetMarkerReserved(cCurrentFurnitureIndex, 0);
            }
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
    apProcess->pAnimation = 0;
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
    apProcess->pFireNode = 0;
    apProcess->pWeapon = 0;
    apProcess->pAmmo = 0;
    apProcess->bWeaponGrenade = 0;
    apProcess->bWeaponThrown = 0;
    apProcess->bWeaponMine = 0;
    eUseItem = apProcess->eUseItem;
    fEquippedWeight = apProcess->fEquippedWeight;
    pLastIdlePlayed = apProcess->pLastIdlePlayed;
    for (void*& rpItem : apProcess->DeadDetectList.ItemsUntilNull()) {
        DeadDetectList.AddHead(*reinterpret_cast<void**>(&rpItem));
    }
    for (QueuedItem*& rpItem : apProcess->pListItemstoEquipUnequip->ItemsUntilNull()) {
        QueuedItem* pitem_2;
        if (!pListItemstoEquipUnequip) {
            BSSimpleList<QueuedItem*>* v4;
            BSSimpleList<QueuedItem*>* v9 =
                static_cast<BSSimpleList<QueuedItem*>*>(ourMalloc(sizeof(BSSimpleList<QueuedItem*>)));
            if (v9) {
                v4 = ::new (v9) BSSimpleList<QueuedItem*>;
            }
            else {
                v4 = 0;
            }
            BSSimpleList<QueuedItem*>* v10 = v4;
            pListItemstoEquipUnequip = v4;
        }
        QueuedItem* pcopy = rpItem;
        void* apMemory = ourMalloc(sizeof(QueuedItem));
        if (apMemory) {
            pitem_2 = ::new (apMemory) QueuedItem;
        }
        else {
            pitem_2 = 0;
        }
        QueuedItem* v8 = pitem_2;
        QueuedItem* pitem_3 = pitem_2;
        pitem_2->pObject = pcopy->pObject;
        pitem_3->bEquip = pcopy->bEquip;
        pitem_3->iNumber = pcopy->iNumber;
        pitem_3->pExtra = pcopy->pExtra;
        pitem_3->bNow = pcopy->bNow;
        pitem_3->bForceEquip = pcopy->bForceEquip;
        pitem_3->bForceUnequip = pcopy->bForceUnequip;
        pitem_3->bEquipCalling = pcopy->bEquipCalling;
        pitem_3->bPlayPickUpSounds = pcopy->bPlayPickUpSounds;
        pitem_3->spQueuedFiles = pcopy->spQueuedFiles;
        pListItemstoEquipUnequip->AddHead(pitem_3);
    }
    bBeenAttacked = apProcess->HasBeenAttacked();
    cSitSleepState = apProcess->cSitSleepState;
    pCurrentFurniture = apProcess->pCurrentFurniture;
    if (pCurrentFurniture) {
        pCurrentFurniture->SetTargeted(1);
    }
    cCurrentFurnitureIndex = apProcess->cCurrentFurnitureIndex;
    ourMemcpy(&FurnitureMarker, &apProcess->FurnitureMarker, sizeof(FurnitureMarker));
    ourMemcpy(pDamageRootNode, apProcess->pDamageRootNode, sizeof(pDamageRootNode));
    pHeadNode = apProcess->pHeadNode;
    pTorsoNode = apProcess->pTorsoNode;
    pLightingProperty.operator=(apProcess->GetLightingShaderProperty());
    pActiveEffectList->RemoveAll();
    for (BSSimpleList<ActiveEffect*>* plist = apProcess->GetActiveEffectList(); plist && !plist->IsEmpty();
         plist = plist->m_pkNext) {
        ActiveEffect* m_item = plist->m_item;
        ActiveEffect* item = m_item->Copy();
        pActiveEffectList->AddHead(item);
    }
    pCommandingActor = apProcess->pCommandingActor;
    pLastBoundWeapon = apProcess->pLastBoundWeapon;
    spCharController.operator=(apProcess->spCharController);
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
    apProcess->pAttachedArrowList = 0;
    spWeaponPreload.operator=(apProcess->spWeaponPreload);
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
    if (apAnim) {
        if (apAnim->GetControllerManager()) {
            NiNode* pweaponBone = GetWeaponBone(apBiped);
            BSAnimGroupSequence* pseq = apAnim->GetCurrentSequence(AGS_WEAPON);
            ANIM_GROUP_ENUM egroup = static_cast<ANIM_GROUP_ENUM>(abDrawn + 23);
            if (!pseq ||
                (animGroupInfo[pseq->GetAnimGroup()->GetType()].eAction != AGAT_ATTACK_MINE &&
                 animGroupInfo[pseq->GetAnimGroup()->GetType()].eAction != AGAT_ATTACK_THROW)) {
                if (!pseq ||
                    (pseq->GetAnimGroup()->GetType() != ANIM_GROUP_EQUIP &&
                     pseq->GetAnimGroup()->GetType() != ANIM_GROUP_UNEQUIP)) {
                    uint16_t sgroup = apActor->GetAnimGroup(egroup, pWeapon, pWeapon == 0, apAnim);
                    if (TESAnimGroup::GetType(sgroup) == egroup) {
                        pseq = apAnim->PlayGroup(sgroup, ACTION_START, -1, AGS_NONE);
                        if (abDrawn) {
                            SetAnimAction(ANIM_ACTION_EQUIP_WEAPON, pseq);
                        }
                    }
                }
                if (pseq) {
                    NiNode* p3d;
                    if (apActor->TESObjectREFR::IsDead(0)) {
                        float a2 = pseq->m_fEndKeyTime;
                        pseq->SetOffset(a2);
                        float a2a = pseq->m_fEndKeyTime;
                        apAnim->Update(apActor, 0.0f, a2a);
                        apAnim->UpdateMovement(apActor);
                    }
                    if (apBiped) {
                        p3d = apBiped->root;
                    }
                    else {
                        p3d = 0;
                    }
                    NiNode* p3d_2 = p3d;
                    if (!p3d) {
                        p3d_2 = apActor->Get3D();
                    }
                    if (p3d_2) {
                        if (!pweaponBone) {
                            if (pWeapon) {
                                SetCurrentWeapon(pWeapon, p3d_2, 0);
                                pweaponBone = GetWeaponBone(apBiped);
                            }
                        }
                    }
                    TESAnimGroup* AnimGroup = pseq->GetAnimGroup();
                    NiNode* pnewParent =
                        static_cast<NiNode*>(BSUtilities::GetObjectByName(p3d_2, AnimGroup->pParentName));
                    if (pnewParent) {
                        pnewParent->AttachChild(pweaponBone, 1);
                        apActor->SetHavokWeapon();
                    }
                    else {
                        Warning(
                            "ANIMATION: Animation '%s' doesn't have a 'prn: ...' to reparent the weapon bone.",
                            (&pseq->m_kName)->GetHandleString()
                        );
                    }
                }
                else {
                    const char* v9;
                    const char* FullName;
                    if (pWeapon) {
                        FullName = pWeapon->GetFullName();
                    }
                    else {
                        FullName = "NONE";
                    }
                    if (abDrawn) {
                        v9 = "Equip";
                    }
                    else {
                        v9 = "Holster";
                    }
                    int32_t fSecondsPassed = apActor->iFormID;
                    Warning(
                        "AI: Missing %s animation for '%s' (%08X) weapon '%s'.", v9, apActor->GetFormEditorID(),
                        fSecondsPassed, FullName
                    );
                }
            }
        }
    }
}

void MiddleHighProcess::ClearFurnitureMarker() {
    if (!pSaveLoadGame->GetSaveGameLoading()) {
        if (pCurrentFurniture) {
            if (!pDataHandler->GetClearingData()) {
                pCurrentFurniture->SetMarkerReserved(cCurrentFurnitureIndex, 0);
            }
        }
    }
    cCurrentFurnitureIndex = (uint8_t)127;
    (&FurnitureMarker)->SetHeading(0.0);
    (&FurnitureMarker)->SetType(0);
    (&FurnitureMarker)->SetPosition(NiPoint3_ZERO);
}

uint16_t MiddleHighProcess::GetCharControllerSaveSize(MobileObject* pmob) {
    int16_t v4;
    int16_t v5;
    int16_t v6;
    int16_t v3 = 0;
    bhkCharacterController* v7 = GetCharController();
    if (v7) {
        if (!pmob->IsDead((bool)0)) {
            v4 = 12;
            if ((static_cast<bhkCharacterListener*>(v7)->GetFlag(0xFFFFFFFFu) & 2048) != 0) {
                v4 = 24;
            }
            v5 = (int16_t)(v4 + 44);
            if (pmob->IsActor()) {
                v6 = (int16_t)(v5 + 8);
            }
            else {
                v6 = (int16_t)(v5 + 4);
            }
            v3 = (int16_t)(v6 + 4);
            if (((uint32_t)((int32_t)pSaveLoadGameOLD->GetCurrentVersion())) >= 0x77u) {
                v3 += 8;
            }
        }
    }
    return (uint16_t)v3;
}

bool MiddleHighProcess::ProcessEscortOneHour(Actor* apActor) {
    float v59;
    Script* v54 = 0;
    TESPackage* pPack = GetPackageThatIsRunning();
    if (!pPack) {
        return 0;
    }
    ItemChange* pitem = apActor->GetItemBeingEscorted();
    bool bManyflag = 0;
    if (!pitem) {
        uint32_t v3;
        if (!pTarget || (!pTarget->IsActor() && (v3 = pTarget->iFormID, !apActor->HasObjectId(v3)))) {
            uint32_t CastingType;
            SetTargetForPackage(apActor);
            if (!pTarget ||
                (!pTarget->IsActor() && (CastingType = pTarget->iFormID, !apActor->HasObjectId(CastingType)))) {
                AddToProcedureIndexRunning(apActor, 1);
                if (!apActor->IsPathingComplete()) {
                    EndMoveMessage(apActor);
                }
                return 0;
            }
        }
    }
    if (!pitem) {
        if (this->pAcquireObject) {
            if (this->pAcquireObject->pRef == apActor) {
                pitem = apActor->GetInventoryItem(this->pAcquireObject->pObject, 0);
                bManyflag = 1;
            }
        }
    }
    bool bIsActor = 0;
    if (pTarget) {
        if (pTarget->IsActor() && pTarget != apActor) {
            bIsActor = 1;
            Actor* pact = dynamic_cast<Actor*>(pTarget);
            BaseProcess* CurrentAIProcess = pact->pCurrentProcess;
            TESPackage* prun = CurrentAIProcess->GetCurrentPackage();
            if (pTarget != pPlayer &&
                (!prun || (prun->GetPackType() != PACKAGE_TYPE_FOLLOW && !prun->GetIsCreated()))) {
                SetProcedureIndexRunning(0);
                return 0;
            }
        }
    }
    NiPoint3 distance = apActor->GetLocationOnReference();
    if (bIsActor) {
        const NiPoint3* v17 = &pTarget->GetLocationOnReference();
        const NiPoint3* v5 = &apActor->GetLocationOnReference();
        distance = *v5 - *v17;
    }
    float fwaitdistance = 0.0f;
    TESObjectREFR* ptarg = 0;
    if (bIsActor) {
        ptarg = pTarget;
        fwaitdistance = 0.0f;
        int32_t ipackdist = pPack->GetPackageTarget()->iValue;
        if (ipackdist <= 0) {
            ipackdist = 200;
        }
        if (pTarget == pPlayer && fwaitdistance == 0.0) {
            v59 = static_cast<float>(ipackdist);
        }
        else {
            if (fwaitdistance == 0.0 && apActor->GetParentCell()->GetInterior()) {
                fwaitdistance = fAIEscortWaitDistanceInterior.Float();
            }
            else {
                if (fwaitdistance == 0.0) {
                    double v25 = static_cast<double>(ipackdist);
                    fwaitdistance = fAIEscortWaitDistanceExterior.Float() * v25;
                }
            }
        }
    }
    if (pPack->pPackLoc && pPack->IsActorAtLocation(apActor, 0, -1.0f, 0)) {
        NiPoint3 ftargdistance;
        NiPoint3 fMydistance;
        if (!bIsActor) {
            PackageLocation* ppackloc;
            PackageTarget* ppacktarg_2;
            ppackloc = pPack->pPackLoc;
            ppacktarg_2 = pPack->GetPackageTarget();
            if (pitem) {
                ExtraDataList* plist = 0;
                if (pitem->pExtraObjectList) {
                    plist = pitem->pExtraObjectList->m_item;
                }
                uint32_t id = 0;
                if (plist) {
                    if (plist->GetReferencePointer()) {
                        TESObjectREFR* ReferencePointer = plist->GetReferencePointer();
                        id = ReferencePointer->iFormID;
                    }
                }
                TESObjectREFR* pcont = pPack->pPackLoc->GetLocReference();
                if (!pcont) {
                    pcont = pGenericLocation;
                }
                if (pcont && pcont->HasContainer()) {
                    int32_t InitialTargetCount = pPack->GetInitialTargetCount();
                    apActor->PutItemInReference(pitem->pContainerObj, plist, pcont, InitialTargetCount, id);
                }
                else {
                    if (pitem->pExtraObjectList) {
                        plist = pitem->pExtraObjectList->m_item;
                    }
                    const NiPoint3* ppoint = 0;
                    if (ppackloc) {
                        TESObjectREFR* pref = ppackloc->GetLocReference();
                        if (!pref) {
                            pref = pGenericLocation;
                        }
                        if (pref) {
                            TESBoundObject* v9;
                            TESBoundObject* ObjectReference = pref->GetObjectReference();
                            if (ObjectReference == pXMarker ||
                                (v9 = pref->GetObjectReference(), v9 == pXMarkerHeading)) {
                                ppoint = &pref->GetLocationOnReference();
                            }
                        }
                    }
                    int32_t inumber = 1;
                    if (!ppacktarg_2->GetTargReference()) {
                        inumber = pPack->GetInitialTargetCount();
                    }
                    apActor->DropObject(pitem->pContainerObj, plist, inumber, ppoint, 0);
                }
                if (pitem) {
                    delete pitem;
                }
                if (ppacktarg_2->GetTargObjectType() == POTYPE_WEAP ||
                    (ppacktarg_2->GetTargObjectType() >= POTYPE_COMBATWEAR &&
                     ppacktarg_2->GetTargObjectType() <= POTYPE_WEAP_RANGED)) {
                    while (!ObjectList.IsEmpty()) {
                        this->pAcquireObject = ObjectList.m_item;
                        ObjectList.Remove(this->pAcquireObject);
                        SetTarget(this->pAcquireObject->pRef);
                        pitem = apActor->GetInventoryItem(this->pAcquireObject->pObject, 0);
                        ObjectstoAcquire* pAcquireObject = this->pAcquireObject;
                        if (pAcquireObject) {
                            pAcquireObject->Delete(1u);
                        }
                        this->pAcquireObject = 0;
                        if (pcont && pcont->HasContainer()) {
                            int32_t v10 = pPack->GetInitialTargetCount();
                            apActor->PutItemInReference(pitem->pContainerObj, plist, pcont, v10, id);
                        }
                        else {
                            if (pitem->pExtraObjectList) {
                                plist = pitem->pExtraObjectList->m_item;
                            }
                            const NiPoint3* ppoint_2 = 0;
                            if (ppackloc) {
                                TESObjectREFR* pref_2 = ppackloc->GetLocReference();
                                if (!pref_2) {
                                    pref_2 = pGenericLocation;
                                }
                                if (pref_2) {
                                    TESBoundObject* v12;
                                    TESBoundObject* v11 = pref_2->GetObjectReference();
                                    if (v11 == pXMarker ||
                                        (v12 = pref_2->GetObjectReference(), v12 == pXMarkerHeading)) {
                                        ppoint_2 = &pref_2->GetLocationOnReference();
                                    }
                                }
                            }
                            int32_t inumber_2 = 1;
                            if (!ppacktarg_2->GetTargReference()) {
                                inumber_2 = pPack->GetInitialTargetCount();
                            }
                            apActor->DropObject(pitem->pContainerObj, plist, inumber_2, ppoint_2, 0);
                        }
                        if (pitem) {
                            delete pitem;
                        }
                        pitem = 0;
                    }
                    bool abweararmor = 1;
                    bool abwearweapon = 1;
                    if (GetCurrentPackage()) {
                        TESPackage* v13 = GetCurrentPackage();
                        if (v13->GetForceUnequipWeapon()) {
                            abwearweapon = 0;
                        }
                    }
                    if (apActor->IsNpc()) {
                        TESNPC* pnpc = dynamic_cast<TESNPC*>(apActor->GetObjectReference());
                        if (pnpc) {
                            pnpc->InitDefaultWorn(apActor, abweararmor, abwearweapon, 0, 1);
                        }
                    }
                    else {
                        TESCreature* pcrea = dynamic_cast<TESCreature*>(apActor->GetObjectReference());
                        if (pcrea) {
                            pcrea->InitDefaultWorn(apActor, abweararmor, abwearweapon, 1);
                        }
                    }
                }
                SetCurrentActionComplete(1);
            }
            AddToProcedureIndexRunning(apActor, 1);
        LABEL_108:;
            if (!apActor->IsPathingComplete()) {
                EndMoveMessage(apActor);
            }
            return 0;
        }
        double v14 = distance.Length();
        if (!(v59 >= v14)) {
            goto LABEL_108;
        }
        double v24 = fMydistance.Length();
        if (!(ftargdistance.Length() >= v24)) {
            goto LABEL_108;
        }
        AddToProcedureIndexRunning(apActor, 1);
        Actor* ptarg_2 = static_cast<Actor*>(pTarget);
        BaseProcess* v23 = ptarg_2->pCurrentProcess;
        if (!v23->GetPackageThatIsRunning()) {
            goto LABEL_108;
        }
        BaseProcess* v22 = ptarg_2->pCurrentProcess;
        v22->AddToProcedureIndexRunning(apActor, 1);
        BaseProcess* v21 = ptarg_2->pCurrentProcess;
        TESPackage* v15 = v21->GetPackageThatIsRunning();
        if (!v15->GetIsCreated()) {
            goto LABEL_108;
        }
        BaseProcess* v20 = ptarg_2->pCurrentProcess;
        v20->ClearCurrentPackage();
        if (ptarg_2->IsPathingComplete()) {
            ptarg_2->EndMovement();
        }
        SetCurrentActionComplete(1);
        CheckforNewPackage(apActor, 0);
        return 0;
    }
    else {
        TESPackage* v16 = GetCurrentPackage();
        int32_t idistance = static_cast<int32_t>(v16->GetPackageRadiusActorToRefTarget(apActor, 0));
        if (pTarget) {
            if (pTarget->IsActor()) {
                apActor->GetDistanceFromReference(pTarget, 0, 0);
            }
        }
        return 0;
    }
}

bool MiddleHighProcess::ProcessEatOneHour(Actor* apActor) {
    ItemChange* BestFoodItem = apActor->GetBestFoodItem();
    if (BestFoodItem) {
        ExtraDataList* apExtraList = 0;
        if (BestFoodItem->GetExtraDataList()) {
            BSSimpleList<ExtraDataList*>* v2 = BestFoodItem->GetExtraDataList();
            apExtraList = v2->m_item;
        }
        TESBoundObject* FormFlags = BestFoodItem->pContainerObj;
        apActor->Eat(FormFlags, apExtraList, 1);
        AddToProcedureIndexRunning(apActor, 1);
    }
    return 0;
}

bool MiddleHighProcess::ProcessActivateOneHour(Actor* apActor) {
    TESPackage* ppack = GetPackageThatIsRunning();
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    if (!pTarget || pTarget->GetDelete()) {
        AddToProcedureIndexRunning(apActor, 1);
        return 0;
    }
    if (ppack->GetPackType() == PACKAGE_TYPE_AMBUSH && !ppack->IsTargetAtLocation(apActor, 300)) {
        AddToProcedureIndexRunning(apActor, -1);
    }
    DoorTeleportData* pTel = pTarget->GetTeleport();
    bool batRef = 0;
    if (pTel) {
        NiPoint3 dist;
        const NiPoint3* v5 = &apActor->GetLocationOnReference();
        const NiPoint3* LinkedDoorTeleportPosition = &pTarget->GetLinkedDoorTeleportPosition();
        dist = *LinkedDoorTeleportPosition - *v5;
        if (dist.Length() <= 128.0) {
            batRef = 1;
        }
    }
    else {
        TESPackage* ppack_2 = GetCurrentPackage();
        batRef = ppack_2->IsActorAtRefTarget(apActor, 0);
    }
    if (!batRef) {
        const NiPoint3* v3;
        if (apActor->GetSitSleepState()) {
            ProcessGetUpOneHour(apActor);
        }
        if (pTarget->GetTeleport()) {
            v3 = &pTarget->GetLinkedDoorTeleportPosition();
        }
        else {
            v3 = &pTarget->GetLocationOnReference();
        }
        NiPoint3 dest = *v3;
        return 0;
    }
    if (pTarget == pPlayer && (pPlayer->IsSleepingorResting() && !pPlayer->btransporting)) {
        pPlayer->SetSleeping(0, 1);
        AddToProcedureIndexRunning(apActor, -2);
        return 0;
    }
    else {
        if (pTarget->IsActor()) {
            if (pTarget == pPlayer && (pPlayer->IsSleepingorResting() && !pPlayer->btransporting)) {
                pPlayer->SetSleeping(0, 1);
                AddToProcedureIndexRunning(apActor, -1);
                return 0;
            }
            else {
                SetTargetActivated(1);
                AddToProcedureIndexRunning(apActor, 1);
                return 1;
            }
        }
        else {
            SetTargetActivated(1);
            pTarget->Activate(apActor, 0, 0, 1);
            AddToProcedureIndexRunning(apActor, 1);
            return 1;
        }
    }
}

void MiddleHighProcess::AlignToFurnitureAndAddPostAnimSitSleep(Actor* apActor) {
    bool v5;
    if (apActor->CanMove()) {
        float afTargetAngle = FurnitureMarker.GetHeading();
        v5 = !apActor->RequestRotate(afTargetAngle, 0);
    }
    else {
        float Heading = FurnitureMarker.GetHeading();
        apActor->SetHeading(Heading);
        v5 = 1;
    }
    if (v5) {
        AddPostAnimationAction(POSTANIM_ACTION_SIT_SLEEP);
    }
}

bool MiddleHighProcess::ShouldPickPackIdle(Actor* apActor) {
    if (GetPackageIdleDone()) {
        return 0;
    }
    TESPackage* pPackage = GetPackageThatIsRunning();
    bool v6 = 0;
    uint32_t Count = pPackage->ePROCEDURE_TYPE;
    PROCEDURE Procedure_ = Procedure_Action[Count][GetProcedureIndexRunning()];
    if (Procedure_ == PROCEDURE_SANDBOX) {
        SandBoxActorPackageData* v5 = static_cast<SandBoxActorPackageData*>(GetInstanceDataThatIsRunning());
        if (v5) {
            Procedure_ = v5->GetProcedure();
        }
    }
    switch (Procedure_) {
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
                    v6 = 1;
                }
            }
            else {
                if (pPackage->IsActorAtLocation(apActor, 0, -1.0f, 0)) {
                    v6 = 1;
                }
            }
            break;
        }
        default: {
            return v6;
        }
    }
    return v6;
}

// The running package and the package extra share one stack slot: a package that is not in-game dialogue is used
// itself.
void MiddleHighProcess::PickPackageIdles(Actor* apActor, bool abForceIdle) {
    using kArray_t = BSSimpleArray<TESIdleForm*, 1024>;
    TESPackage* pPackage = GetPackageThatIsRunning();
    if (pPackage) {
        if (pPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
            pPackage = (&apActor->m_Extra)->GetPackageExtra();
        }
    }
    Animation* pAnimData = apActor->GetAnimation();
    if (pPackage) {
        if (bPickPackIdle) {
            abForceIdle = 1;
            fPackageIdleTimer = 0.0f;
        }
        if (ShouldPickPackIdle(apActor)) {
            BGSIdleCollection* pIdleCollection = pPackage->pIdleCollection;
            if (pIdleCollection) {
                if (pAnimData) {
                    if ((pAnimData->SpecialIdleDonePlaying() || abForceIdle) && !apActor->IsParalyzed()) {
                        if (!(fPackageIdleTimer <= 0.0)) {
                            fPackageIdleTimer = fPackageIdleTimer - appTimer.fDelta;
                        }
                        else {
                            bPickPackIdle = 0;
                            TESIdleForm* pLastIdle = pCurrentIdle;
                            pCurrentIdle = 0;
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
                                            pCurrentIdle = 0;
                                        }
                                        if (iPackageIdleNumber + 1 >= uiIdleCount && pIdleCollection->GetDoOnce()) {
                                            SetPackageIdleDone(1);
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
                                        pCurrentIdle = 0;
                                    }
                                    if (pCurrentIdle) {
                                        fPackageIdleTimer = 0.8f;
                                        if (pIdleCollection->GetDoOnce()) {
                                            SetPackageIdleDone(1);
                                        }
                                    }
                                }
                                if (pCurrentIdle) {
                                    SetupSpecialIdle(apActor, pCurrentIdle, AnimIdle::PLAY_TYPE_AUTO, 1, 0, 1);
                                }
                            }
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
        (pPackage->GetPackType() == PACKAGE_TYPE_FOLLOW && pPackage->GetLocation() &&
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
    NiPoint3 apResult;
    if (!apPackage || !ptarg) {
        return 0;
    }
    TESObjectREFR* pLocationRef = 0;
    if (apPackage->GetPackType() != PACKAGE_TYPE_ESCORT && apPackage->pPackLoc) {
        pLocationRef = apPackage->pPackLoc->GetLocReference();
    }
    else {
        pLocationRef = (TESObjectREFR*)apActor;
    }
    if (ptarg->GetTargType() == PTT_REFERENCE) {
        TESObjectREFR* pref = ptarg->GetTargReference();
        if (!pref || (!pref->IsDead(1) || apPackage->GetIsCreated())) {
            return pref;
        }
        apPackage->SetNeverRun(pref, 1);
        return 0;
    }
    if (ptarg->GetTargType() != PTT_LINKEDREF) {
        bool brebuiltList = 0;
        while (1) {
            if (ObjectList.IsEmpty()) {
                brebuiltList = 1;
                bool bevalflag = 1;
                TESObjectCELL* pCell = apActor->GetParentCell();
                NiPoint3 pointB = apActor->GetLocationOnReference();
                PACK_TARG_TYPE etype = ptarg->GetTargType();
                if (etype == PTT_OBJECT) {
                    TESForm* pForm = ptarg->GetTargObject();
                    if (pForm->GetFormType() == FLST_ID) {
                        if (pTargetFormList) {
                            ++sFormListNumb;
                        }
                        else {
                            pTargetFormList = static_cast<BGSListForm*>(pForm);
                            sFormListNumb = 0;
                        }
                        BSSimpleList<TESForm*>* plist = &pTargetFormList->ListOfForms;
                        if (!plist->IsEmpty()) {
                            for (int32_t inum = 0; plist && (!plist->IsEmpty() && sFormListNumb <= inum); ++inum) {
                                TESForm* pform = plist->m_item;
                                plist = plist->m_pkNext;
                            }
                        }
                        if (pForm) {
                            pObjecttoAcquire = static_cast<TESBoundObject*>(pForm);
                        }
                        eFormType = POTYPE_NONE;
                    }
                    else {
                        if (pForm->GetFormType() == FACT_ID) {
                            pFactiontoAquire = static_cast<TESFaction*>(pForm);
                            eFormType = POTYPE_ACTOR;
                        }
                        else {
                            if (ptarg->GetTargObject()) {
                                TESForm* pForm_2 = ptarg->GetTargObject();
                                if (pForm_2->IsBoundObject()) {
                                    pObjecttoAcquire = static_cast<TESBoundObject*>(ptarg->GetTargObject());
                                    eFormType = POTYPE_NONE;
                                }
                            }
                        }
                    }
                }
                else {
                    if (etype == PTT_TYPE) {
                        pObjecttoAcquire = 0;
                        eFormType = ptarg->GetTargObjectType();
                        eUseItem = eFormType;
                    }
                }
                if (!pObjecttoAcquire) {
                    switch (eFormType) {
                        case POTYPE_NONE: {
                            TESForm* pForm_3 = ptarg->GetTargObject();
                            MagicItem* pSpell = dynamic_cast<MagicItem*>(pForm_3);
                            if (pSpell) {
                                pCurrentPackageSpell = pSpell;
                            }
                            bevalflag = 0;
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
                                bevalflag = 0;
                            }
                            break;
                        }
                        case POTYPE_SPELL:
                        case POTYPE_SPELL_RANGED:
                        case POTYPE_SPELL_TOUCH:
                        case POTYPE_SPELL_SELF: {
                            eUseItem = eFormType;
                            bevalflag = 0;
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
                if (bevalflag) {
                    float facquireDist = 0.0f;
                    if (apPackage->GetPackageSearchLocation()) {
                        float facquireDist_2;
                        TESObjectCELL* searchLocCell_2;
                        uint32_t iradius = apPackage->GetSearchLocationRadius(apActor);
                        TESObjectCELL* searchLocCell = apPackage->GetSearchLocationCell(apActor);
                        if (searchLocCell) {
                            searchLocCell_2 = searchLocCell;
                        }
                        else {
                            searchLocCell_2 = pCell;
                        }
                        pCell = searchLocCell_2;
                        pointB = *apPackage->GetSearchLocationCoord(&apResult, apActor);
                        if (iradius) {
                            facquireDist_2 = static_cast<float>(iradius);
                        }
                        else {
                            float facquireDist_3;
                            if (pCell && pCell->GetInterior()) {
                                facquireDist_3 = 3.4028235e+38f;
                            }
                            else {
                                facquireDist_3 = fAIAcquireObjectDistance.Float();
                            }
                            facquireDist_2 = facquireDist_3;
                        }
                        facquireDist = facquireDist_2;
                        pDataHandler->EnumReferencesCloseToPoint(
                            pCell, pointB, facquireDist_2, pointB, facquireDist_2,
                            &LowProcess_EvaluateReferenceforAcquire, apActor
                        );
                    }
                    else {
                        TESPackage* v6;
                        if (!GetCurrentPackage() ||
                            (v6 = GetCurrentPackage(), !v6->GetAcquireRadius(apActor, facquireDist))) {
                            facquireDist = fAIAcquireObjectDistance.Float();
                        }
                        float afRadius2 = facquireDist;
                        const NiPoint3* v7 = &apActor->GetLocationOnReference();
                        pDataHandler->EnumReferencesCloseToPoint(
                            pCell, pointB, facquireDist, *v7, afRadius2, &LowProcess_EvaluateReferenceforAcquire,
                            apActor
                        );
                    }
                    eFormType = POTYPE_NONE;
                    pObjecttoAcquire = 0;
                    pFactiontoAquire = 0;
                    EvaluateOrderAcquireList(apActor);
                }
                iNumberItemsActivate = apPackage->GetInitialTargetCount();
            }
            bool v10 = apActor != pPlayer && apPackage->GetShouldReserveTarget();
            bool breserveRef = v10;
            while (!ObjectList.IsEmpty()) {
                TESNPC* pnpc;
                TESForm* pownerform;
                pAcquireObject = (&ObjectList)->m_item;
                ObjectList.Remove(pAcquireObject);
                TESObjectREFR* pacquireReference = pAcquireObject->pRef;
                if (pAcquireObject->eType != ACQUIRE_TYPE_BUY_NPC || pacquireReference->IsActor()) {
                    goto LABEL_84;
                }
                pownerform = pAcquireObject->pRef->GetOwner();
                pnpc = 0;
                if (pownerform) {
                    if (pownerform->GetFormType() == NPC__ID) {
                        pnpc = static_cast<TESNPC*>(pownerform);
                    }
                }
                if (pnpc) {
                    pacquireReference = (TESObjectREFR*)ReferenceProcessLists.GetActorRefInHigh(pnpc, 0);
                LABEL_84:;
                    if (!breserveRef) {
                        return pacquireReference;
                    }
                    if (!pacquireReference) {
                        return pacquireReference;
                    }
                    TESObjectREFR* v8 = GetCurrentFurniture();
                    if (v8 == pacquireReference) {
                        return pacquireReference;
                    }
                    if (!breserveRef) {
                        return pacquireReference;
                    }
                    if (!pacquireReference) {
                        return pacquireReference;
                    }
                    if (pTarget == pacquireReference) {
                        return pacquireReference;
                    }
                    int32_t ifreeMarker = pacquireReference->GetFirstFreeMarkerIndex(0);
                    if (ifreeMarker >= 0) {
                        return pacquireReference;
                    }
                }
            }
            if (pTarget || brebuiltList) {
                return 0;
            }
        }
    }
    TESObjectREFR* pref_2 = apActor->GetLinkedRef();
    if (!pref_2 || (!pref_2->IsDead(1) || apPackage->GetIsCreated())) {
        return pref_2;
    }
    apPackage->SetNeverRun(pref_2, 1);
    return 0;
}

void MiddleHighProcess::FindBedChairs(Actor* apActor, bool abSit, TESObjectREFR* apRefLoc) {
    HighProcess::bWantsToSit = abSit;
    TESObjectCELL* pCell = apActor->GetParentCell();
    NiPoint3 pointB = apActor->GetLocationOnReference();
    TESPackage* pPack = GetPackageThatIsRunning();
    if (pPack) {
        if (apActor) {
            PackageLocation* pLocation = pPack->pPackLoc;
            float facquireDist_2 = 0.0f;
            if (pPack->GetPackType() == PACKAGE_TYPE_EAT) {
                TESObjectCELL* LocCell;
                if (pLocation &&
                    (pLocation->GetLocType() == PLT_CELL &&
                     (LocCell = pLocation->GetLocCell(), LocCell->GetInterior()))) {
                    facquireDist_2 = 3.4028235e+38f;
                }
                else {
                    facquireDist_2 = static_cast<float>(pPack->GetLocationRadius(apActor));
                }
            }
            else {
                if (!pPack->GetAcquireRadius(apActor, facquireDist_2)) {
                    TESObjectCELL* v5;
                    if (pLocation &&
                        (pLocation->GetLocType() == PLT_CELL && (v5 = pLocation->GetLocCell(), v5->GetInterior()))) {
                        facquireDist_2 = 3.4028235e+38f;
                    }
                    else {
                        facquireDist_2 = fAIFindBedChairsDistance.Float();
                    }
                }
            }
            if (apRefLoc) {
                const NiPoint3* loc = (const NiPoint3*)&apRefLoc->GetLocationOnReference();
                float afRadius2 = facquireDist_2;
                float afRadius1 = facquireDist_2;
                TESObjectCELL* ParentCell = apRefLoc->GetParentCell();
                pDataHandler->EnumReferencesCloseToPoint(
                    ParentCell, *loc, afRadius1, *loc, afRadius2, &MiddleHighProcess::EvaluateReferenceforSitSleep,
                    apActor
                );
            }
            else {
                float afRadius2a = facquireDist_2;
                pDataHandler->EnumReferencesCloseToPoint(
                    pCell, pointB, facquireDist_2, pPack->GetLocationCoord(apActor), afRadius2a,
                    &MiddleHighProcess::EvaluateReferenceforSitSleep, apActor
                );
            }
            RefListChairBed.RemoveAll();
            for (TESObjectREFR*& rpItem : MiddleHighTempRefListChairBed.ItemsUntilNull()) {
                RefListChairBed.AddHead(*reinterpret_cast<void**>(&rpItem));
            }
            MiddleHighTempRefListChairBed.RemoveAll();
            BSSimpleList<void*>* pFirst = &RefListChairBed;
            NiPlane* v24 = 0;
            int32_t iCount = pFirst->ItemsInList();
            bool bSwappedItems = 1;
            while (iCount && bSwappedItems) {
                bSwappedItems = 0;
                for (BSSimpleList<void*>* pSecond = pFirst; pSecond; pSecond = pSecond->m_pkNext) {
                    void* pFirstData = pFirst->m_item;
                    void* pSecondData = pSecond->m_item;
                    if (!static_cast<TESObjectREFR*>(pFirstData)->GetOwner()) {
                        if (static_cast<TESObjectREFR*>(pSecondData)->GetOwner()) {
                            pFirst->SetItem(pSecondData);
                            pSecond->SetItem(pFirstData);
                            bSwappedItems = 1;
                        }
                    }
                }
                pFirst = pFirst->m_pkNext;
                --iCount;
            }
            nFurnitureListTimer = appTimer.iLastTime;
        }
    }
}

bool MiddleHighProcess::EvaluateReferenceforSitSleep(TESObjectREFR* apRef, void* aiFilterData) {
    Actor* pActor = static_cast<Actor*>(aiFilterData);
    if (!aiFilterData) {
        return 0;
    }
    if (!apRef ||
        (apRef->GetDelete() ||
         (apRef->GetTemporary() ||
          (apRef->GetDisabled() || (pActor->IsChild(0) && !pActor->CanChildUseObject(apRef)))))) {
        return 0;
    }
    if (MiddleHighTempRefListChairBed.IsInList(apRef)) {
        return 0;
    }
    if (!apRef->IsFurniture() || !apRef->HasFreeMarker(0)) {
        return 0;
    }
    if (apRef->GetOwner() && !apRef->IsAnOwner(pActor, 1)) {
        return 0;
    }
    TESForm* Space = apRef->GetSpace();
    if (Space != pActor->GetSpace()) {
        return 0;
    }
    TESFurniture* pFurniture = static_cast<TESFurniture*>(apRef->GetObjectReference());
    if ((HighProcess::bWantsToSit && pFurniture->CanSitOn()) ||
        (!HighProcess::bWantsToSit && pFurniture->CanSleepOn())) {
        MiddleHighTempRefListChairBed.AddHead(apRef);
    }
    return 0;
}

void bhkCharacterController::SetSupportNormal(const hkVector4& arNormal) {
    hkSupportNorm.operator=(arNormal);
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
    int32_t apData;
    Actor* pActor = apSaveGameBuffer->GetActor();
    apSaveGameBuffer->SaveFormID(pObject, 0);
    apData = -1;
    if (pExtra) {
        InventoryChanges* ContainerChanges;
        if (pActor) {
            ContainerChanges = pActor->m_Extra.GetContainerChanges();
        }
        else {
            ContainerChanges = 0;
        }
        if (ContainerChanges) {
            apData = ContainerChanges->GetExtraDataListIndexForItem(pObject, pExtra);
        }
    }
    apSaveGameBuffer->SaveDataEndian(&apData, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&iNumber, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&bEquip, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bNow, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bForceEquip, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bPlayPickUpSounds, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bForceUnequip, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bEquipCalling, 1u, 0);
}

void QueuedItem::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    int32_t apData;
    Actor* pactor = apLoadGameBuffer->GetActor();
    uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
    pObject = dynamic_cast<TESBoundObject*>(FormByNumericID);
    apData = -1;
    apLoadGameBuffer->LoadData(&apData, 4u);
    if (apData != -1) {
        InventoryChanges* pinventory;
        if (pactor) {
            pinventory = (&pactor->m_Extra)->GetContainerChanges();
        }
        else {
            pinventory = 0;
        }
        if (pinventory) {
            pExtra = pinventory->GetExtraDataListFromIndexForItem(pObject, apData);
        }
    }
    apLoadGameBuffer->LoadData(&iNumber, 4u);
    apLoadGameBuffer->LoadData(&bEquip, 1u);
    apLoadGameBuffer->LoadData(&bNow, 1u);
    apLoadGameBuffer->LoadData(&bForceEquip, 1u);
    apLoadGameBuffer->LoadData(&bPlayPickUpSounds, 1u);
    apLoadGameBuffer->LoadData(&bForceUnequip, 1u);
    apLoadGameBuffer->LoadData(&bEquipCalling, 1u);
}

bool QueuedItem::QueueFiles(Actor* apActor) {
    QueuedFile* v15;
    if (!apActor || !pObject) {
        return 0;
    }
    if (!bEquip) {
        if (apActor->IsCreature() && pObject->GetFormType() == WEAP_ID) {
            uint16_t v8;
            BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
            Animation* panim_2 = CurrentAIProcess->GetAnimation();
            if (!panim_2 ||
                (!panim_2->GetControllerManager() ||
                 (v8 = TESAnimGroup::AnimGroup(AGM_NONE, AGW_HAND_TO_HAND, ANIM_GROUP_EQUIP, 0),
                  !panim_2->GroupLoaded(v8)))) {
                QueuedFile* v12;
                QueuedFile* v17 = static_cast<QueuedFile*>(ourMalloc(sizeof(QueuedFile)));
                if (v17) {
                    v12 = ::new (v17) QueuedFile(IO_TASK_PRIORITY_VERY_HIGH);
                }
                else {
                    v12 = 0;
                }
                spQueuedFiles = v12;
                const char* Model_ = apActor->GetModel();
                void* pfileList_2 = (void*)pModelLoader->BuildKFFileList(Model_, 1, 0, AGW_HAND_TO_HAND);
                QueuedFile* v10 = spQueuedFiles.m_pObject;
                pModelLoader->QueueKFList(
                    static_cast<BSSimpleList<const char*>*>(pfileList_2), IO_TASK_PRIORITY_VERY_HIGH, v10, 0
                );
                if (pfileList_2) {
                    delete static_cast<BSSimpleList<ActiveEffect*>*>(pfileList_2);
                }
            }
        }
        return 0;
    }
    if (pObject->GetFormType() != WEAP_ID && pObject->GetFormType() != ARMO_ID) {
        return 0;
    }
    QueuedFile* apMemory = static_cast<QueuedFile*>(ourMalloc(sizeof(QueuedFile)));
    if (apMemory) {
        v15 = ::new (apMemory) QueuedFile(IO_TASK_PRIORITY_VERY_HIGH);
    }
    else {
        v15 = 0;
    }
    spQueuedFiles = v15;
    QueuedFile* v3 = spQueuedFiles.m_pObject;
    pModelLoader->QueueBoundObject(pObject, IO_TASK_PRIORITY_VERY_HIGH, v3, apActor);
    if (apActor->IsCreature() && pObject->GetFormType() == WEAP_ID) {
        WEAPON_TYPE WeaponType;
        uint16_t v5;
        TESObjectWEAP* pweapon = static_cast<TESObjectWEAP*>(pObject);
        BaseProcess* v14 = apActor->pCurrentProcess;
        Animation* panim = v14->GetAnimation();
        if (!panim ||
            (!panim->GetControllerManager() ||
             (WeaponType = pweapon->GetWeaponType(),
              (v5 = TESAnimGroup::AnimGroup(AGM_NONE, weaponToAnimGroupWeapon[WeaponType], ANIM_GROUP_EQUIP, 0),
               !panim->GroupLoaded(v5))))) {
            ANIM_GROUP_WEAPON_ENUM v11 = weaponToAnimGroupWeapon[pweapon->GetWeaponType()];
            const char* v6 = apActor->GetModel();
            void* pfileList = (void*)pModelLoader->BuildKFFileList(v6, 1, 0, v11);
            QueuedFile* v7 = spQueuedFiles.m_pObject;
            pModelLoader->QueueKFList(
                static_cast<BSSimpleList<const char*>*>(pfileList), IO_TASK_PRIORITY_VERY_HIGH, v7, 0
            );
            if (pfileList) {
                delete static_cast<BSSimpleList<ActiveEffect*>*>(pfileList);
            }
        }
    }
    return 1;
}

void QueuedItem::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Actor* apActor = apLoadGameBuffer->GetActor();
    QueueFiles(apActor);
}

void QueuedItem::operator delete(void* apMemory) {
    ourFree(apMemory);
}

bool SandBoxActorPackageData::IsTargetNeededForProcedure(SANDBOX_PROCEDURE aeProcedure) {
    return aeProcedure < SANDBOX_PROCEDURE_EAT || aeProcedure > SANDBOX_PROCEDURE_WANDER;
}
