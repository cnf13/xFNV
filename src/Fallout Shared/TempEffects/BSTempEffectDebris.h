#pragma once

#include "BSCore/BSList.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Reconstruction.hpp"

class NiMatrix3;
class NiPoint3;
class TESObjectCELL;
class TESObjectREFR;

// A piece of debris flying off a destroyed object: a model with its own Havok body, kept until it has lived its
// lifetime or the debris limit pushes it out.
class BSTempEffectDebris : public BSTempEffect {
public:
	BSTempEffectDebris(TESObjectCELL* apCell, float afLifetime, const char* apFilename, TESObjectREFR* apSourceRef,
		const NiPoint3& aPosition, NiMatrix3& aRotation, NiPoint3& aStartLinearVelocity,
		NiPoint3& aStartAngularVelocity, float afScale, bool bUseCache, bool abAddDebrisCount);
	BSTempEffectDebris(TESObjectCELL* apCell, float afLifetime, const char* apFilename, TESObjectREFR* apSourceRef,
		const NiPoint3& aPosition, NiPoint3& aDirection, NiPoint3& aStartLinearVelocity,
		NiPoint3& aStartAngularVelocity, float afScale, bool bUseCache, bool abAddDebrisCount);
	~BSTempEffectDebris() override;

	const NiRTTI* GetRTTI() const override;
	bool Update(float afTimeDelta) override;
	NiAVObject* Get3D() override;
	TEMP_EFFECT_TYPE GetType() override;
	bool IsFirstPerson() override;

	void SetForceDelete(bool abValue);
	// Stores abValue in b1stPerson.
	void SetIs1stPerson(bool abValue);
	void Init(TESObjectREFR* apSourceRef, const NiPoint3& aPosition, NiMatrix3& aRotation,
		NiPoint3& aStartLinearVelocity, NiPoint3& aStartAngularVelocity, float afScale, bool bUseCache);
	void InitHavok(TESObjectREFR* apSourceRef);

	static void AddtoDebrisCounter(BSTempEffectDebris* abDebrisObject);
	static void RemoveFromDebrisCounter(BSTempEffectDebris* abDebrisObject);
	static void LimitDebrisObjects();

	// PDB NiPointer<NiAVObject>; the exe hands it to TES::GetUnusedCachedParticleObject as one (0x682D10).
	NiPointer<NiAVObject> sp3D;
	const char* pFilename;
	bool bUseDebrisCounter;
	bool bForceDelete;
	// IDA name; not in the PDB (a PC-only field, set by SetIs1stPerson 0x7E6580).
	bool b1stPerson;

	static NiRTTI ms_RTTI;
	static BSList<BSTempEffectDebris*> DebrisLimitList;
};
inline constexpr fnvd::Vtable BSTempEffectDebris_vftable{};
