#include "Fallout/Interface/Menus/SurgeryMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSDevices/Controls.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSNoise1D.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileImage.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSDefaultObjectManager.h"
#include "Fallout Shared/BGSTextureSet.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/BSFaceGenNiNode.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/TESClass.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMultiTargetTransformController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/ehvec.h"

#include <limits>

static int32_t iUT_MorphineCount;

// DoClick's function-local statics: the menu tile's user traits for the stimpak and morphine counts, added on the
// first click.
static int32_t iUT_StimpacCount;

// The two traits' guard (bit 0 iUT_StimpacCount, bit 1
// iUT_MorphineCount).
static uint32_t uiDoClickTraitsGuard;

// The game settings this file defines, with their IDs and values, in initialization order.
NiPointer<NiCamera> SurgeryMenu::sp3DCamera;
NiPointer<NiNode> SurgeryMenu::sp3DSkin;
NiPointer<NiAVObject> SurgeryMenu::spPlayer3D;
SettingT<INISettingCollection> fSurgerySkinScale("fSurgerySkinScale:Surgery3D", 0.05f);
SettingT<INISettingCollection> fSurgerySkinZoom("fSurgerySkinZoom:Surgery3D", -75.0f);
SettingT<INISettingCollection> fSurgerySkinHPos("fSurgerySkinHPos:Surgery3D", -4.0f);
SettingT<INISettingCollection> fSurgerySkinVPos("fSurgerySkinVPos:Surgery3D", 0.0f);
SettingT<INISettingCollection> fSurgeryPlayerZoom("fSurgeryPlayerZoom:Surgery3D", 30.0f);
SettingT<INISettingCollection> fSurgeryPlayerDefaultHPos("fSurgeryPlayerDefaultHPos:Surgery3D", 2.75f);
SettingT<INISettingCollection> fSurgeryPlayerDefaultVPos("fSurgeryPlayerDefaultVPos:Surgery3D", -9.9f);
SettingT<INISettingCollection> fSurgeryPlayerDefaultScale("fSurgeryPlayerDefaultScale:Surgery3D", 0.09f);
BSSimpleList<SurgeryMenu::LimbWound*> SurgeryMenu::xLimbWounds;

// Moves and scales the surgery menu's 3D player: the Bip01 bone goes to (-skin zoom, VPos - afY, HPos - afX), so the
// horizontal offset is the third argument, the player is scaled by Scale * afScale, and the scene graph is updated at
// time 0.
void SetTerminalMenuPos(float afX, float afY, float afScale) {
    if (!SurgeryMenu::spPlayer3D) {
        return;
    }
    NiAVObject* pPlayer = SurgeryMenu::spPlayer3D;
    const float fPosZ = static_cast<float>(fSurgeryPlayerDefaultHPos.Float() - static_cast<double>(afX));
    const float fPosY = static_cast<float>(fSurgeryPlayerDefaultVPos.Float() - static_cast<double>(afY));
    const float fPosX = -fSurgerySkinZoom.Float();
    NiFixedString kName("Bip01");
    pPlayer->GetObjectByName(kName)->SetTranslate(fPosX, fPosY, fPosZ);
    NiAVObject* pPlayer2 = SurgeryMenu::spPlayer3D;
    const float fScale = static_cast<float>(fSurgeryPlayerDefaultScale.Float() * static_cast<double>(afScale));
    NiFixedString kScaleName("Bip01");
    pPlayer2->GetObjectByName(kScaleName)->SetScale(fScale);
    NiUpdateData kUpdateData(0.0f, false, false);
    SurgeryMenu::spPlayer3D->Update(kUpdateData);
}

void SurgeryMenu::LimbWound::ResetLimb() {
    bResetLimb = true;
    for (BSSimpleList<SurgeryMenu::SequenceInfo*>* psequenceList = xSequenceInfo;
         psequenceList && !psequenceList->IsEmpty();
         psequenceList = psequenceList->m_pkNext) {
        psequenceList->m_item->xBoneList->RemoveAll();
    }
    xSequenceInfo->RemoveAll();
    int32_t iCondition = pPlayer->GetActorValue(eLimb);
    eDamage = SurgeryMenu::pSurgeryMenu->GetLimbDamage(iCondition);
}

// The members' NiPointers and the Menu base are released by C++.
SurgeryMenu::~SurgeryMenu() {
    SurgeryMenu::sp3DCamera = static_cast<NiCamera*>(nullptr);
    SurgeryMenu::sp3DSkin = static_cast<NiNode*>(nullptr);
    SurgeryMenu::spPlayer3D = static_cast<NiAVObject*>(nullptr);
}

