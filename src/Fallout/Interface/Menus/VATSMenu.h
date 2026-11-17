#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/Combat/VATSTarget.h"
#include "Fallout/Interface/ListBox.h"

#include <stdint.h>

class TESObjectREFR;
class VATSBodyPart;

class VATSMenu : public Menu {
public:
	VATSMenu();
	~VATSMenu() override;

	void AttachTileByID(int32_t, Tile*) override;
	void DoClick(int32_t, Tile*) override;
	void DoEnter(int32_t, Tile*) override;
	void DoLeave(int32_t, Tile*) override;
	void DoStillDown(int32_t, Tile*) override;
	void DoWheelMove(int32_t, Tile*) override;
	void DoIdle() override;
	bool DoKey(int32_t) override;
	int32_t GetClass() override;

	// Updates the queued action list for apCommand.
	void UpdateQueuedActionList(VATS_COMMAND* apCommand, bool abForcedReload, bool bSpecialAttack);
	bool IsWellFormedMenu(int32_t& ai);
	// The node of xTargetList whose target is apTarget; null for a null apTarget or when none is.
	BSSimpleList<VATSTarget*>* GetVatsTarget(TESObjectREFR* apTarget);
	// Refreshes the ammo counts and meters from the player's weapon and the queued commands; true when a queued
	// attack empties the clip and forces a reload.
	bool UpdateAmmo();

protected:
	// The actor value of the target's body part aeBodyPart: ENDURANCE_CONDITION when the target is no actor or has
	// no such part, -1 for a missing weapon part.
	ActorValue::Index GetActorValueFromBodyPart(BGSBodyPart::LIMB_ENUM aeBodyPart);
	// Recomputes the chance to hit of the target's body parts.
	void UpdateChanceToHit();
	// Lays out the body part meters.
	void PrepareMeters();
	// Picks the limb the stick or cursor points at.
	void SetTargetLimb();
	// The node of the target's body part list whose part is pCurrentLimb, or null.
	BSSimpleList<VATSBodyPart*>* GetTargetPart();
	// Computes the hit chances of the target's parts; false while they are not ready.
	bool PrecalcTargetHitChances();
	// The target's health fraction after the queued attacks.
	float CalculateProbableEnemyHealth();
	// Moves to the target under the cursor; true when it changed.
	bool PickNewTarget();
	// How many queued attacks in a row aim at the current limb.
	int32_t GetConsecutiveShots();
	// Shows the chance to hit on the meters.
	void SetMeterChance();
	// Builds the body part meters of the target node apTarget.
	void BuildMetersForTarget(BSSimpleList<VATSTarget*>* apTarget);

public:
	bool LimbAppeared();
	void GetScreenPlaneProjectionMatrix(NiMatrix3* apmatrix);
	bool LimbOnScreen(VATSBodyPart* apLimb);
	// apList is a body part list node; its first element is the VATSBodyPart*.
	double UpdateHitChanceForBodyPartData(BSSimpleList<VATSBodyPart*>* apList);
	BSSimpleList<VATSBodyPart*>* FindTargetPart(ActorValue::Index aeLimb);
	void SelectNearestTarget(bool abLeft);

	static void Close();
	static bool GetIsARescan();
	static int32_t GetClassStatic();
	static void SetHeadingReached(bool abHeadingReached);
	static VATS_COMMAND* GetCurrentQueuedAction();
	// Opens the VATS menu: drops the targets VATS may not offer, spawns the Mysterious Stranger or Miss Fortune when
	// their perks fire, picks the first target and builds the meters, compass and enemy health tiles; false when the
	// menu is already open or cannot be built.
	static bool Create();
	// Fills xTargetList with the references VATS can target.
	static bool BuildTargetList();
	// Shows the special attack buttons for pweapon.
	static void SetupSpecialAttackButtons(TESObjectWEAP* pweapon);

protected:
	// The order of two targets by how far the player's heading is from each (the nearer first: -1 when apObject1 is
	// nearer, 1 when apObject2 is, else 0).
	static int32_t SortAngle(VATSTarget* apObject1, VATSTarget* apObject2);
	// The order of two targets by the player's heading to them: -1 when apObject2 is further clockwise, 1 when
	// apObject1 is, else 0.
	static int32_t SortDirection(VATSTarget* apObject1, VATSTarget* apObject2);
	// Renders the template axName under apParent; when that fails, warns, closes every menu and the VATS menu.
	static Tile* VatsMenuRenderTemplate(Tile* apParent, const char* axName);
	// The tile apName under apTarget; when there is none, warns, closes every menu and the VATS menu.
	static Tile* VatsMenuGetTileByName(Tile* apTarget, const char* apName);
	// Shows the target limb's highlight, always when abForceShow.
	static void ShowTargetLimb(bool abForceShow);
	// The number of shots action aeAction fires, the long burst when abAllowLongBurst.
	static uint8_t GetShotBurstCount(ActionPoints::Action aeAction, bool abAllowLongBurst);

public:
	// Whether the stick has been held left long enough to repeat.
	static bool SelectLeftHeld();
	// Whether the stick has been held right long enough to repeat.
	static bool SelectRightHeld();
	static void ClearTargetList();
	static void operator delete(void* apMemory);
	static void SetLimb(BGSBodyPart::LIMB_ENUM aeLimb);
	static void RemoveTarget(TESObjectREFR* apRef);
	static bool FilterDestructibleObjects(TESObjectREFR* apRef, void* aiFilterData);

