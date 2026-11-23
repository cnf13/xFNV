#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"

class NiCamera;
class NiLODNode;

// Chooses the child of an NiLODNode to show.
class NiLODData : public NiObject {
public:
    NiLODData();
    ~NiLODData() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    virtual int32_t GetLODLevel(const NiCamera* apCamera, NiLODNode* apLOD) = 0;
    virtual void UpdateWorldData(NiLODNode* apLOD) = 0;
    virtual NiLODData* Duplicate() = 0;
    virtual int32_t GetLODIndex(int32_t aiLODLevel) = 0;

    static NiRTTI ms_RTTI;
};
