#include "Fallout/Interface/Menus/LockPickMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSRandom.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Controls.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileImage.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/ExperiencePoints.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/LockLevel.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectMISC.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/stdlib.h"

#include <cmath>

g_iSweetSpotLengthMinArray_t g_iSweetSpotLengthMinArray{
    &iSweetSpotLengthMinVeryEasy,
    &iSweetSpotLengthMinEasy,
    &iSweetSpotLengthMinAverage,
    &iSweetSpotLengthMinHard,
    &iSweetSpotLengthMinVeryHard
};
g_iSweetSpotLengthMaxArray_t g_iSweetSpotLengthMaxArray{
    &iSweetSpotLengthMaxVeryEasy,
    &iSweetSpotLengthMaxEasy,
    &iSweetSpotLengthMaxAverage,
    &iSweetSpotLengthMaxHard,
    &iSweetSpotLengthMaxVeryHard
};
g_iConcentricLengthMinArray_t g_iConcentricLengthMinArray{
    &iConcentricLengthMinVeryEasy,
    &iConcentricLengthMinEasy,
    &iConcentricLengthMinAverage,
    &iConcentricLengthMinHard,
    &iConcentricLengthMinVeryHard
};
g_iConcentricLengthMaxArray_t g_iConcentricLengthMaxArray{
    &iConcentricLengthMaxVeryEasy,
    &iConcentricLengthMaxEasy,
    &iConcentricLengthMaxAverage,
    &iConcentricLengthMaxHard,
    &iConcentricLengthMaxVeryHard
};
NiPointer<NiNode> LockPickMenu::spCameraRoot;
NiPointer<NiCamera> LockPickMenu::spCamera;

LockPickMenu::LockPickMenu()
  : eLockState(LockPickMenu::LS_INTRO_ANIM), iLockpickSkill(pPlayer->GetClampedActorValue(ActorValue::LOCKPICK)),
    fLockpickSkill(static_cast<float>(iLockpickSkill)), uiPrevIdleTick(0), fLockAngle(0.0f),
    fPickHealth(fLockpickSkill * fLockpickBonusHealth.Float() + 1.0), bForceLockSuccess(false), bShowingTutorial(false),
    pIntroAnim(nullptr), pLockRotateAnim(nullptr), pPickIntroAnim(nullptr), pPickBendAnim(nullptr),
    pPickBreakAnim(nullptr) {
    LockPickMenu::iUT_Value = Tile::AddUserTrait("_Value", -1);
    LockPickMenu::iUT_X = Tile::AddUserTrait("_x", -1);
}

LockPickMenu::~LockPickMenu() {
    BSShaderManager::GetShadowSceneNode(1)->DetachChild(LockPickMenu::spCameraRoot.m_pObject);
    LockPickMenu::spCameraRoot->DetachAllChildren();
    LockPickMenu::spCameraRoot = static_cast<NiNode*>(nullptr);
    LockPickMenu::spCamera = static_cast<NiCamera*>(nullptr);
    LockPickMenu::pMe = nullptr;
    BSShaderManager::GetShadowSceneNode(1)->RemoveAllLights();
}

void LockPickMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (static_cast<uint32_t>(aiID) < 16) {
        pTiles[aiID] = apTile;
    }
}

void LockPickMenu::DoClick(int32_t aiID, Tile* apTarget) {
    int32_t fdoPunishment;
    int32_t itweakIncrAmount = 5;
    switch (aiID) {
        case 3:
        case 5: {
            int32_t iSkillLevelRange = LockLevelMaxVeryEasy[eLockLevel + 1] - LockLevelMaxVeryEasy[eLockLevel];
            float fLockLevelRatio = static_cast<double>(iLockpickSkill - LockLevelMaxVeryEasy[eLockLevel]) /
                (static_cast<double>(iSkillLevelRange) + static_cast<double>(iSkillLevelRange));
            fLockLevelRatio = BSMIN(fLockLevelRatio, 1.0f);
            fLockLevelRatio = BSMAX(fLockLevelRatio, 0.0f);
            int32_t iDelta = itweakIncrAmount * (2 * (aiID != 5) - 1);
            BSStringT<char> strtemp;
            g_iconcentricLength += iDelta;
            iTotalSweetSpotRadius += iDelta / 2;
            strtemp.SPrintF("%d", g_iconcentricLength);
            pTiles[4]->Set(4036, strtemp.pString, true);
            g_iSweetSpotLengthMinArray[eLockLevel]->operator=(
                g_iSweetSpotLengthMinArray[eLockLevel]->Int() +
                static_cast<int32_t>(static_cast<double>(iDelta) * (1.0 - fLockLevelRatio))
            );
            g_iSweetSpotLengthMaxArray[eLockLevel]->operator=(
                g_iSweetSpotLengthMaxArray[eLockLevel]->Int() +
                static_cast<int32_t>(static_cast<double>(iDelta) * fLockLevelRatio)
            );
            for (int32_t iMeterCt = 0; iMeterCt < 6; ++iMeterCt) {
                Tile* pCurrTile = pTiles[14]->GetChildByID(iMeterCt + 17);
                float fWidth = pCurrTile->GetFloat(4017) + static_cast<double>(iDelta);
                pCurrTile->Set(4017, fWidth, true);
                float fX = pCurrTile->GetFloat(4001) - static_cast<double>(iDelta / 2);
                pCurrTile->Set(4001, fX, true);
            }
            break;
        }
        case 6:
        case 8: {
            int32_t iSkillLevelRange = LockLevelMaxVeryEasy[eLockLevel + 1] - LockLevelMaxVeryEasy[eLockLevel];
            float fLockLevelRatio = static_cast<double>(iLockpickSkill - LockLevelMaxVeryEasy[eLockLevel]) /
                (static_cast<double>(iSkillLevelRange) + static_cast<double>(iSkillLevelRange));
            fLockLevelRatio = BSMIN(fLockLevelRatio, 1.0f);
            fLockLevelRatio = BSMAX(fLockLevelRatio, 0.0f);
            int32_t iDelta = itweakIncrAmount * (2 * (aiID != 8) - 1);
            BSStringT<char> strtemp;
            g_isweetSpotLength += iDelta;
            iTotalSweetSpotRadius += iDelta;
            strtemp.SPrintF("%d", g_isweetSpotLength);
            pTiles[7]->Set(4036, strtemp.pString, true);
            g_iConcentricLengthMinArray[eLockLevel]->operator=(
                g_iConcentricLengthMinArray[eLockLevel]->Int() +
                static_cast<int32_t>(static_cast<double>(iDelta) * (1.0 - fLockLevelRatio))
            );
            g_iConcentricLengthMaxArray[eLockLevel]->operator=(
                g_iConcentricLengthMaxArray[eLockLevel]->Int() +
                static_cast<int32_t>(static_cast<double>(iDelta) * fLockLevelRatio)
            );
            for (int32_t iMeterCt = 1; iMeterCt < 6; ++iMeterCt) {
                Tile* pCurrTile = pTiles[14]->GetChildByID(iMeterCt + 17);
                float fWidth = pCurrTile->GetFloat(4017) +
                    static_cast<double>(iMeterCt) * 2.0 * static_cast<double>(iDelta) / 5.0;
                pCurrTile->Set(4017, fWidth, true);
                float fX = pCurrTile->GetFloat(4001) - static_cast<double>(iDelta * iMeterCt) / 5.0;
                pCurrTile->Set(4001, fX, true);
            }
            break;
        }
        case 9: {
            *reinterpret_cast<float*>(&fdoPunishment) = 0.0f;
            BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::IGNORE_BROKEN_LOCK, pPlayer, &fdoPunishment);
            uint32_t uiRoll = BSRandom::GetSingleton()->UnsignedInt(100u);
            int32_t iLockLevelSkill = LockLevelMaxVeryEasy[eLockLevel];
            int32_t iPlayerSkill = pPlayer->GetClampedActorValue(ActorValue::LOCKPICK);
            if (uiRoll >= GamePlayFormulas::GetForceLockChance(iPlayerSkill, iLockLevelSkill)) {
                if (pLockTarget || *reinterpret_cast<float*>(&fdoPunishment) != 0.0) {
                    ++pLockTarget->GetLock()->iNumTries;
                    pLockTarget->AddLockChange();
                    Interface::ShowMessage(
                        sLockBroken.String(),
                        0,
                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds",
                        "UILockpickingForceFail",
                        2.0f,
                        false
                    );
                    LockPickMenu::Close();
                }
                else {
                    pTiles[11]->Set(4036, "LOCK BROKEN!  :(", true);
                    pTiles[11]->Set(4003, 1);
                    eLockState = LockPickMenu::LS_RESETTING_LOCK;
                }
            }
            else {
                bForceLockSuccess = true;
                fMaxLockAngle = 90.0f;
                eLockState = LockPickMenu::LS_TURNING_LOCK;
            }
            break;
        }
        case 10: {
            LockPickMenu::Close();
            break;
        }
        default: {
            break;
        }
    }
}

