#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialLibrary.h"

class NiShaderLibrary;
class NiSingleShaderMaterialLibrary;

// The material library of single-shader materials; walks the shader libraries' descriptions.
// sizeof: NiSingleShaderMaterialLibrary::_SDMInit allocates 0x14 bytes.
class NiSingleShaderMaterialLibrary : public NiMaterialLibrary {
public:
    NiSingleShaderMaterialLibrary();
    ~NiSingleShaderMaterialLibrary() override;

    NiMaterial* GetMaterial(const NiFixedString& kName) override;
    const char* GetMaterialName(uint32_t uiMaterial) override;
    uint32_t GetMaterialCount() const override;
    NiShaderDesc* GetFirstMaterialDesc() override;
    NiShaderDesc* GetNextMaterialDesc() override;

    static void _SDMInit();
    static void _SDMShutdown();

    NiShaderLibrary* m_pkLastLibrary;
    uint32_t m_uiLastDesc;

    static inline NiSingleShaderMaterialLibrary* ms_pkThis;
};
