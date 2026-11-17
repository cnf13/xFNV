#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Reconstruction.hpp"

// Name and description shared by the shader library's descriptions.
class NiShaderDescBase : public NiRefObject {
public:
	~NiShaderDescBase() override;

	// Slot 2 (+0x8): NiSingleShaderMaterialLibrary::GetMaterialName (0xA8FC57) calls [vtable+0x8] on a shader
	// description for its name. The exe has no vtable of the class (no RTTI); the body is the SDK's.
	virtual const char* GetName() const { return m_pcName; }

	char* m_pcName;
	char* m_pcDesc;
};
