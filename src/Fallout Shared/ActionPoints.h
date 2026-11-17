#pragma once

#include "BSMain/Setting.h"

class TESObjectWEAP;

enum WEAPON_TYPE : int32_t;

double get_fActionPointsRestoreRate();

// The action point costs of the player's actions. The class has only static members; it has no instance layout.
class ActionPoints {
public:
	// ActionPoints::IsAttackAction counts 0-8, 0x10, 0x11 and 0x13-0x15 as attacks, consistent with these names.
	enum Action : int32_t {
		ACTION_ATTACK_UNARMED = 0x0,
		ACTION_ATTACK_ONE_HAND_MELEE = 0x1,
		ACTION_ATTACK_TWO_HAND_MELEE = 0x2,
		ACTION_ATTACK_PISTOL = 0x3,
		ACTION_ATTACK_RIFLE = 0x4,
		ACTION_ATTACK_HANDLE = 0x5,
		ACTION_ATTACK_LAUNCHER = 0x6,
		ACTION_ATTACK_GRENADE = 0x7,
		ACTION_ATTACK_MINE = 0x8,
		ACTION_RELOAD = 0x9,
		ACTION_CROUCH = 0xA,
		ACTION_STAND = 0xB,
		ACTION_SWITCH_WEAPON = 0xC,
		ACTION_TOGGLE_WEAPON_DRAWN = 0xD,
		ACTION_HEAL = 0xE,
		ACTION_PLAYER_VATS_DEATH = 0xF,
		ACTION_PLAYER_SPECIAL_WEAPON_ATTACK = 0x10,
		ACTION_PLAYER_SPECIAL_UNARMED_ATTACK = 0x11,
		ACTION_TARGET_KILL_SHOT = 0x12,
		ACTION_ATTACK_THROWN = 0x13,
		ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_2 = 0x14,
		ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_3 = 0x15,
		ACTION_COUNT = 0x16,
	};

	// The cost setting and the name setting of each action.
	using SettingPointerArray = Setting* [ACTION_COUNT];

	// The action point cost of the action auiAction.
	static float GetBaseActionPoints(uint32_t auiAction);
	// The cost of the action auiAction, scaled by fActionPointsRunAndGunMult outside VATS.
	static float GetRequiredAPForAction(uint32_t auiAction, bool abVATS);
	// The attack action of weapon type aeWeaponType; ACTION_COUNT for a weapon type without one.
	static ActionPoints::Action GetAttackAction(WEAPON_TYPE aeWeaponType);
	static bool IsAttackAction(ActionPoints::Action aeAction);
	static bool IsMeleeAttackAction(ActionPoints::Action aeAction);
	static float GetWeaponAttackActionPoints(TESObjectWEAP* apWeapon, bool abVATS, bool abSpecialAttack);
	static const char* GetActionPointsString(ActionPoints::Action aeAction);

	static SettingPointerArray pCosts;
	static SettingPointerArray pNames;
};

extern SettingT<GameSettingCollection> sActionPointsReload;
extern SettingT<GameSettingCollection> fActionPointsAttackThrown;
extern SettingT<GameSettingCollection> fActionPointsAttackUnarmed;
extern SettingT<GameSettingCollection> sActionPointsCrouch;
extern SettingT<GameSettingCollection> fActionPointsAttackPistol;
extern SettingT<GameSettingCollection> fActionPointsFOVMult;
extern SettingT<GameSettingCollection> fActionPointsCrouch;
extern SettingT<GameSettingCollection> fActionPointsSwitchWeapon;
extern SettingT<GameSettingCollection> fActionPointsAttackLauncher;
extern SettingT<GameSettingCollection> fActionPointsStand;
extern SettingT<GameSettingCollection> fActionPointsSustainedFireMult;
extern SettingT<GameSettingCollection> fActionPointsReload;
extern SettingT<GameSettingCollection> sActionPointsThrow;
extern SettingT<GameSettingCollection> fActionPointsAttackHandle;
extern SettingT<GameSettingCollection> fActionPointsRunAndGunMult;
extern SettingT<GameSettingCollection> fActionPointsToggleWeaponDrawn;
extern SettingT<GameSettingCollection> fActionPointsPowerAttackMult;
extern SettingT<GameSettingCollection> sActionPointsToggleWeaponDrawn;
extern SettingT<GameSettingCollection> sActionPointsHeal;
extern SettingT<GameSettingCollection> sActionPointsDrop;
extern SettingT<GameSettingCollection> fActionPointsAttackTwoHandMelee;
extern SettingT<GameSettingCollection> fActionPointsVATSMeleeMult;
extern SettingT<GameSettingCollection> fActionPointsRestoreRate;
extern SettingT<GameSettingCollection> sActionPointsPunch;
extern SettingT<GameSettingCollection> sActionPointsStand;
extern SettingT<GameSettingCollection> fActionPointsAttackGrenade;
extern SettingT<GameSettingCollection> fActionPointsAttackMine;
extern SettingT<GameSettingCollection> fActionPointsHeal;
extern SettingT<GameSettingCollection> sActionPointsShoot;
extern SettingT<GameSettingCollection> fActionPointsAttackOneHandMelee;
extern SettingT<GameSettingCollection> fActionPointsFOVBase;
extern SettingT<GameSettingCollection> sActionPointsSwitchWeapon;
extern SettingT<GameSettingCollection> fActionPointsAttackRifle;
extern SettingT<GameSettingCollection> sActionPointsAttack;
extern SettingT<GameSettingCollection> fActionPointsAimAdjustment;
