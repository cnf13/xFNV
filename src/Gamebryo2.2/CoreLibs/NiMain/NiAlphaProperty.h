#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"

NiSmartPointer(NiAlphaProperty);

// Blending and alpha-test state of a geometry object. m_uFlags packs the blend enable bit, the source and
// destination blend functions, the alpha test enable bit, the test function and the no-sorter bit.
class NiAlphaProperty : public NiProperty {
public:
    enum Flags : uint16_t {
        ALPHA_BLEND_MASK = 0x1,
        SRC_BLEND_POS = 0x1,
        DEST_BLEND_POS = 0x5,
        TEST_FUNC_POS = 0xA,
        SRC_BLEND_MASK = 0x1E,
        DEST_BLEND_MASK = 0x1E0,
        TEST_ENABLE_MASK = 0x200,
        TEST_FUNC_MASK = 0x1C00,
        ALPHA_NOSORTER_MASK = 0x2000,
    };

    enum AlphaFunction : int32_t {
        ALPHA_ONE = 0x0,
        ALPHA_ZERO = 0x1,
        ALPHA_SRCCOLOR = 0x2,
        ALPHA_INVSRCCOLOR = 0x3,
        ALPHA_DESTCOLOR = 0x4,
        ALPHA_INVDESTCOLOR = 0x5,
        ALPHA_SRCALPHA = 0x6,
        ALPHA_INVSRCALPHA = 0x7,
        ALPHA_DESTALPHA = 0x8,
        ALPHA_INVDESTALPHA = 0x9,
        ALPHA_SRCALPHASAT = 0xA,
        ALPHA_MAX_MODES = 0xB,
    };

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

    NiAlphaProperty();
    ~NiAlphaProperty() override;

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
        return ALPHA;
    }

    // Replaces the bits of m_uFlags under uMask with uVal << uPos.
    void SetField(uint16_t uVal, uint16_t uMask, uint16_t uPos);
    void SetBit(bool bVal, uint16_t uMask);
    void SetAlphaBlending(bool bAlpha);
    void SetSrcBlendMode(AlphaFunction eSrcBlend);
    void SetDestBlendMode(AlphaFunction eDestBlend);
    void SetAlphaTesting(bool bAlpha);
    void SetTestMode(TestFunction eTestFunc);
    void SetTestRef(uint8_t ucRef);
    void SetNoSorter(bool bNoSort);
    bool GetFlag(uint32_t auiMask) const;
    bool IsEnabled();

    // "<prefix> = <mode name>" in a fresh _NiMalloc block.
    static char* GetViewerString(const char* apcPrefix, AlphaFunction aeMode);
    static char* GetViewerString(const char* apcPrefix, TestFunction aeMode);
    static int32_t GetType() {
        return 0;
    }
    // Create and release ms_spDefault.
    static void _SDMInit();
    static void _SDMShutdown();
    static NiObject* CreateObject();
    static NiAlphaProperty::AlphaFunction GetNiAlphaFunctionFromD3D(uint32_t auiD3D);

    uint16_t m_uFlags;
    uint8_t m_ucAlphaTestRef;
    uint8_t padding1B;

    static NiRTTI ms_RTTI;
    // The static default alpha property, created by _SDMInit and released by _SDMShutdown.
    static NiPointer<NiAlphaProperty> ms_spDefault;
    static inline char abCloneUnique;
};
