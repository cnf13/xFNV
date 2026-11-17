#pragma once

#include "Fallout/AI/Combat/Projectile.h"
#include "Reconstruction.hpp"

// A flame projectile: a short-lived stream of fire.
// Only the entry points the reconstructed code calls are declared.
class FlameProjectile : public Projectile {
public:
	// Constructs the projectile in apMemory, with a process when abAdd (the exe's constructor,
	// thiscall, ret 4, 0x9B3160).
	explicit FlameProjectile(bool abAdd);
	FlameProjectile(BGSProjectile* a1, TESObjectREFR* a2, TESObjectWEAP* a3, NiPoint3 a4, float a5, float a6);

	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	using Projectile::InitLoadGame;
	// Clears fExpirationTimer and fConeAngle after the projectile's revert.
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
	using Projectile::Revert;
	void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	using Projectile::FinishLoadGame;
	Projectile::PROJECTILE_TYPE GetProjectileType() override;
	void Initialize() override;
	void Process3D() override;
	void UpdateProjectile(float afDelta) override;
	bool RunTargetPick() override;

protected:
	bool AddImpact(TESObjectREFR* apCollidee, const NiPoint3& aContactPoint, const NiPoint3& aContactNormal,
		const hkpCollidable* apTargetCollidable, uint32_t aiKey, HavokMaterialType aeForceMaterial) override;

public:
	// fConeAngle.
	float GetConeAngle() { return fConeAngle; }
	void ResetExpirationTimer();

	// The widest cone angle of the flame particle systems at or under apObject (cdecl,
	// 0x9B3DA0). The exe passes any NiAVObject and recurses through nodes.
	static float FindFlameConeAngle(NiAVObject* apObject);

	float fExpirationTimer;
	float fConeAngle;
};
inline constexpr fnvd::Vtable FlameProjectile_vftable{};
inline constexpr fnvd::Vtable FlameProjectile_vftable_TESChildCell{};
