#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"

// Depth test and depth write state; m_uFlags packs the test bit, the write bit and the test function.
class NiZBufferProperty : public NiProperty {
public:
    enum TestFunction : int32_t {
        TEST_ALWAYS = 0x0,
        TEST_LESS = 0x1,
        TEST_EQUAL = 0x2,
        TEST_LESSEQUAL = 0x3,
        TEST_GREATER = 0x4,
        TEST_NOTEQUAL = 0x5,
        TEST_GREATEREQUAL = 0x6,
        TEST_NEVER = 0x7,
        TEST_MAX_MODES = 0x8,
    };

    enum : int32_t {
        MAX_POS = 2,
    };

    enum Flags : uint16_t {
        ZBUFFER_TEST = 0x1,
        ZBUFFER_WRITE = 0x2,
        TEST_FUNCTION = 0x3C,
        TEST_FUNCTION_POS = 0x2,
    };

    NiZBufferProperty();
    ~NiZBufferProperty() override;

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
        return ZBUFFER;
    }

    bool IsEqualFast(const NiZBufferProperty& kProp) const;

    // The shared default z-buffer property.
    static NiZBufferProperty* GetDefault();
    static int32_t GetType() {
        return 0xB;
    }

    uint16_t m_uFlags;

    static NiRTTI ms_RTTI;
    static NiPointer<NiZBufferProperty> ms_spDefault;
};

inline NiZBufferProperty* NiZBufferProperty::GetDefault() {
    return ms_spDefault;
}

inline NiZBufferProperty::NiZBufferProperty() : m_uFlags(15) {}
