#include "BSHavok/bhkHeightFieldShape.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI bhkHeightFieldShape::ms_RTTI("bhkHeightFieldShape", &bhkShape::ms_RTTI);

// The exe's cleanup-only C++ EH frame is left out.
bhkHeightFieldShape::~bhkHeightFieldShape() {
	bhkSerializable::Kill();
	--iCount;
}

const NiRTTI* bhkHeightFieldShape::GetRTTI() const {
	return &bhkHeightFieldShape::ms_RTTI;
}

// The exe's EH frame (scope table 0xf30788) is cleanup-only and left out.
void bhkHeightFieldShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

	bhkShape::GetViewerStrings(apStrings);
	char* pcString = NiGetViewerString(ms_RTTI.m_pcName);
	apStrings->Add(pcString);
}
