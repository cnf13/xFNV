#pragma once

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class BGSProjectile;
class NiAVObject;
class NiLight;
class Projectile;
class TESModel;
class TESObjectREFR;
class TESObjectWEAP;

// the flash model and light a weapon shows at its muzzle while it fires
class MuzzleFlash {
public:
	// The constructor.
	MuzzleFlash(Projectile* apProjectile, TESObjectREFR* apPlayer);
	~MuzzleFlash();

	// Moves the flash with apParentRef and runs its timers.
	void Update(float afDelta, TESObjectREFR* apParentRef);
	// bEnabled; the exe's shared one-byte getter 0x9373F0 (thiscall, ret 0).
	bool GetEnabled();
	// pParentReference; the exe's shared one-field getter 0x441110 (thiscall, ret 0).
	TESObjectREFR* GetParentRef();
	void SetDuration(float a2);
	void UpdateLight();
	NiAVObject* GetAVObject() { return spAVObject; }
	void Enable();
	// The projectile's muzzle flash model, or null when the projectile shows no muzzle flash.
	TESModel* GetMuzzleFlashModel();
	void DetachLight();
	void Init(TESObjectREFR* apParentRef);
	void Position3D(TESObjectREFR* apParentRef);
	void AttachLight();

	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static void SpawnMuzzleFlash(TESObjectREFR* apParentRef, Projectile* apProjectile);

	// 0x0 the flash shows
	bool bEnabled;
	// 0x1
	bool bMPSEnabled;
	// 0x2 the light follows the flash
	bool bUpdateLight;
	// 0x4
	float fEnableTimer;
	// 0x8
	float fDurationTimer;
	// 0xC the flash model
	NiPointer<NiAVObject> spAVObject;
	// 0x10 the flash light
	NiPointer<NiLight> spLight;
	// 0x14 the projectile fired
	BGSProjectile* pProjectile;
	// 0x18 the weapon fired
	TESObjectWEAP* pWeaponSource;
	// 0x1C the reference that fired
	TESObjectREFR* pParentReference;
};

inline bool bwarnOnce = true;
