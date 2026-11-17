#pragma once

#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpAllCdPointCollector.h"
#include "Reconstruction.hpp"

class HitCaster;
class NiPoint3;
class bhkSimpleShapePhantom;
class bhkWorld;

// Casts a shape phantom through the Havok world and collects every contact point (a small and a large instance).
// The Xbox PDB holds the collector as a member, PointCollector; IDA's PC type derives from it (same layout).
// IDA's offsets are 8 short: the asm has the phantom at 0x3A0, bInit at 0x3A4, iGroup at 0x3A8, and allocates 0x3B0.
class HitCaster : public hkpAllCdPointCollector {
public:
	// Values from the Xbox PDB.
	enum HitCasterSize : int32_t {
		SIZE_SMALL = 0x0,
		SIZE_LARGE = 0x1,
	};

	HitCaster(float afRadius, uint32_t aiGroup);
	~HitCaster() override;

	// The record ends at +0x3AC: the collector's 16-byte alignment (hkVector4) rounds sizeof to 0x3B0, the size the
	// exe allocates (HitCaster::GetInstance 0x626DD0) and the PDB's sizeof; neither has a member there.
	uint32_t GetShapeGroup();
	// Points the custom pick layer at the layers a hit cast collides with.
	void SetCustomPickCollisions();
	// Casts the shape phantom from apOrigin towards apOrigin + apDirection * afPickDistance and sorts the contact
	// points it found; true when it hit anything.
	bool CastShape(const NiPoint3& aOrigin, const NiPoint3& aDirection, float afPickDistance);
	void SetShapeGroup(uint32_t aiGroup);
	// Adds the caster's phantom to apWorld.
	void Add(bhkWorld* apWorld);
	// The point collector (the PDB's PointCollector member at +0), through the exe's shared
	// identity function (thiscall, ret 0, 0x6815C0).
	hkpAllCdPointCollector* GetPointCollector();
	void Init(float afRadius, uint32_t aiGroup);

	// The small or the large caster, created on first use when abCreate.
	static HitCaster* GetInstance(HitCasterSize eSize, bool abCreate);
	// Frees with ourMemalignFree.
	static void operator delete(void* apMemory);
	static void ClearInstance();

	NiPointer<bhkSimpleShapePhantom> spShapePhantom;
	bool bInit;
	// IDA: bool unk3A5.
	uint8_t padding3A5;
	// IDA: bool unk3A6.
	uint8_t padding3A6;
	// IDA: bool unk3A7.
	uint8_t padding3A7;
	uint32_t iGroup;

	static inline HitCaster* pSmallHitCaster;
	static inline HitCaster* pLargeHitCaster;
};

// The game settings HitCaster.cpp defines (PDB), at their exe addresses.
extern SettingT<GameSettingCollection> fHitCasterSizeSmall;
extern SettingT<GameSettingCollection> fHitCasterSizeLarge;
