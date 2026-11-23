#include "Fallout/AI/Combat/VATS.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Controls.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSBodyPartData.h"
#include "Fallout Shared/BGSCameraPath.h"
#include "Fallout Shared/BGSCameraPathManager.h"
#include "Fallout Shared/BGSCameraShot.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ImageSpaceModifierInstance.h"
#include "Fallout Shared/ImageSpaceModifierInstanceRB.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Interface/FOVATSEffectManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpAllRayHitCollector.h"

VATS pVATS;
SettingT<INISettingCollection> gs_fVATSLightLevelMin_VATS("fVATSLightLevelMin:VATS", 20.0f);
SettingT<INISettingCollection> gs_fVATSLightLevelMax_VATS("fVATSLightLevelMax:VATS", 40.0f);
SettingT<INISettingCollection> gs_fVATSLightAngle_VATS("fVATSLightAngle:VATS", 0.0f);
SettingT<INISettingCollection> gs_fVATSLightDistance_VATS("fVATSLightDistance:VATS", 100.0f);
SettingT<INISettingCollection> gs_fVATSLightElevation_VATS("fVATSLightElevation:VATS", 100.0f);
SettingT<INISettingCollection> gs_fVatsLightColorR_VATS("fVatsLightColorR:VATS", 1.0f);
SettingT<INISettingCollection> gs_fVatsLightColorG_VATS("fVatsLightColorG:VATS", 1.0f);
SettingT<INISettingCollection> gs_fVatsLightColorB_VATS("fVatsLightColorB:VATS", 1.0f);

bool VATSQueuedAction::IsActionGrenadeOrMine(ActionPoints::Action aeAction) {
    return aeAction >= ActionPoints::ACTION_ATTACK_GRENADE && aeAction <= ActionPoints::ACTION_ATTACK_MINE;
}

VATS::VATS()
  : eMode(VATS::VATS_MODE_NONE), pCameraList(nullptr), pCamera(nullptr), fCameraTimer(0.0f), fCameraMinTimer(0.0f),
    pFirstProjectile(nullptr), pLastProjectile(nullptr), pMeleeAttack(nullptr), pImagespaceMod(nullptr),
    pTransitionImagespaceMod(nullptr), spVATSLight(nullptr), bFirstVatsAttack(true), iKillCount(0),
    pUnEquipWeapon(nullptr), pUnEquipExtra(nullptr) {
    eVATSCameraAction = 0;
}

VATS::~VATS() {
    ClearData();
}

void VATS::QueueAction(VATS_COMMAND* apCommand) {
    commandList.AddTail(apCommand);
}

NiPoint3 VATS::GetTargetPercentagePos(float afPercentage) {
    NiNode* pAttacker3D = pVATSAttacker->TESObjectREFR::Get3D();
    NiPoint3 startingPos = pAttacker3D->GetWorldBound().m_kCenter;
    VATS_COMMAND* pCommand = commandList.GetItem();
    if ((pCommand && pCommand->pTarget) || pVATSTarget) {
        NiPoint3 endingPos = NiPoint3::ZERO;
        if (pCommand && pCommand->pTarget) {
            if (pCommand->pTarget->Get3D()) {
                NiNode* pTarget3D = pCommand->pTarget->Get3D();
                endingPos = pTarget3D->GetWorldBound().m_kCenter;
            }
            else {
                endingPos = pCommand->pTarget->GetLocationOnReference();
            }
        }
        else if (pVATSTarget) {
            if (pVATSTarget->Get3D()) {
                NiNode* pTarget3D = pVATSTarget->Get3D();
                endingPos = pTarget3D->GetWorldBound().m_kCenter;
            }
            else {
                endingPos = pVATSTarget->GetLocationOnReference();
            }
        }
        NiPoint3 delta = endingPos - startingPos;
        float fLength = delta.UnitizeGetLength();
        startingPos += delta * fLength * afPercentage / 100.0f;
    }
    return startingPos;
}

VATS_COMMAND* VATS::GetCurrentAction() {
    bool bEmpty;
    bEmpty = commandList.IsEmpty();
    if (bEmpty) {
        return nullptr;
    }
    VATS_COMMAND* pCommand;
    pCommand = commandList.GetItem();
    return pCommand;
}

// The last queued command: walks the list with IsEmpty/GetNext and returns the node's item when GetNext reports the
// end. GetNext is called twice per node, and the entry node is tested for null before the loop.
VATS_COMMAND* VATS::GetLastAction() {
    for (BSSimpleList<VATS_COMMAND*>* pIter = &commandList; pIter;) {
        bool bEmpty;
        bEmpty = pIter->IsEmpty();
        if (bEmpty) {
            break;
        }
        BSSimpleList<VATS_COMMAND*>* pNext;
        pNext = pIter->GetNext();
        if (!pNext) {
            VATS_COMMAND* pCommand;
            pCommand = pIter->GetItem();
            return pCommand;
        }
        pIter = pIter->GetNext();
    }
    return nullptr;
}

bool VATS::IsMeleeCommand(VATS_COMMAND* apCommand) {
    VATS_COMMAND* pCommand = apCommand;
    if (!apCommand) {
        pCommand = commandList.GetItem();
    }
    if (!pCommand) {
        return false;
    }
    switch (pCommand->eAction) {
        case ActionPoints::ACTION_ATTACK_UNARMED:
        case ActionPoints::ACTION_ATTACK_ONE_HAND_MELEE:
        case ActionPoints::ACTION_ATTACK_TWO_HAND_MELEE:
        case ActionPoints::ACTION_PLAYER_SPECIAL_WEAPON_ATTACK:
        case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK:
        case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_2:
        case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_3: return true;
        default: return false;
    }
}

bool VATS::IsInVats() {
    return this->eMode != VATS::VATS_MODE_NONE;
}

ShadowSceneLight* VATS::GetVATSLight() {
    // The engine routes this through NiPointer::operator->; both return the light.
    return this->spVATSLight;
}

VATS::VATS_MODE_ENUM VATS::GetMode() {
    return this->eMode;
}

void VATS::SetFirstProjectile(Projectile* apProjectile) {
    if (pCamera) {
        if (pCamera->Data.eLocation == BGSCameraShot::CO_PROJECTILE) {
            if (apProjectile) {
                NiNode* p3D = apProjectile->Get3D();
                pCamera->SetCameraLocation(p3D);
            }
            else {
                pCamera->SetCameraLocation(nullptr);
            }
        }
        if (pCamera->Data.eTarget == BGSCameraShot::CO_PROJECTILE) {
            if (apProjectile) {
                NiNode* p3D = apProjectile->Get3D();
                pCamera->SetCameraTarget(p3D);
            }
            else {
                pCamera->SetCameraTarget(nullptr);
            }
        }
    }
    pFirstProjectile = apProjectile;
}

void VATS::SetLastProjectile(Projectile* apProjectile) {
    uint8_t i;
    pLastProjectile = apProjectile;
    if (pCamera) {
        VATS_COMMAND* pCurrentAction = GetCurrentAction();
        UpdateCurrentCameraAction(pCurrentAction);
        if (fCameraMinTimer == 0.0) {
            for (i = 1; pCamera && i && eVATSCameraAction > static_cast<int32_t>(pCamera->GetCameraAction());
                 i = UseNextCamera()) {
                ;
            }
        }
    }
}

