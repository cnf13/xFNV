#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"

NiSmartPointer(NiMaterialProperty);

// Lighting material of a geometry object; m_uiRevID counts changes so renderers know when to re-upload it.
class NiMaterialProperty : public NiProperty {
public:
    NiMaterialProperty();
    ~NiMaterialProperty() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& stream) override;
    void LinkObject(NiStream& stream) override;
    bool RegisterStreamables(NiStream& stream) override;
    void SaveBinary(NiStream& stream) override;
    bool IsEqual(NiObject* pObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;
    int32_t Type() const override {
        return MATERIAL;
    }

    // The engine passes an argument that the function ignores.
    void IncRevID(int32_t aiUnused);
    void SetAlpha(float c);
    void SetEmittance(const NiColor& c);
    // Empty in this build (the property has no ambient or diffuse colour): the engine's
    // shared empty stub.
    void SetAmbientColor(const NiColor& c);
    // Empty in this build, as SetAmbientColor.
    void SetDiffuseColor(const NiColor& c);
    void SetShineness(float c);
    void SetSpecularColor(const NiColor& c) {
        m_spec = c;
        ++m_uiRevID;
    }
    // The external emittance source when one is set, else m_emit.
    const NiColor& GetEmittance() const {
        return m_external_emit ? *m_external_emit : m_emit;
    }
    void SetEmittanceMult(float fMult) {
        m_fEmitMult = fMult;
        ++m_uiRevID;
    }
    void CopyMembers(NiMaterialProperty* pDest, NiCloningProcess& kCloning);

    // Create and release ms_spDefault.
    static void _SDMInit();
    static void _SDMShutdown();
    // The property type id of a material property (MATERIAL).
    static int32_t GetType();
    static NiObject* CreateObject();

    int32_t m_iIndex;
    NiColor m_spec;
    NiColor m_emit;
    NiColor* m_external_emit;
    float m_fShine;
    float m_fAlpha;
    float m_fEmitMult;
    uint32_t m_uiRevID;
    void* m_pvRendererData;

    static NiRTTI ms_RTTI;
    static NiPointer<NiMaterialProperty> ms_spDefault;
    // The index the next material gets (the constructors take it with an interlocked increment).
    static inline int32_t ms_iNextIndex = 1;
};
