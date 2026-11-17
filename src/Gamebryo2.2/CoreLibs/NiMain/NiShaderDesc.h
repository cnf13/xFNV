#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderAttributeDesc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderRequirementDesc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"

// A shader's description: the requirement descriptions of the implementations it can be built as, and the attributes it
// takes. Only the members are declared, not the getters and setters.
class NiShaderDesc : public NiShaderRequirementDesc {
public:
	uint32_t m_uiImplementations;
	NiTPrimitiveArray<NiShaderRequirementDesc*> m_kImplementationDescs;
	// The list position of the attribute walk: Gamebryo's NiTListIterator, a void*.
	void* m_kAttributePos;
	NiTPointerList<NiPointer<NiShaderAttributeDesc>> m_kAttributeList;
};
