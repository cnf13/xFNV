#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"

// How vertex colors feed the lighting (source and lighting modes in m_uFlags).
class NiVertexColorProperty : public NiProperty {
public:
    enum SourceVertexMode : int32_t {
        SOURCE_IGNORE = 0x0,
        SOURCE_EMISSIVE = 0x1,
        SOURCE_AMB_DIFF = 0x2,
        SOURCE_NUM_MODES = 0x3,
    };

    enum LightingMode : int32_t {
        LIGHTING_E = 0x0,
        LIGHTING_E_A_D = 0x1,
        LIGHTING_NUM_MODES = 0x2,
    };

    enum : int32_t {
        MAX_POS = 3,
    };

    enum Flags : uint16_t {
        LIGHTINGMODE_MASK = 0x8,
        LIGHTINGMODE_POS = 0x3,
        SOURCEMODE_MASK = 0x30,
        SOURCEMODE_POS = 0x4,
    };

    NiVertexColorProperty();
    ~NiVertexColorProperty() override;

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
        return VERTEX_COLOR;
    }

    // Whether kProp has the same flags (Gamebryo's inline IsEqualFast).
    bool IsEqualFast(const NiVertexColorProperty& kProp) const;

    // The shared default property.
    static NiVertexColorProperty* GetDefault();
    // Allocates and constructs a property with the default flags.
    static NiObject* CreateObject();

    // "<prefix> = <mode name>" in a fresh _NiMalloc block.
    static char* GetViewerString(const char* apcPrefix, SourceVertexMode aeMode);
    static char* GetViewerString(const char* apcPrefix, LightingMode aeMode);
    static int32_t GetType() {
        return 0xA;
    }

    uint16_t m_uFlags;

    static NiRTTI ms_RTTI;
    static NiPointer<NiVertexColorProperty> ms_spDefault;
};
