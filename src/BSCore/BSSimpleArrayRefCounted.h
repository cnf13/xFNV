#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

#include <stdint.h>

// A BSSimpleArray held through NiPointer: the array at +0 and the reference count's NiRefObject after it, at +0x10,
// where the smart pointers add and release references. Only the layout is declared.
template <class T, uint32_t uiGrowStep>
class BSSimpleArrayRefCounted : public BSSimpleArray<T, uiGrowStep>, public NiRefObject {
public:
	BSSimpleArrayRefCounted() {}

	// The object is freed through NiMemObject's sized delete; declared here, as the array base declares its own.
	static void operator delete(void* apMemory, size_t aSize) { NiMemObject::operator delete(apMemory, aSize); }
};