void LockPickMenu::DoEnter(int32_t aiID, Tile* apTarget) {
    if (aiID == 13) {
        InterfaceManager::Instance()->GetCursorTile()->Set(4009, 0.0f, true);
    }
}

void LockPickMenu::DoLeave(int32_t aiID, Tile* apTarget) {
    if (aiID == 13) {
        InterfaceManager::Instance()->GetCursorTile()->Set(4009, 255.0f, true);
    }
}

void LockPickMenu::DoIdle() {
    static int32_t iLockShakeDirection = 1;
    int32_t iElaspedMillis = 0;
    float fXAxisRatio = 1.0f;
    static BSSoundHandle lockpickSound;
    Interface::enumMenus eMenuID = static_cast<Interface::enumMenus>(GetClass());
    bool bIsMenuOnTop = Interface::IsTopMenuID(eMenuID);
    if (uiPrevIdleTick) {
        iElaspedMillis = appTimer.iLastTime - uiPrevIdleTick;
        if (eprevLockState == eLockState) {
            ftotalStateSecs_1 = static_cast<double>(iElaspedMillis) / 1000.0 + ftotalStateSecs_1;
        }
        else {
            ftotalStateSecs_1 = 0.0f;
        }
    }
    else {
        iElaspedMillis = 0;
        ftotalStateSecs_1 = 0.0f;
        eprevLockState = 0;
    }
    uiPrevIdleTick = appTimer.iLastTime;
    eprevLockState = eLockState;
    if (bShowingTutorial) {
        if (!bIsMenuOnTop) {
            return;
        }
        Interface::TutorialMessageID eTutorialID = LockPickMenu::GetTutorialMessageID();
        InterfaceManager* pInterfaceManager = InterfaceManager::Instance();
        if (!pInterfaceManager->xTutorialManager.HasBeenShown(eTutorialID)) {
            return;
        }
        bShowingTutorial = false;
    }
    if (!bIsMenuOnTop) {
        InterfaceManager::Instance()->pCursor->Set(4009, 255.0f, true);
    }
    int32_t iXAxis = 0;
    const int32_t ciMaxAxisValue = 0x7FFF;
    if (InterfaceManager::Instance()->Has360Controller()) {
        iXAxis = appMain->GetControls()->GetControllerValue(0, 9);
        if (NiAbs(static_cast<float>(iXAxis)) <= 8689.0) {
            iXAxis = 0;
        }
        fXAxisRatio = static_cast<double>(-iXAxis) / static_cast<double>(ciMaxAxisValue);
    }
    switch (eLockState) {
        case LockPickMenu::LS_INTRO_ANIM: {
            if (pIntroAnim && pPickIntroAnim) {
                NiUpdateData UpdateData1(ftotalStateSecs_1, true, false);
                NiUpdateData UpdateData2(ftotalStateSecs_1, true, false);
                LockPickMenu::spCameraRoot->GetAt(0)->Update(UpdateData1);
                if (ftotalStateSecs_1 == 0.0 || !pLockTarget || pTiles[1]->GetFloat(LockPickMenu::iUT_Value) > 0.0) {
                    LockPickMenu::spCameraRoot->GetAt(1u)->Update(UpdateData2);
                }
                if (ftotalStateSecs_1 == 0.0 && pLockTarget) {
                    if (pTiles[1]->GetFloat(LockPickMenu::iUT_Value) <= 0.0) {
                        Interface::ShowMessage(
                            sOutOfLockpicks.String(),
                            0,
                            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds",
                            nullptr,
                            2.0f,
                            false
                        );
                    }
                    else {
                        BSAudio::QInstance().GetSoundHandleByName("UILockpickingEnter", 289u).Play(false);
                    }
                }

                if (static_cast<double>(ftotalStateSecs_1) >= pIntroAnim->m_fEndKeyTime) {
                    NiAVObject* pLockGeometry = LockPickMenu::spCameraRoot->GetAt(0);
                    NiControllerManager* pLockAnimManager = static_cast<NiControllerManager*>(
                        pLockGeometry->GetController(&NiControllerManager::ms_RTTI)
                    );
                    eLockState = LockPickMenu::LS_IDLE;
                    pLockAnimManager->DeactivateSequence(pIntroAnim, 0.0f);
                    pLockAnimManager->ActivateSequence(pLockRotateAnim, 0, true, 1.0f, 0.0f, nullptr);
                    ppickAnimManager->DeactivateSequence(pPickIntroAnim, 0.0f);
                    ppickAnimManager->ActivateSequence(pPickBendAnim, 0, true, 1.0f, 0.0f, nullptr);
                }
            }

            break;
        }
        case LockPickMenu::LS_PICK_INTRO_ANIM: {
            if (pPickIntroAnim) {
                NiUpdateData UpdateData(ftotalStateSecs_1, true, false);
                LockPickMenu::spCameraRoot->GetAt(1u)->Update(UpdateData);
                if (static_cast<double>(ftotalStateSecs_1) >= pPickIntroAnim->m_fEndKeyTime) {
                    eLockState = LockPickMenu::LS_IDLE;
                    ppickAnimManager->DeactivateSequence(pPickIntroAnim, 0.0f);
                    ppickAnimManager->ActivateSequence(pPickBendAnim, 0, true, 1.0f, 0.0f, nullptr);
                }
            }
            break;
        }
        case LockPickMenu::LS_IDLE: {
            Interface::TutorialMessageID eTutorialID;
            if (!ReturnTrue() ||
                (eTutorialID = LockPickMenu::GetTutorialMessageID(),
                 InterfaceManager::Instance()->xTutorialManager.HasBeenShown(eTutorialID))) {
                if (bIsMenuOnTop) {
                    if (!pLockTarget || pTiles[1]->GetFloat(LockPickMenu::iUT_Value) > 0.0) {
                        if ((InterfaceManager::Has360Controller() && fXAxisRatio > 0.0) ||
                            (!InterfaceManager::Has360Controller() && GetPCInput(1))) {
                            BSAudio::QInstance().GetSoundHandleByName("UILockpickingCylinderTurn", 289u).Play(false);
                            eLockState = LockPickMenu::LS_TURNING_LOCK;
                        }
                        RotateLockpick();
                    }
                    if (fLockAngle > 0.0) {
                        float fPrevLockAngle = fLockAngle;
                        fLockAngle = fLockAngle - static_cast<double>(90 * iElaspedMillis) / 1000.0;
                        fLockAngle = BSMAX(fLockAngle, 0.0f);
                        RotateLock(0.0f);
                        if (fLockAngle < 30.0 && fPrevLockAngle > 30.0) {
                            BSAudio::QInstance().GetSoundHandleByName("UILockpickingCylinderSqueakA", 257u).Play(false);
                        }
                        if (fLockAngle < 60.0 && fPrevLockAngle > 60.0) {
                            BSAudio::QInstance().GetSoundHandleByName("UILockpickingCylinderSqueakB", 257u).Play(false);
                        }
                    }
                }
            }
            else {
                int32_t iMenuClass = LockPickMenu::GetClassStatic();
                eTutorialID = LockPickMenu::GetTutorialMessageID();
                bShowingTutorial = InterfaceManager::Instance()
                                       ->xTutorialManager.ShowMessage(eTutorialID, iMenuClass, 0);
            }
            break;
        }
        case LockPickMenu::LS_TURNING_LOCK: {
            if (bForceLockSuccess ||
                (bIsMenuOnTop &&
                 (!InterfaceManager::Has360Controller() || !(fXAxisRatio <= 0.0)) &&
                 (InterfaceManager::Has360Controller() || GetPCInput(0)))) {
                float fPrevLockAngle = fLockAngle;
                if (bForceLockSuccess) {
                    fXAxisRatio = 1.0f;
                }
                if (fMaxLockAngle >= static_cast<double>(fLockAngle)) {
                    fLockAngle = static_cast<double>(90 * iElaspedMillis) * fXAxisRatio / 1000.0 + fLockAngle;
                    fLockAngle = BSMIN(fLockAngle, fMaxLockAngle);
                }
                RotateLock(0.0f);
                if (InterfaceManager::Has360Controller()) {
                    appMain->GetControls()->Rumble(0.1f, 0.0f, 250, 125, PERIODIC_SAWTOOTH, true, false);
                }
                if (fPrevLockAngle < 30.0 && fLockAngle > 30.0) {
                    BSAudio::QInstance().GetSoundHandleByName("UILockpickingCylinderSqueakA", 257u).Play(false);
                }
                if (fPrevLockAngle < 60.0 && fLockAngle > 60.0) {
                    BSAudio::QInstance().GetSoundHandleByName("UILockpickingCylinderSqueakB", 257u).Play(false);
                }
                if (fMaxLockAngle <= static_cast<double>(fLockAngle)) {
                    if (!(fLockAngle >= 90.0)) {
                        BSAudio::QInstance().GetSoundHandleByName("UILockpickingCylinderStop", 257u).Play(false);
                        lockpickSound = BSAudio::QInstance().GetSoundHandleByName("UILockpickingPickTensionLPM", 1u);
                        lockpickSound.Play(true);
                        eLockState = LockPickMenu::LS_DAMAGING_PICK;
                    }
                    else if (pLockTarget) {
                        MiscStatManager::Increment(MiscStatManager::MS_LOCKS_PICKED);
                        if (!pLockTarget->GetLock()->iTimesUnlocked) {
                            int32_t iExperiencePoints = ExperiencePoints::GetExperiencePoints(
                                ExperiencePoints::ACTIVITY_PICK_LOCK,
                                eLockLevel
                            );
                            pPlayer->RewardExperience(iExperiencePoints);
                            ++pLockTarget->GetLock()->iTimesUnlocked;
                            pLockTarget->AddLockChange();
                        }
                        if (pLockTarget->GetOwner()) {
                            TESForm* pForm = pLockTarget->GetOwner();
                            Actor* pVictim = nullptr;
                            if (pForm->GetFormType() == FACT_ID) {
                                TESFaction* pFaction = static_cast<TESFaction*>(pForm);
                                pVictim = ReferenceProcessLists.GetActorRefInHigh(pFaction, true, true);
                            }
                            else {
                                TESNPC* pNPC = static_cast<TESNPC*>(pForm);
                                pVictim = ReferenceProcessLists.GetActorRefInHigh(pNPC, true);
                            }
                            if (pVictim) {
                                pPlayer->bAttackOnNextTheft = true;
                                pPlayer->StealAlarm(pVictim, nullptr, 0, 0, pForm);
                            }
                            else {
                                bool bKarma = false;
                                if (pForm && pForm->GetFormType() == FACT_ID) {
                                    TESFaction* pFaction = static_cast<TESFaction*>(pForm);
                                    if (!pFaction->TESFaction::GetIsEvil()) {
                                        bKarma = true;
                                    }
                                }
                                else if (pForm) {
                                    TESNPC* pNPC = static_cast<TESNPC*>(pForm);
                                    if (pNPC) {
                                        // Slot 3 of the ActorValueOwner subobject: GetActorFloatValue.
                                        float fKarma = static_cast<ActorValueOwner*>(pNPC)->GetActorFloatValue(
                                            ActorValue::KARMA
                                        );
                                        TESActorBaseData::ALIGNMENT eAlign = TESActorBaseData::GetAlignmentForKarma(
                                            fKarma
                                        );
                                        if (eAlign != TESActorBaseData::ALIGNMENT_EVIL &&
                                            eAlign != TESActorBaseData::ALIGNMENT_VERY_EVIL) {
                                            bKarma = true;
                                        }
                                    }
                                }
                                if (bKarma) {
                                    double dKarmaMod = fKarmaModStealing.Float();
                                    pPlayer->RewardKarma(static_cast<int32_t>(dKarmaMod));
                                }
                            }
                        }
                        BSAudio::QInstance().GetSoundHandleByName("UILockpickingUnlock", 257u).Play(false);
                        pLockTarget->UnLock();
                        if (pLockTarget->GetFormType() == DOOR_ID) {
                            pPlayer->btransporting = true;
                        }
                        if (pLockTarget->GetObjectReference() &&
                            pLockTarget->GetObjectReference()->GetFormType() == CONT_ID) {
                            InterfaceManager::SetContainerMenuQueued(true);
                        }

                        pLockTarget->Activate(pPlayer, false, nullptr, 1);
                        if (pLockTarget->GetFormType() == DOOR_ID) {
                            pPlayer->btransporting = false;
                        }
                        LockPickMenu::Close();
                    }
                    else {
                        pTiles[11]->Set(4036, "LOCK OPENED!  :D", true);
                        pTiles[11]->Set(4003, 1);
                        if (!pLockTarget) {
                            pTiles[1]->Set(LockPickMenu::iUT_Value, 0);
                        }
                        bCreateNewSweetSpot = 1;
                        eLockState = LockPickMenu::LS_RESETTING_LOCK;
                    }
                    appMain->GetControls()->KillRumble();
                }
            }
            else {
                appMain->GetControls()->KillRumble();
                eLockState = LockPickMenu::LS_IDLE;
            }
            break;
        }
        case LockPickMenu::LS_DAMAGING_PICK: {
            if (bIsMenuOnTop &&
                (!InterfaceManager::Instance()->Has360Controller() || !(fXAxisRatio <= 0.0)) &&
                (InterfaceManager::Instance()->Has360Controller() || GetPCInput(0))) {
                char strtemp[32];
                NiUpdateData UpdateData(0.0, true, false);
                float fElapsedRatio = static_cast<double>(iElaspedMillis) / 1000.0;
                if (fPickBreakSecs.Float() != 0.0) {
                    fPickHealth = fPickHealth - fElapsedRatio / fPickBreakSecs.Float();
                }
                fPickHealth = BSMAX(fPickHealth, 0.0f);
                float fShakeOffset = static_cast<double>(iLockShakeDirection) * 0.25;
                RotateLock(fShakeOffset);
                if (InterfaceManager::Has360Controller()) {
                    appMain->GetControls()->Rumble(0.5f, 0.0f, 250, 0, CONSTANT, false, false);
                }
                if (pPickBendAnim) {
                    double dBeginKeyTime = pPickBendAnim->GetBeginKeyTime();
                    double dBend = 1.0 - fPickHealth;
                    double dEndKeyTime = pPickBendAnim->m_fEndKeyTime;
                    float fTime = (dEndKeyTime - pPickBendAnim->GetBeginKeyTime()) * dBend + dBeginKeyTime;
                    pPickBendAnim->SetPhase(fTime, false);
                    LockPickMenu::spCameraRoot->Update(UpdateData);
                }
                float fPickX = pTiles[15]->GetFloat(LockPickMenu::iUT_X) +
                    static_cast<double>(iLockShakeDirection) * 2.5;
                RotateLockpickModel(fPickX);
                iLockShakeDirection = -iLockShakeDirection;
                float fHealthPercent = fPickHealth * 100.0;
                BSsprintf(
                    strtemp,
                    32,
                    "PICK HEALTH: %d%s",
                    static_cast<int32_t>(BSRoundToNearest(fHealthPercent, 1.0f)),
                    "%"
                );
                pTiles[12]->Set(4036, strtemp, true);
                if (fPickHealth <= 0.0) {
                    lockpickSound.FadeOutAndRelease(100u);
                    BSAudio::QInstance().GetSoundHandleByName("UILockpickingPickBreak", 257u).Play(false);
                    if (pLockTarget) {
                        pPlayer
                            ->RemoveItem(pLockpick, nullptr, 1, false, false, nullptr, nullptr, nullptr, true, false);
                    }
                    float fPicksRemaining = pTiles[1]->GetFloat(LockPickMenu::iUT_Value) - 1.0;
                    pTiles[1]->Set(LockPickMenu::iUT_Value, fPicksRemaining, true);
                    ppickAnimManager->DeactivateSequence(pPickBendAnim, 0.0f);
                    ppickAnimManager->ActivateSequence(pPickBreakAnim, 0, true, 1.0, 0.0f, nullptr);
                    appMain->GetControls()->KillRumble();
                    eLockState = LockPickMenu::LS_BREAKING_PICK;
                }
            }
            else {
                lockpickSound.FadeOutAndRelease(500u);
                appMain->GetControls()->KillRumble();
                eLockState = LockPickMenu::LS_IDLE;
            }
            break;
        }
        case LockPickMenu::LS_BREAKING_PICK: {
            if (pPickBreakAnim && bIsMenuOnTop) {
                NiUpdateData UpdateData(ftotalStateSecs_1, true, false);
                LockPickMenu::spCameraRoot->GetAt(1u)->Update(UpdateData);
                if (static_cast<double>(ftotalStateSecs_1) >= pPickBreakAnim->m_fEndKeyTime) {
                    eLockState = LockPickMenu::LS_RESETTING_LOCK;
                }
            }

            break;
        }
        case LockPickMenu::LS_RESETTING_LOCK: {
            if (ftotalStateSecs_1 >= 1.0 && bIsMenuOnTop) {
                pTiles[11]->Set(4003, 0);
                pTiles[12]->Set(4036, "PICK HEALTH: 100%", true);
                fPickHealth = fLockpickSkill * fLockpickBonusHealth.Float() + 1.0;
                fLockAngle = 0.0f;
                RotateLock(0.0f);
                float fPickX = pTiles[14]->GetFloat(4017) / 2.0;
                pTiles[15]->Set(LockPickMenu::iUT_X, fPickX, true);
                LockPickMenu::spCameraRoot->GetAt(1u)->SetTranslate(origPickTranslate);
                LockPickMenu::spCameraRoot->GetAt(1u)->SetRotate(origPickRotate);
                ppickAnimManager->DeactivateAll(0.0f);
                ppickAnimManager->ActivateSequence(pPickIntroAnim, 0, true, 1.0f, 0.0f, nullptr);
                if (bCreateNewSweetSpot) {
                    SetupLockLevelSweetSpots();
                    bCreateNewSweetSpot = 0;
                }
                if (pLockTarget && pTiles[1]->GetFloat(LockPickMenu::iUT_Value) == 0.0) {
                    Interface::ShowMessage(
                        sOutOfLockpicks.String(),
                        0,
                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds",
                        nullptr,
                        2.0f,
                        false
                    );
                    pTiles[11]->Set(4003, 0);
                    NiUpdateData UpdateData(0.0f, true, false);
                    LockPickMenu::spCameraRoot->GetAt(1u)->Update(UpdateData);
                    eLockState = LockPickMenu::LS_IDLE;
                }
                else {
                    eLockState = LockPickMenu::LS_PICK_INTRO_ANIM;
                }
            }
            break;
        }
        case LockPickMenu::LS_CLOSING_MENU: {
            if (xFadeState != Interface::FADING_OUT) {
                InterfaceManager::Instance()->pCursor->Set(4009, 255.0f, true);
                InterfaceManager::Instance()->SetLockpickControlsDisable(true);
                pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
                StartFadeOut();
            }
            break;
        }
        default: {
            break;
        }
    }
}

