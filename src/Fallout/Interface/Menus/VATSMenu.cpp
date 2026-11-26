#include "Fallout/Interface/Menus/VATSMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSDevices/Controls.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
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
#include <limits>

// DoIdle's function-local statics: the attack held since an earlier frame, the screen size and safe zone read
// once, and their guard.
static DesiredAttackType heldAttack{};
static int32_t isafeZoneY;
static int32_t isafeZoneX;
static int32_t iscreenHeight;
static int32_t iscreenWidth;

// The guard bits (1 iscreenWidth, 2 iscreenHeight, 4 isafeZoneX, 8 isafeZoneY, 0x10 the _Brightness user trait).
static uint32_t uiDoIdleStaticsGuard;

// The game settings this file defines, with the game's IDs and values, in initialization order.
SettingT<INISettingCollection> bVatsAlwaysHit("bVatsAlwaysHit:Combat", false);
BSSimpleList<TESObjectREFR*> VATSMenu::Destructibles;
BSSimpleList<VATSTarget*> VATSMenu::xTargetList;

VATSMenu::~VATSMenu() {
    pFaderManager->RemoveFader(FADER_BLACK_UNDER_MENUS, false);
    ClearTargetList();
    TileMenu* pMenuTile = VATSMenu::pVatsMenu->pMenu;
    if (pMenuTile) {
        delete pMenuTile;
    }
    VATSMenu::pVatsMenu = nullptr;
}

void VATSMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (aiID > 0 && aiID <= 31) {
        pTiles[aiID] = apTile;
    }
}

// Clicks on the VATS menu: the shoulder buttons pick the nearest target to the left or right; IDs 2 and 3 step to the
// previous or next target with a body to show, 4 closes the menu, 5 queues the current action, 30 and 31 select the
// weapon's special attacks.
void VATSMenu::DoClick(int32_t aiID, Tile* apTarget) {
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
                kSound = BSAudio::QInstance().GetSoundHandleByName(
                    "UIVATSEnterFail",
                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                        BSGameSound::SOUND_FLAG_ONE_SHOT
                );
                kSound.Play(false);
                return;
            }
            kSound = BSAudio::QInstance().GetSoundHandleByName(
                "UIVATSSelectTarget",
                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
            );
            kSound.Play(false);
            // Puts every target's meters back to full alpha.
            for (BSSimpleList<VATSTarget*>* pIter = &VATSMenu::xTargetList; pIter && !pIter->IsEmpty();
                 pIter = pIter->m_pkNext) {
                for (BSSimpleList<VATSBodyPart*>* pPart = &pIter->m_item->kBodyParts;
                     pPart && !pPart->IsEmpty() && pPart->m_item->pMeter;
                     pPart = pPart->m_pkNext) {
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
                     pPart && !pPart->IsEmpty();
                     pPart = pPart->m_pkNext) {
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
                kSound = BSAudio::QInstance().GetSoundHandleByName(
                    "UIVATSEnterFail",
                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                        BSGameSound::SOUND_FLAG_ONE_SHOT
                );
                kSound.Play(false);
                return;
            }
            kSound = BSAudio::QInstance().GetSoundHandleByName(
                "UIVATSSelectTarget",
                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
            );
            kSound.Play(false);
            // Puts every target's meters back to full alpha.
            for (BSSimpleList<VATSTarget*>* pIter = &VATSMenu::xTargetList; pIter && !pIter->IsEmpty();
                 pIter = pIter->m_pkNext) {
                for (BSSimpleList<VATSBodyPart*>* pPart = &pIter->m_item->kBodyParts;
                     pPart && !pPart->IsEmpty() && pPart->m_item->pMeter;
                     pPart = pPart->m_pkNext) {
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
                if (pIter->m_pkNext &&
                    pIter->m_pkNext->m_item->pTargetRef &&
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
                     pPart && !pPart->IsEmpty();
                     pPart = pPart->m_pkNext) {
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
    if (pVATSTarget && pVATSTarget->IsActor()) {
        eVATSTargetLimb = ActorValue::PERCEPTION_CONDITION;
    }

    VATSMenu::SetupSpecialAttackButtons(pWeapon);
    SetTargetLimb();
    if (pVATS.GetVATSLight() && pVATSTarget) {
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
                bMouseOverButton = 1;
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
                bMouseOverButton = 0;
                break;
            }
            default: {
                return;
            }
        }
    }
}

void VATSMenu::DoStillDown(int32_t aiID, Tile* apTarget) {
    if (aiID == 2) {
        if (!bSelectLeftHeld) {
            uiSelectTimer = appTimer.GetTime();
            bSelectLeftHeld = true;
        }
    }
    else if (aiID == 3 && !bSelectRightHeld) {
        uiSelectTimer = appTimer.GetTime();
        bSelectRightHeld = true;
    }
}

void VATSMenu::DoWheelMove(int32_t aiID, Tile* apTarget) {
    if (InterfaceManager::Instance()->GetMouseWheel() / -120 == 1) {
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
    if (!VATSMenu::pVatsMenu ||
        pVATS.GetMode() == VATS::VATS_MODE_NONE ||
        pVATS.GetMode() == VATS::VATS_PLAYBACK ||
        !Interface::IsTopMenuID(Interface::eVATSMenu)) {
        if (!Interface::IsTopMenuID(Interface::eVATSMenu) && bLimbLit) {
            bLimbLit = false;
            Interface::GetVATSEffect()->SetTarget(nullptr, BGSBodyPart::LIMB_NONE, false);
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
        int32_t iRightX = pControls->GetControllerValue(0, 9);
        int32_t iRightY = pControls->GetControllerValue(0, 10);
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
        // The conversion to ActorValueOwner is a reference conversion with no null check.
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
                this->pTiles[26],
                this->pTiles[28],
                fHealthPercent - this->fProbableEnemyDamage,
                fHealthPercent,
                HUDMainMenu::HJ_CENTER,
                0
            );
        }
        else {
            HUDMainMenu::UpdateMeter(
                this->pTiles[26],
                this->pTiles[28],
                fHealthPercent - this->fProbableEnemyDamage,
                fHealthPercent,
                HUDMainMenu::HJ_CENTER,
                2
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
            // The arguments are evaluated right to left.
            const char* pThresholdLabel = sInventoryDamageThreshold.String();
            int32_t iHealth = static_cast<int32_t>(fHealth);
            const char* pHPLabel = sStatsHP.String();
            const char* pName = pVATSTarget->GetFullName();
            std::sprintf(
                acText,
                "%s\n %s:%d %s:%.2f",
                pName,
                pHPLabel,
                iHealth,
                pThresholdLabel,
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
         static_cast<uint32_t>(pVATS.GetCommandList()->ItemsInList()) > 0 &&
         pVATS.GetMode() == VATS::VATS_MENU_MODE)) {
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
    float fRequiredActionPoints = ActionPoints::GetWeaponAttackActionPoints(pWeapon, true, false);
    float fRequiredHeldActionPoints = fRequiredActionPoints;
    if (desiredAttack == DESIRED_ATTACK_SPECIAL) {
        fRequiredActionPoints = fSpecialAttackAPCost;
    }
    else if (desiredAttack == DESIRED_ATTACK_SPECIAL_2) {
        fRequiredActionPoints = fSpecialAttack2APCost;
    }
    if (heldAttack == DESIRED_ATTACK_SPECIAL) {
        fRequiredHeldActionPoints = fSpecialAttackAPCost;
    }
    else if (heldAttack == DESIRED_ATTACK_SPECIAL_2) {
        fRequiredHeldActionPoints = fSpecialAttack2APCost;
    }
    if (PlayerCharacter::IsGodMode()) {
        fRequiredActionPoints = 0.0f;
        fRequiredHeldActionPoints = 0.0f;
    }
    this->UpdateAmmo();
    if (!(uiDoIdleStaticsGuard & 0x10)) {
        uiDoIdleStaticsGuard |= 0x10;
        Tile::AddUserTrait("_Brightness", -1);
    }
    // The held attack's cost shows only while the AP total is below it (not NaN).
    if (pVATSTarget && this->fActionPoints < fRequiredHeldActionPoints && heldAttack != DESIRED_ATTACK_NONE) {
        HUDMainMenu::UpdateMeter(
            this->pTiles[9],
            this->pTiles[10],
            fRequiredHeldActionPoints / this->fActionPointsBase,
            0.0f,
            HUDMainMenu::HJ_RIGHT,
            0
        );
    }
    else {
        float fGhostMeter = this->fActionPoints / this->fActionPointsBase;
        float fMeter = static_cast<float>(
            (static_cast<double>(this->fActionPoints) - fRequiredHeldActionPoints) / this->fActionPointsBase
        );
        HUDMainMenu::UpdateMeter(this->pTiles[9], this->pTiles[10], fMeter, fGhostMeter, HUDMainMenu::HJ_RIGHT, 0);
        this->pTiles[12]->Set(Tile::kTileValue_Visible, 0);
    }
    if (!pVATSTarget || desiredAttack == DESIRED_ATTACK_NONE) {
        // 0x1E: DIK_A, 0x20: DIK_D.
        InterfaceManager::Instance();
        if (!InterfaceManager::Has360Controller() &&
            pControls->GetKeyboardButton(0x1E, Controls::isPressed) &&
            pControls->GetKeyboardButton(0x1E, Controls::isChanged) &&
            iKeyboardKey != 0x1E) {
            this->DoClick(2, nullptr);
            return;
        }
        InterfaceManager::Instance();
        if (!InterfaceManager::Has360Controller() &&
            pControls->GetKeyboardButton(0x20, Controls::isPressed) &&
            pControls->GetKeyboardButton(0x20, Controls::isChanged) &&
            iKeyboardKey != 0x20) {
            this->DoClick(3, nullptr);
        }
        return;
    }
    // The flag is stored as a byte.
    uint8_t bNewTarget = !InterfaceManager::Has360Controller() && this->PickNewTarget();
    // A chance of zero or less (or NaN) attacks nothing.
    if (bNewTarget || !(this->pCurrentLimb->fChance > 0.0)) {
        if (!bNewTarget &&
            this->pCurrentLimb->bChanceCalculated &&
            pVATS.GetMode() == VATS::VATS_MENU_MODE &&
            this->bHeadingReached) {
            BSAudioManager* mgr = &BSAudioManager::QInstance();
            BSSoundHandle soundHand;
            soundHand = mgr->GetSoundHandleByName(
                "UIVATSInsufficientAP",
                static_cast<BSGameSound::SoundFlags>(
                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                    BSGameSound::SOUND_FLAG_ONE_SHOT
                )
            );
            soundHand.Play(false);
            // Only an exact zero chance (not NaN) shows the message.
            if (this->pCurrentLimb->fChance == 0.0) {
                HUDMainMenu::pme
                    ->ShowMessage(sVATSMessageZeroChance.String(), HUDMainMenu::HME_SAD, nullptr, nullptr, 2.0f, false);
            }
        }
        return;
    }
    InterfaceManager::Instance();
    if (!((InterfaceManager::Has360Controller() && pVATS.GetMode() == VATS::VATS_MENU_MODE) ||
          (InterfaceManager::Instance(),
           !InterfaceManager::Has360Controller() &&
               pVATS.GetMode() == VATS::VATS_MENU_MODE &&
               !(this->bMouseOverButton && desiredAttack == DESIRED_ATTACK_NORMAL)))) {
        InterfaceManager::Instance();
        if (!InterfaceManager::Has360Controller() &&
            pVATS.GetMode() == VATS::VATS_TARGET_SELECT &&
            !this->bMouseOverButton) {
            this->PickNewTarget();
        }
        return;
    }
    ActionPoints::Action eAction = ActionPoints::GetAttackAction(
        static_cast<WEAPON_TYPE>(pWeapon ? pWeapon->GetWeaponType() : 0)
    );
    uint8_t cShotBurst = VATSMenu::GetShotBurstCount(eAction, false);
    // A clip that holds no more than the burst (not NaN) adds the reload's AP.
    if (this->fQueuedClipAmmo <= cShotBurst &&
        pWeapon &&
        pWeapon->GetCurrentAmmo(pPlayer) &&
        pWeapon->GetFormClipRounds(false) > 1) {
        fRequiredActionPoints = ActionPoints::GetRequiredAPForAction(ActionPoints::ACTION_RELOAD, true) +
            fRequiredActionPoints;
    }
    ItemChange* pAmmo = pPlayer->GetCurrentAIProcess()->GetCurrentAmmo();
    // The flag is stored as a byte.
    uint8_t bHaveGrenadeAmmo;
    if (!pWeapon) {
        bHaveGrenadeAmmo = false;
    }
    else if (
        pWeapon->GetWeaponType() == WEAPON_ONE_HAND_GRENADE || pWeapon->GetWeaponType() == WEAPON_ONE_HAND_THROWN
    ) {
        if (!pAmmo) {
            bHaveGrenadeAmmo = false;
        }
        else {
            // The ammo count and the queued commands compare unsigned.
            uint32_t uiCount = static_cast<uint32_t>(pAmmo->GetCount());
            uint32_t uiQueued = static_cast<uint32_t>(pVATS.GetCommandList()->ItemsInList());
            bHaveGrenadeAmmo = uiCount > uiQueued;
        }
    }
    else {
        bHaveGrenadeAmmo = true;
    }
    // The sums must be above zero and the cost no more than the AP total, NaN failing both; the chance is the
    // truncation of the double product.
    if ((pWeapon &&
         (pWeapon->GetWeaponType() == WEAPON_ONE_HAND_GRENADE || pWeapon->GetWeaponType() == WEAPON_ONE_HAND_THROWN) &&
         !bHaveGrenadeAmmo) ||
        (bRockItLauncher && !(this->fQueuedTotalAmmo + this->fQueuedClipAmmo > 0.0)) ||
        eAction == ActionPoints::ACTION_COUNT ||
        !(fRequiredActionPoints <= this->fActionPoints) ||
        (pWeapon && pWeapon->GetCurrentAmmo(pPlayer) && !(this->fQueuedTotalAmmo + this->fQueuedClipAmmo > 0.0)) ||
        (pWeapon && pWeapon->GetCurrentAmmo(pPlayer) && (!pAmmo || pAmmo->GetCount() <= 0)) ||
        (this->GetTargetPart() &&
         static_cast<int32_t>(static_cast<double>(this->GetTargetPart()->GetItem()->fChance) * 100.0) <= 0)) {
        if (this->GetTargetPart() &&
            static_cast<int32_t>(static_cast<double>(this->GetTargetPart()->GetItem()->fChance) * 100.0) <= 0 &&
            pVATS.GetMode() == VATS::VATS_MENU_MODE &&
            this->bHeadingReached) {
            BSSoundHandle soundHand;
            soundHand = BSAudio::QInstance().GetSoundHandleByName(
                "UIVATSEnterFail",
                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
            );
            soundHand.Play(false);
            HUDMainMenu::pme
                ->ShowMessage(sVATSMessageZeroChance.String(), HUDMainMenu::HME_SAD, nullptr, nullptr, 2.0f, false);
        }
        else if (fRequiredActionPoints > this->fActionPoints) {
            // Only a cost above the AP total (not NaN) plays the insufficient AP sound.
            BSAudioManager* mgr = &BSAudioManager::QInstance();
            BSSoundHandle soundHand;
            soundHand = mgr->GetSoundHandleByName(
                "UIVATSInsufficientAP",
                static_cast<BSGameSound::SoundFlags>(
                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                    BSGameSound::SOUND_FLAG_ONE_SHOT
                )
            );
            soundHand.Play(false);
            this->pTiles[12]->Set(Tile::kTileValue_Visible, 1);
        }
        else if (this->fQueuedTotalAmmo + this->fQueuedClipAmmo == 0.0) {
            // Only an exact zero (not NaN) reports no ammo.
            HUDMainMenu::pme
                ->ShowMessage(sVATSMessageNoAmmo.String(), HUDMainMenu::HME_SAD, nullptr, nullptr, 2.0f, false);
            BSSoundHandle soundHand;
            soundHand = BSAudio::QInstance().GetSoundHandleByName(
                "UIVATSEnterFail",
                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
            );
            soundHand.Play(false);
        }
    }
    else {
        VATS_COMMAND* pCommand = new VATS_COMMAND;
        Actor* pTargetActor = static_cast<Actor*>(pVATSTarget);
        if (this->GetTargetPart()) {
            this->pLastLimb = this->GetTargetPart()->GetItem();
        }
        // The flag is stored as a byte.
        uint8_t bLimbRandomlySelected = false;
        if (!pWeapon || pWeapon->IsMeleeWeapon()) {
            float fDieRoll = RandomFloat(0.0f, 100.0f);
            int32_t iCount = 0;
            // BODY_PART_TYPE_NONE.
            uint32_t uiPart = 0xFFFFFFFF;
            // The parts counted are body part types 0-6; 1 and 2 (the torso and head) are also remembered.
            for (BSSimpleList<VATSBodyPart*>* pParts = &this->GetVatsTarget(pVATSTarget)->GetItem()->kBodyParts;
                 pParts && !pParts->IsEmpty();
                 pParts = pParts->GetNext()) {
                switch (pParts->GetItem()->eBodyPart) {
                    case 1:
                    case 2:
                        uiPart = pParts->GetItem()->eBodyPart;
                        ++iCount;
                        break;
                    case 0:
                    case 3:
                    case 4:
                    case 5:
                    case 6: ++iCount; break;
                    default: break;
                }
            }
            if (!pWeapon && desiredAttack == DESIRED_ATTACK_SPECIAL && uiPart != 0xFFFFFFFF) {
                pCommand->eLimb = this->GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(uiPart));
                bLimbRandomlySelected = true;
            }
            else if (iCount > 0) {
                float fRange = static_cast<float>(100.0 / iCount);
                // Divides and truncates; a quotient of two floats rounds to no integer in double either.
                int32_t iLimb = static_cast<int32_t>(static_cast<double>(BSMAX(fDieRoll, 1.0f)) / fRange);
                iCount = 0;
                for (BSSimpleList<VATSBodyPart*>* pParts = &this->GetVatsTarget(pVATSTarget)->GetItem()->kBodyParts;
                     pParts && !pParts->IsEmpty();
                     pParts = pParts->GetNext()) {
                    // The part type compares signed.
                    int32_t iPart = static_cast<int32_t>(pParts->GetItem()->eBodyPart);
                    if (iPart >= 0 && iPart <= 6) {
                        if (iCount == iLimb) {
                            pCommand->eLimb = this->GetActorValueFromBodyPart(
                                static_cast<BGSBodyPart::LIMB_ENUM>(pParts->GetItem()->eBodyPart)
                            );
                            bLimbRandomlySelected = true;
                        }
                        ++iCount;
                    }
                }
            }
        }
        if (!bLimbRandomlySelected) {
            pCommand->eLimb = this->GetActorValueFromBodyPart(
                static_cast<BGSBodyPart::LIMB_ENUM>(this->pCurrentLimb->eBodyPart)
            );
        }
        pCommand->cFireShots = VATSMenu::GetShotBurstCount(eAction, false);
        int32_t iFakeShots = static_cast<int32_t>(VATSMenu::GetShotBurstCount(eAction, true)) - pCommand->cFireShots;
        pCommand->cFakeFireShots = static_cast<uint8_t>(BSMAX(iFakeShots, 0));
        if (desiredAttack == DESIRED_ATTACK_SPECIAL) {
            // Only a VATS AP cost above zero (not NaN) is the weapon's special attack.
            if (pWeapon && pWeapon->GetVATSAP() > 0.0) {
                pCommand->eAction = ActionPoints::ACTION_PLAYER_SPECIAL_WEAPON_ATTACK;
            }
            else if (pVATSTarget->IsKnockedOut()) {
                pCommand->eAction = ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_3;
            }
            else {
                pCommand->eAction = ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK;
            }
        }
        else if (desiredAttack == DESIRED_ATTACK_SPECIAL_2) {
            pCommand->eAction = ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_2;
        }
        else {
            pCommand->eAction = eAction;
        }
        pCommand->pTarget = pVATSTarget;
        pCommand->fActionPoints = fRequiredActionPoints;
        // Multiplied by the double 100.0, then stored as a float.
        float fChance = static_cast<float>(fVATSParalyzePalmChance.Float() * 100.0);
        float fDieRoll = RandomFloat(0.0f, 100.0f);
        // A die roll above the chance (or NaN) is no Paralyzing Palm.
        if (bParalyzingPalm && eAction == ActionPoints::ACTION_ATTACK_UNARMED && fChance >= fDieRoll) {
            pCommand->bParalyzingPalm = true;
        }
        if (pVATS.IsMeleeCommand(pCommand) && pVATSTarget->IsActor() && !bLimbRandomlySelected) {
            BGSBodyPartData* pBodyPartData = pTargetActor->GetActorBase()->GetBodyPartData();
            // Actor values are passed to GetBodyPart.
            if (pBodyPartData->GetBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(ActorValue::PERCEPTION_CONDITION))) {
                pCommand->eLimb = ActorValue::PERCEPTION_CONDITION;
            }
            else if (pBodyPartData->GetBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(ActorValue::ENDURANCE_CONDITION))) {
                pCommand->eLimb = ActorValue::ENDURANCE_CONDITION;
            }
            else {
                BGSBodyPart** pArray = pBodyPartData->GetPartArray();
                // The flag is stored as a byte.
                uint8_t bFound = false;
                // The 15 part slots of pPartArray.
                for (int32_t i = 0; i < 15 && !bFound; ++i) {
                    if (pArray[i]) {
                        // The part's actor value is a signed char.
                        pCommand->eLimb = static_cast<ActorValue::Index>(
                            static_cast<int8_t>(pArray[i]->GetActorValue())
                        );
                        bFound = true;
                    }
                }
            }
        }
        pVATS.QueueAction(pCommand);
        float fFinalChance = this->GetTargetPart() ? this->GetTargetPart()->GetItem()->fChance : 0.0f;
        if (this->bConcentratedFirePerk) {
            int32_t iConsShots = this->GetConsecutiveShots();
            int32_t iBonus = static_cast<int32_t>(iVATSConcentratedFireBonus.UInt()) * iConsShots;
            // The bonus is divided by the double 100.0 and added to the float in double precision, then stored as a
            // float.
            fFinalChance = static_cast<float>(static_cast<double>(iBonus) / 100.0 + fFinalChance);
        }
        uint8_t bShouldHit;
        if (bVatsAlwaysHit.Bool() && this->GetTargetPart()) {
            bShouldHit = true;
        }
        else {
            bShouldHit = RandomBool(fFinalChance);
        }
        pCommand->bSuccess = bShouldHit;
        this->PrepareMeters();
        bool bForcedReload = false;
        if (this->UpdateAmmo() &&
            pWeapon &&
            pWeapon->GetWeaponType() != WEAPON_ONE_HAND_GRENADE &&
            pWeapon->GetWeaponType() != WEAPON_ONE_HAND_THROWN) {
            int32_t iClipRounds = pWeapon->GetFormClipRounds(false);
            // Only queued ammo above zero (not NaN) forces the reload.
            bForcedReload = iClipRounds != static_cast<int32_t>(pWeapon->GetAmmoPerShot()) &&
                this->fQueuedTotalAmmo + this->fQueuedClipAmmo > 0.0;
        }
        if (bForcedReload) {
            this->UpdateQueuedActionList(pCommand, true, desiredAttack != DESIRED_ATTACK_NORMAL);
        }
        else {
            this->UpdateQueuedActionList(pCommand, false, desiredAttack != DESIRED_ATTACK_NORMAL);
        }
        // Only a cost above the AP total (not NaN) clears the attack tile's target trait.
        if (fRequiredActionPoints > this->fActionPoints) {
            this->pTiles[7]->Set(Tile::kTileValue_Target, 0);
        }
        this->SetMeterChance();
    }
    VATSMenu::pVatsMenu->fProbableEnemyDamage = VATSMenu::pVatsMenu->CalculateProbableEnemyHealth();
}

bool VATSMenu::DoKey(int32_t aiKey) {
    if (aiKey != 66 && aiKey != 98) {
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
    TESObjectWEAP* pWeapon;
    BaseProcess* pProcess;
    BSSimpleList<VATSTarget*>* pTargetList;
    BSSoundHandle soundHand;
    char acAction[268];
    ItemChange* pWeaponItem;
    char acText[264];
    Tile* pTile;
    BSAudioManager* mgr;
    if (!apCommand) {
        pLastLimb = nullptr;
        float fRequiredPoints = pVATS.GetLastAction() ? pVATS.GetLastAction()->fActionPoints : 0.0f;
        ActionPoints::Action eAction = ActionPoints::ACTION_COUNT;
        bool bIronSights = false;
        pVATS.RemoveLastAction(eAction, bIronSights);
        PrepareMeters();
        if (eAction == ActionPoints::ACTION_COUNT && pVATS.eMode == VATS::VATS_MENU_MODE) {
            Interface::CloseVATSMenu();
        }
        if (!PlayerCharacter::IsGodMode()) {
            pProcess = pPlayer->pCurrentProcess;
            pWeaponItem = pProcess->GetCurrentWeapon();
            fActionPoints = fActionPoints + fRequiredPoints;
        }
        if (fActionPointsBase < static_cast<double>(fActionPoints)) {
            fActionPoints = fActionPointsBase;
        }
        pTiles[7]->Set(Tile::kTileValue_Target, 1);
        if (eAction != ActionPoints::ACTION_RELOAD) {
            Tile* pLastAction = lbActions.GetLastItem();
            if (pLastAction) {
                const char* pText = pLastAction->GetString(Tile::kTileValue_String);
                const char* pReloadText = ActionPoints::GetActionPointsString(ActionPoints::ACTION_RELOAD);
                if (!BSstrcmp(pReloadText, pText)) {
                    lbActions.RemoveAt(lbActions.NumItems() - 1, false);
                }
            }
        }
        lbActions.RemoveAt(lbActions.NumItems() - 1, false);
        BSSoundHandle psoundHand;
        psoundHand = BSAudio::QInstance().GetSoundHandleByName(
            "UIMenuCancel",
            BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
        );
        psoundHand.Play(false);
    }
    else {
        mgr = &BSAudioManager::QInstance();
        soundHand = mgr->GetSoundHandleByName(
            "UIVATSMove",
            BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
        );
        soundHand.Play(false);
        pTargetList = VATSMenu::pVatsMenu->GetVatsTarget(pVATSTarget);
        pProcess = pPlayer->pCurrentProcess;
        pWeaponItem = pProcess->GetCurrentWeapon();
        if (pWeaponItem) {
            pWeapon = static_cast<TESObjectWEAP*>(pWeaponItem->pContainerObj);
        }
        else {
            pWeapon = nullptr;
        }
        fActionPoints = fActionPoints - apCommand->fActionPoints;
        if (ActionPoints::IsAttackAction(apCommand->eAction)) {
            ++VATSMenu::uiActionIndex;
            if (eVATSTargetLimb == ActorValue::VALUE_NONE ||
                !pVATSTarget->IsActor() ||
                !pWeapon ||
                pWeapon->IsMeleeWeapon() ||
                pWeapon->GetWeaponType() == WEAPON_ONE_HAND_GRENADE) {
                BaseProcess* pTargetProcess;
                NiNode* pWeaponBone;
                // Reads the target's process whatever the target's type, then the weapon bone of its
                // biped.
                if (pTargetList->GetItem()->eType == 2 ||
                    (pTargetProcess = static_cast<MobileObject*>(pVATSTarget)->pCurrentProcess,
                     (pWeaponBone = pTargetProcess->GetWeaponBone(pVATSTarget->GetBiped())) == nullptr) ||
                    !pWeapon ||
                    pWeapon->IsMeleeWeapon() ||
                    pWeapon->GetWeaponType() == WEAPON_ONE_HAND_GRENADE) {
                    BSstrcpy(acAction, 260, apCommand->pTarget->GetFullName());
                }
                else {
                    BSstrcpy(acAction, 260, apCommand->pTarget->GetFullName());
                    BSstrcat(acAction, 260, ": ");
                    if (pWeaponBone->m_kName.GetHandleString()) {
                        BSstrcat(acAction, 260, MagicSystem::GetMagicItemCastName(MagicSystem::WHEN_STRIKES));
                    }
                    else {
                        BSstrcat(acAction, 260, "---");
                    }
                }
            }
            else {
                ActorValue::Index eLimb;
                BGSBodyPart* pBodyPart;
                BGSBodyPartData* pBodyPartData;
                // The target is treated as an actor here because IsActor was tested above.
                pBodyPartData = static_cast<Actor*>(pVATSTarget)->GetActorBase()->GetBodyPartData();
                // eBodyPart holds the body part data's LIMB_ENUM.
                eLimb = GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pCurrentLimb->eBodyPart));
                pBodyPart = pBodyPartData->GetBodyPart(eLimb);
                if (!pBodyPart) {
                    return;
                }
                BSstrcpy(acAction, 260, apCommand->pTarget->GetFullName());
                BSstrcat(acAction, 260, ": ");
                if (pBodyPart->GetPartName()) {
                    BSstrcat(acAction, 260, pBodyPart->GetPartName());
                }
                else {
                    BSstrcat(acAction, 260, "---");
                }
            }
            if (bSpecialAttack) {
                if (pWeapon && pWeapon->GetWeaponSkill() == ActorValue::MELEE_WEAPONS) {
                    BSstrcat(acAction, 260, ": ");
                    BSstrcat(acAction, 260, pWeapon->GetVATSAttackName());
                }
                else {
                    switch (apCommand->eAction) {
                        case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK: {
                            BSstrcat(acAction, 260, ": ");
                            BSstrcat(acAction, 260, sVatsUnarmedAttack1.String());
                            break;
                        }
                        case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_3: {
                            BSstrcat(acAction, 260, ": ");
                            BSstrcat(acAction, 260, sVatsUnarmedAttackGround.String());
                            break;
                        }
                        case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_2: {
                            BSstrcat(acAction, 260, ": ");
                            BSstrcat(acAction, 260, sVatsUnarmedAttack2.String());
                            break;
                        }
                    }
                }
            }
            if (abForcedReload) {
                char acAttack[264];
                BSstrcpy(acAttack, 260, acAction);
                pTile = lbActions.AddItem(VATSMenu::uiActionIndex, nullptr, nullptr, nullptr);
                pTile->Set(Tile::kTileValue_String, acAttack, true);
                ++VATSMenu::uiActionIndex;
                BSstrcpy(acAction, 260, ActionPoints::GetActionPointsString(ActionPoints::ACTION_RELOAD));
            }
        }
        else {
            ++VATSMenu::uiActionIndex;
            BSstrcpy(acAction, 260, ActionPoints::GetActionPointsString(apCommand->eAction));
        }
        BSstrcpy(acText, 260, acAction);
        pTile = lbActions.AddItem(VATSMenu::uiActionIndex, nullptr, nullptr, nullptr);
        pTile->Set(Tile::kTileValue_String, acText, true);
    }
    if (lbActions.NumItems()) {
        pTiles[4]->Set(Tile::kTileValue_Target, 1);
    }
    else {
        pTiles[4]->Set(Tile::kTileValue_Target, 0);
    }
}

