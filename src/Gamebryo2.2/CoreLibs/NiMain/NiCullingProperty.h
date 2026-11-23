#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"

// Which faces of a geometry object are drawn.
class NiCullingProperty : public NiProperty {
public:
    enum Flags : uint16_t {
        CULLED_MASK = 0x1,
    };

    NiCullingProperty();
    ~NiCullingProperty() override;

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
    int32_t Type() const override {
        return CULLING;
    }

    // Sets ms_spDefault to a new NiCullingProperty. NiMainSDM::Init calls it.
    static void _SDMInit();
    // Releases ms_spDefault.
    static void _SDMShutdown();
    static NiObject* CreateObject();

    uint16_t m_uFlags;

    static NiRTTI ms_RTTI;
    static NiPointer<NiCullingProperty> ms_spDefault;
};
