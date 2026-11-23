#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

// A property that selects how a geometry is shaded; the shader-based engine also stores the shader type here.
class NiShadeProperty : public NiProperty {
public:
    enum : int32_t {
        MAX_POS = 1,
    };

    enum Flags : uint16_t {
        SMOOTH_MASK = 0x1,
    };

    NiShadeProperty();
    ~NiShadeProperty() override;

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
    int32_t Type() const override;

    int32_t GetShaderPropertyType() const;
    // NiProperty's members, then the flags.
    void CopyMembers(NiShadeProperty* pDest, NiCloningProcess& kCloning);

    // The property type id, NiProperty::SHADE.
    static int32_t GetType();
    // Create and release ms_spDefault.
    static void _SDMInit();
    static void _SDMShutdown();
    static NiObject* CreateObject();

    uint16_t m_uFlags;
    int32_t iShaderPropertyType;

    static NiRTTI ms_RTTI;
    // The static default shade property, created by _SDMInit and released by _SDMShutdown.
    static NiPointer<NiShadeProperty> ms_spDefault;
};
