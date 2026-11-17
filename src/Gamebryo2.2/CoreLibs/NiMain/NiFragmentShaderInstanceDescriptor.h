#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialDescriptor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShader.h"
#include "Reconstruction.hpp"

// The shader instance descriptor of a fragment material: the material descriptor the shader was built from.
// m_kMatDesc's string is released at +0x4 by the vector deleting destructor.
class NiFragmentShaderInstanceDescriptor : public NiShader::NiShaderInstanceDescriptor {
public:
	const NiRTTI* GetRTTI() const override;
	~NiFragmentShaderInstanceDescriptor() override;

	// Freed through MemoryManager::Deallocate (the exe's deleting destructor 0xE8D540), not NiMemObject's.
	static void operator delete(void* apMemory);

	NiMaterialDescriptor m_kMatDesc;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiFragmentShaderInstanceDescriptor_vftable{};
