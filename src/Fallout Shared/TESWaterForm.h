#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/TESAttackDamageForm.h"
#include "Fallout Shared/TESForm.h"
#include "BSShader/BSRenderedTexture.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/WaterShaderData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "Reconstruction.hpp"

class BGSPlaceableWater;
class SpellItem;
class TESSound;
class TESWaterForm;

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
void Init_57E9A0(WaterShaderData* apThis, [[maybe_unused]] void* apEdx);

// A water type form (WATR).
class TESWaterForm : public TESForm, public TESFullName, public TESAttackDamageForm {
public:
	TESWaterForm();
	~TESWaterForm() override;

	void InitializeData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	bool Activate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abIdFlag,
		TESBoundObject* apObjecttoGet, int32_t aiCount) override;

	using TESForm::LoadGame;
	using TESForm::Revert;
	using TESForm::Save;
	using TESForm::SaveGame;

	// The water's sound (pWaterSound); the exe's shared +0x7C getter (thiscall, ret 0,
	// 0x407840).
	TESSound* GetWaterSound();

	// Sets pWaterSound from a chunk's form ID; the exe's folded one-field setter (thiscall,
	// ret 4, 0x4FEBD0).
	void SetWaterSound(TESSound* apSound);
	// Sets the water's drink effect (pSpellItem); Load passes the chunk's form ID, which InitItem resolves (thiscall,
	// ret 4, 0x57F890). The exe leaves this in eax, which its callers never read.
	void SetSpellItem(SpellItem* pItem);

	// The noise texture name's length (TESTexture at +0x64), thiscall at 0x580080.
	uint32_t GetNoiseTextureNameLength();

	// The noise texture's file name, thiscall at 0x5800A0.
	const char* GetNoiseTextureName();

	// Stores apTexture in spNoiseTexture (+0x17C), thiscall ret 4 at 0x5800C0.
	void SetNoiseTexture(NiTexture* apTexture);
	// Sets aspTexture to the noise texture.
	void GetNoiseTexture(NiPointer<NiTexture>& aspTexture);
	float GetHeightUVScale0();
	float GetLightRadius();
	float GetLightBrightness();
	WaterShaderData* GetData();
	float GetAlpha();
	int32_t GetRadiationDamage();
	// The health the water restores.
	int32_t GetHealthRestoration();
	double GetFogScale();
	bool ShouldResetNoiseTextures();
	bool IsDangerous();
	// Inferred type: return: double: used as one at 0x4e3b99
	double GetDepthY();
	SpellItem* GetSpellItem();
	void SetMaterialName(const char* apMaterial);
	// The material name (cMaterialID), or the shared empty string.
	const char* GetMaterialName();
	void SetRemappedWaterForm(TESWaterForm* apWater);
	BGSPlaceableWater* GetPlaceableAutoWater();
	BGSPlaceableWater* GetPlaceableLODWater();

	bool bNeedUpdate;
	NiPointer<BSRenderedTexture> spNoiseNormalMap;
	NiPoint4 fTexScroll0;
	NiPoint4 fTexScroll1;
	NiPoint4 fTexScroll2;
	TESTexture NoiseTexture;
	uint8_t cAlpha;
	uint8_t cFlags;
	BSStringT<char> cMaterialID;
	TESSound* pWaterSound;
	TESWaterForm* pRemappedWaterForm;
	WaterShaderData Data;
	TESWaterForm* pWaterWeatherControl[3];
	int32_t iCurrentTextureSelect01;
	int32_t iCurrentTextureSelect02;
	uint32_t fFrequencyX;
	uint32_t fFrequencyY;
	int32_t iOctaves;
	float fAmplitude;
	float fLacunarity;
	float fBias;
	float fGain;
	SpellItem* pSpellItem;
	NiPointer<NiTexture> spNoiseTexture;
	BGSPlaceableWater* pPlaceableAutoWater;
	BGSPlaceableWater* pPlaceableLODWater;
	int32_t iRadiationDamage;
	int32_t iHealthRestoration;
	bool bResetNoiseTexture;
	uint8_t padding191[3];

	static SettingT<INISettingCollection> iWaterNoiseResolution;
};
inline constexpr fnvd::Vtable TESWaterForm_vftable_TESAttackDamageForm{};
inline constexpr fnvd::Vtable TESWaterForm_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESWaterForm_vftable{};
