#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"

class NiPoint3;

// Reference-counted root of the scene graph and of most Gamebryo objects; deleted when the count drops to zero.
class NiRefObject : public NiMemObject {
public:
	// Count zero; counts the object in ms_uiObjects.
	NiRefObject();
	// Unregistered: the exe's two destructor bodies are the Destruct statics below. Goes at parity.
	virtual ~NiRefObject();

	// Deletes the object; called when the reference count drops to zero.
	virtual void DeleteThis();

	// Gamebryo's inline NiRefObject.inl members; the exe also keeps out-of-line copies (0x40F6E0, 0x401970,
	// registered in NiRefObject.cpp).
	void IncRefCount() { NiAtomicIncrement(m_uiRefCount); }

	void DecRefCount() {
		if (NiAtomicDecrement(m_uiRefCount) == 0) {
			DeleteThis();
		}
	}

	// m_uiRefCount. The exe calls it out of line.
	uint32_t GetRefCount() const { return m_uiRefCount; }
	void sub_4A7C20();

	uint32_t m_uiRefCount;

	// Number of live reference-counted objects.
	static inline uint32_t ms_uiObjects;
};

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
// Inferred type: apThis: NiRefObject* from 0x4a611a (parameter 0 of BSOcclusionPlane::DeletingDestruct)
void au_re_InterlockedDecrement_12_0(NiRefObject* apThis, [[maybe_unused]] void* apEdx);

// The vtable NiRefObject's constructor (0x4968B0) stores; a derived class whose constructor the exe inlined writes
// it first, before its own.
inline constexpr fnvd::Vtable NiRefObject_vftable{};
