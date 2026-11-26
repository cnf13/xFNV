#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"

class NiSkinPartition;

// The bones of a skinned mesh: per bone the skin-to-bone transform, a bound and the vertices it weights.
class NiSkinData : public NiObject {
public:
    class BoneVertData {
    public:
        // Only the vertex index is cleared; the weight is left as allocated.
        BoneVertData() : m_usVert(0) {}

        uint16_t m_usVert;
        float m_fWeight;
    };

    class BoneData {
    public:
        // No vertex data; the transform and bound are left as allocated.
        BoneData();

        NiTransform m_kSkinToBone;
        NiBound m_kBound;
        BoneVertData* m_pkBoneVertData;
        uint16_t m_usVerts;
    };

    NiSkinData();
    NiSkinData(
        uint32_t uiBones,
        NiSkinData::BoneData* pkBoneData,
        const NiTransform& kRootParentToSkin,
        const NiPoint3* pkVerts
    );
    ~NiSkinData() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;

    // Frees every bone's vertex weights, keeping the bone array.
    void FreeBoneVertData();
    uint32_t GetBoneCount() const;
    NiSkinData::BoneData* GetBoneData() const;
    void ComputeBoneBounds(const NiPoint3* pkVerts);

    static NiObject* CreateObject();

    // Only files older than 10.1.0.101 link it; NiSkinInstance::PostLinkObject then takes it over.
    NiPointer<NiSkinPartition> m_spSkinPartition;
    NiTransform m_kRootParentToSkin;
    BoneData* m_pkBoneData;
    uint32_t m_uiBones;

    static NiRTTI ms_RTTI;
};

inline uint32_t NiSkinData::GetBoneCount() const {
    return m_uiBones;
}

inline NiSkinData::BoneData* NiSkinData::GetBoneData() const {
    return m_pkBoneData;
}
