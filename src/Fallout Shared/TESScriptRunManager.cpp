#include "Fallout Shared/TESScriptRunManager.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSTaskManager.h"
#include "Fallout Shared/Expression.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/SCRIPT_PARAMETER.h"
#include "Fallout Shared/SCRIPT_REFERENCED_OBJECT.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout/Interface/Interface.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/Windows/winnt.h"

#include <cstdarg>

// Run's function-local static (S_LDATA32): the performance counter frequency, queried while it is still zero.
static LARGE_INTEGER ifrequency{};

// Zeroes the runner's whole state through the engine's memset wrapper; nothing else runs on construction.
ScriptRunner::ScriptRunner() {
    m_ifNested = 0;
    m_whileNested = 0;
    m_error = ERROR_SCRIPT_NOERROR;
    m_expError = ERROR_EXPRESSION_NOERROR;
    ourMemset(m_ifFlags, 0, sizeof(m_ifFlags));
    ourMemset(m_whileFlags, 0, sizeof(m_whileFlags));
    ourMemset(m_whileOffset, 0, sizeof(m_whileOffset));
    m_currentContainer = nullptr;
    m_currentObject = nullptr;
    m_currentVars = nullptr;
    m_currentInfo = nullptr;
    m_global = nullptr;
    m_scriptRunning = nullptr;
    m_bScriptErrorFound = false;
    m_bStopObjectScriptExecution = false;
}

bool ScriptRunner::IsRunning() {
    return m_scriptRunning != nullptr;
}

bool ScriptRunner::Run(
    Script* apScript,
    TESObjectREFR* apOwnerObject,
    ScriptLocals* apOwnerVars,
    TESObjectREFR* apOwnerContainer,
    bool abIsPartialScript,
    bool abScriptEffectStart,
    bool abScriptEffectFinish,
    float afSecondsElapsed
) {
    bool bdebugTextShow = Interface::IsDebugTextVisible();
    LARGE_INTEGER istart;
    istart.QuadPart = 0;
    if (bdebugTextShow) {
        if (!ifrequency.QuadPart) {
            QueryPerformanceFrequency(&ifrequency);
        }
        QueryPerformanceCounter(&istart);
    }
    m_bStopObjectScriptExecution = false;
    if (!apScript || !apScript->GetInitialized()) {
        return m_bStopObjectScriptExecution;
    }
    if (m_scriptRunning) {
        // The arguments are evaluated right to left, as the game does: the running script's name is read second.
        const char* pScriptName = apScript->GetFormEditorID();
        Warning(
            "SCRIPTS: Nested call to ScriptRunner::Run.  Script '%s' attempting to execute script '%s'.",
            m_scriptRunning->GetFormEditorID(), pScriptName
        );
        return m_bStopObjectScriptExecution;
    }

    m_scriptRunning = apScript;
    CExpression ce;
    ce.~CExpression();
    ::new (&ce) CExpression;
    ScriptLocals* tempVars = nullptr;
    m_ifFlags[0] = 0;
    m_ifNested = 0;
    SCRIPT_OUTPUT eNextExp = SCRIPT_DONE;
    uint32_t uiDataOffset = 0;
    m_currentContainer = apOwnerContainer;
    if (apOwnerObject) {
        m_currentObject = apOwnerObject->GetObjectReference();
    }
    else {
        m_currentObject = nullptr;
    }
    if (apOwnerVars) {
        m_currentVars = apOwnerVars;
    }
    else {
        tempVars = apScript->CloneLocals();
        m_currentVars = tempVars;
    }
    if (apScript->GetHeader()->bIsMagicEffectScript) {
        if (!m_currentVars->m_pScriptEffectData) {
            m_currentVars->m_pScriptEffectData =
                static_cast<SCRIPT_EFFECT_DATA*>(ourMalloc(sizeof(SCRIPT_EFFECT_DATA)));
        }
        m_currentVars->m_pScriptEffectData->m_bScriptEffectStart = abScriptEffectStart;
        m_currentVars->m_pScriptEffectData->m_bScriptEffectFinish = abScriptEffectFinish;
        m_currentVars->m_pScriptEffectData->m_fSecondsElapsed = afSecondsElapsed;
    }

    int32_t iLineSize = 0;
    TESObjectREFR* pRefObject = nullptr;
    uint32_t iSkipLines = 0;
    bool bbadscript = false;
    uint32_t iCurrentLine = 0;
    while (uiDataOffset < apScript->GetHeader()->dataSize) {
        ++iCurrentLine;
        m_global = nullptr;
        iLineSize = 0;
        if (!GetNextLine(apScript, eNextExp, uiDataOffset, iLineSize, &pRefObject, iSkipLines != 0)) {
            apScript->GetHeader()->dataSize = 0;
            break;
        }
        if (iSkipLines) {
            --iSkipLines;
        }
        else {
            if (eNextExp == FLOW_RETURN) {
                break;
            }
            if (!pRefObject) {
                pRefObject = apOwnerObject;
            }
            if (!EvaluateLine(
                    apScript, eNextExp, pRefObject, iLineSize, uiDataOffset, iSkipLines, iCurrentLine, false, false
                )) {
                bbadscript = true;
                break;
            }
        }
        uiDataOffset += iLineSize;
    }
    if (!abIsPartialScript && !bbadscript) {
        m_currentVars->ClearActionFlags();
    }
    if (tempVars) {
        delete tempVars;
    }
    m_scriptRunning = nullptr;
    if (bdebugTextShow) {
        LARGE_INTEGER iend;
        QueryPerformanceCounter(&iend);
        float fprofilerTime = static_cast<float>(
            static_cast<double>(iend.QuadPart - istart.QuadPart) / static_cast<double>(ifrequency.QuadPart)
        );
        apScript->AddProfilerTime(fprofilerTime);
    }
    // The result is read before ce's destructor runs.
    return m_bStopObjectScriptExecution;
}