int32_t LockPickMenu::GetClass() {
    return 1014;
}

bool LockPickMenu::IsWellFormedMenu() {
    for (int32_t i = 0; i < 16; ++i) {
        if (!pTiles[i]) {
            return false;
        }
    }
    return true;
}

bool LockPickMenu::GetPCInput(int32_t state) {
    if (!appMain->GetControls()
             ->GetUserAction(Controls::UACTN_SLDLEFT, static_cast<Controls::CONTROL_TOGGLE_FLAGS>(state))) {
        if (!appMain->GetControls()
                 ->GetUserAction(Controls::UACTN_SLDRIGHT, static_cast<Controls::CONTROL_TOGGLE_FLAGS>(state))) {
            if (!appMain->GetControls()
                     ->GetUserAction(Controls::UACTN_FORWARD, static_cast<Controls::CONTROL_TOGGLE_FLAGS>(state))) {
                if (!appMain->GetControls()
                         ->GetUserAction(Controls::UACTN_BACK, static_cast<Controls::CONTROL_TOGGLE_FLAGS>(state))) {
                    return false;
                }
            }
        }
    }
    return true;
}

void LockPickMenu::RotateLock(float afAngleOffset) {
    float fPhase = (fLockAngle + afAngleOffset) / 90.0;
    NiUpdateData kUpdateData(0.0f, true, false);
    if (pLockRotateAnim) {
        double dBeginKeyTime = pLockRotateAnim->GetBeginKeyTime();
        double dEndKeyTime = pLockRotateAnim->GetEndKeyTime();
        float fTime = (dEndKeyTime - pLockRotateAnim->GetBeginKeyTime()) * fPhase + dBeginKeyTime;
        pLockRotateAnim->SetPhase(fTime, false);
        LockPickMenu::spCameraRoot->GetAt(0)->Update(kUpdateData);
    }
}