bool VATSMenu::IsWellFormedMenu(int32_t& ai) {
    for (ai = 1; ai <= 31; ++ai) {
        if (!pTiles[ai]) {
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
        // The BGSAmmoForm base is reached without a null check, as a reference conversion.
        bool bRockItLauncher = static_cast<BGSAmmoForm&>(*pWeapon).IsRockItLauncher();
        float fRockItAmmo = static_cast<float>(pPlayer->GetNumRockItAmmo());
        bool bTotalFromCount = true;
        int32_t iWeaponType = pWeapon->GetWeaponType();
        if (iWeaponType >= WEAPON_ONE_HAND_GRENADE &&
            (iWeaponType <= WEAPON_ONE_HAND_MINE || iWeaponType == WEAPON_ONE_HAND_THROWN)) {
            fClip = static_cast<float>(pWeapon->GetAmmoPerShot());
            InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(pPlayer);
            int32_t iCount = pChanges->GetObjectCount(pWeapon);
            // The difference is computed exactly and rounded once.
            fTotal = static_cast<float>(static_cast<double>(iCount) - fClip);
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
                fTotal = static_cast<float>(pWeapon->GetFormClipRounds(bClipMod));
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
            fClip = static_cast<float>(iClip);
            if (bTotalFromCount) {
                fTotal = static_cast<float>(static_cast<double>(iCount) - fClip);
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
                    float fLeft = this->fQueuedClipAmmo - static_cast<float>(pIter->GetItem()->cFireShots);
                    this->fQueuedClipAmmo = BSMAX(0.0f, fLeft);
                    // Only an exact zero (not NaN) reloads.
                    if (this->fQueuedClipAmmo == 0.0) {
                        float fClipSize;
                        if (bRockItLauncher) {
                            fClipSize = fRockItAmmo;
                        }
                        else {
                            bool bClipMod = pWeaponItem->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE);
                            fClipSize = static_cast<float>(pWeapon->GetFormClipRounds(bClipMod));
                        }
                        this->fQueuedClipAmmo = BSMIN(this->fQueuedTotalAmmo, fClipSize);
                        float fClipSize2;
                        if (bRockItLauncher) {
                            fClipSize2 = fRockItAmmo;
                        }
                        else {
                            bool bClipMod = pWeaponItem->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE);
                            fClipSize2 = static_cast<float>(pWeapon->GetFormClipRounds(bClipMod));
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
                    float fLeft = static_cast<float>(this->fQueuedClipAmmo - 1.0);
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
                        fClipSize = static_cast<float>(pWeapon->GetFormClipRounds(bClipMod));
                    }
                    this->fQueuedClipAmmo = BSMIN(this->fTotalAmmo, fClipSize);
                    float fClipSize2;
                    if (bRockItLauncher) {
                        fClipSize2 = fRockItAmmo;
                    }
                    else {
                        bool bClipMod = pWeaponItem->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE);
                        fClipSize2 = static_cast<float>(pWeapon->GetFormClipRounds(bClipMod));
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
            float fRatio = static_cast<float>(pWeaponItem->GetItemHealth(false) / static_cast<double>(uiHealth));
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
    BSsprintf(
        acBuffer,
        0x10,
        "%i/%i",
        static_cast<int32_t>(this->fQueuedClipAmmo),
        static_cast<int32_t>(this->fQueuedTotalAmmo)
    );
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
        return ActorValue::VALUE_NONE;
    }
    return ActorValue::ENDURANCE_CONDITION;
}

void VATSMenu::UpdateChanceToHit() {
    static int32_t iUT_MeterPercent = Tile::AddUserTrait("_MeterPercent", -1);
    static int32_t iUT_LimbName = Tile::AddUserTrait("_LimbName", -1);
    static int32_t iUT_LimbNameVisible = Tile::AddUserTrait("_LimbNameVisible", -1);
    static int32_t iUT_Brightness_1 = Tile::AddUserTrait("_Brightness", -1);
    BSSimpleList<VATSTarget*>* pTargetList = GetVatsTarget(pVATSTarget);
    BSSimpleList<VATSBodyPart*>* pPartsList = &pTargetList->GetItem()->kBodyParts;
    if (pVATS.eMode == VATS::VATS_TARGET_SELECT ||
        pVATS.eMode == VATS::VATS_SCAN_MODE ||
        (pVATS.eMode == VATS::VATS_MENU_MODE && !bHeadingReached)) {
        // Outside targeting: stop every meter's fade and hide it.
        while (pPartsList && !pPartsList->IsEmpty()) {
            if (pPartsList->GetItem()->pMeter->GetFloat(Tile::kTileValue_Alpha) > 0.0) {
                pPartsList->GetItem()->pMeter->RemoveFadeControl(Tile::kTileValue_Alpha);
                pPartsList->GetItem()->pMeter->Set(Tile::kTileValue_Alpha, 0.0f, true);
            }
            pPartsList = pPartsList->GetNext();
        }
        bMetersShown = false;
    }
    else if (bMetersShown) {
        ItemChange* pWeaponItem = pPlayer->GetCurrentAIProcess()->GetCurrentWeapon();
        TESObjectWEAP* pWeapon;
        if (pWeaponItem) {
            pWeapon = static_cast<TESObjectWEAP*>(pWeaponItem->pContainerObj);
        }
        else {
            pWeapon = nullptr;
        }
        bool bNoRescan = false;
        if (pTargetList->GetItem()->eType == 2 ||
            pPlayer->GetCurrentAIProcess()->IsCurrentWeaponGrenade() ||
            !pWeapon ||
            pWeapon->IsMeleeWeapon()) {
            bNoRescan = true;
        }
        // The torso's node of the part list, once the scan below has passed it.
        BSSimpleList<VATSBodyPart*>* pTorso = nullptr;
        while (!bNoRescan && pPartsList && !pPartsList->IsEmpty()) {
            if (LimbAppeared() ||
                (!pPartsList->GetItem()->bChanceCalculated &&
                 pPartsList->GetItem()->bOnScreen &&
                 pPartsList->GetItem()->bNeedsRecalc)) {
                VATSMenu::bIsARescan = true;
                pTargetList->GetItem()->bChancesCalculated = false;
                if (pVATS.eMode != VATS::VATS_SCAN_MODE) {
                    pVATS.SetMode(VATS::VATS_SCAN_MODE, false);
                    g_vatsLagTimer = appTimer.iLastTime;
                }
                return;
            }
            if (pPartsList->GetItem()->eBodyPart == 1) {
                pTorso = pPartsList;
            }
            pPartsList = pPartsList->GetNext();
        }
        for (pPartsList = &pTargetList->GetItem()->kBodyParts; pPartsList && !pPartsList->IsEmpty();
             pPartsList = pPartsList->GetNext()) {
            float fAlpha = pPartsList->GetItem()->pMeter->GetFloat(Tile::kTileValue_Alpha);
            if (pPartsList->GetItem()->bOnScreen) {
                pPartsList->GetItem()->bNeedsRecalc = false;
                pWeaponItem = pPlayer->GetCurrentAIProcess()->GetCurrentWeapon();
                if (pWeaponItem) {
                    pWeapon = static_cast<TESObjectWEAP*>(pWeaponItem->pContainerObj);
                }
                else {
                    pWeapon = nullptr;
                }
                if (pPlayer->GetCurrentAIProcess()->IsCurrentWeaponGrenade() || !pWeapon || pWeapon->IsMeleeWeapon()) {
                    // No limb targeting: flash the torso, else the last part.
                    if (((pTorso && pPartsList == pTorso) || (!pTorso && !pPartsList->GetNext())) &&
                        pPartsList->GetItem()->pMeter->GetFadeFinished(Tile::kTileValue_Alpha)) {
                        pPartsList->GetItem()->pMeter->AddFadeControl(
                            Tile::kTileValue_Alpha,
                            0.0f,
                            255.0f,
                            1.0f,
                            Tile::TFCT_NONLINEAR_LONG_DARK_REPEATING
                        );
                    }
                }
                else if (
                    pCurrentLimb == pPartsList->GetItem() ||
                    GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pPartsList->GetItem()->eBodyPart)) ==
                        GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pCurrentLimb->eBodyPart))
                ) {
                    if (pPartsList->GetItem()->pMeter->GetFadeFinished(Tile::kTileValue_Alpha)) {
                        pPartsList->GetItem()->pMeter->AddFadeControl(
                            Tile::kTileValue_Alpha,
                            0.0f,
                            255.0f,
                            1.0f,
                            Tile::TFCT_NONLINEAR_LONG_DARK_REPEATING
                        );
                    }
                }
                else if (
                    (fAlpha == pPartsList->GetItem()->pMeter->GetFadeEndFor(Tile::kTileValue_Alpha) &&
                     fAlpha != 255.0) ||
                    pPartsList->GetItem()->pMeter->GetFadeType(Tile::kTileValue_Alpha) ==
                        Tile::TFCT_NONLINEAR_LONG_DARK_REPEATING
                ) {
                    pPartsList->GetItem()
                        ->pMeter->AddFadeControl(Tile::kTileValue_Alpha, fAlpha, 255.0f, 0.2f, Tile::TFCT_STANDARD);
                }
            }
            else if (pPartsList->GetItem()->pMeter->GetFadeEndFor(Tile::kTileValue_Alpha) > 0.0) {
                float fStart = pPartsList->GetItem()->pMeter->GetFloat(Tile::kTileValue_Alpha);
                pPartsList->GetItem()
                    ->pMeter->AddFadeControl(Tile::kTileValue_Alpha, fStart, 0.0f, 0.5f, Tile::TFCT_STANDARD);
            }
            ActorValue::Index eCurrentLimbAV = VATSMenu::pVatsMenu->GetActorValueFromBodyPart(
                static_cast<BGSBodyPart::LIMB_ENUM>(VATSMenu::pVatsMenu->pCurrentLimb->eBodyPart)
            );
            ActorValue::Index ePartAV = VATSMenu::pVatsMenu->GetActorValueFromBodyPart(
                static_cast<BGSBodyPart::LIMB_ENUM>(pPartsList->GetItem()->eBodyPart)
            );
            pPartsList->GetItem()->pMeter->Set(iUT_LimbNameVisible, static_cast<int32_t>(eCurrentLimbAV == ePartAV));
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
    BSSimpleList<VATSTarget*>* pTargetList = GetVatsTarget(pVATSTarget);
    VATSTarget* pTargetListItem = pTargetList->m_item;
    BSSimpleList<VATSBodyPart*>* pPartsList = &pTargetListItem->kBodyParts;
    NiAVObject* pTarget = nullptr;
    NiPoint3 targetPoint;
    int32_t iLocX = 0;
    int32_t iLocY = 0;
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
        VATSMenu::iMaxX = static_cast<int32_t>(
            InterfaceManager::GetScreenWidth() - static_cast<double>(VATSMenu::iMinX) - 50.0
        );
    }
    if ((PrepareMeters_guards & 32) == 0) {
        PrepareMeters_guards |= 32u;
        double dScreenHeight = InterfaceManager::GetScreenHeight();
        double dSafeHeight = dScreenHeight - InterfaceManager::GetScreenSafeY() * 2.0;
        VATSMenu::iMaxY = static_cast<int32_t>(
            dSafeHeight - VATSMenu::pVatsMenu->pTiles[17]->GetFloat(Tile::kTileValue_Height) + 50.0
        );
    }
    ItemChange* pWeaponItem = pPlayer->pCurrentProcess->GetCurrentWeapon();
    TESObjectWEAP* pWeapon = pWeaponItem ? static_cast<TESObjectWEAP*>(pWeaponItem->pContainerObj) : nullptr;
    for (pPartsList = &pTargetListItem->kBodyParts; pPartsList && !pPartsList->IsEmpty();
         pPartsList = pPartsList->m_pkNext) {
        BGSBodyPartData* pBodyPartData = nullptr;
        BGSBodyPart* pBodyPart = nullptr;
        VATSBodyPart* pPartsListItem = pPartsList->m_item;
        if (pTargetListItem->eType == 2) {
            pTarget = pTargetListItem->pTargetRef->Get3D();
        }
        else if (static_cast<BGSBodyPart::LIMB_ENUM>(pPartsListItem->eBodyPart) == BGSBodyPart::LIMB_WEAPON) {
            NiNode* pWeaponBone = nullptr;
            if (pVATSTarget->IsActor()) {
                BaseProcess* pTargetProcess = static_cast<Actor*>(pVATSTarget)->pCurrentProcess;
                pWeaponBone = pTargetProcess->GetWeaponBone(pVATSTarget->GetBiped());
            }
            pTarget = pWeaponBone;
        }
        else {
            pBodyPartData = static_cast<Actor*>(pVATSTarget)->GetActorBase()->GetBodyPartData();
            pBodyPart = pBodyPartData->GetBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pPartsListItem->eBodyPart));
            if (pBodyPart) {
                const char* pTargetName = pBodyPart->GetTargetName();
                pTarget = BSUtilities::GetObjectByName(pVATSTarget->Get3D(), pTargetName);
            }
        }
        if (pTarget) {
            targetPoint = pTarget->GetWorldTranslate();
            pPartsListItem->kPos = targetPoint;
            Main::GetWorldSceneGraph()
                ->GetCamera()
                ->WorldPtToScreenPt(targetPoint, pPartsListItem->kScreenPos.x, pPartsListItem->kScreenPos.y, 1e-05f);
        }
        pPartsListItem->kScreenPos.x = InterfaceManager::GetScreenWidth() * pPartsListItem->kScreenPos.x;
        double dInvY = 1.0 - pPartsListItem->kScreenPos.y;
        pPartsListItem->kScreenPos.y = InterfaceManager::GetScreenHeight() * dInvY;
        NiMatrix3 projectionMatrix;
        GetScreenPlaneProjectionMatrix(&projectionMatrix);
        pPartsListItem->kRelativePos = projectionMatrix * pPartsListItem->kPos;
        ItemChange* pTargetWeaponItem = nullptr;
        if (pVATSTarget->IsActor()) {
            pTargetWeaponItem = static_cast<Actor*>(pVATSTarget)->pCurrentProcess->GetCurrentWeapon();
        }
        if (pTargetWeaponItem) {
            pTargetWeaponItem->GetItemHealth(true);
        }
        TESObjectWEAP* pTargetWeapon = pTargetWeaponItem ? static_cast<TESObjectWEAP*>(pTargetWeaponItem->pContainerObj)
                                                         : nullptr;
        bDataInTemporaryVATSCommand = 0;
        // Each value is computed before its trait id is added, matching the game's order of argument evaluation.
        if (pTargetListItem->eType == 2 ||
            pPlayer->pCurrentProcess->IsCurrentWeaponGrenade() ||
            !pWeapon ||
            pWeapon->IsMeleeWeapon()) {
            const char* pName = pTargetListItem->pTargetRef->GetFullName();
            pPartsListItem->pMeter->Set(Tile::AddUserTrait("_LimbName", -1), pName, true);
            float fHealth = pTargetListItem->pTargetRef->GetHealth() / 100.0;
            float fPercent = BSMIN(1.0f, BSMAX(0.0f, fHealth));
            pPartsListItem->pMeter->Set(Tile::AddUserTrait("_MeterPercent", -1), fPercent, true);
        }
        else if (pBodyPart) {
            const char* pName = pBodyPart->GetPartName();
            pPartsListItem->pMeter->Set(Tile::AddUserTrait("_LimbName", -1), pName, true);
            ActorValue::Index eLimbValue = GetActorValueFromBodyPart(
                static_cast<BGSBodyPart::LIMB_ENUM>(pPartsListItem->eBodyPart)
            );
            ActorValueOwner* pTargetValues = static_cast<Actor*>(pVATSTarget);
            float fLimbHealth = static_cast<double>(pTargetValues->GetActorValue(eLimbValue)) / 100.0;
            float fPercent = BSMIN(1.0f, BSMAX(0.0f, fLimbHealth));
            pPartsListItem->pMeter->Set(Tile::AddUserTrait("_MeterPercent", -1), fPercent, true);
        }
        else if (pTargetWeapon) {
            const char* pName = pTargetWeapon->GetFullName();
            pPartsListItem->pMeter->Set(Tile::AddUserTrait("_LimbName", -1), pName, true);
            uint32_t uiFormHealth;
            if (TESHealthForm::GetFormHealth(pTargetWeaponItem->pContainerObj)) {
                uiFormHealth = TESHealthForm::GetFormHealth(pTargetWeaponItem->pContainerObj);
            }
            else {
                uiFormHealth = 1;
            }
            float fItemHealth = pTargetWeaponItem->GetItemHealth(false) / static_cast<double>(uiFormHealth);
            float fPercent = BSMIN(1.0f, BSMAX(0.0f, fItemHealth));
            pPartsListItem->pMeter->Set(Tile::AddUserTrait("_MeterPercent", -1), fPercent, true);
        }
        if (!(pPartsListItem->kScreenPos.x < 0.0) &&
            !(static_cast<double>(pPartsListItem->kScreenPos.x) > InterfaceManager::GetScreenWidth()) &&
            !(pPartsListItem->kScreenPos.y < 0.0) &&
            !(static_cast<double>(pPartsListItem->kScreenPos.y) > InterfaceManager::GetScreenHeight())) {
            pPartsListItem->bOnScreen = true;
        }
        else {
            pPartsListItem->bOnScreen = false;
            pPartsListItem->bChanceCalculated = false;
            pPartsListItem->bFirstTimeShown = false;
            pPartsListItem->pMeter->RemoveFadeControl(Tile::kTileValue_Alpha);
            float fStart = pPartsListItem->pMeter->GetFloat(Tile::kTileValue_Alpha);
            pPartsListItem->pMeter->AddFadeControl(Tile::kTileValue_Alpha, fStart, 0.0f, 0.5f, Tile::TFCT_STANDARD);
        }
    }
    for (pPartsList = &pTargetList->m_item->kBodyParts; pPartsList && !pPartsList->IsEmpty();
         pPartsList = pPartsList->m_pkNext) {
        VATSBodyPart* pPart = pPartsList->m_item;
        if (pPart->bOnScreen) {
            switch (static_cast<BGSBodyPart::LIMB_ENUM>(pPart->eBodyPart)) {
                case BGSBodyPart::LIMB_HEAD_1:
                case BGSBodyPart::LIMB_HEAD_2: {
                    iLocX = static_cast<int32_t>(pPart->kScreenPos.x);
                    int32_t iOffsetY = pPart->kScreenPos.y > static_cast<double>(VATSMenu::iScreenCenterY) ? 100 : -100;
                    iLocY = static_cast<int32_t>(static_cast<double>(iOffsetY) + pPart->kScreenPos.y);
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
                    int32_t iOffsetX = pPart->kScreenPos.x > static_cast<double>(VATSMenu::iScreenCenterX) ? 100 : -100;
                    iLocX = static_cast<int32_t>(static_cast<double>(iOffsetX) + pPart->kScreenPos.x);
                    int32_t iOffsetY = pPart->kScreenPos.y > static_cast<double>(VATSMenu::iScreenCenterY) ? 50 : -50;
                    iLocY = static_cast<int32_t>(static_cast<double>(iOffsetY) + pPart->kScreenPos.y);
                    break;
                }
                default: {
                    iLocX = static_cast<int32_t>(pPart->kScreenPos.x);
                    iLocY = static_cast<int32_t>(pPart->kScreenPos.y);
                    break;
                }
            }
            iLocX = BSMAX(VATSMenu::iMinX, BSMIN(VATSMenu::iMaxX, iLocX));
            iLocY = BSMAX(VATSMenu::iMinY, BSMIN(VATSMenu::iMaxY, iLocY));
            pPart->pMeter->Set(Tile::kTileValue_X, iLocX);
            pPart->pMeter->Set(Tile::kTileValue_Y, iLocY);
            // The compares are signed.
            int32_t iBodyPart = static_cast<int32_t>(pPart->eBodyPart);
            if (iBodyPart >= BGSBodyPart::LIMB_LEFT_ARM_1 &&
                (iBodyPart <= BGSBodyPart::LIMB_RIGHT_LEG_3 || iBodyPart == BGSBodyPart::LIMB_WEAPON)) {
                if (iLocX >= VATSMenu::iScreenCenterX) {
                    pPart->pMeter->Set(Tile::kTileValue_Justify, 1);
                }
                else {
                    pPart->pMeter->Set(Tile::kTileValue_Justify, 4);
                }
            }
        }
    }
    bool bOverlapping = true;
    int32_t iCount = 0;
    NiPoint2 limb;
    NiPoint2 compareLimb;
    NiPoint2 center;
    NiPoint2 arbitraryDimensions;
    arbitraryDimensions.x = 150.0f;
    arbitraryDimensions.y = 100.0f;
    while (bOverlapping) {
        bOverlapping = false;
        ++iCount;
        for (pPartsList = &pTargetList->m_item->kBodyParts; pPartsList && !pPartsList->IsEmpty();
             pPartsList = pPartsList->m_pkNext) {
            VATSBodyPart* pPart = pPartsList->m_item;
            for (BSSimpleList<VATSBodyPart*>* pRestOfList = pPartsList->m_pkNext;
                 pRestOfList && !pRestOfList->IsEmpty();
                 pRestOfList = pRestOfList->m_pkNext) {
                VATSBodyPart* pRestOfListItem = pRestOfList->m_item;
                limb.x = pPart->pMeter->GetFloat(Tile::kTileValue_X);
                limb.y = pPart->pMeter->GetFloat(Tile::kTileValue_Y);
                compareLimb.x = pRestOfListItem->pMeter->GetFloat(Tile::kTileValue_X);
                compareLimb.y = pRestOfListItem->pMeter->GetFloat(Tile::kTileValue_Y);
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
                    static_cast<float>(VATSMenu::iMinX),
                    BSMIN(static_cast<float>(VATSMenu::iMaxX), compareLimb.x)
                );
                compareLimb.y = BSMAX(
                    static_cast<float>(VATSMenu::iMinY),
                    BSMIN(static_cast<float>(VATSMenu::iMaxY), compareLimb.y)
                );
                pPart->pMeter->Set(Tile::kTileValue_X, limb.x, true);
                pPart->pMeter->Set(Tile::kTileValue_Y, limb.y, true);
                pRestOfListItem->pMeter->Set(Tile::kTileValue_X, compareLimb.x, true);
                pRestOfListItem->pMeter->Set(Tile::kTileValue_Y, compareLimb.y, true);
                bOverlapping = true;
            }
        }
        if (iCount > 30) {
            bOverlapping = false;
        }
    }
}

