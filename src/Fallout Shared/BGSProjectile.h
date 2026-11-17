#pragma once

#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSPreloadable.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"

class Actor;
class BGSExplosion;
class GrenadeProjectile;
class TESObjectLIGH;
class TESObjectWEAP;
class TESSound;

enum SOUND_LEVEL : int32_t;

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
	// The bits of Data.iFlags; MOTIONTYPES masks the motion type bits.
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

	// The explosion the projectile sets off.
	BGSExplosion* GetExplosion();
	BGSProjectileData* GetData();
	bool GetHasExplosion();
	bool IsFlagSet(BGSProjectileFlags aeFlag);
	// The motion type bits of the flags.
	BGSProjectileFlags GetMotionType();
	int32_t GetPreCloneCount(TESObjectWEAP* apSourceWeapon);
	bool GetHasExplosionDetonates();
	bool GetHasExplosionAltTrigger();
	// The explosion's alternate-trigger proximity.
	float GetExplosionProximity();
	// The weapon the projectile counts as without a firing weapon (data's pDefaultWeaponSource).
	TESObjectWEAP* GetDefaultWeaponSource();
	bool GetCanTurnOff() { return IsFlagSet(CAN_TURN_OFF); }
	bool GetCanPickup() { return IsFlagSet(CAN_PICKUP); }
	float GetAltTimer() const;
	float GetSpeed();
	TESModel& GetMuzzleFlashModel();
	float GetFadeDuration() const;
	// Data.fForce.
	float GetForce() { return Data.fForce; }
	bool GetHitScan();
	bool GetPinsLimbs() { return IsFlagSet(PINS_LIMBS); }
	bool GetRotates();
	bool GetHasMuzzleFlash();
	bool GetSupersonic() { return IsFlagSet(SUPERSONIC); }
	// The projectile's maximum range.
	float GetRange();
	float GetGravity();
	ENUM_FORM_ID GetProjectileRefFormType();
	bool GetPassThroughSmTransp();
	// Rolls the tracer chance: true when a random 0-99 is below it.
	bool DecideIsTracer();
	float GetTracerChance();
	float GetBounceMultiplier() { return Data.fBounceMultiplier; }

	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static GrenadeProjectile* InitLiveGrenade(BGSProjectile* apProjectile, Actor* apSource, TESObjectREFR* apTarget,
		TESObjectWEAP* apGrenade);

	BGSProjectileData Data;
	TESModel MuzzleFlashModel;
	SOUND_LEVEL eSoundLevel;
};
