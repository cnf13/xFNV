#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLODData.h"

// LOD by the screen proportion a bound covers: one proportion threshold per level.
class NiScreenLODData : public NiLODData {
public:
    NiScreenLODData();
    ~NiScreenLODData() override;

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
    int32_t GetLODLevel(const NiCamera* pkCamera, NiLODNode* pkLOD) override;
    void UpdateWorldData(NiLODNode* pkLOD) override;
    NiLODData* Duplicate() override;
    int32_t GetLODIndex(int32_t iLODLevel) override;

    // Resizes the proportion array, keeping the leading proportions.
    void SetNumProportions(uint32_t uiNumProportions);
    // Merges the children's world bounds and takes the result into the node's model space.
    void AutoSetBound(NiLODNode* pkLOD);

    static NiObject* CreateObject();

    NiBound m_kBound;
    NiBound m_kWorldBound;
    uint32_t m_uiNumProportions;
    float* m_pfProportions;

    static NiRTTI ms_RTTI;
};
