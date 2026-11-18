#include "Fallout/Interface/Menus/VATSMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSDevices/Controls.h"
#include "BSHavok/bhkPickData.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/TileShaderProperty.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSAmmoForm.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSBodyPartData.h"
#include "Fallout Shared/BGSDefaultObjectManager.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Character.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/Combat/VATSBodyPart.h"
#include "Fallout/AI/Combat/VATSTarget.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/FOVATSEffectManager.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpAllRayHitCollector.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/stdlib.h"

#include <cmath>

static int32_t iUT_Brightness;

// DoIdle's function-local statics: the attack held since an earlier frame, the screen size and safe zone read
// once, the _Brightness user trait, and their guard.
static DesiredAttackType heldAttack{};
static int32_t isafeZoneY;
static int32_t isafeZoneX;
static int32_t iscreenHeight;
static int32_t iscreenWidth;

// The guard bits of the five guarded statics (1 iscreenWidth,
// 2 iscreenHeight, 4 isafeZoneX, 8 isafeZoneY, 0x10 iUT_Brightness).
static uint32_t uiDoIdleStaticsGuard;

using VATSMenuUnusedBytes = uint8_t[4];
static VATSMenuUnusedBytes data_11DB1D0{};

// The game settings this file defines, with the game's IDs and values, in initialization order.
SettingT<INISettingCollection> bVatsAlwaysHit("bVatsAlwaysHit:Combat", false);
BSSimpleList<TESObjectREFR*> VATSMenu::Destructibles;
BSSimpleList<VATSTarget*> VATSMenu::xTargetList;

VATSMenu::~VATSMenu() {
    pFaderManager->RemoveFader(FADER_BLACK_UNDER_MENUS, false);
    ClearTargetList();
    TileMenu* MenuTile = VATSMenu::pVatsMenu->pMenu;
    if (MenuTile) {
        delete MenuTile;
    }
    VATSMenu::pVatsMenu = nullptr;
}

void VATSMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if ((aiID > 0) && (aiID <= 31)) {
        pTiles[aiID] = apTile;
    }
}

// Clicks on the VATS menu: the shoulder buttons pick the nearest target to the left or right; IDs 2 and 3 step to the
// previous or next target with a body to show, 4 closes the menu, 5 queues the current action, 30 and 31 select the
// weapon's special attacks.
void VATSMenu::DoClick(int32_t aiID, Tile* apTarget) {
    XFNV_INLINED_LOCAL(BSSoundHandle, kPrevFailSound, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(BSSoundHandle, kPrevSelectSound, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(BSSoundHandle, kNextFailSound, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(BSSoundHandle, kNextSelectSound, std::nullopt, std::nullopt);
    BaseProcess* pProcess = pPlayer->pCurrentProcess;
    ItemChange* pWeaponItem = pProcess->GetCurrentWeapon();
    TESObjectWEAP* pWeapon;
    if (pWeaponItem) {
        pWeapon = static_cast<TESObjectWEAP*>(pWeaponItem->pContainerObj);
    }
    else {
        pWeapon = nullptr;
    }
    if (bSelectLeftHeld) {
        if (VATSMenu::SelectLeftHeld()) {
            SelectNearestTarget(true);
            bSelectLeftHeld = false;
            return;
        }
        bSelectLeftHeld = false;
    }
    if (bSelectRightHeld) {
        if (VATSMenu::SelectRightHeld()) {
            SelectNearestTarget(false);
            bSelectRightHeld = false;
            return;
        }
        bSelectRightHeld = false;
    }
    BSSoundHandle kSound;
    InterfaceManager::Instance();
    if (InterfaceManager::Has360Controller() && !bTargetStickReset) {
        return;
    }
    switch (aiID) {
        case 2: {
            // The count is compared unsigned; ItemsInList returns int.
            if (static_cast<uint32_t>(VATSMenu::xTargetList.ItemsInList()) <= 1) {
                kSound = *BSAudio::QInstance().GetSoundHandleByName(&kPrevFailSound, "UIVATSEnterFail", 289u);
                kSound.Play(false);
                return;
            }
            kSound = *BSAudio::QInstance().GetSoundHandleByName(&kPrevSelectSound, "UIVATSSelectTarget", 289u);
            kSound.Play(false);
            // Puts every target's meters back to full alpha.
            for (BSSimpleList<VATSTarget*>* pIter = &VATSMenu::xTargetList; pIter && !pIter->IsEmpty();
                 pIter = pIter->m_pkNext) {
                for (BSSimpleList<VATSBodyPart*>* pPart = &pIter->m_item->kBodyParts;
                     pPart && !pPart->IsEmpty() && pPart->m_item->pMeter; pPart = pPart->m_pkNext) {
                    pPart->m_item->pMeter->RemoveFadeControl(Tile::kTileValue_Alpha);
                    pPart->m_item->pMeter->Set(Tile::kTileValue_Alpha, 0);
                }
            }
            // The target before the current one (the node whose successor holds it) when its reference has 3D, else
            // the last target.
            for (BSSimpleList<VATSTarget*>* pIter = &VATSMenu::xTargetList; pIter; pIter = pIter->m_pkNext) {
                if (pIter->m_item->pTargetRef) {
                    TESObjectREFR* pTargetRef = pIter->m_item->pTargetRef;
                    if (pTargetRef->Get3D() && pIter->m_pkNext && pIter->m_pkNext->m_item->pTargetRef == pVATSTarget) {
                        pVATSTarget = pIter->m_item->pTargetRef;
                        BSSimpleList<VATSBodyPart*>* pParts = &pIter->m_item->kBodyParts;
                        BSSimpleList<VATSBodyPart*>* pLimb = FindTargetPart(ActorValue::ENDURANCE_CONDITION);
                        if (pIter->m_item->eType == 1 && pLimb) {
                            VATSMenu::pVatsMenu->pCurrentLimb = pLimb->m_item;
                        }
                        else {
                            VATSMenu::pVatsMenu->pCurrentLimb = pParts->m_item;
                        }
                        break;
                    }
                }
                if (!pIter->m_pkNext) {
                    pVATSTarget = pIter->m_item->pTargetRef;
                    BSSimpleList<VATSBodyPart*>* pLimb = FindTargetPart(ActorValue::ENDURANCE_CONDITION);
                    if (pIter->m_item->eType == 1 && pLimb) {
                        VATSMenu::pVatsMenu->pCurrentLimb = pLimb->m_item;
                    }
                    else {
                        VATSMenu::pVatsMenu->pCurrentLimb = pIter->m_item->kBodyParts.m_item;
                    }
                }
            }
            VATSMenu::SetHeadingReached(false);
            if (!GetVatsTarget(pVATSTarget)->m_item->bChancesCalculated) {
                for (BSSimpleList<VATSBodyPart*>* pPart = &GetVatsTarget(pVATSTarget)->m_item->kBodyParts;
                     pPart && !pPart->IsEmpty(); pPart = pPart->m_pkNext) {
                    pPart->m_item->bChanceCalculated = false;
                    pPart->m_item->bFirstTimeShown = false;
                }
            }
            if (!GetVatsTarget(pVATSTarget)->m_item->kBodyParts.m_item->pMeter) {
                BuildMetersForTarget(GetVatsTarget(pVATSTarget));
            }
            break;
        }
        case 3: {
            // The count is compared unsigned; ItemsInList returns int.
            if (static_cast<uint32_t>(VATSMenu::xTargetList.ItemsInList()) <= 1) {
                kSound = *BSAudio::QInstance().GetSoundHandleByName(&kNextFailSound, "UIVATSEnterFail", 289u);
                kSound.Play(false);
                return;
            }
            kSound = *BSAudio::QInstance().GetSoundHandleByName(&kNextSelectSound, "UIVATSSelectTarget", 289u);
            kSound.Play(false);
            // Puts every target's meters back to full alpha.
            for (BSSimpleList<VATSTarget*>* pIter = &VATSMenu::xTargetList; pIter && !pIter->IsEmpty();
                 pIter = pIter->m_pkNext) {
                for (BSSimpleList<VATSBodyPart*>* pPart = &pIter->m_item->kBodyParts;
                     pPart && !pPart->IsEmpty() && pPart->m_item->pMeter; pPart = pPart->m_pkNext) {
                    pPart->m_item->pMeter->RemoveFadeControl(Tile::kTileValue_Alpha);
                    pPart->m_item->pMeter->Set(Tile::kTileValue_Alpha, 0);
                }
            }
            // The target after the current one when its reference has 3D; otherwise the first target, and the search
            // goes on past the current one.
            for (BSSimpleList<VATSTarget*>* pIter = &VATSMenu::xTargetList; pIter; pIter = pIter->m_pkNext) {
                if (pVATSTarget != pIter->m_item->pTargetRef) {
                    continue;
                }
                if (pIter->m_pkNext && pIter->m_pkNext->m_item->pTargetRef &&
                    pIter->m_pkNext->m_item->pTargetRef->Get3D()) {
                    pVATSTarget = pIter->m_pkNext->m_item->pTargetRef;
                    BSSimpleList<VATSBodyPart*>* pParts = &pIter->m_pkNext->m_item->kBodyParts;
                    BSSimpleList<VATSBodyPart*>* pLimb = FindTargetPart(ActorValue::ENDURANCE_CONDITION);
                    if (pIter->m_item->eType == 1 && pLimb) {
                        VATSMenu::pVatsMenu->pCurrentLimb = pLimb->m_item;
                    }
                    else {
                        VATSMenu::pVatsMenu->pCurrentLimb = pParts->m_item;
                    }
                    break;
                }
                pVATSTarget = VATSMenu::xTargetList.m_item->pTargetRef;
                BSSimpleList<VATSBodyPart*>* pLimb = FindTargetPart(ActorValue::ENDURANCE_CONDITION);
                if (pIter->m_item->eType == 1 && pLimb) {
                    VATSMenu::pVatsMenu->pCurrentLimb = pLimb->m_item;
                }
                else {
                    VATSMenu::pVatsMenu->pCurrentLimb = VATSMenu::xTargetList.m_item->kBodyParts.m_item;
                }
            }
            VATSMenu::SetHeadingReached(false);
            if (!GetVatsTarget(pVATSTarget)->m_item->bChancesCalculated) {
                for (BSSimpleList<VATSBodyPart*>* pPart = &GetVatsTarget(pVATSTarget)->m_item->kBodyParts;
                     pPart && !pPart->IsEmpty(); pPart = pPart->m_pkNext) {
                    pPart->m_item->bChanceCalculated = false;
                    pPart->m_item->bFirstTimeShown = false;
                }
            }
            if (!GetVatsTarget(pVATSTarget)->m_item->kBodyParts.m_item->pMeter) {
                BuildMetersForTarget(GetVatsTarget(pVATSTarget));
            }
            break;
        }
        case 4: {
            if (!bClosing) {
                Interface::CloseVATSMenu();
            }
            break;
        }
        case 5: {
            if (!bClosing && pVATS.eMode == VATS::VATS_MENU_MODE && bHeadingReached) {
                UpdateQueuedActionList(nullptr, false, false);
                VATSMenu::pVatsMenu->fProbableEnemyDamage = VATSMenu::pVatsMenu->CalculateProbableEnemyHealth();
            }
            break;
        }
        case 30: {
            if (!bClosing) {
                bWeaponSpecialSelected = true;
            }
            break;
        }
        case 31: {
            if (!bClosing) {
                g_bIsVATSSpecialAttack2 = true;
            }
            break;
        }
        default: {
            break;
        }
    }
    if ((pVATSTarget) && (pVATSTarget->IsActor())) {
        eVATSTargetLimb = ActorValue::PERCEPTION_CONDITION;
    }

    VATSMenu::SetupSpecialAttackButtons(pWeapon);
    SetTargetLimb();
    if ((pVATS.GetVATSLight()) && (pVATSTarget)) {
        pVATS.UpdateVATSLight(pVATSTarget);
    }
}

void VATSMenu::DoEnter(int32_t aiID, Tile* apTarget) {
    InterfaceManager::Instance();
    if (!InterfaceManager::Has360Controller()) {
        switch (aiID) {
            case 2:
            case 3:
            case 4:
            case 5:
            case 30:
            case 31: {
                bMouseOverButton = (uint8_t)1;
                break;
            }
            default: {
                return;
            }
        }
    }
}

void VATSMenu::DoLeave(int32_t aiID, Tile* apTarget) {
    InterfaceManager::Instance();
    if (!InterfaceManager::Has360Controller()) {
        switch (aiID) {
            case 2:
            case 3:
            case 4:
            case 5:
            case 30:
            case 31: {
                bMouseOverButton = (uint8_t)0;
                break;
            }
            default: {
                return;
            }
        }
    }
}

void VATSMenu::DoStillDown(int32_t a1, Tile* a2) {
    if (a1 == 2) {
        if (!bSelectLeftHeld) {
            uiSelectTimer = appTimer.GetTime();
            bSelectLeftHeld = true;
        }
    }
    else {
        if (a1 == 3 && !bSelectRightHeld) {
            uiSelectTimer = appTimer.GetTime();
            bSelectRightHeld = true;
        }
    }
}

void VATSMenu::DoWheelMove(int32_t a2, Tile* a3) {
    InterfaceManager* singleton = InterfaceManager::Instance();
    if ((singleton->GetMouseWheel() / (-120)) == 1) {
        DoClick(2, nullptr);
    }
    else {
        DoClick(3, nullptr);
    }
}

// The explicit destructor calls are deliberate and are made where the game makes them.
void VATSMenu::DoIdle() {
    if (this->bClosing || Menu::GetMenuByClass(Interface::eTutorialMenu)) {
        return;
    }
    if (!pVATSTarget || !pVATSTarget->Get3D() || pVATSTarget->IsDead(false) || pPlayer->IsDead(false)) {
        VATSMenu::Close();
        return;
    }
    if (!VATSMenu::pVatsMenu || pVATS.GetMode() == VATS::VATS_MODE_NONE || pVATS.GetMode() == VATS::VATS_PLAYBACK ||
        !Interface::IsTopMenuID(Interface::eVATSMenu)) {
        if (!Interface::IsTopMenuID(Interface::eVATSMenu) && bLimbLit) {
            bLimbLit = false;
            Interface::GetVATSEffect()->SetTarget(nullptr, static_cast<BGSBodyPart::LIMB_ENUM>(0xFFFFFFFF), false);
        }
        this->pTiles[28]->RemoveFadeControl(static_cast<int32_t>(0x80000000));
        this->pTiles[28]->xChildren.GetHead()->RemoveFadeControl(static_cast<int32_t>(0x80000000));
        this->pTiles[10]->RemoveFadeControl(static_cast<int32_t>(0x80000000));
        return;
    }
    // The meters of the target's first body part are built once.
    if (!this->GetVatsTarget(pVATSTarget)->GetItem()->kBodyParts.GetItem()->pMeter) {
        this->BuildMetersForTarget(this->GetVatsTarget(pVATSTarget));
    }
    if (!bLimbLit) {
        VATSMenu::ShowTargetLimb(true);
    }
    bLimbLit = true;
    if (Interface::IsConsoleVisible()) {
        return;
    }
    this->UpdateChanceToHit();
    Controls* pControls = appMain->GetControls();
    InterfaceManager::Instance();
    if (InterfaceManager::Has360Controller()) {
        // A dead zone is computed by mode and never read, as the game does; the stick tests use 0x21F1.
        [[maybe_unused]] int32_t iDeadZone = pVATS.GetMode() != VATS::VATS_TARGET_SELECT ? 0x21F1 : 0x1EA9;
        [[maybe_unused]] int32_t iLeftX = pControls->GetControllerValue(0, 7);
        [[maybe_unused]] int32_t iLeftY = pControls->GetControllerValue(0, 8);
        int32_t iRightX = pControls->GetControllerValue(0, 9);
        int32_t iRightY = pControls->GetControllerValue(0, 10);
        iLeftY = -iLeftY;
        iRightY = -iRightY;
        if (iRightX > 0x21F1) {
            if (!this->bSelectRightHeld) {
                this->uiSelectTimer = appTimer.GetTime();
                this->bSelectRightHeld = true;
            }
        }
        else if (iRightX < -0x21F1) {
            if (!this->bSelectLeftHeld) {
                this->uiSelectTimer = appTimer.GetTime();
                this->bSelectLeftHeld = true;
            }
        }
        else if (std::abs(iRightX) < 0x21F1 && std::abs(iRightY) < 0x21F1) {
            if (VATSMenu::SelectRightHeld()) {
                VATSMenu::pVatsMenu->bSelectRightHeld = false;
                this->DoClick(3, nullptr);
            }
            else if (this->bSelectRightHeld) {
                this->DoClick(3, nullptr);
            }
            else if (VATSMenu::SelectLeftHeld()) {
                VATSMenu::pVatsMenu->bSelectLeftHeld = false;
                this->DoClick(2, nullptr);
            }
            else if (this->bSelectLeftHeld) {
                this->DoClick(2, nullptr);
            }
            this->bTargetStickReset = 1;
        }
    }
    if (pVATSTarget && pVATSTarget->IsActor()) {
        // The conversion to ActorValueOwner is a reference conversion with no null check, as the game does.
        ActorValueOwner& rTargetValues = static_cast<ActorValueOwner&>(*static_cast<Actor*>(pVATSTarget));
        float fMaxHealth = static_cast<float>(rTargetValues.GetPermanentActorValue(ActorValue::HEALTH));
        float fHealth = static_cast<float>(rTargetValues.GetActorValue(ActorValue::HEALTH));
        float fHealthPercent = 0.0f;
        // Only a maximum above zero (not NaN) divides.
        if (fMaxHealth > 0.0) {
            fHealthPercent = fHealth / fMaxHealth;
        }
        if (pVATSTarget->IsDead(false)) {
            HUDMainMenu::UpdateMeter(
                this->pTiles[26], this->pTiles[28], fHealthPercent - this->fProbableEnemyDamage, fHealthPercent,
                HUDMainMenu::HJ_CENTER, 0
            );
        }
        else {
            HUDMainMenu::UpdateMeter(
                this->pTiles[26], this->pTiles[28], fHealthPercent - this->fProbableEnemyDamage, fHealthPercent,
                HUDMainMenu::HJ_CENTER, 2
            );
        }
        this->pTiles[29]->Set(Tile::kTileValue_String, pVATSTarget->GetFullName(), true);
        this->pTiles[25]->Set(Tile::kTileValue_Visible, 1);
        this->pTiles[26]->Set(Tile::kTileValue_Visible, 1);
        this->pTiles[28]->Set(Tile::kTileValue_Visible, 1);
        this->pTiles[27]->Set(Tile::kTileValue_Visible, 1);
        this->pTiles[29]->Set(Tile::kTileValue_Visible, 1);
        float fDamageThreshold = GetAdjustedDamageThreshold(pPlayer, static_cast<Actor*>(pVATSTarget));
        float fSeeEnemyHealth = 0.0f;
        BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::SEE_ENEMY_HEALTH, pPlayer, &fSeeEnemyHealth);
        // Only a value above zero (not NaN) shows the health and threshold.
        if (fSeeEnemyHealth > 0.0) {
            char acText[260];
            // The arguments are evaluated right to left, as the game does.
            const char* pThresholdLabel = sInventoryDamageThreshold.String();
            int32_t iHealth = static_cast<int32_t>(fHealth);
            const char* pHPLabel = sStatsHP.String();
            const char* pName = pVATSTarget->GetFullName();
            std::sprintf(
                acText, "%s\n %s:%d %s:%.2f", pName, pHPLabel, iHealth, pThresholdLabel,
                static_cast<double>(fDamageThreshold)
            );
            this->pTiles[29]->Set(Tile::kTileValue_String, acText, true);
        }
        else {
            this->pTiles[29]->Set(Tile::kTileValue_String, pVATSTarget->GetFullName(), true);
        }
        if (pPlayer->IsActorPerceived(static_cast<Actor*>(pVATSTarget)) &&
            pPlayer->IsActorPerceived(static_cast<Actor*>(pVATSTarget))->bIsHostile) {
            this->pTiles[29]->Set(Tile::kTileValue_SystemColor, 2);
            this->pTiles[25]->Set(Tile::kTileValue_SystemColor, 2);
            this->pTiles[26]->Set(Tile::kTileValue_SystemColor, 2);
            this->pTiles[28]->Set(Tile::kTileValue_SystemColor, 2);
            this->pTiles[27]->Set(Tile::kTileValue_SystemColor, 2);
        }
        else {
            this->pTiles[29]->Set(Tile::kTileValue_SystemColor, 1);
            this->pTiles[25]->Set(Tile::kTileValue_SystemColor, 1);
            this->pTiles[26]->Set(Tile::kTileValue_SystemColor, 1);
            this->pTiles[28]->Set(Tile::kTileValue_SystemColor, 1);
            this->pTiles[27]->Set(Tile::kTileValue_SystemColor, 1);
        }
    }
    else {
        this->pTiles[25]->Set(Tile::kTileValue_Visible, 0);
        this->pTiles[26]->Set(Tile::kTileValue_Visible, 0);
        this->pTiles[28]->Set(Tile::kTileValue_Visible, 0);
        this->pTiles[27]->Set(Tile::kTileValue_Visible, 0);
        this->pTiles[29]->Set(Tile::kTileValue_Visible, 0);
    }
    ActorValueOwner& rPlayerValues = static_cast<ActorValueOwner&>(*pPlayer);
    float fPlayerHealth = static_cast<float>(BSMAX(0, rPlayerValues.GetPermanentActorValue(ActorValue::HEALTH)));
    // Only an exact zero skips the division. The int is divided by the float and the result stored as a float.
    if (fPlayerHealth != 0.0) {
        fPlayerHealth = static_cast<float>(
            static_cast<double>(BSMAX(0, rPlayerValues.GetActorValue(ActorValue::HEALTH))) / fPlayerHealth
        );
    }
    HUDMainMenu::UpdateMeter(this->pTiles[18], fPlayerHealth, HUDMainMenu::HJ_LEFT, 0, 0);
    HUDMainMenu::UpdateCompass(this->pTiles[23], this->pTiles[20], this->pTiles[21], this->pTiles[22]);
    // The VATS key's bindings: the keyboard key, the mouse button and the joystick button.
    int32_t iKeybind = pControls->GetKeybind(Controls::UACTN_VATSMENU);
    int32_t iKeyboardKey = iKeybind >> 16;
    int32_t iMouseKey = (iKeybind & 0xFF00) >> 8;
    int32_t iJoystickKey = iKeybind & 0xFF;
    if (pVATS.GetMode() != VATS::VATS_PLAYBACK) {
        if (pControls->GetUserAction(Controls::UACTN_VATSMENU, Controls::isPressed) &&
            pVATS.GetMode() != VATS::VATS_TARGET_SELECT) {
            if (!InterfaceManager::Has360Controller() ||
                (iJoystickKey != 10 && iJoystickKey != 11 && iJoystickKey != 17)) {
                pVATS.SetMode(VATS::VATS_TARGET_SELECT, false);
                this->bTargetsLit = false;
            }
        }
        else if (
            !pControls->GetUserAction(Controls::UACTN_VATSMENU, Controls::isHeld) &&
            pVATS.GetMode() == VATS::VATS_TARGET_SELECT
        ) {
            pVATS.SetMode(VATS::VATS_MENU_MODE, false);
            this->bTargetsLit = true;
        }
    }
    pPlayer->UpdateHeadingAndLooking(appTimer.GetSeconds(), false, nullptr, false);
    if ((Interface::Has360Controller() &&
         (InterfaceManager::CheckMenuButton(Interface::eButtonB, true) == 1 ||
          InterfaceManager::CheckMenuButton(Interface::eButtonBack, true) == 1)) ||
        (pControls->GetMouseButton(1, Controls::isPressed) && iMouseKey != 1)) {
        InterfaceManager::Instance();
        if ((!InterfaceManager::Has360Controller() && !this->bMouseOverButton) ||
            (InterfaceManager::Instance(),
             InterfaceManager::Has360Controller() &&
                 InterfaceManager::CheckMenuButton(Interface::eButtonB, true) == 1)) {
            if (pVATS.GetMode() == VATS::VATS_MENU_MODE && this->bHeadingReached) {
                this->UpdateQueuedActionList(nullptr, false, false);
                VATSMenu::pVatsMenu->fProbableEnemyDamage = VATSMenu::pVatsMenu->CalculateProbableEnemyHealth();
            }
            this->SetMeterChance();
        }
    }
    if (!this->PrecalcTargetHitChances()) {
        return;
    }
    ItemChange* pWeaponItem = pPlayer->GetCurrentAIProcess()->GetCurrentWeapon();
    TESObjectWEAP* pWeapon = pWeaponItem ? static_cast<TESObjectWEAP*>(pWeaponItem->GetItemChangeObject()) : nullptr;
    // The cast to BGSAmmoForm is a reference conversion done after the null check. The result is stored as a byte in a
    // uint8_t local.
    uint8_t bRockItLauncher = pWeapon ? static_cast<BGSAmmoForm&>(*pWeapon).IsRockItLauncher() : false;
    this->SetTargetLimb();
    if (pVATS.GetMode() == VATS::VATS_PLAYBACK) {
        return;
    }
    Main::iNumberLOSPicks = 0;
    pPlayer->UpdateAutoAimActor();
    pPlayer->Character::Update(appTimer.GetSeconds());
    pPlayer->UpdateAnimationMovement(pPlayer->GetAnimation(), 0.0f);
    pPlayer->UpdateCamera(false, false);
    pPlayer->UpdateFirstPersonZoom();
    VATSMenu::ShowTargetLimb(false);
    InterfaceManager* pim = InterfaceManager::Instance();
    [[maybe_unused]] int32_t icursorX = pim->GetCursorX();
    [[maybe_unused]] int32_t icursorY = pim->GetCursorY();
    if (!(uiDoIdleStaticsGuard & 1)) {
        uiDoIdleStaticsGuard |= 1;
        iscreenWidth = static_cast<int32_t>(InterfaceManager::GetScreenWidth());
    }
    if (!(uiDoIdleStaticsGuard & 2)) {
        uiDoIdleStaticsGuard |= 2;
        iscreenHeight = static_cast<int32_t>(InterfaceManager::GetScreenHeight());
    }
    if (!(uiDoIdleStaticsGuard & 4)) {
        uiDoIdleStaticsGuard |= 4;
        isafeZoneX = static_cast<int32_t>(InterfaceManager::GetScreenSafeX());
    }
    if (!(uiDoIdleStaticsGuard & 8)) {
        uiDoIdleStaticsGuard |= 8;
        isafeZoneY = static_cast<int32_t>(InterfaceManager::GetScreenSafeY());
    }
    // 0x12: DIK_E. The command list count is compared unsigned.
    if ((pControls->GetKeyboardButton(0x12, Controls::isHeld) && iKeyboardKey != 0x12) ||
        ((InterfaceManager::Instance(), InterfaceManager::Has360Controller()) &&
         InterfaceManager::CheckMenuButton(Interface::eButtonA, true) == 2 &&
         static_cast<uint32_t>(pVATS.GetCommandList()->ItemsInList()) > 0 && pVATS.GetMode() == VATS::VATS_MENU_MODE)) {
        Interface::CloseVATSMenu();
        return;
    }
    DesiredAttackType desiredAttack = DESIRED_ATTACK_NONE;
    if (pVATSTarget) {
        if (IsDesiredAttackControlPressed(DESIRED_ATTACK_NORMAL, false, iMouseKey)) {
            desiredAttack = DESIRED_ATTACK_NORMAL;
            heldAttack = desiredAttack;
        }
        else if (IsDesiredAttackControlPressed(DESIRED_ATTACK_SPECIAL, false, iMouseKey)) {
            desiredAttack = DESIRED_ATTACK_SPECIAL;
            heldAttack = desiredAttack;
        }
        else if (IsDesiredAttackControlPressed(DESIRED_ATTACK_SPECIAL_2, false, iMouseKey)) {
            desiredAttack = DESIRED_ATTACK_SPECIAL_2;
            heldAttack = desiredAttack;
        }
        else if (heldAttack != DESIRED_ATTACK_NONE && !IsDesiredAttackControlPressed(heldAttack, true, iMouseKey)) {
            heldAttack = DESIRED_ATTACK_NONE;
        }
    }
    else {
        heldAttack = DESIRED_ATTACK_NONE;
    }
    bWeaponSpecialSelected = false;
    g_bIsVATSSpecialAttack2 = false;
    // An AP total below the cost (not NaN) clears the button's target trait.
    if (this->pTiles[30]->IsVisible()) {
        if (this->fActionPoints < fSpecialAttackAPCost) {
            this->pTiles[30]->Set(Tile::kTileValue_Target, 0);
        }
        else {
            this->pTiles[30]->Set(Tile::kTileValue_Target, 1);
        }
    }
    else if (desiredAttack == DESIRED_ATTACK_SPECIAL) {
        desiredAttack = DESIRED_ATTACK_NONE;
    }
    if (this->pTiles[31]->IsVisible()) {
        if (this->fActionPoints < fSpecialAttack2APCost) {
            this->pTiles[31]->Set(Tile::kTileValue_Target, 0);
        }
        else {
            this->pTiles[31]->Set(Tile::kTileValue_Target, 1);
        }
    }
    else if (desiredAttack == DESIRED_ATTACK_SPECIAL_2) {
        desiredAttack = DESIRED_ATTACK_NONE;
    }
    float frequiredActionPoints = ActionPoints::GetWeaponAttackActionPoints(pWeapon, true, false);
    float frequiredHeldActionPoints = frequiredActionPoints;
    if (desiredAttack == DESIRED_ATTACK_SPECIAL) {
        frequiredActionPoints = fSpecialAttackAPCost;
    }
    else if (desiredAttack == DESIRED_ATTACK_SPECIAL_2) {
        frequiredActionPoints = fSpecialAttack2APCost;
    }
    if (heldAttack == DESIRED_ATTACK_SPECIAL) {
        frequiredHeldActionPoints = fSpecialAttackAPCost;
    }
    else if (heldAttack == DESIRED_ATTACK_SPECIAL_2) {
        frequiredHeldActionPoints = fSpecialAttack2APCost;
    }
    if (PlayerCharacter::IsGodMode()) {
        frequiredActionPoints = 0.0f;
        frequiredHeldActionPoints = 0.0f;
    }
    this->UpdateAmmo();
    if (!(uiDoIdleStaticsGuard & 0x10)) {
        uiDoIdleStaticsGuard |= 0x10;
        iUT_Brightness = Tile::AddUserTrait("_Brightness", -1);
    }
    // The held attack's cost shows only while the AP total is below it (not NaN).
    if (pVATSTarget && this->fActionPoints < frequiredHeldActionPoints && heldAttack != DESIRED_ATTACK_NONE) {
        HUDMainMenu::UpdateMeter(
            this->pTiles[9], this->pTiles[10], frequiredHeldActionPoints / this->fActionPointsBase, 0.0f,
            HUDMainMenu::HJ_RIGHT, 0
        );
    }
    else {
        float fGhostMeter = this->fActionPoints / this->fActionPointsBase;
        float fMeter = static_cast<float>(
            (static_cast<double>(this->fActionPoints) - frequiredHeldActionPoints) / this->fActionPointsBase
        );
        HUDMainMenu::UpdateMeter(this->pTiles[9], this->pTiles[10], fMeter, fGhostMeter, HUDMainMenu::HJ_RIGHT, 0);
        this->pTiles[12]->Set(Tile::kTileValue_Visible, 0);
    }
    if (!pVATSTarget || desiredAttack == DESIRED_ATTACK_NONE) {
        // 0x1E: DIK_A, 0x20: DIK_D.
        InterfaceManager::Instance();
        if (!InterfaceManager::Has360Controller() && pControls->GetKeyboardButton(0x1E, Controls::isPressed) &&
            pControls->GetKeyboardButton(0x1E, Controls::isChanged) && iKeyboardKey != 0x1E) {
            this->DoClick(2, nullptr);
            return;
        }
        InterfaceManager::Instance();
        if (!InterfaceManager::Has360Controller() && pControls->GetKeyboardButton(0x20, Controls::isPressed) &&
            pControls->GetKeyboardButton(0x20, Controls::isChanged) && iKeyboardKey != 0x20) {
            this->DoClick(3, nullptr);
        }
        return;
    }
    // The flag is stored as a byte.
    uint8_t bnewTarget = !InterfaceManager::Has360Controller() && this->PickNewTarget();
    // A chance of zero or less (or NaN) attacks nothing.
    if (bnewTarget || !(this->pCurrentLimb->fChance > 0.0)) {
        if (!bnewTarget && this->pCurrentLimb->bChanceCalculated && pVATS.GetMode() == VATS::VATS_MENU_MODE &&
            this->bHeadingReached) {
            BSAudioManager* mgr = &BSAudioManager::QInstance();
            BSSoundHandle soundHand;
            // GetSoundHandleByName is handed unconstructed storage for its result, as the game does.
            alignas(BSSoundHandle) uint8_t aucLookup[sizeof(BSSoundHandle)];
            BSSoundHandle* pLookup = static_cast<BSSoundHandle*>(static_cast<void*>(aucLookup));
            BSSoundHandle* psoundHand = mgr->GetSoundHandleByName(
                pLookup, "UIVATSInsufficientAP",
                static_cast<BSGameSound::SoundFlags>(
                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                    BSGameSound::SOUND_FLAG_ONE_SHOT
                )
            );
            soundHand = *psoundHand;
            pLookup->~BSSoundHandle();
            soundHand.Play(false);
            // Only an exact zero chance (not NaN) shows the message.
            if (this->pCurrentLimb->fChance == 0.0) {
                HUDMainMenu::pme->ShowMessage(
                    sVATSMessageZeroChance.String(), HUDMainMenu::HME_SAD, nullptr, nullptr, 2.0f, false
                );
            }
        }
        return;
    }
    InterfaceManager::Instance();
    if (!((InterfaceManager::Has360Controller() && pVATS.GetMode() == VATS::VATS_MENU_MODE) ||
          (InterfaceManager::Instance(),
           !InterfaceManager::Has360Controller() && pVATS.GetMode() == VATS::VATS_MENU_MODE &&
               !(this->bMouseOverButton && desiredAttack == DESIRED_ATTACK_NORMAL)))) {
        InterfaceManager::Instance();
        if (!InterfaceManager::Has360Controller() && pVATS.GetMode() == VATS::VATS_TARGET_SELECT &&
            !this->bMouseOverButton) {
            this->PickNewTarget();
        }
        return;
    }
    ActionPoints::Action eaction =
        ActionPoints::GetAttackAction(static_cast<WEAPON_TYPE>(pWeapon ? pWeapon->GetWeaponType() : 0));
    uint8_t cshotBurst = VATSMenu::GetShotBurstCount(eaction, false);
    // A clip that holds no more than the burst (not NaN) adds the reload's AP.
    if (this->fQueuedClipAmmo <= cshotBurst && pWeapon && pWeapon->GetCurrentAmmo(pPlayer) &&
        pWeapon->GetFormClipRounds(false) > 1) {
        frequiredActionPoints =
            ActionPoints::GetRequiredAPForAction(ActionPoints::ACTION_RELOAD, true) + frequiredActionPoints;
    }
    ItemChange* pammo = pPlayer->GetCurrentAIProcess()->GetCurrentAmmo();
    // The flag is stored as a byte.
    uint8_t bhaveGrenadeAmmo;
    if (!pWeapon) {
        bhaveGrenadeAmmo = false;
    }
    else if (
        pWeapon->GetWeaponType() == WEAPON_ONE_HAND_GRENADE || pWeapon->GetWeaponType() == WEAPON_ONE_HAND_THROWN
    ) {
        if (!pammo) {
            bhaveGrenadeAmmo = false;
        }
        else {
            // The ammo count and the queued commands compare unsigned.
            uint32_t uiCount = static_cast<uint32_t>(pammo->GetCount());
            uint32_t uiQueued = static_cast<uint32_t>(pVATS.GetCommandList()->ItemsInList());
            bhaveGrenadeAmmo = uiCount > uiQueued;
        }
    }
    else {
        bhaveGrenadeAmmo = true;
    }
    // The sums must be above zero and the cost no more than the AP total, NaN failing both; the chance is the
    // truncation of the double product.
    if ((pWeapon &&
         (pWeapon->GetWeaponType() == WEAPON_ONE_HAND_GRENADE || pWeapon->GetWeaponType() == WEAPON_ONE_HAND_THROWN) &&
         !bhaveGrenadeAmmo) ||
        (bRockItLauncher && !(this->fQueuedTotalAmmo + this->fQueuedClipAmmo > 0.0)) ||
        eaction == ActionPoints::ACTION_COUNT || !(frequiredActionPoints <= this->fActionPoints) ||
        (pWeapon && pWeapon->GetCurrentAmmo(pPlayer) && !(this->fQueuedTotalAmmo + this->fQueuedClipAmmo > 0.0)) ||
        (pWeapon && pWeapon->GetCurrentAmmo(pPlayer) && (!pammo || pammo->GetCount() <= 0)) ||
        (this->GetTargetPart() &&
         static_cast<int32_t>(static_cast<double>(this->GetTargetPart()->GetItem()->fChance) * 100.0) <= 0)) {
        if (this->GetTargetPart() &&
            static_cast<int32_t>(static_cast<double>(this->GetTargetPart()->GetItem()->fChance) * 100.0) <= 0 &&
            pVATS.GetMode() == VATS::VATS_MENU_MODE && this->bHeadingReached) {
            BSSoundHandle psoundHand;
            alignas(BSSoundHandle) uint8_t aucLookup[sizeof(BSSoundHandle)];
            BSSoundHandle* pLookup = static_cast<BSSoundHandle*>(static_cast<void*>(aucLookup));
            BSSoundHandle* pFound = BSAudio::QInstance().GetSoundHandleByName(
                pLookup, "UIVATSEnterFail",
                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
            );
            psoundHand = *pFound;
            pLookup->~BSSoundHandle();
            psoundHand.Play(false);
            HUDMainMenu::pme->ShowMessage(
                sVATSMessageZeroChance.String(), HUDMainMenu::HME_SAD, nullptr, nullptr, 2.0f, false
            );
        }
        else if (frequiredActionPoints > this->fActionPoints) {
            // Only a cost above the AP total (not NaN) plays the insufficient AP sound.
            BSAudioManager* mgr = &BSAudioManager::QInstance();
            BSSoundHandle psoundHand;
            alignas(BSSoundHandle) uint8_t aucLookup[sizeof(BSSoundHandle)];
            BSSoundHandle* pLookup = static_cast<BSSoundHandle*>(static_cast<void*>(aucLookup));
            BSSoundHandle* pFound = mgr->GetSoundHandleByName(
                pLookup, "UIVATSInsufficientAP",
                static_cast<BSGameSound::SoundFlags>(
                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                    BSGameSound::SOUND_FLAG_ONE_SHOT
                )
            );
            psoundHand = *pFound;
            pLookup->~BSSoundHandle();
            psoundHand.Play(false);
            this->pTiles[12]->Set(Tile::kTileValue_Visible, 1);
        }
        else if (this->fQueuedTotalAmmo + this->fQueuedClipAmmo == 0.0) {
            // Only an exact zero (not NaN) reports no ammo.
            HUDMainMenu::pme->ShowMessage(
                sVATSMessageNoAmmo.String(), HUDMainMenu::HME_SAD, nullptr, nullptr, 2.0f, false
            );
            BSSoundHandle soundHand;
            alignas(BSSoundHandle) uint8_t aucLookup[sizeof(BSSoundHandle)];
            BSSoundHandle* pLookup = static_cast<BSSoundHandle*>(static_cast<void*>(aucLookup));
            BSSoundHandle* pFound = BSAudio::QInstance().GetSoundHandleByName(
                pLookup, "UIVATSEnterFail",
                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
            );
            soundHand = *pFound;
            pLookup->~BSSoundHandle();
            soundHand.Play(false);
        }
    }
    else {
        void* pMemory = ourMalloc(sizeof(VATS_COMMAND));
        VATS_COMMAND* pcommand = pMemory ? ::new (pMemory) VATS_COMMAND : nullptr;
        Actor* pTargetActor = static_cast<Actor*>(pVATSTarget);
        [[maybe_unused]] BGSBodyPart* pbodyPart = pVATSTarget->IsActor()
            ? pTargetActor->GetActorBase()->GetBodyPartData()->GetBodyPart(
                  static_cast<BGSBodyPart::LIMB_ENUM>(eVATSTargetLimb)
              )
            : nullptr;
        ItemChange* ptargetWeaponItem =
            pVATSTarget->IsActor() ? pTargetActor->GetCurrentAIProcess()->GetCurrentWeapon() : nullptr;
        [[maybe_unused]] TESBoundObject* ptargetWeapon =
            ptargetWeaponItem ? ptargetWeaponItem->GetItemChangeObject() : nullptr;
        if (this->GetTargetPart()) {
            this->pLastLimb = this->GetTargetPart()->GetItem();
        }
        // The flag is stored as a byte.
        uint8_t blimbRandomlySelected = false;
        if (!pWeapon || pWeapon->IsMeleeWeapon()) {
            float fdieRoll = RandomFloat(0.0f, 100.0f);
            int32_t icount = 0;
            // BODY_PART_TYPE_NONE.
            uint32_t uiPart = 0xFFFFFFFF;
            // The parts counted are body part types 0-6; 1 and 2 (the torso and head) are also remembered.
            for (BSSimpleList<VATSBodyPart*>* pparts = &this->GetVatsTarget(pVATSTarget)->GetItem()->kBodyParts;
                 pparts && !pparts->IsEmpty(); pparts = pparts->GetNext()) {
                switch (pparts->GetItem()->eBodyPart) {
                    case 1:
                    case 2:
                        uiPart = pparts->GetItem()->eBodyPart;
                        ++icount;
                        break;
                    case 0:
                    case 3:
                    case 4:
                    case 5:
                    case 6: ++icount; break;
                    default: break;
                }
            }
            if (!pWeapon && desiredAttack == DESIRED_ATTACK_SPECIAL && uiPart != 0xFFFFFFFF) {
                pcommand->eLimb = this->GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(uiPart));
                blimbRandomlySelected = true;
            }
            else if (icount > 0) {
                float frange = static_cast<float>(100.0 / icount);
                // Divides and truncates; a quotient of two floats rounds to no integer in double either.
                int32_t ilimb = static_cast<int32_t>(static_cast<double>(BSMAX(fdieRoll, 1.0f)) / frange);
                icount = 0;
                for (BSSimpleList<VATSBodyPart*>* pparts = &this->GetVatsTarget(pVATSTarget)->GetItem()->kBodyParts;
                     pparts && !pparts->IsEmpty(); pparts = pparts->GetNext()) {
                    // The part type compares signed.
                    int32_t iPart = static_cast<int32_t>(pparts->GetItem()->eBodyPart);
                    if (iPart >= 0 && iPart <= 6) {
                        if (icount == ilimb) {
                            pcommand->eLimb = this->GetActorValueFromBodyPart(
                                static_cast<BGSBodyPart::LIMB_ENUM>(pparts->GetItem()->eBodyPart)
                            );
                            blimbRandomlySelected = true;
                        }
                        ++icount;
                    }
                }
            }
        }
        if (!blimbRandomlySelected) {
            pcommand->eLimb =
                this->GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(this->pCurrentLimb->eBodyPart));
        }
        pcommand->cFireShots = VATSMenu::GetShotBurstCount(eaction, false);
        int32_t iFakeShots = static_cast<int32_t>(VATSMenu::GetShotBurstCount(eaction, true)) - pcommand->cFireShots;
        pcommand->cFakeFireShots = static_cast<uint8_t>(BSMAX(iFakeShots, 0));
        if (desiredAttack == DESIRED_ATTACK_SPECIAL) {
            // Only a VATS AP cost above zero (not NaN) is the weapon's special attack.
            if (pWeapon && pWeapon->GetVATSAP() > 0.0) {
                pcommand->eAction = ActionPoints::ACTION_PLAYER_SPECIAL_WEAPON_ATTACK;
            }
            else if (pVATSTarget->IsKnockedOut()) {
                pcommand->eAction = ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_3;
            }
            else {
                pcommand->eAction = ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK;
            }
        }
        else if (desiredAttack == DESIRED_ATTACK_SPECIAL_2) {
            pcommand->eAction = ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_2;
        }
        else {
            pcommand->eAction = eaction;
        }
        pcommand->pTarget = pVATSTarget;
        pcommand->fActionPoints = frequiredActionPoints;
        // Multiplied by the double 100.0, then stored as a float.
        float fchance = static_cast<float>(fVATSParalyzePalmChance.Float() * 100.0);
        float fdieRoll = RandomFloat(0.0f, 100.0f);
        // A die roll above the chance (or NaN) is no Paralyzing Palm.
        if (bParalyzingPalm && eaction == ActionPoints::ACTION_ATTACK_UNARMED && fchance >= fdieRoll) {
            pcommand->bParalyzingPalm = true;
        }
        if (pVATS.IsMeleeCommand(pcommand) && pVATSTarget->IsActor() && !blimbRandomlySelected) {
            BGSBodyPartData* pbodyPartData = pTargetActor->GetActorBase()->GetBodyPartData();
            // Actor values are passed to GetBodyPart, as the game does.
            if (pbodyPartData->GetBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(ActorValue::PERCEPTION_CONDITION))) {
                pcommand->eLimb = ActorValue::PERCEPTION_CONDITION;
            }
            else if (pbodyPartData->GetBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(ActorValue::ENDURANCE_CONDITION))) {
                pcommand->eLimb = ActorValue::ENDURANCE_CONDITION;
            }
            else {
                BGSBodyPart** parray = pbodyPartData->GetPartArray();
                // The flag is stored as a byte.
                uint8_t bfound = false;
                // The 15 part slots of pPartArray.
                for (int32_t i = 0; i < 15 && !bfound; ++i) {
                    if (parray[i]) {
                        // The part's actor value is a signed char.
                        pcommand->eLimb =
                            static_cast<ActorValue::Index>(static_cast<int8_t>(parray[i]->GetActorValue()));
                        bfound = true;
                    }
                }
            }
        }
        pVATS.QueueAction(pcommand);
        float ffinalChance = this->GetTargetPart() ? this->GetTargetPart()->GetItem()->fChance : 0.0f;
        if (this->bConcentratedFirePerk) {
            int32_t iconsShots = this->GetConsecutiveShots();
            int32_t iBonus = static_cast<int32_t>(iVATSConcentratedFireBonus.UInt()) * iconsShots;
            // The bonus is divided by the double 100.0 and added to the float in double precision, then stored as a
            // float.
            ffinalChance = static_cast<float>(static_cast<double>(iBonus) / 100.0 + ffinalChance);
        }
        uint8_t bShouldHit;
        if (bVatsAlwaysHit.Bool() && this->GetTargetPart()) {
            bShouldHit = true;
        }
        else {
            bShouldHit = RandomBool(ffinalChance);
        }
        pcommand->bSuccess = bShouldHit;
        this->PrepareMeters();
        bool bForcedReload = false;
        if (this->UpdateAmmo() && pWeapon && pWeapon->GetWeaponType() != WEAPON_ONE_HAND_GRENADE &&
            pWeapon->GetWeaponType() != WEAPON_ONE_HAND_THROWN) {
            int32_t iClipRounds = pWeapon->GetFormClipRounds(false);
            // Only queued ammo above zero (not NaN) forces the reload.
            bForcedReload = iClipRounds != static_cast<int32_t>(pWeapon->GetAmmoPerShot()) &&
                this->fQueuedTotalAmmo + this->fQueuedClipAmmo > 0.0;
        }
        if (bForcedReload) {
            this->UpdateQueuedActionList(pcommand, true, desiredAttack != DESIRED_ATTACK_NORMAL);
        }
        else {
            this->UpdateQueuedActionList(pcommand, false, desiredAttack != DESIRED_ATTACK_NORMAL);
        }
        // Only a cost above the AP total (not NaN) clears the attack tile's target trait.
        if (frequiredActionPoints > this->fActionPoints) {
            this->pTiles[7]->Set(Tile::kTileValue_Target, 0);
        }
        this->SetMeterChance();
    }
    VATSMenu::pVatsMenu->fProbableEnemyDamage = VATSMenu::pVatsMenu->CalculateProbableEnemyHealth();
}