void VATS::UpdateCurrentCameraAction(VATS_COMMAND* apCommand) {
    if (IsMeleeCommand(apCommand)) {
        Animation* pAnim = pVATSAttacker->Actor::GetAnimation();
        if (!eVATSCameraAction) {
            BSAnimGroupSequence* pSeq = pAnim->GetCurrentSequence(AGS_WEAPON);
            if (pSeq &&
                (!pSeq->GetAnimGroup()->IsAimAnim() || apCommand->cFireShots) &&
                (!pSeq->GetAnimGroup()->IsAttackAction() || pAnim->GetGroupAction(AGS_WEAPON) < AGA_STOP)) {
                if (apCommand->pTarget && apCommand->pTarget->IsDead(false)) {
                    eVATSCameraAction = 2;
                }
            }
            else {
                eVATSCameraAction = 2;
            }
            if (eVATSCameraAction == 2 && fCameraMinTimer == 0.0) {
                for (bool bNewCamera = true; bNewCamera; bNewCamera = UseNextCamera()) {
                    int32_t iCameraAction = pCamera->GetCameraAction();
                    if (eVATSCameraAction <= iCameraAction) {
                        break;
                    }
                }
            }
        }
    }
    else if (apCommand && apCommand->eAction == ActionPoints::ACTION_TARGET_KILL_SHOT) {
        eVATSCameraAction = 2;
        pCamera->bCameraIsKillCam = true;
    }
    else {
        if (pFirstProjectile && !eVATSCameraAction) {
            eVATSCameraAction = 1;
        }

        if (pFirstProjectile || pLastProjectile) {
            if (pFirstProjectile) {
                if (pLastProjectile && (!pCamera || pCamera->GetCameraAction() == BGSCameraShot::CA_HIT)) {
                    eVATSCameraAction = 2;
                }
                else {
                    const NiPoint3& kProjectilePos = pFirstProjectile->GetLocationOnReference();
                    const NiPoint3& kTargetPos = apCommand->pTarget->GetLocationOnReference();
                    NiPoint3 deltaPos = kTargetPos - kProjectilePos;
                    double dDistance = deltaPos.Length();
                    double dHitDist = static_cast<double>(iVATSCameraHitDist.Int());
                    if (apCommand->pTarget->GetBoundMax().y + dHitDist > dDistance) {
                        eVATSCameraAction = 2;
                    }
                    else if (pLastProjectile) {
                        const NiPoint3& kLastProjectilePos = pLastProjectile->GetLocationOnReference();
                        const NiPoint3& kLastTargetPos = apCommand->pTarget->GetLocationOnReference();
                        deltaPos = kLastTargetPos - kLastProjectilePos;
                        dDistance = deltaPos.Length();
                        dHitDist = static_cast<double>(iVATSCameraHitDist.Int());
                        if (apCommand->pTarget->GetBoundMax().y + dHitDist > dDistance) {
                            eVATSCameraAction = 2;
                        }
                    }
                }
            }
            else {
                eVATSCameraAction = 2;
            }
        }
        else if (eVATSCameraAction == 1) {
            eVATSCameraAction = 2;
        }
    }
}

bool VATS::UseNextCamera() {
    BSSimpleList<BGSCameraShot*>* pList = pCameraList;
    bool bRetVal = false;
    while (pList && pList->m_item != pCamera) {
        pList = pList->m_pkNext;
    }
    if (pList) {
        pList = pList->m_pkNext;
    }
    if (pList) {
        if (pCamera) {
            pCamera->FreeCamera();
            pCamera = nullptr;
        }
        pCamera = pList->m_item;
        if (pCamera && static_cast<double>(fCameraMinTimer) < pCamera->Data.fMinTime) {
            fCameraMinTimer = pCamera->Data.fMinTime;
        }

        if (pCamera && pCamera->GetIsFirstPersonCamera()) {
            pPlayer->SetFirstPerson(true);
        }
        else {
            bSwitchTo3rdPerson = true;
        }
        return true;
    }

    if (pCamera && !pCamera->IsInitialized()) {
        pCamera->FreeCamera();
        pCamera = nullptr;
    }

    return bRetVal;
}

void VATS::SetMode(VATS::VATS_MODE_ENUM aeMode, bool bForce) {
    if (aeMode != eMode || bForce) {
        pVATSAttacker = pPlayer;
        switch (aeMode) {
            case VATS::VATS_MODE_NONE: {
                if (VATS::pStranger) {
                    delete VATS::pStranger;
                    VATS::pStranger = nullptr;
                }
                if (VATS::pMissFortune) {
                    delete VATS::pMissFortune;
                    VATS::pMissFortune = nullptr;
                }
                appTimer.SetGlobalTimeMultiplier(1.0f, true);
                pForceCameraPath = nullptr;
                pForceDefaultCameraPath = nullptr;
                eCameraZoom = 0;
                if (pCamera) {
                    pCamera->FreeCamera();
                }
                eVATSCameraAction = 0;
                pCameraList = nullptr;
                pCamera = nullptr;
                fCameraTimer = 0.0f;
                fCameraMinTimer = 0.0f;
                pFirstProjectile = nullptr;
                pLastProjectile = nullptr;
                pCameraPath = nullptr;
                bSwitchTo3rdPerson = false;
                if (pImagespaceMod) {
                    ImageSpaceModifierInstance::Stop(pImagespaceMod);
                    pImagespaceMod = nullptr;
                }
                if (pTransitionImagespaceMod) {
                    ImageSpaceModifierInstance::Stop(pTransitionImagespaceMod);
                    pTransitionImagespaceMod = nullptr;
                }
                bFirstVatsAttack = true;
                pPlayer->SetIronSights(false, false, false);
                pPlayer->SetBlock(false);
                Controls* pControls = appMain->GetControls();
                pControls->ResetControlState(6);
                if (pUnEquipWeapon) {
                    pPlayer->UnEquipObject(pUnEquipWeapon, 1, pUnEquipExtra, false, true, true);
                    pUnEquipWeapon = nullptr;
                    pUnEquipExtra = nullptr;
                }
                break;
            }
            case VATS::VATS_MENU_MODE: {
                pUnEquipWeapon = nullptr;
                pUnEquipExtra = nullptr;
                FOVATSEffectManager* pVATSEffect = Interface::GetVATSEffect();
                pVATSEffect->DisableScan();
                if (!pVATS.GetVATSLight()) {
                    NiLight* pNewLight = new NiPointLight;
                    pVATS.SetVATSLight(pNewLight, true);
                }
                break;
            }
            case VATS::VATS_SCAN_MODE: {
                pUnEquipWeapon = nullptr;
                pUnEquipExtra = nullptr;
                if (!pVATS.GetVATSLight()) {
                    NiLight* pNewLight = new NiPointLight;
                    pVATS.SetVATSLight(pNewLight, true);
                }
                break;
            }
            case VATS::VATS_PLAYBACK: {
                pUnEquipWeapon = nullptr;
                pUnEquipExtra = nullptr;
                iKillCount = 0;
                fVATSPlaybackTimer = 0.0f;
                fVATSCommandTimer = 0.0f;
                bVATSFlushCommandWhenAnimActionIsDone = false;
                bLastCommandBlock = 0;
                if (pImagespaceMod) {
                    ImageSpaceModifierInstance::Stop(pImagespaceMod);
                    pImagespaceMod = nullptr;
                }
                if (pTransitionImagespaceMod) {
                    ImageSpaceModifierInstance::Stop(pTransitionImagespaceMod);
                    pTransitionImagespaceMod = nullptr;
                }
                NiPoint2 positon(0.5f, 0.5f);
                float fVATSCamTransRBDownStart = ::fVATSCamTransRBDownStart.Float();
                float fVATSCamTransRBRampDown = ::fVATSCamTransRBRampDown.Float();
                float fVATSCamTransRBStart = ::fVATSCamTransRBStart.Float();
                pTransitionImagespaceMod = ImageSpaceModifierInstanceRB::Trigger(
                    0.0f,
                    fVATSCamTransRBRampup.Float(),
                    fVATSCamTransRBStart,
                    fVATSCamTransRBRampDown,
                    fVATSCamTransRBDownStart,
                    positon,
                    0.0f
                );
                if (pTransitionImagespaceMod) {
                    pTransitionImagespaceMod->SetPermanent(true);
                }
                break;
            }
            default: {
                break;
            }
        }
        BSShaderManager::SetFullScreenMotionBlurVATS(aeMode == VATS::VATS_MENU_MODE);
        eMode = aeMode;
        if (eMode == VATS::VATS_MODE_NONE) {
            HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_CHANGED);
            bVATSForce1stPerson = true;
        }
    }
    if (aeMode) {
        if (aeMode > VATS::VATS_TARGET_SELECT && aeMode <= VATS::VATS_SCAN_MODE && pVATSTarget) {
            pVATS.UpdateVATSLight(pVATSTarget);
        }
    }
    else if (pVATS.GetVATSLight()) {
        NiLight* pLight = pVATS.GetVATSLight()->GetLight();
        ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(0);
        pShadowSceneNode->RemoveLight(pLight);
        pShadowSceneNode = BSShaderManager::GetShadowSceneNode(0);
        pLight = pVATS.GetVATSLight()->GetLight();
        pShadowSceneNode->DetachChild(pLight);
        pVATS.SetVATSLight(nullptr, true);
    }
}