bool ScriptRunner::InitActionList(Script* apScript, TESObjectREFR* apOwnerObject, ScriptLocals* apOwnerVars) {
    if (!apScript || !apScript->GetInitialized()) {
        return false;
    }
    if (m_scriptRunning) {
        Warning(
            "SCRIPTS: Nested call to ScriptRunner::Run.  Script '%s' attempting to execute script '%s'.",
            m_scriptRunning->GetFormEditorID(), apScript->GetFormEditorID()
        );
        return false;
    }

    m_scriptRunning = apScript;
    CExpression ce;
    ce.~CExpression();
    ::new (&ce) CExpression;
    SCRIPT_OUTPUT eNextExp = SCRIPT_DONE;
    uint32_t uiDataOffset = 0;
    if (apOwnerObject) {
        m_currentObject = apOwnerObject->GetObjectReference();
    }
    else {
        m_currentObject = nullptr;
    }
    m_currentVars = apOwnerVars;

    int32_t iLineSize = 0;
    TESObjectREFR* pRefObject = nullptr;
    uint32_t iSkipLines = 0;
    bool bbadscript = false;
    uint32_t iCurrentLine = 0;
    while (uiDataOffset < apScript->GetHeader()->dataSize) {
        ++iCurrentLine;
        m_global = nullptr;
        iLineSize = 0;
        if (!GetNextLine(apScript, eNextExp, uiDataOffset, iLineSize, &pRefObject, iSkipLines != 0)) {
            apScript->GetHeader()->dataSize = 0;
            break;
        }
        if (iSkipLines) {
            --iSkipLines;
        }
        else {
            if (!pRefObject) {
                pRefObject = apOwnerObject;
            }
            if (!EvaluateLine(
                    apScript, eNextExp, pRefObject, iLineSize, uiDataOffset, iSkipLines, iCurrentLine, false, true
                )) {
                bbadscript = true;
                break;
            }
        }
        uiDataOffset += iLineSize;
    }
    // bbadscript is stored but never read.
    (void)bbadscript;
    m_scriptRunning = nullptr;
    return true;
}