bool VATSMenu::DoKey(int32_t a1) {
    if (a1 != 66 && a1 != 98) {
        return false;
    }
    if (pVATS.GetMode() == VATS::VATS_MENU_MODE && bHeadingReached) {
        UpdateQueuedActionList(nullptr, false, false);
    }
    return true;
}

int32_t VATSMenu::GetClass() {
    return 1056;
}

void VATSMenu::UpdateQueuedActionList(VATS_COMMAND* apCommand, bool abForcedReload, bool bSpecialAttack) {
    const char* v17;
    TESObjectWEAP* pweapon;
    BaseProcess* v22;
    BSSoundHandle* SoundHandleByName;
    XFNV_INLINED_LOCAL(BSSoundHandle, apHandle, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    BSSimpleList<VATSTarget*>* ptargetList;
    XFNV_INLINED_LOCAL(BSSoundHandle, soundHand, std::nullopt, std::nullopt);
    char Dst[268];
    ItemChange* pweaponItem_2;
    char apDest[264];
    Tile* ptile;
    BSAudioManager* mgr;
    if (!apCommand) {
        float frequiredPoints;
        pLastLimb = nullptr;
        if (pVATS.GetLastAction()) {
            frequiredPoints = pVATS.GetLastAction()->fActionPoints;
        }
        else {
            frequiredPoints = 0.0f;
        }
        ActionPoints::Action eaction = ActionPoints::ACTION_COUNT;
        bool bironSights = false;
        pVATS.RemoveLastAction(eaction, bironSights);
        PrepareMeters();
        if (eaction == ActionPoints::ACTION_COUNT && pVATS.eMode == VATS::VATS_MENU_MODE) {
            Interface::CloseVATSMenu();
        }
        if (!PlayerCharacter::IsGodMode()) {
            TESBoundObject* FormFlags;
            BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
            ItemChange* pweaponItem = CurrentAIProcess->GetCurrentWeapon();
            if (pweaponItem) {
                FormFlags = pweaponItem->pContainerObj;
            }
            else {
                FormFlags = nullptr;
            }
            fActionPoints = fActionPoints + frequiredPoints;
        }
        if (fActionPointsBase < static_cast<double>(fActionPoints)) {
            fActionPoints = fActionPointsBase;
        }
        pTiles[7]->Set(Tile::kTileValue_Target, 1);
        if (eaction != ActionPoints::ACTION_RELOAD) {
            Tile* plastAction = lbActions.GetLastItem();
            if (plastAction) {
                const char* String = plastAction->GetString(Tile::kTileValue_String);
                const char* ActionPointsString = ActionPoints::GetActionPointsString(ActionPoints::ACTION_RELOAD);
                if (!BSstrcmp(ActionPointsString, String)) {
                    lbActions.RemoveAt(lbActions.NumItems() - 1, false);
                }
            }
        }
        lbActions.RemoveAt(lbActions.NumItems() - 1, false);
        BSSoundHandle psoundHand;
        BSAudio* v5 = &BSAudio::QInstance();
        BSSoundHandle* arHandle = v5->GetSoundHandleByName(&a1, "UIMenuCancel", 289u);
        psoundHand.operator=(*arHandle);
        psoundHand.Play(false);
        goto LABEL_58;
    }
    mgr = &BSAudioManager::QInstance();
    ::new (&soundHand) BSSoundHandle;
    SoundHandleByName = mgr->GetSoundHandleByName(&apHandle, "UIVATSMove", 289u);
    soundHand.operator=(*SoundHandleByName);
    soundHand.Play(false);
    ptargetList = VATSMenu::pVatsMenu->GetVatsTarget(pVATSTarget);
    v22 = pPlayer->pCurrentProcess;
    pweaponItem_2 = v22->GetCurrentWeapon();
    if (pweaponItem_2) {
        pweapon = static_cast<TESObjectWEAP*>(pweaponItem_2->pContainerObj);
    }
    else {
        pweapon = nullptr;
    }
    fActionPoints = fActionPoints - apCommand->fActionPoints;
    if (ActionPoints::IsAttackAction(apCommand->eAction)) {
        ++VATSMenu::uiActionIndex;
        if (eVATSTargetLimb == ActorValue::VALUE_NONE ||
            (!pVATSTarget->IsActor() ||
             (!pweapon || (pweapon->IsMeleeWeapon() || pweapon->GetWeaponType() == WEAPON_ONE_HAND_GRENADE)))) {
            BaseProcess* ptargetProcess;
            NiNode* pweaponBone;
            // Reads the target's process whatever the target's type, as the game does, then the weapon bone of its
            // biped.
            if (ptargetList->GetItem()->eType == 2 ||
                (ptargetProcess = static_cast<MobileObject*>(pVATSTarget)->pCurrentProcess,
                 (pweaponBone = ptargetProcess->GetWeaponBone(pVATSTarget->GetBiped())) == nullptr) ||
                (!pweapon || (pweapon->IsMeleeWeapon() || pweapon->GetWeaponType() == WEAPON_ONE_HAND_GRENADE))) {
                const char* FullName = apCommand->pTarget->GetFullName();
                BSstrcpy(Dst, 260, FullName);
                goto LABEL_43;
            }
            const char* v9 = apCommand->pTarget->GetFullName();
            BSstrcpy(Dst, 260, v9);
            BSstrcat(Dst, 260, ": ");
            if ((&pweaponBone->m_kName)->GetHandleString()) {
                const char* MagicItemCastName = MagicSystem::GetMagicItemCastName(MagicSystem::WHEN_STRIKES);
                BSstrcat(Dst, 260, MagicItemCastName);
                goto LABEL_43;
            }
        }
        else {
            ActorValue::Index ActorValueFromBodyPart;
            const char* v7;
            BGSBodyPart* pbodyPart;
            BGSBodyPartData* pbodyPartData;
            // The target is treated as an actor here because IsActor was tested above.
            pbodyPartData = static_cast<Actor*>(pVATSTarget)->GetActorBase()->GetBodyPartData();
            // eBodyPart holds the body part data's LIMB_ENUM.
            ActorValueFromBodyPart =
                GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pCurrentLimb->eBodyPart));
            pbodyPart = pbodyPartData->GetBodyPart_005E5130(ActorValueFromBodyPart);
            if (!pbodyPart) {
                return;
            }
            v7 = apCommand->pTarget->GetFullName();
            BSstrcpy(Dst, 260, v7);
            BSstrcat(Dst, 260, ": ");
            if (pbodyPart->GetPartName()) {
                BSstrcat(Dst, 260, pbodyPart->GetPartName());
            LABEL_43:;
                if (bSpecialAttack) {
                    if (pweapon && pweapon->GetWeaponSkill() == ActorValue::MELEE_WEAPONS) {
                        BSstrcat(Dst, 260, ": ");
                        const char* VATSAttackName = pweapon->GetVATSAttackName();
                        BSstrcat(Dst, 260, VATSAttackName);
                    }
                    else {
                        switch (apCommand->eAction) {
                            case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK: {
                                BSstrcat(Dst, 260, ": ");
                                const char* v13 = sVatsUnarmedAttack1.String();
                                BSstrcat(Dst, 260, v13);
                                break;
                            }
                            case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_3: {
                                BSstrcat(Dst, 260, ": ");
                                const char* v14 = sVatsUnarmedAttackGround.String();
                                BSstrcat(Dst, 260, v14);
                                break;
                            }
                            case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_2: {
                                BSstrcat(Dst, 260, ": ");
                                const char* v15 = sVatsUnarmedAttack2.String();
                                BSstrcat(Dst, 260, v15);
                                break;
                            }
                        }
                    }
                }
                if (abForcedReload) {
                    char Src[264];
                    BSstrcpy(Src, 260, Dst);
                    ptile = lbActions.AddItem(VATSMenu::uiActionIndex, nullptr, nullptr, nullptr);
                    ptile->Set(Tile::kTileValue_String, Src, true);
                    ++VATSMenu::uiActionIndex;
                    const char* v16 = ActionPoints::GetActionPointsString(ActionPoints::ACTION_RELOAD);
                    BSstrcpy(Dst, 260, v16);
                }
                goto LABEL_57;
            }
        }
        BSstrcat(Dst, 260, "---");
        goto LABEL_43;
    }
    ++VATSMenu::uiActionIndex;
    v17 = ActionPoints::GetActionPointsString(apCommand->eAction);
    BSstrcpy(Dst, 260, v17);