float VATS::GetPlayerUpdateMult() {
    float offset = 1.0f;
    if (eMode == VATS_PLAYBACK) {
        if (!pCamera || pCamera->IsInitialized()) {
            if (pCamera && pCamera->Data.fPlayerTimeMult > 0.0) {
                offset = pCamera->Data.fPlayerTimeMult;
            }
            else {
                offset = fVATSPlayerTimeUpdateMult.Float();
            }
        }
        else {
            offset = 1.0f;
        }
    }
    double globalTimeMultiplierTarget = BSTimer::GetGlobalTimeMultiplierTarget();
    return static_cast<float>(globalTimeMultiplierTarget / BSTimer::QGlobalTimeMultiplier() * offset);
}

float VATS::GetTargetUpdateMult() {
    float targetTimeMult = 1.0f;
    if (eMode == VATS_PLAYBACK) {
        if (!pCamera || pCamera->IsInitialized()) {
            if (pCamera && pCamera->GetTargetTimeMult() > 0.0) {
                targetTimeMult = pCamera->GetTargetTimeMult();
            }
            else {
                targetTimeMult = fVATSTargetTimeUpdateMult.Float();
            }
        }
        else {
            targetTimeMult = 1.0f;
        }
    }
    double globalTimeMultiplierTarget = BSTimer::GetGlobalTimeMultiplierTarget();
    return static_cast<float>(globalTimeMultiplierTarget / BSTimer::QGlobalTimeMultiplier() * targetTimeMult);
}

