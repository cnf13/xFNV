#pragma once

#include "BSCore/BSStringT.h"

class TESForm;

// An object a script refers to by its editor ID: the form it resolved to and the script variable that holds it.
struct SCRIPT_REFERENCED_OBJECT {
    // Constructs with a null editor ID, no form and no variable.
    SCRIPT_REFERENCED_OBJECT();

    BSStringT<char> cEditorID;
    TESForm* pForm;
    uint32_t uiVariableID;
};
