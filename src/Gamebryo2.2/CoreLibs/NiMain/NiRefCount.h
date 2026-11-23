#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

#include <atomic>

// Atomic reference counting of NiRefObject. The object is deleted through its virtual DeleteThis when the last
// reference goes away.
inline void NiIncRefCount(NiRefObject* apObject) {
    ++std::atomic_ref(apObject->m_uiRefCount);
}

inline void NiDecRefCount(NiRefObject* apObject) {
    if (--std::atomic_ref(apObject->m_uiRefCount) == 0) {
        apObject->DeleteThis();
    }
}
