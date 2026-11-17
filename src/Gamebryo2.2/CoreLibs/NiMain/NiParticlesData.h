#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

class NiStream;

// geometry data of a particle system: per-particle radii, sizes, rotations and sub-texture indices next to the
// vertex data.
class NiParticlesData : public NiGeometryData {
public:
    NiParticlesData();
    ~NiParticlesData() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void SetActiveVertexCount(uint16_t usActive) override;
    uint16_t GetActiveVertexCount() const override {
        return m_usActive;
    }
    void CalculateNormals() override {}
    virtual void RemoveParticle(int32_t aiParticle);

    // Hands new per-particle radii to the data, freeing the old array.
    void ReplaceRadii(float* pfRadii);
    // Hands new per-particle sizes to the data, freeing the old array.
    void ReplaceSizes(float* pfSizes);
    // Hands new per-particle rotation angles to the data, freeing the old array.
    void ReplaceRotationAngles(float* pfRotationAngles);
    // Hands new per-particle rotations to the data, freeing the old array.
    void ReplaceRotations(NiQuaternion* pkRotations);
    // Hands new per-particle sub-texture offsets to the data, freeing the old array.
    void ReplaceSubTextureOffsets(NiColorA* pkSubTextureOffsets, uint8_t ucSubTextureOffsetCount);
    // Hands new per-particle sub-texture indices to the data, freeing the old array.
    void ReplaceTextureIndices(uint8_t* pucTextureIndices);
    void ReplaceRotationAxes(NiPoint3* pkRotationAxes);

    static NiObject* CreateObject();
    // Loader for "NiRotatingParticlesData".
    static NiObject* CreateObject_00A96660();

    bool m_bIsOldRotatingParticlesObject;
    uint8_t padding41[3];
    uint16_t* m_ausRenderOrder;
    float* m_afDepths;
    float* m_pfRadii;
    uint16_t m_usActive;
    uint8_t padding52[2];
    float* m_pfSizes;
    NiQuaternion* m_pkRotations;
    float* m_pfRotationAngles;
    NiPoint3* m_pkRotationAxes;
    uint8_t* m_pucTextureIndices;
    NiColorA* m_pkSubTextureOffsets;
    uint8_t m_ucSubTextureOffsetCount;
    uint8_t padding6D[3];

    // The engine's cap on sub-textures per particle.
    static constexpr uint8_t MAX_SUBTEXTURE_COUNT = 16;

    static NiRTTI ms_RTTI;
};
