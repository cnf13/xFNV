#include "Fallout Shared/ActionPoints.h"

#include "BSCore/BSSimpleArray.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Settings/XGameSetting.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"

#include <cstdint>

ActionPoints::SettingPointerArray ActionPoints::pCosts{
    &fActionPointsAttackUnarmed,
    &fActionPointsAttackOneHandMelee,
    &fActionPointsAttackTwoHandMelee,
    &fActionPointsAttackPistol,
    &fActionPointsAttackRifle,
    &fActionPointsAttackHandle,
    &fActionPointsAttackLauncher,
    &fActionPointsAttackGrenade,
    &fActionPointsAttackMine,
    &fActionPointsReload,
    &fActionPointsCrouch,
    &fActionPointsStand,
    &fActionPointsSwitchWeapon,
    &fActionPointsToggleWeaponDrawn,
    &fActionPointsHeal,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    &fActionPointsAttackThrown
};
ActionPoints::SettingPointerArray ActionPoints::pNames{
    &sActionPointsPunch,
    &sActionPointsAttack,
    &sActionPointsAttack,
    &sActionPointsShoot,
    &sActionPointsShoot,
    &sActionPointsShoot,
    &sActionPointsShoot,
    &sActionPointsThrow,
    &sActionPointsDrop,
    &sActionPointsReload,
    &sActionPointsCrouch,
    &sActionPointsStand,
    &sActionPointsSwitchWeapon,
    &sActionPointsToggleWeaponDrawn,
    &sActionPointsHeal,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    &sActionPointsThrow
};

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<GameSettingCollection> fActionPointsAttackUnarmed("fActionPointsAttackUnarmed", 1.0f);
SettingT<GameSettingCollection> fActionPointsAttackOneHandMelee("fActionPointsAttackOneHandMelee", 1.0f);
SettingT<GameSettingCollection> fActionPointsAttackTwoHandMelee("fActionPointsAttackTwoHandMelee", 1.0f);
SettingT<GameSettingCollection> fActionPointsAttackPistol("fActionPointsAttackPistol", 1.0f);
SettingT<GameSettingCollection> fActionPointsAttackRifle("fActionPointsAttackRifle", 1.0f);
SettingT<GameSettingCollection> fActionPointsAttackHandle("fActionPointsAttackHandle", 1.0f);
SettingT<GameSettingCollection> fActionPointsAttackLauncher("fActionPointsAttackLauncher", 1.0f);
SettingT<GameSettingCollection> fActionPointsAttackGrenade("fActionPointsAttackGrenade", 1.0f);
SettingT<GameSettingCollection> fActionPointsAttackMine("fActionPointsAttackMine", 1.0f);
SettingT<GameSettingCollection> fActionPointsReload("fActionPointsReload", 1.0f);
SettingT<GameSettingCollection> fActionPointsCrouch("fActionPointsCrouch", 1.0f);
SettingT<GameSettingCollection> fActionPointsStand("fActionPointsStand", 1.0f);
SettingT<GameSettingCollection> fActionPointsSwitchWeapon("fActionPointsSwitchWeapon", 1.0f);
SettingT<GameSettingCollection> fActionPointsToggleWeaponDrawn("fActionPointsToggleWeaponDrawn", 1.0f);
SettingT<GameSettingCollection> fActionPointsHeal("fActionPointsHeal", 1.0f);
SettingT<GameSettingCollection> fActionPointsAttackThrown("fActionPointsAttackThrown", 1.0f);
SettingT<GameSettingCollection> sActionPointsPunch("sActionPointsPunch", "Punch");
SettingT<GameSettingCollection> sActionPointsShoot("sActionPointsShoot", "Shot");
SettingT<GameSettingCollection> sActionPointsThrow("sActionPointsThrow", "Throw");
SettingT<GameSettingCollection> sActionPointsDrop("sActionPointsDrop", "Drop");
SettingT<GameSettingCollection> sActionPointsAttack("sActionPointsAttack", "Attack");
SettingT<GameSettingCollection> sActionPointsReload("sActionPointsReload", "Reload");
SettingT<GameSettingCollection> sActionPointsCrouch("sActionPointsCrouch", "Crouch");
SettingT<GameSettingCollection> sActionPointsStand("sActionPointsStand", "Stand");
SettingT<GameSettingCollection> sActionPointsSwitchWeapon("sActionPointsSwitchWeapon", "Switch Weapon");
SettingT<GameSettingCollection> sActionPointsToggleWeaponDrawn("sActionPointsToggleWeaponDrawn", "Toggle Weapon Drawn");
SettingT<GameSettingCollection> sActionPointsHeal("sActionPointsHeal", "Heal");
SettingT<GameSettingCollection> fActionPointsFOVMult("fActionPointsFOVMult", 1.0f);
SettingT<GameSettingCollection> fActionPointsFOVBase("fActionPointsFOVBase", 0.0f);
SettingT<GameSettingCollection> fActionPointsAimAdjustment("fActionPointsAimAdjustment", 2.0f);
SettingT<GameSettingCollection> fActionPointsPowerAttackMult("fActionPointsPowerAttackMult", 1.5f);
SettingT<GameSettingCollection> fActionPointsRunAndGunMult("fActionPointsRunAndGunMult", 0.5f);
SettingT<GameSettingCollection> fActionPointsSustainedFireMult("fActionPointsSustainedFireMult", 2.0f);
SettingT<GameSettingCollection> fActionPointsVATSMeleeMult("fActionPointsVATSMeleeMult", 5.0f);
SettingT<GameSettingCollection> fActionPointsRestoreRate("fActionPointsRestoreRate", 1.0f);