void VATS::ClearTarget(TESObjectREFR* apRef) {
    if (apRef == pVATSTarget) {
        pVATSTarget = nullptr;
    }
    BSSimpleList<VATS_COMMAND*>* pList = &commandList;
    BSSimpleList<VATS_COMMAND*>* pLastList = nullptr;
    while (pList && !pList->IsEmpty()) {
        VATS_COMMAND* pCommand = pList->m_item;
        if (pCommand->pTarget == apRef) {
            if (pCommand) {
                delete pCommand;
            }
            if (pLastList) {
                pLastList->Remove(pCommand);
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

void VATS::QuitVATSPlayback(bool abForceQuit, bool abFinishCommandPlayback) {
    if (eMode &&
        (fEndVATSDelay == 0.0 || !commandList.IsEmpty()) &&
        (abForceQuit ||
         commandList.IsEmpty() ||
         commandList.m_item->eAction != ActionPoints::ACTION_PLAYER_VATS_DEATH)) {
        if (abFinishCommandPlayback) {
            VATS_COMMAND* pCommand = GetCurrentAction();
            if (pCommand && pCommand->cFireShots > 1u) {
                pCommand->cFireShots = 1;
                pCommand->cFakeFireShots = 0;
            }

            if (!commandList.IsEmpty()) {
                for (BSSimpleList<VATS_COMMAND*>* pList = commandList.m_pkNext; pList; pList = commandList.m_pkNext) {
                    VATS_COMMAND* pDelete = pList->m_item;
                    if (pDelete) {
                        delete pDelete;
                    }
                    commandList.Remove(pDelete);
                }
            }
        }
        else {
            ClearCommandList();
            fEndVATSDelay = fVATSPlaybackDelay.Float();
            appTimer.SetGlobalTimeMultiplier(1.0f, false);
        }
    }
}

BGSCameraShot* VATS::GetCurrentCamera() {
    return pCamera;
}

float VATS::GetCurrentCameraMinTimer() {
    return fCameraMinTimer;
}

void VATS::ClearCommandList() {
    if (pMeleeAttack) {
        Animation* pAnimation = pVATSAttacker->Actor::GetAnimation();
        pAnimation->SpecialIdleFree(true, false);
        pMeleeAttack = nullptr;
    }
    while (!commandList.IsEmpty()) {
        VATS_COMMAND* pCommand = commandList.m_item;
        if (pCommand) {
            delete pCommand;
        }
        commandList.RemoveHead();
    }
}

void VATS::ClearData() {
    ClearCommandList();
    if (pCamera) {
        pCamera->FreeCamera();
    }
    pCameraList = nullptr;
    pCamera = nullptr;
    fCameraTimer = 0.0f;
    fCameraMinTimer = 0.0f;
    if (pImagespaceMod) {
        ImageSpaceModifierInstance::Stop(pImagespaceMod);
        pImagespaceMod = nullptr;
    }
    if (pTransitionImagespaceMod) {
        ImageSpaceModifierInstance::Stop(pTransitionImagespaceMod);
        pTransitionImagespaceMod = nullptr;
    }
}

void VATS::IncrementKillCount() {
    ++iKillCount;
}

void VATS::EvaluateCommand(VATS_COMMAND* apCommand) {
    if (apCommand && ActionPoints::IsAttackAction(apCommand->eAction)) {
        if (apCommand->bSuccess &&
            apCommand->pTarget &&
            !bhkWorld::QPickTimeOverMax() &&
            !pVATSAttacker->LineOfSight(true, apCommand->pTarget, true, nullptr, false)) {
            apCommand->bSuccess = false;
        }

        if (apCommand->pHitData) {
            apCommand->pHitData->ClearData();
        }
        HitData::InitializeHitData(apCommand->pHitData, apCommand);
    }
}

void VATS::InitPlayerDeath() {
    QuitVATSPlayback(false, false);
    VATS_COMMAND* pCommand = new VATS_COMMAND;
    pCommand->eAction = ActionPoints::ACTION_PLAYER_VATS_DEATH;
    pCommand->bSuccess = true;
    pCommand->pTarget = pPlayer;
    QueueAction(pCommand);
    SetMode(VATS::VATS_PLAYBACK, false);
    HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_START);
}

void VATS::Update() {
    float fAPReward;
    bhkWorld::SetVatsReplay(eMode == VATS::VATS_PLAYBACK);
    if (eMode == VATS::VATS_PLAYBACK) {
        if (!commandList.IsEmpty()) {
            VATS_COMMAND* pCommand = commandList.m_item;
            Controls* pControls = appMain->GetControls();
            HighProcess* pCurrentProcess = static_cast<HighProcess*>(pVATSAttacker->pCurrentProcess);
            bool bGetNextCamera = false;
            pVATSTarget = pCommand->pTarget;
            if (pCamera && pCamera->IsInitialized() && fCameraMinTimer > 0.0) {
                double dDelta = appTimer.fDelta;
                fCameraMinTimer = fCameraMinTimer - dDelta / BSTimer::QGlobalTimeMultiplier();
                if (fCameraMinTimer < 0.0) {
                    fCameraMinTimer = 0.0f;
                    for (bool bNewCamera = true; bNewCamera; bNewCamera = UseNextCamera()) {
                        int32_t iCameraAction = pCamera->GetCameraAction();
                        if (eVATSCameraAction <= iCameraAction) {
                            break;
                        }
                    }
                }
            }
            else if (pCamera && pCommand != pcameraCommand) {
                pCamera->FreeCamera();
                eVATSCameraAction = 0;
                pCamera = nullptr;
                pCameraList = nullptr;
                fCameraTimer = 0.0f;
                fCameraMinTimer = 0.0f;
            }
            if (pCamera && fCameraTimer > 0.0) {
                double dDelta = appTimer.fDelta;
                fCameraTimer = fCameraTimer - dDelta / BSTimer::QGlobalTimeMultiplier();
                if (fCameraTimer <= 0.0) {
                    bGetNextCamera = true;
                    fCameraTimer = 0.0f;
                    fCameraMinTimer = 0.0f;
                }
            }

            if (!pCameraList || bGetNextCamera) {
                bool bNewCamera = false;
                if (!pCameraList) {
                    bNewCamera = true;
                    EvaluateCommand(pCommand);
                    if (pForceCameraPath) {
                        pCameraPath = pForceCameraPath;
                    }
                    else if (pForceDefaultCameraPath) {
                        pCameraPath = pForceDefaultCameraPath;
                        pForceDefaultCameraPath = nullptr;
                    }
                    else {
                        pCameraPath = pCameraPathManager->GetCameraPathToPlay(pVATSAttacker, pCommand->pTarget);
                    }
                    if (pCameraPath) {
                        if (pCameraPath->GetZoomDefault()) {
                            eCameraZoom = 0;
                        }
                        else if (pCameraPath->GetZoomDisable()) {
                            eCameraZoom = 1;
                        }
                        else if (pCameraPath->GetZoomShotList()) {
                            eCameraZoom = 2;
                        }
                    }
                    else {
                        eCameraZoom = 0;
                    }
                    pCameraList = pCameraPath ? &pCameraPath->ShotList : nullptr;
                }
                if (pCameraList) {
                    if (bGetNextCamera) {
                        if (!UseNextCamera()) {
                            if ((pVATSAttacker == VATS::pStranger && pCommand->bStranger) ||
                                (pVATSAttacker == VATS::pMissFortune && pCommand->bMissFortune)) {
                                pCommand->cFireShots = 0;
                            }
                            else {
                                SetLastProjectile(nullptr);
                            }
                        }
                    }
                    else {
                        pCamera = pCameraList->m_item;
                        if (pCamera && static_cast<double>(fCameraMinTimer) < pCamera->Data.fMinTime) {
                            fCameraMinTimer = pCamera->Data.fMinTime;
                        }

                        if (pCamera && pCamera->GetIsFirstPersonCamera()) {
                            pPlayer->SetFirstPerson(true);
                        }
                        else {
                            bSwitchTo3rdPerson = true;
                        }
                    }
                }
                if (bNewCamera) {
                    UpdateMeleeAttack(pCommand);
                }
                pcameraCommand = pCommand;
                if (icameraIndex > 1) {
                    icameraIndex = 0;
                }
            }
            AnimIdle* pAnimIdle = pVATSAttacker->Actor::GetAnimation()->spAnimIdle.m_pObject;
            if (pMeleeAttack &&
                (!pAnimIdle ||
                 pAnimIdle->pIdleForm != pMeleeAttack ||
                 (pAnimIdle->eFlags >= AnimIdle::ANIM_IDLE_PLAYING && !pAnimIdle->GetSequence()))) {
                ANIM_GROUP_SECTION eSection = pMeleeAttack->GetIdleAnimGroupSection();
                TESIdleForm* pIdle = pMeleeAttack;
                Animation* pAnimation = pVATSAttacker->Actor::GetAnimation();
                pAnimation->SpecialIdleQueue(pIdle, eSection);
            }

            if (pCamera) {
                if (!pCamera->IsInitialized()) {
                    UpdateCurrentCameraAction(pCommand);
                    if (pCamera) {
                        int32_t iShotAction = pCamera->GetCameraAction();
                        if (eVATSCameraAction >= iShotAction || pCamera->GetCameraAction() == BGSCameraShot::CA_ZOOM) {
                            fCameraTimer = pCamera->Data.fMaxTime;
                            if (fCameraTimer <= 0.0010000000474974513) {
                                fCameraTimer = fVATSCameraMaxTime.Float();
                                fVATSCommandTimer = 0.0f;
                            }
                            if (fCameraMinTimer < static_cast<double>(fVATSCameraMinTime.Float())) {
                                fCameraMinTimer = fVATSCameraMinTime.Float();
                            }
                            if (pCommand->pTarget &&
                                (pCamera->Data.eLocation ||
                                 pCommand->eAction == ActionPoints::ACTION_TARGET_KILL_SHOT)) {
                                pCamera->InitCamera(pCommand->pTarget, eCameraZoom);
                            }
                            else {
                                pCamera->InitCamera(pVATSAttacker, eCameraZoom);
                            }
                            Animation* pAnimation = pVATSAttacker->Actor::GetAnimation();
                            NiAVObject* pAttacker3D = pAnimation->pAccumRoot->GetAtChecked(0);
                            NiAVObject* pAttackerWeapon = nullptr;
                            NiAVObject* pTarget3D = nullptr;
                            NiAVObject* pTargetTorso = nullptr;
                            if (pCamera->GetFollowsBone()) {
                                BaseProcess* pProcess = pVATSAttacker->pCurrentProcess;
                                BipedAnim* pBiped = pVATSAttacker->GetVATSBiped();
                                pAttackerWeapon = pProcess->GetWeaponBone(pBiped);
                            }
                            if (pCommand->pTarget) {
                                pTarget3D = pCommand->pTarget->Get3D();
                                if (pCommand->pTarget->IsActor()) {
                                    Actor* pActor = static_cast<Actor*>(pCommand->pTarget);
                                    TESActorBase* pActorBase = pActor->GetActorBase();
                                    BGSBodyPartData* pBodyPartData = pActorBase->GetBodyPartData();
                                    if (pBodyPartData) {
                                        BGSBodyPart* pBodyPart = nullptr;
                                        if (pCommand->pHitData->eDamageLimb != BGSBodyPart::LIMB_NONE) {
                                            pBodyPart = pBodyPartData->GetBodyPart(pCommand->eLimb);
                                            if (pBodyPart) {
                                                if (pActor->GetDismembered(pCommand->pHitData->eDamageLimb)) {
                                                    BaseProcess* pProcess = pActor->pCurrentProcess;
                                                    NiNode* pDismemberedRoot = pProcess->GetDamageNode(
                                                        pCommand->pHitData->eDamageLimb
                                                    );
                                                    if (pDismemberedRoot) {
                                                        pTargetTorso = BSUtilities::GetObjectByName(
                                                            pDismemberedRoot,
                                                            pBodyPart->GetTargetName()
                                                        );
                                                    }
                                                }
                                                else {
                                                    pTargetTorso = BSUtilities::GetObjectByName(
                                                        pTarget3D,
                                                        pBodyPart->GetTargetName()
                                                    );
                                                }
                                            }
                                        }
                                        if (!pTargetTorso) {
                                            pBodyPart = pBodyPartData->GetBodyPart(BGSBodyPart::LIMB_TORSO);
                                            if (pBodyPart) {
                                                pTargetTorso = BSUtilities::GetObjectByName(
                                                    pTarget3D,
                                                    pBodyPart->GetTargetName()
                                                );
                                            }
                                        }
                                    }
                                }
                            }
                            switch (pCamera->Data.eLocation) {
                                case BGSCameraShot::CO_ATTACKER:
                                    if (pAttackerWeapon) {
                                        pCamera->SetCameraLocation(pAttackerWeapon);
                                    }
                                    else {
                                        pCamera->SetCameraLocation(pAttacker3D);
                                    }
                                    break;
                                case BGSCameraShot::CO_TARGET:
                                    if (pTargetTorso) {
                                        pCamera->SetCameraLocation(pTargetTorso);
                                    }
                                    else if (pTarget3D) {
                                        pCamera->SetCameraLocation(pTarget3D);
                                    }
                                    else if (pAttackerWeapon) {
                                        pCamera->SetCameraLocation(pAttackerWeapon);
                                    }
                                    else {
                                        pCamera->SetCameraLocation(pAttacker3D);
                                    }
                                    break;
                                case BGSCameraShot::CO_PROJECTILE:
                                    if (!pCamera->GetLocationNode()) {
                                        if (pFirstProjectile) {
                                            NiNode* p3D = pFirstProjectile->Get3D();
                                            pCamera->SetCameraLocation(p3D);
                                        }
                                        else if (pLastProjectile) {
                                            NiNode* p3D = pLastProjectile->Get3D();
                                            pCamera->SetCameraLocation(p3D);
                                        }
                                        else if (pAttackerWeapon) {
                                            pCamera->SetCameraTarget(pAttackerWeapon);
                                        }
                                        else {
                                            pCamera->SetCameraTarget(pAttacker3D);
                                        }
                                    }
                                    break;
                                default: break;
                            }
                            switch (pCamera->Data.eTarget) {
                                case BGSCameraShot::CO_ATTACKER:
                                    if (pAttackerWeapon) {
                                        pCamera->SetCameraTarget(pAttackerWeapon);
                                    }
                                    else {
                                        pCamera->SetCameraTarget(pAttacker3D);
                                    }
                                    break;
                                case BGSCameraShot::CO_TARGET:
                                    if (pTargetTorso) {
                                        pCamera->SetCameraTarget(pTargetTorso);
                                    }
                                    else if (pTarget3D) {
                                        pCamera->SetCameraTarget(pTarget3D);
                                    }
                                    else if (pAttackerWeapon) {
                                        pCamera->SetCameraTarget(pAttackerWeapon);
                                    }
                                    else {
                                        pCamera->SetCameraTarget(pAttacker3D);
                                    }
                                    break;
                                case BGSCameraShot::CO_PROJECTILE:
                                    if (!pCamera->GetTargetNode()) {
                                        if (pFirstProjectile) {
                                            if (pCamera->GetNoTracer() &&
                                                pFirstProjectile->IsFlagSet(Projectile::TRACER)) {
                                                pFirstProjectile->SetFlag(Projectile::TRACER, false);
                                                pFirstProjectile->Process3D();
                                            }
                                            NiNode* p3D = pFirstProjectile->Get3D();
                                            pCamera->SetCameraTarget(p3D);
                                        }
                                        else if (pLastProjectile) {
                                            if (pCamera->GetNoTracer() &&
                                                pFirstProjectile->IsFlagSet(Projectile::TRACER)) {
                                                pLastProjectile->SetFlag(Projectile::TRACER, false);
                                                pLastProjectile->Process3D();
                                            }
                                            NiNode* p3D = pLastProjectile->Get3D();
                                            pCamera->SetCameraTarget(p3D);
                                        }
                                        else if (pAttackerWeapon) {
                                            pCamera->SetCameraTarget(pAttackerWeapon);
                                        }
                                        else {
                                            pCamera->SetCameraTarget(pAttacker3D);
                                        }
                                    }
                                    break;
                                default: break;
                            }
                            NiAVObject* pImagespaceTarget = pCamera->GetImagespaceTarget();
                            if (pImagespaceMod) {
                                pImagespaceMod->StartTransition(
                                    pCamera->pFormImageSpaceModifying,
                                    fVATSImageSpaceTransitionTime.Float(),
                                    1.0f,
                                    pImagespaceTarget
                                );
                            }
                            else {
                                pImagespaceMod = ImageSpaceModifierInstanceForm::Trigger(
                                    pCamera->pFormImageSpaceModifying,
                                    1.0f,
                                    pImagespaceTarget
                                );
                                if (pImagespaceMod) {
                                    pImagespaceMod->SetPermanent(true);
                                }
                            }
                            if (pCamera && !pCamera->IsInitialized()) {
                                UseNextCamera();
                            }
                        }
                    }
                }
                if (bDollyToNewPos && fDollyPercentage == 0.0) {
                    fDollyStartTime = fVATSPlaybackTimer;
                }
                double dDelta = appTimer.fDelta;
                fVATSPlaybackTimer = dDelta / BSTimer::QGlobalTimeMultiplier() + fVATSPlaybackTimer;
                dDelta = appTimer.fDelta;
                fVATSCommandTimer = dDelta / BSTimer::QGlobalTimeMultiplier() + fVATSCommandTimer;
                if (fVATSCameraMaxTime.Float() < static_cast<double>(fVATSCommandTimer)) {
                    QuitVATSPlayback(false, false);
                }
                if (pCamera && pCamera->IsInitialized()) {
                    if (bSwitchTo3rdPerson) {
                        pPlayer->SetFirstPerson(false);
                        bSwitchTo3rdPerson = false;
                    }
                    pCamera->Update(fVATSPlaybackTimer);
                    if (bDollyToNewPos) {
                        appTimer.SetGlobalTimeMultiplier(0.0001f, false);
                    }
                    else if (pcameraCommand && pcameraCommand->eAction == ActionPoints::ACTION_TARGET_KILL_SHOT) {
                        appTimer.SetGlobalTimeMultiplier(0.3f, false);
                    }
                    else {
                        float fGlobalTimeMult = pCamera->Data.fGlobalTimeMult;
                        appTimer.SetGlobalTimeMultiplier(fGlobalTimeMult, false);
                    }
                    float fTransStrength = fDollyPercentage;
                    if (fDollyPercentage > 0.5) {
                        fTransStrength = 1.0 - fTransStrength;
                    }
                    float fScaledTransStrength = fTransStrength * fVATSCamTransRBStrengthCap.Float();
                    pTransitionImagespaceMod->SetStrength(fScaledTransStrength);
                }
                else {
                    appTimer.SetGlobalTimeMultiplier(1.0f, false);
                }
            }
            float fUseActionPoints = 0.0f;
            if (ActionPoints::IsAttackAction(pCommand->eAction)) {
                if (IsMeleeCommand(pCommand) ||
                    VATSQueuedAction::IsActionGrenadeOrMine(pCommand->eAction) ||
                    pCommand->eAction == ActionPoints::ACTION_ATTACK_LAUNCHER ||
                    pPlayer->GetAnimAction() == ANIM_ACTION_RELOAD ||
                    pPlayer->GetAnimAction() == ANIM_ACTION_RELOAD_LOOP) {
                    bLastCommandBlock = 0;
                }
                else {
                    pControls->ForceUserAction(Controls::UACTN_BLOCK);
                    bLastCommandBlock = 1;
                }
                if (!pCommand->cFireShots) {
                    bool bRemoveCommand = false;
                    if (!pCamera || fCameraMinTimer == 0.0) {
                        if (IsMeleeCommand(pCommand)) {
                            UpdateCurrentCameraAction(pCommand);
                            if (pVATSAttacker->GetAnimAction() == ANIM_ACTION_NONE ||
                                TESAnimGroup::IsAttackSpinAction(
                                    pVATSAttacker->Actor::GetAnimation()->GetGroup(AGS_WEAPON)
                                )) {
                                bRemoveCommand = true;
                            }
                        }
                        else if (!pLastProjectile) {
                            bRemoveCommand = true;
                        }
                    }
                    if (bRemoveCommand) {
                        fUseActionPoints = pCommand->fActionPoints;
                        FreeHeadAction();
                        pCommand = nullptr;
                    }
                }
            }
            else if (bVATSFlushCommandWhenAnimActionIsDone) {
                if (pVATSAttacker->GetAnimAction() == ANIM_ACTION_NONE) {
                    fUseActionPoints = pCommand->fActionPoints;
                    FreeHeadAction();
                    pCommand = nullptr;
                }
            }
            else {
                switch (pCommand->eAction) {
                    case ActionPoints::ACTION_RELOAD: {
                        if (pVATSAttacker->GetAnimAction() == ANIM_ACTION_NONE) {
                            if (pCurrentProcess->GetCurrentWeapon()) {
                                ItemChange* pCurrentWeapon = pCurrentProcess->GetCurrentWeapon();
                                bool bClipMod = pCurrentWeapon->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE);
                                TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(
                                    pCurrentProcess->GetCurrentWeapon()->GetItemChangeObject()
                                );
                                pVATSAttacker->ReloadWeaponNV(pWeapon, 2, bClipMod, false);
                                pCurrentProcess->SetSustainedFire(false);
                            }
                            bVATSFlushCommandWhenAnimActionIsDone = true;
                        }
                        break;
                    }
                    case ActionPoints::ACTION_CROUCH:
                    case ActionPoints::ACTION_STAND: {
                        if (pVATSAttacker->GetAnimAction() == ANIM_ACTION_NONE) {
                            if (!pVATSAttacker->IsParalyzed() &&
                                !pVATSAttacker->IsDead(false) &&
                                !pVATSAttacker->IsKnockedOut() &&
                                !pVATSAttacker->IsRestrained() &&
                                !pVATSAttacker->IsUnConscious()) {
                                if (pVATSAttacker == pPlayer) {
                                    pPlayer->SetMoveMode(1024u, pCommand->eAction == ActionPoints::ACTION_CROUCH);
                                }
                                else {
                                    pVATSAttacker->SetForceSneak(pCommand->eAction == ActionPoints::ACTION_CROUCH);
                                }
                            }
                            fUseActionPoints = pCommand->fActionPoints;
                            FreeHeadAction();
                            pCommand = nullptr;
                        }
                        break;
                    }
                    case ActionPoints::ACTION_TOGGLE_WEAPON_DRAWN: {
                        if (pVATSAttacker->GetAnimAction() == ANIM_ACTION_NONE) {
                            bool bWantWeaponDrawn = pVATSAttacker->GetWantWeaponDrawn();
                            pVATSAttacker->SetWantWeaponDrawn(!bWantWeaponDrawn);
                            bVATSFlushCommandWhenAnimActionIsDone = true;
                        }
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
            if (fUseActionPoints > 0.0) {
                pPlayer->UseActionPoints(fUseActionPoints);
                fUseActionPoints = 0.0f;
            }
        }
        if (commandList.IsEmpty()) {
            if (fEndVATSDelay == 0.0) {
                fEndVATSDelay = fVATSPlaybackDelay.Float();
            }
            else {
                double dDelta = appTimer.fDelta;
                double dScaledDelta = BSTimer::GetGlobalTimeMultiplierTarget() * dDelta;
                fEndVATSDelay = fEndVATSDelay - dScaledDelta / BSTimer::QGlobalTimeMultiplier();
                if (fEndVATSDelay <= 0.0) {
                    appTimer.SetGlobalTimeMultiplier(1.0f, false);
                    if (pVATSAttacker == VATS::pStranger ||
                        pVATSAttacker == VATS::pMissFortune ||
                        pVATSAttacker->GetAnimAction() == ANIM_ACTION_NONE ||
                        pVATSAttacker->GetAnimAction() == ANIM_ACTION_BLOCK ||
                        pVATSAttacker->GetAnimAction() == ANIM_ACTION_RELOAD ||
                        pVATSAttacker->GetAnimAction() == ANIM_ACTION_RELOAD_LOOP_START ||
                        pVATSAttacker->GetAnimAction() == ANIM_ACTION_RELOAD_LOOP) {
                        SetMode(VATS::VATS_MODE_NONE, false);
                        if (iKillCount > 0) {
                            fAPReward = 0.0f;
                            TESObjectWEAP* pPlayerWeapon = pPlayer->GetCurrentWeapon();
                            if (!pPlayerWeapon) {
                                pPlayerWeapon = pFists;
                            }
                            BGSEntryPoint::HandleEntryPoint(
                                BGSEntryPoint::PLAYER_KILL_AP_REWARD,
                                pPlayer,
                                pPlayerWeapon,
                                pPlayer,
                                &fAPReward
                            );
                            if (fAPReward > 0.0) {
                                pPlayer->RestoreActorValue(ActorValue::ACTION_POINTS, fAPReward);
                            }
                            iKillCount = 0;
                        }
                        fEndVATSDelay = 0.0f;
                    }
                }
                else {
                    dDelta = appTimer.fDelta;
                    fVATSPlaybackTimer = dDelta / BSTimer::QGlobalTimeMultiplier() + fVATSPlaybackTimer;
                    if (pCamera && pCamera->IsInitialized()) {
                        pCamera->Update(fVATSPlaybackTimer);
                    }

                    if (bDollyToNewPos) {
                        appTimer.SetGlobalTimeMultiplier(0.0001f, false);
                    }
                    else if (pCamera && pCamera->IsInitialized()) {
                        float fGlobalTimeMult = pCamera->Data.fGlobalTimeMult;
                        appTimer.SetGlobalTimeMultiplier(fGlobalTimeMult, false);
                    }
                    else {
                        appTimer.SetGlobalTimeMultiplier(1.0f, false);
                    }
                    if (bLastCommandBlock &&
                        pPlayer->GetAnimAction() != ANIM_ACTION_RELOAD &&
                        pPlayer->GetAnimAction() != ANIM_ACTION_RELOAD_LOOP) {
                        Controls* pControls = appMain->GetControls();
                        pControls->ForceUserAction(Controls::UACTN_BLOCK);
                    }
                }
            }
        }
        if (pVATSTarget && eMode) {
            pVATS.UpdateVATSLight(pVATSTarget);
        }
    }
}

void VATS::FreeHeadAction() {
    if (!commandList.IsEmpty()) {
        VATS_COMMAND* pCommand = commandList.m_item;
        if (pCommand) {
            delete pCommand;
            pCommand = nullptr;
            commandList.RemoveHead();
            if (pMeleeAttack) {
                Animation* pAnimation = pVATSAttacker->Actor::GetAnimation();
                pAnimation->SpecialIdleFree(true, false);
                pMeleeAttack = nullptr;
            }
            while (!commandList.IsEmpty()) {
                pCommand = commandList.m_item;
                bool bSkip = false;
                if (pCommand && pCommand->pTarget) {
                    if (pCommand->pTarget->IsDead(false)) {
                        bSkip = true;
                    }
                    else if (pCommand->bStranger) {
                        double dHealth = pCommand->pTarget->GetHealth();
                        bSkip = static_cast<double>(iVATSStrangerMaxHP.Int()) < dHealth;
                    }
                }

                if (!bSkip) {
                    break;
                }
                if (pCommand) {
                    delete pCommand;
                }
                pCommand = nullptr;
                commandList.RemoveHead();
            }
            if (pCommand && pCommand->bStranger) {
                pVATSAttacker = VATS::pStranger;
                MiscStatManager::Increment(MiscStatManager::MS_STRANGER_ASSISTS);
            }

            if (pCommand && pCommand->bMissFortune) {
                pVATSAttacker = VATS::pMissFortune;
                MiscStatManager::Increment(MiscStatManager::MS_MISS_FORTUNE);
            }
        }
    }
}

void VATS::UpdateMeleeAttack(VATS_COMMAND* apCommand) {
    NiPoint3 kSafePoint;
    if (apCommand->cFireShots) {
        if ((pVATSAttacker == VATS::pStranger || pVATSAttacker == VATS::pMissFortune || IsMeleeCommand(apCommand)) &&
            apCommand->pTarget) {
            float fDistance = 0.0;
            if (apCommand->bStranger) {
                fDistance = fVATSStrangerDistance.Float();
                if (VATS::pStranger->GetAlphaMult() != 1.0) {
                    VATS::pStranger->SetAlphaMult(1.0);
                    VATS::pStranger->UpdateAlpha();
                    PlayMysteriousStrangerMusic();
                }
            }
            else if (apCommand->bMissFortune) {
                fDistance = fVATSStrangerDistance.Float();
                if (VATS::pMissFortune->GetAlphaMult() != 1.0) {
                    VATS::pMissFortune->SetAlphaMult(1.0);
                    VATS::pMissFortune->UpdateAlpha();
                    PlayMysteriousStrangerMusic();
                }
            }
            else {
                TESObjectWEAP* pCurrentWeapon = pVATSAttacker->GetCurrentWeapon();
                fDistance = CombatUtilities::GetWeaponReach(pVATSAttacker, pCurrentWeapon);
                if (pVATSAttacker->GetCurrentWeapon()) {
                    TESObjectWEAP* pWeapon = pVATSAttacker->GetCurrentWeapon();
                    if (pWeapon->IsMeleeWeapon()) {
                        fDistance = fDistance * fVATSMeleeWarpDistanceMult.Float();
                    }
                }
                else {
                    fDistance = fDistance * fVATSH2HWarpDistanceMult.Float();
                }
            }
            if (pVATSAttacker == VATS::pStranger || pVATSAttacker == VATS::pMissFortune) {
                const NiPoint3& kPlayerPos = pPlayer->GetLocationOnReference();
                pVATSAttacker->SetLocationOnReference(kPlayerPos);
            }
            if (Pathing::FindSafeStraightLinePathPoint(pVATSAttacker, apCommand->pTarget, fDistance, &kSafePoint)) {
                pVATSAttacker->SetPosition(kSafePoint);
                pVATSAttacker->SetHeading(apCommand->pTarget);
                if (pPlayer == pVATSAttacker && !GetLineOfSight(apCommand)) {
                    const NiPoint3& kTargetPos = apCommand->pTarget->GetLocationOnReference();
                    pVATSAttacker->SetPosition(kTargetPos);
                    pVATSAttacker->SetHeading(apCommand->pTarget);
                }
            }
            else {
                const NiPoint3& kTargetPos = apCommand->pTarget->GetLocationOnReference();
                pVATSAttacker->SetPosition(kTargetPos);
                pVATSAttacker->SetHeading(apCommand->pTarget);
            }
            Animation* pAnimation = pVATSAttacker->GetAnimation();
            pAnimation->UpdateSceneGraphNoController();
        }

        pMeleeAttack = pIdleManager->GetIdleToPlay(pVATSAttacker, apCommand->pTarget);
        if (pMeleeAttack && pMeleeAttack->GetIdleAnimGroupSection() != AGS_WEAPON) {
            const char* pEditorID = pMeleeAttack->GetFormEditorID();
            Warning(
                "COMBAT: Bad VATS Melee Attack '%s' in the idle manager. Attacks should be tagged for 'Weapon' "
                "anim group section.",
                pEditorID
            );
            pMeleeAttack = nullptr;
        }
    }
}

void VATS::UpdateVATSLight(TESObjectREFR* apVictim) {
    if (apVictim->IsActor() && apVictim->Get3D() && spVATSLight.m_pObject) {
        Actor* pActor = static_cast<Actor*>(apVictim);
        if (pActor->GetCurrentAIProcess()) {
            NiMatrix3 rotmat;
            NiNode* pVictim3D = apVictim->Get3D();
            NiPoint3 pos = pVictim3D->GetWorldBound().m_kCenter;
            const NiPoint3& kVictimPos = apVictim->GetLocationOnReference();
            const NiPoint3& kAttackerPos = pVATSAttacker->GetLocationOnReference();
            NiPoint3 vec = kAttackerPos - kVictimPos;
            rotmat.MakeZRotation(gs_fVATSLightAngle_VATS);
            vec.Unitize();
            vec = vec * rotmat;
            vec *= gs_fVATSLightDistance_VATS;
            pos = pos + vec;
            double dElevation = static_cast<float>(gs_fVATSLightElevation_VATS);
            pos.z = dElevation + pos.z;
            float fColorB = gs_fVatsLightColorB_VATS.Float();
            float fColorG = gs_fVatsLightColorG_VATS.Float();
            NiColor lightcolor(gs_fVatsLightColorR_VATS.Float(), fColorG, fColorB);
            int32_t inumberlights = 0;
            float fambient = 0.0f;
            float fdirectional = 0.0f;
            float fHeight = pActor->GetHeight();
            NiPoint3 kHeightOffset(0.0f, 0.0f, fHeight);
            const NiPoint3& kActorPos = pActor->GetLocationOnReference();
            NiPoint3 ktarget = kActorPos + kHeightOffset;
            NiLight* pLight = spVATSLight->GetLight();
            NiPoint3 kAttackerLocation = pVATSAttacker->GetLocationOnReference();
            NiPoint3 kLuminancePoint = ktarget;
            ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(0);
            float ftargetluminance = pShadowSceneNode->GetViewLuminanceAtPoint(
                                         kLuminancePoint,
                                         inumberlights,
                                         fambient,
                                         fdirectional,
                                         kAttackerLocation,
                                         pLight
                                     ) *
                100.0;
            float
                intensity = FLerp(1.0f, 0.0f, gs_fVATSLightLevelMin_VATS, gs_fVATSLightLevelMax_VATS, ftargetluminance);
            intensity = BSMAX(0.0f, intensity);
            intensity = BSMIN(1.0f, intensity);
            lightcolor *= intensity;
            pVictim3D = apVictim->Get3D();
            double dDistance = (pos - pVictim3D->GetWorldBound().m_kCenter).Length();
            float radius = dDistance + dDistance;
            pLight = spVATSLight->GetLight();
            pLight->SetWorldTranslate(pos);
            NiColor specularcolor(radius, 0.0f, 0.0f);
            pLight = spVATSLight->GetLight();
            pLight->SetSpecularColor(specularcolor);
            pLight = spVATSLight->GetLight();
            pLight->SetDiffuseColor(lightcolor);
            pLight = spVATSLight->GetLight();
            pLight->SetAppCulled(false);
        }
    }
}

void VATS::RemoveLastAction(ActionPoints::Action& aeAction, bool& abIronSights) {
    for (BSSimpleList<VATS_COMMAND*>* iter = &commandList; iter && !iter->IsEmpty(); iter = iter->m_pkNext) {
        if (!iter->m_pkNext) {
            VATS_COMMAND* pCommand = iter->m_item;
            aeAction = pCommand->eAction;
            abIronSights = pCommand->bIronSights;
            if (pCommand) {
                delete pCommand;
            }
            commandList.Remove(pCommand);
            return;
        }
    }
}

void VATS::QuitVATSPlayback_0(TESBoundObject* apObject, ExtraDataList* apExtra) {
    pUnEquipWeapon = apObject;
    pUnEquipExtra = apExtra;
    QuitVATSPlayback(true, true);
}

NiPoint3 VATS::GetCamshotTargetPos() {
    NiPoint3 kTargetPos = NiPoint3::ZERO;
    if (pCamera) {
        kTargetPos = pCamera->GetTargetPos();
    }
    if (kTargetPos == NiPoint3::ZERO) {
        float fEyeHeight = pPlayer->GetEyeHeight();
        NiPoint3 kEyeOffset(static_cast<float>(0.0), static_cast<float>(0.0), fEyeHeight);
        NiNode* pAttacker3D = pVATSAttacker->TESObjectREFR::Get3D();
        kTargetPos = pAttacker3D->GetWorldTranslate() + kEyeOffset;
    }
    return kTargetPos;
}

bool VATS::GetLineOfSight(VATS_COMMAND* apCommand) {
    CFilter fltr;
    bhkWorld* pw;
    Actor* pTarget = nullptr;
    if (apCommand->pTarget->IsActor()) {
        pTarget = static_cast<Actor*>(apCommand->pTarget);
    }
    if (!pTarget) {
        return false;
    }
    if (pTarget->GetParentCell()) {
        pw = pTarget->GetParentCell()->GetbhkWorld();
    }
    else {
        pw = nullptr;
    }
    if (!pw) {
        return false;
    }
    bhkCollisionFilter::SetMultiLayerCollision(L_CUSTOMPICK1, 0x26206u);
    NiPoint3 aLocation = pVATSAttacker->GetLocationOnReference();
    alignas(16) NiPoint3 targetLocation = pTarget->GetLocationOnReference();
    aLocation.z = aLocation.z + 60.0;
    targetLocation.z = targetLocation.z + 60.0;
    bhkPickData pd;
    fltr.iFilter = 0;
    fltr.SetLayer(L_CUSTOMPICK1);
    bool bIsActor = pTarget->IsActor();
    Actor* pactorTarget = bIsActor ? pTarget : nullptr;
    if (pactorTarget && pactorTarget->GetCharController()) {
        bhkCharacterController* pCharController = pactorTarget->GetCharController();
        uint32_t uiGroup = pCharController->GetFilterGroup();
        fltr.SetGroup(uiGroup);
    }
    else {
        uint32_t uiGroup = bhkCollisionFilter::GetNewSystemGroup();
        fltr.SetGroup(uiGroup);
    }
    pd.SetFilter(fltr);
    pd.NiSetFrom(aLocation);
    pd.NiSetTo(targetLocation);
    hkpAllRayHitCollector collector;
    pd.SetAllCollector(&collector);
    char bDone = 0;
    pw->PickObject(pd);
    for (int32_t i = 0; !bDone; ++i) {
        int32_t iHitCount = pd.pAllCollector->getHits().m_size;
        if (i >= iHitCount) {
            break;
        }
        hkpWorldRayCastOutput point(pd.pAllCollector->getHits()[i]);
        NiAVObject* pAV = GetAVObjectForCollidable(point.m_rootCollidable);
        TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pAV);
        if (pAV && pRef != pTarget && pRef != pVATSAttacker) {
            return false;
        }
    }
    return true;
}

void VATS::SetVATSLight(NiLight* apLight, bool abRemoveOld) {
    if (abRemoveOld && spVATSLight != static_cast<ShadowSceneLight*>(nullptr)) {
        spVATSLight = static_cast<ShadowSceneLight*>(nullptr);
    }
    if (apLight) {
        ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(0);
        spVATSLight = pShadowSceneNode->AddLight(apLight, true);
        pShadowSceneNode = BSShaderManager::GetShadowSceneNode(0);
        pShadowSceneNode->AttachChild(apLight, 1);
    }
}

void VATS::AppendCamTarget(TESObjectREFR* apTarget, int32_t aiLimb) {
    QuitVATSPlayback(false, false);
    VATS_COMMAND* pCommand = new VATS_COMMAND;
    pCommand->eAction = ActionPoints::ACTION_TARGET_KILL_SHOT;
    pCommand->bSuccess = true;
    pCommand->pTarget = apTarget;
    if (aiLimb == -1) {
        aiLimb = 0;
    }
    pCommand->pHitData->eDamageLimb = static_cast<BGSBodyPart::LIMB_ENUM>(aiLimb);
    if (pPlayer->pCurrentProcess) {
        BaseProcess* pProcess = pPlayer->pCurrentProcess;
        if (pProcess->GetCurrentWeapon()) {
            pProcess = pPlayer->pCurrentProcess;
            pCommand->pHitData->pWeapon = static_cast<TESObjectWEAP*>(pProcess->GetCurrentWeapon()->pContainerObj);
        }
    }
    pCommand->pHitData->SetFlag(HitData::HD_FATAL, 1);
    pVATSAttacker = pPlayer;
    QueueAction(pCommand);
    SetMode(VATS::VATS_PLAYBACK, false);
}

uint32_t VATS::GetCount() {
    // The queued commands counted in place: the head node is this list's inline first node.
    uint32_t uiCount = 0;
    for (const BSSimpleList<VATS_COMMAND*>* pNode = &commandList; pNode; pNode = pNode->GetNext()) {
        if (pNode->GetItem()) {
            ++uiCount;
        }
    }
    return uiCount;
}

VATS_COMMAND::VATS_COMMAND() {
    Initialize();
}

VATS_COMMAND::~VATS_COMMAND() {
    if (pHitData) {
        ourFree(pHitData);
        pHitData = nullptr;
    }
}

void VATS_COMMAND::Initialize() {
    eAction = ActionPoints::ACTION_COUNT;
    bSuccess = false;
    bIronSights = false;
    bStranger = false;
    bParalyzingPalm = false;
    cFireShots = 0;
    cFakeFireShots = 0;
    pTarget = nullptr;
    eLimb = ActorValue::ACTOR_VALUE_COUNT;
    pHitData = new HitData;
    fTotalDamage = 0.0f;
    fActionPoints = 0.0f;
    fCumulativeDamage = 0.0f;
    bMissFortune = false;
}

void VATS_COMMAND::operator delete(void* apMemory) {
    ourFree(apMemory);
}

float bhkWorld::QMaxPickTime() {
    if (bhkWorld::bVatsReplay) {
        return bhkWorld::fMaxPickTimeVATS;
    }

    return bhkWorld::fMaxPickTime;
}
