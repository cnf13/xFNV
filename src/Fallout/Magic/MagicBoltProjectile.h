#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSMain/Setting.h"
#include "Fallout/Magic/MagicProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"

class BoltShaderProperty;
class EffectSetting;
class MagicCaster;
class MagicItem;

// The bolt's state LoadGame (0x814870) reads from the save and keeps until the load finishes (PDB names; the PDB's
// source is MagicBoltProjectile.cpp).
struct BoltSavedData {
	BoltSavedData();

	NiPoint3 StartingLocation;
	NiQuaternion Rotation;
	float fBoltTimer;
	// The area bolts' targets as read: a count, then that many form IDs (LoadGame 0x814870).
	uint32_t* pAreaBoltData;
};

class MagicBoltProjectile : public MagicProjectile {
public:
	// An area bolt's shader, scene node, source and target, linked into the projectile's bolt list.
	struct AreaBolt {
		AreaBolt();

		static void operator delete(void* apMemory);

		NiPointer<BoltShaderProperty> spBoltProperty;
		NiPointer<NiNode> spBoltNode;
		NiPoint3 SourceLoc;
		NiPointer<NiAVObject> spSource;
		NiPointer<NiAVObject> spTarget;
		AreaBolt* pNext;
	};

	// The bolt's stage; values from the Xbox PDB. A new bolt starts as TRUNK (0x811980 passes 0).
	enum State : int32_t {
		TRUNK = 0,
		BRANCH = 1,
		PERSIST = 2,
	};

	MagicBoltProjectile();
	MagicBoltProjectile(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apDominantEffect,
		EffectSetting* apAVEffect, NiPoint3 aOrigin, NiPoint3 aRotation);
	~MagicBoltProjectile() override;

	uint16_t GetSaveSize(uint32_t aiFlags) override;
	void SaveGame(uint32_t aiFlags) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) override;
	void FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) override;
	void InitHavok() override;
	void Update(float afDelta) override;
	void Collide(NiPoint3 aContactPoint, bhkRigidBody* apTargetBody, TESObjectREFR* apCollidee, bool abFizzle) override;
	void AddAreaTarget(MagicTarget* apTarget, ActiveEffect* apActiveEffect) override;
	bool PostProcess() override;

	void SetState(State aeNewState);
	void UpdateBranch(float afDelta, MagicBoltProjectile::AreaBolt* apBranch);
	void Init3D();

	// The PC MagicProjectile base is 0xA8 bytes, not the Xbox PDB's 0xB8.
	NiPointer<BoltShaderProperty> spBoltProperty;
	State eState;

	// Between LoadGame and FinishInitLoadGame the field holds the loaded BoltSavedData, which
	// FinishInitLoadGame applies, frees and replaces with null.
	union {
		AreaBolt* pAreaBolts;
		// Not named in the PDB; name chosen here.
		BoltSavedData* pSavedData;
	};

	NiPointer<NiNode> spBoltNode;
	NiPointer<NiRefObject> spSource;
	NiPointer<NiAVObject> spTarget;
	NiPointer<NiNode> spSoundNode;
	// The reference the bolt hit (Collide 0x8133F0 stores its collidee; InitLoadGame casts it to TESObjectREFR).
	TESObjectREFR* pTarget;
	BSSoundHandle Sound;
	float fCollideTime;
};
inline constexpr fnvd::Vtable MagicBoltProjectile_vftable_TESChildCell{};
inline constexpr fnvd::Vtable MagicBoltProjectile_vftable{};

// GameSettingCollection: 0xf6c56e calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fSubSegmentVariance;
