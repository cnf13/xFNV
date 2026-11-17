#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Reconstruction.hpp"

class BGSImpactData;
class NiAVObject;
class NiNode;

class BSTempEffectParticle : public BSTempEffect {
public:
	BSTempEffectParticle(TESObjectCELL* apCell, float afLifetime, const char* apFilename, NiAVObject* apParent);
	~BSTempEffectParticle() override;

	const NiRTTI* GetRTTI() const override;
	void Detach() override;
	bool Update(float) override;
	TEMP_EFFECT_TYPE GetType() override;
	bool CheckShouldSave() override;
	uint16_t GetSaveSize() override;
	void SaveGame() override;
	bool LoadGame() override;
	using BSTempEffect::LoadGame;
	using BSTempEffect::SaveGame;

	// Loads the particle file and attaches it below apSceneParent at aRotation, aPosition and
	// afScale; aiFlags' bits 1, 2 and 4 are tested as a dword.
	void Init(NiNode* apSceneParent, NiMatrix3 aRotation, NiPoint3 aPosition, float afScale, uint32_t aiFlags);
	// Plays the impact data's two sounds at the particle model (thiscall, ret 0, 0x68A600). PDB: void; the exe's
	// only caller, SetDecalImpactData, returns whatever eax it leaves, and its callers ignore eax.
	void StartSounds();
	// The lifetime of apAVObject's controller, returned in ST0 (thiscall, ret 4).
	float GetControllerLifetime(NiAVObject* apAVObject);
	// Plays the particle model's sequence apcSequenceName.
	void PlaySequence(const char* apcSequenceName);
	// The particle model (spParticleObject). The body is folded with BSTempEffectDebris::Get3D at 0x490E40.
	NiAVObject* GetParticleObject();
	// The followed object (spFollowObject), through the exe's +0x20 NiPointer getter (thiscall,
	// ret 0, 0x700300).
	NiAVObject* GetFollowObject();
	void StopSounds();
	void SetDecalImpactData(BGSImpactData* apObject);
	void ResetGravityRecursive(NiAVObject* apObject, NiTPrimitiveArray<NiAVObject*>& iArray, uint32_t& aiCount);
	void ResetGravity(NiAVObject* apObject);

	// Creates the particle effect apParticleFilename in apCell at aPosition facing aVector, for
	// afLifetime seconds at afScale, following apFollowObject when given (cdecl, 0x6890B0; the NiPoint3 overload).
	static BSTempEffectParticle* Spawn(TESObjectCELL* apCell, float afLifetime, const char* apParticleFilename,
		NiPoint3 aVector, NiPoint3 aPosition, float afScale, uint32_t aiFlags, NiAVObject* apFollowObject);
	// The same with the effect's rotation given as aRotation (cdecl, 0x689210; the NiMatrix3
	// overload).
	static BSTempEffectParticle* Spawn(TESObjectCELL* apCell, float afLifetime, const char* apParticleFilename,
		NiMatrix3 aRotation, NiPoint3 aPosition, float afScale, uint32_t aiFlags, NiAVObject* apFollowObject);

	NiPointer<NiAVObject> spParticleObject;
	const char* pFilename;
	NiPointer<NiAVObject> spFollowObject;
	// PC NiTransform uses four-byte alignment, unlike the Xbox PDB's follow transform at 0x30.
	NiTransform kFollowOffset;
	BGSImpactData* pDecalImpactData;
	BSSoundHandle Sound1;
	BSSoundHandle Sound2;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable BSTempEffectParticle_vftable{};
