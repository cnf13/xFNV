#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"

// Per-object cache of the extra data a shader constant map's attribute entries resolved, by register, pass and program
// type. Only some of the members are declared.
class NiSCMExtraData : public NiExtraData {
public:
    // One cached extra data: the key is the register, with the pass (0xFF for global) shifted by m_uiPassShift.
    class SCMExtraDataEntry {
    public:
        uint32_t m_uiIndex;
        NiExtraData* m_pkExtraData;
    };

    NiSCMExtraData(const NiFixedString& kName, uint32_t uiNumVertexEntries, uint32_t uiNumPixelEntries);

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    // The cache is neither streamed nor cloned.
    bool IsStreamable() const override {
        return false;
    }
    bool IsCloneable() const override {
        return false;
    }

    // The next cached extra data for that slot, or null.
    NiExtraData* GetNextEntry(uint32_t uiConstantIndex, uint32_t uiPass, NiGPUProgram::ProgramType eType, bool bGlobal);

    // Caches an extra data for that slot.
    void AddEntry(
        uint32_t uiConstantIndex,
        uint32_t uiPass,
        NiGPUProgram::ProgramType eType,
        NiExtraData* pkExtraData,
        bool bGlobal
    );

    // Per program type (NiGPUProgram::ProgramType: vertex 0, pixel 2): entry counts, entries filled, entries.
    uint32_t m_auiNumEntries[3];
    uint32_t m_auiCurrentEntry[3];
    SCMExtraDataEntry* m_apkEntries[3];

    static inline uint32_t m_uiPassShift = 16;
    static NiRTTI ms_RTTI;
};
