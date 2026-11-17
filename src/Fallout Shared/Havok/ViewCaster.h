#pragma once

#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class NiPoint3;
class TESObjectREFR;
class bhkRigidBody;
class bhkShapePhantom;
class bhkWorld;

// The phantom sphere the interface casts along the player's view to find the reference to activate.
class ViewCaster {
public:
	ViewCaster();
	~ViewCaster();

	// Builds the phantom with the given radius in the given collision group.
	void Init(float afRadius, uint32_t aiGroup);
	// The Havok world the phantom is in.
	bhkWorld* GetWorld();
	// Moves the phantom into apWorld.
	void Add(bhkWorld* apWorld);
	void SetShapeGroup(uint32_t aiGroup);
	// Casts the phantom along the ray up to afPickDistance; returns the reference found and
	// writes its distance and whether the hit was fuzzy.
	TESObjectREFR* FindActivateREFR(const NiPoint3& aOrigin, const NiPoint3& aDirection, float afPickDistance,
		float& afObjectDistance, bool& abIsFuzzy);
	// bInit. Same function as BSExtraData::GetExtraType (0x4F1540), registered there.
	bool GetInit() { return bInit; }
	// iGroup. Same function as TESForm::GetFormFlags (0x44DDC0), registered there.
	int32_t GetGroup() { return iGroup; }
	// Makes the second custom pick layer collide with the solid, actor, water and debris layers.
	void SetCustomPickCollisions();
	// Name not in the PDB (the field at +0x0); folded accessor, IDA: ??CNiPointer@@QBE?AV0@ABV0@@Z.
	void* GetObject();

	static bhkRigidBody* GetFuzzyBody();
	static void operator delete(void* apMemory);

	NiPointer<bhkShapePhantom> spShapePhantom;
	bool bInit;
	int32_t iGroup;

	static inline bhkRigidBody* pFuzzyBody;
};

// The game settings ViewCaster.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> iMaxViewCasterPicksHavok;
extern SettingT<INISettingCollection> bUseFuzzyPicking;
extern SettingT<INISettingCollection> iMaxViewCasterPicksGamebryo;
extern SettingT<INISettingCollection> iMaxViewCasterPicksFuzzy;