// The stimpak and morphine buttons use one item and play their animation; auto-attempt heals the limb by a random share
// and costs health; the switch buttons cycle the current limb through the six limb conditions (from none, -1, as from
// perception). Each stored float is a float local; the arithmetic in between is double.
void SurgeryMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (!(uiDoClickTraitsGuard & 1)) {
        uiDoClickTraitsGuard |= 1;
        iUT_StimpacCount = Tile::AddUserTrait("_StimpacCount", -1);
    }
    if (!(uiDoClickTraitsGuard & 2)) {
        uiDoClickTraitsGuard |= 2;
        iUT_MorphineCount = Tile::AddUserTrait("_MorphineCount", -1);
    }
    switch (GetIndex(aiID)) {
        case SM_CLOSE_BUTTON: Close(); break;
        case SM_STIMPAK:
            if (pSurgeryMenu->pStimPaks && pSurgeryMenu->pStimPaks->GetCount() > 0) {
                float fCount = static_cast<float>(GetMenuTile()->GetFloat(iUT_StimpacCount) - 1.0);
                GetMenuTile()->Set(iUT_StimpacCount, fCount, true);
                ExtraDataList* pExtra = nullptr;
                if (pSurgeryMenu->pStimPaks->GetExtraDataList()) {
                    pExtra = pSurgeryMenu->pStimPaks->GetExtraDataList()->GetItem();
                }
                int32_t iCount = pSurgeryMenu->pStimPaks->GetCount();
                pPlayer->QueueEquipObject(
                    pSurgeryMenu->pStimPaks->GetItemChangeObject(),
                    iCount,
                    pExtra,
                    false,
                    false,
                    true
                );
            }
            SetAnimation(SMA_STIMPAK);
            break;
        case SM_MORPHINE:
            // Only a count above zero (not NaN) uses one.
            if (GetMenuTile()->GetFloat(iUT_MorphineCount) > 0.0) {
                ItemChange* pItem = pPlayer->GetInventoryItem(
                    static_cast<TESBoundObject*>(BGSDefaultObjectManager::GetDefaultObject(DO_MORPHINE)),
                    0
                );
                float fCount = static_cast<float>(GetMenuTile()->GetFloat(iUT_MorphineCount) - 1.0);
                GetMenuTile()->Set(iUT_MorphineCount, fCount, true);
                ExtraDataList* pExtra = nullptr;
                if (pItem->GetExtraDataList()) {
                    pExtra = pItem->GetExtraDataList()->GetItem();
                }
                int32_t iCount = pItem->GetCount();
                pPlayer->QueueEquipObject(
                    static_cast<TESBoundObject*>(BGSDefaultObjectManager::GetDefaultObject(DO_MORPHINE)),
                    iCount,
                    pExtra,
                    false,
                    false,
                    true
                );
                if (pItem) {
                    delete pItem;
                }
                fPainPercent = static_cast<float>(fPainPercent - 25.0);
                // Below 25 or NaN clears the injury percent.
                if (!(GetMenuTile()->GetFloat(iUT_InjuryPercent) >= 25.0)) {
                    GetMenuTile()->Set(iUT_InjuryPercent, 0);
                }
                else {
                    float fInjury = static_cast<float>(GetMenuTile()->GetFloat(iUT_InjuryPercent) - 25.0);
                    GetMenuTile()->Set(iUT_InjuryPercent, fInjury, true);
                }
            }
            SetAnimation(SMA_MORPHINE);
            break;
        case SM_SWITCH_LEFT_BUTTON: {
            ActorValue::Index eOldLimb = eCurrentDamagedPart;
            switch (static_cast<int32_t>(eCurrentDamagedPart)) {
                case -1:
                case ActorValue::PERCEPTION_CONDITION: eCurrentDamagedPart = ActorValue::ENDURANCE_CONDITION; break;
                case ActorValue::ENDURANCE_CONDITION: eCurrentDamagedPart = ActorValue::LEFT_ATTACK_CONDITION; break;
                case ActorValue::LEFT_ATTACK_CONDITION: eCurrentDamagedPart = ActorValue::RIGHT_ATTACK_CONDITION; break;
                case ActorValue::RIGHT_ATTACK_CONDITION:
                    eCurrentDamagedPart = ActorValue::LEFT_MOBILITY_CONDITION;
                    break;
                case ActorValue::LEFT_MOBILITY_CONDITION:
                    eCurrentDamagedPart = ActorValue::RIGHT_MOBILITY_CONDITION;
                    break;
                case ActorValue::RIGHT_MOBILITY_CONDITION:
                    eCurrentDamagedPart = ActorValue::PERCEPTION_CONDITION;
                    break;
                default: break;
            }
            ActorValue::Index eLimb = eCurrentDamagedPart;
            fPainPercent = GamePlayFormulas::GetPainCausedByWound(
                static_cast<ActorValueOwner&>(*pPlayer).GetActorFloatValue(eLimb)
            );
            float fCondition = static_cast<float>(
                static_cast<ActorValueOwner&>(*pPlayer).GetActorFloatValue(eLimb) / 100.0
            );
            pTiles[SM_LIMB_METER]->Set(Tile::TextToTrait("_Value"), fCondition, true);
            pTiles[SM_LIMB_DAMAGE]
                ->Set(Tile::kTileValue_String, ActorValue::GetActorValueName(eCurrentDamagedPart), true);
            SwitchSkinGeometry(eOldLimb, eCurrentDamagedPart);
            int32_t iIndex = GetIndexFromActorValue(eCurrentDamagedPart);
            PrepareSequences(NiSmartPointerCast<NiAVObject>(pSurgeryMenu->pSkinGeometry[iIndex]));
            break;
        }
        case SM_SWITCH_RIGHT_BUTTON: {
            ActorValue::Index eOldLimb = eCurrentDamagedPart;
            switch (static_cast<int32_t>(eCurrentDamagedPart)) {
                case -1:
                case ActorValue::PERCEPTION_CONDITION:
                    eCurrentDamagedPart = ActorValue::RIGHT_MOBILITY_CONDITION;
                    break;
                case ActorValue::ENDURANCE_CONDITION: eCurrentDamagedPart = ActorValue::PERCEPTION_CONDITION; break;
                case ActorValue::LEFT_ATTACK_CONDITION: eCurrentDamagedPart = ActorValue::ENDURANCE_CONDITION; break;
                case ActorValue::RIGHT_ATTACK_CONDITION: eCurrentDamagedPart = ActorValue::LEFT_ATTACK_CONDITION; break;
                case ActorValue::LEFT_MOBILITY_CONDITION:
                    eCurrentDamagedPart = ActorValue::RIGHT_ATTACK_CONDITION;
                    break;
                case ActorValue::RIGHT_MOBILITY_CONDITION:
                    eCurrentDamagedPart = ActorValue::LEFT_MOBILITY_CONDITION;
                    break;
                default: break;
            }
            ActorValue::Index eLimb = eCurrentDamagedPart;
            fPainPercent = GamePlayFormulas::GetPainCausedByWound(
                static_cast<ActorValueOwner&>(*pPlayer).GetActorFloatValue(eLimb)
            );
            float fCondition = static_cast<float>(
                static_cast<ActorValueOwner&>(*pPlayer).GetActorFloatValue(eLimb) / 100.0
            );
            pTiles[SM_LIMB_METER]->Set(Tile::TextToTrait("_Value"), fCondition, true);
            pTiles[SM_LIMB_DAMAGE]
                ->Set(Tile::kTileValue_String, ActorValue::GetActorValueName(eCurrentDamagedPart), true);
            SwitchSkinGeometry(eOldLimb, eCurrentDamagedPart);
            int32_t iIndex = GetIndexFromActorValue(eCurrentDamagedPart);
            PrepareSequences(NiSmartPointerCast<NiAVObject>(pSurgeryMenu->pSkinGeometry[iIndex]));
            break;
        }
        case SM_AUTOATTEMPT_BUTTON: {
            // The morphine's MagicItem base (+0x30), or null without a default morphine.
            AlchemyItem* pMorphine = static_cast<AlchemyItem*>(BGSDefaultObjectManager::GetDefaultObject(DO_MORPHINE));
            // Reference casts, not pointer casts, so pPlayer is converted to its base subobjects without a null test.
            bool bMorphineUsed = static_cast<MagicTarget&>(*pPlayer).IsSpellTarget(
                static_cast<MagicItem*>(pMorphine),
                true
            );
            ActorValue::Index eLimb = eCurrentDamagedPart;
            float fWoundPercent = static_cast<ActorValueOwner&>(*pPlayer).GetActorFloatValue(eLimb);
            int32_t iMedicine = static_cast<ActorValueOwner&>(*pPlayer).GetActorValue(ActorValue::MEDICINE);
            float fLimbHealPercent;
            float fHealthDamagePercent;
            GamePlayFormulas::GetSurgeryAutoAttemptPercentages(
                fLimbHealPercent,
                fHealthDamagePercent,
                iMedicine,
                bMorphineUsed,
                fWoundPercent
            );
            float fHealed = static_cast<float>(GetLimbPercentFromSequences() * 100.0 + fLimbHealPercent);
            float fClamped = BSMIN(fHealed, 100.0f);
            SetLimbSequences(static_cast<float>(1.0 - fClamped / 100.0));
            int32_t iCondition = ShowLimbCondition();
            int32_t iRestore = iCondition - static_cast<ActorValueOwner&>(*pPlayer).GetActorValue(eLimb);
            pPlayer->RestoreActorValue(eCurrentDamagedPart, static_cast<float>(iRestore));
            float fHealthDamage = -fHealthDamagePercent;
            pPlayer->DamageModActorValue(ActorValue::HEALTH, fHealthDamage, nullptr);
            pSurgeryMenu->GetCurrentLimbWound()->bResetLimb = true;
            break;
        }
        default: break;
    }
}

// The tool follows the mouse (or the gamepad's tool tile), burns while the use button is held, and the pain
// decays towards the wound's floor otherwise.
void SurgeryMenu::DoIdle() {
    if (!Interface::IsTopMenuID(Interface::eSurgeryMenu)) {
        GetMenuTile()->Set(Tile::TextToTrait("_IsTopMenu"), 0);
        return;
    }
    GetMenuTile()->Set(Tile::TextToTrait("_IsTopMenu"), 1);
    UpdateAnimations();
    bool bResting = false;
    InterfaceManager::Instance();
    if (InterfaceManager::Has360Controller()) {
        if (InterfaceManager::CheckMenuButton(Interface::eButtonA, true) == 2) {
            ToolLocation.x = pTiles[6]->GetFloat(Tile::kTileValue_X);
            ToolLocation.y = pTiles[6]->GetFloat(Tile::kTileValue_Y);
        }
        if (!InterfaceManager::CheckMenuButton(Interface::eButtonA, true)) {
            UpdateToolLocation(true);
            BurnFlesh();
        }
        else {
            bResting = true;
        }
    }
    else {
        Controls* pControls = appMain->GetControls();
        if (pControls->GetUserAction(Controls::UACTN_USE, Controls::isPressed)) {
            InterfaceManager* pManager = InterfaceManager::Instance();
            ToolLocation.x = static_cast<float>(pManager->GetCursorX());
            ToolLocation.y = static_cast<float>(pManager->GetCursorY());
        }
        bool bInFlesh = IsCursorInFlesh();
        if (bInFlesh) {
            InterfaceManager::Instance()->GetCursorTile()->Set(Tile::kTileValue_Alpha, 0);
        }
        else {
            InterfaceManager::Instance()->GetCursorTile()->Set(Tile::kTileValue_Alpha, 255);
        }
        bool bBurn = false;
        if ((pControls->GetUserAction(Controls::UACTN_USE, Controls::isHeld) && bToolBuriedInFlesh) ||
            (pControls->GetUserAction(Controls::UACTN_USE, Controls::isPressed) && bInFlesh)) {
            bBurn = true;
            bToolBuriedInFlesh = true;
        }
        else {
            bBurn = false;
            bToolBuriedInFlesh = false;
        }
        if (bBurn) {
            UpdateToolLocation(true);
            BurnFlesh();
        }
        else {
            bResting = true;
        }
    }
    if (bResting) {
        float fChange = GamePlayFormulas::PainChangePerSecond(1.0f);
        uint64_t ulElapsed = static_cast<uint64_t>(appTimer.GetTime() - uiPainTimer);
        fPainPercent = static_cast<float>(fPainPercent - static_cast<double>(ulElapsed) / 1000.0 * fChange);
        float fWound = static_cast<float>(GetMenuTile()->GetFloat(iUT_InjuryPercent) / 100.0);
        // The floor is applied only when fPainPercent is below it (not on NaN).
        if (fPainPercent < GamePlayFormulas::GetPainCausedByWound(fWound) * 100.0) {
            fWound = static_cast<float>(GetMenuTile()->GetFloat(iUT_InjuryPercent) / 100.0);
            fPainPercent = static_cast<float>(GamePlayFormulas::GetPainCausedByWound(fWound) * 100.0);
        }
        uiPainTimer = appTimer.GetTime();
        uiWoundCloseTime = uiPainTimer;
        UpdateToolLocation(false);
        if (BurningFleshSoundHand.IsPlaying()) {
            BurningFleshSoundHand.Stop();
        }
        if (BurningWoundSoundHand.IsPlaying()) {
            BurningWoundSoundHand.Stop();
        }
    }
    UpdateFace();
    float fPain = static_cast<float>(fPainPercent / 100.0);
    pTiles[10]->Set(Tile::kTileValue_User0, fPain, true);
    char acBuffer[256];
    ActorValueOwner* pOwner = pPlayer;
    // The base value (the last argument) is evaluated first.
    int32_t iBase = pOwner->GetPermanentActorValue(ActorValue::HEALTH);
    pOwner = pPlayer;
    int32_t iCurrent = pOwner->GetActorValue(ActorValue::HEALTH);
    BSsprintf(acBuffer, 0x100u, "%i/%i", iCurrent, iBase);
    pTiles[7]->Set(Tile::TextToTrait("_Value"), acBuffer, true);
}

