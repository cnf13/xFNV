#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

// A versioned library of material nodes a fragment material builds its shaders from.
class NiMaterialNodeLibrary : public NiRefObject {
public:
    ~NiMaterialNodeLibrary() override;

    uint16_t GetVersion() const {
        return m_usVersion;
    }

    NiTObjectArray<NiPointer<NiMaterialNode>> m_kNodes;
    uint16_t m_usVersion;
};