LABEL_57:;
    BSstrcpy(apDest, 260, Dst);
    ptile = lbActions.AddItem(VATSMenu::uiActionIndex, nullptr, nullptr, nullptr);
    ptile->Set(Tile::kTileValue_String, apDest, true);
LABEL_58:;
    if (lbActions.NumItems()) {
        pTiles[4]->Set(Tile::kTileValue_Target, 1);
    }
    else {
        pTiles[4]->Set(Tile::kTileValue_Target, 0);
    }
}

bool VATSMenu::IsWellFormedMenu(int32_t& ai) {
    for (*(&ai) = 1; (*(&ai)) <= 31; ++(*(&ai))) {
        if (!pTiles[*(&ai)]) {
            return false;
        }
    }
    return true;
}

BSSimpleList<VATSTarget*>* VATSMenu::GetVatsTarget(TESObjectREFR* apTarget) {
    if (!apTarget) {
        return nullptr;
    }
    BSSimpleList<VATSTarget*>* pIter = &xTargetList;
    while (pIter && !pIter->IsEmpty()) {
        if (pIter->GetItem()->pTargetRef == apTarget) {
            return pIter;
        }
        pIter = pIter->GetNext();
    }
    return nullptr;
}

// The counts are floats: every int count is converted to float before it is stored.
bool VATSMenu::UpdateAmmo() {
    char acBuffer[16];
    ItemChange* pWeaponItem = pPlayer->GetCurrentAIProcess()->GetCurrentWeapon();
    ItemChange* pAmmoItem = pPlayer->GetCurrentAIProcess()->GetCurrentAmmo();
    float fClip = 0.0f;
    float fTotal = 0.0f;
    this->fTotalAmmo = 0.0f;
    this->fClipAmmo = 0.0f;
    bool bForcedReload = false;
    if (!pWeaponItem) {
        this->pTiles[14]->Set(Tile::kTileValue_Visible, 0);
        this->pTiles[15]->Set(Tile::kTileValue_Visible, 0);
        this->pTiles[16]->Set(Tile::kTileValue_String, 0);
        this->pTiles[16]->Set(Tile::kTileValue_Visible, 0);
        this->pTiles[13]->Set(Tile::kTileValue_Visible, 0);
        this->fQueuedClipAmmo = 0.0f;
        this->fQueuedTotalAmmo = 0.0f;
    }
    else {
        TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(pWeaponItem->GetContainerObject());
        // The BGSAmmoForm base is reached without a null check, as a reference conversion, as the game does.
        bool bRockItLauncher = static_cast<BGSAmmoForm&>(*pWeapon).IsRockItLauncher();
        float fRockItAmmo = (float)pPlayer->GetNumRockItAmmo();
        bool bTotalFromCount = true;
        int32_t iWeaponType = pWeapon->GetWeaponType();
        if (iWeaponType >= WEAPON_ONE_HAND_GRENADE &&
            (iWeaponType <= WEAPON_ONE_HAND_MINE || iWeaponType == WEAPON_ONE_HAND_THROWN)) {
            fClip = (float)pWeapon->GetAmmoPerShot();
            InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(pPlayer);
            int32_t iCount = pChanges->GetObjectCount(pWeapon);
            // The difference is computed exactly and rounded once.
            fTotal = (float)((double)iCount - fClip);
        }
        else if (bRockItLauncher) {
            fTotal = 0.0f;
            fClip = fRockItAmmo;
        }
        else {
            InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(pPlayer);
            TESForm* pAmmo = pWeapon->GetCurrentAmmo(pPlayer);
            int32_t iCount = pChanges->GetObjectCount(static_cast<TESBoundObject*>(pAmmo));
            bool bRegen = pWeaponItem->HasModEffectActive(WEAPON_MOD_AMMO_REGEN_SECONDS);
            // Only a rate above zero (not NaN) takes the clip size as the total.
            if (pWeapon->GetAmmoRegenRate(bRegen) > 0.0) {
                bool bClipMod = pWeaponItem->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE);
                fTotal = (float)pWeapon->GetFormClipRounds(bClipMod);
                bTotalFromCount = false;
            }
            int32_t iClip;
            if (pAmmoItem) {
                iClip = pAmmoItem->GetCount();
            }
            else {
                bool bClipMod = pWeaponItem->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE);
                if (iCount <= pWeapon->GetFormClipRounds(bClipMod)) {
                    iClip = iCount;
                }
                else {
                    bool bClipMod2 = pWeaponItem->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE);
                    iClip = pWeapon->GetFormClipRounds(bClipMod2);
                }
            }
            fClip = (float)iClip;
            if (bTotalFromCount) {
                fTotal = (float)((double)iCount - fClip);
            }
        }
        this->fClipAmmo = fClip;
        this->fQueuedClipAmmo = fClip;
        this->fTotalAmmo = fTotal;
        this->fQueuedTotalAmmo = fTotal;
        for (BSSimpleList<VATS_COMMAND*>* pIter = pVATS.GetCommandList(); pIter && !pIter->IsEmpty();
             pIter = pIter->GetNext()) {
            switch (pIter->GetItem()->eAction) {
                case ActionPoints::ACTION_ATTACK_PISTOL:
                case ActionPoints::ACTION_ATTACK_RIFLE:
                case ActionPoints::ACTION_ATTACK_HANDLE:
                case ActionPoints::ACTION_ATTACK_LAUNCHER: {
                    float fLeft = this->fQueuedClipAmmo - (float)(int32_t)pIter->GetItem()->cFireShots;
                    this->fQueuedClipAmmo = BSMAX(0.0f, fLeft);
                    // Only an exact zero (not NaN) reloads.
                    if (this->fQueuedClipAmmo == 0.0) {
                        float fClipSize;
                        if (bRockItLauncher) {
                            fClipSize = fRockItAmmo;
                        }
                        else {
                            bool bClipMod = pWeaponItem->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE);
                            fClipSize = (float)pWeapon->GetFormClipRounds(bClipMod);
                        }
                        this->fQueuedClipAmmo = BSMIN(this->fQueuedTotalAmmo, fClipSize);
                        float fClipSize2;
                        if (bRockItLauncher) {
                            fClipSize2 = fRockItAmmo;
                        }
                        else {
                            bool bClipMod = pWeaponItem->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE);
                            fClipSize2 = (float)pWeapon->GetFormClipRounds(bClipMod);
                        }
                        this->fQueuedTotalAmmo = this->fQueuedTotalAmmo - BSMIN(this->fTotalAmmo, fClipSize2);
                        this->fQueuedTotalAmmo = BSMAX(0.0f, this->fQueuedTotalAmmo);
                        bForcedReload = true;
                    }
                    else {
                        bForcedReload = false;
                    }
                    break;
                }
                case ActionPoints::ACTION_ATTACK_GRENADE:
                case ActionPoints::ACTION_ATTACK_MINE: {
                    float fLeft = (float)(this->fQueuedClipAmmo - 1.0);
                    this->fQueuedClipAmmo = BSMAX(0.0f, fLeft);
                    break;
                }
                case ActionPoints::ACTION_RELOAD: {
                    float fClipSize;
                    if (bRockItLauncher) {
                        fClipSize = fRockItAmmo;
                    }
                    else {
                        bool bClipMod = pWeaponItem->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE);
                        fClipSize = (float)pWeapon->GetFormClipRounds(bClipMod);
                    }
                    this->fQueuedClipAmmo = BSMIN(this->fTotalAmmo, fClipSize);
                    float fClipSize2;
                    if (bRockItLauncher) {
                        fClipSize2 = fRockItAmmo;
                    }
                    else {
                        bool bClipMod = pWeaponItem->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE);
                        fClipSize2 = (float)pWeapon->GetFormClipRounds(bClipMod);
                    }
                    this->fQueuedTotalAmmo = this->fQueuedTotalAmmo - BSMIN(this->fTotalAmmo, fClipSize2);
                    break;
                }
                default: break;
            }
        }
        if (pWeapon->GetCurrentAmmo(pPlayer)) {
            this->pTiles[14]->Set(Tile::kTileValue_Visible, 1);
            this->pTiles[15]->Set(Tile::kTileValue_String, 1);
            this->pTiles[16]->Set(Tile::kTileValue_String, 1);
            this->pTiles[16]->Set(Tile::kTileValue_Visible, 1);
            uint32_t uiHealth;
            if (TESHealthForm::GetFormHealth(pWeaponItem->GetContainerObject())) {
                uiHealth = TESHealthForm::GetFormHealth(pWeaponItem->GetContainerObject());
            }
            else {
                uiHealth = 1;
            }
            // Divides the health by the maximum, converted as an unsigned value, in double precision and stores the
            // result as a float.
            float fRatio = (float)(pWeaponItem->GetItemHealth(false) / (double)uiHealth);
            float fMeter = BSMIN(1.0f, BSMAX(0.0f, fRatio));
            HUDMainMenu::UpdateMeter(this->pTiles[15], fMeter, HUDMainMenu::HJ_LEFT, 0, 0);
            this->pTiles[13]->Set(Tile::kTileValue_Visible, 1);
        }
        else {
            this->pTiles[14]->Set(Tile::kTileValue_Visible, 0);
            this->pTiles[15]->Set(Tile::kTileValue_Visible, 0);
            this->pTiles[16]->Set(Tile::kTileValue_String, 0);
            this->pTiles[16]->Set(Tile::kTileValue_Visible, 0);
            this->pTiles[13]->Set(Tile::kTileValue_Visible, 0);
            if (pWeapon->GetWeaponType() == WEAPON_ONE_HAND_GRENADE ||
                pWeapon->GetWeaponType() == WEAPON_ONE_HAND_MINE ||
                pWeapon->GetWeaponType() == WEAPON_ONE_HAND_THROWN) {
                this->pTiles[13]->Set(Tile::kTileValue_Visible, 1);
            }
        }
    }
    BSsprintf(acBuffer, 0x10, "%i/%i", (int32_t)this->fQueuedClipAmmo, (int32_t)this->fQueuedTotalAmmo);
    this->pTiles[13]->Set(Tile::kTileValue_String, acBuffer, true);
    return bForcedReload;
}

ActorValue::Index VATSMenu::GetActorValueFromBodyPart(BGSBodyPart::LIMB_ENUM aeBodyPart) {
    if (!pVATSTarget || !pVATSTarget->IsActor()) {
        return ActorValue::ENDURANCE_CONDITION;
    }
    TESActorBase* pActorBase = static_cast<Actor*>(pVATSTarget)->GetActorBase();
    BGSBodyPartData* pBodyPartData = pActorBase->GetBodyPartData();
    if (pBodyPartData) {
        BGSBodyPart* pBodyPart = pBodyPartData->GetBodyPart(aeBodyPart);
        if (pBodyPart) {
            return static_cast<ActorValue::Index>(pBodyPart->GetActorValue());
        }
    }
    if (aeBodyPart == BGSBodyPart::LIMB_WEAPON) {
        return static_cast<ActorValue::Index>(-1);
    }
    return ActorValue::ENDURANCE_CONDITION;
}

void VATSMenu::UpdateChanceToHit() {
    static int32_t iUT_MeterPercent = Tile::AddUserTrait("_MeterPercent", -1);
    static int32_t iUT_LimbName = Tile::AddUserTrait("_LimbName", -1);
    static int32_t iUT_LimbNameVisible = Tile::AddUserTrait("_LimbNameVisible", -1);
    static int32_t iUT_Brightness_1 = Tile::AddUserTrait("_Brightness", -1);
    BSSimpleList<VATSTarget*>* ptargetList = GetVatsTarget(pVATSTarget);
    BSSimpleList<VATSBodyPart*>* ppartsList = &ptargetList->GetItem()->kBodyParts;
    if (pVATS.eMode == VATS::VATS_TARGET_SELECT || pVATS.eMode == VATS::VATS_SCAN_MODE ||
        (pVATS.eMode == VATS::VATS_MENU_MODE && !bHeadingReached)) {
        // Outside targeting: stop every meter's fade and hide it.
        while (ppartsList && !ppartsList->IsEmpty()) {
            if (ppartsList->GetItem()->pMeter->GetFloat(Tile::kTileValue_Alpha) > 0.0) {
                ppartsList->GetItem()->pMeter->RemoveFadeControl(Tile::kTileValue_Alpha);
                ppartsList->GetItem()->pMeter->Set(Tile::kTileValue_Alpha, 0.0f, true);
            }
            ppartsList = ppartsList->GetNext();
        }
        bMetersShown = false;
    }
    else if (bMetersShown) {
        ItemChange* pweaponItem = pPlayer->GetCurrentAIProcess()->GetCurrentWeapon();
        TESObjectWEAP* pweapon;
        if (pweaponItem) {
            pweapon = static_cast<TESObjectWEAP*>(pweaponItem->pContainerObj);
        }
        else {
            pweapon = nullptr;
        }
        bool bNoRescan = false;
        if (ptargetList->GetItem()->eType == 2 || pPlayer->GetCurrentAIProcess()->IsCurrentWeaponGrenade() ||
            !pweapon || pweapon->IsMeleeWeapon()) {
            bNoRescan = true;
        }
        // The torso's node of the part list, once the scan below has passed it.
        BSSimpleList<VATSBodyPart*>* ptorso = nullptr;
        while (!bNoRescan && ppartsList && !ppartsList->IsEmpty()) {
            if (LimbAppeared() ||
                (!ppartsList->GetItem()->bChanceCalculated && ppartsList->GetItem()->bOnScreen &&
                 ppartsList->GetItem()->bNeedsRecalc)) {
                VATSMenu::bIsARescan = true;
                ptargetList->GetItem()->bChancesCalculated = false;
                if (pVATS.eMode != VATS::VATS_SCAN_MODE) {
                    pVATS.SetMode(VATS::VATS_SCAN_MODE, false);
                    g_vatsLagTimer = appTimer.iLastTime;
                }
                return;
            }
            if (ppartsList->GetItem()->eBodyPart == 1) {
                ptorso = ppartsList;
            }
            ppartsList = ppartsList->GetNext();
        }
        for (ppartsList = &ptargetList->GetItem()->kBodyParts; ppartsList && !ppartsList->IsEmpty();
             ppartsList = ppartsList->GetNext()) {
            float falpha = ppartsList->GetItem()->pMeter->GetFloat(Tile::kTileValue_Alpha);
            if (ppartsList->GetItem()->bOnScreen) {
                ppartsList->GetItem()->bNeedsRecalc = false;
                ItemChange* pweaponItem_2 = pPlayer->GetCurrentAIProcess()->GetCurrentWeapon();
                TESObjectWEAP* pweapon_2;
                if (pweaponItem_2) {
                    pweapon_2 = static_cast<TESObjectWEAP*>(pweaponItem_2->pContainerObj);
                }
                else {
                    pweapon_2 = nullptr;
                }
                if (pPlayer->GetCurrentAIProcess()->IsCurrentWeaponGrenade() || !pweapon_2 ||
                    pweapon_2->IsMeleeWeapon()) {
                    // No limb targeting: flash the torso, else the last part.
                    if (((ptorso && ppartsList == ptorso) || (!ptorso && !ppartsList->GetNext())) &&
                        ppartsList->GetItem()->pMeter->GetFadeFinished(Tile::kTileValue_Alpha)) {
                        ppartsList->GetItem()->pMeter->AddFadeControl(
                            Tile::kTileValue_Alpha, 0.0f, 255.0f, 1.0f, Tile::TFCT_NONLINEAR_LONG_DARK_REPEATING
                        );
                    }
                }
                else if (
                    pCurrentLimb == ppartsList->GetItem() ||
                    GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(ppartsList->GetItem()->eBodyPart)) ==
                        GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pCurrentLimb->eBodyPart))
                ) {
                    if (ppartsList->GetItem()->pMeter->GetFadeFinished(Tile::kTileValue_Alpha)) {
                        ppartsList->GetItem()->pMeter->AddFadeControl(
                            Tile::kTileValue_Alpha, 0.0f, 255.0f, 1.0f, Tile::TFCT_NONLINEAR_LONG_DARK_REPEATING
                        );
                    }
                }
                else if (
                    (falpha == ppartsList->GetItem()->pMeter->GetFadeEndFor(Tile::kTileValue_Alpha) &&
                     falpha != 255.0) ||
                    ppartsList->GetItem()->pMeter->GetFadeType(Tile::kTileValue_Alpha) ==
                        Tile::TFCT_NONLINEAR_LONG_DARK_REPEATING
                ) {
                    ppartsList->GetItem()->pMeter->AddFadeControl(
                        Tile::kTileValue_Alpha, falpha, 255.0f, 0.2f, Tile::TFCT_STANDARD
                    );
                }
            }
            else if (ppartsList->GetItem()->pMeter->GetFadeEndFor(Tile::kTileValue_Alpha) > 0.0) {
                float afStart = ppartsList->GetItem()->pMeter->GetFloat(Tile::kTileValue_Alpha);
                ppartsList->GetItem()->pMeter->AddFadeControl(
                    Tile::kTileValue_Alpha, afStart, 0.0f, 0.5f, Tile::TFCT_STANDARD
                );
            }
            ActorValue::Index eCurrentLimbAV = VATSMenu::pVatsMenu->GetActorValueFromBodyPart(
                static_cast<BGSBodyPart::LIMB_ENUM>(VATSMenu::pVatsMenu->pCurrentLimb->eBodyPart)
            );
            ActorValue::Index ePartAV = VATSMenu::pVatsMenu->GetActorValueFromBodyPart(
                static_cast<BGSBodyPart::LIMB_ENUM>(ppartsList->GetItem()->eBodyPart)
            );
            ppartsList->GetItem()->pMeter->Set(iUT_LimbNameVisible, static_cast<int32_t>(eCurrentLimbAV == ePartAV));
        }
    }
    else {
        bMetersShown = true;
        PrepareMeters();
    }
}

// Places each body part meter of the current target: its screen position (the part's node projected through the world
// camera), its name and condition, whether it is on screen, its tile position beside the part, then pushes overlapping
// meters apart (at most 31 passes). VATSBodyPart::eBodyPart is kept uint32_t for its other users, so the calls taking
// the enum convert it.
void VATSMenu::PrepareMeters() {
    BSSimpleList<VATSTarget*>* ptargetList = GetVatsTarget(pVATSTarget);
    VATSTarget* ptargetListItem = ptargetList->m_item;
    BSSimpleList<VATSBodyPart*>* ppartsList = &ptargetListItem->kBodyParts;
    NiAVObject* ptarget = nullptr;
    NiPoint3 targetPoint;
    int32_t ilocX = 0;
    int32_t ilocY = 0;
    if ((PrepareMeters_guards & 1) == 0) {
        PrepareMeters_guards |= 1u;
        VATSMenu::iScreenCenterX = static_cast<int32_t>(InterfaceManager::GetScreenWidth() / 2.0);
    }
    if ((PrepareMeters_guards & 2) == 0) {
        PrepareMeters_guards |= 2u;
        VATSMenu::iScreenCenterY = static_cast<int32_t>(InterfaceManager::GetScreenHeight() / 2.0);
    }
    if ((PrepareMeters_guards & 4) == 0) {
        PrepareMeters_guards |= 4u;
        double dScreenSafeX = InterfaceManager::GetScreenSafeX();
        VATSMenu::iMinX = static_cast<int32_t>(dScreenSafeX + dScreenSafeX + 100.0);
    }
    if ((PrepareMeters_guards & 8) == 0) {
        PrepareMeters_guards |= 8u;
        double dScreenSafeY = InterfaceManager::GetScreenSafeY();
        VATSMenu::iMinY = static_cast<int32_t>(dScreenSafeY + dScreenSafeY + 100.0);
    }
    if ((PrepareMeters_guards & 16) == 0) {
        PrepareMeters_guards |= 16u;
        VATSMenu::iMaxX =
            static_cast<int32_t>(InterfaceManager::GetScreenWidth() - static_cast<double>(VATSMenu::iMinX) - 50.0);
    }
    if ((PrepareMeters_guards & 32) == 0) {
        PrepareMeters_guards |= 32u;
        double dScreenHeight = InterfaceManager::GetScreenHeight();
        double dSafeHeight = dScreenHeight - InterfaceManager::GetScreenSafeY() * 2.0;
        VATSMenu::iMaxY = static_cast<int32_t>(
            dSafeHeight - VATSMenu::pVatsMenu->pTiles[17]->GetFloat(Tile::kTileValue_Height) + 50.0
        );
    }
    ItemChange* pweaponItem = pPlayer->pCurrentProcess->GetCurrentWeapon();
    TESObjectWEAP* pweapon = pweaponItem ? static_cast<TESObjectWEAP*>(pweaponItem->pContainerObj) : nullptr;
    for (ppartsList = &ptargetListItem->kBodyParts; ppartsList && !ppartsList->IsEmpty();
         ppartsList = ppartsList->m_pkNext) {
        BGSBodyPartData* pbodyPartData = nullptr;
        BGSBodyPart* pbodyPart = nullptr;
        VATSBodyPart* ppartsListItem = ppartsList->m_item;
        if (ptargetListItem->eType == 2) {
            ptarget = ptargetListItem->pTargetRef->Get3D();
        }
        else if (static_cast<BGSBodyPart::LIMB_ENUM>(ppartsListItem->eBodyPart) == BGSBodyPart::LIMB_WEAPON) {
            NiNode* pweaponBone = nullptr;
            if (pVATSTarget->IsActor()) {
                BaseProcess* ptargetProcess = static_cast<Actor*>(pVATSTarget)->pCurrentProcess;
                pweaponBone = ptargetProcess->GetWeaponBone(pVATSTarget->GetBiped());
            }
            ptarget = pweaponBone;
        }
        else {
            pbodyPartData = static_cast<Actor*>(pVATSTarget)->GetActorBase()->GetBodyPartData();
            pbodyPart = pbodyPartData->GetBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(ppartsListItem->eBodyPart));
            if (pbodyPart) {
                const char* pTargetName = pbodyPart->GetTargetName();
                ptarget = BSUtilities::GetObjectByName(pVATSTarget->Get3D(), pTargetName);
            }
        }
        if (ptarget) {
            targetPoint = ptarget->GetWorldTranslate();
            ppartsListItem->kPos = targetPoint;
            Main::GetWorldSceneGraph()->GetCamera()->WorldPtToScreenPt(
                targetPoint, ppartsListItem->kScreenPos.x, ppartsListItem->kScreenPos.y, 1e-05f
            );
        }
        ppartsListItem->kScreenPos.x = InterfaceManager::GetScreenWidth() * ppartsListItem->kScreenPos.x;
        double dInvY = 1.0 - ppartsListItem->kScreenPos.y;
        ppartsListItem->kScreenPos.y = InterfaceManager::GetScreenHeight() * dInvY;
        NiMatrix3 projectionMatrix;
        GetScreenPlaneProjectionMatrix(&projectionMatrix);
        ppartsListItem->kRelativePos = projectionMatrix * ppartsListItem->kPos;
        ItemChange* ptargetWeaponItem = nullptr;
        if (pVATSTarget->IsActor()) {
            ptargetWeaponItem = static_cast<Actor*>(pVATSTarget)->pCurrentProcess->GetCurrentWeapon();
        }
        // The condition is computed and stored but never read, as the game does.
        [[maybe_unused]] float fweaponCondition =
            ptargetWeaponItem ? ptargetWeaponItem->GetItemHealth(true) / 100.0 : 1.0;
        TESObjectWEAP* ptargetWeapon =
            ptargetWeaponItem ? static_cast<TESObjectWEAP*>(ptargetWeaponItem->pContainerObj) : nullptr;
        bDataInTemporaryVATSCommand = 0;
        // Each value is computed before its trait id is added, matching the game's order of argument evaluation.
        if (ptargetListItem->eType == 2 || pPlayer->pCurrentProcess->IsCurrentWeaponGrenade() || !pweapon ||
            pweapon->IsMeleeWeapon()) {
            const char* pName = ptargetListItem->pTargetRef->GetFullName();
            ppartsListItem->pMeter->Set(Tile::AddUserTrait("_LimbName", -1), pName, true);
            float fHealth = ptargetListItem->pTargetRef->GetHealth() / 100.0;
            float fPercent = BSMIN(1.0f, BSMAX(0.0f, fHealth));
            ppartsListItem->pMeter->Set(Tile::AddUserTrait("_MeterPercent", -1), fPercent, true);
        }
        else if (pbodyPart) {
            const char* pName = pbodyPart->GetPartName();
            ppartsListItem->pMeter->Set(Tile::AddUserTrait("_LimbName", -1), pName, true);
            ActorValue::Index eLimbValue =
                GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(ppartsListItem->eBodyPart));
            ActorValueOwner* pTargetValues = static_cast<Actor*>(pVATSTarget);
            float fLimbHealth = static_cast<double>(pTargetValues->GetActorValue(eLimbValue)) / 100.0;
            float fPercent = BSMIN(1.0f, BSMAX(0.0f, fLimbHealth));
            ppartsListItem->pMeter->Set(Tile::AddUserTrait("_MeterPercent", -1), fPercent, true);
        }
        else if (ptargetWeapon) {
            const char* pName = ptargetWeapon->GetFullName();
            ppartsListItem->pMeter->Set(Tile::AddUserTrait("_LimbName", -1), pName, true);
            uint32_t uiFormHealth;
            if (TESHealthForm::GetFormHealth(ptargetWeaponItem->pContainerObj)) {
                uiFormHealth = TESHealthForm::GetFormHealth(ptargetWeaponItem->pContainerObj);
            }
            else {
                uiFormHealth = 1;
            }
            float fItemHealth = ptargetWeaponItem->GetItemHealth(false) / static_cast<double>(uiFormHealth);
            float fPercent = BSMIN(1.0f, BSMAX(0.0f, fItemHealth));
            ppartsListItem->pMeter->Set(Tile::AddUserTrait("_MeterPercent", -1), fPercent, true);
        }
        if (!(ppartsListItem->kScreenPos.x < 0.0) &&
            !(static_cast<double>(ppartsListItem->kScreenPos.x) > InterfaceManager::GetScreenWidth()) &&
            !(ppartsListItem->kScreenPos.y < 0.0) &&
            !(static_cast<double>(ppartsListItem->kScreenPos.y) > InterfaceManager::GetScreenHeight())) {
            ppartsListItem->bOnScreen = true;
        }
        else {
            ppartsListItem->bOnScreen = false;
            ppartsListItem->bChanceCalculated = false;
            ppartsListItem->bFirstTimeShown = false;
            ppartsListItem->pMeter->RemoveFadeControl(Tile::kTileValue_Alpha);
            float fStart = ppartsListItem->pMeter->GetFloat(Tile::kTileValue_Alpha);
            ppartsListItem->pMeter->AddFadeControl(Tile::kTileValue_Alpha, fStart, 0.0f, 0.5f, Tile::TFCT_STANDARD);
        }
    }
    for (ppartsList = &ptargetList->m_item->kBodyParts; ppartsList && !ppartsList->IsEmpty();
         ppartsList = ppartsList->m_pkNext) {
        VATSBodyPart* ppart = ppartsList->m_item;
        if (ppart->bOnScreen) {
            switch (static_cast<BGSBodyPart::LIMB_ENUM>(ppart->eBodyPart)) {
                case BGSBodyPart::LIMB_HEAD_1:
                case BGSBodyPart::LIMB_HEAD_2: {
                    ilocX = static_cast<int32_t>(ppart->kScreenPos.x);
                    int32_t iOffsetY = ppart->kScreenPos.y > static_cast<double>(VATSMenu::iScreenCenterY) ? 100 : -100;
                    ilocY = static_cast<int32_t>(static_cast<double>(iOffsetY) + ppart->kScreenPos.y);
                    break;
                }
                case BGSBodyPart::LIMB_LEFT_ARM_1:
                case BGSBodyPart::LIMB_LEFT_ARM_2:
                case BGSBodyPart::LIMB_RIGHT_ARM_1:
                case BGSBodyPart::LIMB_RIGHT_ARM_2:
                case BGSBodyPart::LIMB_LEFT_LEG_1:
                case BGSBodyPart::LIMB_LEFT_LEG_2:
                case BGSBodyPart::LIMB_LEFT_LEG_3:
                case BGSBodyPart::LIMB_RIGHT_LEG_1:
                case BGSBodyPart::LIMB_RIGHT_LEG_2:
                case BGSBodyPart::LIMB_RIGHT_LEG_3:
                case BGSBodyPart::LIMB_WEAPON: {
                    int32_t iOffsetX = ppart->kScreenPos.x > static_cast<double>(VATSMenu::iScreenCenterX) ? 100 : -100;
                    ilocX = static_cast<int32_t>(static_cast<double>(iOffsetX) + ppart->kScreenPos.x);
                    int32_t iOffsetY = ppart->kScreenPos.y > static_cast<double>(VATSMenu::iScreenCenterY) ? 50 : -50;
                    ilocY = static_cast<int32_t>(static_cast<double>(iOffsetY) + ppart->kScreenPos.y);
                    break;
                }
                default: {
                    ilocX = static_cast<int32_t>(ppart->kScreenPos.x);
                    ilocY = static_cast<int32_t>(ppart->kScreenPos.y);
                    break;
                }
            }
            ilocX = BSMAX(VATSMenu::iMinX, BSMIN(VATSMenu::iMaxX, ilocX));
            ilocY = BSMAX(VATSMenu::iMinY, BSMIN(VATSMenu::iMaxY, ilocY));
            ppart->pMeter->Set(Tile::kTileValue_X, ilocX);
            ppart->pMeter->Set(Tile::kTileValue_Y, ilocY);
            // The compares are signed.
            int32_t iBodyPart = static_cast<int32_t>(ppart->eBodyPart);
            if (iBodyPart >= BGSBodyPart::LIMB_LEFT_ARM_1 &&
                (iBodyPart <= BGSBodyPart::LIMB_RIGHT_LEG_3 || iBodyPart == BGSBodyPart::LIMB_WEAPON)) {
                if (ilocX >= VATSMenu::iScreenCenterX) {
                    ppart->pMeter->Set(Tile::kTileValue_Justify, 1);
                }
                else {
                    ppart->pMeter->Set(Tile::kTileValue_Justify, 4);
                }
            }
        }
    }
    bool boverlapping = true;
    int32_t icount = 0;
    NiPoint2 limb;
    NiPoint2 compareLimb;
    NiPoint2 center;
    NiPoint2 arbitraryDimensions;
    arbitraryDimensions.x = 150.0f;
    arbitraryDimensions.y = 100.0f;
    while (boverlapping) {
        boverlapping = false;
        ++icount;
        for (ppartsList = &ptargetList->m_item->kBodyParts; ppartsList && !ppartsList->IsEmpty();
             ppartsList = ppartsList->m_pkNext) {
            VATSBodyPart* ppart = ppartsList->m_item;
            for (BSSimpleList<VATSBodyPart*>* prestOfList = ppartsList->m_pkNext;
                 prestOfList && !prestOfList->IsEmpty(); prestOfList = prestOfList->m_pkNext) {
                VATSBodyPart* prestOfListItem = prestOfList->m_item;
                limb.x = ppart->pMeter->GetFloat(Tile::kTileValue_X);
                limb.y = ppart->pMeter->GetFloat(Tile::kTileValue_Y);
                compareLimb.x = prestOfListItem->pMeter->GetFloat(Tile::kTileValue_X);
                compareLimb.y = prestOfListItem->pMeter->GetFloat(Tile::kTileValue_Y);
                float fDiffX = limb.x - compareLimb.x;
                if (!(arbitraryDimensions.x > std::fabs(fDiffX))) {
                    continue;
                }
                float fDiffY = limb.y - compareLimb.y;
                if (!(arbitraryDimensions.y > std::fabs(fDiffY))) {
                    continue;
                }
                if (limb == compareLimb) {
                    compareLimb.x = compareLimb.x + 1.0;
                }
                float fLowX = compareLimb.x < static_cast<double>(limb.x) ? compareLimb.x : limb.x;
                float fCenterDiffX = limb.x - compareLimb.x;
                center.x = std::fabs(fCenterDiffX) / 2.0 + fLowX;
                float fLowY = compareLimb.y < static_cast<double>(limb.y) ? compareLimb.y : limb.y;
                float fCenterDiffY = limb.y - compareLimb.y;
                center.y = std::fabs(fCenterDiffY) / 2.0 + fLowY;
                // Unitizes one difference for its length and divides a second one by it.
                float fLength = (limb - center).Unitize();
                limb = (limb - center) / fLength;
                float fHalfSize = arbitraryDimensions.Length() / 2.0;
                limb = center + limb * fHalfSize;
                fLength = (compareLimb - center).Unitize();
                compareLimb = (compareLimb - center) / fLength;
                fHalfSize = arbitraryDimensions.Length() / 2.0;
                compareLimb = center + compareLimb * fHalfSize;
                limb.x = BSMAX(static_cast<float>(VATSMenu::iMinX), BSMIN(static_cast<float>(VATSMenu::iMaxX), limb.x));
                limb.y = BSMAX(static_cast<float>(VATSMenu::iMinY), BSMIN(static_cast<float>(VATSMenu::iMaxY), limb.y));
                compareLimb.x = BSMAX(
                    static_cast<float>(VATSMenu::iMinX), BSMIN(static_cast<float>(VATSMenu::iMaxX), compareLimb.x)
                );
                compareLimb.y = BSMAX(
                    static_cast<float>(VATSMenu::iMinY), BSMIN(static_cast<float>(VATSMenu::iMaxY), compareLimb.y)
                );
                ppart->pMeter->Set(Tile::kTileValue_X, limb.x, true);
                ppart->pMeter->Set(Tile::kTileValue_Y, limb.y, true);
                prestOfListItem->pMeter->Set(Tile::kTileValue_X, compareLimb.x, true);
                prestOfListItem->pMeter->Set(Tile::kTileValue_Y, compareLimb.y, true);
                boverlapping = true;
            }
        }
        if (icount > 30) {
            boverlapping = false;
        }
    }
}

