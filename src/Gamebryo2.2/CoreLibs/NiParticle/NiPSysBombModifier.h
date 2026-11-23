#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

class NiAVObject;
class NiCloningProcess;
class NiPSysData;
class NiStream;

// pushes particles away from a bomb object.
class NiPSysBombModifier : public NiPSysModifier {
public:
    // how the push falls off with distance.
    enum DecayType {
        NONE = 0x0,
        LINEAR = 0x1,
        EXPONENTIAL = 0x2,
    };

    // the shape of the push.
    enum SymmType {
        SPHERICAL = 0x0,
        CYLINDRICAL = 0x1,
        PLANAR = 0x2,
    };

    NiPSysBombModifier();
    ~NiPSysBombModifier() override;

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

    NiAVObject* m_pkBombObj;
    NiPoint3 m_kBombAxis;
    float m_fDecay;
    float m_fDeltaV;
    DecayType m_eDecayType;
    SymmType m_eSymmType;

    static NiRTTI ms_RTTI;
};
