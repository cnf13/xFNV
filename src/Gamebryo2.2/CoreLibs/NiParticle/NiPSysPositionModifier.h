#pragma once

#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

class NiCloningProcess;
class NiPSysData;
class NiStream;

// moves particles by their velocity.
class NiPSysPositionModifier : public NiPSysModifier {
public:
    NiPSysPositionModifier();
    ~NiPSysPositionModifier() override;

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
    void Update(float fTime, NiPSysData* pkData) override;

    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
};
