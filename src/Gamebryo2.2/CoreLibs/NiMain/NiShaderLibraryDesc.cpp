#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderLibraryDesc.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDesc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

NiShaderDesc* NiShaderLibraryDesc::GetShaderDesc(uint32_t uiShader) const {
	return m_kShaderDescs.m_pBase[uiShader].m_pObject;
}

// A trivial accessor (declarations.md "Trivial accessors"): one word field read into eax.
uint32_t NiShaderLibraryDesc::GetShaderCount() const {
	return m_kShaderDescs.m_usESize;
}