bool ScriptRunner::GetNextLine(
    Script* apScript,
    SCRIPT_OUTPUT& aeNextExp,
    uint32_t& auiDataOffset,
    int32_t& aiLineSize,
    TESObjectREFR** appRefObject,
    bool abSkip
) {
    // The compiled data is a stream of 16-bit words, read sign-extended.
    aiLineSize = 0;
    aeNextExp = static_cast<SCRIPT_OUTPUT>(*reinterpret_cast<int16_t*>(apScript->GetCompiledData() + auiDataOffset));
    auiDataOffset += 2;
    if (aeNextExp == FLOW_REFERENCE_FUNCTION) {
        bool bIsRefVar = false;
        uint32_t uiRefIndex = *reinterpret_cast<int16_t*>(apScript->GetCompiledData() + auiDataOffset);
        auiDataOffset += 2;
        if (!abSkip) {
            Script::RefVariable* pScriptRefObj = apScript->GetReferencedObject(uiRefIndex, m_currentVars);
            if (pScriptRefObj) {
                bIsRefVar = pScriptRefObj->varIdx != 0;
                if (pScriptRefObj->form) {
                    *appRefObject = dynamic_cast<TESObjectREFR*>(pScriptRefObj->form);
                }
            }
        }
        aeNextExp =
            static_cast<SCRIPT_OUTPUT>(*reinterpret_cast<int16_t*>(apScript->GetCompiledData() + auiDataOffset));
        auiDataOffset += 2;
        if (!abSkip && !*appRefObject && !bIsRefVar) {
            SCRIPT_FUNCTION* pFunction = ScriptCompiler::GetFunctionDef(aeNextExp);
            if (pFunction) {
                ScriptError(
                    apScript, "Unknown reference object for function '%s'.  Execution of script '%s' stopped.",
                    pFunction->pFunctionName, apScript->GetFormEditorID()
                );
            }
            else {
                ScriptError(
                    apScript, "Unknown reference object.  Execution of script '%s' stopped.",
                    apScript->GetFormEditorID()
                );
            }
            return false;
        }
    }
    else {
        *appRefObject = nullptr;
    }
    aiLineSize = *reinterpret_cast<int16_t*>(apScript->GetCompiledData() + auiDataOffset);
    auiDataOffset += 2;
    return true;
}