// Moves the targeted limb: in auto-target, grenade or melee VATS to the head (or none); with a controller, to the part
// of the target's body part list nearest the stick's direction; with the keyboard or the mouse wheel, to the next or
// previous part whose meter is visible. VATSBodyPart::eBodyPart is passed on as the limb.
void VATSMenu::SetTargetLimb() {
    Controls* pcontrols = appMain->GetControls();
    BaseProcess* pprocess = pPlayer->pCurrentProcess;
    ItemChange* pweap = pprocess->GetCurrentWeapon();
    TESObjectWEAP* pweapon;
    if (pweap) {
        pweapon = static_cast<TESObjectWEAP*>(pweap->pContainerObj);
    }
    else {
        pweapon = nullptr;
    }
    BSSimpleList<VATSTarget*>* ptargetList = GetVatsTarget(pVATSTarget);
    if (pVATS.eMode != VATS::VATS_SCAN_MODE) {
        BaseProcess* pprocess2 = pPlayer->pCurrentProcess;
        if (pprocess2->IsCurrentWeaponGrenade() || ptargetList->m_item->eType == 2 || !pweapon ||
            pweapon->IsMeleeWeapon()) {
            if (ptargetList->m_item->eType == 2) {
                eVATSTargetLimb = ActorValue::VALUE_NONE;
            }
            else {
                // A target of another type is an actor.
                TESActorBase* pactorBase = static_cast<Actor*>(ptargetList->m_item->pTargetRef)->GetActorBase();
                BGSBodyPartData* pbodyPartData = pactorBase->GetBodyPartData();
                if (pbodyPartData->pPartArray[BGSBodyPart::LIMB_HEAD_1]) {
                    eVATSTargetLimb = ActorValue::PERCEPTION_CONDITION;
                }
                else {
                    eVATSTargetLimb = ActorValue::VALUE_NONE;
                }
            }
            return;
        }
    }
    InterfaceManager::Instance();
    if (InterfaceManager::Has360Controller()) {
        int32_t ileftStickX = pcontrols->GetControllerValue(0, 7);
        int32_t ileftStickY = pcontrols->GetControllerValue(0, 8);
        int32_t imaxAnalogValue = 32767;
        if (std::abs(ileftStickY) < imaxAnalogValue / 4 && std::abs(ileftStickX) < imaxAnalogValue / 4) {
            bStickReset = 1;
        }
        NiMatrix3 cameraRotationMatrix = Main::GetWorldSceneGraph()->GetCamera()->GetWorldTransform().m_Rotate;
        NiPoint3 stick(0.0f, static_cast<float>(-ileftStickY), static_cast<float>(-ileftStickX));
        stick = cameraRotationMatrix * stick;
        stick.Unitize();
        if (!bStickReset) {
            return;
        }
        if (std::abs(ileftStickY) <= 3 * imaxAnalogValue / 4 && std::abs(ileftStickX) <= 3 * imaxAnalogValue / 4) {
            return;
        }
        float fstoredAngle = 180.0f;
        float fstoredDist = 3.4028235e+38f;
        ActorValue::Index estoredTarget = eVATSTargetLimb;
        BSSimpleList<VATSBodyPart*>* ppartsList = &GetVatsTarget(pVATSTarget)->m_item->kBodyParts;
        VATSBodyPart* poldLimb = pCurrentLimb;
        bool bpartFound = false;
        while (ppartsList && !ppartsList->IsEmpty()) {
            NiPoint3 tempTarget = poldLimb->kRelativePos - ppartsList->m_item->kRelativePos;
            float flength = tempTarget.Length();
            tempTarget.Unitize();
            float fangle = std::acosf(stick.Dot(tempTarget));
            fangle = fangle * 57.2957763671875;
            // The last node of the list whose part maps to the same actor value.
            BSSimpleList<VATSBodyPart*>* pfirstOfType = nullptr;
            for (BSSimpleList<VATSBodyPart*>* pIter = &GetVatsTarget(pVATSTarget)->m_item->kBodyParts;
                 pIter && !pIter->IsEmpty(); pIter = pIter->m_pkNext) {
                ActorValue::Index eiterValue =
                    GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pIter->m_item->eBodyPart));
                if (eiterValue ==
                    GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(ppartsList->m_item->eBodyPart))) {
                    pfirstOfType = pIter;
                }
            }
            ppartsList->m_item->unk34 = fangle;
            if (fangle < 45.0 && flength != 0.0 && fstoredDist > flength && pfirstOfType &&
                pfirstOfType->m_item == ppartsList->m_item) {
                bpartFound = true;
                fstoredDist = flength;
                fstoredAngle = fangle;
                estoredTarget =
                    GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(ppartsList->m_item->eBodyPart));
                bStickReset = 0;
                pCurrentLimb->pMeter->RemoveFadeControl(Tile::kTileValue_Alpha);
                float fstart = pCurrentLimb->pMeter->GetFloat(Tile::kTileValue_Alpha);
                pCurrentLimb->pMeter->AddFadeControl(Tile::kTileValue_Alpha, fstart, 255.0f, 0.2f, Tile::TFCT_STANDARD);
                pCurrentLimb = ppartsList->m_item;
            }
            ppartsList = ppartsList->m_pkNext;
        }
        if (!bpartFound) {
            fstoredAngle = 180.0f;
            for (ppartsList = &GetVatsTarget(pVATSTarget)->m_item->kBodyParts; ppartsList && !ppartsList->IsEmpty();
                 ppartsList = ppartsList->m_pkNext) {
                BSSimpleList<VATSBodyPart*>* pfirstOfType = nullptr;
                for (BSSimpleList<VATSBodyPart*>* pIter = &GetVatsTarget(pVATSTarget)->m_item->kBodyParts;
                     pIter && !pIter->IsEmpty(); pIter = pIter->m_pkNext) {
                    ActorValue::Index eiterValue =
                        GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pIter->m_item->eBodyPart));
                    if (eiterValue ==
                        GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(ppartsList->m_item->eBodyPart))) {
                        pfirstOfType = pIter;
                    }
                }
                if (fstoredAngle > ppartsList->m_item->unk34 && poldLimb != ppartsList->m_item && pfirstOfType &&
                    pfirstOfType->m_item == ppartsList->m_item) {
                    fstoredAngle = ppartsList->m_item->unk34;
                    estoredTarget =
                        GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(ppartsList->m_item->eBodyPart));
                    bStickReset = 0;
                    pCurrentLimb->pMeter->RemoveFadeControl(Tile::kTileValue_Alpha);
                    float fstart = pCurrentLimb->pMeter->GetFloat(Tile::kTileValue_Alpha);
                    pCurrentLimb->pMeter->AddFadeControl(
                        Tile::kTileValue_Alpha, fstart, 255.0f, 0.2f, Tile::TFCT_STANDARD
                    );
                    pCurrentLimb = ppartsList->m_item;
                }
            }
        }
        eVATSTargetLimb = estoredTarget;
        return;
    }
    // 0x11 and 0x1F: the W and S keys' scan codes.
    if (pcontrols->GetKeyboardButton(0x11, Controls::isPressed) ||
        static_cast<double>(InterfaceManager::Instance()->GetMouseWheel()) > 0.0) {
        // The next part with a visible meter, wrapping to the first.
        do {
            for (BSSimpleList<VATSBodyPart*>* pIter = &GetVatsTarget(pVATSTarget)->m_item->kBodyParts;
                 pIter && !pIter->IsEmpty(); pIter = pIter->m_pkNext) {
                if (pIter->m_pkNext && pIter->m_item == pCurrentLimb) {
                    pCurrentLimb = pIter->m_pkNext->m_item;
                    break;
                }
                if (!pIter->m_pkNext) {
                    pCurrentLimb = GetVatsTarget(pVATSTarget)->m_item->kBodyParts.m_item;
                }
            }
            eVATSTargetLimb = GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pCurrentLimb->eBodyPart));
        }
        while (!pCurrentLimb->pMeter->IsTrue(Tile::kTileValue_Visible));
        VATSMenu::ShowTargetLimb(false);
        return;
    }
    if (pcontrols->GetKeyboardButton(0x1F, Controls::isPressed) ||
        static_cast<double>(InterfaceManager::Instance()->GetMouseWheel()) < 0.0) {
        // The previous part with a visible meter, wrapping to the last.
        do {
            for (BSSimpleList<VATSBodyPart*>* pIter = &GetVatsTarget(pVATSTarget)->m_item->kBodyParts;
                 pIter && !pIter->IsEmpty(); pIter = pIter->m_pkNext) {
                if (pIter->m_pkNext && pIter->m_pkNext->m_item == pCurrentLimb) {
                    pCurrentLimb = pIter->m_item;
                    break;
                }
                if (!pIter->m_pkNext) {
                    pCurrentLimb = pIter->m_item;
                }
            }
            eVATSTargetLimb = GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pCurrentLimb->eBodyPart));
        }
        while (!pCurrentLimb->pMeter->IsTrue(Tile::kTileValue_Visible));
        VATSMenu::ShowTargetLimb(false);
    }
}

BSSimpleList<VATSBodyPart*>* VATSMenu::GetTargetPart() {
    for (BSSimpleList<VATSBodyPart*>* piter = &GetVatsTarget(pVATSTarget)->m_item->kBodyParts;
         piter && !piter->IsEmpty(); piter = piter->m_pkNext) {
        if (piter->m_item == pCurrentLimb) {
            return piter;
        }
    }
    return nullptr;
}

bool VATSMenu::PrecalcTargetHitChances() {
    if (!pVATSTarget || bClosing) {
        return false;
    }
    if (pVATS.eMode == VATS::VATS_TARGET_SELECT) {
        if (!bTargetsLit) {
            FOVATSEffectManager::VATS_EFFECT_MODE a2 = FOVATSEffectManager::MULTIPLE_TARGETS;
            FOVATSEffectManager* VATSEffect = Interface::GetVATSEffect();
            VATSEffect->SetEffectMode(a2);
            for (BSSimpleList<VATSTarget*>* ptargets = &VATSMenu::xTargetList; ptargets && !ptargets->IsEmpty();
                 ptargets = ptargets->m_pkNext) {
                TESObjectREFR* v22 = ptargets->m_item->pTargetRef;
                FOVATSEffectManager* v3 = Interface::GetVATSEffect();
                v3->SetMultiTarget(v22);
            }
            bTargetsLit = true;
        }
        TESObjectREFR* v23 = pVATSTarget;
        FOVATSEffectManager* v4 = Interface::GetVATSEffect();
        v4->SetSelectedMultiTarget(v23);
        return true;
    }
    if (bTargetsLit) {
        bTargetsLit = false;
        FOVATSEffectManager* v5 = Interface::GetVATSEffect();
        v5->SetTarget(nullptr, BGSBodyPart::LIMB_TORSO, false);
        FOVATSEffectManager* v6 = Interface::GetVATSEffect();
        v6->ClearMultiTargets();
        FOVATSEffectManager::VATS_EFFECT_MODE arMode = FOVATSEffectManager::SINGLE_TARGET;
        FOVATSEffectManager* v7 = Interface::GetVATSEffect();
        v7->SetEffectMode(arMode);
        VATSMenu::ShowTargetLimb(true);
        return false;
    }
    VATSTarget* currentTarget = GetVatsTarget(pVATSTarget)->m_item;
    if (!currentTarget) {
        return false;
    }
    TESObjectWEAP* pPlayerWeapon;
    BaseProcess* pPlayerProcess = pPlayer->pCurrentProcess;
    ItemChange* v61 = pPlayerProcess->GetCurrentWeapon();
    if (v61) {
        pPlayerWeapon = static_cast<TESObjectWEAP*>(v61->pContainerObj);
    }
    else {
        pPlayerWeapon = nullptr;
    }
    if (currentTarget->bChancesCalculated) {
        if (pVATS.eMode == VATS::VATS_SCAN_MODE) {
            pVATS.SetMode(VATS::VATS_MENU_MODE, false);
        }
        return true;
    }
    FOVATSEffectManager* v8 = Interface::GetVATSEffect();
    if (!v8->IsScanning() && !VATSMenu::bIsARescan) {
        FOVATSEffectManager* v9 = Interface::GetVATSEffect();
        v9->TriggerScan();
        if (!currentTarget->pTargetRef->IsActor()) {
            TESObjectREFR* pTargetRef = currentTarget->pTargetRef;
            FOVATSEffectManager* v10 = Interface::GetVATSEffect();
            v10->SetTarget(pTargetRef, BGSBodyPart::LIMB_HEAD_1, true);
            FOVATSEffectManager* v11 = Interface::GetVATSEffect();
            v11->DisableScan();
        }
    }
    if (pVATS.eMode == VATS::VATS_SCAN_MODE) {
        // BSTimer::GetTime is unsigned, so the difference is compared unsigned.
        if (appTimer.GetTime() - g_vatsLagTimer > 2000) {
            // Parts whose chance is known: their count and the sum of their chances.
            int32_t iCalculated = 0;
            float fChanceSum = 0.0f;
            for (BSSimpleList<VATSBodyPart*>* pIter = &currentTarget->kBodyParts; pIter && !pIter->IsEmpty();
                 pIter = pIter->m_pkNext) {
                if (pIter->m_item->bChanceCalculated) {
                    ++iCalculated;
                    fChanceSum = fChanceSum + pIter->m_item->fChance;
                }
            }
            FOVATSEffectManager* v12 = Interface::GetVATSEffect();
            v12->DisableScan();
            for (BSSimpleList<VATSBodyPart*>* pIter = &currentTarget->kBodyParts; pIter && !pIter->IsEmpty();
                 pIter = pIter->m_pkNext) {
                if (!pIter->m_item->bChanceCalculated) {
                    if (pIter->m_item->fChance <= 0.0) {
                        pIter->m_item->fChance = fChanceSum / static_cast<double>(iCalculated);
                    }
                    pIter->m_item->bChanceCalculated = true;
                    pIter->m_item->bFirstTimeShown = true;
                    pIter->m_item->bNeedsRecalc = false;
                }
            }
            g_vatsLagTimer = appTimer.GetTime();
        }
    }
    else {
        pVATS.SetMode(VATS::VATS_SCAN_MODE, false);
        g_vatsLagTimer = appTimer.GetTime();
    }
    UpdateAmmo();
    // The last body part whose eBodyPart is 0.
    VATSBodyPart* m_item = nullptr;
    for (BSSimpleList<VATSBodyPart*>* pIter = &currentTarget->kBodyParts; pIter && !pIter->IsEmpty();
         pIter = pIter->m_pkNext) {
        if (!pIter->m_item->eBodyPart) {
            m_item = pIter->m_item;
        }
    }
    BSSimpleList<VATSBodyPart*>* i;
    for (i = &currentTarget->kBodyParts; i &&
         (!i->IsEmpty() &&
          (i->m_item->bChanceCalculated || (i->m_item->bFirstTimeShown || (bHeadingReached && !i->m_item->bOnScreen))));
         i = i->m_pkNext) {
        ;
    }
    bool v51 = true;
    for (BSSimpleList<VATSBodyPart*>* pIter = &currentTarget->kBodyParts; pIter && !pIter->IsEmpty();
         pIter = pIter->m_pkNext) {
        if (!pIter->m_item->bChanceCalculated && pIter->m_item->bOnScreen) {
            v51 = false;
            break;
        }
    }
    if (v51 && (!i || i->IsEmpty())) {
        SetMeterChance();
        FOVATSEffectManager* v13 = Interface::GetVATSEffect();
        v13->DisableOcclusion();
        BSSimpleList<VATSBodyPart*>* plimb = FindTargetPart(ActorValue::PERCEPTION_CONDITION);
        if (!plimb || !((plimb->m_item->kPos - pPlayer->Get3D()->GetWorldTranslate()).Length() < 300.0) ||
            currentTarget == VATSMenu_PrecalcTargetHitChances_pLastTarget) {
            if (currentTarget == VATSMenu_PrecalcTargetHitChances_pLastTarget) {
                BSSimpleList<VATSBodyPart*>* plimb_2 = FindTargetPart(eVATSTargetLimb);
                VATSBodyPart* pLimb;
                if (plimb_2) {
                    pLimb = plimb_2->m_item;
                }
                else {
                    pLimb = currentTarget->kBodyParts.m_item;
                }
                pCurrentLimb = pLimb;
            }
            else {
                if (m_item) {
                    eVATSTargetLimb = ActorValue::ENDURANCE_CONDITION;
                    pCurrentLimb = m_item;
                }
                else {
                    // eBodyPart holds the body part data's LIMB_ENUM.
                    eVATSTargetLimb =
                        GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pCurrentLimb->eBodyPart));
                }
            }
        }
        else {
            pCurrentLimb = plimb->m_item;
        }
        VATSMenu_PrecalcTargetHitChances_pLastTarget = currentTarget;
        currentTarget->bChancesCalculated = true;
        VATSMenu::bIsARescan = false;
        pVATS.SetMode(VATS::VATS_MENU_MODE, false);
        FOVATSEffectManager* v15 = Interface::GetVATSEffect();
        v15->DisableScan();
        bool bNoRangedWeapon = false;
        if (currentTarget->eType == 2 || pPlayer->pCurrentProcess->IsCurrentWeaponGrenade() || !pPlayerWeapon ||
            pPlayerWeapon->IsMeleeWeapon()) {
            bNoRangedWeapon = true;
        }
        if (bNoRangedWeapon) {
            VATSMenu::ShowTargetLimb(true);
        }
        UpdateChanceToHit();
        return true;
    }
    if ((i) && (i->m_item)) {
        pCurrentLimb = i->m_item;
        if (!i->m_item->bFirstTimeShown) {
            VATSMenu::ShowTargetLimb(true);
            i->m_item->bFirstTimeShown = true;
        }
    }

    char bContinue = 1;
    for (BSSimpleList<VATSBodyPart*>* j = &currentTarget->kBodyParts;; j = j->m_pkNext) {
        if (!j || j->IsEmpty()) {
            pPlayer->UpdateCamera(false, false);
            pPlayer->UpdateFirstPersonZoom();
            return true;
        }
        VATSBodyPart* pBodyPart = j->m_item;
        float a3 = 0.0f;
        if (pBodyPart->bFirstTimeShown) {
            uint32_t eBodyPart = pBodyPart->eBodyPart;
            FOVATSEffectManager* v16 = Interface::GetVATSEffect();
            if (v16->GetVATSGeometryOcclusionPercent(eBodyPart, a3)) {
                pBodyPart->fPercentVisible = a3;
                if (pBodyPart->kPos.operator==(NiPoint3_ZERO)) {
                    PrepareMeters();
                }
                if (LimbOnScreen(pBodyPart)) {
                    pBodyPart->bChanceCalculated = true;
                }
                else {
                    pBodyPart->bNeedsRecalc = true;
                    pBodyPart->bFirstTimeShown = false;
                }
                if (!pBodyPart->bChanceCalculated && pBodyPart->bOnScreen) {
                    bContinue = 0;
                }
                UpdateHitChanceForBodyPartData(j);
                if ((bContinue) && (!j->m_pkNext)) {
                    break;
                }
            }
        }
    }
    SetMeterChance();
    FOVATSEffectManager* v17 = Interface::GetVATSEffect();
    v17->DisableOcclusion();
    BSSimpleList<VATSBodyPart*>* plimb_3 = FindTargetPart(ActorValue::PERCEPTION_CONDITION);
    if (!plimb_3 || !((plimb_3->m_item->kPos - pPlayer->Get3D()->GetWorldTranslate()).Length() < 300.0) ||
        currentTarget == VATSMenu_PrecalcTargetHitChances_pLastTarget) {
        if (currentTarget == VATSMenu_PrecalcTargetHitChances_pLastTarget) {
            BSSimpleList<VATSBodyPart*>* plimb_4 = FindTargetPart(eVATSTargetLimb);
            VATSBodyPart* pLimb;
            if (plimb_4) {
                pLimb = plimb_4->m_item;
            }
            else {
                pLimb = currentTarget->kBodyParts.m_item;
            }
            pCurrentLimb = pLimb;
        }
        else {
            if (m_item) {
                eVATSTargetLimb = ActorValue::ENDURANCE_CONDITION;
                pCurrentLimb = m_item;
            }
            else {
                // eBodyPart holds the body part data's LIMB_ENUM.
                eVATSTargetLimb =
                    GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pCurrentLimb->eBodyPart));
            }
        }
    }
    else {
        pCurrentLimb = plimb_3->m_item;
    }
    VATSMenu_PrecalcTargetHitChances_pLastTarget = currentTarget;
    pVATS.SetMode(VATS::VATS_MENU_MODE, false);
    FOVATSEffectManager* v19 = Interface::GetVATSEffect();
    v19->DisableScan();
    UpdateChanceToHit();
    char v38 = 0;
    if (currentTarget->eType == 2 || pPlayer->pCurrentProcess->IsCurrentWeaponGrenade() || !pPlayerWeapon ||
        pPlayerWeapon->IsMeleeWeapon()) {
        v38 = 1;
    }
    if (v38) {
        VATSMenu::ShowTargetLimb(true);
    }
    currentTarget->bChancesCalculated = true;
    VATSMenu::bIsARescan = false;
    return true;
}

