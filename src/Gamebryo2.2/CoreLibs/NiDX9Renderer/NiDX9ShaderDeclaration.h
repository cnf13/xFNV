#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"

class NiDX9Renderer;

// The D3D9 shader declaration: builds the D3DVERTEXELEMENT9 array and the vertex declaration from the entries. PC only.
class NiDX9ShaderDeclaration : public NiD3DShaderDeclaration {
public:
    using TypeTable = D3DDECLTYPE[SPTYPE_COUNT];
    using MethodTable = D3DDECLMETHOD[SPTESS_COUNT];
    using UsageTable = D3DDECLUSAGE[SPUSAGE_COUNT];

    NiDX9ShaderDeclaration();
    ~NiDX9ShaderDeclaration() override;

    bool SetEntry(
        uint32_t auiEntry,
        uint32_t auiRegister,
        NiShaderDeclaration::ShaderParameter aeInput,
        NiShaderDeclaration::ShaderParameterType aeType,
        uint32_t auiStream
    ) override;
    bool RemoveEntry(uint32_t auiEntry, uint32_t auiStream) override;
    bool SetEntry(
        uint32_t auiStream,
        uint32_t auiEntry,
        uint32_t auiRegister,
        NiShaderDeclaration::ShaderParameter aeInput,
        NiShaderDeclaration::ShaderParameterType aeType,
        NiShaderDeclaration::ShaderParameterUsage aeUsage,
        uint32_t auiIndex,
        NiD3DShaderDeclaration::ShaderParameterTesselator aeTess
    ) override;
    bool ClearAllEntries(uint32_t auiStream) override;
    uint32_t GetVertexStride(uint32_t auiStream) override;
    void SetSoftwareVertexProcessing(bool abSoftwareVB) override {
        m_bSoftwareVB = abSoftwareVB;
    }
    bool GetSoftwareVertexProcessing() override {
        return m_bSoftwareVB;
    }
    NiVBChip* PackUntransformedVB(
        NiGeometryData* apData,
        NiSkinInstance* apSkin,
        NiSkinPartition::Partition* apPartition,
        uint16_t ausDirtyFlags,
        NiVBChip* apOldVBChip,
        uint32_t auiStream,
        void* apvLockedBuff,
        void*,
        void*
    ) override;
    bool GenerateVertexParameters(IDirect3DVertexDeclaration9** apDeclaration, uint32_t* apuiNumStreams) override;
    IDirect3DVertexDeclaration9* GetD3DDeclaration() override;

protected:
    // Writes the element for apEntry at m_uiCurrDeclIndex (stream auiStream, low 16 bits) and advances the index;
    // returns the entry type's size in bytes.
    uint32_t AddDeclarationEntry(ShaderRegisterEntry* apEntry, uint32_t auiStream);

public:
    static NiDX9ShaderDeclaration*
    Create(NiDX9Renderer* pkRenderer, uint32_t uiMaxStreamEntryCount, uint32_t uiStreamCount);

    D3DVERTEXELEMENT9* m_pkElements;
    IDirect3DVertexDeclaration9* m_hVertexDecl;
    bool m_bSoftwareVB;

    // The Direct3D declaration type, method and usage of each Gamebryo parameter type, tesselator and usage.
    static inline TypeTable ms_aeTypes{
        D3DDECLTYPE_FLOAT1,
        D3DDECLTYPE_FLOAT2,
        D3DDECLTYPE_FLOAT3,
        D3DDECLTYPE_FLOAT4,
        D3DDECLTYPE_D3DCOLOR,
        D3DDECLTYPE_UBYTE4,
        D3DDECLTYPE_SHORT2,
        D3DDECLTYPE_SHORT4,
        D3DDECLTYPE_UBYTE4N,
        D3DDECLTYPE_SHORT2N,
        D3DDECLTYPE_SHORT4N,
        D3DDECLTYPE_USHORT2N,
        D3DDECLTYPE_USHORT4N,
        D3DDECLTYPE_UDEC3,
        D3DDECLTYPE_DEC3N,
        D3DDECLTYPE_FLOAT16_2,
        D3DDECLTYPE_FLOAT16_4
    };
    static inline MethodTable ms_aeMethods{
        D3DDECLMETHOD_DEFAULT,
        D3DDECLMETHOD_PARTIALU,
        D3DDECLMETHOD_PARTIALV,
        D3DDECLMETHOD_CROSSUV,
        D3DDECLMETHOD_UV,
        D3DDECLMETHOD_LOOKUP,
        D3DDECLMETHOD_LOOKUPPRESAMPLED
    };
    static inline UsageTable ms_aeUsage{
        D3DDECLUSAGE_POSITION,
        D3DDECLUSAGE_BLENDWEIGHT,
        D3DDECLUSAGE_BLENDINDICES,
        D3DDECLUSAGE_NORMAL,
        D3DDECLUSAGE_PSIZE,
        D3DDECLUSAGE_TEXCOORD,
        D3DDECLUSAGE_TANGENT,
        D3DDECLUSAGE_BINORMAL,
        D3DDECLUSAGE_TESSFACTOR,
        D3DDECLUSAGE_POSITIONT,
        D3DDECLUSAGE_COLOR,
        D3DDECLUSAGE_FOG,
        D3DDECLUSAGE_DEPTH,
        D3DDECLUSAGE_SAMPLE
    };
};

NiSmartPointer(NiDX9ShaderDeclaration);
