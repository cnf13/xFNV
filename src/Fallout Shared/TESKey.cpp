#include "Fallout Shared/TESKey.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/TESObjectMISC.h"

TESKey::TESKey() : TESObjectMISC() {
    SetFormType(46u);
    SetInitialized(true);
}

TESKey::~TESKey() {}
