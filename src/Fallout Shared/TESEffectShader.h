#pragma once

#include "BSShader/BSShaderPPLightingProperty.h"
#include "Fallout Shared/EffectShaderData.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESTexture.h"

class ParticleShaderProperty;

// An effect shader form (EFSH).
class TESEffectShader : public TESForm {
public:
	// The bits of EffectShaderData::cFlags. IsFlagSet ands the whole 32-bit value with the flags byte.
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
	// One animated shader value at afTime: the fade (abFade) or the fade-in, full and fade-out ramps.
	float AnimateValue(float afCurrentValue, float afTimeDelta, float afTime, bool abFade, float afFadeIn,
		float afFadeOut, float afFullTime, float afFullValue, float afPersistentValue);
	float GetEdgeAlphaFadeOutTime();
	TESTexture* GetTextureShaderTexture();
	ParticleShaderProperty* StartParticleShader(NiAVObject* apRootObject, NiAVObject* apTargetObject,
		NiSourceTexture* apTex, float afTime);
	// Stops the particle shader property apProp on apTargetObject.
	void StopParticleShader(NiAVObject* apTargetObject, ParticleShaderProperty* apProp);
	float AnimateEdgeEffectAlpha(float afCurrentAlpha, float afTimeDelta, float afTime, bool abFade);
	EffectShaderData* GetData() { return &Data; }
	float GetFillAlphaPulseAmplitude() { return Data.fFillAlphaPulseAmplitude; }
	float GetFillAlphaPulseFrequency() { return Data.fFillAlphaPulseFrequency; }
	// Clears apTexEffectData from every lighting shader property below apObject that still uses it.
	void StopTextureShader(NiAVObject* apObject, BSShaderPPLightingProperty::TextureEffectData* apTexEffectData);
	// The fill alpha pulse at afTime: 1 without a pulse amplitude, else the amplitude times the sine of the scaled
	// time.
	float CalculateFillEffectPulse(float afTime);
	TESTexture* GetBlockOutTexture();
	float GetAddOnScaleOutTime();
	float GetEdgeAlphaPulseFrequency();
	float GetEdgeAlphaPulseAmplitude();
	double CalculateEdgeAlphaPulse(float a2);
	float GetFillAlphaFullTime();
	float GetFillAlphaFadeOutTime();
	float GetFillAlphaFadeInTime();
	float GetFillAlphaFullPercent();
	float GetFillAlphaPersistentPercent();
	float AnimateFillEffectAlpha(float afCurrentAlpha, float afTimeDelta, float afTime, bool abFade);
	BSShaderPPLightingProperty::TextureEffectData* CreateTextureShaderData(NiAVObject* apObject, NiSourceTexture* apTex,
		NiSourceTexture* apBlockOutTex);
	double AnimateParticleEffectBirthrate(float a1, float a2, float a3, bool a4);
	void AnimateTextureEffect(BSShaderPPLightingProperty::TextureEffectData* pData, float a3, float a4, float a5,
		bool abFade);

	EffectShaderData Data;
	TESTexture TextureShaderTexture;
	TESTexture ParticleShaderTexture;
	TESTexture BlockOutTexture;
};