bool SurgeryMenu::DoKey(int32_t aiKey) {
    if (aiKey == 87 || aiKey == 119) {
        DoClick(6, nullptr);
        return true;
    }

    if (aiKey == 83 || aiKey == 115) {
        DoClick(5, nullptr);
        return true;
    }

    return false;
}

int32_t SurgeryMenu::GetClass() {
    return 1054;
}

SurgeryMenu::LimbWound* SurgeryMenu::GetCurrentLimbWound() {
    for (BSSimpleList<LimbWound*>* pIter = &xLimbWounds; pIter;) {
        bool bEmpty = pIter->IsEmpty();
        if (bEmpty) {
            break;
        }
        LimbWound* pWound = pIter->GetItem();
        if (pWound->eLimb == pSurgeryMenu->eCurrentDamagedPart) {
            return pIter->GetItem();
        }
        pIter = pIter->GetNext();
    }
    return nullptr;
}

int32_t SurgeryMenu::GetIndexFromActorValue(ActorValue::Index aeLimb) {
    int32_t iIndex;
    switch (static_cast<int32_t>(aeLimb)) {
        case -1: {
            iIndex = 6;
            break;
        }
        case 25: {
            iIndex = 5;
            break;
        }
        case 26: {
            iIndex = 0;
            break;
        }
        case 27: {
            iIndex = 1;
            break;
        }
        case 28: {
            iIndex = 3;
            break;
        }
        case 29: {
            iIndex = 2;
            break;
        }
        case 30: {
            iIndex = 4;
            break;
        }
        default: {
            iIndex = 0;
            break;
        }
    }
    return iIndex;
}

SurgeryMenu::SURGERY_MENU_WOUND_DAMAGE SurgeryMenu::GetLimbDamage(int32_t aiLimbCondition) {
    if (aiLimbCondition >= 100) {
        return SurgeryMenu::SMWD_NONE;
    }
    if (aiLimbCondition >= 75) {
        return SurgeryMenu::SMWD_LIGHT;
    }
    if (aiLimbCondition >= 50) {
        return SurgeryMenu::SMWD_MODERATE;
    }
    if (aiLimbCondition < 25) {
        return SurgeryMenu::SMWD_SEVERE;
    }
    return SurgeryMenu::SMWD_HEAVY;
}

void SurgeryMenu::UpdateAnimations() {
    NiTimeController* psurgerygeomController = pFrameGeometry->GetControllers();
    if (psurgerygeomController) {
        NiControllerManager* ppsurgerygeomControllerManager = static_cast<NiControllerManager*>(psurgerygeomController);
        NiControllerSequence* pseq = nullptr;
        switch (this->eAnimation) {
            case SurgeryMenu::SMA_GAME_START: {
                NiFixedString kName("Forward");
                pseq = ppsurgerygeomControllerManager->GetSequenceByName(kName);
                break;
            }
            case SurgeryMenu::SMA_MORPHINE: {
                NiFixedString kName("Right");
                pseq = ppsurgerygeomControllerManager->GetSequenceByName(kName);
                break;
            }
            case SurgeryMenu::SMA_STIMPAK: {
                NiFixedString kName("Left");
                pseq = ppsurgerygeomControllerManager->GetSequenceByName(kName);
                break;
            }
        }
        fFrameAnimTimer = appTimer.fDelta + fFrameAnimTimer;
        if (pseq) {
            pseq->Update(fFrameAnimTimer, true);
            while (psurgerygeomController) {
                if (NiObject::DynamicCastStatic(&NiMultiTargetTransformController::ms_RTTI, psurgerygeomController)) {
                    NiUpdateData UpdateData(fFrameAnimTimer, false, false);
                    psurgerygeomController->Update(UpdateData);
                    break;
                }
                psurgerygeomController = psurgerygeomController->GetNext();
            }
        }
    }
    NiUpdateData update(0.0f, false, false);
    pFrameGeometry->Update(update);
}

