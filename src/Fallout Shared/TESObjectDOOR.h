#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESMagicCasterForm.h"
#include "Fallout Shared/TESMagicTargetForm.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class Actor;
class DoorLock;
class MobileObject;
class TESObjectREFR;
class TESSound;

// A door base form (DOOR).
class TESObjectDOOR : public TESBoundAnimObject,
					  public TESFullName,
					  public TESModelTextureSwap,
					  public TESScriptableForm,
					  public TESMagicCasterForm,
					  public TESMagicTargetForm,
					  public BGSDestructibleObjectForm,
					  public BGSOpenCloseForm {
public:
	TESObjectDOOR();
	~TESObjectDOOR() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	bool Activate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abIdFlag,
		TESBoundObject* apObjecttoGet, int32_t aicount) override;
	NiAVObject* Clone3D(TESObjectREFR* apRequester) override;

	using TESBoundObject::Clone3D;
	using TESForm::Save;

	// Whether the door slides rather than swings: flag 0x10 of the byte at +0x84.
	bool IsSlidingDoor();
	// The looping sound (pLoopSound); the exe's shared +0x80 getter (thiscall, ret 0,
	// 0x4FB070).
	TESSound* GetLoopSound();
	bool IsRandomDoor();
	void ClearRandomTeleportList();
	bool IsHidden();
	bool IsMinimalUse();
	void SetMinimalUse(bool abVal);
	bool IsAutomatic();
	void HandlePlayerTeleport(TESObjectREFR* apActivatedDoor);
	bool RandomlyTeleportsTo(TESForm* apSpace);
	TESObjectREFR* FindRandomTeleportTarget(TESObjectREFR* apStartDoor, TESObjectREFR* apActionRef);

	// Whether apActor may pass apLock without unlocking it: a follower of the player through a teleport door, a
	// trespasser, or whoever the lock lets through.
	static bool CanActorIgnoreLock(DoorLock* apLock, Actor* apActor, TESObjectREFR* apDoorRef, bool abActivate,
		bool abMovement);
	// The overload for a door reference: builds a DoorLock from apDoorRef, asks the overload
	// above with it, then destroys it.
	static bool CanActorIgnoreLock(TESObjectREFR* apDoorRef, Actor* apActor, bool abActivate, bool abMovement);
	static void AutoDoorCallback(NiAVObject* apDoor3D);
	static void DoorTeleportPlayerArrivalCallback(uintptr_t aiArrivalData);
	static void PlayerPullFollower();
	static void LinkRandomTeleportDoors(TESObjectREFR* apDoor1, TESObjectREFR* apDoor2);

	TESSound* pOpenSound;
	TESSound* pCloseSound;
	TESSound* pLoopSound;
	uint8_t cFlags;
	uint8_t padding85[3];
	BSSimpleList<TESForm*> RandomTeleports;
};

inline MobileObject* pstaticdoor;
inline bool bStaticWasInInterior;
inline constexpr fnvd::Vtable TESObjectDOOR_vftable{};
inline constexpr fnvd::Vtable TESObjectDOOR_vftable_BGSDestructibleObjectForm{};
inline constexpr fnvd::Vtable TESObjectDOOR_vftable_BGSOpenCloseForm{};
inline constexpr fnvd::Vtable TESObjectDOOR_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESObjectDOOR_vftable_TESModelTextureSwap{};
inline constexpr fnvd::Vtable TESObjectDOOR_vftable_TESScriptableForm{};

// GameSettingCollection: 0xf4989d calls 0x40c150.
extern SettingT<GameSettingCollection> gs_sRandomDoorTeleportFailureMessage;

// GameSettingCollection: 0xf49842 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fRandomDoorDistance;

// GameSettingCollection: 0xf49872 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fAutoDoorActivateDistance;

// INIPrefSettingCollection: 0xf8c83a calls 0x709f00.
extern SettingT<INIPrefSettingCollection> gs_bSaveOnTravel_GamePlay;
