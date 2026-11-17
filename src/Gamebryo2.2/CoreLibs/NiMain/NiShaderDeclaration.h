#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// The vertex layout a shader expects: which geometry data goes to which register, in which format.
class NiShaderDeclaration : public NiObject {
public:
    enum ShaderParameter : int32_t {
        SHADERPARAM_INVALID = -1,
        SHADERPARAM_NI_POSITION = 0,
        SHADERPARAM_NI_BLENDWEIGHT = 1,
        SHADERPARAM_NI_BLENDINDICES = 2,
        SHADERPARAM_NI_NORMAL = 3,
        SHADERPARAM_NI_COLOR = 4,
        SHADERPARAM_NI_TEXCOORD0 = 5,
        SHADERPARAM_NI_TEXCOORD1 = 6,
        SHADERPARAM_NI_TEXCOORD2 = 7,
        SHADERPARAM_NI_TEXCOORD3 = 8,
        SHADERPARAM_NI_TEXCOORD4 = 9,
        SHADERPARAM_NI_TEXCOORD5 = 10,
        SHADERPARAM_NI_TEXCOORD6 = 11,
        SHADERPARAM_NI_TEXCOORD7 = 12,
        SHADERPARAM_NI_TANGENT = 13,
        SHADERPARAM_NI_BINORMAL = 14,
        SHADERPARAM_DATA_ZERO = 15,
        SHADERPARAM_DATA_ONE = 16,
        SHADERPARAM_NI_COLOR1 = 17,
        SHADERPARAM_SKIP_COUNT_MASK = 0x40000000,
        SHADERPARAM_EXTRA_DATA_MASK = -0x7FFFFFFF - 1,
    };

    enum ShaderParameterType : int32_t {
        SPTYPE_FLOAT1 = 0,
        SPTYPE_FLOAT2 = 1,
        SPTYPE_FLOAT3 = 2,
        SPTYPE_FLOAT4 = 3,
        SPTYPE_UBYTECOLOR = 4,
        SPTYPE_UBYTE4 = 5,
        SPTYPE_SHORT2 = 6,
        SPTYPE_SHORT4 = 7,
        SPTYPE_NORMUBYTE4 = 8,
        SPTYPE_NORMSHORT2 = 9,
        SPTYPE_NORMSHORT4 = 10,
        SPTYPE_NORMUSHORT2 = 11,
        SPTYPE_NORMUSHORT4 = 12,
        SPTYPE_UDEC3 = 13,
        SPTYPE_NORMDEC3 = 14,
        SPTYPE_FLOAT16_2 = 15,
        SPTYPE_FLOAT16_4 = 16,
        SPTYPE_NONE = 17,
        SPTYPE_COUNT,
    };

    enum ShaderParameterUsage : int32_t {
        SPUSAGE_POSITION = 0,
        SPUSAGE_BLENDWEIGHT = 1,
        SPUSAGE_BLENDINDICES = 2,
        SPUSAGE_NORMAL = 3,
        SPUSAGE_PSIZE = 4,
        SPUSAGE_TEXCOORD = 5,
        SPUSAGE_TANGENT = 6,
        SPUSAGE_BINORMAL = 7,
        SPUSAGE_TESSFACTOR = 8,
        SPUSAGE_POSITIONT = 9,
        SPUSAGE_COLOR = 10,
        SPUSAGE_FOG = 11,
        SPUSAGE_DEPTH = 12,
        SPUSAGE_SAMPLE = 13,
        SPUSAGE_COUNT,
    };

    NiShaderDeclaration();
    ~NiShaderDeclaration() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    virtual bool SetEntry(
        uint32_t auiEntry,
        uint32_t auiRegister,
        NiShaderDeclaration::ShaderParameter aeInput,
        NiShaderDeclaration::ShaderParameterType aeType,
        uint32_t auiStream
    ) = 0;
    virtual bool RemoveEntry(uint32_t auiEntry, uint32_t auiStream) = 0;

    static NiRTTI ms_RTTI;
};

NiSmartPointer(NiShaderDeclaration);