// Moves the targeted limb: in auto-target, grenade or melee VATS to the head (or none); with a controller, to the part
// of the target's body part list nearest the stick's direction; with the keyboard or the mouse wheel, to the next or
// previous part whose meter is visible. VATSBodyPart::eBodyPart is passed on as the limb.
void VATSMenu::SetTargetLimb() {
    Controls* pControls = appMain->GetControls();
    BaseProcess* pProcess = pPlayer->pCurrentProcess;
    ItemChange* pWeap = pProcess->GetCurrentWeapon();
    TESObjectWEAP* pWeapon;
    if (pWeap) {
        pWeapon = static_cast<TESObjectWEAP*>(pWeap->pContainerObj);
    }
    else {
        pWeapon = nullptr;
    }
    BSSimpleList<VATSTarget*>* pTargetList = GetVatsTarget(pVATSTarget);
    if (pVATS.eMode != VATS::VATS_SCAN_MODE) {
        pProcess = pPlayer->pCurrentProcess;
        if (pProcess->IsCurrentWeaponGrenade() ||
            pTargetList->m_item->eType == 2 ||
            !pWeapon ||
            pWeapon->IsMeleeWeapon()) {
            if (pTargetList->m_item->eType == 2) {
                eVATSTargetLimb = ActorValue::VALUE_NONE;
            }
            else {
                // A target of another type is an actor.
                TESActorBase* pActorBase = static_cast<Actor*>(pTargetList->m_item->pTargetRef)->GetActorBase();
                BGSBodyPartData* pBodyPartData = pActorBase->GetBodyPartData();
                if (pBodyPartData->pPartArray[BGSBodyPart::LIMB_HEAD_1]) {
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
        int32_t iLeftStickX = pControls->GetControllerValue(0, 7);
        int32_t iLeftStickY = pControls->GetControllerValue(0, 8);
        int32_t iMaxAnalogValue = 32767;
        if (std::abs(iLeftStickY) < iMaxAnalogValue / 4 && std::abs(iLeftStickX) < iMaxAnalogValue / 4) {
            bStickReset = 1;
        }
        NiMatrix3 cameraRotationMatrix = Main::GetWorldSceneGraph()->GetCamera()->GetWorldTransform().m_Rotate;
        NiPoint3 stick(0.0f, static_cast<float>(-iLeftStickY), static_cast<float>(-iLeftStickX));
        stick = cameraRotationMatrix * stick;
        stick.Unitize();
        if (!bStickReset) {
            return;
        }
        if (std::abs(iLeftStickY) <= 3 * iMaxAnalogValue / 4 && std::abs(iLeftStickX) <= 3 * iMaxAnalogValue / 4) {
            return;
        }
        float fStoredAngle = 180.0f;
        float fStoredDist = std::numeric_limits<float>::max();
        ActorValue::Index eStoredTarget = eVATSTargetLimb;
        BSSimpleList<VATSBodyPart*>* pPartsList = &GetVatsTarget(pVATSTarget)->m_item->kBodyParts;
        VATSBodyPart* pOldLimb = pCurrentLimb;
        bool bPartFound = false;
        while (pPartsList && !pPartsList->IsEmpty()) {
            NiPoint3 tempTarget = pOldLimb->kRelativePos - pPartsList->m_item->kRelativePos;
            float fLength = tempTarget.Length();
            tempTarget.Unitize();
            float fAngle = std::acosf(stick.Dot(tempTarget));
            fAngle = fAngle * 57.2957763671875;
            // The last node of the list whose part maps to the same actor value.
            BSSimpleList<VATSBodyPart*>* pFirstOfType = nullptr;
            for (BSSimpleList<VATSBodyPart*>* pIter = &GetVatsTarget(pVATSTarget)->m_item->kBodyParts;
                 pIter && !pIter->IsEmpty();
                 pIter = pIter->m_pkNext) {
                ActorValue::Index eIterValue = GetActorValueFromBodyPart(
                    static_cast<BGSBodyPart::LIMB_ENUM>(pIter->m_item->eBodyPart)
                );
                if (eIterValue ==
                    GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pPartsList->m_item->eBodyPart))) {
                    pFirstOfType = pIter;
                }
            }
            pPartsList->m_item->fAngleToStick = fAngle;
            if (fAngle < 45.0 &&
                fLength != 0.0 &&
                fStoredDist > fLength &&
                pFirstOfType &&
                pFirstOfType->m_item == pPartsList->m_item) {
                bPartFound = true;
                fStoredDist = fLength;
                fStoredAngle = fAngle;
                eStoredTarget = GetActorValueFromBodyPart(
                    static_cast<BGSBodyPart::LIMB_ENUM>(pPartsList->m_item->eBodyPart)
                );
                bStickReset = 0;
                pCurrentLimb->pMeter->RemoveFadeControl(Tile::kTileValue_Alpha);
                float fStart = pCurrentLimb->pMeter->GetFloat(Tile::kTileValue_Alpha);
                pCurrentLimb->pMeter->AddFadeControl(Tile::kTileValue_Alpha, fStart, 255.0f, 0.2f, Tile::TFCT_STANDARD);
                pCurrentLimb = pPartsList->m_item;
            }
            pPartsList = pPartsList->m_pkNext;
        }
        if (!bPartFound) {
            fStoredAngle = 180.0f;
            for (pPartsList = &GetVatsTarget(pVATSTarget)->m_item->kBodyParts; pPartsList && !pPartsList->IsEmpty();
                 pPartsList = pPartsList->m_pkNext) {
                BSSimpleList<VATSBodyPart*>* pFirstOfType = nullptr;
                for (BSSimpleList<VATSBodyPart*>* pIter = &GetVatsTarget(pVATSTarget)->m_item->kBodyParts;
                     pIter && !pIter->IsEmpty();
                     pIter = pIter->m_pkNext) {
                    ActorValue::Index eIterValue = GetActorValueFromBodyPart(
                        static_cast<BGSBodyPart::LIMB_ENUM>(pIter->m_item->eBodyPart)
                    );
                    if (eIterValue ==
                        GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pPartsList->m_item->eBodyPart))) {
                        pFirstOfType = pIter;
                    }
                }
                if (fStoredAngle > pPartsList->m_item->fAngleToStick &&
                    pOldLimb != pPartsList->m_item &&
                    pFirstOfType &&
                    pFirstOfType->m_item == pPartsList->m_item) {
                    fStoredAngle = pPartsList->m_item->fAngleToStick;
                    eStoredTarget = GetActorValueFromBodyPart(
                        static_cast<BGSBodyPart::LIMB_ENUM>(pPartsList->m_item->eBodyPart)
                    );
                    bStickReset = 0;
                    pCurrentLimb->pMeter->RemoveFadeControl(Tile::kTileValue_Alpha);
                    float fStart = pCurrentLimb->pMeter->GetFloat(Tile::kTileValue_Alpha);
                    pCurrentLimb->pMeter
                        ->AddFadeControl(Tile::kTileValue_Alpha, fStart, 255.0f, 0.2f, Tile::TFCT_STANDARD);
                    pCurrentLimb = pPartsList->m_item;
                }
            }
        }
        eVATSTargetLimb = eStoredTarget;
        return;
    }
    // 0x11 and 0x1F: the W and S keys' scan codes.
    if (pControls->GetKeyboardButton(0x11, Controls::isPressed) ||
        static_cast<double>(InterfaceManager::Instance()->GetMouseWheel()) > 0.0) {
        // The next part with a visible meter, wrapping to the first.
        do {
            for (BSSimpleList<VATSBodyPart*>* pIter = &GetVatsTarget(pVATSTarget)->m_item->kBodyParts;
                 pIter && !pIter->IsEmpty();
                 pIter = pIter->m_pkNext) {
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
    if (pControls->GetKeyboardButton(0x1F, Controls::isPressed) ||
        static_cast<double>(InterfaceManager::Instance()->GetMouseWheel()) < 0.0) {
        // The previous part with a visible meter, wrapping to the last.
        do {
            for (BSSimpleList<VATSBodyPart*>* pIter = &GetVatsTarget(pVATSTarget)->m_item->kBodyParts;
                 pIter && !pIter->IsEmpty();
                 pIter = pIter->m_pkNext) {
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
    for (BSSimpleList<VATSBodyPart*>* pIter = &GetVatsTarget(pVATSTarget)->m_item->kBodyParts;
         pIter && !pIter->IsEmpty();
         pIter = pIter->m_pkNext) {
        if (pIter->m_item == pCurrentLimb) {
            return pIter;
        }
    }
    return nullptr;
}

bool VATSMenu::PrecalcTargetHitChances() {
    static VATSTarget* pLastTarget;
    if (!pVATSTarget || bClosing) {
        return false;
    }
    if (pVATS.eMode == VATS::VATS_TARGET_SELECT) {
        if (!bTargetsLit) {
            FOVATSEffectManager::VATS_EFFECT_MODE eMode = FOVATSEffectManager::MULTIPLE_TARGETS;
            Interface::GetVATSEffect()->SetEffectMode(eMode);
            for (BSSimpleList<VATSTarget*>* pTargets = &VATSMenu::xTargetList; pTargets && !pTargets->IsEmpty();
                 pTargets = pTargets->m_pkNext) {
                TESObjectREFR* pTargetRef = pTargets->m_item->pTargetRef;
                Interface::GetVATSEffect()->SetMultiTarget(pTargetRef);
            }
            bTargetsLit = true;
        }
        TESObjectREFR* pTargetRef = pVATSTarget;
        Interface::GetVATSEffect()->SetSelectedMultiTarget(pTargetRef);
        return true;
    }
    if (bTargetsLit) {
        bTargetsLit = false;
        Interface::GetVATSEffect()->SetTarget(nullptr, BGSBodyPart::LIMB_TORSO, false);
        Interface::GetVATSEffect()->ClearMultiTargets();
        FOVATSEffectManager::VATS_EFFECT_MODE eMode = FOVATSEffectManager::SINGLE_TARGET;
        Interface::GetVATSEffect()->SetEffectMode(eMode);
        VATSMenu::ShowTargetLimb(true);
        return false;
    }
    VATSTarget* pCurrentTarget = GetVatsTarget(pVATSTarget)->m_item;
    if (!pCurrentTarget) {
        return false;
    }
    TESObjectWEAP* pPlayerWeapon;
    BaseProcess* pPlayerProcess = pPlayer->pCurrentProcess;
    ItemChange* pWeaponItem = pPlayerProcess->GetCurrentWeapon();
    if (pWeaponItem) {
        pPlayerWeapon = static_cast<TESObjectWEAP*>(pWeaponItem->pContainerObj);
    }
    else {
        pPlayerWeapon = nullptr;
    }
    if (pCurrentTarget->bChancesCalculated) {
        if (pVATS.eMode == VATS::VATS_SCAN_MODE) {
            pVATS.SetMode(VATS::VATS_MENU_MODE, false);
        }
        return true;
    }
    if (!Interface::GetVATSEffect()->IsScanning() && !VATSMenu::bIsARescan) {
        Interface::GetVATSEffect()->TriggerScan();
        if (!pCurrentTarget->pTargetRef->IsActor()) {
            TESObjectREFR* pTargetRef = pCurrentTarget->pTargetRef;
            Interface::GetVATSEffect()->SetTarget(pTargetRef, BGSBodyPart::LIMB_HEAD_1, true);
            Interface::GetVATSEffect()->DisableScan();
        }
    }
    if (pVATS.eMode == VATS::VATS_SCAN_MODE) {
        // BSTimer::GetTime is unsigned, so the difference is compared unsigned.
        if (appTimer.GetTime() - g_vatsLagTimer > 2000) {
            // Parts whose chance is known: their count and the sum of their chances.
            int32_t iCalculated = 0;
            float fChanceSum = 0.0f;
            for (BSSimpleList<VATSBodyPart*>* pIter = &pCurrentTarget->kBodyParts; pIter && !pIter->IsEmpty();
                 pIter = pIter->m_pkNext) {
                if (pIter->m_item->bChanceCalculated) {
                    ++iCalculated;
                    fChanceSum = fChanceSum + pIter->m_item->fChance;
                }
            }
            Interface::GetVATSEffect()->DisableScan();
            for (BSSimpleList<VATSBodyPart*>* pIter = &pCurrentTarget->kBodyParts; pIter && !pIter->IsEmpty();
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
    VATSBodyPart* pTorsoPart = nullptr;
    for (BSSimpleList<VATSBodyPart*>* pIter = &pCurrentTarget->kBodyParts; pIter && !pIter->IsEmpty();
         pIter = pIter->m_pkNext) {
        if (!pIter->m_item->eBodyPart) {
            pTorsoPart = pIter->m_item;
        }
    }
    BSSimpleList<VATSBodyPart*>* pPending = &pCurrentTarget->kBodyParts;
    while (pPending &&
           !pPending->IsEmpty() &&
           (pPending->m_item->bChanceCalculated ||
            pPending->m_item->bFirstTimeShown ||
            (bHeadingReached && !pPending->m_item->bOnScreen))) {
        pPending = pPending->m_pkNext;
    }
    bool bAllCalculated = true;
    for (BSSimpleList<VATSBodyPart*>* pIter = &pCurrentTarget->kBodyParts; pIter && !pIter->IsEmpty();
         pIter = pIter->m_pkNext) {
        if (!pIter->m_item->bChanceCalculated && pIter->m_item->bOnScreen) {
            bAllCalculated = false;
            break;
        }
    }
    if (bAllCalculated && (!pPending || pPending->IsEmpty())) {
        SetMeterChance();
        Interface::GetVATSEffect()->DisableOcclusion();
        BSSimpleList<VATSBodyPart*>* pLimb = FindTargetPart(ActorValue::PERCEPTION_CONDITION);
        if (!pLimb ||
            !((pLimb->m_item->kPos - pPlayer->Get3D()->GetWorldTranslate()).Length() < 300.0) ||
            pCurrentTarget == pLastTarget) {
            if (pCurrentTarget == pLastTarget) {
                pLimb = FindTargetPart(eVATSTargetLimb);
                pCurrentLimb = pLimb ? pLimb->m_item : pCurrentTarget->kBodyParts.m_item;
            }
            else if (pTorsoPart) {
                eVATSTargetLimb = ActorValue::ENDURANCE_CONDITION;
                pCurrentLimb = pTorsoPart;
            }
            else {
                // eBodyPart holds the body part data's LIMB_ENUM.
                eVATSTargetLimb = GetActorValueFromBodyPart(
                    static_cast<BGSBodyPart::LIMB_ENUM>(pCurrentLimb->eBodyPart)
                );
            }
        }
        else {
            pCurrentLimb = pLimb->m_item;
        }
        pLastTarget = pCurrentTarget;
        pCurrentTarget->bChancesCalculated = true;
        VATSMenu::bIsARescan = false;
        pVATS.SetMode(VATS::VATS_MENU_MODE, false);
        Interface::GetVATSEffect()->DisableScan();
        bool bNoRangedWeapon = false;
        if (pCurrentTarget->eType == 2 ||
            pPlayer->pCurrentProcess->IsCurrentWeaponGrenade() ||
            !pPlayerWeapon ||
            pPlayerWeapon->IsMeleeWeapon()) {
            bNoRangedWeapon = true;
        }
        if (bNoRangedWeapon) {
            VATSMenu::ShowTargetLimb(true);
        }
        UpdateChanceToHit();
        return true;
    }
    if (pPending && pPending->m_item) {
        pCurrentLimb = pPending->m_item;
        if (!pPending->m_item->bFirstTimeShown) {
            VATSMenu::ShowTargetLimb(true);
            pPending->m_item->bFirstTimeShown = true;
        }
    }

    char bContinue = 1;
    for (BSSimpleList<VATSBodyPart*>* pIter = &pCurrentTarget->kBodyParts;; pIter = pIter->m_pkNext) {
        if (!pIter || pIter->IsEmpty()) {
            pPlayer->UpdateCamera(false, false);
            pPlayer->UpdateFirstPersonZoom();
            return true;
        }
        VATSBodyPart* pBodyPart = pIter->m_item;
        float fPercentVisible = 0.0f;
        if (pBodyPart->bFirstTimeShown) {
            uint32_t uiBodyPart = pBodyPart->eBodyPart;
            if (Interface::GetVATSEffect()->GetVATSGeometryOcclusionPercent(uiBodyPart, fPercentVisible)) {
                pBodyPart->fPercentVisible = fPercentVisible;
                if (pBodyPart->kPos == NiPoint3::ZERO) {
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
                UpdateHitChanceForBodyPartData(pIter);
                if (bContinue && !pIter->m_pkNext) {
                    break;
                }
            }
        }
    }
    SetMeterChance();
    Interface::GetVATSEffect()->DisableOcclusion();
    BSSimpleList<VATSBodyPart*>* pLimb = FindTargetPart(ActorValue::PERCEPTION_CONDITION);
    if (!pLimb ||
        !((pLimb->m_item->kPos - pPlayer->Get3D()->GetWorldTranslate()).Length() < 300.0) ||
        pCurrentTarget == pLastTarget) {
        if (pCurrentTarget == pLastTarget) {
            pLimb = FindTargetPart(eVATSTargetLimb);
            pCurrentLimb = pLimb ? pLimb->m_item : pCurrentTarget->kBodyParts.m_item;
        }
        else if (pTorsoPart) {
            eVATSTargetLimb = ActorValue::ENDURANCE_CONDITION;
            pCurrentLimb = pTorsoPart;
        }
        else {
            // eBodyPart holds the body part data's LIMB_ENUM.
            eVATSTargetLimb = GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pCurrentLimb->eBodyPart));
        }
    }
    else {
        pCurrentLimb = pLimb->m_item;
    }
    pLastTarget = pCurrentTarget;
    pVATS.SetMode(VATS::VATS_MENU_MODE, false);
    Interface::GetVATSEffect()->DisableScan();
    UpdateChanceToHit();
    char bNoRangedWeapon = 0;
    if (pCurrentTarget->eType == 2 ||
        pPlayer->pCurrentProcess->IsCurrentWeaponGrenade() ||
        !pPlayerWeapon ||
        pPlayerWeapon->IsMeleeWeapon()) {
        bNoRangedWeapon = 1;
    }
    if (bNoRangedWeapon) {
        VATSMenu::ShowTargetLimb(true);
    }
    pCurrentTarget->bChancesCalculated = true;
    VATSMenu::bIsARescan = false;
    return true;
}

float VATSMenu::CalculateProbableEnemyHealth() {
    if (pVATSTarget && pVATSTarget->IsActor()) {
        TESObjectWEAP* pWeapon;
        float fEnemyDamage = 0.0f;
        for (BSSimpleList<VATS_COMMAND*>* pIter = &pVATS.commandList; pIter && !pIter->IsEmpty();
             pIter = pIter->m_pkNext) {
            TESObjectREFR* pTarget = pIter->m_item->pTarget;
            if (pTarget == pVATSTarget) {
                float fDamage = HitData::PredictVATSCommandHealthDamage(pIter->m_item);
                fEnemyDamage = fEnemyDamage -
                    static_cast<Actor*>(pVATSTarget)->Actor::DifficultyLevelAdjustHealthModifier(-fDamage, pPlayer);
            }
        }
        BaseProcess* pProcess = pPlayer->pCurrentProcess;
        ItemChange* pWeaponItem = pProcess->GetCurrentWeapon();
        if (pWeaponItem) {
            pWeapon = static_cast<TESObjectWEAP*>(pWeaponItem->pContainerObj);
        }
        else {
            pWeapon = nullptr;
        }
        WEAPON_TYPE eWeaponType = pWeapon ? pWeapon->GetWeaponType() : WEAPON_HAND_TO_HAND;
        ActionPoints::Action eAction = ActionPoints::GetAttackAction(eWeaponType);
        VATS_COMMAND vc;
        vc.eAction = eAction;
        vc.bSuccess = true;
        vc.pTarget = pVATSTarget;
        // eBodyPart holds the body part data's LIMB_ENUM.
        vc.eLimb = GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pCurrentLimb->eBodyPart));
        vc.cFireShots = VATSMenu::GetShotBurstCount(eAction, false);
        float fDamage = HitData::PredictVATSCommandHealthDamage(&vc);
        double dAdjustedDamage = static_cast<Actor*>(pVATSTarget)
                                     ->Actor::DifficultyLevelAdjustHealthModifier(-fDamage, pPlayer);
        fEnemyDamage = fEnemyDamage - dAdjustedDamage;
        ActorValueOwner* pOwner = static_cast<Actor*>(pVATSTarget);
        if (!pOwner->GetActorValue(ActorValue::HEALTH)) {
            return 0.0f;
        }
        pOwner = static_cast<Actor*>(pVATSTarget);
        int32_t iHealth = pOwner->GetActorValue(ActorValue::HEALTH);
        if (!(fEnemyDamage > static_cast<double>(iHealth))) {
            pOwner = static_cast<Actor*>(pVATSTarget);
            int32_t iBaseHealth = pOwner->GetBaseActorValue(ActorValue::HEALTH);
            float fDamageRatio = fEnemyDamage / static_cast<double>(iBaseHealth);
            return fDamageRatio;
        }
        pOwner = static_cast<Actor*>(pVATSTarget);
        float fHealth = static_cast<float>(pOwner->GetActorValue(ActorValue::HEALTH));
        return fHealth;
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
    float fPickLength = fFarthestTarget;
    int32_t iCursorY = static_cast<int32_t>(InterfaceManager::Instance()->sRealCursorPos.z);
    int32_t iCursorX = static_cast<int32_t>(InterfaceManager::Instance()->sRealCursorPos.x);
    Main::GetWorldSceneGraph()->GetCamera()->WindowPointToRay(iCursorX, iCursorY, origin, dir, nullptr);
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
    pd.NiSetLength(dir * fPickLength);
    hkpAllRayHitCollector collector;
    pd.SetAllCollector(&collector);
    pPlayer->GetParentCell()->GetbhkWorld()->PickObject(pd);
    for (int32_t i = 0; i < pd.pAllCollector->getHits().m_size; ++i) {
        hkpWorldRayCastOutput point(pd.pAllCollector->getHits()[i]);
        NiAVObject* pAV = GetAVObjectForCollidable(point.m_rootCollidable);
        TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pAV);
        if (pAV) {
            BSSimpleList<VATSTarget*>* pTarget = this->GetVatsTarget(pRef);
            if (pTarget && pTarget->m_item->pTargetRef == pVATSTarget) {
                break;
            }
            if (pTarget) {
                VATSMenu::SetHeadingReached(false);
                this->UpdateChanceToHit();
                pVATSTarget = pTarget->m_item->pTargetRef;
                return true;
            }
        }
    }
    return false;
}

int32_t VATSMenu::GetConsecutiveShots() {
    int32_t iTotalConsecutiveShots = 0;
    for (BSSimpleList<VATS_COMMAND*>* pIter = &pVATS.commandList; pIter && !pIter->IsEmpty(); pIter = pIter->m_pkNext) {
        bool bNext = pIter->m_pkNext != nullptr;
        VATS_COMMAND* pNext = bNext ? pIter->m_pkNext->m_item : nullptr;
        VATS_COMMAND* pCurrent = pIter->m_item;
        if (!bNext) {
            return iTotalConsecutiveShots;
        }
        if (pCurrent->pTarget == pNext->pTarget && pCurrent->eLimb == pNext->eLimb) {
            ++iTotalConsecutiveShots;
        }
        else {
            iTotalConsecutiveShots = 0;
        }
    }
    return iTotalConsecutiveShots;
}

// Writes each body part's chance into its meter. A part sharing its actor value with a later part of the target
// takes the higher meter percentage and chance of the two, and only one of the two meters stays visible.
void VATSMenu::SetMeterChance() {
    BSSimpleList<VATSBodyPart*>* pParts = &GetVatsTarget(pVATSTarget)->GetItem()->kBodyParts;
    while (pParts && !pParts->IsEmpty()) {
        VATSBodyPart* pPart = pParts->GetItem();
        BSSimpleList<VATSBodyPart*>* pOther = pParts->GetNext();
        // The part type compares signed.
        int32_t iPart = static_cast<int32_t>(pPart->eBodyPart);
        if (iPart > 0 && iPart <= 12) {
            while (pOther && !pOther->IsEmpty()) {
                VATSBodyPart* pOtherPart = pOther->GetItem();
                ActorValue::Index ePartValue = GetActorValueFromBodyPart(
                    static_cast<BGSBodyPart::LIMB_ENUM>(pPart->eBodyPart)
                );
                ActorValue::Index eOtherValue = GetActorValueFromBodyPart(
                    static_cast<BGSBodyPart::LIMB_ENUM>(pOtherPart->eBodyPart)
                );
                if (ePartValue == eOtherValue) {
                    if (!pOtherPart->pMeter) {
                        BuildMetersForTarget(GetVatsTarget(pVATSTarget));
                    }
                    if (pOtherPart != pPart) {
                        // Tests the part's type too, and hides the other meter either way.
                        if (pOtherPart->eBodyPart == 1) {
                            pPart->pMeter->Set(Tile::kTileValue_Visible, 0);
                        }
                        else if (pPart->eBodyPart == 1) {
                            pOtherPart->pMeter->Set(Tile::kTileValue_Visible, 0);
                        }
                        else {
                            pOtherPart->pMeter->Set(Tile::kTileValue_Visible, 0);
                        }
                    }
                    // The other meter's percentage is kept as a double.
                    double dOtherPercent = pOtherPart->pMeter->GetFloat(Tile::AddUserTrait("_MeterPercent", -1));
                    // Only a greater percentage (not NaN) is copied.
                    if (pPart->pMeter->GetFloat(Tile::AddUserTrait("_MeterPercent", -1)) > dOtherPercent) {
                        float fPercent = pOtherPart->pMeter->GetFloat(Tile::AddUserTrait("_MeterPercent", -1));
                        pPart->pMeter->Set(Tile::AddUserTrait("_MeterPercent", -1), fPercent, true);
                    }
                    // Only a lower chance (not NaN) is raised.
                    if (pPart->fChance < pOtherPart->fChance) {
                        pPart->fChance = pOtherPart->fChance;
                    }
                }
                pOther = pOther->GetNext();
            }
        }
        int32_t iConsShots;
        if (!pVATS.commandList.IsEmpty()) {
            VATS_COMMAND* pLastAction = pVATS.GetLastAction();
            if (pLastAction->eLimb ==
                GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pPart->eBodyPart))) {
                iConsShots = GetConsecutiveShots() + 1;
            }
            else {
                iConsShots = 0;
            }
        }
        else {
            iConsShots = 0;
        }
        char cBuffer[8];
        if ((!pPlayer->GetCurrentWeapon() || pPlayer->GetCurrentWeapon()->IsMeleeWeapon()) &&
            pVATSTarget->IsActor() &&
            static_cast<TESActorBaseData*>(static_cast<Actor*>(pVATSTarget)->GetActorBase())->GetNoVATSMelee()) {
            BSsprintf(cBuffer, 8, "%i%%", 0);
            pPart->fChance = 0.0f;
        }
        else if (iConsShots > 0 && bConcentratedFirePerk) {
            // The chance is multiplied by the double 100.0, the bonus is added in double precision, and the sum is
            // truncated to an integer.
            double dChance = pPart->fChance * 100.0;
            int32_t iBonus = iVATSConcentratedFireBonus.Int() * iConsShots;
            BSsprintf(
                cBuffer,
                8,
                "%i%%",
                BSMAX(0, BSMIN(static_cast<int32_t>(static_cast<double>(iBonus) + dChance), 95))
            );
        }
        else {
            BSsprintf(cBuffer, 8, "%i%%", BSMAX(0, BSMIN(static_cast<int32_t>(pPart->fChance * 100.0), 95)));
        }
        if (!pPart->pMeter) {
            BuildMetersForTarget(GetVatsTarget(pVATSTarget));
        }
        pPart->pMeter->Set(Tile::kTileValue_String, cBuffer, true);
        pParts = pParts->GetNext();
    }
}

// Builds a percent meter under the menu's glow branch for each body part of apTarget's target, justified by the part.
void VATSMenu::BuildMetersForTarget(BSSimpleList<VATSTarget*>* apTarget) {
    Tile* pGlow = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eVATSMenu), "GlowBranch");
    for (BSSimpleList<VATSBodyPart*>* pParts = &apTarget->GetItem()->kBodyParts; pParts && !pParts->IsEmpty();
         pParts = pParts->GetNext()) {
        pParts->GetItem()->pMeter = VATSMenu::pVatsMenu->RenderTemplate(pGlow, "body_part_percent", nullptr);
        if (pParts->GetItem()->pMeter) {
            switch (static_cast<BGSBodyPart::LIMB_ENUM>(pParts->GetItem()->eBodyPart)) {
                case BGSBodyPart::LIMB_NONE:
                case BGSBodyPart::LIMB_HEAD_1:
                case BGSBodyPart::LIMB_HEAD_2:
                case BGSBodyPart::LIMB_BRAIN: pParts->GetItem()->pMeter->Set(Tile::kTileValue_Justify, 5); break;
                case BGSBodyPart::LIMB_TORSO: pParts->GetItem()->pMeter->Set(Tile::kTileValue_Justify, 6); break;
                case BGSBodyPart::LIMB_LEFT_ARM_1:
                case BGSBodyPart::LIMB_LEFT_ARM_2:
                case BGSBodyPart::LIMB_LEFT_LEG_1:
                case BGSBodyPart::LIMB_LEFT_LEG_2:
                case BGSBodyPart::LIMB_LEFT_LEG_3:
                case BGSBodyPart::LIMB_WEAPON: pParts->GetItem()->pMeter->Set(Tile::kTileValue_Justify, 1); break;
                case BGSBodyPart::LIMB_RIGHT_ARM_1:
                case BGSBodyPart::LIMB_RIGHT_ARM_2:
                case BGSBodyPart::LIMB_RIGHT_LEG_1:
                case BGSBodyPart::LIMB_RIGHT_LEG_2:
                case BGSBodyPart::LIMB_RIGHT_LEG_3: pParts->GetItem()->pMeter->Set(Tile::kTileValue_Justify, 4); break;
                default: break;
            }
        }
    }
}

bool VATSMenu::LimbAppeared() {
    BSSimpleList<VATSTarget*>* pTargetList = GetVatsTarget(pVATSTarget);
    for (BSSimpleList<VATSBodyPart*>* pPartsList = &pTargetList->GetItem()->kBodyParts;
         pPartsList && !pPartsList->IsEmpty();
         pPartsList = pPartsList->GetNext()) {
        if (!pPartsList->GetItem()->bOnScreen) {
            NiPoint3 targetPoint = pPartsList->GetItem()->kPos;
            float fX = 0.0;
            float fY = 0.0;
            Main::GetWorldSceneGraph()->GetCamera()->WorldPtToScreenPt(targetPoint, fX, fY, 1e-05f);
            if (!(fX <= 0.0) && !(fY <= 0.0) && !(fX >= 1.0) && !(fY >= 1.0)) {
                return true;
            }
        }
    }
    return false;
}

void VATSMenu::GetScreenPlaneProjectionMatrix(NiMatrix3* apmatrix) {
    NiPoint3 kDir = Main::GetWorldSceneGraph()->GetCamera()->GetWorldDirection();
    // One row buffer, refilled for each of the three rows.
    NiPoint3 kRow;
    kRow.x = kDir.y * kDir.y + kDir.z * kDir.z;
    kRow.y = -kDir.x * kDir.y;
    kRow.z = -kDir.x * kDir.z;
    apmatrix->SetRow(0, kRow);
    kRow.x = -kDir.x * kDir.y;
    kRow.y = kDir.x * kDir.x + kDir.z * kDir.z;
    kRow.z = -kDir.y * kDir.z;
    apmatrix->SetRow(1, kRow);
    kRow.x = -kDir.x * kDir.z;
    kRow.y = -kDir.y * kDir.z;
    kRow.z = kDir.x * kDir.x + kDir.y * kDir.y;
    apmatrix->SetRow(2, kRow);
}

bool VATSMenu::LimbOnScreen(VATSBodyPart* apLimb) {
    NiPoint3 kPt = apLimb->kPos;
    float fBx = 0.0;
    float fBy = 0.0;
    Main::GetWorldSceneGraph()->GetCamera()->WorldPtToScreenPt(kPt, fBx, fBy, 1e-05f);
    if (!(fBx <= 0.0) && !(fBy <= 0.0) && !(fBx >= 1.0) && !(fBy >= 1.0)) {
        apLimb->bOnScreen = true;
        return true;
    }

    apLimb->bOnScreen = false;
    return false;
}

// The player's chance to hit the body part apList holds (its first item) on the current target, with the player's
// weapon. The weapon condition and the targeted weapon are the target's own weapon's. Melee and
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
    TESObjectWEAP* pWeapon = pPlayerWeaponItem ? static_cast<TESObjectWEAP*>(pPlayerWeaponItem->pContainerObj)
                                               : nullptr;
    ItemChange* pTargetWeaponItem = pVATSTarget->IsActor()
        ? static_cast<Actor*>(pVATSTarget)->pCurrentProcess->GetCurrentWeapon()
        : nullptr;
    double dWeaponHealth;
    if (pTargetWeaponItem) {
        dWeaponHealth = pTargetWeaponItem->GetItemHealth(true) / 100.0;
    }
    else {
        dWeaponHealth = 1.0;
    }
    float fWeaponHealth = static_cast<float>(dWeaponHealth);
    TESObjectWEAP* pTargetWeapon = pTargetWeaponItem ? static_cast<TESObjectWEAP*>(pTargetWeaponItem->pContainerObj)
                                                     : nullptr;
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
    float fGunSkill = pWeapon ? pPlayer->GetGunSkill(GUN_SKILL_VATS) : 1.0f;
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
        pShooter,
        pVATSTarget,
        pWeapon,
        fWeaponHealth,
        fReach,
        fGunSkill,
        pTargetedLimb,
        pTargetedWeapon,
        fPercentVisible,
        fDistance,
        fBoundSize,
        nullptr
    );
    bool bPathTooLong = false;
    if (!pWeapon || pWeapon->TESObjectWEAP::IsMeleeWeapon()) {
        PathingLocation kPlayerLocation(pPlayer);
        PathingLocation kTargetLocation(pVATSTarget);
        float fPathDistance = std::numeric_limits<float>::max();
        if (!Pathing::ComputeApproximatePathingDistance(
                kPlayerLocation,
                kTargetLocation,
                nullptr,
                &fPathDistance,
                nullptr
            )) {
            fPathDistance = std::numeric_limits<float>::max();
        }
        // The target's position is taken first; the length is rounded to a float before the doubling.
        const NiPoint3& rTargetPos = pVATSTarget->GetLocationOnReference();
        float fStraightDistance = (pPlayer->GetLocationOnReference() - rTargetPos).Length();
        if (fPathDistance > fStraightDistance * 2.0) {
            bPathTooLong = true;
        }
    }
    float fModifiedChance = bPathTooLong ? 0.0f : fFinalHitChance;
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
    TESObjectWEAP* pPerkWeapon = pWeapon ? pWeapon : pFists;
    BGSEntryPoint::HandleEntryPoint(
        BGSEntryPoint::CALCULATE_TO_HIT_CHANCE,
        pPlayer,
        pPerkWeapon,
        pVATSTarget,
        &fModifiedChance
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
    VATSTarget* pTarget = GetVatsTarget(pVATSTarget)->m_item;
    if (pTarget) {
        for (BSSimpleList<VATSBodyPart*>* pIter = &pTarget->kBodyParts; pIter && !pIter->IsEmpty();
             pIter = pIter->m_pkNext) {
            if (GetActorValueFromBodyPart(static_cast<BGSBodyPart::LIMB_ENUM>(pIter->m_item->eBodyPart)) == aeLimb) {
                return pIter;
            }
        }
    }
    return nullptr;
}