void LockPickMenu::Prepare3DElements() {
    NiMatrix3 matZ;
    NiMatrix3 matX;
    NiAVObject* pLockGeometry = pModelLoader->LoadFile(
        "Meshes\\Terminals\\LockInterface01.NIF",
        LOD_MULT_NONE,
        true,
        false,
        false,
        false
    );
    NiAVObject* pPickGeometry = pModelLoader->LoadFile(
        "Meshes\\Terminals\\BobbyPin01.NIF",
        LOD_MULT_NONE,
        true,
        false,
        false,
        false
    );
    origPickRotate = NiMatrix3::IDENTITY;
    origPickTranslate = NiPoint3(0.0f, 0.0f, -1.6f);
    pPickGeometry->SetTranslate(origPickTranslate);
    pPickGeometry->SetRotate(origPickRotate);
    LockPickMenu::spCameraRoot = new NiNode(0);
    LockPickMenu::spCameraRoot->AttachChild(pLockGeometry, 1);
    LockPickMenu::spCameraRoot->AttachChild(pPickGeometry, 1);
    matX.MakeXRotation(1.5707964f);
    matZ.MakeZRotation(1.5707964f);
    LockPickMenu::spCameraRoot->SetRotate(matX * matZ);
    LockPickMenu::spCameraRoot->SetTranslate(125.0f, 0.0f, 0.0f);
    NiUpdateData UpdateData1(0.0f, false, false);
    LockPickMenu::spCameraRoot->Update(UpdateData1);
    NiTimeController* pLockAnimController = pLockGeometry->GetController(&NiControllerManager::ms_RTTI);
    if (pLockAnimController) {
        NiControllerManager* pLockAnimManager = static_cast<NiControllerManager*>(pLockAnimController);
        pIntroAnim = pLockAnimManager->GetSequenceByName("Forward");
        pLockRotateAnim = pLockAnimManager->GetSequenceByName("Backward");
        pLockAnimManager->DeactivateAll(0.0f);
        if (pIntroAnim) {
            pLockAnimManager->ActivateSequence(pIntroAnim, 0, true, 1.0f, 0.0f, nullptr);
        }
        if (pLockRotateAnim) {
            pLockRotateAnim->SetCycleType(NiTimeController::CLAMP);
        }
        NiTimeController::StartAnimations(pLockGeometry);
    }
    NiTimeController* pPickAnimController = pPickGeometry->GetController(&NiControllerManager::ms_RTTI);
    if (pPickAnimController) {
        ppickAnimManager = static_cast<NiControllerManager*>(pPickAnimController);
        pPickIntroAnim = ppickAnimManager->GetSequenceByName("Forward");
        pPickBendAnim = ppickAnimManager->GetSequenceByName("Backward");
        pPickBreakAnim = ppickAnimManager->GetSequenceByName("Left");
        ppickAnimManager->DeactivateAll(0.0f);
        if (pPickIntroAnim) {
            ppickAnimManager->ActivateSequence(pPickIntroAnim, 0, true, 1.0f, 0.0f, nullptr);
        }
        if (pPickBreakAnim) {
            pPickBreakAnim->SetCycleType(NiTimeController::CLAMP);
        }
        NiTimeController::StartAnimations(pPickGeometry);
    }
    LockPickMenu::spCamera = Create3DCamera();
    BSShaderManager::GetShadowSceneNode(1)->AttachChild(LockPickMenu::spCameraRoot.m_pObject, 1);
    float fRadius = LockPickMenu::spCameraRoot->GetWorldBound().m_fRadius * 3.0;
    NiNode* pCameraRoot = LockPickMenu::spCameraRoot.m_pObject;
    BSShaderManager::GetShadowSceneNode(1)->AddAllLightsFromNode_Recurse(pCameraRoot, fRadius);
}