float VATSMenu::CalculateProbableEnemyHealth() {
    if (pVATSTarget && pVATSTarget->IsActor()) {
        WEAPON_TYPE eweaponType;
        TESObjectWEAP* pweapon;
        float fenemyDamage = 0.0f;
        for (BSSimpleList<VATS_COMMAND*>* iter = &(&pVATS)->commandList; iter && !iter->IsEmpty();
             iter = iter->m_pkNext) {
            TESObjectREFR* pTarget = iter->m_item->pTarget;
            if (pTarget == pVATSTarget) {
                float fdamage = HitData::PredictVATSCommandHealthDamage(iter->m_item);
                fenemyDamage = fenemyDamage -
                    static_cast<Actor*>(pVATSTarget)->Actor::DifficultyLevelAdjustHealthModifier(-fdamage, pPlayer);
            }
        }
        BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
        ItemChange* pweaponItem = CurrentAIProcess->GetCurrentWeapon();
        if (pweaponItem) {
            pweapon = static_cast<TESObjectWEAP*>(pweaponItem->pContainerObj);
        }
        else {
            pweapon = nullptr;
        }
        if (pweapon) {
            eweaponType = pweapon->GetWeaponType();
        }
        else {
            eweaponType = WEAPON_HAND_TO_HAND;
        }
        ActionPoints::Action eaction = ActionPoints::GetAttackAction(eweaponType);
        VATS_COMMAND vc;
        vc.eAction = eaction;
        vc.bSuccess = true;
        vc.pTarget = pVATSTarget;
        // eBodyPart holds the body part data's LIMB_ENUM.
        vc.eLimb = GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pCurrentLimb->eBodyPart));
        vc.cFireShots = VATSMenu::GetShotBurstCount(eaction, false);
        float fdamage_2 = HitData::PredictVATSCommandHealthDamage(&vc);
        double v3 = static_cast<Actor*>(pVATSTarget)->Actor::DifficultyLevelAdjustHealthModifier(-fdamage_2, pPlayer);
        fenemyDamage = fenemyDamage - v3;
        ActorValueOwner* pOwner = static_cast<Actor*>(pVATSTarget);
        if (!pOwner->GetActorValue(ActorValue::HEALTH)) {
            return 0.0f;
        }
        pOwner = static_cast<Actor*>(pVATSTarget);
        int32_t v8 = pOwner->GetActorValue(ActorValue::HEALTH);
        if (!(fenemyDamage > static_cast<double>(v8))) {
            pOwner = static_cast<Actor*>(pVATSTarget);
            int32_t v7 = pOwner->GetBaseActorValue(ActorValue::HEALTH);
            float v13 = fenemyDamage / static_cast<double>(v7);
            return v13;
        }
        pOwner = static_cast<Actor*>(pVATSTarget);
        float v14 = static_cast<float>(pOwner->GetActorValue(ActorValue::HEALTH));
        return v14;
    }
    return 0.0f;
}

bool VATSMenu::PickNewTarget() {
    CFilter kFilter;
    CFilter kPlayerFilter;
    NiPoint3 dir;
    NiPoint3 origin;
    if (bMouseOverButton) {
        return false;
    }
    float fpickLength = fFarthestTarget;
    InterfaceManager* v2 = InterfaceManager::Instance();
    int32_t ScriptProcessingDelay = static_cast<int32_t>(v2->sRealCursorPos.z);
    InterfaceManager* v3 = InterfaceManager::Instance();
    int32_t DataClipDist = static_cast<int32_t>(v3->sRealCursorPos.x);
    SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
    WorldSceneGraph->GetCamera()->WindowPointToRay(DataClipDist, ScriptProcessingDelay, origin, dir, nullptr);
    bhkPickData pd;
    // The custom pick layer collides with projectiles and bipeds.
    uint32_t uiPickLayers = (1u << L_PROJECTILE) | (1u << L_BIPED);
    bhkCollisionFilter::SetMultiLayerCollision(L_CUSTOMPICK1, uiPickLayers);
    pPlayer->GetCollisionFilter(&kPlayerFilter.iFilter);
    kFilter.iFilter = 0;
    kFilter.SetLayer(L_CUSTOMPICK1);
    kFilter.SetGroup(kPlayerFilter.GetGroup());
    pd.SetFilter(kFilter);
    pd.NiSetFrom(origin);
    pd.NiSetLength(dir * fpickLength);
    hkpAllRayHitCollector collector;
    pd.SetAllCollector(&collector);
    bhkWorld* v18 = pPlayer->GetParentCell()->GetbhkWorld();
    v18->PickObject(pd);
    for (int32_t i = 0;; ++i) {
        int32_t FormHealth_0 = (&pd.pAllCollector->getHits())->m_size;
        if (i >= FormHealth_0) {
            break;
        }
        const hkpWorldRayCastOutput* v8 = &(&pd.pAllCollector->getHits())->operator[]<hkpWorldRayCastOutput>(i);
        hkpWorldRayCastOutput point(*v8);
        NiAVObject* pav = GetAVObjectForCollidable(point.m_rootCollidable);
        TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pav);
        if (pav) {
            BSSimpleList<VATSTarget*>* ptarget = this->GetVatsTarget(pref);
            if (ptarget) {
                VATSTarget* m_item = ptarget->m_item;
                if (m_item->pTargetRef == pVATSTarget) {
                    break;
                }
            }
            if (ptarget) {
                VATSMenu::SetHeadingReached(false);
                this->UpdateChanceToHit();
                pVATSTarget = ptarget->m_item->pTargetRef;
                return true;
            }
        }
    }
    return false;
}

int32_t VATSMenu::GetConsecutiveShots() {
    int32_t itotalConsecutiveShots = 0;
    for (BSSimpleList<VATS_COMMAND*>* iter = &(&pVATS)->commandList; iter && !iter->IsEmpty(); iter = iter->m_pkNext) {
        VATS_COMMAND* pnext;
        bool bnext = iter->m_pkNext != nullptr;
        if (bnext) {
            pnext = iter->m_pkNext->m_item;
        }
        else {
            pnext = nullptr;
        }
        VATS_COMMAND* pcurrent = iter->m_item;
        if (!bnext) {
            return itotalConsecutiveShots;
        }
        if (pcurrent->pTarget == pnext->pTarget && pcurrent->eLimb == pnext->eLimb) {
            ++itotalConsecutiveShots;
        }
        else {
            itotalConsecutiveShots = 0;
        }
    }
    return itotalConsecutiveShots;
}

// Writes each body part's chance into its meter. A part sharing its actor value with a later part of the target
// takes the higher meter percentage and chance of the two, and only one of the two meters stays visible.
void VATSMenu::SetMeterChance() {
    BSSimpleList<VATSBodyPart*>* pparts = &GetVatsTarget(pVATSTarget)->GetItem()->kBodyParts;
    while (pparts && !pparts->IsEmpty()) {
        VATSBodyPart* ppart = pparts->GetItem();
        BSSimpleList<VATSBodyPart*>* pother = pparts->GetNext();
        // The part type compares signed.
        int32_t iPart = static_cast<int32_t>(ppart->eBodyPart);
        if (iPart > 0 && iPart <= 12) {
            while (pother && !pother->IsEmpty()) {
                VATSBodyPart* potherPart = pother->GetItem();
                ActorValue::Index ePartValue =
                    GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(ppart->eBodyPart));
                ActorValue::Index eOtherValue =
                    GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(potherPart->eBodyPart));
                if (ePartValue == eOtherValue) {
                    if (!potherPart->pMeter) {
                        BuildMetersForTarget(GetVatsTarget(pVATSTarget));
                    }
                    if (potherPart != ppart) {
                        // Tests the part's type too, and hides the other meter either way.
                        if (potherPart->eBodyPart == 1) {
                            ppart->pMeter->Set(Tile::kTileValue_Visible, 0);
                        }
                        else if (ppart->eBodyPart == 1) {
                            potherPart->pMeter->Set(Tile::kTileValue_Visible, 0);
                        }
                        else {
                            potherPart->pMeter->Set(Tile::kTileValue_Visible, 0);
                        }
                    }
                    // The other meter's percentage is kept as a double.
                    double fOtherPercent = potherPart->pMeter->GetFloat(Tile::AddUserTrait("_MeterPercent", -1));
                    // Only a greater percentage (not NaN) is copied.
                    if (ppart->pMeter->GetFloat(Tile::AddUserTrait("_MeterPercent", -1)) > fOtherPercent) {
                        float fPercent = potherPart->pMeter->GetFloat(Tile::AddUserTrait("_MeterPercent", -1));
                        ppart->pMeter->Set(Tile::AddUserTrait("_MeterPercent", -1), fPercent, true);
                    }
                    // Only a lower chance (not NaN) is raised.
                    if (ppart->fChance < potherPart->fChance) {
                        ppart->fChance = potherPart->fChance;
                    }
                }
                pother = pother->GetNext();
            }
        }
        int32_t iconsShots;
        if (!pVATS.commandList.IsEmpty()) {
            VATS_COMMAND* plastAction = pVATS.GetLastAction();
            if (plastAction->eLimb ==
                GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(ppart->eBodyPart))) {
                iconsShots = GetConsecutiveShots() + 1;
            }
            else {
                iconsShots = 0;
            }
        }
        else {
            iconsShots = 0;
        }
        char cBuffer[8];
        if ((!pPlayer->GetCurrentWeapon() || pPlayer->GetCurrentWeapon()->IsMeleeWeapon()) && pVATSTarget->IsActor() &&
            static_cast<TESActorBaseData*>(static_cast<Actor*>(pVATSTarget)->GetActorBase())->GetNoVATSMelee()) {
            BSsprintf(cBuffer, 8, "%i%%", 0);
            ppart->fChance = 0.0f;
        }
        else if (iconsShots > 0 && bConcentratedFirePerk) {
            // The chance is multiplied by the double 100.0, the bonus is added in double precision, and the sum is
            // truncated to an integer.
            double fChance = ppart->fChance * 100.0;
            int32_t iBonus = iVATSConcentratedFireBonus.Int() * iconsShots;
            BSsprintf(
                cBuffer, 8, "%i%%", BSMAX(0, BSMIN(static_cast<int32_t>(static_cast<double>(iBonus) + fChance), 95))
            );
        }
        else {
            BSsprintf(cBuffer, 8, "%i%%", BSMAX(0, BSMIN(static_cast<int32_t>(ppart->fChance * 100.0), 95)));
        }
        if (!ppart->pMeter) {
            BuildMetersForTarget(GetVatsTarget(pVATSTarget));
        }
        ppart->pMeter->Set(Tile::kTileValue_String, cBuffer, true);
        pparts = pparts->GetNext();
    }
}

// Builds a percent meter under the menu's glow branch for each body part of apTarget's target, justified by the part.
void VATSMenu::BuildMetersForTarget(BSSimpleList<VATSTarget*>* apTarget) {
    Tile* pglow = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eVATSMenu), "GlowBranch");
    for (BSSimpleList<VATSBodyPart*>* pparts = &apTarget->GetItem()->kBodyParts; pparts && !pparts->IsEmpty();
         pparts = pparts->GetNext()) {
        pparts->GetItem()->pMeter = VATSMenu::pVatsMenu->RenderTemplate(pglow, "body_part_percent", nullptr);
        if (pparts->GetItem()->pMeter) {
            switch (static_cast<BGSBodyPart::LIMB_ENUM>(pparts->GetItem()->eBodyPart)) {
                case BGSBodyPart::LIMB_NONE:
                case BGSBodyPart::LIMB_HEAD_1:
                case BGSBodyPart::LIMB_HEAD_2:
                case BGSBodyPart::LIMB_BRAIN: pparts->GetItem()->pMeter->Set(Tile::kTileValue_Justify, 5); break;
                case BGSBodyPart::LIMB_TORSO: pparts->GetItem()->pMeter->Set(Tile::kTileValue_Justify, 6); break;
                case BGSBodyPart::LIMB_LEFT_ARM_1:
                case BGSBodyPart::LIMB_LEFT_ARM_2:
                case BGSBodyPart::LIMB_LEFT_LEG_1:
                case BGSBodyPart::LIMB_LEFT_LEG_2:
                case BGSBodyPart::LIMB_LEFT_LEG_3:
                case BGSBodyPart::LIMB_WEAPON: pparts->GetItem()->pMeter->Set(Tile::kTileValue_Justify, 1); break;
                case BGSBodyPart::LIMB_RIGHT_ARM_1:
                case BGSBodyPart::LIMB_RIGHT_ARM_2:
                case BGSBodyPart::LIMB_RIGHT_LEG_1:
                case BGSBodyPart::LIMB_RIGHT_LEG_2:
                case BGSBodyPart::LIMB_RIGHT_LEG_3: pparts->GetItem()->pMeter->Set(Tile::kTileValue_Justify, 4); break;
                default: break;
            }
        }
    }
}

bool VATSMenu::LimbAppeared() {
    BSSimpleList<VATSTarget*>* ptargetList = GetVatsTarget(pVATSTarget);
    for (BSSimpleList<VATSBodyPart*>* ppartsList = &ptargetList->GetItem()->kBodyParts;
         ppartsList && !ppartsList->IsEmpty(); ppartsList = ppartsList->GetNext()) {
        if (!ppartsList->GetItem()->bOnScreen) {
            NiPoint3 targetPoint = ppartsList->GetItem()->kPos;
            float x = 0.0;
            float y = 0.0;
            SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
            WorldSceneGraph->GetCamera()->WorldPtToScreenPt(targetPoint, x, y, 1e-05f);
            if (!(x <= 0.0) && (!(y <= 0.0) && (!(x >= 1.0) && !(y >= 1.0)))) {
                return true;
            }
        }
    }
    return false;
}

void VATSMenu::GetScreenPlaneProjectionMatrix(NiMatrix3* apmatrix) {
    SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
    NiCamera* Camera = WorldSceneGraph->GetCamera();
    NiPoint3 kDir = Camera->GetWorldDirection();
    // One row buffer, refilled for each of the three rows.
    NiPoint3 kRow;
    kRow.x = kDir.y * kDir.y + kDir.z * kDir.z;
    kRow.y = -kDir.x * kDir.y;
    kRow.z = -kDir.x * kDir.z;
    apmatrix->SetRow_005DF510(0, &kRow);
    kRow.x = -kDir.x * kDir.y;
    kRow.y = kDir.x * kDir.x + kDir.z * kDir.z;
    kRow.z = -kDir.y * kDir.z;
    apmatrix->SetRow_005DF510(1, &kRow);
    kRow.x = -kDir.x * kDir.z;
    kRow.y = -kDir.y * kDir.z;
    kRow.z = kDir.x * kDir.x + kDir.y * kDir.y;
    apmatrix->SetRow_005DF510(2, &kRow);
}

bool VATSMenu::LimbOnScreen(VATSBodyPart* apLimb) {
    NiPoint3 kPt = apLimb->kPos;
    float fBx = 0.0;
    float fBy = 0.0;
    SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
    NiCamera* Camera = WorldSceneGraph->GetCamera();
    Camera->WorldPtToScreenPt(kPt, fBx, fBy, 1e-05f);
    if (!(fBx <= 0.0) && (!(fBy <= 0.0) && (!(fBx >= 1.0) && !(fBy >= 1.0)))) {
        apLimb->bOnScreen = true;
        return true;
    }

    apLimb->bOnScreen = false;
    return false;
}

// The player's chance to hit the body part apList holds (its first item) on the current target, with the player's
// weapon. The weapon condition and the targeted weapon are the target's own weapon's, as the game does. Melee and
// unarmed attacks get no chance when the path to the target is more than twice the straight distance. Fills
// kCurrentCommand for the perk entry points, takes the perks' modified chance when it is higher and caches the result
// in the body part when it had none.
double VATSMenu::UpdateHitChanceForBodyPartData(BSSimpleList<VATSBodyPart*>* apList) {
    float fFinalHitChance = 0.0f;
    if (!apList) {
        return fFinalHitChance;
    }
    VATSBodyPart* pBodyPart = apList->m_item;
    VATSTarget* pTargetEntry = GetVatsTarget(pVATSTarget)->m_item;
    ItemChange* pPlayerWeaponItem = pPlayer->pCurrentProcess->GetCurrentWeapon();
    TESObjectWEAP* pWeapon =
        pPlayerWeaponItem ? static_cast<TESObjectWEAP*>(pPlayerWeaponItem->pContainerObj) : nullptr;
    ItemChange* pTargetWeaponItem =
        pVATSTarget->IsActor() ? static_cast<Actor*>(pVATSTarget)->pCurrentProcess->GetCurrentWeapon() : nullptr;
    double dWeaponHealth;
    if (pTargetWeaponItem) {
        dWeaponHealth = pTargetWeaponItem->GetItemHealth(true) / 100.0;
    }
    else {
        dWeaponHealth = 1.0;
    }
    float fWeaponHealth = static_cast<float>(dWeaponHealth);
    TESObjectWEAP* pTargetWeapon =
        pTargetWeaponItem ? static_cast<TESObjectWEAP*>(pTargetWeaponItem->pContainerObj) : nullptr;
    BGSBodyPart* pLimb = nullptr;
    if (pVATSTarget->IsActor()) {
        BGSBodyPartData* pBodyPartData = static_cast<Actor*>(pVATSTarget)->GetActorBase()->GetBodyPartData();
        pLimb = pBodyPartData->GetBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pBodyPart->eBodyPart));
    }
    float fBoundSize = 0.0f;
    if (pWeapon && !pVATSTarget) {
        float fSize = pWeapon->GetBoundSize();
        fBoundSize = fSize + fSize;
    }
    else {
        float fSize = pVATSTarget->GetModelBoundSize();
        fBoundSize = fSize + fSize;
    }
    if (fBoundSize < 1.0) {
        fBoundSize = 128.0f;
    }
    float fGunSkill;
    if (pWeapon) {
        fGunSkill = pPlayer->GetGunSkill(GUN_SKILL_VATS);
    }
    else {
        fGunSkill = 1.0f;
    }
    ActorValueOwner* pShooter;
    if (pPlayer) {
        pShooter = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*pPlayer)));
    }
    else {
        pShooter = nullptr;
    }
    float fDistance = CombatUtilities::GetDistanceBetweenFacingActors(pPlayer, pVATSTarget, false);
    float fPercentVisible = pBodyPart->fPercentVisible;
    TESObjectWEAP* pTargetedWeapon = pTargetEntry->eType != 2 ? pTargetWeapon : nullptr;
    BGSBodyPart* pTargetedLimb = pTargetEntry->eType != 2 ? pLimb : nullptr;
    float fReach = CombatUtilities::GetWeaponReach(pPlayer, pWeapon);
    fFinalHitChance = CombatFormulas::GetVATSChanceToHit(
        pShooter, pVATSTarget, pWeapon, fWeaponHealth, fReach, fGunSkill, pTargetedLimb, pTargetedWeapon,
        fPercentVisible, fDistance, fBoundSize, nullptr
    );
    bool bPathTooLong = false;
    if (!pWeapon || pWeapon->TESObjectWEAP::IsMeleeWeapon()) {
        PathingLocation kPlayerLocation(pPlayer);
        PathingLocation kTargetLocation(pVATSTarget);
        float fPathDistance = 3.4028235e+38f;
        if (!Pathing::ComputeApproximatePathingDistance(
                kPlayerLocation, kTargetLocation, nullptr, &fPathDistance, nullptr
            )) {
            fPathDistance = 3.4028235e+38f;
        }
        // The target's position is taken first; the length is rounded to a float before the doubling.
        const NiPoint3& rTargetPos = pVATSTarget->GetLocationOnReference();
        float fStraightDistance = (pPlayer->GetLocationOnReference() - rTargetPos).Length();
        if (fPathDistance > fStraightDistance * 2.0) {
            bPathTooLong = true;
        }
    }
    float fModifiedChance;
    if (bPathTooLong) {
        fModifiedChance = 0.0f;
    }
    else {
        fModifiedChance = fFinalHitChance;
    }
    float fConcentratedFire = 0.0f;
    bDataInTemporaryVATSCommand = 1;
    kCurrentCommand.Initialize();
    ActorValue::Index eLimbValue;
    if (pLimb) {
        eLimbValue = static_cast<ActorValue::Index>(pLimb->GetActorValue());
    }
    else {
        eLimbValue = ActorValue::VALUE_NONE;
    }
    kCurrentCommand.eLimb = eLimbValue;
    kCurrentCommand.pTarget = pVATSTarget;
    kCurrentCommand.bSuccess = true;
    kCurrentCommand.pHitData->SetFlag(HitData::HD_IGNORE_CRITICAL, 1);
    HitData::InitializeHitData(kCurrentCommand.pHitData, &kCurrentCommand);
    TESObjectWEAP* pPerkWeapon;
    if (pWeapon) {
        pPerkWeapon = pWeapon;
    }
    else {
        pPerkWeapon = pFists;
    }
    BGSEntryPoint::HandleEntryPoint(
        BGSEntryPoint::CALCULATE_TO_HIT_CHANCE, pPlayer, pPerkWeapon, pVATSTarget, &fModifiedChance
    );
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_CONCENTRATED_FIRE, pPlayer, &fConcentratedFire);
    bConcentratedFirePerk = fConcentratedFire != 0.0;
    if (fFinalHitChance < fModifiedChance) {
        fFinalHitChance = fModifiedChance;
    }
    if (pBodyPart->fChance == 0.0 && fFinalHitChance > 0.0) {
        pBodyPart->fChance = fFinalHitChance;
    }
    return fFinalHitChance;
}

BSSimpleList<VATSBodyPart*>* VATSMenu::FindTargetPart(ActorValue::Index aeLimb) {
    if (!pVATSTarget->IsActor()) {
        return nullptr;
    }
    BSSimpleList<VATSTarget*>* VatsTarget = GetVatsTarget(pVATSTarget);
    VATSTarget* v7 = VatsTarget->m_item;
    if (v7) {
        for (BSSimpleList<VATSBodyPart*>* pIter = &v7->kBodyParts; pIter && !pIter->IsEmpty();
             pIter = pIter->m_pkNext) {
            BSSimpleList<VATSBodyPart*>* v4 = pIter;
            if (GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(v4->m_item->eBodyPart)) == aeLimb) {
                return pIter;
            }
        }
    }
    return nullptr;
}

// BSSoundHandle's destructor is an empty function, so neither the sound handle nor the temporary GetSoundHandleByName
// fills is destroyed.
void VATSMenu::SelectNearestTarget(bool abLeft) {
    TESObjectWEAP* pweapon;
    XFNV_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    BSSoundHandle psoundHand;
    BSAudio* v2 = &BSAudio::QInstance();
    BSSoundHandle* arHandle = v2->GetSoundHandleByName(&a1, "UIVATSSelectTarget", 289u);
    psoundHand.operator=(*arHandle);
    psoundHand.Play(false);
    BSSimpleList<VATSTarget*>* ptargetIter = &VATSMenu::xTargetList;
    float fplayerHeading = pPlayer->GetHeading(false) * 57.2957763671875;
    NiPoint3 cPlayerLoc = pPlayer->GetLocationOnReference();
    float fstoredAngle = static_cast<float>(abLeft ? -360 : 360);
    BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
    ItemChange* pweap = CurrentAIProcess->GetCurrentWeapon();
    if (pweap) {
        pweapon = static_cast<TESObjectWEAP*>(pweap->pContainerObj);
    }
    else {
        pweapon = nullptr;
    }
    while (ptargetIter && !ptargetIter->IsEmpty()) {
        TESObjectREFR* pTargetRef = ptargetIter->m_item->pTargetRef;
        float ftargetHeading = HUDMainMenu::HeadingBetweenLocations(cPlayerLoc, pTargetRef->GetLocationOnReference());
        float fangle = fplayerHeading - ftargetHeading;
        double dangle;
        if (!(fangle > 180.0)) {
            dangle = fangle;
        }
        else {
            dangle = fangle - 360.0;
        }
        fangle = dangle;
        if ((fstoredAngle > static_cast<double>(fangle) && (fangle > 0.0 && !abLeft)) ||
            (fstoredAngle < static_cast<double>(fangle) && (fangle < 0.0 && abLeft))) {
            fstoredAngle = fangle;
            pVATSTarget = ptargetIter->m_item->pTargetRef;
            VATSMenu::SetupSpecialAttackButtons(pweapon);
            BSSimpleList<VATSBodyPart*>* pparts = &ptargetIter->m_item->kBodyParts;
            VATSMenu::pVatsMenu->pCurrentLimb = pparts->m_item;
            while (pparts && !pparts->IsEmpty()) {
                if (ptargetIter->m_item->eType == 1 && pparts->m_item->eBodyPart == 1) {
                    VATSMenu::pVatsMenu->pCurrentLimb = pparts->m_item;
                }
                pparts = pparts->m_pkNext;
            }
        }
        ptargetIter = ptargetIter->m_pkNext;
    }
}

