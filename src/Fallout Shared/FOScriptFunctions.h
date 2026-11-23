#pragma once

#include "Fallout Shared/TESConditionItem.h"

class Script;
struct SCRIPT_PARAMETER;
class ScriptLocals;

bool GetVATSValueFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_GetVATSValue(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);

// The queries of the Fallout script functions that need more than the function table. Every method is static.
class FOScriptFunctions {
public:
    // Whether the first parameter of function 408 (a VATS value function) holds a form. aiParam is the parameter index.
    static bool GetParamIsTESForm(FunctionData* apFuncData, uint32_t aiParam);
};
