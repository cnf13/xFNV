#pragma once

#include "BSShader/BSShaderProperty.h"
#include "BSShader/ParticleShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

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

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    bool SetupGeometry(NiGeometry* apGeometry) override;
    BSShaderProperty::RenderPassArray* GetRenderPasses(
        NiGeometry* apGeom,
        int32_t aiEnabledPasses,
        uint16_t& asCount,
        uint32_t auiRenderMode,
        BSShaderAccumulator* apAccumulator,
        int32_t abUpdateList
    ) override;

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
    // Emit along a ray of length afRayLength, from collision bones, or from the geometry.
    ParticleShaderProperty* SetEmitterTypeRay(float afRayLength);
    ParticleShaderProperty* SetEmitterTypeCollision();
    ParticleShaderProperty* SetEmitterTypeGeometry();
    void CopyMembers(ParticleShaderProperty* pDest, NiCloningProcess& arCloneData);
    void SetColor2(NiColorA aColor);
    void SetLifetimeVar(float afLifetimeVar);
    void SetBlendOp(D3DBLENDOP aeBlendOp);
    void SetZTestFunc(D3DCMPFUNC aeZTestFunc);
    void SetExplosionWindSpeed(float afWindSpeed);
    void SetFade(float afFade);
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
    void SetMaxParticleRatio(float afMaxParticle);
    void SetVelocity(NiPoint3 aVelocity);
    void SetAcceleration(NiPoint3 aAcceleration);
    void SetColor1(NiColorA aColor);
    void SetColor3(NiColorA aColor);
    void SetScale1(float aScale);
    NiTObjectArray<NiPointer<NiAVObject>>* GetTargetArray();
    void SetByte6C(bool abWorldSpace);
    void RewindTimer(float afNewTime);
    void CollectGeometryRecursive(NiAVObject* apObject);
    void CollectCollisionBonesRecursive(NiAVObject* apObject);
    void CollectTargets(NiAVObject* apObject);

    static uint8_t* GetInstanceIndex();

    // ParticleShader::uiInstanceCount particles; UpdateParticles walks them as ParticleData.
    ParticleShader::ParticleData* pInstanceData;
    EMITTER_TYPE eEmitterType;
    float fRayLength;
    bool bWorldspace;
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

inline void ParticleShaderProperty::SetTexture(NiTexture* pTex) {
    spBaseTexture = pTex;
}

inline void ParticleShaderProperty::SetScaleTime2(float aScaleTime) {
    ScaleTime2 = aScaleTime;
}

inline void ParticleShaderProperty::SetVelocity(NiPoint3 aVelocity) {
    Velocity = aVelocity;
}

inline void ParticleShaderProperty::SetAcceleration(NiPoint3 aAcceleration) {
    Acceleration = aAcceleration;
}

inline void ParticleShaderProperty::SetColor1(NiColorA aColor) {
    Color1 = aColor;
}

inline void ParticleShaderProperty::SetColor3(NiColorA aColor) {
    Color3 = aColor;
}

inline ParticleShaderProperty* ParticleShaderProperty::SetEmitterTypeRay(float afRayLength) {
    eEmitterType = EMIT_RAY;
    fRayLength = afRayLength;
    return this;
}

inline ParticleShaderProperty* ParticleShaderProperty::SetEmitterTypeCollision() {
    eEmitterType = EMIT_COLLISION;
    return this;
}

inline ParticleShaderProperty* ParticleShaderProperty::SetEmitterTypeGeometry() {
    eEmitterType = EMIT_GEOMETRY;
    return this;
}

inline NiTObjectArray<NiPointer<NiAVObject>>* ParticleShaderProperty::GetTargetArray() {
    return &TargetArray;
}