void SurgeryMenu::UpdateToolLocation(bool abBurning) {
    NiPoint2 kScaledDelta;
    NiPoint2 delta;
    InterfaceManager* pim = InterfaceManager::Instance();
    static int32_t g_surgeryMenuTrait_MinResistanceTime = Tile::AddUserTrait("_MinResistanceTime", -1);
    static int32_t g_surgeryMenuTrait_MaxResistanceTime = Tile::AddUserTrait("_MaxResistanceTime", -1);
    static int32_t g_surgeryMenuTrait_MinMagnitude = Tile::AddUserTrait("_MinMagnitude", -1);
    static int32_t g_surgeryMenuTrait_MaxMagnitude = Tile::AddUserTrait("_MaxMagnitude", -1);
    static int32_t g_surgeryMenuTrait_MinXBoxMag = Tile::AddUserTrait("_MinXBoxMag", -1);
    static int32_t g_surgeryMenuTrait_MaxXBoxMag = Tile::AddUserTrait("_MaxXBoxMag", -1);
    static int32_t g_surgeryMenuTrait_MouseMinPercent = Tile::AddUserTrait("_MouseMinPercent", -1);
    static int32_t g_surgeryMenuTrait_MouseMaxPercent = Tile::AddUserTrait("_MouseMaxPercent", -1);
    float fMinResistance = pMenu->GetFloat(g_surgeryMenuTrait_MinResistanceTime);
    float fMaxResistance = pMenu->GetFloat(g_surgeryMenuTrait_MaxResistanceTime);
    float fMinMagnitude = pMenu->GetFloat(g_surgeryMenuTrait_MinMagnitude);
    float fMaxMagnitude = pMenu->GetFloat(g_surgeryMenuTrait_MaxMagnitude);
    pMenu->GetFloat(g_surgeryMenuTrait_MinXBoxMag);
    pMenu->GetFloat(g_surgeryMenuTrait_MaxXBoxMag);
    float fMouseMinPercent = pMenu->GetFloat(g_surgeryMenuTrait_MouseMinPercent) / 100.0;
    float fMouseMaxPercent = pMenu->GetFloat(g_surgeryMenuTrait_MouseMaxPercent) / 100.0;
    if (abBurning) {
        bBurning = true;
        pTiles[12]->Set(4003, 1);
        int32_t imedicine = pPlayer->GetActorValue(ActorValue::MEDICINE);
        float fPainRatio = fPainPercent / 100.0;
        float fInjuryRatio = pMenu->GetFloat(iUT_InjuryPercent) / 100.0;
        float fresistanceTime;
        float fmagnitude;
        float fmouseSpeedScalar;
        GamePlayFormulas::CalculateToolResistance(
            fresistanceTime,
            fmagnitude,
            fmouseSpeedScalar,
            fInjuryRatio,
            fPainRatio,
            imedicine
        );
        fresistanceTime = FLerp(fMinResistance, fMaxResistance, 0.0, 1.0f, fresistanceTime);
        float fNoiseTimeX = static_cast<double>(appTimer.iLastTime) * 0.0002500000118743628;
        float fnoise = BSNoise1D::Noise(fNoiseTimeX);
        float fNoiseTimeY = static_cast<double>(appTimer.iLastTime + 12345) * 0.0002500000118743628;
        float fnoise2 = BSNoise1D::Noise(fNoiseTimeY);
        fmagnitude = FLerp(fMinMagnitude, fMaxMagnitude, 0.0, 1.0f, fmagnitude);
        float fMaxNoiseX = fnoise >= 0.0 ? fMaxMagnitude : -fMaxMagnitude;
        double dNoiseX;
        if (fnoise >= 0.0) {
            dNoiseX = FLerp(fMinMagnitude, fMaxNoiseX, -0.5f, 0.5f, fnoise);
        }
        else {
            dNoiseX = FLerp(-fMinMagnitude, fMaxNoiseX, -0.5f, 0.5f, fnoise);
        }
        fnoise = dNoiseX * fmagnitude;
        float fMaxNoiseY = fnoise2 >= 0.0 ? fMaxMagnitude : -fMaxMagnitude;
        double dNoiseY;
        if (fnoise2 >= 0.0) {
            dNoiseY = FLerp(fMinMagnitude, fMaxNoiseY, -0.5f, 0.5f, fnoise2);
        }
        else {
            dNoiseY = FLerp(-fMinMagnitude, fMaxNoiseY, -0.5f, 0.5f, fnoise2);
        }
        fnoise2 = dNoiseY * fmagnitude;
        fmouseSpeedScalar = FLerp(fMouseMinPercent, fMouseMaxPercent, 0.0f, 1.0f, fmouseSpeedScalar);
        Resistance.x = fnoise;
        Resistance.y = fnoise2;
        ToolLocation += Resistance;
        Controls* pcontrols = appMain->GetControls();
        InterfaceManager::Instance();
        if (InterfaceManager::Has360Controller()) {
            int32_t iStickX = pcontrols->GetControllerValue(0, 7);
            int32_t iDeltaX = pcontrols->GetControllerValue(0, 8);
            delta.x = static_cast<float>(
                static_cast<int32_t>(fXenonMenuMouseXYMult.Float() * static_cast<double>(iStickX))
            );
            delta.y = static_cast<float>(
                static_cast<int32_t>(fXenonMenuMouseXYMult.Float() * static_cast<double>(-iDeltaX))
            );
            float fStickScale = FLerp(0.25f, 0.5f, 0.0f, 1.0f, fmouseSpeedScalar);
            ToolLocation += delta * fStickScale;
        }
        else {
            delta.x = static_cast<float>(pcontrols->GetMouseAxis(1));
            delta.y = static_cast<float>(pcontrols->GetMouseAxis(2));
            ToolLocation += *NiPoint2::Scale_(&kScaledDelta, fmouseSpeedScalar, &delta);
        }
        // The product is kept as a double across the Length call.
        double dSpeed = fmagnitude * fmouseSpeedScalar;
        fToolMovement = delta.Length() * dSpeed;
        if (fToolMovement > 10.0) {
            fToolMovement = 10.0f;
        }
        fToolMovement = fToolMovement / 5.0;
        PlaceToolInWindow(static_cast<int32_t>(ToolLocation.x), static_cast<int32_t>(ToolLocation.y));
        float fscreenRatio = InterfaceManager::GetScreenAspect();
        double dRealY = pTiles[6]->GetRealY();
        float fCursorY = dRealY / static_cast<double>(Renderer::GetScreenHeight()) * fscreenRatio;
        double dRealX = pTiles[6]->GetRealX();
        float fCursorX = dRealX / static_cast<double>(Renderer::GetScreenWidth()) * fscreenRatio;
        pim->SetCursorPos(fCursorX, fCursorY);
    }
    else {
        bBurning = false;
        fToolMovement = 0.0f;
        pTiles[12]->Set(4003, 0);
        InterfaceManager::Instance();
        if (InterfaceManager::Has360Controller()) {
            Controls* pcontrols = appMain->GetControls();
            int32_t iStickX = pcontrols->GetControllerValue(0, 7);
            int32_t iStickY = pcontrols->GetControllerValue(0, 8);
            iStickX = static_cast<int32_t>(fXenonMenuMouseXYMult.Float() * static_cast<double>(iStickX));
            iStickY = static_cast<int32_t>(fXenonMenuMouseXYMult.Float() * static_cast<double>(-iStickY));
            int32_t iToolY = iStickY + static_cast<int32_t>(pTiles[6]->GetFloat(4002));
            int32_t iToolX = static_cast<int32_t>(pTiles[6]->GetFloat(4001));
            PlaceToolInWindow(iStickX + iToolX, iToolY);
        }
        else {
            int32_t iCursorY = pim->GetCursorY();
            int32_t iCursorX = pim->GetCursorX();
            PlaceToolInWindow(iCursorX, iCursorY);
        }
    }
}

void SurgeryMenu::BurnFlesh() {
    char acBuffer[256];
    UpdateFlesh();
    float foldPain = fPainPercent;
    fPainPercent = static_cast<double>(appTimer.iLastTime - uiPainTimer) / 1000.0 * 30.0 + fPainPercent;
    if (fPainPercent > 100.0) {
        fPainPercent = 100.0f;
    }
    int32_t iPlay = RandomInt(0, 5);
    if (iPlay == 1) {
        if (foldPain < 60.0 && fPainPercent > 60.0) {
            BSSoundHandle soundHand;
            soundHand = BSAudio::QInstance().GetSoundHandleByName("UISurgeryPain01", 289u);
            soundHand.Play(false);
        }
        else if (foldPain < 85.0 && fPainPercent > 85.0) {
            BSSoundHandle soundHand;
            soundHand = BSAudio::QInstance().GetSoundHandleByName("UISurgeryPain02", 289u);
            soundHand.Play(false);
        }
        else if (foldPain < 100.0 && fPainPercent == 100.0) {
            BSSoundHandle soundHand;
            soundHand = BSAudio::QInstance().GetSoundHandleByName("UISurgeryPain03", 289u);
            soundHand.Play(false);
        }
    }
    uiPainTimer = appTimer.iLastTime;
    int32_t iCondition = ShowLimbCondition();
    float fAmount = static_cast<float>(iCondition - pPlayer->GetActorValue(eCurrentDamagedPart));
    pPlayer->RestoreActorValue(eCurrentDamagedPart, fAmount);
    int64_t iElapsed = appTimer.iLastTime - uiHitPointTimer;
    float fmedicine = static_cast<float>(pPlayer->GetActorValue(ActorValue::MEDICINE));
    float fMinInterval = FLerp(100.0f, 500.0f, 0.0f, 100.0f, fmedicine);
    if (FLerp(fMinInterval, 1000.0f, 100.0f, 0.0f, fPainPercent) < static_cast<double>(iElapsed)) {
        pPlayer->DamageModActorValue(ActorValue::HEALTH, -1, nullptr);
        uiHitPointTimer = appTimer.iLastTime;
    }
    BSsprintf(acBuffer, 256, "%i%%", pPlayer->GetActorValue(ActorValue::MEDICINE));
    int32_t iValueTrait = Tile::TextToTrait("_Value");
    pTiles[9]->Set(iValueTrait, acBuffer, true);
}

bool SurgeryMenu::IsCursorInFlesh() {
    int32_t icenterX;
    int32_t icenterY;
    int32_t iRadius;
    NiPoint2 kToCursor;
    InterfaceManager::Instance();
    if (InterfaceManager::Has360Controller()) {
        return true;
    }
    NiAVObject* pcenter = pFrameGeometry->GetObjectByName("CenterNode");
    NiAVObject* pouterEdge = pFrameGeometry->GetObjectByName("FrameNode");
    if (pcenter && pouterEdge) {
        NiPoint2 outerEdge;
        NiPoint2 center;
        SurgeryMenu::sp3DCamera->WorldPtToScreenPt(pcenter->GetWorldTranslate(), center.x, center.y, 1e-05f);
        SurgeryMenu::sp3DCamera->WorldPtToScreenPt(pouterEdge->GetWorldTranslate(), outerEdge.x, outerEdge.y, 1e-05f);
        center.x = InterfaceManager::GetScreenWidth() * center.x;
        center.y = InterfaceManager::GetScreenHeight() * (1.0 - center.y);
        outerEdge.x = InterfaceManager::GetScreenWidth() * outerEdge.x;
        outerEdge.y = InterfaceManager::GetScreenHeight() * (1.0 - outerEdge.y);
        icenterX = static_cast<int32_t>(center.x);
        icenterY = static_cast<int32_t>(center.y);
        iRadius = static_cast<int32_t>((center - outerEdge).Length());
    }
    else {
        icenterX = 800;
        icenterY = 475;
        iRadius = 325;
    }
    InterfaceManager* pim = InterfaceManager::Instance();
    int32_t iCursorX = pim->GetCursorX();
    kToCursor.x = static_cast<float>(iCursorX - icenterX);
    int32_t iCursorY = pim->GetCursorY();
    kToCursor.y = static_cast<float>(iCursorY - icenterY);
    return static_cast<double>(iRadius) >= kToCursor.Length();
}

