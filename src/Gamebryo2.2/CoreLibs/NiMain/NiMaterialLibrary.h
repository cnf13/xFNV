#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDesc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"

class NiMaterial;

// A named source of materials; the constructor adds every library to a global set.
class NiMaterialLibrary : public NiRefObject {
public:
    NiMaterialLibrary(const NiFixedString& kName);
    ~NiMaterialLibrary() override;

    virtual NiMaterial* GetMaterial(const NiFixedString& kName);
    virtual const char* GetMaterialName(uint32_t uiMaterial);
    virtual uint32_t GetMaterialCount() const;
    virtual NiShaderDesc* GetFirstMaterialDesc() = 0;
    virtual NiShaderDesc* GetNextMaterialDesc() = 0;

    // Empties the set, then frees its storage.
    static void _SDMShutdown();

    NiFixedString m_kName;

    // The material libraries the engine holds.
    static NiTPrimitiveSet<NiMaterialLibrary*> ms_kMaterialLibraries;
};
