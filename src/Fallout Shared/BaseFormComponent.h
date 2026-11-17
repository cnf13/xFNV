#pragma once

#include "BSCore/MemoryManager.h"
#include "Reconstruction.hpp"

// Root of TESForm and of the components forms are assembled from (TESFullName, TESModel, ...). It has no virtual
// destructor in the exe: its four slots are the per-component data hooks, all pure.
class BaseFormComponent {
public:
	BaseFormComponent();

	virtual void InitializeDataComponent() = 0;
	virtual void ClearDataComponent() = 0;
	virtual void CopyComponent(BaseFormComponent* apCopyFC) = 0;
	virtual bool CompareComponent(BaseFormComponent* apCompareFC) = 0;

	// Forms and their components are freed through ourFree (TESForm 0x483600, TESModel 0x489010). Declared once
	// here: a form holding several components finds this one static member through every base, unambiguously.
	static void operator delete(void* apMemory) { ourFree(apMemory); }
};
inline constexpr fnvd::Vtable BaseFormComponent_vftable{};
