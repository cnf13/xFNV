#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiParticlesData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiNode;
class NiStream;

// particle data of NiParticleMeshes: the node holding the meshes.
class NiParticleMeshesData : public NiParticlesData {
public:
    NiParticleMeshesData();
    ~NiParticleMeshesData() override;

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
    void RemoveParticle(int32_t aiParticle) override;

    void UpdateParticles();

    static NiObject* CreateObject();

    NiPointer<NiNode> m_spParticleMeshes;
    bool m_bNeedsPropertyEffectUpdate;

    static NiRTTI ms_RTTI;
};