void SurgeryMenu::UpdateFace() {
    if (SurgeryMenu::spPlayer3D.m_pObject) {
        BSFaceGenAnimationData* pAnimData = static_cast<BSFaceGenNiNode*>(
                                                SurgeryMenu::spPlayer3D->GetObjectByName("BSFaceGenNiNodeSkinned")
        )
                                                ->GetAnimationData();
        float fPained = FLerp(0.0f, 1.0f, 0.0f, 100.0f, fPainPercent);
        pAnimData->SetExpressionTarget(BS_FG_EXPRESSION_MOOD_PAINED, fPained);
        float fTime = static_cast<double>(appTimer.iLastTime) / 1000.0;
        NiUpdateData update(fTime, true, false);
        SurgeryMenu::spPlayer3D->Update(update);
    }
}

void SurgeryMenu::SetAnimation(SurgeryMenu::SURGERY_MENU_ANIMATION aeAnimation) {
    this->eAnimation = aeAnimation;
    fFrameAnimTimer = 0.0f;
    NiTimeController* psurgerygeomController = SurgeryMenu::pSurgeryMenu->pFrameGeometry->GetController(
        &NiControllerManager::ms_RTTI
    );
    if (psurgerygeomController) {
        NiControllerManager* ppsurgerygeomControllerManager = static_cast<NiControllerManager*>(psurgerygeomController);
        NiControllerSequence* pseq = nullptr;
        switch (this->eAnimation) {
            case SurgeryMenu::SMA_GAME_START: {
                NiFixedString kName("Forward");
                pseq = ppsurgerygeomControllerManager->GetSequenceByName(kName);
                break;
            }
            case SurgeryMenu::SMA_MORPHINE: {
                NiFixedString kName("Right");
                pseq = ppsurgerygeomControllerManager->GetSequenceByName(kName);
                break;
            }
            case SurgeryMenu::SMA_STIMPAK: {
                NiFixedString kName("Left");
                pseq = ppsurgerygeomControllerManager->GetSequenceByName(kName);
                break;
            }
        }
        if (pseq) {
            ppsurgerygeomControllerManager->DeactivateAll(0.0f);
            ppsurgerygeomControllerManager->ActivateSequence(pseq, 0, true, 1.0f, 0.0f, nullptr);
            ppsurgerygeomControllerManager->SetActive(true);
            pseq->ResetSequence();
        }
    }
    NiTimeController::StartAnimations(SurgeryMenu::pSurgeryMenu->pFrameGeometry);
}

void SurgeryMenu::SwitchSkinGeometry(ActorValue::Index aeOldLimb, ActorValue::Index aeNewLimb) {
    NiNode* pSkin = SurgeryMenu::sp3DSkin.m_pObject;
    int32_t iIndex = GetIndexFromActorValue(aeOldLimb);
    pSkin->DetachChild(static_cast<NiAVObject*>(SurgeryMenu::pSurgeryMenu->pSkinGeometry[iIndex].m_pObject));
    pSkin = SurgeryMenu::sp3DSkin.m_pObject;
    iIndex = GetIndexFromActorValue(aeNewLimb);
    pSkin->AttachChild(static_cast<NiAVObject*>(SurgeryMenu::pSurgeryMenu->pSkinGeometry[iIndex].m_pObject), 1);
    pSkin = SurgeryMenu::sp3DSkin.m_pObject;
    NiFixedString kName("ScarMesh");
    SurgeryMenu::pSurgeryMenu->pScar = static_cast<NiGeometry*>(pSkin->GetObjectByName(kName));
}

int32_t SurgeryMenu::ShowLimbCondition() {
    float flimbPercent = 1.0 - GetLimbPercentFromSequences();
    float fscale = 0.0f;
    switch (SurgeryMenu::pSurgeryMenu->eCurrentDamageLevel) {
        case SurgeryMenu::SMWD_NONE: {
            fscale = 100.0f;
            break;
        }
        case SurgeryMenu::SMWD_LIGHT: {
            fscale = FLerp(100.0f, 75.0f, 0.0f, 1.0f, flimbPercent);
            break;
        }
        case SurgeryMenu::SMWD_MODERATE: {
            fscale = FLerp(100.0f, 50.0f, 0.0f, 1.0f, flimbPercent);
            break;
        }
        case SurgeryMenu::SMWD_HEAVY: {
            fscale = FLerp(100.0f, 25.0f, 0.0f, 1.0f, flimbPercent);
            break;
        }
        case SurgeryMenu::SMWD_SEVERE: {
            fscale = FLerp(100.0f, 0.0f, 0.0f, 1.0f, flimbPercent);
            break;
        }
        default: {
            break;
        }
    }
    float fCondition = static_cast<double>(static_cast<int32_t>(fscale)) / 100.0;
    int32_t iValueTrait = Tile::TextToTrait("_Value");
    pTiles[17]->Set(iValueTrait, fCondition, true);
    const char* pLimbName = ActorValue::GetActorValueName(eCurrentDamagedPart);
    pTiles[8]->Set(4036, pLimbName, true);
    return static_cast<int32_t>(fscale);
}

float SurgeryMenu::GetLimbPercentFromSequences() {
    float flimbPercent = 0.0f;
    float fTotalLength = 0.0f;
    SurgeryMenu::LimbWound* pwound = GetCurrentLimbWound();
    if (pwound) {
        for (BSSimpleList<SurgeryMenu::SequenceInfo*>* psequenceList = pwound->xSequenceInfo;
             psequenceList && !psequenceList->IsEmpty();
             psequenceList = psequenceList->m_pkNext) {
            NiControllerSequence* pseq = psequenceList->m_item->Sequence;
            flimbPercent = pseq->GetPhase() + flimbPercent;
            double dEndKeyTime = pseq->m_fEndKeyTime;
            fTotalLength = dEndKeyTime - pseq->GetBeginKeyTime() + fTotalLength;
        }
        if (flimbPercent < 0.0) {
            flimbPercent = 0.0f;
        }
        flimbPercent = flimbPercent / fTotalLength;
        return 1.0 - flimbPercent;
    }

    int32_t iValueTrait = Tile::TextToTrait("_Value");
    pTiles[17]->Set(iValueTrait, 1.0f, true);
    const char* pLimbName = ActorValue::GetActorValueName(eCurrentDamagedPart);
    pTiles[8]->Set(4036, pLimbName, true);
    return 0.0f;
}

void SurgeryMenu::SetLimbSequences(float afPercent) {
    BSSimpleList<SurgeryMenu::SequenceInfo*>* psequenceInfoList = SurgeryMenu::pSurgeryMenu->GetCurrentLimbWound()
                                                                      ->xSequenceInfo;
    while (psequenceInfoList && !psequenceInfoList->IsEmpty()) {
        // The phase is rounded to a float before it is passed.
        float afPhase = psequenceInfoList->m_item->Sequence->m_fEndKeyTime * afPercent;
        psequenceInfoList->m_item->Sequence->SetPhase(afPhase, true);
        psequenceInfoList = psequenceInfoList->m_pkNext;
    }
    NiUpdateData update(0.0f, true, false);
    SurgeryMenu::sp3DSkin->Update(update);
}

