#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

// a light; Bethesda keeps the light radius in the first float of the specular colour
class NiLight : public NiDynamicEffect {
public:
    NiLight();
    ~NiLight() override;

    const NiRTTI* GetRTTI() const override;
    void LoadBinary(NiStream& stream) override;
    void LinkObject(NiStream& stream) override;
    bool RegisterStreamables(NiStream& stream) override;
    void SaveBinary(NiStream& stream) override;
    bool IsEqual(NiObject* pObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

    void SetAmbientColor(const NiColor& color);
    void SetDiffuseColor(const NiColor& color);
    // Stores m_fDimmer and bumps the revision.
    void SetDimmer(float fDimmer);
    // Stores m_kSpec.
    void SetSpecularColor(const NiColor& color);

protected:
    // Copies the dimmer and the three colours after the effect's members; the renderer data stays the clone's own.
    void CopyMembers(NiLight* pDest, NiCloningProcess& kCloning);

public:
    float m_fDimmer;
    NiColor m_kAmb;
    NiColor m_kDiff;
    NiColor m_kSpec;
    void* m_pvRendererData;

    static NiRTTI ms_RTTI;
};
