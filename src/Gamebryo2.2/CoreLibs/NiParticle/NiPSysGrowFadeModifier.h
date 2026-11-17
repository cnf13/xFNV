#pragma once

#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

class NiCloningProcess;
class NiPSysData;
class NiStream;

// grows particles in at birth and fades them out at death.
class NiPSysGrowFadeModifier : public NiPSysModifier {
public:
    NiPSysGrowFadeModifier();
    ~NiPSysGrowFadeModifier() override;

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

    float m_fGrowTime;
    uint16_t m_usGrowGeneration;
    float m_fFadeTime;
    uint16_t m_usFadeGeneration;
    float m_fBaseScale;

    static NiRTTI ms_RTTI;
};