float SurgeryMenu::UpdateFlesh() {
    SurgeryMenu::LimbWound* pwound = GetCurrentLimbWound();
    if (!SurgeryMenu::sp3DCamera.m_pObject || !pwound) {
        return 0.0f;
    }
    pwound->xSequenceInfo->Sort(&SurgeryMenu::SortSequenceByCursorDistance);
    bool binsideWound = false;
    // The compare is unsigned, although ItemsInList returns int.
    if (static_cast<uint32_t>(pwound->xSequenceInfo->ItemsInList()) > 1u) {
        NiPoint2 toolLoc;
        toolLoc = ToolLocation;
        toolLoc.x = toolLoc.x / InterfaceManager::GetScreenWidth();
        toolLoc.y = toolLoc.y / InterfaceManager::GetScreenHeight();
        double pgon[24][2];
        double point[2];
        for (BSSimpleList<SurgeryMenu::SequenceInfo*>* psequenceList = pwound->xSequenceInfo;
             psequenceList && !psequenceList->IsEmpty();
             psequenceList = psequenceList->GetNext()) {
            NiPoint2 screenPoint1;
            NiPoint2 screenPoint2;
            BSSimpleList<NiAVObject*>* pbones = psequenceList->GetItem()->xBoneList;
            SurgeryMenu::sp3DCamera
                ->WorldPtToScreenPt(pbones->GetItem()->GetWorldTranslate(), screenPoint1.x, screenPoint1.y, 1e-05f);
            pbones = psequenceList->GetItem()->xBoneList;
            SurgeryMenu::sp3DCamera->WorldPtToScreenPt(
                pbones->GetNext()->GetItem()->GetWorldTranslate(),
                screenPoint2.x,
                screenPoint2.y,
                1e-05f
            );
            screenPoint1.y = 1.0f - screenPoint1.y;
            screenPoint2.y = 1.0f - screenPoint2.y;
            pgon[0][0] = screenPoint1.x;
            pgon[0][1] = screenPoint1.y;
            pgon[1][0] = screenPoint2.x;
            pgon[1][1] = screenPoint2.y;
            point[0] = toolLoc.x;
            point[1] = toolLoc.y;
            if (psequenceList->GetNext()) {
                NiPoint2 screenPoint3;
                NiPoint2 screenPoint4;
                pbones = psequenceList->GetNext()->GetItem()->xBoneList;
                SurgeryMenu::sp3DCamera
                    ->WorldPtToScreenPt(pbones->GetItem()->GetWorldTranslate(), screenPoint3.x, screenPoint3.y, 1e-05f);
                pbones = psequenceList->GetNext()->GetItem()->xBoneList;
                SurgeryMenu::sp3DCamera->WorldPtToScreenPt(
                    pbones->GetNext()->GetItem()->GetWorldTranslate(),
                    screenPoint4.x,
                    screenPoint4.y,
                    1e-05f
                );
                screenPoint3.y = 1.0f - screenPoint3.y;
                screenPoint4.y = 1.0f - screenPoint4.y;
                pgon[2][0] = screenPoint3.x;
                pgon[2][1] = screenPoint3.y;
                pgon[3][0] = screenPoint4.x;
                pgon[3][1] = screenPoint4.y;
                if (SurgeryMenu::PointInPoly(pgon, 4, point)) {
                    binsideWound = true;
                }
            }
        }
        if (binsideWound && !uiWoundBurnTime) {
            uiWoundBurnTime = appTimer.iLastTime;
        }

        if (binsideWound) {
            if (!BurningWoundSoundHand.IsPlaying()) {
                BurningWoundSoundHand.Play(false);
            }
            if (uiWoundBurnTime) {
                uint32_t uiBurnTime = appTimer.iLastTime - uiWoundBurnTime;
                float fmedicine = static_cast<float>(pPlayer->GetActorValue(ActorValue::MEDICINE));
                if (uiBurnTime >
                    static_cast<uint32_t>(static_cast<int64_t>(FLerp(500.0f, 0.0f, 0.0f, 100.0f, fmedicine)))) {
                    NiControllerSequence* psequence[2];
                    float fsequenceTime[2];
                    SurgeryMenu::SequenceInfo* pseqinfo[2];
                    psequence[0] = pwound->xSequenceInfo->GetItem()->Sequence;
                    psequence[1] = pwound->xSequenceInfo->GetNext()->GetItem()->Sequence;
                    pseqinfo[0] = pwound->xSequenceInfo->GetItem();
                    pseqinfo[1] = pwound->xSequenceInfo->GetNext()->GetItem();
                    fsequenceTime[0] = psequence[0]->m_fLastTime;
                    fsequenceTime[1] = psequence[1]->m_fLastTime;
                    if (uiWoundCloseTime) {
                        double dEndKeyTime = psequence[0]->m_fEndKeyTime;
                        float fspan = dEndKeyTime - psequence[0]->GetBeginKeyTime();
                        double dRate = std::fabs(fspan) * 0.014999999664723873;
                        fsequenceTime[0] = fsequenceTime[0] -
                            static_cast<double>(appTimer.iLastTime - uiWoundCloseTime) / 33.0 * dRate;
                        dEndKeyTime = psequence[1]->m_fEndKeyTime;
                        fspan = dEndKeyTime - psequence[1]->GetBeginKeyTime();
                        dRate = std::fabs(fspan) * 0.006000000052154064;
                        fsequenceTime[1] = fsequenceTime[1] -
                            static_cast<double>(appTimer.iLastTime - uiWoundCloseTime) / 33.0 * dRate;
                    }
                    else {
                        uiWoundCloseTime = appTimer.iLastTime;
                    }
                    for (int32_t i = 0; i < 2; ++i) {
                        psequence[i]->SetPhase(fsequenceTime[i], true);
                        pseqinfo[i]->fSequenceTime = fsequenceTime[i];
                    }
                    NiUpdateData update(0.0f, true, false);
                    int32_t iindex = GetIndexFromActorValue(pwound->eLimb);
                    static_cast<NiAVObject*>(SurgeryMenu::pSurgeryMenu->pSkinGeometry[iindex].m_pObject)
                        ->NiAVObject::Update(update);
                    if (SurgeryMenu::pSurgeryMenu->pScar &&
                        (fsequenceTime[0] + fsequenceTime[1]) / 2.0 < 0.10000000149011612) {
                        PlaceDecal(toolLoc, true);
                    }
                }
            }
        }
        else {
            PlaceDecal(toolLoc, false);
            if (!BurningFleshSoundHand.IsPlaying()) {
                BurningFleshSoundHand.Play(false);
            }
            if (BurningWoundSoundHand.IsPlaying()) {
                BurningWoundSoundHand.Stop();
            }
            uiWoundBurnTime = 0;
        }
    }
    uiWoundCloseTime = appTimer.iLastTime;
    float fresistanceTime;
    float fmagnitude;
    float fmouseSpeedScalar;
    int32_t imedicine = pPlayer->GetActorValue(ActorValue::MEDICINE);
    float fPainRatio = fPainPercent / 100.0;
    float fInjuryRatio = pMenu->GetFloat(iUT_InjuryPercent) / 100.0;
    GamePlayFormulas::CalculateToolResistance(
        fresistanceTime,
        fmagnitude,
        fmouseSpeedScalar,
        fInjuryRatio,
        fPainRatio,
        imedicine
    );
    NiPoint2 movement;
    float fNoiseTimeX = static_cast<double>(appTimer.iLastTime) * 0.0002500000118743628;
    movement.x = BSNoise1D::Noise(fNoiseTimeX);
    float fNoiseTimeY = static_cast<double>(appTimer.iLastTime + 12345) * 0.0002500000118743628;
    movement.y = BSNoise1D::Noise(fNoiseTimeY);
    int32_t iindex = GetIndexFromActorValue(pwound->eLimb);
    iindex = GetIndexFromActorValue(pwound->eLimb);
    static_cast<NiAVObject*>(pSkinGeometry[iindex].m_pObject)->NiAVObject::SetTranslate(movement.x, movement.y, 1.0f);
    NiUpdateData UpdateData1(0.0f, false, false);
    iindex = GetIndexFromActorValue(pwound->eLimb);
    static_cast<NiAVObject*>(pSkinGeometry[iindex].m_pObject)->NiAVObject::Update(UpdateData1);
    if (binsideWound) {
        return 0.0f;
    }

    return 100.0f;
}

void SurgeryMenu::PlaceToolInWindow(int32_t aiX, int32_t aiY) {
    int32_t icenterX;
    int32_t icenterY;
    int32_t iRadius;
    NiPoint2 kToTool;
    NiAVObject* pcenter = pFrameGeometry->GetObjectByName("CenterNode");
    NiAVObject* pouterEdge = pFrameGeometry->GetObjectByName("FrameNode");
    if (pcenter && pouterEdge) {
        NiPoint2 outerEdge;
        NiPoint2 center;
        SurgeryMenu::sp3DCamera->WorldPtToScreenPt(pcenter->GetWorldTranslate(), center.x, center.y, 1e-05f);
        SurgeryMenu::sp3DCamera->WorldPtToScreenPt(pouterEdge->GetWorldTranslate(), outerEdge.x, outerEdge.y, 1e-05f);
        center.x = InterfaceManager::GetScreenWidth() * center.x;
        center.y = InterfaceManager::GetScreenHeight() * (1.0 - center.y);
        outerEdge.x = InterfaceManager::GetScreenWidth() * outerEdge.x;
        outerEdge.y = InterfaceManager::GetScreenHeight() * (1.0 - outerEdge.y);
        icenterX = static_cast<int32_t>(center.x);
        icenterY = static_cast<int32_t>(center.y);
        iRadius = static_cast<int32_t>((center - outerEdge).Length());
    }
    else {
        icenterX = 800;
        icenterY = 475;
        iRadius = 325;
    }
    kToTool.x = static_cast<float>(aiX - icenterX);
    kToTool.y = static_cast<float>(aiY - icenterY);
    if (static_cast<double>(iRadius) < kToTool.Length()) {
        kToTool.Unitize();
        kToTool *= static_cast<float>(iRadius);
    }
    float fToolX = static_cast<double>(icenterX) + kToTool.x;
    pTiles[6]->Set(4001, fToolX, true);
    float fToolY = static_cast<double>(icenterY) + kToTool.y;
    pTiles[6]->Set(4002, fToolY, true);
}

