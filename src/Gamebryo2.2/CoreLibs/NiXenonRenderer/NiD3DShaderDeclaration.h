#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"

#include <stdint.h>

class NiDX9Renderer;
class NiDX9VertexBufferManager;
class NiGeometryData;
class NiSkinInstance;
class NiVBChip;

// A shader declaration for D3D: per stream, the register entries and how the geometry data is packed into the
// vertex buffer.
class NiD3DShaderDeclaration : public NiShaderDeclaration {
public:
    enum ShaderParameterTesselator : int32_t {
        SPTESS_DEFAULT,
        SPTESS_PARTIALU,
        SPTESS_PARTIALV,
        SPTESS_CROSSUV,
        SPTESS_UV,
        SPTESS_LOOKUP,
        SPTESS_LOOKUPPRESAMPLED,
        SPTESS_COUNT,
    };

    // One register of a stream: the geometry data it takes and its packed format.
    class ShaderRegisterEntry : public NiMemObject {
    public:
        ShaderRegisterEntry();

        uint32_t m_uiRegister;
        uint32_t m_uiPackingOffset;
        ShaderParameter m_eInput;
        ShaderParameterType m_eType;
        ShaderParameterTesselator m_eTesselator;
        ShaderParameterUsage m_eUsage;
        uint32_t m_uiUsageIndex;
    };

    // One vertex stream: its register entries and vertex stride.
    class ShaderRegisterStream : public NiMemObject {
    public:
        ShaderRegisterStream();

        bool m_bValid;
        uint32_t m_uiEntryCount;
        ShaderRegisterEntry* m_pkEntries;
        uint32_t m_uiStride;
    };

    using TypeSizeTable = uint32_t[SPTYPE_COUNT];

    // One packing call: m_usCount vertices of m_pvInData (picked through m_pusVertexMap when it is set) converted
    // into m_pvOutData.
    class PackingParameters {
    public:
        ShaderParameterType m_ePackType;
        uint32_t m_uiDataType;
        uint16_t m_usCount;
        uint16_t* m_pusVertexMap;
        void* m_pvInData;
        uint32_t m_uiInSize;
        uint32_t m_uiInStride;
        uint32_t m_uiOutSize;
        uint32_t m_uiOutStride;
        // PC-only fields: PackEntry stores its last argument and the raw low byte of the one before (a byte store, not
        // a normalised bool).
        void* m_pvPCOnly24;
        uint8_t m_bPCOnly28;
        void* m_pvOutData;
    };

    // Packs kParams.m_usCount vertices and returns the bytes packed.
    using PackFunction = uint32_t (*)(PackingParameters& kParams);

    // The packing functions of one output type, indexed by NiAdditionalGeometryData type.
    class NiPackerEntry : public NiMemObject {
    public:
        NiPackerEntry();
        ~NiPackerEntry() {
            m_kPackFunctions.RemoveAll();
        }

        uint32_t m_uiShaderParameterType;
        NiTPrimitiveArray<PackFunction> m_kPackFunctions;
    };

    NiD3DShaderDeclaration();
    ~NiD3DShaderDeclaration() override;

    virtual bool SetEntry(
        uint32_t auiStream,
        uint32_t auiEntry,
        uint32_t auiRegister,
        NiShaderDeclaration::ShaderParameter aeInput,
        NiShaderDeclaration::ShaderParameterType aeType,
        NiShaderDeclaration::ShaderParameterUsage aeUsage,
        uint32_t auiIndex,
        NiD3DShaderDeclaration::ShaderParameterTesselator aeTess
    ) = 0;
    virtual bool SetConstantData(uint32_t, uint32_t, float*) {
        return true;
    }
    virtual bool ClearAllEntries(uint32_t auiStream) = 0;
    virtual uint32_t GetVertexStride(uint32_t auiStream) = 0;
    virtual void SetSoftwareVertexProcessing(bool) {}
    virtual bool GetSoftwareVertexProcessing() {
        return false;
    }
    // the last two parameters are unused
    virtual NiVBChip* PackUntransformedVB(
        NiGeometryData* apData,
        NiSkinInstance* apSkin,
        NiSkinPartition::Partition* apPartition,
        uint16_t ausDirtyFlags,
        NiVBChip* apOldVBChip,
        uint32_t auiStream,
        void* apvLockedBuff,
        void*,
        void*
    ) = 0;
    virtual bool GenerateVertexParameters(IDirect3DVertexDeclaration9** apDeclaration, uint32_t* apuiNumStreams) = 0;
    virtual IDirect3DVertexDeclaration9* GetD3DDeclaration() {
        return nullptr;
    }
    using NiShaderDeclaration::SetEntry;

