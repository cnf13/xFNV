#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"

class NiAVObject;
class NiCloningProcess;
class NiNode;
class NiRenderer;
class NiStream;

// particle data of a NiMeshParticleSystem: the meshes and their pools.
class NiMeshPSysData : public NiPSysData {
public:
    NiMeshPSysData();
    ~NiMeshPSysData() override;

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
    void ProcessClone(NiCloningProcess& kCloning) override;
    void SetActiveVertexCount(uint16_t usActive) override;
    void RemoveParticle(int32_t aiParticle) override;
    void ResolveAddedParticles() override;

    // Purges the renderer data of every pooled mesh.
    void PurgeRendererDataForPools(NiRenderer* pkRenderer);
    // Rebuilds the pools, one per generation.
    void SetNumGenerations(uint32_t uiNumGenerations);
    // Fills generation uiGeneration's pool with clones of pkOriginal.
    void FillPool(uint32_t uiGeneration, NiAVObject* pkOriginal);
    // Takes the last mesh of generation uiGeneration's pool, or null when the pool is empty or missing.
    void RemoveFromPool(uint32_t uiGeneration, NiPointer<NiAVObject>& aspAVObject);
    // Returns spObject to generation uiGeneration's pool unless the pool is full or missing.
    void AddToPool(uint32_t uiGeneration, NiPointer<NiAVObject> spObject);
    // Copies the meshes (cloned) and the fill-pools flag into pkDest after the base's members.
    void CopyMembers(NiMeshPSysData* pkDest, NiCloningProcess& kCloning);

    static NiObject* CreateObject();

    NiPointer<NiNode> m_spParticleMeshes;
    bool m_bFillPoolsOnLoad;
    uint32_t m_uiDefaultPoolSize;
    NiTPrimitiveArray<NiTObjectArray<NiPointer<NiAVObject>>*> m_kPools;

    static NiRTTI ms_RTTI;
};
