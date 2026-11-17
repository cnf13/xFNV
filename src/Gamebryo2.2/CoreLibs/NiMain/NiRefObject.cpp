#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"

NiRefObject::NiRefObject() : m_uiRefCount(0) {
	NiAtomicIncrement(ms_uiObjects);
}

// Unregistered: the exe's destructor bodies are NiRefObject::Destruct and DestructCopy. Goes at parity.
NiRefObject::~NiRefObject() {
	NiAtomicDecrement(ms_uiObjects);
}

void NiRefObject::DeleteThis() {
	delete this;
}

void NiRefObject::sub_4A7C20() {
	au_re_InterlockedDecrement_12_0(this, nullptr);
}

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
void au_re_InterlockedDecrement_12_0(NiRefObject* apThis, [[maybe_unused]] void* apEdx) {
	apThis->~NiRefObject();
}