NiCamera* LockPickMenu::Create3DCamera() {
    NiRect<float> port;
    NiCamera* pCam = new NiCamera;
    ::new (&port) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
    pCam->SetViewPort(port);
    NiFrustum fr(false);
    fr.m_fFar = Main::GetWorldSceneGraph()->GetFarDistance();
    fr.m_fNear = BSSceneGraph::fNearDistance.Float();
    pCam->SetMaxFarNearRatio(fr.m_fFar / fr.m_fNear);
    int32_t iScreenHeight = BSShaderManager::bLetterbox ? BSShaderManager::iLetterboxHeight
                                                        : Renderer::GetScreenHeight();
    float fHeightByWidth = static_cast<double>(iScreenHeight) / static_cast<double>(Renderer::GetScreenWidth());
    float fAspectDelta = 0.75f;
    float theta = BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369 * 0.15000000596046448;
    fr.m_fLeft = -std::tan(theta) * fAspectDelta;
    fr.m_fRight = std::tan(theta) * fAspectDelta;
    double dTan = std::tan(theta);
    fr.m_fBottom = -dTan * fHeightByWidth * fAspectDelta;
    dTan = std::tan(theta);
    fr.m_fTop = dTan * fHeightByWidth * fAspectDelta;
    fr.m_bOrtho = false;
    pCam->SetViewFrustum(fr);
    NiPoint3 kWorldUp(0.0f, 0.0f, 1.0f);
    NiPoint3 kWorldPt(0.0f, 0.0f, 1.0f);
    pCam->LookAtWorldPoint(kWorldPt, kWorldUp);
    pCam->SetMinNearPlaneDist(1.0f);
    NiUpdateData UpdateData2(0.0f, false, false);
    pCam->Update(UpdateData2);
    return pCam;
}

