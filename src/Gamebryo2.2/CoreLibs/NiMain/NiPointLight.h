#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

// a light that shines from a point with distance attenuation
class NiPointLight : public NiLight {
public:
    NiPointLight();
    ~NiPointLight() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& stream) override;
    void LinkObject(NiStream& stream) override;
    bool RegisterStreamables(NiStream& stream) override;
    void SaveBinary(NiStream& stream) override;
    bool IsEqual(NiObject* pObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;
    void UpdateWorldData(NiUpdateData& UpdateData) override;

    void SetConstantAttenuation(float fC);
    void SetLinearAttenuation(float fL);
    void SetQuadraticAttenuation(float fQ);

protected:
    void CopyMembers(NiPointLight* pDest, NiCloningProcess& kCloning);

public:
    static NiObject* CreateObject();

    float m_fAtten0;
    float m_fAtten1;
    float m_fAtten2;

    static NiRTTI ms_RTTI;
};

NiSmartPointer(NiPointLight);
