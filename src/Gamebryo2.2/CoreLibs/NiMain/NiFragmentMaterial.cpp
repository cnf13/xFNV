#include "Gamebryo2.2/CoreLibs/NiMain/NiFragmentMaterial.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterial.h"

NiRTTI NiFragmentMaterial::ms_RTTI("NiFragmentMaterial", &NiMaterial::ms_RTTI);

// Gamebryo's switch: other types start from 0.
uint32_t NiFragmentMaterial::GetProgramVersion(NiGPUProgram::ProgramType eType) const {
    uint32_t uiVersion = 0;
    switch (eType) {
        case NiGPUProgram::PROGRAM_VERTEX: uiVersion |= m_usVertexVersion << 16; break;
        case NiGPUProgram::PROGRAM_PIXEL: uiVersion |= m_usPixelVersion << 16; break;
        default: break;
    }
    for (uint32_t ui = 0; ui < m_kLibraries.m_uiUsed; ++ui) {
        NiMaterialNodeLibrary* pkLib = m_kLibraries.m_pBase[ui].m_pObject;
        if (pkLib) {
            uiVersion += pkLib->GetVersion();
        }
    }
    return uiVersion;
}
