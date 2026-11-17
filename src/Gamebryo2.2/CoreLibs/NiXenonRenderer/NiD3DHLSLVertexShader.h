#pragma once

#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"

// A vertex shader compiled from HLSL: keeps its entry point, shader target and D3DX constant table.
class NiD3DHLSLVertexShader : public NiD3DVertexShader {
public:
    NiD3DHLSLVertexShader(NiDX9Renderer* pkRenderer);
    ~NiD3DHLSLVertexShader() override;

    const NiRTTI* GetRTTI() const override;
    uint32_t GetVariableCount() override;
    const char* GetVariableName(uint32_t uiIndex) override;
    bool SetShaderConstant(NiShaderConstantMapEntry* pkEntry, const void* pvDataSource, uint32_t __formal) override;
    bool SetShaderConstantArray(
        NiShaderConstantMapEntry* pkEntry,
        const void* pvDataSource,
        uint32_t uiNumEntries,
        uint32_t uiRegistersPerEntry,
        uint16_t* pusReorderArray
    ) override;
    void DestroyRendererData() override;
    void RecreateRendererData() override;
    virtual const char* GetEntryPoint() const {
        return m_pcEntryPoint;
    }
    virtual void SetEntryPoint(const char* pcEntryPoint);
    virtual const char* GetShaderTarget() const;
    virtual void SetShaderTarget(const char* pcShaderTarget);
    virtual ID3DXConstantTable* GetConstantTable() const;
    virtual void SetConstantTable(ID3DXConstantTable* pkTable);

    char* m_pcEntryPoint;
    char* m_pcShaderTarget;
    ID3DXConstantTable* m_pkConstantTable;

    static NiRTTI ms_RTTI;
};
