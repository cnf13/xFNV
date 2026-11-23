#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiParticles.h"

class NiCloningProcess;
class NiCullingProcess;
class NiRenderer;
class NiStream;
struct NiUpdateData;

// old-style particles drawn as copies of meshes.
class NiParticleMeshes : public NiParticles {
public:
    NiParticleMeshes();
    ~NiParticleMeshes() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void SetSelectiveUpdateFlags(bool& bSelectiveUpdate, int32_t bSelectiveUpdateTransforms, bool& bRigid) override;
    void UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
    void UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
    void UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
    void UpdatePropertiesDownward(NiPropertyState* pkParentState) override;
    void UpdateWorldData(NiUpdateData& UpdateData) override;
    void OnVisible(NiCullingProcess& kCuller) override;
    void PurgeRendererData(NiRenderer* pkRenderer) override;

    void CopyMembers(NiParticleMeshes* pkDest, NiCloningProcess& kCloning);

    static NiObject* CreateObject();

    float m_fTime;
    bool m_bUpdateControllers;

    static NiRTTI ms_RTTI;
};
