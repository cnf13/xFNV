#include "Gamebryo2.2/CoreLibs/NiMain/NiFragmentShaderInstanceDescriptor.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShader.h"

NiRTTI NiFragmentShaderInstanceDescriptor::ms_RTTI(
    "NiFragmentShaderInstanceDescriptor",
    &NiShader::NiShaderInstanceDescriptor::ms_RTTI
);