	Tile* pTiles[32];
	Tile* pActive;
	bool bDebounceMenuButton;
	uint8_t paddingAD[3];
	ListBox<uint32_t> lbActions;
	float fActionPoints;
	float fActionPointsBase;
	float fClipAmmo;
	float fTotalAmmo;
	float fQueuedClipAmmo;
	float fQueuedTotalAmmo;
	bool bHeadingReached;
	bool bMetersShown;
	bool bTargetsLit;
	bool bClosing;
	float fProbableEnemyDamage;
	VATSBodyPart* pCurrentLimb;
	bool bConcentratedFirePerk;
	uint8_t padding105[3];
	VATSBodyPart* pLastLimb;
	uint8_t bStickReset;
	uint8_t bTargetStickReset;
	uint8_t bMouseOverButton;
	bool bSelectLeftHeld;
	bool bSelectRightHeld;
	uint8_t padding111[3];
	uint32_t uiSelectTimer;
	uint8_t bDataInTemporaryVATSCommand;
	uint8_t padding119[3];
	VATS_COMMAND kCurrentCommand;

	static inline VATSMenu* pVatsMenu;
	static inline bool bIsARescan;
	// The list of targets VATS offers.
	static BSSimpleList<VATSTarget*> xTargetList;
	// The last body part shown, as VATSBodyPart::eBodyPart (-1 for none).
	static inline uint32_t uiLastBodyPart = 0xFFFFFFFF;
	// The target ShowTargetLimb, its one user, showed last: it stores the target's reference there and compares it with
	// the next one.
	static inline TESObjectREFR* pLastTarget;
	static inline uint32_t uiActionIndex;
	static inline int32_t iMaxY;
	static inline int32_t iMaxX;
	static inline int32_t iMinY;
	static inline int32_t iMinX;
	static inline int32_t iScreenCenterY;
	static inline int32_t iScreenCenterX;
	static BSSimpleList<TESObjectREFR*> Destructibles;
	static inline char bWeaponHasSpecial;
	static inline char bWeaponHasSpecial2;
};

// The attack VATSMenu::DoIdle asks for: the normal attack or one of the weapon's two special attacks.
enum DesiredAttackType : int32_t {
	DESIRED_ATTACK_NONE = 0,
	DESIRED_ATTACK_NORMAL = 1,
	DESIRED_ATTACK_SPECIAL = 2,
	DESIRED_ATTACK_SPECIAL_2 = 3,
};

// Whether the control of attack desiredAttack is pressed, or still held when held. aiMouseKey is the VATS key's mouse
// binding, which the mouse attack also requires.
bool IsDesiredAttackControlPressed(DesiredAttackType desiredAttack, bool held, int32_t aiMouseKey);

// Whether the target limb's highlight is shown.
inline bool bLimbLit = true;

// The AP cost of the weapon's special attacks.
inline float fSpecialAttackAPCost;
inline float fSpecialAttack2APCost;

// Whether the player has the Paralyzing Palm perk.
inline bool bParalyzingPalm;

inline bool bWeaponSpecialSelected;
inline bool g_bIsVATSSpecialAttack2;
inline bool bDelayVATSMenu;

// The limb VATS targets, as its condition actor value.
inline ActorValue::Index eVATSTargetLimb{ActorValue::VALUE_NONE};

// The Mysterious Stranger and Miss Fortune base objects VATSMenu::Create looks up once.
inline TESBoundObject* pStrangerObject;
inline TESBoundObject* pMissFortuneObject;

// Flags VATSMenu::Create clears.
inline bool bUnk11DB0B1;
inline bool bUnk11DB0CC;

extern SettingT<INISettingCollection> bVatsAlwaysHit;
inline float fFarthestTarget = 40960.0f;
inline float fSpecialAttackDamageMult;
inline float fSpecialAttack2DamageMult;
inline char bcancelVATS;
inline int32_t g_vatsLagTimer;
inline int32_t PrepareMeters_guards;

// PrecalcTargetHitChances's own static, named apart from VATSMenu::pLastTarget, which hides a global of that name
// inside the class.
inline VATSTarget* VATSMenu_PrecalcTargetHitChances_pLastTarget;
