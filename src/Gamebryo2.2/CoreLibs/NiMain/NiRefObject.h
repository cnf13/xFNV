#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

class NiPoint3;

// Reference-counted root of the scene graph and of most Gamebryo objects; deleted when the count drops to zero.
class NiRefObject : public NiMemObject {
public:
    // Count zero; counts the object in ms_uiObjects.
    NiRefObject();
    virtual ~NiRefObject();

    // Deletes the object; called when the reference count drops to zero.
    virtual void DeleteThis();

    void IncRefCount() {
        NiAtomicIncrement(m_uiRefCount);
    }

    void DecRefCount() {
        if (NiAtomicDecrement(m_uiRefCount) == 0) {
            DeleteThis();
        }
    }

    uint32_t GetRefCount() const {
        return m_uiRefCount;
    }

    uint32_t m_uiRefCount;

    // Number of live reference-counted objects.
    static inline uint32_t ms_uiObjects;
};

void au_re_InterlockedDecrement_12_0(NiRefObject* apThis, [[maybe_unused]] void* apEdx);