void LockPickMenu::RotateLockpick() {
    float fXDelta = 0.0f;
    int32_t iXAxis = 0;
    float cfMaxAxisValue = 32767.0f;
    if (InterfaceManager::Has360Controller()) {
        iXAxis = appMain->GetControls()->GetControllerValue(0, 7);
        if (NiAbs(static_cast<float>(iXAxis)) <= 7849.0) {
            iXAxis = 0;
        }
        float fAxisRatio = static_cast<double>(iXAxis) / cfMaxAxisValue;
        fXDelta = hypotf_3(fAxisRatio, 3.0f) * 80.0;
    }
    else {
        double xDelta = static_cast<double>(appMain->GetControls()->GetMouseAxis(1));
        double dScreenWidth = InterfaceManager::GetScreenWidth();
        fXDelta = dScreenWidth / InterfaceManager::GetRealScreenWidth() * xDelta;
    }
    float fNewX = pTiles[15]->GetFloat(LockPickMenu::iUT_X) + fXDelta;
    int32_t iPrevXMod = static_cast<int32_t>(pTiles[15]->GetFloat(LockPickMenu::iUT_X)) % 100;
    int32_t iNewXMod = static_cast<int32_t>(fNewX) % 100;
    fNewX = BSMAX(fNewX, 0.0);
    float fWidth = pTiles[14]->GetFloat(4017);
    fNewX = BSMIN(fNewX, fWidth);
    if (((!(fXDelta > 0.0) || iPrevXMod <= iNewXMod) && (!(fXDelta < 0.0) || iNewXMod <= iPrevXMod)) ||
        BSRandom::GetSingleton()->UnsignedInt(10u)) {
        if (fXDelta != 0.0 && LockPickMenu::PreviousAngleDelta == 0.0) {
            BSAudio::QInstance().GetSoundHandleByName("UILockpickingPickMovement", 257u).Play(false);
        }
    }
    else {
        BSAudio::QInstance().GetSoundHandleByName("UILockpickingPickMovement", 257u).Play(false);
    }
    pTiles[15]->Set(LockPickMenu::iUT_X, fNewX, true);
    LockPickMenu::PreviousAngleDelta = fXDelta;
    RotateLockpickModel(fNewX);
    float fDistFromCenter = NiAbs(fNewX - fSweetSpot);
    if (!(fDistFromCenter < static_cast<double>(iTotalSweetSpotRadius))) {
        double dOuterEdge = static_cast<double>(iTotalSweetSpotRadius) + fDistFromCenter;
        double dPastEdge = dOuterEdge - pTiles[14]->GetFloat(4017);
        double dSpotWidth = static_cast<double>(iTotalSweetSpotRadius) * 2.0;
        float fDistRatio = dPastEdge / (dSpotWidth - pTiles[14]->GetFloat(4017));
        fMaxLockAngle = fDistRatio * 15.0;
        float fRed = pTiles[14]->GetFloat(4018);
        pTiles[15]->Set(4018, fRed, true);
        float fGreen = pTiles[14]->GetFloat(4019);
        pTiles[15]->Set(4019, fGreen, true);
        float fBlue = pTiles[14]->GetFloat(4020);
        pTiles[15]->Set(4020, fBlue, true);
    }
    else {
        NiTListItem<Tile*>* pos = pTiles[14]->xChildren.m_pkHead;
        float fMinHalfLength = pTiles[14]->GetFloat(4017);
        Tile* pClosestChild = nullptr;
        while (pos) {
            Tile* pCurrChild = pTiles[14]->xChildren.GetNext(pos);
            float fCurrHalfLength = pCurrChild->GetFloat(4017) / 2.0;
            if (fMinHalfLength > static_cast<double>(fCurrHalfLength) &&
                fCurrHalfLength > static_cast<double>(fDistFromCenter)) {
                fMinHalfLength = fCurrHalfLength;
                pClosestChild = pCurrChild;
            }
        }
        if (pClosestChild) {
            fMaxLockAngle = (1.0 - (pClosestChild->GetFloat(4010) - 17.0) / 6.0) * 90.0;
            float fRed = pClosestChild->GetFloat(4018);
            pTiles[15]->Set(4018, fRed, true);
            float fGreen = pClosestChild->GetFloat(4019);
            pTiles[15]->Set(4019, fGreen, true);
            float fBlue = pClosestChild->GetFloat(4020);
            pTiles[15]->Set(4020, fBlue, true);
        }
    }
}