void SurgeryMenu::PlaceDecal(NiPoint2 aToolLocation, bool abInWound) {
    NiPoint3 korigin;
    NiPoint3 kdir;
    float fx = static_cast<double>(Renderer::GetScreenWidth()) * aToolLocation.x;
    float fy = static_cast<double>(Renderer::GetScreenHeight()) * aToolLocation.y;
    SurgeryMenu::sp3DCamera
        ->WindowPointToRay(static_cast<int32_t>(fx), static_cast<int32_t>(fy), korigin, kdir, nullptr);
    NiPoint3 isect = NiPoint3::ZERO;
    NiPick activatePick(0, 8u);
    activatePick.SetObserveAppCullFlag(false);
    activatePick.SetTarget(SurgeryMenu::pSurgeryMenu->pScar);
    if (activatePick.PickObjects(korigin, kdir, false)) {
        uint32_t uiIndex = 0;
        NiPick::Record* ppickRecord = activatePick.m_pickResults.GetAt(uiIndex);
        if (ppickRecord) {
            isect = ppickRecord->m_intersect;
            isect.z = isect.z + 10.0;
        }
    }
    TESForm* pForm;
    if (abInWound) {
        pForm = TESForm::GetFormByEditorID("DecalSurgeryGood");
    }
    else {
        pForm = TESForm::GetFormByEditorID("DecalSurgeryBad");
    }
    BGSTextureSet* pgoodTextureSet = dynamic_cast<BGSTextureSet*>(pForm);
    DECAL_CREATION_DATA pdecalData;
    pdecalData.pOrigin = isect;
    pdecalData.pDirection = NiPoint3(0.0f, 0.0f, 1.0f);
    pdecalData.pAVObj = SurgeryMenu::pSurgeryMenu->pScar;
    pdecalData.fRotation = RandomFloatTwoPi();
    pdecalData.pTexSet = pgoodTextureSet;
    pdecalData.bUseOcclusionQuery = false;
    pdecalData.pParentCell = pPlayer->GetParentCell();
    pdecalData.fDecalWidth = 0.5f;
    pdecalData.fDecalHeight = 0.5f;
    pdecalData.fDecalDepth = 20.0f;
    pdecalData.fPlacementRadius = 0.0f;
    pdecalData.fVertexColor = NiColor(1.0f, 0.0f, 0.0f);
    pdecalData.fDecalEpsilon = 75.0f;
    pdecalData.bAlphaTesting = true;
    pdecalData.bAlphaBlending = true;
    pdecalData.bModelSpace = false;
    BGSDecalManager::GetSingleton()->AddDecal(pdecalData, BGSDecalManager::DECAL_TYPE_SIMPLE, true);
}

void SurgeryMenu::Draw3DElements() {
    if (SurgeryMenu::sp3DSkin.m_pObject && SurgeryMenu::spPlayer3D.m_pObject) {
        NiRenderer::GetRenderer()->ClearBuffer(nullptr, 4u);
        BSCullingProcess* pCuller = Main::GetWorldSceneGraph()->pCuller;
        pCuller->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
        NiCamera* pcam = SurgeryMenu::sp3DCamera.m_pObject;
        NiCamera::HandleEyeFinityFOVScale(pcam, 0.15f);
        ShadowSceneNode* pssnode = BSShaderManager::GetShadowSceneNode(1);
        NiPointer<BSShaderAccumulator> spaccum(InterfaceManager::Instance()->Get3dAccumulator());
        pCuller->SetAccumulator(spaccum.m_pObject);
        BSShaderUtil::AccumulateScene(pcam, pssnode, *pCuller);
        BSShaderUtil::RenderScene(pcam, spaccum.m_pObject, false);
        SurgeryMenu::spPlayer3D->SetAppCulled(false);
        pssnode = BSShaderManager::GetShadowSceneNode(3);
        BSShaderUtil::AccumulateScene(pcam, pssnode, *pCuller);
        BSShaderUtil::RenderScene(pcam, spaccum.m_pObject, false);
        pCuller->SetAccumulator(nullptr);
        spaccum = static_cast<BSShaderAccumulator*>(nullptr);
        pCuller->PopCullMode();
    }
}

void SurgeryMenu::SetCursorPos(int32_t aiX, int32_t aiY) {
    if (SurgeryMenu::pSurgeryMenu && !SurgeryMenu::pSurgeryMenu->bBurning) {
        float fY = static_cast<float>(static_cast<double>(aiY) / Renderer::GetScreenHeight());
        float fX = static_cast<float>(static_cast<double>(aiX) / Renderer::GetScreenWidth());
        Interface::SetCursorPos(fX, fY);
    }
}

int32_t SurgeryMenu::GetClassStatic() {
    return 1054;
}

// Walks xLimbWounds through IsEmpty, GetItem and GetNext.
void SurgeryMenu::ResetAllLimbs() {
    if (!pSurgeryMenu) {
        return;
    }
    for (BSSimpleList<LimbWound*>* pIter = &xLimbWounds; pIter;) {
        bool bEmpty = pIter->IsEmpty();
        if (bEmpty) {
            break;
        }
        LimbWound* pWound = pIter->GetItem();
        pWound->ResetLimb();
        pIter = pIter->GetNext();
    }
}

void SurgeryMenu::ResetLimb(ActorValue::Index aeLimb) {
    if (!pSurgeryMenu || Interface::IsTopMenuID(Interface::eSurgeryMenu)) {
        return;
    }
    for (BSSimpleList<LimbWound*>* pIter = &xLimbWounds; pIter;) {
        bool bEmpty = pIter->IsEmpty();
        if (bEmpty) {
            break;
        }
        LimbWound* pWound = pIter->GetItem();
        if (pWound->eLimb == aeLimb) {
            pIter->GetItem()->ResetLimb();
            return;
        }
        pIter = pIter->GetNext();
    }
}

void SurgeryMenu::Close() {
    Tile* ptile = Menu::GetMenuByClass(SurgeryMenu::GetClassStatic());
    if (ptile) {
        if (SurgeryMenu::pSurgeryMenu) {
            BSSoundHandle soundHand;
            soundHand = BSAudio::QInstance().GetSoundHandleByName("UISurgeryExit", 289u);
            soundHand.Play(false);
            ptile->Set(Tile::kTileValue_DeleteOnFade, 1);
            SurgeryMenu::pSurgeryMenu->StartFadeOut();
            for (BSSimpleList<SurgeryMenu::LimbWound*>* plist = &SurgeryMenu::xLimbWounds; plist && !plist->IsEmpty();
                 plist = plist->m_pkNext) {
                SurgeryMenu::ResetLimb(plist->m_item->eLimb);
            }
            InterfaceManager::Instance()->pCursor->Set(4009, 255);
        }
        SurgeryMenu::Shutdown3DElements();
        SurgeryMenu::pSurgeryMenu->BurningFleshSoundHand.Stop();
        SurgeryMenu::pSurgeryMenu->BurningWoundSoundHand.Stop();
        SurgeryMenu::pSurgeryMenu->BurningFleshSoundHand.Release();
        SurgeryMenu::pSurgeryMenu->BurningWoundSoundHand.Release();
    }
}