void VATSMenu::SelectNearestTarget(bool abLeft) {
    TESObjectWEAP* pWeapon;
    BSSoundHandle soundHand;
    soundHand = BSAudio::QInstance().GetSoundHandleByName(
        "UIVATSSelectTarget",
        BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
    );
    soundHand.Play(false);
    BSSimpleList<VATSTarget*>* pTargetIter = &VATSMenu::xTargetList;
    float fPlayerHeading = pPlayer->GetHeading(false) * 57.2957763671875;
    NiPoint3 cPlayerLoc = pPlayer->GetLocationOnReference();
    float fStoredAngle = static_cast<float>(abLeft ? -360 : 360);
    BaseProcess* pProcess = pPlayer->pCurrentProcess;
    ItemChange* pWeap = pProcess->GetCurrentWeapon();
    if (pWeap) {
        pWeapon = static_cast<TESObjectWEAP*>(pWeap->pContainerObj);
    }
    else {
        pWeapon = nullptr;
    }
    while (pTargetIter && !pTargetIter->IsEmpty()) {
        TESObjectREFR* pTargetRef = pTargetIter->m_item->pTargetRef;
        float fTargetHeading = HUDMainMenu::HeadingBetweenLocations(cPlayerLoc, pTargetRef->GetLocationOnReference());
        float fAngle = fPlayerHeading - fTargetHeading;
        double dAngle;
        if (fAngle > 180.0) {
            dAngle = fAngle - 360.0;
        }
        else {
            dAngle = fAngle;
        }
        fAngle = dAngle;
        if ((fStoredAngle > static_cast<double>(fAngle) && fAngle > 0.0 && !abLeft) ||
            (fStoredAngle < static_cast<double>(fAngle) && fAngle < 0.0 && abLeft)) {
            fStoredAngle = fAngle;
            pVATSTarget = pTargetIter->m_item->pTargetRef;
            VATSMenu::SetupSpecialAttackButtons(pWeapon);
            BSSimpleList<VATSBodyPart*>* pParts = &pTargetIter->m_item->kBodyParts;
            VATSMenu::pVatsMenu->pCurrentLimb = pParts->m_item;
            while (pParts && !pParts->IsEmpty()) {
                if (pTargetIter->m_item->eType == 1 && pParts->m_item->eBodyPart == 1) {
                    VATSMenu::pVatsMenu->pCurrentLimb = pParts->m_item;
                }
                pParts = pParts->m_pkNext;
            }
        }
        pTargetIter = pTargetIter->m_pkNext;
    }
}

