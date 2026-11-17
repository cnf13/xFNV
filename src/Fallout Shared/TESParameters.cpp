#include "Fallout Shared/TESParameters.h"

#include "BSCore/BSStringT.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/FOScriptFunctions.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/SCRIPT_PARAMETER.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESConditionItem.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "SDK/MSVC/rtti.h"

// Resolves the form IDs among the item's parameters to forms, warning on each that is missing, and checks that each
// script-variable parameter names a variable of the script the parameter before it holds. The exe's cleanup-only C++
// EH frame is left out.
void TESParameters::InitItem(TESForm* apOwner, FunctionData* apFuncData) {
	FNVD_INLINED_LOCAL(BSStringT<char>, tempstring, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSStringT<char>, tempstring_2, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSStringT<char>, tempstring_3, std::nullopt, std::nullopt);
	if (apOwner) {
		if (apFuncData) {
			uint32_t iparamcount = TESParameters::GetParamCount(apFuncData);
			TESFile* pownerFile = apOwner ? apOwner->GetFile(-1) : nullptr;
			for (uint32_t i = 0; i < iparamcount; ++i) {
				if (TESParameters::GetParamIsTESForm(apFuncData, i)) {
					uint32_t uiFormID = apFuncData->uParam[i].uiNumber;
					if (uiFormID) {
						TESForm::AddCompileIndex(uiFormID, pownerFile);
						apFuncData->uParam[i].pPointer = TESForm::GetFormByNumericID(uiFormID);
						if (!apFuncData->uParam[i].pPointer) {
							::new (&tempstring) BSStringT<char>;
							apOwner->GetFormDetailedString(tempstring);
							Warning("MASTERFILE: Unable to find Function Info TESForm (%08X) in TESConditionItem "
									"Parameter Init for %s.",
								uiFormID, tempstring.String());
							tempstring.~BSStringT<char>();
						} else if (scriptParameterDefs
									   [scriptFunctions[apFuncData->usFunction].pParameters[i].eParamType]
										   .eParamType == SCRIPT_PARAM_MAGIC_ITEM) {
							apFuncData->uParam[i].pPointer =
								FNVD_DYNAMIC_CAST_VOID(MagicItem)(static_cast<TESForm*>(apFuncData->uParam[i].pPointer),
									RTTI_TESForm, RTTI_MagicItem);
							if (!apFuncData->uParam[i].pPointer) {
								::new (&tempstring_2) BSStringT<char>;
								apOwner->GetFormDetailedString(tempstring_2);
								// The exe passes the form ID as well, which the text does not use.
								Warning("MASTERFILE: Unable to find Function Info MagicItem in TESConditionItem "
										"Parameter "
										"Init for %s.",
									uiFormID, tempstring_2.String());
								tempstring_2.~BSStringT<char>();
							}
						}
					}
				} else if (TESParameters::GetParamIsScriptVariable(apFuncData, i)) {
					if (i > 0) {
						Script* pScript = TESParameters::GetScriptFromParam(i - 1, apFuncData);
						if (pScript && !pScript->GetVariableName(apFuncData->uParam[i].uiNumber)) {
							::new (&tempstring_3) BSStringT<char>;
							apOwner->GetFormDetailedString(tempstring_3);
							const char* pFormString = tempstring_3.String();
							Warning("MASTERFILE: Unable to find variableID %d on script '%s' in TESConditionItem "
									"Parameter Init for %s.",
								apFuncData->uParam[i].uiNumber, pScript->GetFormEditorID(), pFormString);
							tempstring_3.~BSStringT<char>();
						}
					}
				}
			}
		}
	}
}

uint32_t TESParameters::GetParamCount(FunctionData* apFuncData) {
	uint32_t v2 = 0;
	if (apFuncData) {
		// The exe zero-extends the 16-bit count.
		return static_cast<uint16_t>(scriptFunctions[apFuncData->usFunction].sParamCount);
	}
	return v2;
}

bool TESParameters::GetParamIsTESForm(FunctionData* apFuncData, uint32_t aiParam) {
	bool bretval = 0;
	if (apFuncData && apFuncData->usFunction == 408) {
		return FOScriptFunctions::GetParamIsTESForm(apFuncData, aiParam);
	}
	if (apFuncData) {
		if (aiParam < TESParameters::GetParamCount(apFuncData)) {
			return scriptParameterDefs[scriptFunctions[apFuncData->usFunction].pParameters[aiParam].eParamType]
				.bReferencedObject;
		}
	}
	return bretval;
}

bool TESParameters::GetParamIsReference(FunctionData* apFuncData, uint32_t aiParam) {
	bool bretval = 0;
	if (apFuncData) {
		if (aiParam < TESParameters::GetParamCount(apFuncData)) {
			SCRIPT_PARAM_TYPE eParamType = scriptFunctions[apFuncData->usFunction].pParameters[aiParam].eParamType;
			if (eParamType == SCRIPT_PARAM_OBJECTREF || eParamType == SCRIPT_PARAM_ACTOR) {
				return 1;
			}
		}
	}
	return bretval;
}

bool TESParameters::GetParamIsScriptVariable(FunctionData* apFuncData, uint32_t aiParam) {
	bool bretval = 0;
	if (apFuncData) {
		if (aiParam < TESParameters::GetParamCount(apFuncData)) {
			return scriptFunctions[apFuncData->usFunction].pParameters[aiParam].eParamType == SCRIPT_PARAM_SCRIPT_VAR;
		}
	}
	return bretval;
}

Script* TESParameters::GetScriptFromParam(uint32_t aiParam, FunctionData* apFuncData) {
	Script* pScript = 0;
	if (TESParameters::GetParamIsTESForm(apFuncData, aiParam)) {
		TESForm* pform = static_cast<TESForm*>(apFuncData->uParam[aiParam].pPointer);
		TESScriptableForm* pscriptableform = dynamic_cast<TESScriptableForm*>(pform);
		if (pscriptableform) {
			return pscriptableform->pFormScript;
		} else {
			TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(pform);
			if (pRef) {
				return TESScriptableForm::GetFormScript(pRef->GetObjectReference());
			}
		}
	}
	return pScript;
}