void SurgeryMenu::PrepareSequences(NiAVObject* apNode) {
    if (apNode) {
        NiTimeController* pcontroller = apNode->GetController(&NiControllerManager::ms_RTTI);
        bool bwoundPreviouslyCreated = true;
        if (pcontroller) {
            SurgeryMenu::pSurgeryMenu->pSkinControllerManager = static_cast<NiControllerManager*>(pcontroller);
            SurgeryMenu::LimbWound* plimbWound = SurgeryMenu::pSurgeryMenu->GetCurrentLimbWound();
            SurgeryMenu::LimbWound* pwound = nullptr;
            if (plimbWound) {
                pwound = plimbWound;
            }
            else {
                bwoundPreviouslyCreated = false;
                pwound = static_cast<SurgeryMenu::LimbWound*>(ourMalloc(sizeof(SurgeryMenu::LimbWound)));
                pwound->eLimb = SurgeryMenu::pSurgeryMenu->eCurrentDamagedPart;
                double dCondition = pPlayer->GetActorFloatValue(pwound->eLimb);
                pwound->eDamage = SurgeryMenu::pSurgeryMenu->GetLimbDamage(static_cast<int32_t>(dCondition));
                pwound->xSequenceInfo = new BSSimpleList<SurgeryMenu::SequenceInfo*>;
                pwound->bResetLimb = false;
                SurgeryMenu::xLimbWounds.AddHead(pwound);
            }
            for (int32_t n = 0; n < SurgeryMenu::pSurgeryMenu->pSkinControllerManager->GetSequenceCount(); ++n) {
                char goodTargets[32];
                NiControllerSequence* pseq = SurgeryMenu::pSurgeryMenu->pSkinControllerManager->GetSequenceAt(
                    static_cast<uint32_t>(n)
                );
                std::sprintf(goodTargets, "Bone%s", pseq->m_kName.GetHandleString());
                SurgeryMenu::SequenceInfo* pSeqInfo = nullptr;
                if (!plimbWound || plimbWound->bResetLimb) {
                    pSeqInfo = static_cast<SurgeryMenu::SequenceInfo*>(ourMalloc(sizeof(SurgeryMenu::SequenceInfo)));
                    pSeqInfo->xBoneList = new BSSimpleList<NiAVObject*>;
                    pSeqInfo->Sequence = pseq;
                }
                for (int32_t m = 0; m < pseq->m_uiArraySize; ++m) {
                    NiControllerSequence::IDTag* ptag = pseq->GetIDTag(static_cast<uint32_t>(m));
                    if (pseq->GetInterpolatorAt(static_cast<uint32_t>(m))) {
                        if (!_strnicmp(goodTargets, ptag->m_kAVObjectName.GetHandleString(), 6u)) {
                            if (!plimbWound || plimbWound->bResetLimb) {
                                NiAVObject* pBone = pseq->m_pkAccumRoot->GetObjectByName(ptag->m_kAVObjectName);
                                pSeqInfo->xBoneList->AddHead(pBone);
                            }
                        }
                        else {
                            pseq->RemoveInterpolator_0(m);
                        }
                    }
                }
                if (pSeqInfo) {
                    pwound->xSequenceInfo->AddHead(pSeqInfo);
                }
            }
        }
        float fscale = 0.0f;
        double dCondition = pPlayer->GetActorFloatValue(SurgeryMenu::pSurgeryMenu->eCurrentDamagedPart);
        SurgeryMenu::pSurgeryMenu->eCurrentDamageLevel = SurgeryMenu::pSurgeryMenu->GetLimbDamage(
            static_cast<int32_t>(dCondition)
        );
        switch (SurgeryMenu::pSurgeryMenu->eCurrentDamageLevel) {
            case SurgeryMenu::SMWD_LIGHT: {
                fscale = (25.0 - (pPlayer->GetActorFloatValue(SurgeryMenu::pSurgeryMenu->eCurrentDamagedPart) - 75.0)) /
                    25.0;
                break;
            }
            case SurgeryMenu::SMWD_MODERATE: {
                fscale = (25.0 - (pPlayer->GetActorFloatValue(SurgeryMenu::pSurgeryMenu->eCurrentDamagedPart) - 50.0) +
                          25.0) /
                    50.0;
                break;
            }
            case SurgeryMenu::SMWD_HEAVY: {
                fscale = (25.0 - (pPlayer->GetActorFloatValue(SurgeryMenu::pSurgeryMenu->eCurrentDamagedPart) - 25.0) +
                          50.0) /
                    75.0;
                break;
            }
            case SurgeryMenu::SMWD_SEVERE: {
                fscale = (25.0 - pPlayer->GetActorFloatValue(SurgeryMenu::pSurgeryMenu->eCurrentDamagedPart) + 75.0) /
                    100.0;
                break;
            }
            default: {
                break;
            }
        }
        SurgeryMenu::LimbWound* plimbWounds = SurgeryMenu::pSurgeryMenu->GetCurrentLimbWound();
        if (plimbWounds && SurgeryMenu::pSurgeryMenu->eCurrentDamageLevel) {
            for (BSSimpleList<SurgeryMenu::SequenceInfo*>* psequenceInfoList = plimbWounds->xSequenceInfo;
                 psequenceInfoList && !psequenceInfoList->IsEmpty();
                 psequenceInfoList = psequenceInfoList->m_pkNext) {
                if (psequenceInfoList->m_item->Sequence->m_pkOwner ==
                    SurgeryMenu::pSurgeryMenu->pSkinControllerManager) {
                    SurgeryMenu::pSurgeryMenu->pSkinControllerManager
                        ->ActivateSequence(psequenceInfoList->m_item->Sequence, 0, true, 1.0f, 0.0f, nullptr);
                }
                psequenceInfoList->m_item->Sequence->Update(0.0f, true);
                if (bwoundPreviouslyCreated && !plimbWounds->bResetLimb) {
                    SurgeryMenu::SequenceInfo* pSeqInfo = psequenceInfoList->m_item;
                    psequenceInfoList->m_item->Sequence->SetPhase(pSeqInfo->fSequenceTime, true);
                }
                else {
                    double dEndKeyTime = psequenceInfoList->m_item->Sequence->m_fEndKeyTime;
                    float fPhase = dEndKeyTime * fscale;
                    psequenceInfoList->m_item->Sequence->SetPhase(fPhase, true);
                    dEndKeyTime = psequenceInfoList->m_item->Sequence->m_fEndKeyTime;
                    psequenceInfoList->m_item->fSequenceTime = dEndKeyTime * fscale;
                }
            }
            plimbWounds->bResetLimb = false;
        }

        NiUpdateData update(0.0f, true, false);
        apNode->Update(update);
    }
}

void SurgeryMenu::Shutdown3DElements() {
    BSShaderManager::GetShadowSceneNode(1)->DetachChild(SurgeryMenu::sp3DSkin.m_pObject);
    BSShaderManager::GetShadowSceneNode(3)->DetachChild(SurgeryMenu::spPlayer3D.m_pObject);
    BSShaderManager::GetShadowSceneNode(1)->RemoveAllLights();
    BSShaderManager::GetShadowSceneNode(3)->RemoveAllLights();
    pPlayer->CloneInventory3D(false);
    int32_t iPropertyType = TESClass::GetTagSkillCount();
    SurgeryMenu::spPlayer3D->RemoveProperty(iPropertyType);
    SurgeryMenu::spPlayer3D->UpdateProperties();
    SurgeryMenu::sp3DCamera = static_cast<NiCamera*>(nullptr);
    SurgeryMenu::sp3DSkin = static_cast<NiNode*>(nullptr);
    SurgeryMenu::spPlayer3D = static_cast<NiAVObject*>(nullptr);
}

// Compares each bone's distance and, when it is the nearest so far, computes it again to store it.
int32_t
SurgeryMenu::SortSequenceByCursorDistance(SurgeryMenu::SequenceInfo* apObject1, SurgeryMenu::SequenceInfo* apObject2) {
    BSSimpleList<NiAVObject*>* pbones;
    NiPoint2 tool;
    NiPoint2 screenPoint;
    if (!SurgeryMenu::pSurgeryMenu) {
        return 0;
    }
    float fdist = std::numeric_limits<float>::max();
    float fToolY = SurgeryMenu::pSurgeryMenu->ToolLocation.y;
    tool.x = SurgeryMenu::pSurgeryMenu->ToolLocation.x;
    tool.y = fToolY;
    tool.x = tool.x / InterfaceManager::GetScreenWidth();
    tool.y = tool.y / InterfaceManager::GetScreenHeight();
    for (pbones = apObject1->xBoneList; SurgeryMenu::sp3DCamera.m_pObject && pbones && !pbones->IsEmpty();
         pbones = pbones->m_pkNext) {
        SurgeryMenu::sp3DCamera
            ->WorldPtToScreenPt(pbones->m_item->GetWorldTranslate(), screenPoint.x, screenPoint.y, 1e-05f);
        screenPoint.y = 1.0 - screenPoint.y;
        if (fdist > (screenPoint - tool).Length()) {
            fdist = (screenPoint - tool).Length();
        }
    }
    for (pbones = apObject2->xBoneList; SurgeryMenu::sp3DCamera.m_pObject && pbones && !pbones->IsEmpty();
         pbones = pbones->m_pkNext) {
        SurgeryMenu::sp3DCamera
            ->WorldPtToScreenPt(pbones->m_item->GetWorldTranslate(), screenPoint.x, screenPoint.y, 1e-05f);
        screenPoint.y = 1.0 - screenPoint.y;
        if (fdist > (screenPoint - tool).Length()) {
            return 1;
        }
    }
    return -1;
}

int32_t SurgeryMenu::PointInPoly(double (*pgon)[2], int32_t numverts, double* point) {
    double dTestY = point[1];
    double (*pVtx0)[2] = &pgon[numverts - 1];
    int32_t iYFlag0 = dTestY <= (*pVtx0)[1];
    double (*pVtx1)[2] = pgon;
    int32_t iInsideFlag = 0;
    int32_t iCount = numverts + 1;
    while (--iCount) {
        int32_t iYFlag1 = dTestY <= (*pVtx1)[1];
        if (iYFlag0 != iYFlag1 &&
            ((*pVtx1)[0] - *point) * ((*pVtx0)[1] - (*pVtx1)[1]) <=
                    ((*pVtx1)[1] - dTestY) * ((*pVtx0)[0] - (*pVtx1)[0]) ==
                iYFlag1) {
            iInsideFlag = iInsideFlag == 0;
        }
        iYFlag0 = dTestY <= (*pVtx1)[1];
        pVtx0 = pVtx1;
        ++pVtx1;
    }
    return iInsideFlag;
}