void LockPickMenu::RotateLockpickModel(float afX) {
    NiNode* pPick = static_cast<NiNode*>(LockPickMenu::spCameraRoot->GetAt(1u));
    NiUpdateData kUpdateData(0.0, false, false);
    NiPoint3 pt = -origPickTranslate + NiPoint3(0.0f, 0.0f, 0.6f);
    float fAngle = (afX / pTiles[14]->GetFloat(4017) * -180.0 + 90.0) * 0.01745329238474369;
    NiMatrix3 kRotation;
    kRotation.MakeYRotation(fAngle);
    NiPoint3 kRotatedPt = kRotation * pt;
    pPick->SetTranslate(origPickTranslate + pt - kRotatedPt);
    kRotation = kRotation * origPickRotate;
    pPick->SetRotate(kRotation);
    pPick->Update(kUpdateData);
}

void LockPickMenu::SetupLockLevelSweetSpots() {
    int32_t iSkillLevelRange = LockLevelMaxVeryEasy[eLockLevel + 1] - LockLevelMaxVeryEasy[eLockLevel];
    float fLockLevelRatio = static_cast<double>(iLockpickSkill - LockLevelMaxVeryEasy[eLockLevel]) /
        (static_cast<double>(iSkillLevelRange) + static_cast<double>(iSkillLevelRange));
    fLockLevelRatio = BSMIN(fLockLevelRatio, 1.0f);
    fLockLevelRatio = BSMAX(fLockLevelRatio, 0.0f);
    int32_t iX = 0;
    int32_t iDepth = 6;
    NiColorA kStartColor(0.0f, 1.0f, 0.0f, 1.0f);
    NiColorA kEndColor(1.0f, 0.0f, 0.0f, 1.0f);
    NiColorA kColor = kStartColor;
    pTiles[14]->DeleteChildren();
    BSStringT<char> strtemp;
    double dSweetSpotLengthMin = static_cast<double>(g_iSweetSpotLengthMinArray[eLockLevel]->Int());
    int32_t& iSweetSpotLengthMax = g_iSweetSpotLengthMaxArray[eLockLevel]->Int();
    g_iconcentricLength = static_cast<int32_t>(
        static_cast<double>(iSweetSpotLengthMax - g_iSweetSpotLengthMinArray[eLockLevel]->Int()) * fLockLevelRatio +
        dSweetSpotLengthMin
    );
    double dConcentricLengthMin = static_cast<double>(g_iConcentricLengthMinArray[eLockLevel]->Int());
    int32_t& iConcentricLengthMax = g_iConcentricLengthMaxArray[eLockLevel]->Int();
    g_isweetSpotLength = static_cast<int32_t>(
        static_cast<double>(iConcentricLengthMax - g_iConcentricLengthMinArray[eLockLevel]->Int()) * fLockLevelRatio +
        dConcentricLengthMin
    );
    strtemp.SPrintF("%d", g_iconcentricLength);
    pTiles[4]->Set(4036, strtemp.pString, true);
    strtemp.SPrintF("%d", g_isweetSpotLength);
    pTiles[7]->Set(4036, strtemp.pString, true);
    iTotalSweetSpotRadius = g_isweetSpotLength + g_iconcentricLength / 2;
    double dWidth = pTiles[14]->GetFloat(4017);
    fSweetSpot = BSRandom::GetSingleton()->Float0To1() * dWidth;
    float fMaxSpot = pTiles[14]->GetFloat(4017) - static_cast<double>(iTotalSweetSpotRadius);
    float fSpot = BSMAX(fSweetSpot, static_cast<float>(iTotalSweetSpotRadius));
    fSweetSpot = BSMIN(fSpot, fMaxSpot);
    iX = static_cast<int32_t>(fSweetSpot - static_cast<double>(g_iconcentricLength / 2));
    for (int32_t i = 0; i < 6; ++i) {
        Tile* pSpot = pMenu->pMenu->RenderTemplate(pTiles[14], "LPM_SweetSpotTemplate", nullptr);
        pSpot->Set(4010, i + 17);
        double dHalfWidth = fSweetSpot - static_cast<double>(iX);
        float fSpotWidth = dHalfWidth + dHalfWidth;
        pSpot->Set(4017, fSpotWidth, true);
        pSpot->Set(4001, iX);
        pSpot->Set(4013, iDepth);
        float fRed = kColor.r * 255.0;
        pSpot->Set(4018, fRed, true);
        float fGreen = kColor.g * 255.0;
        pSpot->Set(4019, fGreen, true);
        float fBlue = kColor.b * 255.0;
        pSpot->Set(4020, fBlue, true);
        iX -= g_isweetSpotLength / 5;
        --iDepth;
        float fBlend = static_cast<double>(i + 1) / 5.0;
        kColor = kStartColor + (kEndColor - kStartColor) * fBlend;
    }
}

