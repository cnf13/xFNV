#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderAttributeDesc.h"

// A shader constant the application registers under a name for every shader to read: the key, the value's type and
// size, and where the value lives.
class NiGlobalConstantEntry : public NiRefObject {
public:
    NiGlobalConstantEntry(
        const char* pcKey,
        NiShaderAttributeDesc::AttributeType eType,
        uint32_t uiDataSize,
        const void* pvDataSource
    );
    ~NiGlobalConstantEntry() override;

    void SetData(uint32_t uiDataSize, const void* pvDataSource);

    NiFixedString m_kKey;
    NiShaderAttributeDesc::AttributeType m_eType;
    uint32_t m_uiDataSize;
    void* m_pvDataSource;
};
