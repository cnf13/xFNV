#pragma once

#include "BSCore/NewOverloads.h"

class TESFile;
class TESForm;

// The data of a perk entry point function; the kinds hold one value, two values, a leveled list or an activate
// choice (label, script, flags).
class BGSEntryPointFunctionData {
public:
    // The kind of data a function takes, as the function type's data layout.
    enum ENTRY_POINT_FUNCTION_DATA : uint32_t {
        NONE = 0,
        ONE_VALUE = 1,
        TWO_VALUE = 2,
        LEVELED_LIST = 3,
        ACTIVATE_CHOICE = 4,
    };

    BGSEntryPointFunctionData();
    virtual ~BGSEntryPointFunctionData();

    // The kind of data.
    virtual ENTRY_POINT_FUNCTION_DATA GetType() = 0;
    // A new data object of the same kind holding the same data.
    virtual BGSEntryPointFunctionData* Duplicate() = 0;
    // true if apCompare is another kind or holds other data.
    virtual bool Compare(BGSEntryPointFunctionData* apCompare) = 0;
    virtual void Save() = 0;
    virtual bool Load(TESFile* apFile) = 0;
    // Resolves the form IDs the data holds, apOwner being the form that owns it.
    virtual void InitItem(TESForm* apOwner) {}

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
};
