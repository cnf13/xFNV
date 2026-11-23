#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

class NiAVObject;
class NiCloningProcess;
class NiPSysData;
class NiStream;

// slows particles down.
class NiPSysDragModifier : public NiPSysModifier {
public:
    NiPSysDragModifier();
    ~NiPSysDragModifier() override;

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
    void Update(float fTime, NiPSysData* pkData) override;

    static NiObject* CreateObject();

    NiAVObject* m_pkDragObj;
    NiPoint3 m_kDragAxis;
    float m_fPercentage;
    float m_fRange;
    float m_fFalloff;

    static NiRTTI ms_RTTI;
};