void VATSMenu::Close() {
    XFNV_INLINED_LOCAL(BSSoundHandle, v15, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(BSSoundHandle, apHandle, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(BSSoundHandle, th_s, std::nullopt, std::nullopt);
    float fFortune;
    float fStranger;
    if ((VATSMenu::pVatsMenu) && (VATSMenu::pVatsMenu->xFadeState == Interface::FADE_SHOWN)) {
        if (!VATSMenu::pVatsMenu->bClosing) {
            BSAudio* v0 = &BSAudio::QInstance();
            v0->UnPauseWorldSounds();
        }
        VATSMenu::pVatsMenu->bClosing = true;
        int32_t ClassStatic = VATSMenu::GetClassStatic();
        Tile* ptile = Menu::GetMenuByClass(ClassStatic);
        if (ptile) {
            if (VATSMenu::pVatsMenu) {
                if (!VATSMenu::Destructibles.IsEmpty()) {
                    VATSMenu::Destructibles.RemoveAll();
                }
                InterfaceManager* v2 = InterfaceManager::Instance();
                v2->RefreshActivateREFR();
                BSAudio* v3 = &BSAudio::QInstance();
                v3->EndAllSoundsOfType(256u);
                if (!pSaveLoadGame->GetSaveGameLoading()) {
                    BSAudioManager* mgr = &BSAudioManager::QInstance();
                    BSSoundHandle soundHand;
                    if (VATSMenu::xTargetList.IsEmpty()) {
                        BSSoundHandle* arHandle = mgr->GetSoundHandleByName(&th_s, "UIVATSEnterFail", 289u);
                        soundHand.operator=(*arHandle);
                    }
                    else {
                        BSSoundHandle* SoundHandleByName = mgr->GetSoundHandleByName(&apHandle, "UIVATSExit", 289u);
                        soundHand.operator=(*SoundHandleByName);
                    }
                    soundHand.Play(false);
                }
                ptile->Set(Tile::kTileValue_DeleteOnFade, 1);
                VATSMenu::pVatsMenu->StartFadeOut();
            }
            if (pVATS.GetCount()) {
                BSAudioManager* mgr_2 = &BSAudioManager::QInstance();
                BSSoundHandle soundHand_2;
                BSSoundHandle* v10 = mgr_2->GetSoundHandleByName(&v15, "UIVATSReady", 289u);
                soundHand_2.operator=(*v10);
                soundHand_2.Play(false);
                fFortune = fVATSFortuneOdds.Float();
                fStranger = fVATSStrangerOdds.Float();
                BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::MODIFY_VATS_HELPER_CHANCE, pPlayer, &fFortune);
                BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::MODIFY_VATS_HELPER_CHANCE, pPlayer, &fStranger);
                if ((VATS::pStranger) && (pVATSTarget) &&
                    (pVATSTarget->IsActor() && !static_cast<Actor*>(pVATSTarget)->Actor::GetEssential())) {
                    double v4 = RandomFloat(0.0f, 1.0f);
                    if (fStranger >= v4) {
                        TESFaction* pfaction = static_cast<TESFaction*>(
                            BGSDefaultObjectManager::GetDefaultObject(DO_PERK_STRANGER_FACTION)
                        );
                        if ((pfaction) && (!static_cast<Actor*>(pVATSTarget)->Actor::IsInFaction(pfaction))) {
                            VATS_COMMAND* pcommand;
                            VATS_COMMAND* apMemory = static_cast<VATS_COMMAND*>(ourMalloc(sizeof(VATS_COMMAND)));
                            if (apMemory) {
                                pcommand = ::new (apMemory) VATS_COMMAND;
                            }
                            else {
                                pcommand = nullptr;
                            }
                            pcommand->eLimb = ActorValue::PERCEPTION_CONDITION;
                            pcommand->cFireShots = 5;
                            pcommand->eAction = ActionPoints::ACTION_ATTACK_PISTOL;
                            pcommand->bStranger = true;
                            pcommand->pTarget = pVATSTarget;
                            pcommand->bSuccess = true;
                            pVATS.QueueAction(pcommand);
                            VATS::pStranger->StartCombat(
                                static_cast<Actor*>(pVATSTarget), nullptr, true, true, false, 0, true, nullptr
                            );
                        }
                    }
                }

                if ((VATS::pMissFortune) && (pVATSTarget) &&
                    (pVATSTarget->IsActor() && !static_cast<Actor*>(pVATSTarget)->Actor::GetEssential())) {
                    double v5 = RandomFloat(0.0f, 1.0f);
                    if (fFortune >= v5) {
                        VATS_COMMAND* pcommand_3;
                        VATS_COMMAND* v13 = static_cast<VATS_COMMAND*>(ourMalloc(sizeof(VATS_COMMAND)));
                        if (v13) {
                            pcommand_3 = ::new (v13) VATS_COMMAND;
                        }
                        else {
                            pcommand_3 = nullptr;
                        }
                        pcommand_3->eLimb = ActorValue::PERCEPTION_CONDITION;
                        pcommand_3->cFireShots = 5;
                        pcommand_3->eAction = ActionPoints::ACTION_ATTACK_PISTOL;
                        pcommand_3->bMissFortune = true;
                        pcommand_3->pTarget = pVATSTarget;
                        pcommand_3->bSuccess = true;
                        pVATS.QueueAction(pcommand_3);
                        VATS::pMissFortune->StartCombat(
                            static_cast<Actor*>(pVATSTarget), nullptr, true, true, false, 0, true, nullptr
                        );
                    }
                }

                pVATS.SetMode(VATS::VATS_PLAYBACK, false);
            }
            else {
                pVATS.SetMode(VATS::VATS_MODE_NONE, false);
                bFOVTargetReached = false;
                if (VATS::pStranger) {
                    delete VATS::pStranger;
                    VATS::pStranger = nullptr;
                }
                if (VATS::pMissFortune) {
                    delete VATS::pMissFortune;
                    VATS::pMissFortune = nullptr;
                }
            }
            FOVATSEffectManager* VATSEffect = Interface::GetVATSEffect();
            VATSEffect->SetTarget(nullptr, BGSBodyPart::LIMB_NONE, false);
            FOVATSEffectManager* v7 = Interface::GetVATSEffect();
            v7->ClearMultiTargets();
            bcancelVATS = 0;
        }
        else {
            pVATS.SetMode(VATS::VATS_MODE_NONE, false);
            bFOVTargetReached = false;
        }
    }
}

bool VATSMenu::GetIsARescan() {
    return (bool)((int8_t)VATSMenu::bIsARescan);
}

int32_t VATSMenu::GetClassStatic() {
    return 1056;
}

void VATSMenu::SetHeadingReached(bool abHeadingReached) {
    if (VATSMenu::pVatsMenu) {
        VATSMenu::pVatsMenu->bHeadingReached = (bool)((uint8_t)abHeadingReached);
    }
}

VATS_COMMAND* VATSMenu::GetCurrentQueuedAction() {
    if (VATSMenu::pVatsMenu && VATSMenu::pVatsMenu->bDataInTemporaryVATSCommand) {
        return &VATSMenu::pVatsMenu->kCurrentCommand;
    }

    return nullptr;
}

// The target sorts Create passes are VATSMenu's VATSMenu_SortAngle and VATSMenu_SortDirection (VATSMenu.h). The numeric
// form IDs are the banned-target (0x1770BC) and banned-weapon (0x174256) form lists.
bool VATSMenu::Create() {
    bWeaponSpecialSelected = false;
    g_bIsVATSSpecialAttack2 = false;
    bUnk11DB0B1 = false;
    Tile* pTileRoot = Menu::GetMenuByClass(VATSMenu::GetClassStatic());
    VATSMenu::pVatsMenu = static_cast<VATSMenu*>(pTileRoot->GetMenu());
    if (VATSMenu::pVatsMenu || Interface::IsTopMenuID(Interface::eSleepWaitMenu)) {
        return false;
    }
    VATSMenu::xTargetList.RemoveAll();
    VATSMenu::BuildTargetList();
    TESForm* pBannedTargets = TESForm::GetFormByNumericID(0x1770BC);
    // Drops dead actors, banned actors, swimming actors and missiles stuck in something.
    for (BSSimpleList<VATSTarget*>* pIter = &VATSMenu::xTargetList; pIter && !pIter->IsEmpty();
         pIter = pIter->GetNext()) {
        TESBoundObject* pTargetBase = nullptr;
        if (pIter->GetItem()->pTargetRef->IsActor()) {
            pTargetBase = pIter->GetItem()->pTargetRef->GetObjectReference();
        }
        Actor* pTargetActor =
            pIter->GetItem()->pTargetRef->IsActor() ? static_cast<Actor*>(pIter->GetItem()->pTargetRef) : nullptr;
        Projectile* pTargetProjectile = pIter->GetItem()->pTargetRef->IsProjectile()
            ? static_cast<Projectile*>(pIter->GetItem()->pTargetRef)
            : nullptr;
        bool bStuck = false;
        if (pTargetProjectile && pTargetProjectile->GetProjectileType() == Projectile::PROJECTILE_TYPE_MISSILE) {
            MissileProjectile* pMissile = static_cast<MissileProjectile*>(pTargetProjectile);
            if (pMissile->GetImpactResult() == MissileProjectile::IR_IMPALE ||
                pMissile->GetImpactResult() == MissileProjectile::IR_STICK) {
                bStuck = true;
            }
        }
        TESForm* pBannedBase = pTargetBase;
        if ((pIter->GetItem()->pTargetRef->IsActor() && pIter->GetItem()->pTargetRef->IsDead(false)) ||
            (pBannedTargets && pTargetBase &&
             static_cast<BGSListForm*>(pBannedTargets)->GetFormList()->IsInList(pBannedBase)) ||
            (pTargetActor && pTargetActor->IsSwimming()) || bStuck) {
            if (pVATSTarget == pIter->GetItem()->pTargetRef) {
                pVATSTarget = nullptr;
            }
            pIter->Remove(pIter->GetItem());
        }
    }
    if (VATSMenu::xTargetList.IsEmpty()) {
        if (bVATSReturnPlayerTo1stPerson) {
            pPlayer->SetFirstPerson(true);
        }
        return false;
    }
    TESForm* pPlayerWeapon = nullptr;
    if (pPlayer && pPlayer->GetCurrentAIProcess()) {
        pPlayerWeapon = pPlayer->GetCurrentAIProcess()->GetCurrentWeapon()
            ? pPlayer->GetCurrentAIProcess()->GetCurrentWeapon()->GetItemChangeObject()
            : nullptr;
        if (pPlayer->GetCurrentAIProcess()->IsCurrentWeaponMine()) {
            return false;
        }
    }
    TESForm* pBannedWeapons = TESForm::GetFormByNumericID(0x174256);
    if (pPlayerWeapon && pBannedWeapons &&
        static_cast<BGSListForm*>(pBannedWeapons)->GetFormList()->IsInList(pPlayerWeapon)) {
        return false;
    }
    if (!pVATSTarget) {
        pVATSTarget = VATSMenu::xTargetList.GetItem()->pTargetRef;
    }
    InterfaceManager* pInterfaceManager = InterfaceManager::Instance();
    float fMaxDepth = Tile::GetMaximumDepth();
    pTileRoot = pInterfaceManager->GetMenusRoot()->ReadFile("Data\\Menus\\VATS_menu.xml");
    Menu* pMenu = pTileRoot->GetMenu();
    if (!pMenu || pMenu->GetClass() != VATSMenu::GetClassStatic()) {
        if (pMenu && pMenu->GetMenuTile()) {
            delete pMenu;
        }
        return false;
    }
    pMenu->SetMenuTile(dynamic_cast<TileMenu*>(pTileRoot), false);
    VATSMenu::pVatsMenu = dynamic_cast<VATSMenu*>(pMenu);

    // The Mysterious Stranger appears when his perk fires and the roll beats Miss Fortune's half chance; else Miss
    // Fortune when hers fires. Either is placed 3000 units behind the player and faded in at once, invisible.
    float fHasStranger = 0.0f;
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::MYSTERIOUS_STRANGER, pPlayer, &fHasStranger);
    float fHasMissFortune = 0.0f;
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::MISS_FORTUNE_ENTRY, pPlayer, &fHasMissFortune);
    float fRoll = RandomFloat(0.0f, 1.0f);
    float fStrangerChance = static_cast<float>(1.0 - fHasMissFortune * 0.5);
    if (fHasStranger > 0.0 && fStrangerChance >= fRoll) {
        if (!pStrangerObject) {
            TESForm* pStrangerForm = BGSDefaultObjectManager::GetDefaultObject(DO_PERK_STRANGER_NPC);
            if (pStrangerForm && pStrangerForm->IsBoundObject()) {
                pStrangerObject = static_cast<TESBoundObject*>(pStrangerForm);
            }
        }
        if (fHasStranger != 0.0 && pStrangerObject) {
            NiNode* pPlayer3D = pPlayer->Get3D();
            NiPoint3 kBack = NiPoint3_ZERO;
            NiMatrix3 kRotate = pPlayer3D->GetLocalTransform().m_Rotate;
            NiPoint3 kLocation = pPlayer3D->GetTranslate();
            kBack.x = kRotate.GetEntry(0, 1);
            kBack.y = kRotate.GetEntry(1, 1);
            kBack.z = kRotate.GetEntry(2, 1);
            kBack *= 3000.0f;
            kLocation -= kBack;
            TESWorldSpace* pWorld = TES::pInstance->GetWorldSpace();
            TESObjectCELL* pInterior = TES::pInstance->GetInterior();
            VATS::pStranger = static_cast<Actor*>(pDataHandler->CreateReferenceAtLocation(
                pStrangerObject, kLocation, NiPoint3_ZERO, pInterior, pWorld, nullptr, nullptr, nullptr
            ));
            VATS::pStranger->SetWantWeaponDrawn(true);
            Actor* pStranger = VATS::pStranger;
            static_cast<HighProcess*>(VATS::pStranger->GetCurrentAIProcess())->SkipFadeIn(pStranger);
            VATS::pStranger->SetAlphaMult(0.0f);
        }
    }
    else if (fHasMissFortune > 0.0) {
        if (!pMissFortuneObject) {
            TESForm* pFortuneForm = BGSDefaultObjectManager::GetDefaultObject(DO_PERK_FORTUNE_NPC);
            if (pFortuneForm && pFortuneForm->IsBoundObject()) {
                pMissFortuneObject = static_cast<TESBoundObject*>(pFortuneForm);
            }
        }
        if (fHasMissFortune != 0.0 && pMissFortuneObject) {
            NiNode* pPlayer3D = pPlayer->Get3D();
            NiPoint3 kBack = NiPoint3_ZERO;
            NiMatrix3 kRotate = pPlayer3D->GetLocalTransform().m_Rotate;
            NiPoint3 kLocation = pPlayer3D->GetTranslate();
            kBack.x = kRotate.GetEntry(0, 1);
            kBack.y = kRotate.GetEntry(1, 1);
            kBack.z = kRotate.GetEntry(2, 1);
            kBack *= 3000.0f;
            kLocation -= kBack;
            TESWorldSpace* pWorld = TES::pInstance->GetWorldSpace();
            TESObjectCELL* pInterior = TES::pInstance->GetInterior();
            VATS::pMissFortune = static_cast<Actor*>(pDataHandler->CreateReferenceAtLocation(
                pMissFortuneObject, kLocation, NiPoint3_ZERO, pInterior, pWorld, nullptr, nullptr, nullptr
            ));
            VATS::pMissFortune->SetWantWeaponDrawn(true);
            Actor* pMissFortune = VATS::pMissFortune;
            static_cast<HighProcess*>(VATS::pMissFortune->GetCurrentAIProcess())->SkipFadeIn(pMissFortune);
            VATS::pMissFortune->SetAlphaMult(0.0f);
        }
    }
    float fHasPalm = 0.0f;
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_PARALYZING_PALM, pPlayer, &fHasPalm);
    bParalyzingPalm = fHasPalm > 0.0;
    VATSMenu::pVatsMenu->pCurrentLimb = nullptr;
    VATSMenu::pVatsMenu->GetMenuTile();
    Tile::UpdateAll(false);

    // Scores every target by distance and by how far it is off the player's heading, dropping targets without 3D.
    VATSMenu::xTargetList.Sort(&VATSMenu::SortAngle);
    BSSimpleList<VATSTarget*>* pIter = &VATSMenu::xTargetList;
    float fPlayerHeading = static_cast<float>(pPlayer->GetHeading(false) * 57.2957763671875);
    NiPoint3 kPlayerLocation = pPlayer->GetLocationOnReference();
    if (pIter && !pIter->IsEmpty()) {
        pVATSTarget = pIter->GetItem()->pTargetRef;
    }
    while (pIter && !pIter->IsEmpty()) {
        if (!pIter->GetItem()->pTargetRef || !pIter->GetItem()->pTargetRef->Get3D()) {
            BSSimpleList<VATSTarget*>* pNext = pIter->GetNext();
            VATSMenu::xTargetList.Remove(pIter->GetItem());
            pIter = pNext;
            continue;
        }
        float fHeadingTo = HUDMainMenu::HeadingBetweenLocations(
            kPlayerLocation, pIter->GetItem()->pTargetRef->GetLocationOnReference()
        );
        float fHeadingDelta = fPlayerHeading - fHeadingTo;
        float fAngle = std::fabs(fHeadingDelta);
        NiPoint3 kOffset = kPlayerLocation - pIter->GetItem()->pTargetRef->GetLocationOnReference();
        float fDistanceSqr = NiPoint3::VectorLength(&kOffset);
        double dDistance = fDistanceSqr > 0.0 ? fDistanceSqr : 0.001;
        double dAngle = fAngle > 180.0 ? 360.0 - fAngle : fAngle;
        float fScore = static_cast<float>(40.0 / dDistance + (180.0 - dAngle) / 180.0);
        pIter->GetItem()->unk14 = fScore;
        double dWrapped = fAngle > 180.0 ? 360.0 - fAngle : fAngle;
        float fWrapped = static_cast<float>(dWrapped);
        fAngle = std::fabs(fWrapped);
        pIter->GetItem()->fAngle = fAngle;
        pIter->GetItem()->fDistance = fDistanceSqr;
        pIter = pIter->GetNext();
    }

    // Picks the best scored target, preferring hostile targets within iVatsTargetAngle (the nearest of those), and
    // its body part: the first, or the last part of type 1 on a target of type 1.
    pIter = &VATSMenu::xTargetList;
    float fBestScore = 0.0f;
    BSSimpleList<VATSTarget*>* pBest = nullptr;
    bool bInAngle = false;
    while (pIter && !pIter->IsEmpty()) {
        VATSTarget* pTarget = pIter->GetItem();
        bool bSelect = fBestScore < pTarget->unk14 && !bInAngle;
        if (!bSelect) {
            pTarget = pIter->GetItem();
            // Compares against the setting's value as a signed int.
            int32_t iTargetAngle = static_cast<int32_t>(iVatsTargetAngle.UInt());
            bSelect = static_cast<double>(pTarget->fAngle) < static_cast<double>(iTargetAngle) &&
                pIter->GetItem()->bHostile && (!pBest || pBest->GetItem()->fDistance > pIter->GetItem()->fDistance);
        }
        if (bSelect) {
            pTarget = pIter->GetItem();
            int32_t iTargetAngle = static_cast<int32_t>(iVatsTargetAngle.UInt());
            if (static_cast<double>(pTarget->fAngle) < static_cast<double>(iTargetAngle)) {
                bInAngle = true;
            }
            fBestScore = pIter->GetItem()->unk14;
            pBest = pIter;
            pVATSTarget = pIter->GetItem()->pTargetRef;
            BSSimpleList<VATSBodyPart*>* pPart = &pIter->GetItem()->kBodyParts;
            VATSMenu::pVatsMenu->pCurrentLimb = pPart->GetItem();
            while (pPart && !pPart->IsEmpty()) {
                if (pIter->GetItem()->eType == 1 && pPart->GetItem()->eBodyPart == 1) {
                    VATSMenu::pVatsMenu->pCurrentLimb = pPart->GetItem();
                }
                pPart = pPart->GetNext();
            }
        }
        pIter = pIter->GetNext();
    }
    VATSMenu::pVatsMenu->bDataInTemporaryVATSCommand = 0;
    VATSMenu::xTargetList.Sort(&VATSMenu::SortDirection);
    VATSMenu::pVatsMenu->bDebounceMenuButton = true;
    pVATS.SetMode(VATS::VATS_TARGET_SELECT, false);
    int32_t iScreenWidth = static_cast<int32_t>(InterfaceManager::GetScreenWidth());
    int32_t iScreenHeight = static_cast<int32_t>(InterfaceManager::GetScreenHeight());
    int32_t iSafeX = static_cast<int32_t>(InterfaceManager::GetScreenSafeX());
    int32_t iSafeY = static_cast<int32_t>(InterfaceManager::GetScreenSafeY());
    int32_t iOriginalX = Tile::AddUserTrait("_OriginalX", -1);
    int32_t iTotalWidth = Tile::AddUserTrait("_TotalWidth", -1);
    int32_t iImageWidth = Tile::AddUserTrait("_ImageWidth", -1);
    BSAudio::QInstance().PauseWorldSounds();

    // The action points block, at the bottom right.
    Tile* pAPTile = VATSMenu::VatsMenuGetTileByName(Menu::GetMenuByClass(Interface::eVATSMenu), "ActionPoints");
    if (!pAPTile) {
        return false;
    }
    double dRight = iScreenWidth - 2 * iSafeX;
    float fValue = static_cast<float>(dRight - pAPTile->GetFloat(Tile::kTileValue_Width) + 30.0);
    pAPTile->Set(Tile::kTileValue_X, fValue, true);
    double dBottom = iScreenHeight - 2 * iSafeY;
    fValue = static_cast<float>(dBottom - pAPTile->GetFloat(Tile::kTileValue_Height));
    pAPTile->Set(Tile::kTileValue_Y, fValue, true);
    pAPTile->Set(Tile::kTileValue_Locus, 1);
    VATSMenu::pVatsMenu->pTiles[8] = VATSMenu::VatsMenuRenderTemplate(pAPTile, "template_right_bracket");
    if (!VATSMenu::pVatsMenu->pTiles[8]) {
        return false;
    }
    HUDMainMenu::SetBrightness(VATSMenu::pVatsMenu->pTiles[8], HUDMainMenu::HB_HUD_BRACKETS_GLOW);
    VATSMenu::pVatsMenu->pTiles[8]->Set(Tile::kTileValue_X, -37);
    VATSMenu::pVatsMenu->pTiles[9] = VATSMenu::VatsMenuRenderTemplate(pAPTile, "template_meter");
    if (!VATSMenu::pVatsMenu->pTiles[9]) {
        return false;
    }
    VATSMenu::pVatsMenu->pTiles[9]->Set(iOriginalX, 326);
    VATSMenu::pVatsMenu->pTiles[9]->Set(Tile::kTileValue_Y, 41);
    VATSMenu::pVatsMenu->pTiles[9]->Set(Tile::kTileValue_Depth, 5);
    HUDMainMenu::SetBrightness(VATSMenu::pVatsMenu->pTiles[9], HUDMainMenu::HB_HUD_METERS);
    VATSMenu::pVatsMenu->pTiles[10] = VATSMenu::VatsMenuRenderTemplate(pAPTile, "template_meter");
    if (!VATSMenu::pVatsMenu->pTiles[10]) {
        return false;
    }
    VATSMenu::pVatsMenu->pTiles[10]->Set(iOriginalX, 326);
    VATSMenu::pVatsMenu->pTiles[10]->Set(Tile::kTileValue_Y, 41);
    VATSMenu::pVatsMenu->pTiles[10]->Set(Tile::kTileValue_Alpha, 128);
    HUDMainMenu::SetBrightness(VATSMenu::pVatsMenu->pTiles[10], HUDMainMenu::HB_HUD_METERS);
    VATSMenu::pVatsMenu->pTiles[11] = VATSMenu::VatsMenuRenderTemplate(pAPTile, "template_justify_right_text");
    if (!VATSMenu::pVatsMenu->pTiles[11]) {
        return false;
    }
    VATSMenu::pVatsMenu->pTiles[11]->Set(Tile::kTileValue_X, 315);
    VATSMenu::pVatsMenu->pTiles[11]->Set(Tile::kTileValue_Y, 8);
    HUDMainMenu::SetBrightness(VATSMenu::pVatsMenu->pTiles[11], HUDMainMenu::HB_HUD_TEXT);
    VATSMenu::pVatsMenu->pTiles[11]->Set(Tile::kTileValue_String, sActionPointsShort.String(), true);
    VATSMenu::pVatsMenu->pTiles[12] = VATSMenu::VatsMenuRenderTemplate(pAPTile, "template_justify_right_text");
    if (!VATSMenu::pVatsMenu->pTiles[12]) {
        return false;
    }
    VATSMenu::pVatsMenu->pTiles[12]->Set(Tile::kTileValue_X, 270);
    VATSMenu::pVatsMenu->pTiles[12]->Set(Tile::kTileValue_Y, 8);
    HUDMainMenu::SetBrightness(VATSMenu::pVatsMenu->pTiles[12], HUDMainMenu::HB_HUD_TEXT);
    VATSMenu::pVatsMenu->pTiles[12]->Set(Tile::kTileValue_String, sVATSMessageLowAP.String(), true);
    VATSMenu::pVatsMenu->pTiles[12]->Set(Tile::kTileValue_Visible, 0);
    VATSMenu::pVatsMenu->pTiles[13] = VATSMenu::VatsMenuRenderTemplate(pAPTile, "template_justify_right_text");
    if (!VATSMenu::pVatsMenu->pTiles[13]) {
        return false;
    }
    VATSMenu::pVatsMenu->pTiles[13]->Set(Tile::kTileValue_X, 315);
    VATSMenu::pVatsMenu->pTiles[13]->Set(Tile::kTileValue_Y, 80);
    VATSMenu::pVatsMenu->pTiles[13]->Set(Tile::kTileValue_Font, 7);
    HUDMainMenu::SetBrightness(VATSMenu::pVatsMenu->pTiles[13], HUDMainMenu::HB_HUD_TEXT);
    VATSMenu::pVatsMenu->pTiles[14] = VATSMenu::VatsMenuRenderTemplate(pAPTile, "template_justify_right_text");
    if (!VATSMenu::pVatsMenu->pTiles[14]) {
        return false;
    }
    VATSMenu::pVatsMenu->pTiles[14]->Set(Tile::kTileValue_X, 80);
    fValue = VATSMenu::pVatsMenu->pTiles[13]->GetFloat(Tile::kTileValue_Y);
    VATSMenu::pVatsMenu->pTiles[14]->Set(Tile::kTileValue_Y, fValue, true);
    VATSMenu::pVatsMenu->pTiles[14]->Set(Tile::kTileValue_Font, 7);
    VATSMenu::pVatsMenu->pTiles[14]->Set(Tile::kTileValue_String, sInventoryCondition.String(), true);
    HUDMainMenu::SetBrightness(VATSMenu::pVatsMenu->pTiles[14], HUDMainMenu::HB_HUD_TEXT);
    VATSMenu::pVatsMenu->pTiles[15] = pMenu->RenderTemplate(pAPTile, "template_meter", nullptr);
    VATSMenu::pVatsMenu->pTiles[15]->Set(iOriginalX, 90);
    fValue = static_cast<float>(VATSMenu::pVatsMenu->pTiles[13]->GetFloat(Tile::kTileValue_Y) + 3.0);
    VATSMenu::pVatsMenu->pTiles[15]->Set(Tile::kTileValue_Y, fValue, true);
    VATSMenu::pVatsMenu->pTiles[15]->Set(Tile::kTileValue_Filename, "Interface\\VATS\\vats_bar.dds", true);
    VATSMenu::pVatsMenu->pTiles[15]->Set(iTotalWidth, 60);
    VATSMenu::pVatsMenu->pTiles[15]->Set(iImageWidth, 1);
    VATSMenu::pVatsMenu->pTiles[15]->Set(Tile::kTileValue_Zoom, 200);
    HUDMainMenu::SetBrightness(VATSMenu::pVatsMenu->pTiles[15], HUDMainMenu::HB_HUD_METERS);
    VATSMenu::pVatsMenu->pTiles[16] = pMenu->RenderTemplate(pAPTile, "template_meter_background", nullptr);
    fValue = static_cast<float>(VATSMenu::pVatsMenu->pTiles[13]->GetFloat(Tile::kTileValue_Y) + 3.0);
    VATSMenu::pVatsMenu->pTiles[16]->Set(Tile::kTileValue_Y, fValue, true);
    HUDMainMenu::SetBrightness(VATSMenu::pVatsMenu->pTiles[16], HUDMainMenu::HB_HUD_METER_BACKGROUND);
    // The action points: the current value first, then the permanent one, through the player's ActorValueOwner.
    float fCurrentAP = static_cast<ActorValueOwner*>(pPlayer)->GetActorFloatValue(ActorValue::ACTION_POINTS);
    float fPermanentAP = static_cast<ActorValueOwner*>(pPlayer)->GetPermanentActorFloatValue(ActorValue::ACTION_POINTS);
    float fMaxAP = BSMAX(fPermanentAP, fCurrentAP);
    VATSMenu::pVatsMenu->fActionPointsBase = BSMAX(0.0f, fMaxAP);
    fCurrentAP = static_cast<ActorValueOwner*>(pPlayer)->GetActorFloatValue(ActorValue::ACTION_POINTS);
    VATSMenu::pVatsMenu->fActionPoints = BSMAX(0.0f, fCurrentAP);
    float fMeter = VATSMenu::pVatsMenu->fActionPoints / VATSMenu::pVatsMenu->fActionPointsBase;
    HUDMainMenu::UpdateMeter(VATSMenu::pVatsMenu->pTiles[9], fMeter, HUDMainMenu::HJ_RIGHT, 0, 0);
    fMeter = VATSMenu::pVatsMenu->fActionPoints / VATSMenu::pVatsMenu->fActionPointsBase;
    HUDMainMenu::UpdateMeter(VATSMenu::pVatsMenu->pTiles[10], fMeter, HUDMainMenu::HJ_RIGHT, 0, 0);

    // The hit points block and the compass, at the bottom left.
    Tile* pHPTile = VATSMenu::VatsMenuGetTileByName(Menu::GetMenuByClass(Interface::eVATSMenu), "HitPoints");
    if (!pHPTile) {
        return false;
    }
    pHPTile->Set(Tile::kTileValue_X, 2 * iSafeX + 10);
    dBottom = iScreenHeight - 2 * iSafeY;
    fValue = static_cast<float>(dBottom - pHPTile->GetFloat(Tile::kTileValue_Height));
    pHPTile->Set(Tile::kTileValue_Y, fValue, true);
    pHPTile->Set(Tile::kTileValue_Locus, 1);
    VATSMenu::pVatsMenu->pTiles[17] = VATSMenu::VatsMenuRenderTemplate(pHPTile, "template_left_bracket");
    if (!VATSMenu::pVatsMenu->pTiles[17]) {
        return false;
    }
    HUDMainMenu::SetBrightness(VATSMenu::pVatsMenu->pTiles[17], HUDMainMenu::HB_HUD_BRACKETS_GLOW);
    VATSMenu::pVatsMenu->pTiles[18] = VATSMenu::VatsMenuRenderTemplate(pHPTile, "template_meter");
    if (!VATSMenu::pVatsMenu->pTiles[18]) {
        return false;
    }
    VATSMenu::pVatsMenu->pTiles[18]->Set(iOriginalX, 20);
    VATSMenu::pVatsMenu->pTiles[18]->Set(Tile::kTileValue_Y, 41);
    HUDMainMenu::SetBrightness(VATSMenu::pVatsMenu->pTiles[18], HUDMainMenu::HB_HUD_METERS);
    VATSMenu::pVatsMenu->pTiles[19] = VATSMenu::VatsMenuRenderTemplate(pHPTile, "template_justify_right_text");
    if (!VATSMenu::pVatsMenu->pTiles[19]) {
        return false;
    }
    VATSMenu::pVatsMenu->pTiles[19]->Set(Tile::kTileValue_X, 53);
    VATSMenu::pVatsMenu->pTiles[19]->Set(Tile::kTileValue_Y, 8);
    HUDMainMenu::SetBrightness(VATSMenu::pVatsMenu->pTiles[19], HUDMainMenu::HB_HUD_TEXT);
    VATSMenu::pVatsMenu->pTiles[19]->Set(Tile::kTileValue_String, sHitPointsShort.String(), true);
    VATSMenu::pVatsMenu->pTiles[23] = VATSMenu::VatsMenuRenderTemplate(pHPTile, "template_compass_window");
    if (!VATSMenu::pVatsMenu->pTiles[23]) {
        return false;
    }
    VATSMenu::pVatsMenu->pTiles[20] =
        VATSMenu::VatsMenuRenderTemplate(VATSMenu::pVatsMenu->pTiles[23], "template_compass_icon_group");
    if (!VATSMenu::pVatsMenu->pTiles[20]) {
        return false;
    }
    VATSMenu::pVatsMenu->pTiles[20]->Set(Tile::kTileValue_Y, 30);
    VATSMenu::pVatsMenu->pTiles[21] =
        VATSMenu::VatsMenuRenderTemplate(VATSMenu::pVatsMenu->pTiles[23], "template_compass_icon_group");
    if (!VATSMenu::pVatsMenu->pTiles[21]) {
        return false;
    }
    VATSMenu::pVatsMenu->pTiles[21]->Set(Tile::kTileValue_Y, 30);
    VATSMenu::pVatsMenu->pTiles[22] =
        VATSMenu::VatsMenuRenderTemplate(VATSMenu::pVatsMenu->pTiles[23], "template_compass_icon_group");
    if (!VATSMenu::pVatsMenu->pTiles[22]) {
        return false;
    }
    VATSMenu::pVatsMenu->pTiles[22]->Set(Tile::kTileValue_Y, 10);
    // The tick count is compared signed.
    for (int32_t i = 0; i < static_cast<int32_t>(iHUDMaxCompassNPCTicks.UInt()); ++i) {
        VATSMenu::pVatsMenu->RenderTemplate(VATSMenu::pVatsMenu->pTiles[22], "compass_npc_icon", nullptr);
    }
    for (int32_t i = 0; i < 10; ++i) {
        VATSMenu::pVatsMenu->RenderTemplate(VATSMenu::pVatsMenu->pTiles[21], "template_compass_icon_quest", nullptr);
    }
    VATSMenu::pVatsMenu->RenderTemplate(VATSMenu::pVatsMenu->pTiles[21], "template_compass_icon_player", nullptr);
    for (int32_t i = 0; i < 10; ++i) {
        VATSMenu::pVatsMenu->RenderTemplate(VATSMenu::pVatsMenu->pTiles[20], "template_compass_icon_marker", nullptr);
    }

    // The enemy health block, centred above the bottom.
    Tile* pEnemyTile = VATSMenu::VatsMenuGetTileByName(Menu::GetMenuByClass(Interface::eVATSMenu), "EnemyHealth");
    if (!pEnemyTile) {
        return false;
    }
    double dCentre = iScreenWidth / 2;
    fValue = static_cast<float>(dCentre - pEnemyTile->GetFloat(Tile::kTileValue_Width) / 2.0);
    pEnemyTile->Set(Tile::kTileValue_X, fValue, true);
    pEnemyTile->Set(Tile::kTileValue_Y, iScreenHeight - 2 * iSafeY - 88);
    pEnemyTile->Set(Tile::kTileValue_SystemColor, 2);
    VATSMenu::pVatsMenu->pTiles[24] = VATSMenu::VatsMenuRenderTemplate(pEnemyTile, "template_warning_background");
    if (!VATSMenu::pVatsMenu->pTiles[24]) {
        return false;
    }
    fValue = pEnemyTile->GetFloat(Tile::kTileValue_Width);
    VATSMenu::pVatsMenu->pTiles[24]->Set(Tile::kTileValue_Width, fValue, true);
    fValue = static_cast<float>(pEnemyTile->GetFloat(Tile::kTileValue_Height) + 5.0);
    VATSMenu::pVatsMenu->pTiles[24]->Set(Tile::kTileValue_Height, fValue, true);
    VATSMenu::pVatsMenu->GetMenuTile();
    Tile::UpdateAll(false);
    VATSMenu::pVatsMenu->pTiles[25] = VATSMenu::VatsMenuRenderTemplate(pEnemyTile, "template_enemy_health_bracket");
    if (!VATSMenu::pVatsMenu->pTiles[25]) {
        return false;
    }
    HUDMainMenu::SetBrightness(VATSMenu::pVatsMenu->pTiles[25], HUDMainMenu::HB_HUD_BRACKETS);
    VATSMenu::pVatsMenu->pTiles[25]->Set(Tile::kTileValue_Visible, 1);
    VATSMenu::pVatsMenu->pTiles[26] = VATSMenu::VatsMenuRenderTemplate(pEnemyTile, "template_meter");
    if (!VATSMenu::pVatsMenu->pTiles[26]) {
        return false;
    }
    VATSMenu::pVatsMenu->pTiles[26]->Set(iOriginalX, 171);
    VATSMenu::pVatsMenu->pTiles[26]->Set(iTotalWidth, 240);
    VATSMenu::pVatsMenu->pTiles[26]->Set(Tile::kTileValue_Y, 3);
    HUDMainMenu::SetBrightness(VATSMenu::pVatsMenu->pTiles[26], HUDMainMenu::HB_HUD_METERS);
    VATSMenu::pVatsMenu->pTiles[26]->Set(Tile::kTileValue_Visible, 1);
    VATSMenu::pVatsMenu->pTiles[28] = VATSMenu::VatsMenuRenderTemplate(pEnemyTile, "template_meter");
    if (!VATSMenu::pVatsMenu->pTiles[28]) {
        return false;
    }
    VATSMenu::pVatsMenu->pTiles[28]->Set(iOriginalX, 171);
    VATSMenu::pVatsMenu->pTiles[28]->Set(iTotalWidth, 240);
    VATSMenu::pVatsMenu->pTiles[28]->Set(Tile::kTileValue_Y, 3);
    HUDMainMenu::SetBrightness(VATSMenu::pVatsMenu->pTiles[28], HUDMainMenu::HB_HUD_METERS);
    VATSMenu::pVatsMenu->pTiles[28]->Set(Tile::kTileValue_Visible, 0);
    VATSMenu::pVatsMenu->pTiles[27] =
        VATSMenu::VatsMenuRenderTemplate(VATSMenu::pVatsMenu->pTiles[28], "template_meter");
    if (!VATSMenu::pVatsMenu->pTiles[27]) {
        return false;
    }
    VATSMenu::pVatsMenu->pTiles[27]->Set(iOriginalX, 175);
    VATSMenu::pVatsMenu->pTiles[27]->Set(iTotalWidth, 240);
    VATSMenu::pVatsMenu->pTiles[27]->Set(Tile::kTileValue_Y, 3);
    HUDMainMenu::SetBrightness(VATSMenu::pVatsMenu->pTiles[27], HUDMainMenu::HB_HUD_METERS);
    VATSMenu::pVatsMenu->pTiles[27]->Set(Tile::kTileValue_Visible, 0);
    VATSMenu::pVatsMenu->pTiles[29] = VATSMenu::VatsMenuRenderTemplate(pEnemyTile, "template_justify_center_text");
    if (!VATSMenu::pVatsMenu->pTiles[29]) {
        return false;
    }
    VATSMenu::pVatsMenu->pTiles[29]->Set(Tile::kTileValue_X, 170);
    VATSMenu::pVatsMenu->pTiles[29]->Set(Tile::kTileValue_Y, 35);
    VATSMenu::pVatsMenu->pTiles[29]->Set(Tile::kTileValue_WrapWidth, 350);
    HUDMainMenu::SetBrightness(VATSMenu::pVatsMenu->pTiles[29], HUDMainMenu::HB_HUD_TEXT);

    // The compass's alpha map, which the compass's shader rotates.
    NiPointer<NiSourceTexture> spAlphaMap(nullptr);
    {
        NiFixedString kFilename("Textures\\Interface\\HUD\\hud_compass_alphamap.dds");
        spAlphaMap = NiSourceTexture::Create(kFilename, NiSourceTexture::ms_kDefaultPrefs, true, false);
    }
    NiSourceTexture* pAlphaMap = spAlphaMap.operator->();
    VATSMenu::pVatsMenu->pTiles[23]->GetShaderProperty()->SetAlphaMap(pAlphaMap);
    VATSMenu::pVatsMenu->pTiles[23]->GetShaderProperty()->SetUseTexScroll(true);
    int32_t iMissingTile;
    if (!VATSMenu::pVatsMenu->IsWellFormedMenu(iMissingTile)) {
        InitializationFlow(
            "VATS Menu Creation Failed... This likely means that the archives were not built and the "
            "xml doesn't match the code."
        );
        Interface::EmergencyCloseAllMenusAndBreakStuff();
        return false;
    }
    if (pTileRoot->GetFloat(Tile::kTileValue_StackingType) == 6006.0 ||
        pTileRoot->GetFloat(Tile::kTileValue_StackingType) == 102.0) {
        pTileRoot->Set(Tile::kTileValue_Depth, fMaxDepth, true);
    }
    // An unused string and clock reading (hour and minute, the hour on a 12 hour clock).
    BSStringT<char> kTimeString;
    float fHour = appCalendar.GetHour();
    [[maybe_unused]] int8_t cHour = static_cast<int8_t>(static_cast<int32_t>(fHour));
    [[maybe_unused]] int8_t cMinute = static_cast<int8_t>(static_cast<int32_t>((fHour - cHour) * 60.0));
    double dHour12 = cHour < 1.0 ? 12.0 : (cHour > 12.0 ? cHour - 12.0 : static_cast<double>(cHour));
    [[maybe_unused]] int8_t cHour12 = static_cast<int8_t>(static_cast<int32_t>(dHour12));
    // BODY_PART_TYPE_NONE.
    VATSMenu::uiLastBodyPart = 0xFFFFFFFF;
    eVATSTargetLimb = ActorValue::PERCEPTION_CONDITION;
    pPlayer->SetIronSights(false, false, false);
    bUnk11DB0CC = false;
    VATSMenu::pVatsMenu->lbActions.Initialize(VATSMenu::pVatsMenu->pTiles[1], "queued_action");
    BSAudioManager* pAudioManager = &BSAudioManager::QInstance();
    BSSoundHandle kEnterSound;
    // 0x121: SOUND_FLAG_IS_2D | SOUND_FLAG_SYSTEM_SOUND | SOUND_FLAG_ONE_SHOT.
    XFNV_INLINED_LOCAL(BSSoundHandle, kLookup, std::nullopt, std::nullopt);
    BSSoundHandle* pFound =
        pAudioManager->GetSoundHandleByName(&kLookup, "UIVATSEnter", static_cast<BSGameSound::SoundFlags>(0x121));
    kEnterSound = *pFound;
    kLookup.~BSSoundHandle();
    kEnterSound.Play(false);
    VATSMenu::pVatsMenu->bStickReset = 0;
    VATSMenu::pVatsMenu->bTargetStickReset = 0;
    VATSMenu::pVatsMenu->bMouseOverButton = 0;
    VATSMenu::pVatsMenu->bMetersShown = false;
    VATSMenu::pVatsMenu->bTargetsLit = false;
    VATSMenu::pVatsMenu->bSelectLeftHeld = false;
    VATSMenu::pVatsMenu->bSelectRightHeld = false;
    VATSMenu::pVatsMenu->bClosing = false;
    VATSMenu::pVatsMenu->uiSelectTimer = appTimer.GetTime();
    ItemChange* pWeaponItem = pPlayer->GetCurrentAIProcess()->GetCurrentWeapon();
    TESObjectWEAP* pWeapon = pWeaponItem ? static_cast<TESObjectWEAP*>(pWeaponItem->GetItemChangeObject()) : nullptr;
    if (!pWeapon || pWeapon->IsMeleeWeapon() || pWeapon->GetWeaponType() == WEAPON_ONE_HAND_GRENADE) {
        VATSMenu::pVatsMenu->pTiles[6]->Set(Tile::kTileValue_Visible, 0);
    }
    // Tutorial message 27 with a 360 controller, else 21.
    int32_t iMenuID = VATSMenu::GetClassStatic();
    int32_t eTutorial = Interface::Has360Controller() ? 27 : 21;
    InterfaceManager::Instance()->xTutorialManager.ShowMessage(
        static_cast<Interface::TutorialMessageID>(eTutorial), iMenuID, 512
    );
    float fAttackCost = ActionPoints::GetWeaponAttackActionPoints(pWeapon, true, false);
    if (fAttackCost > VATSMenu::pVatsMenu->fActionPoints) {
        VATSMenu::pVatsMenu->pTiles[7]->Set(Tile::kTileValue_Target, 0);
    }
    if (BSstrcmp(sLanguage.String(), "ENGLISH")) {
        VATSMenu::pVatsMenu->pTiles[7]->Set(Tile::kTileValue_String, sVatsSelect.String(), true);
        VATSMenu::pVatsMenu->pTiles[4]->Set(Tile::kTileValue_String, sAccept.String(), true);
        VATSMenu::pVatsMenu->pTiles[5]->Set(Tile::kTileValue_String, sReturn.String(), true);
        VATSMenu::pVatsMenu->pTiles[6]->Set(Tile::kTileValue_String, sVatsBodyPart.String(), true);
        const char* pTargetText = sVatsTarget.String();
        VATSMenu::pVatsMenu->GetMenuTile()
            ->GetChildByName("Target_button")
            ->Set(Tile::kTileValue_String, pTargetText, true);
    }
    Tile* pButton = VATSMenu::pVatsMenu->GetMenuTile()->GetChildByName("BodyPart_button");
    if (pButton) {
        pButton->Set(Tile::TextToTrait("_xbox_button"), "&xbuttonls;", true);
        pButton->Set(Tile::TextToTrait("_xbox_button"), 17);
    }
    pButton = VATSMenu::pVatsMenu->GetMenuTile()->GetChildByName("Target_button");
    if (pButton) {
        pButton->Set(Tile::TextToTrait("_xbox_button"), "&xbuttonrs;", true);
        pButton->Set(Tile::TextToTrait("_xbox_button"), 18);
    }
    VATSMenu::SetupSpecialAttackButtons(pWeapon);
    pMenu->PrepForVisibility(false);
    return true;
}

