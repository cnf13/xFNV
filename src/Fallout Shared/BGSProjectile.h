#pragma once

#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSPreloadable.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"
#include "Reconstruction.hpp"

class Actor;
class BGSExplosion;
class GrenadeProjectile;
enum SOUND_LEVEL : int32_t;
class TESObjectLIGH;
class TESObjectWEAP;
class TESSound;

// The projectile record's DATA.
struct BGSProjectileData {
	BGSProjectileData();

	// Sets the defaults: the flags, a speed of 1000, a range of 10000, a fade of 0.5 and a bounce of 1.
	void Initialize();
	// Swaps the flags and the floats read from a big-endian file; the form pointers are
	// left for InitItem.
	void Endian();

	uint32_t iFlags;
	float fGravity;
	float fSpeed;
	float fRange;
	TESObjectLIGH* pLight;
	TESObjectLIGH* pMuzzleFlashLight;
	float fTracerChance;
	float fExplosionProximity;
	float fExplosionTimer;
	BGSExplosion* pExplosionType;
	TESSound* pActiveSoundLoop;
	float fMuzzleFlashDuration;
	float fFadeOutTime;
	float fForce;
	TESSound* pCountdownSound;
	TESSound* pDeactivateSound;
	TESObjectWEAP* pDefaultWeaponSource;
	float fRotationX;
	float fRotationY;
	float fRotationZ;
	float fBounceMultiplier;
};

// A projectile form (PROJ).
class BGSProjectile : public TESBoundObject,
					  public TESFullName,
					  public TESModel,
					  public BGSPreloadable,
					  public BGSDestructibleObjectForm {
public:
	// The bits of Data.iFlags (PDB names and values); MOTIONTYPES masks the motion type bits.
	enum BGSProjectileFlags : int32_t {
		HITSCAN = 0x1,
		EXPLOSION = 0x2,
		EXPLOSION_ALT_TRIGGER = 0x4,
		MUZZLE_FlASH = 0x8,
		BOUNCE_ON_IMPACT = 0x10,
		CAN_TURN_OFF = 0x20,
		CAN_PICKUP = 0x40,
		SUPERSONIC = 0x80,
		PINS_LIMBS = 0x100,
		PASS_SM_TRANSPARENT = 0x200,
		EXPLOSION_DETONATES = 0x400,
		HAS_ROTATION = 0x800,
		MOTION_MISSILE = 0x10000,
		MOTION_GRENADE = 0x20000,
		MOTION_BEAM = 0x40000,
		MOTION_FLAMETHROWER = 0x80000,
		MOTION_CONTINUOUS_BEAM = 0x100000,
		MOTIONTYPES = 0x1F0000,
	};

	BGSProjectile();
	~BGSProjectile() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	bool Activate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abIdFlag,
		TESBoundObject* apObjecttoGet, int32_t aiCount) override;
	NiAVObject* Clone3D(TESObjectREFR* pRequester, bool deepCopy) override;
	void UnClone3D(TESObjectREFR* apRequester) override;
	void QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) override;

	using TESBoundObject::Clone3D;
	using TESForm::Save;

	// The explosion the projectile sets off. Same function as InterfaceManager::GetCursorRoot (0x4FD3C0),
	// registered there.
	BGSExplosion* GetExplosion();
	BGSProjectileData* GetData();
	bool GetHasExplosion();
	bool IsFlagSet(BGSProjectileFlags aeFlag);
	// The motion type bits of the flags.
	BGSProjectileFlags GetMotionType();
	int32_t GetPreCloneCount(TESObjectWEAP* apSourceWeapon);
	bool GetHasExplosionDetonates();
	bool GetHasExplosionAltTrigger();
	// The explosion's alternate-trigger proximity (thiscall, ST0 float, 0x45CD80; a shared
	// one-field getter).
	float GetExplosionProximity();
	// The weapon the projectile counts as without a firing weapon (data's
	// pDefaultWeaponSource). Same function as TESObjectCELL::GetCriticalQueuedRefCount (0x4FD400), registered there.
	TESObjectWEAP* GetDefaultWeaponSource();
	bool GetCanTurnOff() { return IsFlagSet(CAN_TURN_OFF); }
	bool GetCanPickup() { return IsFlagSet(CAN_PICKUP); }
	float GetAltTimer() const;
	// Out of line: the exe calls it (0x69EF80).
	float GetSpeed();
	// Out of line: the exe calls it (0x6AB360).
	TESModel& GetMuzzleFlashModel();
	float GetFadeDuration() const;
	// Data.fForce.
	float GetForce() { return Data.fForce; }
	// Out of line: the exe calls it (0x9A7F80).
	bool GetHitScan();
	bool GetPinsLimbs() { return IsFlagSet(PINS_LIMBS); }
	bool GetRotates();
	// Out of line: the exe calls it (0x9BAF30).
	bool GetHasMuzzleFlash();
	bool GetSupersonic() { return IsFlagSet(SUPERSONIC); }
	// The projectile's maximum range (thiscall, x87 double, 0x9A7C40).
	float GetRange();
	float GetGravity();
	ENUM_FORM_ID GetProjectileRefFormType();
	bool GetPassThroughSmTransp();
	// Rolls the tracer chance: true when a random 0-99 is below it.
	bool DecideIsTracer();
	// Name not in the PDB (the field at +0x78); folded accessor, IDA: BGSExplosion::GetDamage.
	float GetTracerChance();
	// The bounce multiplier in Data (+0xB0; thiscall, ST0 float).
	float GetBounceMultiplier() { return Data.fBounceMultiplier; }

	static void operator delete(void* apMemory) { ourFree(apMemory); }
	// Inferred type: apProjectile: BGSProjectile* from 0x75d795 (result of TESObjectWEAP::GetProjectile)
	// apTarget is stored in the grenade's Projectile::pDesiredTarget (+0x100, through the one-field setter 0x420BA0);
	// ContainerMenu::SwitchContainer (0x75D6B0) passes the reference receiving the item.
	static GrenadeProjectile* InitLiveGrenade(BGSProjectile* apProjectile, Actor* apSource, TESObjectREFR* apTarget,
		TESObjectWEAP* apGrenade);

	BGSProjectileData Data;
	TESModel MuzzleFlashModel;
	SOUND_LEVEL eSoundLevel;
};
inline constexpr fnvd::Vtable BGSProjectile_vftable{};
inline constexpr fnvd::Vtable BGSProjectile_vftable_BGSDestructibleObjectForm{};
inline constexpr fnvd::Vtable BGSProjectile_vftable_BGSPreloadable{};
inline constexpr fnvd::Vtable BGSProjectile_vftable_TESFullName{};
inline constexpr fnvd::Vtable BGSProjectile_vftable_TESModel{};
