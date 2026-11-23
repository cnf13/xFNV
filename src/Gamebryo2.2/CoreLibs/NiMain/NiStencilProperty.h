#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"

NiSmartPointer(NiStencilProperty);

// Stencil buffer state; m_uFlags packs the enable bit, the three actions, the draw mode and the test function.
class NiStencilProperty : public NiProperty {
public:
    enum TestFunc : int32_t {
        TEST_NEVER = 0x0,
        TEST_LESS = 0x1,
        TEST_EQUAL = 0x2,
        TEST_LESSEQUAL = 0x3,
        TEST_GREATER = 0x4,
        TEST_NOTEQUAL = 0x5,
        TEST_GREATEREQUAL = 0x6,
        TEST_ALWAYS = 0x7,
        TEST_MAX = 0x8,
    };

    enum Action : int32_t {
        ACTION_KEEP = 0x0,
        ACTION_ZERO = 0x1,
        ACTION_REPLACE = 0x2,
        ACTION_INCREMENT = 0x3,
        ACTION_DECREMENT = 0x4,
        ACTION_INVERT = 0x5,
        ACTION_MAX = 0x6,
    };

    enum DrawMode : int32_t {
        DRAW_CCW_OR_BOTH = 0x0,
        DRAW_CCW = 0x1,
        DRAW_CW = 0x2,
        DRAW_BOTH = 0x3,
        DRAW_MAX = 0x4,
    };

    enum Flags : uint16_t {
        ENABLE_MASK = 0x1,
        FAILACTION_MASK = 0xE,
        FAILACTION_POS = 0x1,
        ZFAILACTION_MASK = 0x70,
        ZFAILACTION_POS = 0x4,
        PASSACTION_MASK = 0x380,
        PASSACTION_POS = 0x7,
        DRAWMODE_MASK = 0xC00,
        DRAWMODE_POS = 0xA,
        TESTFUNC_MASK = 0xF000,
        TESTFUNC_POS = 0xC,
    };

    NiStencilProperty();
    ~NiStencilProperty() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pcStrings) override;
    int32_t Type() const override {
        return STENCIL;
    }

    // Sets the enable bit of m_uFlags.
    void SetStencilOn(bool bOn);
    // Sets or clears the bits uMask of m_uFlags.
    void SetBit(bool bVal, uint16_t uMask);
    // Stores uVal in the bits uMask of m_uFlags, shifted to uPos.
    void SetField(uint16_t uVal, uint16_t uMask, uint16_t uPos);
    // Each setter stores its field of m_uFlags.
    void SetStencilFunction(TestFunc eFunc);
    void SetStencilFailAction(Action eAct);
    void SetStencilPassZFailAction(Action eAct);
    void SetStencilPassAction(Action eAct);
    void SetDrawMode(DrawMode eDraw);
    void SetStencilReference(uint32_t uiRef);

    // "<prefix> = <name>" in a fresh _NiMalloc block.
    static char* GetViewerString(const char* apcPrefix, TestFunc aeFunc);
    static char* GetViewerString(const char* apcPrefix, Action aeAction);
    static char* GetViewerString(const char* apcPrefix, DrawMode aeMode);
    // Builds the engine's default stencil property into ms_spDefault.
    static void _SDMInit();
    // Releases ms_spDefault.
    static void _SDMShutdown();
    static NiObject* CreateObject();

    uint16_t m_uFlags;
    uint32_t m_uiRef;
    uint32_t m_uiMask;

    static NiRTTI ms_RTTI;
    static NiPointer<NiStencilProperty> ms_spDefault;
};
