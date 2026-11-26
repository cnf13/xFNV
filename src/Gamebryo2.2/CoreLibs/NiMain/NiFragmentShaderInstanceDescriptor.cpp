#include "Gamebryo2.2/CoreLibs/NiMain/NiFragmentShaderInstanceDescriptor.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShader.h"

NiRTTI NiFragmentShaderInstanceDescriptor::ms_RTTI(
    "NiFragmentShaderInstanceDescriptor",
    &NiShader::NiShaderInstanceDescriptor::ms_RTTI
);

// m_kMatDesc's identifier releases its own handle.
NiFragmentShaderInstanceDescriptor::~NiFragmentShaderInstanceDescriptor() {}

void NiFragmentShaderInstanceDescriptor::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}
