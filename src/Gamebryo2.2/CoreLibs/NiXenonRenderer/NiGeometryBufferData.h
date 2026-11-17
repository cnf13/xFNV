#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"

class NiGeometryGroup;
class NiVBChip;

// Renderer-side data of a geometry; the renderer derives its buffer data from it.
class NiGeometryData::RendererData : public NiMemObject {
public:
    virtual ~RendererData();

    virtual bool ContainsVertexData(NiShaderDeclaration::ShaderParameter aeParameter) = 0;

    // Renderer data is freed through MemoryManager, not NiMemObject's operator.
    static void operator delete(void* apMemory);
};

// The device buffers of one geometry: vertex chips per stream, index buffer and draw counts.
class NiGeometryBufferData : public NiGeometryData::RendererData {
public:
    NiGeometryBufferData();
    ~NiGeometryBufferData() override;

    bool ContainsVertexData(NiShaderDeclaration::ShaderParameter eParameter) override;

    void SetStreamCount(uint32_t uiStreamCount);
    void SetFVF(uint32_t uiFVF);
    void SetVertexDeclaration(IDirect3DVertexDeclaration9* hDecl);
    bool IsVBChipValid();
    NiVBChip* GetVBChip(uint32_t uiIndex);
    // Releases the index buffer.
    void RemoveIB();

    uint32_t m_uiFlags;
    NiGeometryGroup* m_pkGeometryGroup;
    uint32_t m_uiFVF;
    IDirect3DVertexDeclaration9* m_hDeclaration;
    bool m_bSoftwareVP;
    uint32_t m_uiVertCount;
    uint32_t m_uiMaxVertCount;
    uint32_t m_uiStreamCount;
    uint32_t* m_puiVertexStride;
    NiVBChip** m_ppkVBChip;
    uint32_t m_uiIndexCount;
    uint32_t m_uiIBSize;
    IDirect3DIndexBuffer9* m_pkIB;
    uint32_t m_uiBaseVertexIndex;
    D3DPRIMITIVETYPE m_eType;
    uint32_t m_uiTriCount;
    uint32_t m_uiMaxTriCount;
    uint32_t m_uiNumArrays;
    const uint16_t* m_pusArrayLengths;
    const uint16_t* m_pusIndexArray;
};