double get_fActionPointsRestoreRate() {
    return fActionPointsRestoreRate.Float();
}

float ActionPoints::GetBaseActionPoints(uint32_t auiAction) {
    if (auiAction <= 21) {
        return ActionPoints::pCosts[auiAction]->Float();
    }
    return 0.0f;
}

// The cost and the scaled cost are rounded to float before they are returned.
float ActionPoints::GetRequiredAPForAction(uint32_t auiAction, bool abVATS) {
    float fCost = GetBaseActionPoints(auiAction);
    if (!abVATS) {
        float fScaled = fCost * fActionPointsRunAndGunMult.Float();
        return fScaled;
    }
    return fCost;
}

// The lunchbox mine has no attack action of its own.
ActionPoints::Action ActionPoints::GetAttackAction(WEAPON_TYPE aeWeaponType) {
    ActionPoints::Action eAction = ACTION_COUNT;
    switch (aeWeaponType) {
        case WEAPON_HAND_TO_HAND: eAction = ACTION_ATTACK_UNARMED; break;
        case WEAPON_ONE_HAND_MELEE: eAction = ACTION_ATTACK_ONE_HAND_MELEE; break;
        case WEAPON_TWO_HAND_MELEE: eAction = ACTION_ATTACK_TWO_HAND_MELEE; break;
        case WEAPON_ONE_HAND_PISTOL:
        case WEAPON_ONE_HAND_PISTOL_ENERGY: eAction = ACTION_ATTACK_PISTOL; break;
        case WEAPON_TWO_HAND_RIFLE:
        case WEAPON_TWO_HAND_AUTOMATIC:
        case WEAPON_TWO_HAND_RIFLE_ENERGY: eAction = ACTION_ATTACK_RIFLE; break;
        case WEAPON_TWO_HAND_HANDLE: eAction = ACTION_ATTACK_HANDLE; break;
        case WEAPON_TWO_HAND_LAUNCHER: eAction = ACTION_ATTACK_LAUNCHER; break;
        case WEAPON_ONE_HAND_GRENADE: eAction = ACTION_ATTACK_GRENADE; break;
        case WEAPON_ONE_HAND_MINE: eAction = ACTION_ATTACK_MINE; break;
        case WEAPON_ONE_HAND_THROWN: eAction = ACTION_ATTACK_THROWN; break;
        default: break;
    }
    return eAction;
}

bool ActionPoints::IsAttackAction(ActionPoints::Action aeAction) {
    switch (aeAction) {
        case ACTION_ATTACK_UNARMED:
        case ACTION_ATTACK_ONE_HAND_MELEE:
        case ACTION_ATTACK_TWO_HAND_MELEE:
        case ACTION_ATTACK_PISTOL:
        case ACTION_ATTACK_RIFLE:
        case ACTION_ATTACK_HANDLE:
        case ACTION_ATTACK_LAUNCHER:
        case ACTION_ATTACK_GRENADE:
        case ACTION_ATTACK_MINE:
        case ACTION_PLAYER_SPECIAL_WEAPON_ATTACK:
        case ACTION_PLAYER_SPECIAL_UNARMED_ATTACK:
        case ACTION_ATTACK_THROWN:
        case ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_2:
        case ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_3: {
            return true;
        }
        default: {
            return false;
        }
    }
}

bool ActionPoints::IsMeleeAttackAction(ActionPoints::Action aeAction) {
    switch (aeAction) {
        case ACTION_ATTACK_UNARMED:
        case ACTION_ATTACK_ONE_HAND_MELEE:
        case ACTION_ATTACK_TWO_HAND_MELEE:
        case ACTION_PLAYER_SPECIAL_WEAPON_ATTACK:
        case ACTION_PLAYER_SPECIAL_UNARMED_ATTACK:
        case ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_2:
        case ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_3: {
            return true;
        }
        default: {
            return false;
        }
    }
}

float ActionPoints::GetWeaponAttackActionPoints(TESObjectWEAP* apWeapon, bool abVATS, bool abSpecialAttack) {
    int32_t iAttackAction;
    if (apWeapon) {
        int32_t iWeaponType = apWeapon->GetWeaponType();
        iAttackAction = ActionPoints::GetAttackAction(static_cast<WEAPON_TYPE>(iWeaponType));
    }
    else {
        iAttackAction = ActionPoints::GetAttackAction(WEAPON_HAND_TO_HAND);
    }
    uint32_t uiAction = static_cast<uint32_t>(iAttackAction);
    float fActionPoints = 0.0f;
    if (apWeapon && apWeapon->IsActionPointOverride()) {
        fActionPoints = apWeapon->data.fActionPoints;
    }
    else {
        fActionPoints = ActionPoints::GetBaseActionPoints(uiAction);
    }
    if (apWeapon) {
        BGSEntryPoint::HandleEntryPoint(
            BGSEntryPoint::ACTION_POINT_COST_MODIFIER_ENTRY,
            pPlayer,
            apWeapon,
            &fActionPoints
        );
    }
    else {
        BGSEntryPoint::HandleEntryPoint(
            BGSEntryPoint::ACTION_POINT_COST_MODIFIER_ENTRY,
            pPlayer,
            pFists,
            &fActionPoints
        );
    }
    return fActionPoints;
}

const char* ActionPoints::GetActionPointsString(ActionPoints::Action aeAction) {
    return ActionPoints::pNames[aeAction]->String();
}
