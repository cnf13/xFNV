#pragma once

#include "Fallout Shared/EffectShaderData.h"
#include "Fallout Shared/TESForm.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "Fallout Shared/TESTexture.h"
#include "Reconstruction.hpp"

class ParticleShaderProperty;

// An effect shader form (EFSH).
class TESEffectShader : public TESForm {
public:
	// The bits of EffectShaderData::cFlags. IsFlagSet ands the whole dword with the flags byte.
	enum EffectShaderFlags : int32_t {
		DISABLE_TEXTURE_SHADER = 1,
		UNUSED_FLAG1 = 2,
		UNUSED_FLAG2 = 4,
		DISABLE_PARTICLE_SHADER = 8,
		EDGECOLOR_SUBTRACTIVE = 16,
		SKIN_ONLY = 32,
	};

	TESEffectShader();
	~TESEffectShader() override;

	void InitializeData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;

	using TESForm::Save;

	// Sets apShaderProp up as this shader's particle shader.
	void SetupParticleShader(ParticleShaderProperty* apProperty, NiSourceTexture* apTex);
	// Starts the texture shader on apNode.
	void StartTextureShader(NiAVObject* apObject, BSShaderPPLightingProperty::TextureEffectData* apTexEffectData);
	// Whether flag aeFlag of the shader's flags is set.
	bool IsFlagSet(EffectShaderFlags aeFlag);
	// One animated shader value at afTime: the fade (abFade) or the fade-in, full and fade-out ramps (ret 0x24).
	float AnimateValue(float afCurrentValue, float afTimeDelta, float afTime, bool abFade, float afFadeIn,
		float afFadeOut, float afFullTime, float afFullValue, float afPersistentValue);
	float GetEdgeAlphaFadeOutTime();
	TESTexture* GetTextureShaderTexture();
	ParticleShaderProperty* StartParticleShader(NiAVObject* apRootObject, NiAVObject* apTargetObject,
		NiSourceTexture* apTex, float afTime);
	// Stops the particle shader property apProp on apTargetObject.
	void StopParticleShader(NiAVObject* apTargetObject, ParticleShaderProperty* apProp);
	float AnimateEdgeEffectAlpha(float afCurrentAlpha, float afTimeDelta, float afTime, bool abFade);
	// The exe calls that shared "this + 0x18" getter.
	EffectShaderData* GetData() { return &Data; }
	// Same function as BGSLightingTemplate::GetDataFogPower (0x598040); the exe calls that shared float getter of +0x3C.
	float GetFillAlphaPulseAmplitude() { return Data.fFillAlphaPulseAmplitude; }
	// The exe calls that shared float getter of +0x40.
	float GetFillAlphaPulseFrequency() { return Data.fFillAlphaPulseFrequency; }
	// Clears apTexEffectData from every lighting shader property below apObject that still uses it.
	void StopTextureShader(NiAVObject* apObject, BSShaderPPLightingProperty::TextureEffectData* apTexEffectData);
	// The fill alpha pulse at afTime: 1 without a pulse amplitude, else the amplitude times the sine of the scaled
	// time.
	float CalculateFillEffectPulse(float afTime);
	TESTexture* GetBlockOutTexture();
	// The exe returns the float field with fld dword.
	float GetAddOnScaleOutTime();
	// Name not in the PDB (the field at +0x68); folded accessor, IDA: BGSProjectile::GetSpeed.
	float GetEdgeAlphaPulseFrequency();
	// Name not in the PDB (the field at +0x64); folded accessor, IDA: NiNode::GetLocalScale.
	float GetEdgeAlphaPulseAmplitude();
	// Inferred type: return: double: used as one at 0x507f37; a2: float from 0x507f37 (parameter 3 of
	// TESEffectShader::AnimateTextureEffect)
	double CalculateEdgeAlphaPulse(float a2);
	// Name not in the PDB (the field at +0x30); folded accessor, IDA: NiControllerSequence::GetEndKeyTime.
	float GetFillAlphaFullTime();
	// Name not in the PDB (the field at +0x34); a folded accessor.
	float GetFillAlphaFadeOutTime();
	// Name not in the PDB (the field at +0x2C); folded accessor, IDA: NiControllerSequence::GetBeginKeyTime.
	float GetFillAlphaFadeInTime();
	// Name not in the PDB (the field at +0x6C); folded accessor, IDA: BGSProjectile::GetRange.
	float GetFillAlphaFullPercent();
	// Name not in the PDB (the field at +0x38); folded accessor, IDA: BGSImpactData::GetAngleThreshold.
	float GetFillAlphaPersistentPercent();
	float AnimateFillEffectAlpha(float afCurrentAlpha, float afTimeDelta, float afTime, bool abFade);
	BSShaderPPLightingProperty::TextureEffectData* CreateTextureShaderData(NiAVObject* apObject, NiSourceTexture* apTex,
		NiSourceTexture* apBlockOutTex);
	// Inferred type: a1: float from 0x507dfa (passed to TESEffectShader::AnimateValue 0); a2: float from 0x8211a5
	// (parameter 0 of MagicShaderHitEffect::Update); a3: float from 0x8211a5 (MagicShaderHitEffect::fAlphaTimer (this
	// of MagicShaderHitEffect::Update))
	double AnimateParticleEffectBirthrate(float a1, float a2, float a3, bool a4);
	void AnimateTextureEffect(BSShaderPPLightingProperty::TextureEffectData* pData, float a3, float a4, float a5,
		bool abFade);

	EffectShaderData Data;
	TESTexture TextureShaderTexture;
	TESTexture ParticleShaderTexture;
	TESTexture BlockOutTexture;
};
inline constexpr fnvd::Vtable TESEffectShader_vftable{};