bool VATSMenu::BuildTargetList() {
    if (!VATSMenu::xTargetList.IsEmpty()) {
        return true;
    }
    bool bvalidTargetsFound = false;
    SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
    WorldSceneGraph->GetCamera()->WorldToCameraMatrix();
    BSSimpleList<CompassTarget*>* plist = pPlayer->GetListofPerceivedActors();
    float fmaxNPCDist = 0.0f;
    bool binterior = pPlayer->GetParentCell()->GetInterior();
    VATSMenu::Destructibles.RemoveAll();
    while (plist && !plist->IsEmpty()) {
        CompassTarget* pact = plist->m_item;
        bool v143 = true;
        if (pact) {
            int32_t v142 = BGSDestructibleObjectForm::GetDestructionForm(pact->pTarget->GetObjectReference())->pData &&
                !pact->pTarget->GetVATStarget();
            v143 = v142;
        }
        if ((pact) && (!pact->pTarget->IsDead(false)) && (pact->pTarget->Get3D()) &&
            (pact->pTarget->GetCurrentProcessType() == PROCESS_TYPE_HIGH && !pact->pTarget->IsChild(false) && !v143)) {
            float fdist = pact->pTarget->GetDistanceFromReference(pPlayer, false, false);
            if (!(fdist < 2.0)) {
                float* v3 = &fVATSMaxEngageDistance.Float();
                if (!(*v3 < static_cast<double>(fdist)) &&
                    pPlayer->LineOfSight(true, pact->pTarget, true, nullptr, true)) {
                    float* v7;
                    float* v8;
                    float* v9;
                    if (fmaxNPCDist < static_cast<double>(fdist)) {
                        fmaxNPCDist = fdist;
                    }
                    alignas(16) NiPoint3 targetPoint = pact->pTarget->Get3D()->GetWorldTranslate();
                    float x = 0.0;
                    float y = 0.0;
                    SceneGraph* v4 = Main::GetWorldSceneGraph();
                    v4->GetCamera()->WorldPtToScreenPt(targetPoint, x, y, 1e-05f);
                    bool v137 = !(x <= 0.0) && (!(y <= 0.0) && (!(x >= 1.0) && !(y >= 1.0)));
                    char v136 = v137;
                    if (v137) {
                        bvalidTargetsFound = true;
                    }
                    else {
                        TESActorBase* bonScreen = pact->pTarget->GetActorBase();
                        BGSBodyPartData* pbodyPartData = bonScreen->GetBodyPartData();
                        BGSBodyPart** pbodyPartArray = pbodyPartData->pPartArray;
                        for (BGSBodyPart::LIMB_ENUM eindex = BGSBodyPart::LIMB_TORSO; eindex < BGSBodyPart::LIMB_COUNT;
                             eindex = static_cast<BGSBodyPart::LIMB_ENUM>(eindex + 1)) {
                            if (pbodyPartArray[eindex]) {
                                Actor* pTarget = plist->m_item->pTarget;
                                const char* fZeroTolerance = pbodyPartData->GetBodyPart(eindex)->GetTargetName();
                                NiNode* v5 = pTarget->Get3D();
                                NiAVObject* plimb = BSUtilities::GetObjectByName(v5, fZeroTolerance);
                                if (plimb) {
                                    float v130;
                                    alignas(16) float v131;
                                    const NiPoint3* WorldTranslate = &plimb->GetWorldTranslate();
                                    SceneGraph* v6 = Main::GetWorldSceneGraph();
                                    v6->GetCamera()->WorldPtToScreenPt(*WorldTranslate, v130, v131, 1e-05f);
                                    if (v130 > 0.0 && (v131 > 0.0 && (v130 < 1.0 && v131 < 1.0))) {
                                        v136 = 1;
                                        bvalidTargetsFound = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if ((((!binterior || (v7 = &fSneakMaxDistance.Float(), !(*v7 < static_cast<double>(fdist)))) &&
                          (binterior ||
                           (v8 = &fSneakExteriorDistanceMult.Float(),
                            (v9 = &fSneakMaxDistance.Float(), !(fdist > *v8 * *v9))))) ||
                         v136) &&
                        (pact->pTarget->Get3D())) {
                        TESObjectWEAP* ptargetWeapon;
                        ItemChange* ptargetWeaponItem;
                        VATSTarget* v125[2];
                        VATSTarget* vt;
                        VATSTarget* v127 = static_cast<VATSTarget*>(ourMalloc(sizeof(VATSTarget)));
                        if (v127) {
                            vt = ::new (v127) VATSTarget;
                        }
                        else {
                            vt = nullptr;
                        }
                        v125[1] = vt;
                        v125[0] = vt;
                        vt->eType = 1;
                        v125[0]->pTargetRef = (TESObjectREFR*)pact->pTarget;
                        v125[0]->bChancesCalculated = false;
                        CompassTarget* m_item = plist->m_item;
                        v125[0]->bHostile = m_item->bIsHostile;
                        // Reads the target's base through Actor::GetActorBase without testing that the
                        // reference is an actor, as the game does.
                        TESActorBase* ptargetBase = static_cast<Actor*>(v125[0]->pTargetRef)->GetActorBase();
                        BGSBodyPart** ppartArray = ptargetBase->GetBodyPartData()->pPartArray;
                        for (int32_t j = 0; j < 15; ++j) {
                            if (ppartArray[j]) {
                                VATSBodyPart* v118[2];
                                VATSBodyPart* v119;
                                VATSBodyPart* v120 = static_cast<VATSBodyPart*>(ourMalloc(sizeof(VATSBodyPart)));
                                if (v120) {
                                    v119 = ::new (v120) VATSBodyPart;
                                }
                                else {
                                    v119 = nullptr;
                                }
                                v118[1] = v119;
                                v118[0] = v119;
                                v119->bOnScreen = false;
                                v118[0]->bChanceCalculated = false;
                                v118[0]->bFirstTimeShown = false;
                                v118[0]->bNeedsRecalc = true;
                                v118[0]->kPos = NiPoint3_ZERO;
                                v118[0]->fChance = 0.0f;
                                v118[0]->eBodyPart = j;
                                v125[0]->kBodyParts.AddHead(*v118);
                            }
                        }
                        NiNode* pweaponBone = nullptr;
                        if (v125[0]->pTargetRef->IsActor()) {
                            BaseProcess* pprocess = static_cast<Actor*>(v125[0]->pTargetRef)->GetCurrentAIProcess();
                            BipedAnim* v11 = v125[0]->pTargetRef->GetBiped();
                            pweaponBone = pprocess->GetWeaponBone(v11);
                        }
                        if (v125[0]->pTargetRef->IsActor()) {
                            ptargetWeaponItem =
                                static_cast<Actor*>(v125[0]->pTargetRef)->GetCurrentAIProcess()->GetCurrentWeapon();
                        }
                        else {
                            ptargetWeaponItem = nullptr;
                        }
                        ItemChange* v113 = ptargetWeaponItem;
                        if (ptargetWeaponItem) {
                            ptargetWeapon = static_cast<TESObjectWEAP*>(v113->pContainerObj);
                        }
                        else {
                            ptargetWeapon = nullptr;
                        }
                        TESObjectWEAP* ptargetWeapon_2 = ptargetWeapon;
                        if ((pweaponBone) && (ptargetWeapon_2) && (!ptargetWeapon_2->IsEmbeddedWeapon())) {
                            VATSBodyPart* v108[2];
                            VATSBodyPart* v109;
                            VATSBodyPart* v110 = static_cast<VATSBodyPart*>(ourMalloc(sizeof(VATSBodyPart)));
                            if (v110) {
                                v109 = ::new (v110) VATSBodyPart;
                            }
                            else {
                                v109 = nullptr;
                            }
                            v108[1] = v109;
                            v108[0] = v109;
                            v109->bOnScreen = false;
                            v108[0]->bChanceCalculated = false;
                            v108[0]->bFirstTimeShown = false;
                            v108[0]->bNeedsRecalc = true;
                            v108[0]->kPos = NiPoint3_ZERO;
                            v108[0]->eBodyPart = 14;
                            v125[0]->kBodyParts.AddHead(*v108);
                        }

                        if (v125[0]->kBodyParts.IsEmpty()) {
                            VATSBodyPart* v105[2];
                            VATSBodyPart* v106;
                            v125[0]->eType = 2;
                            VATSBodyPart* v107 = static_cast<VATSBodyPart*>(ourMalloc(sizeof(VATSBodyPart)));
                            if (v107) {
                                v106 = ::new (v107) VATSBodyPart;
                            }
                            else {
                                v106 = nullptr;
                            }
                            v105[1] = v106;
                            v105[0] = v106;
                            v106->eBodyPart = -1;
                            v105[0]->bOnScreen = false;
                            v105[0]->bChanceCalculated = false;
                            v105[0]->bFirstTimeShown = false;
                            v105[0]->bNeedsRecalc = true;
                            v105[0]->kPos = NiPoint3_ZERO;
                            v125[0]->kBodyParts.AddHead(*v105);
                        }
                        VATSMenu::xTargetList.AddHead(*v125);
                    }
                }
            }
        }

        plist = plist->m_pkNext;
    }
    if (fmaxNPCDist != 0.0) {
        fmaxNPCDist = fmaxNPCDist + 128.0;
    }
    if (pPlayer->GetWorldSpace()) {
        PlayerCharacter* fZeroTolerancea = pPlayer;
        float* v12 = &fSneakExteriorDistanceMult.Float();
        float afRadius2 = *v12 * fSneakMaxDistance.Float();
        const NiPoint3* v45 = &pPlayer->GetLocationOnReference();
        float* v13 = &fSneakExteriorDistanceMult.Float();
        float afRadius1 = *v13 * fSneakMaxDistance.Float();
        const NiPoint3* v43 = &pPlayer->GetLocationOnReference();
        TESWorldSpace* WorldSpace = pPlayer->GetWorldSpace();
        WorldSpace->EnumReferencesCloseToPoint(
            *v43, afRadius1, *v45, afRadius2, &VATSMenu::FilterDestructibleObjects, fZeroTolerancea
        );
    }
    else {
        PlayerCharacter* fZeroToleranceb = pPlayer;
        float* v15 = &fSneakExteriorDistanceMult.Float();
        float v102 = *v15 * fSneakMaxDistance.Float();
        const NiPoint3* v46 = &pPlayer->GetLocationOnReference();
        float* v16 = &fSneakExteriorDistanceMult.Float();
        float v101 = *v16 * fSneakMaxDistance.Float();
        const NiPoint3* v17 = &pPlayer->GetLocationOnReference();
        pPlayer->GetParentCell()->EnumReferencesCloseToPoint(
            *v17, v101, *v46, v102, &VATSMenu::FilterDestructibleObjects, fZeroToleranceb
        );
    }
    for (BSSimpleList<TESObjectREFR*>* pdestructibleIter = &VATSMenu::Destructibles;
         pdestructibleIter && !pdestructibleIter->IsEmpty(); pdestructibleIter = pdestructibleIter->m_pkNext) {
        float fdist_2 = pdestructibleIter->m_item->GetDistanceFromReference(pPlayer, false, false);
        Actor* v98 = static_cast<Actor*>(pdestructibleIter->m_item);
        if (!v98->IsActor()) {
            TESObjectREFR* phitRef = pdestructibleIter->m_item;
            if (!phitRef->IsExplosion()) {
                double DistanceFromReference;
                if ((!(fmaxNPCDist > 0.0) ||
                     (DistanceFromReference =
                          pdestructibleIter->m_item->GetDistanceFromReference(pPlayer, false, false),
                      !(fmaxNPCDist < DistanceFromReference))) &&
                    (!pdestructibleIter->m_item->GetDestroyed() && !pdestructibleIter->m_item->GetDisabled() &&
                     !pdestructibleIter->m_item->GetDelete())) {
                    float* v19;
                    if (!binterior || (v19 = &fSneakMaxDistance.Float(), !(*v19 < static_cast<double>(fdist_2)))) {
                        float* v20;
                        float* v21;
                        if ((binterior ||
                             (v20 = &fSneakExteriorDistanceMult.Float(),
                              (v21 = &fSneakMaxDistance.Float(), !(fdist_2 > *v20 * *v21)))) &&
                            (pdestructibleIter->m_item->GetVATStarget())) {
                            Projectile* pproj;
                            Actor* v96 = static_cast<Actor*>(pdestructibleIter->m_item);
                            if ((!v96->IsProjectile() ||
                                 (pproj = static_cast<Projectile*>(pdestructibleIter->m_item),
                                  pproj->GetShooter() != pPlayer)) &&
                                (!pdestructibleIter->m_item->GetDestroyed())) {
                                alignas(16) VATSBodyPart* v89[2];
                                VATSBodyPart* v90;
                                alignas(16) VATSTarget* v92[2];
                                VATSTarget* v93;
                                VATSTarget* v94 = static_cast<VATSTarget*>(ourMalloc(sizeof(VATSTarget)));
                                if (v94) {
                                    v93 = ::new (v94) VATSTarget;
                                }
                                else {
                                    v93 = nullptr;
                                }
                                v92[1] = v93;
                                v92[0] = v93;
                                v93->eType = 2;
                                BSSimpleList<TESObjectREFR*>* v22 = pdestructibleIter;
                                v92[0]->pTargetRef = v22->m_item;
                                v92[0]->bChancesCalculated = false;
                                v92[0]->bHostile = false;
                                VATSBodyPart* v91 = static_cast<VATSBodyPart*>(ourMalloc(sizeof(VATSBodyPart)));
                                if (v91) {
                                    v90 = ::new (v91) VATSBodyPart;
                                }
                                else {
                                    v90 = nullptr;
                                }
                                v89[1] = v90;
                                v89[0] = v90;
                                v90->eBodyPart = -1;
                                v89[0]->bOnScreen = false;
                                v89[0]->bChanceCalculated = false;
                                v89[0]->bFirstTimeShown = false;
                                v89[0]->bNeedsRecalc = true;
                                v89[0]->kPos = NiPoint3_ZERO;
                                v92[0]->kBodyParts.AddHead(*v89);
                                if (!bvalidTargetsFound) {
                                    TESObjectREFR* bonScreen_2 = pdestructibleIter->m_item;
                                    alignas(16) NiPoint3 targetPoint_2 = bonScreen_2->Get3D()->GetWorldTranslate();
                                    float x_2 = 0.0;
                                    float y_2 = 0.0;
                                    SceneGraph* v23 = Main::GetWorldSceneGraph();
                                    v23->GetCamera()->WorldPtToScreenPt(targetPoint_2, x_2, y_2, 1e-05f);
                                    bool v84 = !(x_2 <= 0.0) && (!(y_2 <= 0.0) && (!(x_2 >= 1.0) && !(y_2 >= 1.0)));
                                    bool v83 = v84;
                                    if (v84) {
                                        bhkPickData pd;
                                        NiPoint3 origin = Main::GetWorldSceneGraph()->GetCamera()->GetWorldTranslate();
                                        CFilter fltr;
                                        CFilter kPlayerFilter;
                                        fltr.iFilter = 0;
                                        fltr.SetLayer(L_LOS);
                                        pPlayer->GetCollisionFilter(&kPlayerFilter.iFilter);
                                        fltr.SetGroup(kPlayerFilter.GetGroup());
                                        pd.SetFilter(fltr);
                                        hkpAllRayHitCollector collector;
                                        pd.SetAllCollector(&collector);
                                        pd.NiSetFrom(origin);
                                        TESObjectREFR* v76 = pdestructibleIter->m_item;
                                        NiNode* v26 = v76->Get3D();
                                        const NiBound* WorldBound = &v26->GetWorldBound();
                                        pd.NiSetTo(WorldBound->m_kCenter);
                                        TESObjectREFR* v75 = pdestructibleIter->m_item;
                                        NiNode* v28 = v75->Get3D();
                                        const NiBound* v29 = &v28->GetWorldBound();
                                        pd.NiSetLength(v29->m_kCenter - origin);
                                        bhkWorld* v73 = pPlayer->GetParentCell()->GetbhkWorld();
                                        if ((v73->PickObject(pd)) && (pd.pAllCollector)) {
                                            for (int32_t i = 0; !bvalidTargetsFound; ++i) {
                                                int32_t FormHealth_0 = (&pd.pAllCollector->getHits())->m_size;
                                                if (i >= FormHealth_0) {
                                                    break;
                                                }
                                                const hkpWorldRayCastOutput* v32 =
                                                    &(&pd.pAllCollector->getHits())
                                                         ->operator[]<hkpWorldRayCastOutput>(i);
                                                hkpWorldRayCastOutput point(*v32);
                                                NiNode* pav = static_cast<NiNode*>(
                                                    GetAVObjectForCollidable(point.m_rootCollidable)
                                                );
                                                if (pav) {
                                                    TESObjectREFR* v68 = pdestructibleIter->m_item;
                                                    if (pav == v68->Get3D()) {
                                                        bvalidTargetsFound = true;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                VATSMenu::xTargetList.AddHead(*v92);
                            }
                        }
                    }
                }
            }
        }
    }
    if (!bvalidTargetsFound) {
        double v60;
        bhkPickData pd_2;
        NiPoint3 origin_2 = Main::GetWorldSceneGraph()->GetCamera()->GetWorldTranslate();
        CFilter fltr_2;
        CFilter kPlayerFilter_2;
        fltr_2.iFilter = 0;
        fltr_2.SetLayer(L_LOS);
        pPlayer->GetCollisionFilter(&kPlayerFilter_2.iFilter);
        fltr_2.SetGroup(kPlayerFilter_2.GetGroup());
        pd_2.SetFilter(fltr_2);
        hkpAllRayHitCollector collector_2;
        pd_2.SetAllCollector(&collector_2);
        pd_2.NiSetFrom(origin_2);
        SceneGraph* v35 = Main::GetWorldSceneGraph();
        pd_2.NiSetTo(v35->GetCamera()->GetWorldDirection());
        if (binterior) {
            v60 = fSneakMaxDistance.Float();
        }
        else {
            float* v37 = &fSneakExteriorDistanceMult.Float();
            v60 = *v37 * fSneakMaxDistance.Float();
        }
        float v59 = v60;
        SceneGraph* v38 = Main::GetWorldSceneGraph();
        pd_2.NiSetLength(v38->GetCamera()->GetWorldDirection() * v59);
        bhkWorld* v56 = pPlayer->GetParentCell()->GetbhkWorld();
        if ((v56->PickObject(pd_2)) && (pd_2.pAllCollector)) {
            for (int32_t i_2 = 0; !bvalidTargetsFound; ++i_2) {
                int32_t v41 = (&pd_2.pAllCollector->getHits())->m_size;
                if (i_2 >= v41) {
                    break;
                }
                const hkpWorldRayCastOutput* v42 =
                    &(&pd_2.pAllCollector->getHits())->operator[]<hkpWorldRayCastOutput>(i_2);
                hkpWorldRayCastOutput point_2(*v42);
                NiAVObject* pav_2 = GetAVObjectForCollidable(point_2.m_rootCollidable);
                if (pav_2) {
                    TESObjectREFR* phitRef_2 = TESObjectREFR::FindReferenceFor3D(pav_2);
                    if ((phitRef_2) &&
                        (BGSDestructibleObjectForm::IsDestructible(phitRef_2->GetObjectReference()) &&
                         phitRef_2->IsProjectile())) {
                        bvalidTargetsFound = true;
                    }
                }
            }
        }
    }
    if (!bvalidTargetsFound) {
        VATSMenu::ClearTargetList();
    }
    return bvalidTargetsFound;
}

void VATSMenu::SetupSpecialAttackButtons(TESObjectWEAP* pweapon) {
    ActorValue::Index WeaponSkill;
    double VATSSkill;
    if (pweapon &&
        (pweapon->GetVATSAP() > 0.0 &&
         (VATSSkill = pweapon->GetVATSSkill(),
          (WeaponSkill = pweapon->GetWeaponSkill(),
           static_cast<double>(pPlayer->GetActorValue(WeaponSkill)) >= VATSSkill)))) {
        const char* VATSAttackName = pweapon->GetVATSAttackName();
        VATSMenu::pVatsMenu->pTiles[30]->Set(4036, VATSAttackName, true);
        VATSMenu::pVatsMenu->pTiles[30]->Set(4003, 1);
        bWeaponHasSpecial = 1;
        VATSMenu::pVatsMenu->pTiles[31]->Set(4003, 0);
        bWeaponHasSpecial2 = 0;
        fSpecialAttackDamageMult = pweapon->specialVATSData.fVATSSpecialMultiplier;
        fSpecialAttackAPCost = pweapon->GetVATSAP();
        BGSEntryPoint::HandleEntryPoint(
            BGSEntryPoint::ACTION_POINT_COST_MODIFIER_ENTRY, pPlayer, pweapon, &fSpecialAttackAPCost
        );
    }
    else {
        double v9;
        if ((pweapon && pweapon->GetWeaponSkill() != ActorValue::UNARMED) ||
            (v9 = static_cast<double>(pPlayer->GetActorValue(ActorValue::UNARMED)),
             !(fUpperCutThreshold.Float() <= v9))) {
            VATSMenu::pVatsMenu->pTiles[30]->Set(4003, 0);
            bWeaponHasSpecial = 0;
            VATSMenu::pVatsMenu->pTiles[31]->Set(4003, 0);
            bWeaponHasSpecial = 0;
        }
        else {
            double v7;
            TESObjectWEAP* v8;
            char v11 = 0;
            if (pVATSTarget &&
                (pVATSTarget->IsKnockedOut() ||
                 (pVATSTarget->IsActor() && static_cast<Actor*>(pVATSTarget)->Actor::IsUnConscious()))) {
                v11 = 1;
                const char* v3 = sVatsUnarmedAttackGround.String();
                VATSMenu::pVatsMenu->pTiles[30]->Set(4036, v3, true);
                VATSMenu::pVatsMenu->pTiles[30]->Set(4003, 1);
            }
            else {
                const char* v4 = sVatsUnarmedAttack1.String();
                VATSMenu::pVatsMenu->pTiles[30]->Set(4036, v4, true);
                VATSMenu::pVatsMenu->pTiles[30]->Set(4003, 1);
            }
            bWeaponHasSpecial = 1;
            fSpecialAttackDamageMult = 1.0f;
            fSpecialAttackAPCost = fUpperCutAPCost.Float();
            if (pweapon) {
                v8 = pweapon;
            }
            else {
                v8 = pFists;
            }
            BGSEntryPoint::HandleEntryPoint(
                BGSEntryPoint::ACTION_POINT_COST_MODIFIER_ENTRY, pPlayer, v8, &fSpecialAttackAPCost
            );
            if (v11 ||
                (v7 = static_cast<double>(pPlayer->GetActorValue(ActorValue::UNARMED)),
                 !(fCrossThreshold.Float() <= v7))) {
                VATSMenu::pVatsMenu->pTiles[31]->Set(4003, 0);
                bWeaponHasSpecial2 = 0;
            }
            else {
                TESObjectWEAP* v6;
                const char* v5 = sVatsUnarmedAttack2.String();
                VATSMenu::pVatsMenu->pTiles[31]->Set(4036, v5, true);
                VATSMenu::pVatsMenu->pTiles[31]->Set(4003, 1);
                bWeaponHasSpecial2 = 1;
                fSpecialAttack2DamageMult = 1.0f;
                fSpecialAttack2APCost = fCrossAPCost.Float();
                if (pweapon) {
                    v6 = pweapon;
                }
                else {
                    v6 = pFists;
                }
                BGSEntryPoint::HandleEntryPoint(
                    BGSEntryPoint::ACTION_POINT_COST_MODIFIER_ENTRY, pPlayer, v6, &fSpecialAttack2APCost
                );
            }
        }
    }
}

int32_t VATSMenu::SortAngle(VATSTarget* apObject1, VATSTarget* apObject2) {
    double v6;
    double v9;
    NiPoint3 cPlayerLoc = pPlayer->GetLocationOnReference();
    float v15 = pPlayer->GetHeading(false) * 57.2957763671875;
    const NiPoint3* v2 = &apObject1->pTargetRef->GetLocationOnReference();
    float fobject1Heading = HUDMainMenu::HeadingBetweenLocations(cPlayerLoc, *v2);
    const NiPoint3* v3 = &apObject2->pTargetRef->GetLocationOnReference();
    float fobject2Heading = HUDMainMenu::HeadingBetweenLocations(cPlayerLoc, *v3);
    float v10 = v15 - fobject1Heading;
    float v17 = std::fabs(v10);
    if (!(v17 > 180.0)) {
        v9 = v17;
    }
    else {
        v9 = 360.0 - v17;
    }
    float v8 = v9;
    float v12 = std::fabs(v8);
    float v7 = v15 - fobject2Heading;
    v17 = std::fabs(v7);
    if (!(v17 > 180.0)) {
        v6 = v17;
    }
    else {
        v6 = 360.0 - v17;
    }
    float v5 = v6;
    float v14 = std::fabs(v5);
    if (!(v14 > static_cast<double>(v12))) {
        return v14 < static_cast<double>(v12);
    }

    return -1;
}

// The headings are rounded to float; a NaN heading sorts as equal.
int32_t VATSMenu::SortDirection(VATSTarget* apObject1, VATSTarget* apObject2) {
    NiPoint3 kPlayerLocation = pPlayer->GetLocationOnReference();
    float fHeading1 =
        HUDMainMenu::HeadingBetweenLocations(kPlayerLocation, apObject1->pTargetRef->GetLocationOnReference());
    float fHeading2 =
        HUDMainMenu::HeadingBetweenLocations(kPlayerLocation, apObject2->pTargetRef->GetLocationOnReference());
    if (fHeading2 > fHeading1) {
        return -1;
    }
    if (fHeading1 > fHeading2) {
        return 1;
    }
    return 0;
}

Tile* VATSMenu::VatsMenuRenderTemplate(Tile* apParent, const char* axName) {
    if (!VATSMenu::pVatsMenu) {
        return nullptr;
    }
    Tile* pTile = VATSMenu::pVatsMenu->RenderTemplate(apParent, axName, nullptr);
    if (pTile) {
        return pTile;
    }
    Warning(
        "MENUS: VATS Menu Creation Failed... This likely means that the archives were not built and the xml "
        "doesn't match the code."
    );
    Interface::EmergencyCloseAllMenusAndBreakStuff();
    VATSMenu::Close();
    return nullptr;
}

Tile* VATSMenu::VatsMenuGetTileByName(Tile* apTarget, const char* apName) {
    Tile* pTile = Tile::GetTileByName(apTarget, apName);
    if (pTile) {
        return pTile;
    }
    Warning(
        "MENUS: VATS Menu Creation Failed... This likely means that the archives were not built and the xml "
        "doesn't match the code."
    );
    Interface::EmergencyCloseAllMenusAndBreakStuff();
    VATSMenu::Close();
    return nullptr;
}

// VATSBodyPart::eBodyPart and uiLastBodyPart are uint32_t; they are passed on as the LIMB_ENUM the callees take.
void VATSMenu::ShowTargetLimb(bool abForceShow) {
    XFNV_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    if (!VATSMenu::pVatsMenu) {
        return;
    }
    BSSimpleList<VATSTarget*>* ptargetList = VATSMenu::pVatsMenu->GetVatsTarget(pVATSTarget);
    if (!abForceShow) {
        if (pVATS.eMode != VATS::VATS_MENU_MODE) {
            return;
        }
        ActorValue::Index eLastLimb = VATSMenu::pVatsMenu->GetActorValueFromBodyPart(
            static_cast<BGSBodyPart::LIMB_ENUM>(VATSMenu::uiLastBodyPart)
        );
        if (eLastLimb ==
                VATSMenu::pVatsMenu->GetActorValueFromBodyPart(
                    static_cast<BGSBodyPart::LIMB_ENUM>(VATSMenu::pVatsMenu->pCurrentLimb->eBodyPart)
                ) &&
            VATSMenu::pLastTarget == pVATSTarget) {
            return;
        }
    }
    if (!abForceShow && VATSMenu::pLastTarget == pVATSTarget) {
        BaseProcess* pProcess = pPlayer->pCurrentProcess;
        if (!pProcess->IsCurrentWeaponGrenade()) {
            BSSoundHandle psoundHand;
            BSAudio* pAudio = &BSAudio::QInstance();
            BSSoundHandle* arHandle = pAudio->GetSoundHandleByName(&a1, "UIVATSSelectTargetPart", 289u);
            psoundHand.operator=(*arHandle);
            psoundHand.Play(false);
        }
    }
    VATSMenu::pVatsMenu->fProbableEnemyDamage = VATSMenu::pVatsMenu->CalculateProbableEnemyHealth();
    Actor* pactor = nullptr;
    if (ptargetList && ptargetList->m_item && ptargetList->m_item->pTargetRef->IsActor()) {
        pactor = static_cast<Actor*>(ptargetList->m_item->pTargetRef);
    }
    ItemChange* pweaponItem = pPlayer->pCurrentProcess->GetCurrentWeapon();
    TESObjectWEAP* pweapon;
    if (pweaponItem) {
        pweapon = static_cast<TESObjectWEAP*>(pweaponItem->pContainerObj);
    }
    else {
        pweapon = nullptr;
    }
    if (ptargetList &&
        (ptargetList->m_item->eType == 2 || pPlayer->pCurrentProcess->IsCurrentWeaponGrenade() || !pweapon ||
         pweapon->IsMeleeWeapon())) {
        if (abForceShow || (pVATS.eMode == VATS::VATS_MENU_MODE && VATSMenu::pLastTarget != pVATSTarget)) {
            if (pVATS.eMode == VATS::VATS_SCAN_MODE) {
                BGSBodyPart::LIMB_ENUM eLimb =
                    static_cast<BGSBodyPart::LIMB_ENUM>(VATSMenu::pVatsMenu->pCurrentLimb->eBodyPart);
                TESObjectREFR* pTarget = ptargetList->m_item->pTargetRef;
                Interface::GetVATSEffect()->SetTarget(pTarget, eLimb, true);
            }
            else {
                TESObjectREFR* pTarget = ptargetList->m_item->pTargetRef;
                Interface::GetVATSEffect()->SetTarget(pTarget, BGSBodyPart::LIMB_COUNT, false);
            }
            VATSMenu::pLastTarget = ptargetList->m_item->pTargetRef;
        }
        VATSMenu::uiLastBodyPart = VATSMenu::pVatsMenu->pCurrentLimb->eBodyPart;
        return;
    }
    if (!pactor || !pactor->pCurrentProcess) {
        return;
    }
    if (VATSMenu::pVatsMenu->pCurrentLimb->bFirstTimeShown || pVATS.eMode != VATS::VATS_SCAN_MODE) {
        BGSBodyPart::LIMB_ENUM eLimb =
            static_cast<BGSBodyPart::LIMB_ENUM>(VATSMenu::pVatsMenu->pCurrentLimb->eBodyPart);
        TESObjectREFR* pTarget = pVATSTarget;
        Interface::GetVATSEffect()->SetTarget(pTarget, eLimb, false);
    }
    else {
        BGSBodyPart::LIMB_ENUM eLimb =
            static_cast<BGSBodyPart::LIMB_ENUM>(VATSMenu::pVatsMenu->pCurrentLimb->eBodyPart);
        TESObjectREFR* pTarget = pVATSTarget;
        Interface::GetVATSEffect()->SetTarget(pTarget, eLimb, true);
    }
    // Targets every other part of the target that maps to the current limb's actor value as well.
    for (BSSimpleList<VATSBodyPart*>* piter = &VATSMenu::pVatsMenu->GetVatsTarget(pVATSTarget)->m_item->kBodyParts;
         piter && !piter->IsEmpty(); piter = piter->m_pkNext) {
        ActorValue::Index ePartLimb = VATSMenu::pVatsMenu->GetActorValueFromBodyPart(
            static_cast<BGSBodyPart::LIMB_ENUM>(piter->m_item->eBodyPart)
        );
        if (ePartLimb ==
                VATSMenu::pVatsMenu->GetActorValueFromBodyPart(
                    static_cast<BGSBodyPart::LIMB_ENUM>(VATSMenu::pVatsMenu->pCurrentLimb->eBodyPart)
                ) &&
            piter->m_item != VATSMenu::pVatsMenu->pCurrentLimb) {
            BGSBodyPart::LIMB_ENUM eLimb = static_cast<BGSBodyPart::LIMB_ENUM>(piter->m_item->eBodyPart);
            Interface::GetVATSEffect()->TargetAdditionalLimb(eLimb);
        }
    }
    VATSMenu::uiLastBodyPart = VATSMenu::pVatsMenu->pCurrentLimb->eBodyPart;
    VATSMenu::pLastTarget = pVATSTarget;
}

uint8_t VATSMenu::GetShotBurstCount(ActionPoints::Action aeAction, bool abAllowLongBurst) {
    float fburstTime;
    int32_t iNumAmmoPerShot;
    TESObjectWEAP* pweapon;
    BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
    ItemChange* pweaponItem = CurrentAIProcess->GetCurrentWeapon();
    if (pweaponItem) {
        pweapon = static_cast<TESObjectWEAP*>(pweaponItem->pContainerObj);
    }
    else {
        pweapon = nullptr;
    }
    if (pweapon) {
        iNumAmmoPerShot = pweapon->GetAmmoPerShot();
    }
    else {
        iNumAmmoPerShot = 0;
    }
    uint8_t v10 = 1;
    if (iNumAmmoPerShot > 0) {
        v10 = iNumAmmoPerShot;
    }
    if (aeAction == ActionPoints::ACTION_ATTACK_UNARMED || (pweapon && !pweapon->IsAutomaticWeapon())) {
        return v10;
    }
    if (abAllowLongBurst && (pweapon && pweapon->IsLongBurst())) {
        fburstTime = fVATSShotLongBurstTime.Float();
    }
    else {
        fburstTime = fVATSShotBurstTime.Float();
    }
    double v4 = pweapon->data.fShotsPerSec * fburstTime;
    bool HasModEffectActive_0 = pweaponItem->HasModEffectActive(8);
    uint8_t v11 = static_cast<int32_t>(pweapon->GetAttackSpeed(HasModEffectActive_0) * v4);
    if (!v11) {
        return 1;
    }
    return v11;
}

bool VATSMenu::SelectLeftHeld() {
    if (!VATSMenu::pVatsMenu) {
        return false;
    }
    return VATSMenu::pVatsMenu->bSelectLeftHeld && appTimer.GetTime() - VATSMenu::pVatsMenu->uiSelectTimer > 500;
}

bool VATSMenu::SelectRightHeld() {
    if (!VATSMenu::pVatsMenu) {
        return false;
    }
    return VATSMenu::pVatsMenu->bSelectRightHeld && appTimer.GetTime() - VATSMenu::pVatsMenu->uiSelectTimer > 500;
}

void VATSMenu::ClearTargetList() {
    while (VATSMenu::xTargetList.m_item) {
        VATSTarget* i;
        for (i = VATSMenu::xTargetList.m_item; i->kBodyParts.m_item; i->kBodyParts.RemoveHead()) {
            void* Block = i->kBodyParts.m_item;
            ourFree(Block);
        }
        if (i) {
            delete i;
        }
        VATSMenu::xTargetList.RemoveHead();
    }
}

void VATSMenu::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void VATSMenu::SetLimb(BGSBodyPart::LIMB_ENUM aeLimb) {
    if (VATSMenu::pVatsMenu) {
        for (BSSimpleList<VATSBodyPart*>* plist = &VATSMenu::pVatsMenu->GetVatsTarget(pVATSTarget)->m_item->kBodyParts;
             plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
            if (plist->m_item && plist->m_item->eBodyPart == aeLimb) {
                BSSimpleList<VATSBodyPart*>* v1 = plist;
                VATSMenu::pVatsMenu->pCurrentLimb = v1->m_item;
                return;
            }
        }
    }
}

void VATSMenu::RemoveTarget(TESObjectREFR* apRef) {
    if (!VATSMenu::xTargetList.IsEmpty()) {
        for (BSSimpleList<VATSTarget*>* plist = &VATSMenu::xTargetList; plist && !plist->IsEmpty();
             plist = plist->m_pkNext) {
            if (plist->m_item->pTargetRef == apRef) {
                if ((pVATSTarget == apRef) && (VATSMenu::pVatsMenu)) {
                    VATSMenu::pVatsMenu->DoClick(2, nullptr);
                }

                VATSMenu::xTargetList.Remove(plist->m_item);
                if ((VATSMenu::xTargetList.IsEmpty()) && (VATSMenu::pVatsMenu)) {
                    VATSMenu::Close();
                }
            }
        }
        VATSTarget* pcurrItem = (&VATSMenu::xTargetList)->m_item;
    }
}

bool VATSMenu::FilterDestructibleObjects(TESObjectREFR* apRef, void* aiFilterData) {
    const NiPoint3* v7 = &apRef->GetLocationOnReference();
    const NiPoint3* v2 = &pPlayer->GetLocationOnReference();
    float fdist = (*v2 - *v7).Length();
    if (pPlayer->GetCurrentWeapon()) {
        TESObjectWEAP* CurrentWeapon = pPlayer->GetCurrentWeapon();
        if (CurrentWeapon->IsMeleeWeapon()) {
            float* v5 = &fVATSMeleeMaxDistance.Float();
            if (*v5 < static_cast<double>(fdist)) {
                return false;
            }
        }
    }
    if (apRef->GetDestructible() && apRef->IsProjectile()) {
        Projectile* pproj = static_cast<Projectile*>(apRef);
        if (pproj->pWeaponSource &&
            (pproj->pWeaponSource->GetWeaponType() == WEAPON_ONE_HAND_MINE ||
             pproj->pWeaponSource->GetWeaponType() == WEAPON_ONE_HAND_LUNCHBOX_MINE)) {
            return false;
        }
        if (pproj->GetShooter() != pPlayer) {
            VATSMenu::Destructibles.AddHead(apRef);
        }
    }
    return false;
}

bool IsDesiredAttackControlPressed(DesiredAttackType desiredAttack, bool held, int32_t aiMouseKey) {
    InterfaceManager::Instance();
    if (InterfaceManager::Has360Controller()) {
        Interface::enumGamepadButtons button = Interface::eButtonRT;
        if (desiredAttack == DESIRED_ATTACK_SPECIAL) {
            button = Interface::eButtonY;
        }
        else {
            if (desiredAttack == DESIRED_ATTACK_SPECIAL_2) {
                button = Interface::eButtonX;
            }
        }
        if (held) {
            return InterfaceManager::CheckMenuButton(button, true) == 0;
        }

        return InterfaceManager::CheckMenuButton(button, true) == 2;
    }

    Controls* Controls_ = appMain->GetControls();
    // The toggle flags Controls' button tests take: 1 for a press, 0 while held.
    int32_t iToggleFlags = !held;
    if (desiredAttack == DESIRED_ATTACK_NORMAL) {
        return Controls_->GetMouseButton(0, iToggleFlags) && aiMouseKey;
    }

    if (desiredAttack == DESIRED_ATTACK_SPECIAL) {
        return Controls_->GetKeyboardButton(19, iToggleFlags) || bWeaponSpecialSelected;
    }

    return desiredAttack == DESIRED_ATTACK_SPECIAL_2 &&
        (Controls_->GetKeyboardButton(33, iToggleFlags) || g_bIsVATSSpecialAttack2);
}

BSSimpleList<VATS_COMMAND*>* VATS::GetCommandList() {
    return &commandList;
}
