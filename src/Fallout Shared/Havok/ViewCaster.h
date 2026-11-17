#pragma once

#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class bhkRigidBody;
class bhkShapePhantom;
class bhkWorld;
class NiPoint3;
class TESObjectREFR;

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
	// bInit.
	bool GetInit() { return bInit; }
	// iGroup.
	int32_t GetGroup() { return iGroup; }
	// Makes the second custom pick layer collide with the solid, actor, water and debris layers.
	void SetCustomPickCollisions();
	void* GetObject();

	static bhkRigidBody* GetFuzzyBody();
	static void operator delete(void* apMemory);

	NiPointer<bhkShapePhantom> spShapePhantom;
	bool bInit;
	int32_t iGroup;

	static inline bhkRigidBody* pFuzzyBody;
};

// The game settings ViewCaster.cpp defines.
extern SettingT<INISettingCollection> iMaxViewCasterPicksHavok;
extern SettingT<INISettingCollection> bUseFuzzyPicking;
extern SettingT<INISettingCollection> iMaxViewCasterPicksGamebryo;
extern SettingT<INISettingCollection> iMaxViewCasterPicksFuzzy;
