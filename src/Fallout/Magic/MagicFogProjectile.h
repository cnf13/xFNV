#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "Fallout/Magic/MagicProjectile.h"

class MagicLight;

class MagicFogProjectile : public MagicProjectile {
public:
	// One reference the fog affects, with the effect applied to it; a singly linked list.
	struct SpellTarget {
		SpellTarget(TESObjectREFR* apTarget, EffectItem* apEffect, MagicFogProjectile::SpellTarget* apNext);

		TESObjectREFR* pTarget;
		EffectItem* pEffect;
		SpellTarget* pNext;
	};

	// The fog's stage; values and spelling from the Xbox PDB.
	enum State : int32_t {
		FOG_STATE_SHOOTING = 0,
		FOG_STATE_PERSISTANT = 1,
	};

	// The default constructor (Construct_008192A0).
	MagicFogProjectile();
	MagicFogProjectile(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apDominantEffect,
		EffectSetting* apAVEffect, NiPoint3 aOrigin, NiPoint3 aRotation, NiAVObject* apData3D, MagicLight* apLight);
	~MagicFogProjectile() override;

	uint16_t GetSaveSize(uint32_t aiFlags) override;
	void SaveGame(uint32_t aiFlags) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) override;
	void FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) override;
	void InitHavok() override;
	float GetAcrobatics() override;
	void Update(float afDelta) override;
	void Collide(NiPoint3 aContactPoint, bhkRigidBody* apTargetBody, TESObjectREFR* apCollidee, bool abFizzle) override;
	bool IsActive() override;
	void BuildAreaEffectGeometry() override;
	bool CheckAddTarget(MagicTarget* apSpellTarget, ActiveEffect* apEffect) override;

	void ClearTargetList();
	// Sets up the fog's particle 3D.
	void Init3D();
	// Scales the particle systems under apav by the fog's duration and area settings.
	void AdjustParticles(NiAVObject* apav);

	// The PC base is 0xA8 bytes, sixteen bytes smaller than the PDB's Xbox base.
	float fAge;
	float fProjectileDuration;
	float fProjectileArea;
	State eState;
	float fScale;
	BSSoundHandle Sound;
	SpellTarget* pTargetList;
	MagicLight* pLight;
};

inline constexpr fnvd::Vtable MagicFogProjectile_vftable{};
// The vtable of the TESChildCell part, which the constructor writes at +0x18.
inline constexpr fnvd::Vtable MagicFogProjectile_vftable_TESChildCell{};
