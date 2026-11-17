#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDescBase.h"

// What a shader implementation requires: the pixel and vertex shader model versions it may be built for, the user
// version range, its platform flags, how many bones a vertex shader may skin, and its binormal/tangent method. Only the
// members are declared; the virtual getters and setters are not.
class NiShaderRequirementDesc : public NiShaderDescBase {
public:
    // The binormal/tangent UV source that leaves the choice to the shader (USHRT_MAX).
    enum {
        BINORMALTANGENTUVSOURCEDEFAULT = 0xFFFF,
    };

    uint32_t m_uiMinPixelShaderVersion;
    uint32_t m_uiMaxPixelShaderVersion;
    uint32_t m_uiMinVertexShaderVersion;
    uint32_t m_uiMaxVertexShaderVersion;
    uint32_t m_uiMinUserVersion;
    uint32_t m_uiMaxUserVersion;
    uint32_t m_uiPlatformFlags;
    uint32_t m_uiBonesPerPartition;
    NiGeometryData::DataFlags m_eBinormalTangentMethod;
    uint16_t m_usNBTSourceUV;
    bool m_bSoftwareVPAcceptable;
    bool m_bSoftwareVPRequired;
};
