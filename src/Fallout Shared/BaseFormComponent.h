#pragma once

#include "BSCore/NewOverloads.h"

// Root of TESForm and of the components forms are assembled from (TESFullName, TESModel, ...). It has no virtual
// destructor: its four virtual functions are the per-component data hooks, all pure.
class BaseFormComponent {
public:
    BaseFormComponent();

    virtual void InitializeDataComponent() = 0;
    virtual void ClearDataComponent() = 0;
    virtual void CopyComponent(BaseFormComponent* apCopyFC) = 0;
    virtual bool CompareComponent(BaseFormComponent* apCompareFC) = 0;

    // Forms and their components are freed through ourFree. Declared once here: a form holding several components finds
    // this one static member through every base, unambiguously.
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
};