void VATSMenu::Close() {
    float fFortune;
    float fStranger;
    if (VATSMenu::pVatsMenu && VATSMenu::pVatsMenu->xFadeState == Interface::FADE_SHOWN) {
        if (!VATSMenu::pVatsMenu->bClosing) {
            BSAudio::QInstance().UnPauseWorldSounds();
        }
        VATSMenu::pVatsMenu->bClosing = true;
        Tile* pTile = Menu::GetMenuByClass(VATSMenu::GetClassStatic());
        if (pTile) {
            if (VATSMenu::pVatsMenu) {
                if (!VATSMenu::Destructibles.IsEmpty()) {
                    VATSMenu::Destructibles.RemoveAll();
                }
                InterfaceManager::Instance()->RefreshActivateREFR();
                BSAudio::QInstance().EndAllSoundsOfType(256u);
                if (!pSaveLoadGame->GetSaveGameLoading()) {
                    BSAudioManager* mgr = &BSAudioManager::QInstance();
                    BSSoundHandle soundHand;
                    if (VATSMenu::xTargetList.IsEmpty()) {
                        soundHand = mgr->GetSoundHandleByName(
                            "UIVATSEnterFail",
                            BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                BSGameSound::SOUND_FLAG_ONE_SHOT
                        );
                    }
                    else {
                        soundHand = mgr->GetSoundHandleByName(
                            "UIVATSExit",
                            BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                BSGameSound::SOUND_FLAG_ONE_SHOT
                        );
                    }
                    soundHand.Play(false);
                }
                pTile->Set(Tile::kTileValue_DeleteOnFade, 1);
                VATSMenu::pVatsMenu->StartFadeOut();
            }
            if (pVATS.GetCount()) {
                BSAudioManager* mgr = &BSAudioManager::QInstance();
                BSSoundHandle soundHand;
                soundHand = mgr->GetSoundHandleByName(
                    "UIVATSReady",
                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                        BSGameSound::SOUND_FLAG_ONE_SHOT
                );
                soundHand.Play(false);
                fFortune = fVATSFortuneOdds.Float();
                fStranger = fVATSStrangerOdds.Float();
                BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::MODIFY_VATS_HELPER_CHANCE, pPlayer, &fFortune);
                BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::MODIFY_VATS_HELPER_CHANCE, pPlayer, &fStranger);
                if (VATS::pStranger &&
                    pVATSTarget &&
                    pVATSTarget->IsActor() &&
                    !static_cast<Actor*>(pVATSTarget)->Actor::GetEssential()) {
                    double dRoll = RandomFloat(0.0f, 1.0f);
                    if (fStranger >= dRoll) {
                        TESFaction* pFaction = static_cast<TESFaction*>(
                            BGSDefaultObjectManager::GetDefaultObject(DO_PERK_STRANGER_FACTION)
                        );
                        if (pFaction && !static_cast<Actor*>(pVATSTarget)->Actor::IsInFaction(pFaction)) {
                            VATS_COMMAND* pCommand = new VATS_COMMAND;
                            pCommand->eLimb = ActorValue::PERCEPTION_CONDITION;
                            pCommand->cFireShots = 5;
                            pCommand->eAction = ActionPoints::ACTION_ATTACK_PISTOL;
                            pCommand->bStranger = true;
                            pCommand->pTarget = pVATSTarget;
                            pCommand->bSuccess = true;
                            pVATS.QueueAction(pCommand);
                            VATS::pStranger->StartCombat(
                                static_cast<Actor*>(pVATSTarget),
                                nullptr,
                                true,
                                true,
                                false,
                                0,
                                true,
                                nullptr
                            );
                        }
                    }
                }

                if (VATS::pMissFortune &&
                    pVATSTarget &&
                    pVATSTarget->IsActor() &&
                    !static_cast<Actor*>(pVATSTarget)->Actor::GetEssential()) {
                    double dRoll = RandomFloat(0.0f, 1.0f);
                    if (fFortune >= dRoll) {
                        VATS_COMMAND* pCommand = new VATS_COMMAND;
                        pCommand->eLimb = ActorValue::PERCEPTION_CONDITION;
                        pCommand->cFireShots = 5;
                        pCommand->eAction = ActionPoints::ACTION_ATTACK_PISTOL;
                        pCommand->bMissFortune = true;
                        pCommand->pTarget = pVATSTarget;
                        pCommand->bSuccess = true;
                        pVATS.QueueAction(pCommand);
                        VATS::pMissFortune->StartCombat(
                            static_cast<Actor*>(pVATSTarget),
                            nullptr,
                            true,
                            true,
                            false,
                            0,
                            true,
                            nullptr
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
            Interface::GetVATSEffect()->SetTarget(nullptr, BGSBodyPart::LIMB_NONE, false);
            Interface::GetVATSEffect()->ClearMultiTargets();
            bcancelVATS = 0;
        }
        else {
            pVATS.SetMode(VATS::VATS_MODE_NONE, false);
            bFOVTargetReached = false;
        }
    }
}

bool VATSMenu::GetIsARescan() {
    return VATSMenu::bIsARescan;
}

int32_t VATSMenu::GetClassStatic() {
    return 1056;
}

void VATSMenu::SetHeadingReached(bool abHeadingReached) {
    if (VATSMenu::pVatsMenu) {
        VATSMenu::pVatsMenu->bHeadingReached = abHeadingReached;
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
        Actor* pTargetActor = pIter->GetItem()->pTargetRef->IsActor()
            ? static_cast<Actor*>(pIter->GetItem()->pTargetRef)
            : nullptr;
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
            (pBannedTargets &&
             pTargetBase &&
             static_cast<BGSListForm*>(pBannedTargets)->GetFormList()->IsInList(pBannedBase)) ||
            (pTargetActor && pTargetActor->IsSwimming()) ||
            bStuck) {
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
    if (pPlayerWeapon &&
        pBannedWeapons &&
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
            NiPoint3 kBack = NiPoint3::ZERO;
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
                pStrangerObject,
                kLocation,
                NiPoint3::ZERO,
                pInterior,
                pWorld,
                nullptr,
                nullptr,
                nullptr
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
            NiPoint3 kBack = NiPoint3::ZERO;
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
                pMissFortuneObject,
                kLocation,
                NiPoint3::ZERO,
                pInterior,
                pWorld,
                nullptr,
                nullptr,
                nullptr
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
            kPlayerLocation,
            pIter->GetItem()->pTargetRef->GetLocationOnReference()
        );
        float fHeadingDelta = fPlayerHeading - fHeadingTo;
        float fAngle = std::fabs(fHeadingDelta);
        NiPoint3 kOffset = kPlayerLocation - pIter->GetItem()->pTargetRef->GetLocationOnReference();
        float fDistanceSqr = NiPoint3::VectorLength(&kOffset);
        double dDistance = fDistanceSqr > 0.0 ? fDistanceSqr : 0.001;
        double dAngle = fAngle > 180.0 ? 360.0 - fAngle : fAngle;
        float fScore = static_cast<float>(40.0 / dDistance + (180.0 - dAngle) / 180.0);
        pIter->GetItem()->fDesiredTarget = fScore;
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
        bool bSelect = fBestScore < pTarget->fDesiredTarget && !bInAngle;
        if (!bSelect) {
            pTarget = pIter->GetItem();
            // Compares against the setting's value as a signed int.
            int32_t iTargetAngle = static_cast<int32_t>(iVatsTargetAngle.UInt());
            bSelect = static_cast<double>(pTarget->fAngle) < static_cast<double>(iTargetAngle) &&
                pIter->GetItem()->bHostile &&
                (!pBest || pBest->GetItem()->fDistance > pIter->GetItem()->fDistance);
        }
        if (bSelect) {
            pTarget = pIter->GetItem();
            int32_t iTargetAngle = static_cast<int32_t>(iVatsTargetAngle.UInt());
            if (static_cast<double>(pTarget->fAngle) < static_cast<double>(iTargetAngle)) {
                bInAngle = true;
            }
            fBestScore = pIter->GetItem()->fDesiredTarget;
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
    VATSMenu::pVatsMenu->pTiles[20] = VATSMenu::VatsMenuRenderTemplate(
        VATSMenu::pVatsMenu->pTiles[23],
        "template_compass_icon_group"
    );
    if (!VATSMenu::pVatsMenu->pTiles[20]) {
        return false;
    }
    VATSMenu::pVatsMenu->pTiles[20]->Set(Tile::kTileValue_Y, 30);
    VATSMenu::pVatsMenu->pTiles[21] = VATSMenu::VatsMenuRenderTemplate(
        VATSMenu::pVatsMenu->pTiles[23],
        "template_compass_icon_group"
    );
    if (!VATSMenu::pVatsMenu->pTiles[21]) {
        return false;
    }
    VATSMenu::pVatsMenu->pTiles[21]->Set(Tile::kTileValue_Y, 30);
    VATSMenu::pVatsMenu->pTiles[22] = VATSMenu::VatsMenuRenderTemplate(
        VATSMenu::pVatsMenu->pTiles[23],
        "template_compass_icon_group"
    );
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
    VATSMenu::pVatsMenu->pTiles[27] = VATSMenu::VatsMenuRenderTemplate(
        VATSMenu::pVatsMenu->pTiles[28],
        "template_meter"
    );
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
    NiSourceTexture* pAlphaMap = spAlphaMap;
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
    // BODY_PART_TYPE_NONE.
    VATSMenu::uiLastBodyPart = 0xFFFFFFFF;
    eVATSTargetLimb = ActorValue::PERCEPTION_CONDITION;
    pPlayer->SetIronSights(false, false, false);
    bUnk11DB0CC = false;
    VATSMenu::pVatsMenu->lbActions.Initialize(VATSMenu::pVatsMenu->pTiles[1], "queued_action");
    BSAudioManager* pAudioManager = &BSAudioManager::QInstance();
    BSSoundHandle kEnterSound;
    kEnterSound = pAudioManager->GetSoundHandleByName(
        "UIVATSEnter",
        static_cast<BSGameSound::SoundFlags>(
            BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
        )
    );
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
    InterfaceManager::Instance()
        ->xTutorialManager.ShowMessage(static_cast<Interface::TutorialMessageID>(eTutorial), iMenuID, 512);
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
    bool bValidTargetsFound = false;
    Main::GetWorldSceneGraph()->GetCamera()->WorldToCameraMatrix();
    BSSimpleList<CompassTarget*>* pList = pPlayer->GetListofPerceivedActors();
    float fMaxNPCDist = 0.0f;
    bool bInterior = pPlayer->GetParentCell()->GetInterior();
    VATSMenu::Destructibles.RemoveAll();
    while (pList && !pList->IsEmpty()) {
        CompassTarget* pAct = pList->m_item;
        bool bUntargetable = true;
        if (pAct) {
            bUntargetable = BGSDestructibleObjectForm::GetDestructionForm(pAct->pTarget->GetObjectReference())->pData &&
                !pAct->pTarget->GetVATStarget();
        }
        if (pAct &&
            !pAct->pTarget->IsDead(false) &&
            pAct->pTarget->Get3D() &&
            pAct->pTarget->GetCurrentProcessType() == PROCESS_TYPE_HIGH &&
            !pAct->pTarget->IsChild(false) &&
            !bUntargetable) {
            float fDist = pAct->pTarget->GetDistanceFromReference(pPlayer, false, false);
            if (!(fDist < 2.0)) {
                if (!(fVATSMaxEngageDistance.Float() < static_cast<double>(fDist)) &&
                    pPlayer->LineOfSight(true, pAct->pTarget, true, nullptr, true)) {
                    if (fMaxNPCDist < static_cast<double>(fDist)) {
                        fMaxNPCDist = fDist;
                    }
                    alignas(16) NiPoint3 targetPoint = pAct->pTarget->Get3D()->GetWorldTranslate();
                    float fX = 0.0;
                    float fY = 0.0;
                    Main::GetWorldSceneGraph()->GetCamera()->WorldPtToScreenPt(targetPoint, fX, fY, 1e-05f);
                    bool bOnScreen = !(fX <= 0.0) && !(fY <= 0.0) && !(fX >= 1.0) && !(fY >= 1.0);
                    char bVisible = bOnScreen;
                    if (bOnScreen) {
                        bValidTargetsFound = true;
                    }
                    else {
                        TESActorBase* pActorBase = pAct->pTarget->GetActorBase();
                        BGSBodyPartData* pBodyPartData = pActorBase->GetBodyPartData();
                        BGSBodyPart** pBodyPartArray = pBodyPartData->pPartArray;
                        for (BGSBodyPart::LIMB_ENUM eIndex = BGSBodyPart::LIMB_TORSO; eIndex < BGSBodyPart::LIMB_COUNT;
                             eIndex = static_cast<BGSBodyPart::LIMB_ENUM>(eIndex + 1)) {
                            if (pBodyPartArray[eIndex]) {
                                Actor* pTarget = pList->m_item->pTarget;
                                const char* pTargetName = pBodyPartData->GetBodyPart(eIndex)->GetTargetName();
                                NiAVObject* pLimb = BSUtilities::GetObjectByName(pTarget->Get3D(), pTargetName);
                                if (pLimb) {
                                    float fLimbX;
                                    alignas(16) float fLimbY;
                                    const NiPoint3& rLimbPos = pLimb->GetWorldTranslate();
                                    Main::GetWorldSceneGraph()
                                        ->GetCamera()
                                        ->WorldPtToScreenPt(rLimbPos, fLimbX, fLimbY, 1e-05f);
                                    if (fLimbX > 0.0 && fLimbY > 0.0 && fLimbX < 1.0 && fLimbY < 1.0) {
                                        bVisible = 1;
                                        bValidTargetsFound = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if ((((!bInterior || !(fSneakMaxDistance.Float() < static_cast<double>(fDist))) &&
                          (bInterior || !(fDist > fSneakExteriorDistanceMult.Float() * fSneakMaxDistance.Float()))) ||
                         bVisible) &&
                        pAct->pTarget->Get3D()) {
                        VATSTarget* pNewTarget = new VATSTarget;
                        pNewTarget->eType = 1;
                        pNewTarget->pTargetRef = pAct->pTarget;
                        pNewTarget->bChancesCalculated = false;
                        pNewTarget->bHostile = pList->m_item->bIsHostile;
                        // Reads the target's base through Actor::GetActorBase without testing that the
                        // reference is an actor.
                        TESActorBase* pTargetBase = static_cast<Actor*>(pNewTarget->pTargetRef)->GetActorBase();
                        BGSBodyPart** pPartArray = pTargetBase->GetBodyPartData()->pPartArray;
                        for (int32_t j = 0; j < 15; ++j) {
                            if (pPartArray[j]) {
                                VATSBodyPart* pPart = new VATSBodyPart;
                                pPart->bOnScreen = false;
                                pPart->bChanceCalculated = false;
                                pPart->bFirstTimeShown = false;
                                pPart->bNeedsRecalc = true;
                                pPart->kPos = NiPoint3::ZERO;
                                pPart->fChance = 0.0f;
                                pPart->eBodyPart = j;
                                pNewTarget->kBodyParts.AddHead(pPart);
                            }
                        }
                        NiNode* pWeaponBone = nullptr;
                        if (pNewTarget->pTargetRef->IsActor()) {
                            BaseProcess* pProcess = static_cast<Actor*>(pNewTarget->pTargetRef)->GetCurrentAIProcess();
                            pWeaponBone = pProcess->GetWeaponBone(pNewTarget->pTargetRef->GetBiped());
                        }
                        ItemChange* pTargetWeaponItem;
                        if (pNewTarget->pTargetRef->IsActor()) {
                            pTargetWeaponItem = static_cast<Actor*>(pNewTarget->pTargetRef)
                                                    ->GetCurrentAIProcess()
                                                    ->GetCurrentWeapon();
                        }
                        else {
                            pTargetWeaponItem = nullptr;
                        }
                        TESObjectWEAP* pTargetWeapon;
                        if (pTargetWeaponItem) {
                            pTargetWeapon = static_cast<TESObjectWEAP*>(pTargetWeaponItem->pContainerObj);
                        }
                        else {
                            pTargetWeapon = nullptr;
                        }
                        if (pWeaponBone && pTargetWeapon && !pTargetWeapon->IsEmbeddedWeapon()) {
                            VATSBodyPart* pPart = new VATSBodyPart;
                            pPart->bOnScreen = false;
                            pPart->bChanceCalculated = false;
                            pPart->bFirstTimeShown = false;
                            pPart->bNeedsRecalc = true;
                            pPart->kPos = NiPoint3::ZERO;
                            pPart->eBodyPart = 14;
                            pNewTarget->kBodyParts.AddHead(pPart);
                        }

                        if (pNewTarget->kBodyParts.IsEmpty()) {
                            pNewTarget->eType = 2;
                            VATSBodyPart* pPart = new VATSBodyPart;
                            pPart->eBodyPart = -1;
                            pPart->bOnScreen = false;
                            pPart->bChanceCalculated = false;
                            pPart->bFirstTimeShown = false;
                            pPart->bNeedsRecalc = true;
                            pPart->kPos = NiPoint3::ZERO;
                            pNewTarget->kBodyParts.AddHead(pPart);
                        }
                        VATSMenu::xTargetList.AddHead(pNewTarget);
                    }
                }
            }
        }

        pList = pList->m_pkNext;
    }
    if (fMaxNPCDist != 0.0) {
        fMaxNPCDist = fMaxNPCDist + 128.0;
    }
    if (pPlayer->GetWorldSpace()) {
        PlayerCharacter* pFilterData = pPlayer;
        float fRadius2 = fSneakExteriorDistanceMult.Float() * fSneakMaxDistance.Float();
        const NiPoint3& rPoint2 = pPlayer->GetLocationOnReference();
        float fRadius1 = fSneakExteriorDistanceMult.Float() * fSneakMaxDistance.Float();
        const NiPoint3& rPoint1 = pPlayer->GetLocationOnReference();
        pPlayer->GetWorldSpace()->EnumReferencesCloseToPoint(
            rPoint1,
            fRadius1,
            rPoint2,
            fRadius2,
            &VATSMenu::FilterDestructibleObjects,
            pFilterData
        );
    }
    else {
        PlayerCharacter* pFilterData = pPlayer;
        float fRadius2 = fSneakExteriorDistanceMult.Float() * fSneakMaxDistance.Float();
        const NiPoint3& rPoint2 = pPlayer->GetLocationOnReference();
        float fRadius1 = fSneakExteriorDistanceMult.Float() * fSneakMaxDistance.Float();
        const NiPoint3& rPoint1 = pPlayer->GetLocationOnReference();
        pPlayer->GetParentCell()->EnumReferencesCloseToPoint(
            rPoint1,
            fRadius1,
            rPoint2,
            fRadius2,
            &VATSMenu::FilterDestructibleObjects,
            pFilterData
        );
    }
    for (BSSimpleList<TESObjectREFR*>* pDestructibleIter = &VATSMenu::Destructibles;
         pDestructibleIter && !pDestructibleIter->IsEmpty();
         pDestructibleIter = pDestructibleIter->m_pkNext) {
        float fDist = pDestructibleIter->m_item->GetDistanceFromReference(pPlayer, false, false);
        if (!pDestructibleIter->m_item->IsActor() && !pDestructibleIter->m_item->IsExplosion()) {
            double dDistance;
            if ((!(fMaxNPCDist > 0.0) ||
                 (dDistance = pDestructibleIter->m_item->GetDistanceFromReference(pPlayer, false, false),
                  !(fMaxNPCDist < dDistance))) &&
                !pDestructibleIter->m_item->GetDestroyed() &&
                !pDestructibleIter->m_item->GetDisabled() &&
                !pDestructibleIter->m_item->GetDelete()) {
                if (!bInterior || !(fSneakMaxDistance.Float() < static_cast<double>(fDist))) {
                    if ((bInterior || !(fDist > fSneakExteriorDistanceMult.Float() * fSneakMaxDistance.Float())) &&
                        pDestructibleIter->m_item->GetVATStarget()) {
                        if ((!pDestructibleIter->m_item->IsProjectile() ||
                             static_cast<Projectile*>(pDestructibleIter->m_item)->GetShooter() != pPlayer) &&
                            !pDestructibleIter->m_item->GetDestroyed()) {
                            VATSTarget* pNewTarget = new VATSTarget;
                            pNewTarget->eType = 2;
                            pNewTarget->pTargetRef = pDestructibleIter->m_item;
                            pNewTarget->bChancesCalculated = false;
                            pNewTarget->bHostile = false;
                            VATSBodyPart* pPart = new VATSBodyPart;
                            pPart->eBodyPart = -1;
                            pPart->bOnScreen = false;
                            pPart->bChanceCalculated = false;
                            pPart->bFirstTimeShown = false;
                            pPart->bNeedsRecalc = true;
                            pPart->kPos = NiPoint3::ZERO;
                            pNewTarget->kBodyParts.AddHead(pPart);
                            if (!bValidTargetsFound) {
                                alignas(16)
                                    NiPoint3 targetPoint = pDestructibleIter->m_item->Get3D()->GetWorldTranslate();
                                float fX = 0.0;
                                float fY = 0.0;
                                Main::GetWorldSceneGraph()->GetCamera()->WorldPtToScreenPt(targetPoint, fX, fY, 1e-05f);
                                bool bOnScreen = !(fX <= 0.0) && !(fY <= 0.0) && !(fX >= 1.0) && !(fY >= 1.0);
                                if (bOnScreen) {
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
                                    pd.NiSetTo(pDestructibleIter->m_item->Get3D()->GetWorldBound().m_kCenter);
                                    pd.NiSetLength(
                                        pDestructibleIter->m_item->Get3D()->GetWorldBound().m_kCenter - origin
                                    );
                                    if (pPlayer->GetParentCell()->GetbhkWorld()->PickObject(pd) && pd.pAllCollector) {
                                        for (int32_t i = 0;
                                             !bValidTargetsFound && i < pd.pAllCollector->getHits().m_size;
                                             ++i) {
                                            hkpWorldRayCastOutput point(pd.pAllCollector->getHits()[i]);
                                            NiNode* pAV = static_cast<NiNode*>(
                                                GetAVObjectForCollidable(point.m_rootCollidable)
                                            );
                                            if (pAV && pAV == pDestructibleIter->m_item->Get3D()) {
                                                bValidTargetsFound = true;
                                            }
                                        }
                                    }
                                }
                            }
                            VATSMenu::xTargetList.AddHead(pNewTarget);
                        }
                    }
                }
            }
        }
    }
    if (!bValidTargetsFound) {
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
        pd.NiSetTo(Main::GetWorldSceneGraph()->GetCamera()->GetWorldDirection());
        double dSneakDistance;
        if (bInterior) {
            dSneakDistance = fSneakMaxDistance.Float();
        }
        else {
            dSneakDistance = fSneakExteriorDistanceMult.Float() * fSneakMaxDistance.Float();
        }
        float fRange = dSneakDistance;
        pd.NiSetLength(Main::GetWorldSceneGraph()->GetCamera()->GetWorldDirection() * fRange);
        if (pPlayer->GetParentCell()->GetbhkWorld()->PickObject(pd) && pd.pAllCollector) {
            for (int32_t i = 0; !bValidTargetsFound && i < pd.pAllCollector->getHits().m_size; ++i) {
                hkpWorldRayCastOutput point(pd.pAllCollector->getHits()[i]);
                NiAVObject* pAV = GetAVObjectForCollidable(point.m_rootCollidable);
                if (pAV) {
                    TESObjectREFR* pHitRef = TESObjectREFR::FindReferenceFor3D(pAV);
                    if (pHitRef &&
                        BGSDestructibleObjectForm::IsDestructible(pHitRef->GetObjectReference()) &&
                        pHitRef->IsProjectile()) {
                        bValidTargetsFound = true;
                    }
                }
            }
        }
    }
    if (!bValidTargetsFound) {
        VATSMenu::ClearTargetList();
    }
    return bValidTargetsFound;
}

void VATSMenu::SetupSpecialAttackButtons(TESObjectWEAP* pweapon) {
    ActorValue::Index eWeaponSkill;
    double dVATSSkill;
    if (pweapon &&
        pweapon->GetVATSAP() > 0.0 &&
        (dVATSSkill = pweapon->GetVATSSkill(),
         eWeaponSkill = pweapon->GetWeaponSkill(),
         static_cast<double>(pPlayer->GetActorValue(eWeaponSkill)) >= dVATSSkill)) {
        const char* pAttackName = pweapon->GetVATSAttackName();
        VATSMenu::pVatsMenu->pTiles[30]->Set(4036, pAttackName, true);
        VATSMenu::pVatsMenu->pTiles[30]->Set(4003, 1);
        bWeaponHasSpecial = 1;
        VATSMenu::pVatsMenu->pTiles[31]->Set(4003, 0);
        bWeaponHasSpecial2 = 0;
        fSpecialAttackDamageMult = pweapon->specialVATSData.fVATSSpecialMultiplier;
        fSpecialAttackAPCost = pweapon->GetVATSAP();
        BGSEntryPoint::HandleEntryPoint(
            BGSEntryPoint::ACTION_POINT_COST_MODIFIER_ENTRY,
            pPlayer,
            pweapon,
            &fSpecialAttackAPCost
        );
    }
    else {
        double dUnarmed;
        if ((pweapon && pweapon->GetWeaponSkill() != ActorValue::UNARMED) ||
            (dUnarmed = static_cast<double>(pPlayer->GetActorValue(ActorValue::UNARMED)),
             !(fUpperCutThreshold.Float() <= dUnarmed))) {
            VATSMenu::pVatsMenu->pTiles[30]->Set(4003, 0);
            bWeaponHasSpecial = 0;
            VATSMenu::pVatsMenu->pTiles[31]->Set(4003, 0);
            bWeaponHasSpecial = 0;
        }
        else {
            char bGroundAttack = 0;
            if (pVATSTarget &&
                (pVATSTarget->IsKnockedOut() ||
                 (pVATSTarget->IsActor() && static_cast<Actor*>(pVATSTarget)->Actor::IsUnConscious()))) {
                bGroundAttack = 1;
                const char* pAttackName = sVatsUnarmedAttackGround.String();
                VATSMenu::pVatsMenu->pTiles[30]->Set(4036, pAttackName, true);
                VATSMenu::pVatsMenu->pTiles[30]->Set(4003, 1);
            }
            else {
                const char* pAttackName = sVatsUnarmedAttack1.String();
                VATSMenu::pVatsMenu->pTiles[30]->Set(4036, pAttackName, true);
                VATSMenu::pVatsMenu->pTiles[30]->Set(4003, 1);
            }
            bWeaponHasSpecial = 1;
            fSpecialAttackDamageMult = 1.0f;
            fSpecialAttackAPCost = fUpperCutAPCost.Float();
            TESObjectWEAP* pPerkWeapon = pweapon ? pweapon : pFists;
            BGSEntryPoint::HandleEntryPoint(
                BGSEntryPoint::ACTION_POINT_COST_MODIFIER_ENTRY,
                pPlayer,
                pPerkWeapon,
                &fSpecialAttackAPCost
            );
            if (bGroundAttack ||
                (dUnarmed = static_cast<double>(pPlayer->GetActorValue(ActorValue::UNARMED)),
                 !(fCrossThreshold.Float() <= dUnarmed))) {
                VATSMenu::pVatsMenu->pTiles[31]->Set(4003, 0);
                bWeaponHasSpecial2 = 0;
            }
            else {
                const char* pAttackName = sVatsUnarmedAttack2.String();
                VATSMenu::pVatsMenu->pTiles[31]->Set(4036, pAttackName, true);
                VATSMenu::pVatsMenu->pTiles[31]->Set(4003, 1);
                bWeaponHasSpecial2 = 1;
                fSpecialAttack2DamageMult = 1.0f;
                fSpecialAttack2APCost = fCrossAPCost.Float();
                pPerkWeapon = pweapon ? pweapon : pFists;
                BGSEntryPoint::HandleEntryPoint(
                    BGSEntryPoint::ACTION_POINT_COST_MODIFIER_ENTRY,
                    pPlayer,
                    pPerkWeapon,
                    &fSpecialAttack2APCost
                );
            }
        }
    }
}

int32_t VATSMenu::SortAngle(VATSTarget* apObject1, VATSTarget* apObject2) {
    NiPoint3 cPlayerLoc = pPlayer->GetLocationOnReference();
    float fPlayerHeading = pPlayer->GetHeading(false) * 57.2957763671875;
    float fObject1Heading = HUDMainMenu::HeadingBetweenLocations(
        cPlayerLoc,
        apObject1->pTargetRef->GetLocationOnReference()
    );
    float fObject2Heading = HUDMainMenu::HeadingBetweenLocations(
        cPlayerLoc,
        apObject2->pTargetRef->GetLocationOnReference()
    );
    float fHeadingDelta = fPlayerHeading - fObject1Heading;
    float fAngle = std::fabs(fHeadingDelta);
    double dWrapped;
    if (fAngle > 180.0) {
        dWrapped = 360.0 - fAngle;
    }
    else {
        dWrapped = fAngle;
    }
    float fWrapped = dWrapped;
    float fAngle1 = std::fabs(fWrapped);
    fHeadingDelta = fPlayerHeading - fObject2Heading;
    fAngle = std::fabs(fHeadingDelta);
    if (fAngle > 180.0) {
        dWrapped = 360.0 - fAngle;
    }
    else {
        dWrapped = fAngle;
    }
    fWrapped = dWrapped;
    float fAngle2 = std::fabs(fWrapped);
    if (fAngle2 > static_cast<double>(fAngle1)) {
        return -1;
    }
    return fAngle2 < static_cast<double>(fAngle1);
}

// The headings are rounded to float; a NaN heading sorts as equal.
int32_t VATSMenu::SortDirection(VATSTarget* apObject1, VATSTarget* apObject2) {
    NiPoint3 kPlayerLocation = pPlayer->GetLocationOnReference();
    float fHeading1 = HUDMainMenu::HeadingBetweenLocations(
        kPlayerLocation,
        apObject1->pTargetRef->GetLocationOnReference()
    );
    float fHeading2 = HUDMainMenu::HeadingBetweenLocations(
        kPlayerLocation,
        apObject2->pTargetRef->GetLocationOnReference()
    );
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
    if (!VATSMenu::pVatsMenu) {
        return;
    }
    BSSimpleList<VATSTarget*>* pTargetList = VATSMenu::pVatsMenu->GetVatsTarget(pVATSTarget);
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
            BSSoundHandle soundHand;
            soundHand = BSAudio::QInstance().GetSoundHandleByName(
                "UIVATSSelectTargetPart",
                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
            );
            soundHand.Play(false);
        }
    }
    VATSMenu::pVatsMenu->fProbableEnemyDamage = VATSMenu::pVatsMenu->CalculateProbableEnemyHealth();
    Actor* pActor = nullptr;
    if (pTargetList && pTargetList->m_item && pTargetList->m_item->pTargetRef->IsActor()) {
        pActor = static_cast<Actor*>(pTargetList->m_item->pTargetRef);
    }
    ItemChange* pWeaponItem = pPlayer->pCurrentProcess->GetCurrentWeapon();
    TESObjectWEAP* pWeapon;
    if (pWeaponItem) {
        pWeapon = static_cast<TESObjectWEAP*>(pWeaponItem->pContainerObj);
    }
    else {
        pWeapon = nullptr;
    }
    if (pTargetList &&
        (pTargetList->m_item->eType == 2 ||
         pPlayer->pCurrentProcess->IsCurrentWeaponGrenade() ||
         !pWeapon ||
         pWeapon->IsMeleeWeapon())) {
        if (abForceShow || (pVATS.eMode == VATS::VATS_MENU_MODE && VATSMenu::pLastTarget != pVATSTarget)) {
            if (pVATS.eMode == VATS::VATS_SCAN_MODE) {
                BGSBodyPart::LIMB_ENUM eLimb = static_cast<BGSBodyPart::LIMB_ENUM>(
                    VATSMenu::pVatsMenu->pCurrentLimb->eBodyPart
                );
                TESObjectREFR* pTarget = pTargetList->m_item->pTargetRef;
                Interface::GetVATSEffect()->SetTarget(pTarget, eLimb, true);
            }
            else {
                TESObjectREFR* pTarget = pTargetList->m_item->pTargetRef;
                Interface::GetVATSEffect()->SetTarget(pTarget, BGSBodyPart::LIMB_COUNT, false);
            }
            VATSMenu::pLastTarget = pTargetList->m_item->pTargetRef;
        }
        VATSMenu::uiLastBodyPart = VATSMenu::pVatsMenu->pCurrentLimb->eBodyPart;
        return;
    }
    if (!pActor || !pActor->pCurrentProcess) {
        return;
    }
    if (VATSMenu::pVatsMenu->pCurrentLimb->bFirstTimeShown || pVATS.eMode != VATS::VATS_SCAN_MODE) {
        BGSBodyPart::LIMB_ENUM eLimb = static_cast<BGSBodyPart::LIMB_ENUM>(
            VATSMenu::pVatsMenu->pCurrentLimb->eBodyPart
        );
        TESObjectREFR* pTarget = pVATSTarget;
        Interface::GetVATSEffect()->SetTarget(pTarget, eLimb, false);
    }
    else {
        BGSBodyPart::LIMB_ENUM eLimb = static_cast<BGSBodyPart::LIMB_ENUM>(
            VATSMenu::pVatsMenu->pCurrentLimb->eBodyPart
        );
        TESObjectREFR* pTarget = pVATSTarget;
        Interface::GetVATSEffect()->SetTarget(pTarget, eLimb, true);
    }
    // Targets every other part of the target that maps to the current limb's actor value as well.
    for (BSSimpleList<VATSBodyPart*>* pIter = &VATSMenu::pVatsMenu->GetVatsTarget(pVATSTarget)->m_item->kBodyParts;
         pIter && !pIter->IsEmpty();
         pIter = pIter->m_pkNext) {
        ActorValue::Index ePartLimb = VATSMenu::pVatsMenu->GetActorValueFromBodyPart(
            static_cast<BGSBodyPart::LIMB_ENUM>(pIter->m_item->eBodyPart)
        );
        if (ePartLimb ==
                VATSMenu::pVatsMenu->GetActorValueFromBodyPart(
                    static_cast<BGSBodyPart::LIMB_ENUM>(VATSMenu::pVatsMenu->pCurrentLimb->eBodyPart)
                ) &&
            pIter->m_item != VATSMenu::pVatsMenu->pCurrentLimb) {
            BGSBodyPart::LIMB_ENUM eLimb = static_cast<BGSBodyPart::LIMB_ENUM>(pIter->m_item->eBodyPart);
            Interface::GetVATSEffect()->TargetAdditionalLimb(eLimb);
        }
    }
    VATSMenu::uiLastBodyPart = VATSMenu::pVatsMenu->pCurrentLimb->eBodyPart;
    VATSMenu::pLastTarget = pVATSTarget;
}

uint8_t VATSMenu::GetShotBurstCount(ActionPoints::Action aeAction, bool abAllowLongBurst) {
    float fBurstTime;
    int32_t iNumAmmoPerShot;
    TESObjectWEAP* pWeapon;
    BaseProcess* pProcess = pPlayer->pCurrentProcess;
    ItemChange* pWeaponItem = pProcess->GetCurrentWeapon();
    if (pWeaponItem) {
        pWeapon = static_cast<TESObjectWEAP*>(pWeaponItem->pContainerObj);
    }
    else {
        pWeapon = nullptr;
    }
    if (pWeapon) {
        iNumAmmoPerShot = pWeapon->GetAmmoPerShot();
    }
    else {
        iNumAmmoPerShot = 0;
    }
    uint8_t cShotBurst = 1;
    if (iNumAmmoPerShot > 0) {
        cShotBurst = iNumAmmoPerShot;
    }
    if (aeAction == ActionPoints::ACTION_ATTACK_UNARMED || (pWeapon && !pWeapon->IsAutomaticWeapon())) {
        return cShotBurst;
    }
    if (abAllowLongBurst && pWeapon && pWeapon->IsLongBurst()) {
        fBurstTime = fVATSShotLongBurstTime.Float();
    }
    else {
        fBurstTime = fVATSShotBurstTime.Float();
    }
    double dShots = pWeapon->data.fShotsPerSec * fBurstTime;
    bool bModActive = pWeaponItem->HasModEffectActive(8);
    cShotBurst = static_cast<int32_t>(pWeapon->GetAttackSpeed(bModActive) * dShots);
    if (!cShotBurst) {
        return 1;
    }
    return cShotBurst;
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
        VATSTarget* pTarget;
        for (pTarget = VATSMenu::xTargetList.m_item; pTarget->kBodyParts.m_item; pTarget->kBodyParts.RemoveHead()) {
            ourFree(pTarget->kBodyParts.m_item);
        }
        if (pTarget) {
            delete pTarget;
        }
        VATSMenu::xTargetList.RemoveHead();
    }
}

void VATSMenu::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void VATSMenu::SetLimb(BGSBodyPart::LIMB_ENUM aeLimb) {
    if (VATSMenu::pVatsMenu) {
        for (BSSimpleList<VATSBodyPart*>* pList = &VATSMenu::pVatsMenu->GetVatsTarget(pVATSTarget)->m_item->kBodyParts;
             pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            if (pList->m_item && pList->m_item->eBodyPart == aeLimb) {
                VATSMenu::pVatsMenu->pCurrentLimb = pList->m_item;
                return;
            }
        }
    }
}

void VATSMenu::RemoveTarget(TESObjectREFR* apRef) {
    if (!VATSMenu::xTargetList.IsEmpty()) {
        for (BSSimpleList<VATSTarget*>* pList = &VATSMenu::xTargetList; pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            if (pList->m_item->pTargetRef == apRef) {
                if (pVATSTarget == apRef && VATSMenu::pVatsMenu) {
                    VATSMenu::pVatsMenu->DoClick(2, nullptr);
                }

                VATSMenu::xTargetList.Remove(pList->m_item);
                if (VATSMenu::xTargetList.IsEmpty() && VATSMenu::pVatsMenu) {
                    VATSMenu::Close();
                }
            }
        }
    }
}

bool VATSMenu::FilterDestructibleObjects(TESObjectREFR* apRef, void* aiFilterData) {
    const NiPoint3& rRefPos = apRef->GetLocationOnReference();
    const NiPoint3& rPlayerPos = pPlayer->GetLocationOnReference();
    float fDist = (rPlayerPos - rRefPos).Length();
    if (pPlayer->GetCurrentWeapon()) {
        TESObjectWEAP* pWeapon = pPlayer->GetCurrentWeapon();
        if (pWeapon->IsMeleeWeapon()) {
            if (fVATSMeleeMaxDistance.Float() < static_cast<double>(fDist)) {
                return false;
            }
        }
    }
    if (apRef->GetDestructible() && apRef->IsProjectile()) {
        Projectile* pProj = static_cast<Projectile*>(apRef);
        if (pProj->pWeaponSource &&
            (pProj->pWeaponSource->GetWeaponType() == WEAPON_ONE_HAND_MINE ||
             pProj->pWeaponSource->GetWeaponType() == WEAPON_ONE_HAND_LUNCHBOX_MINE)) {
            return false;
        }
        if (pProj->GetShooter() != pPlayer) {
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
        else if (desiredAttack == DESIRED_ATTACK_SPECIAL_2) {
            button = Interface::eButtonX;
        }
        if (held) {
            return InterfaceManager::CheckMenuButton(button, true) == 0;
        }

        return InterfaceManager::CheckMenuButton(button, true) == 2;
    }

    Controls* pControls = appMain->GetControls();
    // The toggle flags Controls' button tests take: 1 for a press, 0 while held.
    int32_t iToggleFlags = !held;
    if (desiredAttack == DESIRED_ATTACK_NORMAL) {
        return pControls->GetMouseButton(0, iToggleFlags) && aiMouseKey;
    }

    if (desiredAttack == DESIRED_ATTACK_SPECIAL) {
        return pControls->GetKeyboardButton(19, iToggleFlags) || bWeaponSpecialSelected;
    }

    return desiredAttack == DESIRED_ATTACK_SPECIAL_2 &&
        (pControls->GetKeyboardButton(33, iToggleFlags) || g_bIsVATSSpecialAttack2);
}

BSSimpleList<VATS_COMMAND*>* VATS::GetCommandList() {
    return &commandList;
}
