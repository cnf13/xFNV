#pragma once

#include "Fallout Shared/DecalData.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Reconstruction.hpp"

class BGSTextureSet;
enum SOUND_LEVEL : int32_t;
class TESSound;

// An impact data form (IPCT): the effect, decal and sounds of a hit on one material.
class BGSImpactData : public TESForm, public TESModel {
public:
	enum ORIENTATION : int32_t {
		ORIENT_SURFACE_NORMAL = 0,
		ORIENT_PROJ_VECTOR = 1,
		ORIENT_PROJ_REFLECT = 2,
		ORIENT_COUNT = 3,
	};

	// The impact's data as older files store it, the decal data included.
	struct OLD_IMPACT_DATA_DATA {
		// Swaps every multi-byte field read from a big-endian file.
		void Endian();

		float fDecalMinWidth;
		float fEffectDuration;
		float fParallaxScale;
		ORIENTATION eOrient;
		uint8_t cFlags;
		uint8_t cParallaxPasses;
		uint8_t padding12[2];
		float fDecalMaxWidth;
		float fDepth;
		float fDecalMinHeight;
		float fDecalMaxHeight;
		float fShininess;
		float fAngleThreshold;
		float fPlacementRadius;
		uint32_t iColor;
		SOUND_LEVEL eSoundLevel;
	};

	// The impact's effect settings (DATA).
	struct IMPACT_DATA_DATA {
		// Swaps the five dwords read from a big-endian file, in field order. Called out of line, as the exe does.
		void Endian();

		float fEffectDuration;
		ORIENTATION eOrient;
		float fAngleThreshold;
		float fPlacementRadius;
		SOUND_LEVEL eSoundLevel;
		uint8_t cFlags;
		// IDA name (ucResultOverride); not in the PDB.
		uint8_t cResultOverride;
		uint8_t padding16[2];
	};

	BGSImpactData();
	~BGSImpactData() override;

	void InitializeData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	using TESForm::Save;

	bool GetUseTextureSetDecalData();
	float GetDecalShininess();
	float GetDecalParallaxScale();
	uint32_t GetDecalColor();
	float GetDecalDepth();
	bool GetIsAlphaBlend();
	uint8_t GetParallaxPasses();
	// The game setting value of the impact's sound level; 0 for an unknown level.
	int32_t GetSoundLevelValue();
	// Copies the texture set's decal data (or the default one) into DData when the impact uses
	// the texture set's decals.
	void CopyDecalDataFromTextureSet();
	double GetMinWidth();
	// The decal's maximum width (thiscall, ST0, 0x4A40C0).
	float GetDecalMaxWidth();
	// pDecalTextureSet: the exe's shared one-dword getter at +0x48 (thiscall, 0x6733E0).
	BGSTextureSet* GetDecalTextureSet();
	bool GetIsParallax();
	bool GetIsAlphaTest();
	// The decal placement radius: the exe's shared float getter (thiscall, ret 0, ST0,
	// 0x598040).
	float GetPlacementRadius();
	// The decal angle threshold: the exe's shared float getter (thiscall, ret 0, ST0,
	// 0x9A9350).
	float GetDecalAngleThreshold();
	// Data.fEffectDuration (thiscall, ret 0, ST0, 0x508100).
	float GetEffectDuration();
	// pSound1.
	TESSound* GetSound1();
	// pSound2.
	TESSound* GetSound2();

	// Sets the decal texture set (pDecalTextureSet): the exe's shared one-dword setter at +0x48
	// (thiscall, ret 4, 0x673400).
	void SetDecalTextureSet(BGSTextureSet* apSet);
	// Sets pSound1: the exe's shared one-dword setter at +0x4C (thiscall, ret 4, 0x589F50).
	void SetSound1(TESSound* apSound);
	// Sets pSound2: the exe's shared one-dword setter at +0x50 (thiscall, ret 4, 0x58E430).
	void SetSound2(TESSound* apSound);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	IMPACT_DATA_DATA Data;
	BGSTextureSet* pDecalTextureSet;
	TESSound* pSound1;
	TESSound* pSound2;
	DecalData DData;
};
inline constexpr fnvd::Vtable BGSImpactData_vftable_TESModel{};
inline constexpr fnvd::Vtable BGSImpactData_vftable{};
