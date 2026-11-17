#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "Reconstruction.hpp"

class TESForm;

// An array of forms (allocated with ourMalloc, 0x18 bytes).
class NiFormArray : public NiTLargePrimitiveArray<TESForm*> {
public:
	NiFormArray();
	~NiFormArray() override;

	// The form at aiIndex.
	TESForm* GetFormAt(uint32_t aiIndex);
	// Inserts apForm at aiIndex.
	void InsertFormAt(uint32_t aiIndex, TESForm* apForm);
	// The index of apForm, or 0xFFFFFFFF.
	uint32_t GetIndexForForm(TESForm* apForm);

	// The exe's deleting destructor (0x58B420) frees through ourFree.
	static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable NiFormArray_vftable{};