    bool Initialize(NiDX9Renderer* pkRenderer, uint32_t uiMaxStreamEntryCount, uint32_t uiStreamCount);
    // Packs one entry's data of ausCount vertices into apucData with the given stride; the last two parameters are
    // passed through. Returns the packer's result (bytes packed), 0 when nothing is packed, which includes the inputs
    // its switch does not set up (SHADERPARAM_DATA_ZERO and above, a blend weight partition with 0 or more than 4 bones
    // per vertex).
    uint32_t PackEntry(
        uint16_t ausCount,
        NiGeometryData* apData,
        NiSkinInstance* apSkin,
        NiSkinPartition::Partition* apPartition,
        ShaderRegisterEntry* apEntry,
        uint32_t auiStride,
        uint8_t* apucData,
        void* apv8,
        void* apv9
    );

    // Bytes per vertex of aeType; 0 for SPTYPE_COUNT and above.
    static uint32_t GetTypeSize(ShaderParameterType eType) {
        if (eType >= SPTYPE_COUNT) {
            return 0;
        }
        return ms_auiTypeSizes[eType];
    }

    // Allocates the packer entry array, then fills the packing functions.
    static void _SDMInit();
    // Deletes the packer entries and the packer entry array.
    static void _SDMShutdown();
    // Fills the type sizes and the packing functions of the packer entries, once.
    static bool InitializePackingFunctions();

    // Packer of an unsupported type pair: packs nothing and returns 0.
    static uint32_t Pack_IllegalCombination(PackingParameters& kParams);
    static uint32_t Pack_UnsignedLongToUByteColor(PackingParameters& kParams);
    static uint32_t Pack_ShortToUByteColor(PackingParameters& kParams);
    static uint32_t Pack_UnsignedShortToUByteColor(PackingParameters& kParams);
    static uint32_t Pack_FloatToShort(PackingParameters& kParams);
    static uint32_t Pack_LongToShort(PackingParameters& kParams);
    static uint32_t Pack_UnsignedLongToShort(PackingParameters& kParams);
    static uint32_t Pack_ShortToShort(PackingParameters& kParams);
    static uint32_t Pack_UnsignedShortToShort(PackingParameters& kParams);
    static uint32_t Pack_ByteToShort(PackingParameters& kParams);
    static uint32_t Pack_UnsignedByteToShort(PackingParameters& kParams);
    static uint32_t Pack_FloatToUnsignedByte(PackingParameters& kParams);
    static uint32_t Pack_LongToUnsignedByte(PackingParameters& kParams);
    static uint32_t Pack_UnsignedLongToUnsignedByte(PackingParameters& kParams);
    static uint32_t Pack_ShortToUnsignedByte(PackingParameters& kParams);
    static uint32_t Pack_UnsignedShortToUnsignedByte(PackingParameters& kParams);
    static uint32_t Pack_ByteToUnsignedByte(PackingParameters& kParams);
    static uint32_t Pack_UnsignedByteToUnsignedByte(PackingParameters& kParams);
    static uint32_t Pack_Float_DropIns(PackingParameters& kParams);
    static uint32_t Pack_Float_ExtraOutsToZero(PackingParameters& kParams);
    static uint32_t Pack_FloatToFloat(PackingParameters& kParams);
    static uint32_t Pack_LongToFloat(PackingParameters& kParams);
    static uint32_t Pack_UnsignedLongToFloat(PackingParameters& kParams);
    static uint32_t Pack_ShortToFloat(PackingParameters& kParams);
    static uint32_t Pack_UnsignedShortToFloat(PackingParameters& kParams);
    static uint32_t Pack_ByteToFloat(PackingParameters& kParams);
    static uint32_t Pack_UnsignedByteToFloat(PackingParameters& kParams);
    static uint32_t Pack_FloatToUByteColor(PackingParameters& kParams);
    static uint32_t Pack_ByteToUByteColor(PackingParameters& kParams);
    static uint32_t Pack_UnsignedByteToUByteColor(PackingParameters& kParams);
    // Sets the default packer of one output type and one NiAdditionalGeometryData type.
    static void SetDefaultPackingFunction(NiShaderDeclaration::ShaderParameterType eType, uint32_t uiAGDType);

    NiDX9Renderer* m_pkRenderer;
    NiDX9VertexBufferManager* m_pkVBManager;
    IDirect3DDevice9* m_pkD3DDevice;
    uint32_t m_uiDeclSize;
    uint32_t m_uiCurrDeclIndex;
    uint32_t m_uiMaxStreamEntryCount;
    uint32_t m_uiStreamCount;
    ShaderRegisterStream* m_pkStreamEntries;
    bool m_bModified;

    // Bytes per vertex of each parameter type.
    static inline TypeSizeTable ms_auiTypeSizes{};
    // One packer entry per output type (ShaderParameterType).
    static inline NiTPrimitiveArray<NiPackerEntry*>* ms_pkPackFunctions;
};

NiSmartPointer(NiD3DShaderDeclaration);
