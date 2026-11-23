#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

class NiCloningProcess;
class NiPSysData;
class NiStream;

// pushes particles along the wind.
class BSWindModifier : public NiPSysModifier {
public:
    BSWindModifier();
    ~BSWindModifier() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void Update(float afTime, NiPSysData* apData) override;

    void CopyMembers(BSWindModifier* apDest, NiCloningProcess& arCloneData);

    // Sets the wind every modifier applies: afSpeed (scaled by 100) along y, turned by afAngle about z.
    static void SetWind(float afSpeed, float afAngle);
    static NiObject* CreateObject();

    float fStrength;

    static NiRTTI ms_RTTI;
    // the wind direction shared by every wind modifier.
    static NiPoint3 WindDir;
};
