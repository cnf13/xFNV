#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"

// A buffer of null-terminated strings that old-format sequences index into.
class NiStringPalette : public NiObject {
public:
    NiStringPalette();
    ~NiStringPalette() override;

    const NiRTTI* GetRTTI() const override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;

    static NiObject* CreateObject();

    char* m_pcBuffer;
    uint32_t m_uiAllocatedSize;
    uint32_t m_uiEndOfBuffer;

    static NiRTTI ms_RTTI;
};
