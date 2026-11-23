#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFragmentShaderInstanceDescriptor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialDescriptor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShader.h"

const NiRTTI* NiFragmentShaderInstanceDescriptor::GetRTTI() const {
    return &NiFragmentShaderInstanceDescriptor::ms_RTTI;
}

// m_kMatDesc's identifier releases its own handle.
NiFragmentShaderInstanceDescriptor::~NiFragmentShaderInstanceDescriptor() {}

void NiFragmentShaderInstanceDescriptor::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}