bool ScriptRunner::EvaluateLine(
    Script* apScript,
    SCRIPT_OUTPUT aeExp,
    TESObjectREFR* apFunctionOwner,
    int32_t& aiLineSize,
    uint32_t auiDataOffset,
    uint32_t& aiSkipLines,
    uint32_t auiCurrentLine,
    bool abEndian,
    bool abActionInit
) {
    double v23;
    FNVD_INLINED_LOCAL(CExpression, ce_2, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(CExpression, ce, std::nullopt, std::nullopt);
    double fVal;
    uint32_t uiCurrentOffset;
    uiCurrentOffset = auiDataOffset;
    char* pCompiledData = apScript->GetCompiledData();
    if (aeExp == START_OF_FLOW_SECTION) {
        // The compiled data is untyped bytes: unaligned 16-bit and 32-bit values, read in place.
        uint32_t eBlockType = *reinterpret_cast<int16_t*>(&pCompiledData[uiCurrentOffset]);
        uiCurrentOffset += 2;
        if (eBlockType >= 38) {
            int32_t afValue = apScript->iFormID;
            Warning(
                "Invalid script block for BEGIN on line %i of script '%s' (%08X).", auiCurrentLine,
                apScript->GetFormEditorID(), afValue
            );
            return false;
        }
        uint32_t ibeginEndJumpDelta = *reinterpret_cast<uint32_t*>(&pCompiledData[uiCurrentOffset]);
        uiCurrentOffset += 4;
        SCRIPT_PARAMETER* pParameterDef = scriptBlocks[eBlockType].pParameters;
        auto pExecute = scriptBlocks[eBlockType].pExecuteFunction;
        if (abEndian) {
            if ((pParameterDef) && (aiLineSize)) {
                Script::ParseParameters(
                    pParameterDef, pCompiledData, uiCurrentOffset, apFunctionOwner, m_currentContainer, apScript,
                    m_currentVars
                );
            }
        }
        else {
            if (scriptBlocks[eBlockType].bReferenceFunction && !apFunctionOwner) {
                Warning(
                    "SCRIPTS: Script '%s', line %d: Null for a required ref pointer.", apScript->GetFormEditorID(),
                    auiCurrentLine
                );
                return false;
            }
            if (!pExecute ||
                !pExecute(
                    pParameterDef, pCompiledData, apFunctionOwner, m_currentContainer, apScript, m_currentVars, fVal,
                    uiCurrentOffset
                )) {
                uint32_t afValuea = uiCurrentOffset;
                const char* v11 = apScript->GetFormEditorID();
                Warning(
                    "SCRIPTS: Script '%s', line %d: Error Executing line (Offset %d).", v11, auiCurrentLine, afValuea
                );
                return false;
            }
            if (0.0 == fVal || abActionInit) {
                aiLineSize += ibeginEndJumpDelta;
            }
        }
    }
    else {
        if (!abActionInit) {
            switch (aeExp) {
                case FLOW_END:
                case FLOW_REFERENCE_FUNCTION:
                case FLOW_SCRIPTNAME: {
                    return true;
                }
                case FLOW_SET: {
                    TESObjectREFR* pRefForm;
                    int16_t sRefObjectIndex;
                    char cType = pCompiledData[uiCurrentOffset++];
                    int16_t sVariableID = 0;
                    // One variable holds the running script's locals unless the SET names another object's.
                    ScriptLocals* pScriptLocals = m_currentVars;
                    Script::RefVariable* pRefObj = nullptr;
                    if (cType != 71 && cType != 114) {
                        goto LABEL_37;
                    }
                    sRefObjectIndex = *reinterpret_cast<uint16_t*>(&pCompiledData[uiCurrentOffset]);
                    uiCurrentOffset += 2;
                    if (cType == 114) {
                        cType = pCompiledData[uiCurrentOffset++];
                    }
                    pRefObj = apScript->GetReferencedObject(sRefObjectIndex, m_currentVars);
                    if (abEndian) {
                        goto LABEL_37;
                    }
                    if (cType == 114 || (!pRefObj || !pRefObj->form)) {
                        int32_t afValuec = apScript->iFormID;
                        Warning(
                            "Missing ref object in SET on line %i of script '%s' (%08X).", auiCurrentLine,
                            apScript->GetFormEditorID(), afValuec
                        );
                        return false;
                    }
                    pRefForm = dynamic_cast<TESObjectREFR*>(pRefObj->form);
                    if (pRefForm) {
                        pScriptLocals = pRefForm->GetScriptVariables();
                    }
                    else {
                        TESQuest* pQuest = dynamic_cast<TESQuest*>(pRefObj->form);
                        if (pQuest) {
                            pScriptLocals = pQuest->GetScriptLocals();
                        }
                    }
                    if (!pScriptLocals) {
                        int32_t afValueb = apScript->iFormID;
                        Warning(
                            "Missing script variables on object for SET on line %i of script '%s' (%08X).",
                            auiCurrentLine, apScript->GetFormEditorID(), afValueb
                        );
                        return false;
                    }
                LABEL_37:;
                    if (cType != 71) {
                        sVariableID = *reinterpret_cast<uint16_t*>(&pCompiledData[uiCurrentOffset]);
                        uiCurrentOffset += 2;
                    }
                    int16_t sSize = *reinterpret_cast<uint16_t*>(&pCompiledData[uiCurrentOffset]);
                    uiCurrentOffset += 2;
                    ::new (&ce) CExpression;
                    double val = ce.Eval(
                        &pCompiledData[uiCurrentOffset], apFunctionOwner, m_currentContainer, apScript, m_currentVars,
                        sSize, abEndian
                    );
                    uiCurrentOffset += sSize;
                    if (ce.m_ExpressionError) {
                        Warning(
                            "SCRIPTS: Script '%s', line %d: Set expression returned an error: %s.\r\n",
                            apScript->GetFormEditorID(), auiCurrentLine, expressionErrors[ce.m_ExpressionError]
                        );
                        (&apScript->m_header)->dataSize = 0;
                        return false;
                    }
                    if (abEndian) {
                        goto LABEL_74;
                    }
                    if (cType == 71) {
                        if (!pRefObj || !pRefObj->form) {
                            int32_t afValued = apScript->iFormID;
                            Warning(
                                "Invalid variable type for SET on line %i of script '%s' (%08X).", auiCurrentLine,
                                apScript->GetFormEditorID(), afValued
                            );
                            return false;
                        }
                        float val_2 = val;
                        static_cast<TESGlobal*>(pRefObj->form)->TESGlobal::SetValue(val_2);
                        if (Script::VerboseOutput()) {
                            float v20 = val;
                            Interface::PrintLine("set global >> %0.2f", v20);
                        }
                    }
                    else {
                        if (!pScriptLocals) {
                            int32_t afValuee = apScript->iFormID;
                            Warning(
                                "Missing scriptvariables for object in SET on line %i of script '%s' (%08X).",
                                auiCurrentLine, apScript->GetFormEditorID(), afValuee
                            );
                            return false;
                        }
                        if (cType == 102) {
                            pScriptLocals->SetVariable(sVariableID, val);
                            if (Script::VerboseOutput()) {
                                Interface::PrintLine(
                                    "set %s >> %.2f", pScriptLocals->m_pMasterScript->GetVariableName(sVariableID), val
                                );
                            }
                        }
                        else {
                            if (cType == 108 || cType == 115) {
                                pScriptLocals->SetVariable(sVariableID, static_cast<double>(static_cast<int32_t>(val)));
                                if (Script::VerboseOutput()) {
                                    Interface::PrintLine(
                                        "set %s >> %i", pScriptLocals->m_pMasterScript->GetVariableName(sVariableID),
                                        static_cast<int32_t>(val)
                                    );
                                }
                            }
                        }
                        if ((pRefObj && pRefObj->form) || apFunctionOwner) {
                            TESObjectREFR* pref = nullptr;
                            if (pRefObj) {
                                if ((pRefObj->form) && (pRefObj->form->IsReference())) {
                                    pref = static_cast<TESObjectREFR*>(pRefObj->form);
                                }
                            }
                            else {
                                pref = apFunctionOwner;
                            }
                            if ((pref) &&
                                (!pref->GetTemporary() &&
                                 (!pref->GetParentCell() || !pref->GetParentCell()->GetAttached()))) {
                                pref->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
                            }
                        }
                    }
                LABEL_74:;
                    return true;
                }
                case FLOW_IF: {
                    if (m_ifFlags[m_ifNested]) {
                        ++m_ifNested;
                    }
                    m_ifFlags[m_ifNested] = 1;
                    goto LABEL_78;
                }
                case FLOW_ELSE: {
                    int16_t iBlockLines_2 = *reinterpret_cast<uint16_t*>(&pCompiledData[uiCurrentOffset]);
                    uiCurrentOffset += 2;
                    if ((m_ifFlags[m_ifNested] & 2) != 0) {
                        if (!abEndian) {
                            aiSkipLines += iBlockLines_2;
                        }
                    }
                    else {
                        m_ifFlags[m_ifNested] |= 2;
                    }
                    return true;
                }
                case FLOW_ELSEIF: {
                LABEL_78:;
                    int16_t iBlockLines = *reinterpret_cast<uint16_t*>(&pCompiledData[uiCurrentOffset]);
                    uiCurrentOffset += 2;
                    int16_t uiSize = *reinterpret_cast<uint16_t*>(&pCompiledData[uiCurrentOffset]);
                    uiCurrentOffset += 2;
                    ::new (&ce_2) CExpression;
                    if (abEndian) {
                        ce_2.Eval(
                            &pCompiledData[uiCurrentOffset], apFunctionOwner, m_currentContainer, apScript,
                            m_currentVars, uiSize, abEndian
                        );
                        m_ifFlags[m_ifNested] |= 2;
                    }
                    else {
                        if ((m_ifFlags[m_ifNested] & 2) != 0 ||
                            ce_2.Eval(
                                &pCompiledData[uiCurrentOffset], apFunctionOwner, m_currentContainer, apScript,
                                m_currentVars, uiSize, false
                            ) == 0.0) {
                            aiSkipLines += iBlockLines;
                        }
                        else {
                            m_ifFlags[m_ifNested] |= 2;
                        }
                    }
                    if (ce_2.m_ExpressionError) {
                        SetError(apScript, ERROR_SCRIPT_EXPRESSION);
                        Warning(
                            "SCRIPTS: Script '%s', line %d: failed to evaluate expression.",
                            apScript->GetFormEditorID(), auiCurrentLine
                        );
                        (&apScript->m_header)->dataSize = 0;
                        m_expError = ce_2.m_ExpressionError;
                        return false;
                    }
                    uiCurrentOffset += uiSize;
                    return true;
                }
                case FLOW_ENDIF: {
                    m_ifFlags[m_ifNested] = 0;
                    if (m_ifNested) {
                        --m_ifNested;
                    }
                    return true;
                }
                case FLOW_RETURN: {
                    if (abEndian) {
                        return true;
                    }
                    goto LABEL_96;
                }
                default: {
                LABEL_96:;
                    SCRIPT_FUNCTION* pFunctionDef = ScriptCompiler::GetFunctionDef(aeExp);
                    if (!pFunctionDef) {
                        BSStringT<char> message;
                        message.SPrintF(
                            "SCRIPTS: Unable to find function definition for command %d in script '%s'.", aeExp,
                            apScript->GetFormEditorID()
                        );
                        if (Script::VerboseOutput()) {
                            Interface::PrintLine(static_cast<const char*>(message.pString));
                        }
                        else {
                            Warning(message.pString);
                        }
                        return false;
                    }
                    SCRIPT_PARAMETER* pParameterDef_2 = pFunctionDef->pParameters;
                    if (pFunctionDef->bReferenceFunction && (!apFunctionOwner && !abEndian)) {
                        BSStringT<char> message_2;
                        message_2.SPrintF(
                            "SCRIPTS: Script '%s', line %d: Function '%s' requires a reference.",
                            apScript->GetFormEditorID(), auiCurrentLine, pFunctionDef->pFunctionName
                        );
                        if (Script::VerboseOutput()) {
                            Interface::PrintLine(static_cast<const char*>(message_2.pString));
                        }
                        else {
                            Warning(message_2.pString);
                        }
                        return false;
                    }
                    if (abEndian) {
                        if ((pParameterDef_2) && (aiLineSize)) {
                            Script::ParseParameters(
                                pParameterDef_2, pCompiledData, uiCurrentOffset, apFunctionOwner, m_currentContainer,
                                apScript, m_currentVars
                            );
                            if (aeExp == FUNCTION_MESSAGE) {
                                int16_t sVarCount = *reinterpret_cast<uint16_t*>(&pCompiledData[uiCurrentOffset]);
                                uiCurrentOffset += 2;
                                for (int16_t n = 0; n < sVarCount; ++n) {
                                    double dtmp;
                                    Script::GetValue(
                                        dtmp, pCompiledData, uiCurrentOffset, apFunctionOwner, m_currentContainer,
                                        apScript, m_currentVars, true
                                    );
                                }
                                uiCurrentOffset += 4;
                            }
                        }
                    }
                    else {
                        auto pExecute_2 = pFunctionDef->pExecuteFunction;
                        if (pFunctionDef->bInvalidatesCellList) {
                            m_bStopObjectScriptExecution = true;
                        }
                        if (!pExecute_2 ||
                            !pExecute_2(
                                pParameterDef_2, pCompiledData, apFunctionOwner, m_currentContainer, apScript,
                                m_currentVars, v23, uiCurrentOffset
                            )) {
                            int32_t afValuef = apScript->iFormID;
                            const char* v12 = apScript->GetFormEditorID();
                            Warning(
                                "Failed to execute function '%s' on line %i of script '%s' (%08X).",
                                pFunctionDef->pFunctionName, auiCurrentLine, v12, afValuef
                            );
                            return false;
                        }
                    }
                    break;
                }
            }
        }
    }
    return true;
}

void ScriptRunner::ScriptError(Script* apScript, const char* text, ...) {
    char buffer[260];
    va_list args;
    va_start(args, text);
    BSvsprintf(buffer, text, args);
    va_end(args);
    if (Script::VerboseOutput()) {
        Interface::PrintLine(buffer);
    }
    else {
        Warning(buffer);
    }
    apScript->GetHeader()->dataSize = 0;
    m_bScriptErrorFound = true;
}

void ScriptRunner::SetError(Script* apScript, SCRIPT_ERROR errorCode) {
    m_error = errorCode;
}

ScriptRunManager::ScriptRunManager() : m_pScriptRunner(nullptr), pCurrentRunner(nullptr) {}

// The runner's memory is freed without running its destructor, as the game does.
ScriptRunManager::~ScriptRunManager() {
    if (m_pScriptRunner) {
        ourFree(m_pScriptRunner);
        m_pScriptRunner = nullptr;
    }
    pCurrentRunner = nullptr;
}

bool ScriptRunManager::Run(
    Script* apScript,
    TESObjectREFR* apOwnerObject,
    ScriptLocals* apOwnerVars,
    TESObjectREFR* apOwnerContainer,
    bool abIsPartialScript,
    bool abScriptEffectStart,
    bool abScriptEffectFinish,
    float afSecondsElapsed
) {
    bool v16;
    if (NiAtomicIncrement(bIsScriptExecuting) == 1) {
        if (!m_pScriptRunner) {
            ScriptRunner* v11;
            ScriptRunner* v14 = static_cast<ScriptRunner*>(ourMalloc(sizeof(ScriptRunner)));
            if (v14) {
                v11 = ::new (v14) ScriptRunner();
            }
            else {
                v11 = nullptr;
            }
            m_pScriptRunner = v11;
        }
        pCurrentRunner = m_pScriptRunner;
        v16 = m_pScriptRunner->Run(
            apScript, apOwnerObject, apOwnerVars, apOwnerContainer, abIsPartialScript, abScriptEffectStart,
            abScriptEffectFinish, afSecondsElapsed
        );
        pCurrentRunner = nullptr;
    }
    else {
        ScriptRunner* pNewRunner;
        ScriptRunner* v13 = static_cast<ScriptRunner*>(ourMalloc(sizeof(ScriptRunner)));
        if (v13) {
            pNewRunner = ::new (v13) ScriptRunner();
        }
        else {
            pNewRunner = nullptr;
        }
        ScriptRunner* pOldRunner = pCurrentRunner;
        pCurrentRunner = pNewRunner;
        v16 = pNewRunner->Run(
            apScript, apOwnerObject, apOwnerVars, apOwnerContainer, abIsPartialScript, abScriptEffectStart,
            abScriptEffectFinish, afSecondsElapsed
        );
        pCurrentRunner = pOldRunner;
        ourFree(pNewRunner);
    }
    NiAtomicDecrement(bIsScriptExecuting);
    return v16;
}

bool ScriptRunManager::InitActionList(Script* apScript, TESObjectREFR* apOwnerObject, ScriptLocals* apOwnerVars) {
    if (!m_pScriptRunner) {
        void* pMemory = ourMalloc(sizeof(ScriptRunner));
        m_pScriptRunner = pMemory ? ::new (pMemory) ScriptRunner() : nullptr;
    }
    if (m_pScriptRunner->IsRunning()) {
        void* pMemory = ourMalloc(sizeof(ScriptRunner));
        ScriptRunner* pRunner = pMemory ? ::new (pMemory) ScriptRunner() : nullptr;
        pRunner->InitActionList(apScript, apOwnerObject, apOwnerVars);
        ourFree(pRunner);
    }
    else {
        m_pScriptRunner->InitActionList(apScript, apOwnerObject, apOwnerVars);
    }
    return true;
}

ScriptRunManager* ScriptRunManager::Instance() {
    static ScriptRunManager instance;
    return &instance;
}
