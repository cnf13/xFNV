#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

// Name and description shared by the shader library's descriptions.
class NiShaderDescBase : public NiRefObject {
public:
	~NiShaderDescBase() override;

	virtual const char* GetName() const { return m_pcName; }

	char* m_pcName;
	char* m_pcDesc;
};