Tile* LockPickMenu::Create(TESObjectREFR* apTarget, int32_t aiLockLevel, bool abDebugMode) {
    int32_t iLockLevel;
    Tile* pOldMenuTile = Menu::GetMenuByClass(1014);
    InterfaceManager* pin = InterfaceManager::Instance();
    if (apTarget) {
        iLockLevel = GetLockPickLevel(apTarget->GetLockLevel());
    }
    else {
        iLockLevel = aiLockLevel;
    }
    iLockLevel = BSMAX(iLockLevel, 0);
    iLockLevel = BSMIN(iLockLevel, 4);
    if (apTarget && pPlayer->GetClampedActorValue(ActorValue::LOCKPICK) < LockLevelMaxVeryEasy[iLockLevel]) {
        char msg[268];
        BSsprintf(msg, 260, sLockpickSkillTooLow.String(), LockLevelMaxVeryEasy[iLockLevel]);
        Interface::ShowMessage(
            msg,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_padlock.dds",
            nullptr,
            2.0f,
            false
        );
        BSAudio::QInstance().GetSoundHandleByName("UIPopUpMessageGeneral", 289u).Play(false);
        return nullptr;
    }

    if (pOldMenuTile) {
        delete pOldMenuTile;
    }
    Tile* ptile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\lockpick_menu.xml");
    LockPickMenu::pMe = static_cast<LockPickMenu*>(ptile->GetMenu());
    if (LockPickMenu::pMe) {
        int32_t iMenuClass = LockPickMenu::pMe->GetClass();
        if (iMenuClass == LockPickMenu::GetClassStatic()) {
            LockPickMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), false);
            if (LockPickMenu::pMe->IsWellFormedMenu()) {
                char strtemp[64];
                if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                    float fDepth = Tile::GetMaximumDepth();
                    ptile->Set(4013, fDepth, true);
                }
                LockPickMenu::pMe->pLockTarget = apTarget;
                LockPickMenu::pMe->eLockLevel = static_cast<LOCK_LEVEL>(iLockLevel);
                int32_t iDebugModeTrait = Tile::TextToTrait("_DebugMode");
                LockPickMenu::pMe->pMenu->Set(iDebugModeTrait, static_cast<int32_t>(abDebugMode));
                BSsprintf(strtemp, 64, "%d", LockPickMenu::pMe->iLockpickSkill);
                LockPickMenu::pMe->pTiles[0]->Set(LockPickMenu::iUT_Value, strtemp, true);
                int32_t iLockLevelSkill = LockLevelMaxVeryEasy[LockPickMenu::pMe->eLockLevel];
                int32_t iPlayerSkill = pPlayer->GetClampedActorValue(ActorValue::LOCKPICK);
                int32_t iForceLockChance = GamePlayFormulas::GetForceLockChance(iPlayerSkill, iLockLevelSkill);
                BSsprintf(strtemp, 64, "%s [%d", sForceLock.String(), iForceLockChance);
                BSstrcat(strtemp, 64, "%]");
                LockPickMenu::pMe->pTiles[9]->Set(4036, strtemp, true);
                const char* pcText = pLockLevelNames[LockPickMenu::pMe->eLockLevel]->String();
                LockPickMenu::pMe->pTiles[2]->Set(LockPickMenu::iUT_Value, pcText, true);
                LockPickMenu::pMe->pTiles[12]->Set(4036, "PICK HEALTH: 100%", true);
                LockPickMenu::pMe->pTiles[1]->Poke(LockPickMenu::iUT_Value, 1.0f);
                if (apTarget) {
                    int32_t iLockpickCount = pPlayer->GetInventoryObjectCount(pLockpick);
                    LockPickMenu::pMe->pTiles[1]->Set(LockPickMenu::iUT_Value, iLockpickCount);
                }
                else {
                    LockPickMenu::pMe->pTiles[1]->Set(LockPickMenu::iUT_Value, 0);
                }
                if (BSstrcmp(sLanguage.String(), "ENGLISH")) {
                    pcText = sLockpickSkillText.String();
                    ptile->GetChildByName("LPM_SkillTitle")->Set(4036, pcText, true);
                    pcText = sPicksRemainingText.String();
                    ptile->GetChildByName("LPM_PickCountTitle")->Set(4036, pcText, true);
                    pcText = sLockLevelText.String();
                    ptile->GetChildByName("LPM_LevelTitle")->Set(4036, pcText, true);
                    pcText = sExit.String();
                    LockPickMenu::pMe->pTiles[10]->Set(4036, pcText, true);
                }
                LockPickMenu::pMe->Prepare3DElements();
                LockPickMenu::pMe->SetupLockLevelSweetSpots();
                LockPickMenu::pMe->PrepForVisibility(false);
                return ptile;
            }

            Warning("MENUS: Lockpicking Menu Creation Failed... Are your menu and art resources up to date?");
            return nullptr;
        }
    }

    if (LockPickMenu::pMe && LockPickMenu::pMe->pMenu && LockPickMenu::pMe) {
        delete LockPickMenu::pMe;
    }

    return nullptr;
}

void LockPickMenu::Close() {
    if (LockPickMenu::pMe) {
        LockPickMenu::pMe->eLockState = LS_CLOSING_MENU;
    }
}

int32_t LockPickMenu::GetClassStatic() {
    return 1014;
}

Interface::TutorialMessageID LockPickMenu::GetTutorialMessageID() {
    return static_cast<Interface::TutorialMessageID>(Interface::Has360Controller() ? 28 : 20);
}

void LockPickMenu::Draw3DElements() {
    if (LockPickMenu::spCameraRoot.m_pObject) {
        NiRenderer::GetRenderer()->ClearBuffer(nullptr, 4u);
        SceneGraph* pSceneGraph = Main::GetWorldSceneGraph();
        BSCullingProcess* pCuller = pSceneGraph->pCuller;
        pCuller->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
        NiCamera* pMenuCamera = LockPickMenu::spCamera.m_pObject;
        NiCamera::HandleEyeFinityFOVScale(pMenuCamera, 0.15f);
        NiPointer<NiRefObject> spAccumulator(
            static_cast<NiRefObject*>(InterfaceManager::Instance()->Get3dAccumulator())
        );
        pCuller->SetAccumulator(static_cast<NiAccumulator*>(spAccumulator.m_pObject));
        NiCamera* pCamera = pMenuCamera ? pMenuCamera : pSceneGraph->GetCamera();
        BSShaderUtil::AccumulateScene(pCamera, BSShaderManager::GetShadowSceneNode(1), *pCuller);
        BSShaderUtil::RenderScene(pCamera, static_cast<BSShaderAccumulator*>(spAccumulator.m_pObject), false);
        pCuller->SetAccumulator(nullptr);
        spAccumulator = static_cast<NiRefObject*>(nullptr);
        pCuller->PopCullMode();
    }
}
