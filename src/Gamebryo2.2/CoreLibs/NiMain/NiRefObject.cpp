#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

NiRefObject::NiRefObject() : m_uiRefCount(0) {
    NiAtomicIncrement(ms_uiObjects);
}

NiRefObject::~NiRefObject() {
    NiAtomicDecrement(ms_uiObjects);
}

void NiRefObject::DeleteThis() {
    delete this;
}

void NiRefObject::sub_4A7C20() {
    au_re_InterlockedDecrement_12_0(this, nullptr);
}

void au_re_InterlockedDecrement_12_0(NiRefObject* apThis, [[maybe_unused]] void* apEdx) {
    apThis->~NiRefObject();
}
