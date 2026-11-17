#pragma once

#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "BSShader/ParticleShader.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// The shader property of the particles an effect shader emits over its target.
class ParticleShaderProperty : public BSShaderProperty {
public:
	enum EMITTER_TYPE {
		EMIT_GEOMETRY = 0,
		EMIT_COLLISION = 1,
		EMIT_RAY = 2,
		EMIT_TOTAL = 3,
	};

	ParticleShaderProperty();
	~ParticleShaderProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	bool SetupGeometry(NiGeometry* apGeometry) override;
	BSShaderProperty::RenderPassArray* GetRenderPasses(NiGeometry* apGeom, int32_t aiEnabledPasses, uint16_t& asCount,
		uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator, int32_t abUpdateList) override;

	// Starts the particle data at afTime.
	void InitParticleData(float afTime);
	// Advances the particles to afTime: retires the expired ones and, with abGenerateNew, emits new ones.
	void UpdateParticles(float afTime, bool abNeedsOffset, bool abGenerateNew);
	// Emit particle aiIndex along the ray, from the collision bones or
	// from the geometry.
	void GenerateFromRay(uint32_t aiIndex);
	void GenerateFromCollisionBones(uint32_t aiIndex);
	void GenerateFromGeometry(uint32_t aiIndex);
	// Replaces the base texture.
	void SetTexture(NiTexture* pTex);
	// Emit along a ray of length afRayLength, from collision bones, or from the geometry. The exe leaves this in
	// eax, so they return it.
	ParticleShaderProperty* SetEmitterTypeRay(float afRayLength);
	ParticleShaderProperty* SetEmitterTypeCollision();
	ParticleShaderProperty* SetEmitterTypeGeometry();
	void CopyMembers(ParticleShaderProperty* pDest, NiCloningProcess& arCloneData);
	void SetColor2(NiColorA aColor);
	// Inferred type: a2: float from 0x50709d (EffectShaderData::fParticleLifeVar (this of
	// TESEffectShader::SetupParticleShader))
	void SetLifetimeVar(float a2);
	void SetBlendOp(D3DBLENDOP a2);
	void SetZTestFunc(D3DCMPFUNC a2);
	// Inferred type: a2: float from 0x5071d4 (EffectShaderData::fExplosionSpeed (this of
	// TESEffectShader::SetupParticleShader))
	void SetExplosionWindSpeed(float a2);
	void SetFade(float a2);
	// The PDB's float setters; each is a fold of another class's setter of the same field
	// offset (thiscall, ret 4).
	void SetLifetime(float afLifetime);
	void SetNormalSpeed(float afNormalSpeed);
	void SetNormalSpeedVar(float afValue);
	void SetNormalAcceleration(float afNormalAcc);
	void SetRotate(float afValue);
	void SetRotateVar(float afValue);
	void SetRotateSpeed(float afValue);
	void SetRotateSpeedVar(float afValue);
	void SetColorTime1(float aColorTime);
	void SetColorTime2(float aColorTime);
	void SetColorTime3(float aColorTime);
	void SetScale2(float aScale);
	void SetScaleTime1(float aScaleTime);
	void SetScaleTime2(float aScaleTime);
	void SetTextureCountX(float afValue);
	void SetTextureCountY(float afValue);
	// The PDB's setter of fMaxParticleRatio; a fold of TileShaderProperty::SetTileAlpha
	// (thiscall, ret 4).
	void SetMaxParticleRatio(float afMaxParticle);
	void SetVelocity(NiPoint3 aVelocity);
	void SetAcceleration(NiPoint3 aAcceleration);
	void SetColor1(NiColorA aColor);
	void SetColor3(NiColorA aColor);
	// Inferred type: a2: float from 0x507453 (EffectShaderData::fParticleScale1 (this of
	// TESEffectShader::SetupParticleShader))
	void SetScale1(float a2);
	NiTObjectArray<NiPointer<NiAVObject>>* GetTargetArray();
	// IDA name; the PDB names it SetWorldSpace(bool).
	void SetByte6C(bool abWorldSpace);
	void RewindTimer(float afNewTime);
	void CollectGeometryRecursive(NiAVObject* apObject);
	void CollectCollisionBonesRecursive(NiAVObject* apObject);
	void CollectTargets(NiAVObject* apObject);

	// Inferred type: return: uint8_t* from 0xba67c3 (passed to NiAdditionalGeometryData::SetDataBlock 1)
	static uint8_t* GetInstanceIndex();

	// ParticleShader::uiInstanceCount particles; UpdateParticles walks them as ParticleData.
	ParticleShader::ParticleData* pInstanceData;
	EMITTER_TYPE eEmitterType;
	float fRayLength;
	bool bWorldspace;
	uint8_t padding06D[3];
	int32_t iParticleCount;
	float fGenerateRemainder;
	float fMaxParticleRatio;
	float fLifetime;
	float fLifeVar;
	float fNormalSpeed;
	float fNormalSpeedVar;
	float fNormalAcceleration;
	NiPoint3 Velocity;
	NiPoint3 Acceleration;
	float fRotate;
	float fRotateVar;
	float fRotateSpeed;
	float fRotateSpeedVar;
	float ColorTime1;
	float ColorTime2;
	float ColorTime3;
	NiColorA Color1;
	NiColorA Color2;
	NiColorA Color3;
	float Scale1;
	float Scale2;
	float ScaleTime1;
	float ScaleTime2;
	float fTimer;
	D3DBLEND eSrcBlendMode;
	D3DBLEND eDestBlendMode;
	D3DBLENDOP eBlendOperation;
	D3DCMPFUNC eZTestFunction;
	NiPointer<NiTexture> spBaseTexture;
	// The PDB's TargetArray: NiTObjectArray<NiPointer<NiAVObject>> (vtable 0x101FBD4 at 0x11C, m_pBase at 0x120,
	// m_usMaxSize at 0x124, m_usSize at 0x126, m_usESize at 0x128, m_usGrowBy at 0x12A).
	NiTObjectArray<NiPointer<NiAVObject>> TargetArray;
	NiGeometry* pMyGeometry;
	float fFade;
	float fTextureCountX;
	float fTextureCountY;
	NiPoint3 WindPoint;
	float fWindSpeed;

	// The number of live particle shader properties; the last one's destruction frees the shared data.
	static inline int32_t iRefCount;
	static inline NiPoint3 Gravity{};
	static NiRTTI ms_RTTI;
	static inline void* pfWindData;
};
inline constexpr fnvd::Vtable ParticleShaderProperty_vftable{};
