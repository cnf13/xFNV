#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Reconstruction.hpp"

// Atomic reference counting of NiRefObject, as the engine's inlined smart-pointer code does it. The object is
// deleted through its virtual DeleteThis when the last reference goes away.
inline void NiIncRefCount(NiRefObject* apObject) {
	__atomic_fetch_add(&apObject->m_uiRefCount, 1, __ATOMIC_SEQ_CST);
}

inline void NiDecRefCount(NiRefObject* apObject) {
	if (__atomic_sub_fetch(&apObject->m_uiRefCount, 1, __ATOMIC_SEQ_CST) == 0) {
		apObject->DeleteThis();
	}
}
