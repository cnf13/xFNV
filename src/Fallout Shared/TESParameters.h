#pragma once

#include "Fallout Shared/TESConditionItem.h"
#include "Reconstruction.hpp"

class Script;
class TESForm;

// The script function parameter queries: how many parameters a function call has, and which of them hold a form, a
// reference or a script variable. Every method is static.
class TESParameters {
public:
	// Resolves the form IDs in apFuncData's parameters for apForm's file.
	static void InitItem(TESForm* apOwner, FunctionData* apFuncData);
	// The function's parameter count; 0 without a function.
	static uint32_t GetParamCount(FunctionData* apFuncData);
	// Whether parameter auiParam holds a form.
	static bool GetParamIsTESForm(FunctionData* apFuncData, uint32_t aiParam);
	// Whether parameter auiParam is an object or a reference.
	static bool GetParamIsReference(FunctionData* apFuncData, uint32_t aiParam);
	// Whether parameter auiParam is a script variable.
	static bool GetParamIsScriptVariable(FunctionData* apFuncData, uint32_t aiParam);
	// The script of the form parameter auiParam.
	static Script* GetScriptFromParam(uint32_t aiParam, FunctionData* apFuncData);
};
