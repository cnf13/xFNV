#include "Fallout Shared/TESScriptCompiler.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSTaskManager.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSEncounterZone.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSMenuIcon.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/Expression.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/SCRIPT_PARAMETER.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/ScriptCompileData.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESCombatStyle.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESObjectBOOK.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectSTAT.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESScriptVariable.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

#include <cctype>
#include <cstdint>

uint32_t BSstrtoul(const char* apString, char** apEndPtr, int32_t aiBase) {
    return std::strtoul(apString, apEndPtr, aiBase);
}

SCRIPT_LINE::SCRIPT_LINE()
  : uiLineNumber(0), pLine(), uiSize(0), uiOffset(0), pOutput(), uiOutputSize(0), eScriptError(ERROR_SCRIPT_NOERROR) {}

SCRIPT_WORD::SCRIPT_WORD()
  : pText(), uiRefObjectIndex(0), cVariableType(0), eFunctionCode(static_cast<SCRIPT_OUTPUT>(0)), uiVariableID(0),
    pForm(nullptr) {}

ScriptCompiler::ScriptCompiler() {
    // A table that does not end at its last index has not been built yet: its name is logged through
    // InitializationFlow, which is empty.
    if (scriptFunctions[FUNCTION_COUNT].eOutput != END_OF_FUNCTION_SECTION) {
        InitializationFlow("The script function list doesn't have all the names filled in.\r\n");
    }
    if (scriptConsole[CONSOLE_COUNT].eOutput != END_OF_CONSOLE_SECTION) {
        InitializationFlow("The script console-command list doesn't have all the names filled in.\r\n");
    }
}

bool ScriptCompiler::CompilePartialScript(Script* apScript, TESObjectREFR* apOwnerObject, COMPILER_NAME index, bool) {
    alignas(ScriptCompileData) uint8_t aucCompileData[sizeof(ScriptCompileData)];
    ScriptCompileData& kCompileData = *::new (aucCompileData) ScriptCompileData;
    kCompileData.eCompilerIndex = index;
    kCompileData.bIsPartialScript = true;

    // The owner's own script lends its header and referenced objects; otherwise the script compiles alone.
    if (apOwnerObject && apOwnerObject->GetScriptPointer()) {
        kCompileData.pCurrentScript = apOwnerObject->GetScriptPointer();
        ourMemcpy(&kCompileData.header, kCompileData.pCurrentScript->GetHeader(), sizeof(SCRIPT_HEADER));
        Script::CopyReferencedObjectList(
            kCompileData.pCurrentScript->GetReferencedObjectList(), &kCompileData.listRefObjects, nullptr
        );
    }
    else {
        kCompileData.pCurrentScript = apScript;
    }

    // The console, and a script without an editor ID, compile under the compiler's own name.
    if (index == SYSTEM_WINDOW_COMPILER || !kCompileData.pCurrentScript->GetFormEditorIDLength()) {
        kCompileData.cScriptName = compilerNames[kCompileData.eCompilerIndex];
    }
    else {
        kCompileData.cScriptName.Set(kCompileData.pCurrentScript->GetFormEditorID(), 0);
    }

    kCompileData.input = apScript->GetText();
    bool bResult = Compile(apScript, &kCompileData);
    kCompileData.~ScriptCompileData();
    return bResult;
}

bool ScriptCompiler::Compile(Script* apScript, ScriptCompileData* apCompileData) {
    if (!apScript || !apCompileData) {
        return false;
    }

    apScript->SetIsCompiled(false);
    apCompileData->header.bIsQuestScript = apScript->GetHeader()->bIsQuestScript;
    apCompileData->header.bIsMagicEffectScript = apScript->GetHeader()->bIsMagicEffectScript;
    apCompileData->header.m_uiLastID = apScript->GetHeader()->m_uiLastID;

    if (apCompileData->input) {
        while (GetNextExpression(apCompileData) != SCRIPT_DONE) {}
        if (apCompileData->eLastError > 0) {
            return false;
        }
        apCompileData->uiLastLineNumber = 0;

        // A full script must open with its scriptname line and have well-formed blocks.
        if (!apCompileData->bIsPartialScript) {
            BSSimpleList<SCRIPT_LINE*>* pLines = apCompileData->QScriptLineList();
            if (!pLines || pLines->IsEmpty()) {
                ScriptError(apCompileData, "You need to start scripts with \"scriptname ScriptName\"");
                return false;
            }
            SCRIPT_OUTPUT eExpression = static_cast<SCRIPT_LINE*>(pLines->GetItem())->sExpression;
            if (eExpression == SCRIPT_DONE) {
                return false;
            }
            if (eExpression != FLOW_SCRIPTNAME) {
                ScriptError(apCompileData, "You need to start scripts with \"scriptname ScriptName\"");
                return false;
            }
            if (!VerifyBlocks(apCompileData)) {
                return false;
            }
        }
    }

    BSSimpleList<SCRIPT_LINE*>* pIter = apCompileData->QScriptLineList();
    while (pIter && !pIter->IsEmpty()) {
        SCRIPT_LINE* pScriptLine = static_cast<SCRIPT_LINE*>(pIter->GetItem());
        apCompileData->uiLastLineNumber = pScriptLine->uiLineNumber;
        if (!CompileLine(apScript, apCompileData, pScriptLine)) {
            return false;
        }
        pIter = pIter->GetNext();
    }

    apCompileData->header.dataSize = apCompileData->uiOutputOffset;
    if (!apCompileData->VerifyReferencedObjectList()) {
        return false;
    }
    apScript->Init(apCompileData);
    apScript->SetIsCompiled(true);
    return true;
}

// The compiled output is a byte buffer that 16-bit and 32-bit values are written into through casts.
bool ScriptCompiler::CompileLine(Script* apScript, ScriptCompileData* apCompileData, SCRIPT_LINE* apScriptLine) {
    static int32_t ibeginEndJumpOffset;
    uint32_t uiSize_2;
    SCRIPT_WORD scriptWord2;
    uint32_t uiSize;
    SCRIPT_LINE tempLine;
    FNVD_INLINED_LOCAL(CExpression, ce, std::nullopt, std::nullopt);
    char* postfix;
    SCRIPT_WORD scriptWord;
    scriptWord.~SCRIPT_WORD();
    ::new (&scriptWord) SCRIPT_WORD;
    uint32_t auiLineOffset = 0;
    ::new (&ce) CExpression;
    apScriptLine->uiOutputSize = 0;
    apScriptLine->uiOffset = 0;
    switch (apScriptLine->sExpression) {
        case START_OF_FLOW_SECTION: {
            if (ScriptCompiler::GetNextWord(
                    apCompileData, &scriptWord, apScriptLine->pLine, apScriptLine->uiOffset, false, false
                )) {
                int32_t n;
                bool bFound = false;
                for (n = 0; n < 38; ++n) {
                    if (!BSstricmp(scriptBlocks[n].pFunctionName, scriptWord.pText) ||
                        !BSstricmp(scriptBlocks[n].pShortName, scriptWord.pText)) {
                        bFound = true;
                        break;
                    }
                }
                if (bFound) {
                    *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) =
                        static_cast<uint16_t>(scriptBlocks[n].eOutput);
                    apScriptLine->uiOutputSize += 2;
                    int32_t lines = CountBlockLines(apCompileData, apScriptLine);
                    if (lines == -1) {
                        ScriptCompiler::ScriptError(apCompileData, "Mismatched begin/end block.");
                        return false;
                    }
                    // A placeholder for the block's end offset, patched below and at the block's 'end'.
                    *reinterpret_cast<uint32_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) = 0x55555555;
                    ibeginEndJumpOffset = apScriptLine->uiOutputSize + apCompileData->uiOutputOffset;
                    apScriptLine->uiOutputSize += 4;
                    SCRIPT_FUNCTION* pFunctionDef = &scriptBlocks[n];
                    if (pFunctionDef) {
                        if (pFunctionDef->bReferenceFunction && apCompileData->header.bIsQuestScript) {
                            ScriptCompiler::ScriptError(apCompileData, "Invalid block type for quest script.");
                            return false;
                        }
                        if (pFunctionDef->pParameters) {
                            SCRIPT_PARAMETER* pParameterDef = pFunctionDef->pParameters;
                            bool (*pCompile)(uint16_t, const SCRIPT_PARAMETER*, SCRIPT_LINE*, ScriptCompileData*) =
                                pFunctionDef->pCompileFunction;
                            uint16_t sCount = pFunctionDef->sParamCount;
                            tempLine.~SCRIPT_LINE();
                            ::new (&tempLine) SCRIPT_LINE;
                            BSstrcpy(tempLine.pLine, 512, &apScriptLine->pLine[apScriptLine->uiOffset]);
                            tempLine.uiOutputSize = 0;
                            tempLine.uiLineNumber = apScriptLine->uiLineNumber;
                            if (!pCompile(sCount, pParameterDef, &tempLine, apCompileData)) {
                                return false;
                            }
                            ourMemcpy(
                                &apScriptLine->pOutput[apScriptLine->uiOutputSize], tempLine.pOutput,
                                tempLine.uiOutputSize
                            );
                            apScriptLine->uiOutputSize += tempLine.uiOutputSize;
                        }
                    }
                    if (ibeginEndJumpOffset) {
                        *reinterpret_cast<uint32_t*>(
                            &apScriptLine->pOutput[ibeginEndJumpOffset - apCompileData->uiOutputOffset]
                        ) = apScriptLine->uiOutputSize + apCompileData->uiOutputOffset;
                    }
                    goto LABEL_92;
                }
                ScriptCompiler::ScriptError(apCompileData, "Syntax Error.  Invalid block type in 'begin' command.");
                return false;
            }
            else {
                ScriptCompiler::ScriptError(apCompileData, "Syntax Error.  Missing block type in 'begin' command.");
                return false;
            }
        }
        case FLOW_END: {
            if (ibeginEndJumpOffset) {
                uint32_t ibeginEndJumpDelta =
                    *reinterpret_cast<uint32_t*>(&apCompileData->pOutput[ibeginEndJumpOffset + 4]);
                *reinterpret_cast<uint32_t*>(&apCompileData->pOutput[ibeginEndJumpOffset + 4]) =
                    apCompileData->uiOutputOffset - ibeginEndJumpDelta;
                ibeginEndJumpOffset = 0;
                goto LABEL_92;
            }
            ScriptCompiler::ScriptError(apCompileData, "Syntax Error.  Failed to store the 'begin' 'end' jump block.");
            return false;
        }
        case FLOW_SHORT:
        case FLOW_LONG:
        case FLOW_FLOAT:
        case FLOW_REF: {
            ScriptVariable* pNewVariable;
            if (!ScriptCompiler::GetNextWord(
                    apCompileData, &scriptWord, apScriptLine->pLine, apScriptLine->uiOffset, false, false
                )) {
                ScriptCompiler::ScriptError(apCompileData, "Missing variable name in variable declaration.");
                return false;
            }
            const char* pvarname = (&scriptWord)->pText;
            if (!ScriptCompiler::IsValidVariableName(scriptWord.pText)) {
                ScriptCompiler::ScriptError(
                    apCompileData, "Invalid variable name '%s'.\r\nCompiled script not saved!", pvarname
                );
                return false;
            }
            void* pMemory = ourMalloc(sizeof(ScriptVariable));
            pNewVariable = pMemory ? ::new (pMemory) ScriptVariable : nullptr;
            ScriptVariable* pNewVariable_2 = pNewVariable;
            bool v6 = apScriptLine->sExpression != FLOW_FLOAT && apScriptLine->sExpression != FLOW_REF;
            pNewVariable_2->data.bIsInteger = v6;
            pNewVariable_2->cName.Set(pvarname, 0);
            if (!apScript->FindVariable(pNewVariable_2->cName.pString, &pNewVariable_2->data.uiID)) {
                pNewVariable_2->data.uiID = ++apCompileData->header.m_uiLastID;
            }
            (&apCompileData->listVariables)->AddTail(pNewVariable_2);
            if (apScriptLine->sExpression == FLOW_REF) {
                scriptWord.uiVariableID = pNewVariable_2->data.uiID;
                apCompileData->GetReferencedObject(&scriptWord, true, false);
            }
            goto LABEL_92;
        }
        case FLOW_SET: {
            char expLine[512];
            if (!ScriptCompiler::GetNextWord(
                    apCompileData, &scriptWord, apScriptLine->pLine, apScriptLine->uiOffset, true, true
                )) {
                ScriptCompiler::ScriptError(apCompileData, "Missing variable name in set command.");
                return false;
            }
            if (!scriptWord.cVariableType || scriptWord.cVariableType == 88) {
                ScriptCompiler::ScriptError(apCompileData, "Unknown variable '%s'.", &scriptWord);
                return false;
            }
            scriptWord2.~SCRIPT_WORD();
            ::new (&scriptWord2) SCRIPT_WORD;
            ScriptCompiler::GetNextWord(
                apCompileData, &scriptWord2, apScriptLine->pLine, apScriptLine->uiOffset, false, false
            );
            if (std::tolower(scriptWord2.pText[0]) != 116 || std::tolower(scriptWord2.pText[1]) != 111) {
                ScriptCompiler::ScriptError(apCompileData, "Syntax Error.  Missing \"to\" in set command.");
                return false;
            }
            if (scriptWord.uiRefObjectIndex) {
                if (scriptWord.cVariableType == 71) {
                    apScriptLine->pOutput[apScriptLine->uiOutputSize] = 71;
                }
                else {
                    apScriptLine->pOutput[apScriptLine->uiOutputSize] = 114;
                }
                *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[++apScriptLine->uiOutputSize]) =
                    static_cast<uint16_t>(scriptWord.uiRefObjectIndex);
                apScriptLine->uiOutputSize += 2;
            }
            if (scriptWord.uiVariableID) {
                apScriptLine->pOutput[apScriptLine->uiOutputSize++] = scriptWord.cVariableType;
                *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) =
                    static_cast<uint16_t>(scriptWord.uiVariableID);
                apScriptLine->uiOutputSize += 2;
            }
            if (!ScriptCompiler::CheckParentheses(apScriptLine->pLine, apScriptLine->uiOffset)) {
                ScriptCompiler::ScriptError(apCompileData, "Syntax Error.  Mismatched parentheses in set command.");
                return false;
            }
            ScriptCompiler::SkipWhiteSpace(apScriptLine->pLine, apScriptLine->uiOffset);
            ourMemset(expLine, 0, 512u);
            uint32_t auiSize = 0;
            uiSize = ReplaceVariablesAndFunctions(
                apCompileData, apScriptLine, &apScriptLine->pLine[apScriptLine->uiOffset], expLine
            );
            if (!uiSize) {
                ScriptCompiler::ScriptError(apCompileData, "Syntax Error.  Missing expression in set command.");
                return false;
            }
            postfix = ce.InfixToPostfix(expLine, uiSize);
            if (ce.m_ExpressionError) {
                ScriptCompiler::ScriptError(apCompileData, "InfixToPostfix Error.");
                return false;
            }
            *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) =
                static_cast<uint16_t>(uiSize);
            apScriptLine->uiOutputSize += 2;
            ourMemcpy(&apScriptLine->pOutput[apScriptLine->uiOutputSize], postfix, uiSize);
            apScriptLine->uiOutputSize += uiSize;
            goto LABEL_92;
        }
        case FLOW_IF:
        case FLOW_ELSEIF: {
            char expLine_2[512];
            uint32_t uiOffset = 0;
            if (!ScriptCompiler::CheckParentheses(apScriptLine->pLine, 0)) {
                ScriptCompiler::ScriptError(apCompileData, "Mismatched parentheses.");
                return false;
            }
            ScriptCompiler::SkipWhiteSpace(apScriptLine->pLine, uiOffset);
            ourMemset(expLine_2, 0, 512u);
            uint32_t v14 = 0;
            uiSize_2 =
                ReplaceVariablesAndFunctions(apCompileData, apScriptLine, &apScriptLine->pLine[uiOffset], expLine_2);
            if (!uiSize_2) {
                return false;
            }
            postfix = ce.InfixToPostfix(expLine_2, uiSize_2);
            if (ce.m_ExpressionError) {
                ScriptCompiler::ScriptError(apCompileData, expressionErrors[ce.m_ExpressionError]);
                return false;
            }
            int32_t lines_2 = CountIfLines(apCompileData, apScriptLine);
            if (lines_2 == -1) {
                ScriptCompiler::ScriptError(apCompileData, "Mismatched if/then/else block.");
                return false;
            }
            *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) =
                static_cast<uint16_t>(lines_2);
            apScriptLine->uiOutputSize += 2;
            *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) =
                static_cast<uint16_t>(uiSize_2);
            apScriptLine->uiOutputSize += 2;
            ourMemcpy(&apScriptLine->pOutput[apScriptLine->uiOutputSize], postfix, uiSize_2);
            apScriptLine->uiOutputSize += uiSize_2;
            goto LABEL_92;
        }
        case FLOW_ELSE: {
            int32_t lines_3 = CountIfLines(apCompileData, apScriptLine);
            if (lines_3 == -1) {
                ScriptCompiler::ScriptError(apCompileData, "Mismatched if/then/else block.");
                return false;
            }
            *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) =
                static_cast<uint16_t>(lines_3);
            apScriptLine->uiOutputSize += 2;
            goto LABEL_92;
        }
        case FLOW_ENDIF:
        case FLOW_REFERENCE_FUNCTION:
        case FLOW_SCRIPTNAME:
        case FLOW_RETURN: {
            goto LABEL_92;
        }
        default: {
            SCRIPT_FUNCTION* pFunctionDef_2;
            pFunctionDef_2 = ScriptCompiler::GetFunctionDef(apScriptLine->sExpression);
            if (!pFunctionDef_2) {
                ScriptCompiler::ScriptError(apCompileData, "Unknown function code %d.", apScriptLine->sExpression);
                return false;
            }
            if (pFunctionDef_2->bReferenceFunction &&
                (apCompileData->header.bIsQuestScript && !apScriptLine->uiRefObjectIndex)) {
                ScriptCompiler::ScriptError(
                    apCompileData, "Reference function requires explicit reference in quest script."
                );
                return false;
            }
            if (pFunctionDef_2->bReferenceFunction) {
                if (apScriptLine->uiRefObjectIndex) {
                    Script::RefVariable* pScriptRefObj =
                        apCompileData->GetReferencedObject(apScriptLine->uiRefObjectIndex);
                    if (!dynamic_cast<TESObjectREFR*>(pScriptRefObj->form) && !pScriptRefObj->varIdx) {
                        ScriptCompiler::ScriptError(
                            apCompileData,
                            "Syntax error.  Invalid reference '%s' (only object references and reference variables are "
                            "allowed in this context).",
                            pScriptRefObj->name.String()
                        );
                        return false;
                    }
                }
            }
            if (pFunctionDef_2->pParameters) {
                uint16_t sCount_2 = pFunctionDef_2->sParamCount;
                if (!pFunctionDef_2->pCompileFunction(
                        sCount_2, pFunctionDef_2->pParameters, apScriptLine, apCompileData
                    )) {
                    return false;
                }
            }
        LABEL_92:;
            SCRIPT_OUTPUT sExpression = apScriptLine->sExpression;
            if (sExpression >= FLOW_SHORT && (sExpression <= FLOW_FLOAT || sExpression == FLOW_REF)) {
                goto LABEL_101;
            }
            // The compiled script's size limit.
            if (apCompileData->uiOutputOffset + apScriptLine->uiOutputSize + 10 < 0x4000) {
                if (apScriptLine->uiRefObjectIndex) {
                    *reinterpret_cast<uint16_t*>(&apCompileData->pOutput[apCompileData->uiOutputOffset]) = 28;
                    apCompileData->uiOutputOffset += 2;
                    *reinterpret_cast<uint16_t*>(&apCompileData->pOutput[apCompileData->uiOutputOffset]) =
                        static_cast<uint16_t>(apScriptLine->uiRefObjectIndex);
                    apCompileData->uiOutputOffset += 2;
                }
                *reinterpret_cast<uint16_t*>(&apCompileData->pOutput[apCompileData->uiOutputOffset]) =
                    static_cast<uint16_t>(apScriptLine->sExpression);
                apCompileData->uiOutputOffset += 2;
                *reinterpret_cast<uint16_t*>(&apCompileData->pOutput[apCompileData->uiOutputOffset]) =
                    static_cast<uint16_t>(apScriptLine->uiOutputSize);
                apCompileData->uiOutputOffset += 2;
                ourMemcpy(
                    &apCompileData->pOutput[apCompileData->uiOutputOffset], apScriptLine->pOutput,
                    apScriptLine->uiOutputSize
                );
                apCompileData->uiOutputOffset += apScriptLine->uiOutputSize;
            LABEL_101:;
                return true;
            }
            else {
                ScriptCompiler::ScriptError(apCompileData, "MAX_SCRIPT_SIZE exceeded.\r\nCompiled script not saved!");
                return false;
            }
        }
    }
}

SCRIPT_OUTPUT ScriptCompiler::GetNextExpression(ScriptCompileData* apCompileData) {
    if (apCompileData->uiInputOffset >= std::strlen(apCompileData->input)) {
        return SCRIPT_DONE;
    }

    void* pMemory = ourMalloc(sizeof(SCRIPT_LINE));
    SCRIPT_LINE* pScriptLine = pMemory ? ::new (pMemory) SCRIPT_LINE : nullptr;
    pScriptLine->uiLineNumber = apCompileData->uiLastLineNumber + 1;

    int32_t iLineSize = GetNextLine(apCompileData, pScriptLine);
    if (!iLineSize) {
        apCompileData->eLastError = pScriptLine->eScriptError;
        ourFree(pScriptLine);
        return SCRIPT_DONE;
    }
    apCompileData->uiInputOffset += iLineSize;

    // The first word names the expression the line starts with.
    SCRIPT_WORD kWord;
    kWord.~SCRIPT_WORD();
    ::new (&kWord) SCRIPT_WORD;
    if (!GetNextWord(apCompileData, &kWord, pScriptLine->pLine, pScriptLine->uiOffset, true, true)) {
        ScriptError(apCompileData, "Syntax Error\r\n%s\r\nCould not parse this line.", pScriptLine->pLine);
        ourFree(pScriptLine);
        return SCRIPT_DONE;
    }
    pScriptLine->uiRefObjectIndex = kWord.uiRefObjectIndex;

    bool bValid = kWord.cVariableType == 'X';
    if (bValid) {
        // Console functions only compile in the console.
        if (kWord.eFunctionCode >= 0x100 && kWord.eFunctionCode < 0x1CD &&
            apCompileData->eCompilerIndex != SYSTEM_WINDOW_COMPILER) {
            bValid = false;
            ScriptError(apCompileData, "Script command \"%s\" is a console-only command.", kWord.pText);
        }
    }
    else {
        ++apCompileData->uiLastLineNumber;
        ScriptError(apCompileData, "Script command \"%s\" not found.", kWord.pText);
        apCompileData->eLastError = ERROR_SCRIPT_UNHANDLEDCOMMAND;
    }
    if (!bValid) {
        ourFree(pScriptLine);
        return SCRIPT_DONE;
    }

    // The line keeps only what follows its first word.
    pScriptLine->sExpression = kWord.eFunctionCode;
    BSstrcpy(pScriptLine->pLine, 0x200, &pScriptLine->pLine[pScriptLine->uiOffset]);
    pScriptLine->uiSize -= pScriptLine->uiOffset;
    apCompileData->uiLastLineNumber = pScriptLine->uiLineNumber;

    if (pScriptLine->sExpression == FLOW_SCRIPTNAME) {
        uint32_t uiOffset = 0;
        SCRIPT_WORD kName;
        kName.~SCRIPT_WORD();
        ::new (&kName) SCRIPT_WORD;
        if (!GetNextWord(apCompileData, &kName, pScriptLine->pLine, uiOffset, false, false)) {
            ScriptError(apCompileData, "Syntax Error.  Missing script name.");
            apCompileData->eLastError = ERROR_SCRIPT_SCRIPT_NAME_MISSING;
            ourFree(pScriptLine);
            return SCRIPT_DONE;
        }
        apCompileData->cScriptName.Set(kName.pText, 0);
    }

    if ((pScriptLine->sExpression == FLOW_IF || pScriptLine->sExpression == FLOW_ELSEIF) && !pScriptLine->pLine[0]) {
        ScriptError(apCompileData, "Syntax Error.  No condition specified for 'if' or 'elseif'.");
        apCompileData->eLastError = ERROR_SCRIPT_EXPRESSION;
        ourFree(pScriptLine);
        return SCRIPT_DONE;
    }

    apCompileData->QScriptLineList()->AddTail(pScriptLine);
    return pScriptLine->sExpression;
}

bool ScriptCompiler::VerifyBlocks(ScriptCompileData* apCompileData) {
    bool bInsideBlock = false;
    BSSimpleList<SCRIPT_LINE*>* pList = &apCompileData->listLines;
    while (pList && !pList->IsEmpty()) {
        SCRIPT_LINE* pCurrentLine = pList->m_item;
        switch (pCurrentLine->sExpression) {
            case FLOW_BEGIN: {
                if (!bInsideBlock) {
                    bInsideBlock = true;
                    goto LABEL_13;
                }
                apCompileData->uiLastLineNumber = pCurrentLine->uiLineNumber;
                ScriptCompiler::ScriptError(apCompileData, "Nested begin/end blocks are not allowed.");
                return false;
            }
            case FLOW_END: {
                if (bInsideBlock) {
                    bInsideBlock = false;
                    goto LABEL_13;
                }
                apCompileData->uiLastLineNumber = pCurrentLine->uiLineNumber;
                ScriptCompiler::ScriptError(apCompileData, "End without matching Begin.");
                return false;
            }
            case FLOW_SHORT:
            case FLOW_LONG:
            case FLOW_FLOAT:
            case FLOW_SCRIPTNAME:
            case FLOW_REF: {
                goto LABEL_13;
            }
            default: {
                if (!bInsideBlock) {
                    apCompileData->uiLastLineNumber = pCurrentLine->uiLineNumber;
                    ScriptCompiler::ScriptError(apCompileData, "All script commands must be inside a begin/end block.");
                    return false;
                }
            LABEL_13:;
                pList = pList->m_pkNext;
                break;
            }
        }
    }
    return true;
}

int32_t ScriptCompiler::CountBlockLines(ScriptCompileData* apCompileData, SCRIPT_LINE* apStartingLine) {
    int32_t iCount = 0;
    bool bFoundEnd = false;
    BSSimpleList<SCRIPT_LINE*>* pLine = apCompileData->QScriptLineList();
    while (pLine && !pLine->IsEmpty() && !bFoundEnd) {
        SCRIPT_LINE* pScriptLine = static_cast<SCRIPT_LINE*>(pLine->GetItem());
        // Lines up to and including the one the block starts at are not part of the block.
        if (pScriptLine->uiLineNumber > apStartingLine->uiLineNumber) {
            switch (pScriptLine->sExpression) {
                case SCRIPT_DONE:
                    ScriptError(
                        apCompileData, "Begin without matching end starting on line %d.", apStartingLine->uiLineNumber
                    );
                    return -1;
                case FLOW_BEGIN: ScriptError(apCompileData, "Nested begin/end blocks are not allowed."); return -1;
                case FLOW_END: bFoundEnd = true; break;
                // A variable declaration opens no block, so it neither counts nor ends one.
                case FLOW_SHORT:
                case FLOW_LONG:
                case FLOW_FLOAT: break;
                default: ++iCount; break;
            }
        }
        pLine = pLine->GetNext();
    }
    if (!bFoundEnd) {
        ScriptError(apCompileData, "Mismatched begin/end block starting on line %d.", apStartingLine->uiLineNumber);
        return -1;
    }
    return iCount;
}

// The word's codes are written as unaligned 16-bit values into apExpLine.
uint32_t ScriptCompiler::ReplaceVariablesAndFunctions(
    ScriptCompileData* apCompileData,
    const SCRIPT_LINE* apScriptLine,
    char* apPostfix,
    char* apExpLine
) {
    uint32_t uiLength = BSstrlen(apPostfix);
    uint32_t uiOut = 0;
    for (uint32_t i = 0; i < uiLength; ++i) {
        if (std::isalpha(apPostfix[i]) || apPostfix[i] == '"') {
            SCRIPT_WORD kWord;
            kWord.~SCRIPT_WORD();
            ::new (&kWord) SCRIPT_WORD;
            if (!GetNextWord(apCompileData, &kWord, apPostfix, i, true, true)) {
                return 0;
            }
            if (kWord.uiRefObjectIndex > 0) {
                if (kWord.cVariableType == 'G') {
                    apExpLine[uiOut++] = 'G';
                }
                else {
                    apExpLine[uiOut++] = 'r';
                }
                *reinterpret_cast<uint16_t*>(apExpLine + uiOut) = static_cast<uint16_t>(kWord.uiRefObjectIndex);
                uiOut += 2;
            }
            if (kWord.uiVariableID > 0) {
                apExpLine[uiOut++] = kWord.cVariableType;
                *reinterpret_cast<uint16_t*>(apExpLine + uiOut) = static_cast<uint16_t>(kWord.uiVariableID);
                uiOut += 2;
            }
            else if (kWord.cVariableType == 'X') {
                apExpLine[uiOut++] = 'X';
                *reinterpret_cast<uint16_t*>(apExpLine + uiOut) = static_cast<uint16_t>(kWord.eFunctionCode);
                uiOut += 2;
                SCRIPT_FUNCTION* pFunction = GetFunctionDef(kWord.eFunctionCode);
                if (!pFunction) {
                    ScriptError(apCompileData, "Syntax Error.  Undefined function '%s'.", kWord.pText);
                    return 0;
                }
                if (pFunction->bReferenceFunction && apCompileData->header.bIsQuestScript && !kWord.uiRefObjectIndex) {
                    ScriptError(
                        apCompileData, "Reference function '%s' requires explicit reference in quest script.",
                        kWord.pText
                    );
                    return 0;
                }
                if (pFunction->pParameters) {
                    SCRIPT_PARAMETER* pParameters = pFunction->pParameters;
                    bool (*pCompileFunction)(uint16_t, const SCRIPT_PARAMETER*, SCRIPT_LINE*, ScriptCompileData*) =
                        pFunction->pCompileFunction;
                    uint16_t usParamCount = static_cast<uint16_t>(pFunction->sParamCount);
                    SCRIPT_LINE kLine;
                    kLine.~SCRIPT_LINE();
                    ::new (&kLine) SCRIPT_LINE;
                    BSstrcpy(kLine.pLine, 512, apPostfix + i);
                    kLine.uiOutputSize = 0;
                    kLine.uiLineNumber = apScriptLine->uiLineNumber;
                    if (!pCompileFunction(usParamCount, pParameters, &kLine, apCompileData)) {
                        return 0;
                    }
                    *reinterpret_cast<uint16_t*>(apExpLine + uiOut) = static_cast<uint16_t>(kLine.uiOutputSize);
                    uiOut += 2;
                    ourMemcpy(apExpLine + uiOut, kLine.pOutput, kLine.uiOutputSize);
                    uiOut += kLine.uiOutputSize;
                    i += kLine.uiOffset;
                }
                else {
                    *reinterpret_cast<uint16_t*>(apExpLine + uiOut) = 0;
                    uiOut += 2;
                }
            }
            else if (kWord.cVariableType != 'G') {
                if (kWord.uiRefObjectIndex || !apCompileData->GetReferencedObject(&kWord, false, false)) {
                    ScriptError(apCompileData, "Syntax Error.  Unknown command '%s'.", kWord.pText);
                    return 0;
                }
                apExpLine[uiOut++] = 'Z';
                *reinterpret_cast<uint16_t*>(apExpLine + uiOut) = static_cast<uint16_t>(kWord.uiRefObjectIndex);
                uiOut += 2;
            }
        }
        if (i < uiLength) {
            apExpLine[uiOut++] = apPostfix[i];
        }
    }
    apExpLine[uiOut + 1] = 0;
    return uiOut;
}

int32_t ScriptCompiler::CountIfLines(ScriptCompileData* apCompileData, SCRIPT_LINE* apStartingLine) {
    int32_t count = 0;
    int32_t nested = 0;
    int32_t done = 0;
    for (BSSimpleList<SCRIPT_LINE*>* pList = &apCompileData->listLines; pList && (!pList->IsEmpty() && !done);
         pList = pList->m_pkNext) {
        SCRIPT_LINE* pCurrentLine = pList->m_item;
        if (pCurrentLine->uiLineNumber <= apStartingLine->uiLineNumber) {
            continue;
        }
        SCRIPT_OUTPUT exp = pCurrentLine->sExpression;
        if (exp > FLOW_ELSEIF) {
            if (exp != FLOW_ENDIF) {
                if (exp == SCRIPT_DONE) {
                    ScriptCompiler::ScriptError(apCompileData, "Mismatched if/else/endif.");
                    return -1;
                }
            LABEL_25:;
                ++count;
                continue;
            }
            if (nested) {
                --nested;
                ++count;
            }
            else {
                done = 1;
            }
        }
        else {
            if (exp >= FLOW_ELSE) {
                if (nested) {
                    ++count;
                }
                else {
                    done = 1;
                }
            }
            else {
                if (exp < FLOW_SHORT) {
                    goto LABEL_25;
                }
                if (exp > FLOW_FLOAT) {
                    if (exp != FLOW_IF) {
                        goto LABEL_25;
                    }
                    ++count;
                    if (++nested >= 10) {
                        ScriptCompiler::ScriptError(apCompileData, "Max nesting of %d exceeded.", 10);
                        return -1;
                    }
                }
            }
        }
    }
    if (done) {
        return count;
    }
    ScriptCompiler::ScriptError(
        apCompileData, "Mismatched if/else/endif block starting on line %d.", apStartingLine->uiLineNumber
    );
    return -1;
}

SCRIPT_FUNCTION* ScriptCompiler::GetFunctionDef(SCRIPT_OUTPUT aeExpression) {
    // The console's functions live at 0x100, the scripts' ones at 0x1000; each table ends at its own last index.
    if (aeExpression >= 0x100 && aeExpression < 0x1CD) {
        return &scriptConsole[aeExpression - 0x100];
    }
    if (aeExpression >= 0x1000 && aeExpression < 0x1280) {
        return &scriptFunctions[aeExpression - 0x1000];
    }
    return nullptr;
}

char ScriptCompiler::SkipWhiteSpace(const char* apExpLine, uint32_t& auiLineOffset) {
    char cChar;
    while (true) {
        cChar = apExpLine[auiLineOffset];
        if (cChar != '\t' && cChar != ' ' && cChar != ',') {
            break;
        }
        ++auiLineOffset;
    }
    return cChar;
}

// The message is passed to PrintLine/Warning as their format.
void ScriptCompiler::ScriptError(ScriptCompileData* apCompileData, const char* text, ...) {
    char cBuffer[260];
    va_list kArgs;
    va_start(kArgs, text);
    BSvsprintf(cBuffer, text, kArgs);
    va_end(kArgs);

    alignas(BSStringT<char>) uint8_t aucMessage[sizeof(BSStringT<char>)];
    BSStringT<char>* pMessage = ::new (aucMessage) BSStringT<char>;
    pMessage->SPrintF(
        "SCRIPTS: Script '%s', line %d:\n%s", apCompileData->cScriptName.String(), apCompileData->uiLastLineNumber,
        cBuffer
    );
    if (apCompileData->eCompilerIndex == SYSTEM_WINDOW_COMPILER) {
        Interface::PrintLine(NavMeshInfo::GetCellOrCoords(const_cast<char*>(pMessage->String()), 0));
    }
    else {
        Warning(NavMeshInfo::GetCellOrCoords(const_cast<char*>(pMessage->String()), 0));
    }
    pMessage->~BSStringT<char>();
}

int32_t ScriptCompiler::GetNextLine(ScriptCompileData* apCompileData, SCRIPT_LINE* pScriptLine) {
    const char* pInput = &apCompileData->input[apCompileData->uiInputOffset];
    int32_t iOffset = 0;
    // Leading blanks are skipped until the first letter, digit or quote.
    bool bSkipping = true;
    while (true) {
        // The character is kept unsigned so isalnum gets it zero-extended.
        uint8_t cChar = pInput[iOffset++];
        if (cChar) {
            if (cChar == ';') {
                // A comment runs to the end of the line.
                while (cChar && cChar != '\n') {
                    cChar = pInput[iOffset++];
                }
            }
            else if (cChar != '\r') {
                if (pScriptLine->uiSize + 1 >= 0x200) {
                    ScriptError(apCompileData, "Max script line length (%d characters) exceeded.", 0x200);
                    pScriptLine->eScriptError = ERROR_SCRIPT_LINE_TOO_LONG;
                    return 0;
                }
                if (std::isalnum(cChar) || cChar == '"') {
                    bSkipping = false;
                }
                if (!bSkipping) {
                    pScriptLine->pLine[pScriptLine->uiSize++] = cChar;
                }
                continue;
            }
            if (cChar && pInput[iOffset] == '\n') {
                ++iOffset;
            }
        }

        // The end of a line: a line with code ends without trailing blanks; an empty one is started over.
        if (!bSkipping) {
            while (pScriptLine->uiSize &&
                   (pScriptLine->pLine[pScriptLine->uiSize - 1] == ' ' ||
                    pScriptLine->pLine[pScriptLine->uiSize - 1] == '\t')) {
                --pScriptLine->uiSize;
                pScriptLine->pLine[pScriptLine->uiSize] = '\0';
            }
            if (pScriptLine->uiSize) {
                pScriptLine->pLine[pScriptLine->uiSize] = '\0';
                return iOffset;
            }
        }
        else if (!cChar) {
            return 0;
        }
        ++pScriptLine->uiLineNumber;
        pScriptLine->uiSize = 0;
        ourMemset(pScriptLine->pLine, 0, 0x200);
    }
}

bool ScriptCompiler::GetFunction(SCRIPT_WORD* apWord) {
    // The flow keywords are compared with the table name first, the function tables with the word first.
    for (int32_t i = 0; i < FLOW_COUNT; ++i) {
        if (!BSstricmp(flow[i].pFunctionName, apWord->pText) || !BSstricmp(flow[i].pShortName, apWord->pText)) {
            apWord->cVariableType = 'X';
            apWord->eFunctionCode = flow[i].eOutput;
            return true;
        }
    }
    for (int32_t i = 0; i < 0x280; ++i) {
        if (!BSstricmp(apWord->pText, scriptFunctions[i].pFunctionName) ||
            !BSstricmp(apWord->pText, scriptFunctions[i].pShortName)) {
            apWord->cVariableType = 'X';
            apWord->eFunctionCode = scriptFunctions[i].eOutput;
            return true;
        }
    }
    for (int32_t i = 0; i < 0xCD; ++i) {
        if (!BSstricmp(apWord->pText, scriptConsole[i].pFunctionName) ||
            !BSstricmp(apWord->pText, scriptConsole[i].pShortName)) {
            apWord->cVariableType = 'X';
            apWord->eFunctionCode = scriptConsole[i].eOutput;
            return true;
        }
    }
    return false;
}

int32_t ScriptCompiler::GetNextWord(
    ScriptCompileData* apCompileData,
    SCRIPT_WORD* apNextWord,
    const char* apExpLine,
    uint32_t& auiLineOffset,
    bool abAllowVariable,
    bool abAllowUnknownRef
) {
    SkipWhiteSpace(apExpLine, auiLineOffset);
    int32_t iLength = 0;

    // A quoted word is the text between the quotes; the quotes are blanked out while it is copied.
    if (apExpLine[auiLineOffset] == '"') {
        if (!CheckQuotes(apExpLine, auiLineOffset)) {
            ScriptError(apCompileData, "Syntax Error.  Mismatched quotes.");
            return 0;
        }
        char* pOpen = const_cast<char*>(std::strchr(&apExpLine[auiLineOffset], '"'));
        *pOpen = ' ';
        char* pClose = const_cast<char*>(std::strchr(&apExpLine[auiLineOffset], '"'));
        *pClose = '\0';
        BSstrcpy(apNextWord->pText, 0x200, pOpen + 1);
        iLength = BSstrlen(apNextWord->pText);
        *pOpen = '"';
        *pClose = '"';
        auiLineOffset = iLength + auiLineOffset + 2;

        // "Name".word: the quoted name is a reference the rest is called on.
        if (apExpLine[auiLineOffset] == '.') {
            if (!abAllowVariable) {
                ScriptError(apCompileData, "Syntax Error.  Reference not allowed in this context.");
                return 0;
            }
            if (apNextWord->uiRefObjectIndex) {
                ScriptError(apCompileData, "Syntax Error.  Invalid reference syntax.");
                return 0;
            }
            ++auiLineOffset;
            if (!apCompileData->GetReferencedObject(apNextWord, false, abAllowUnknownRef)) {
                ScriptError(apCompileData, "Unknown object '%s'.", apNextWord->pText);
                return 0;
            }
            return GetNextWord(apCompileData, apNextWord, apExpLine, auiLineOffset, true, abAllowUnknownRef);
        }
        apNextWord->pText[iLength] = '\0';
    }

    if (!iLength) {
        while (std::isalnum(apExpLine[auiLineOffset]) || apExpLine[auiLineOffset] == '.' ||
               apExpLine[auiLineOffset] == '-' || apExpLine[auiLineOffset] == '_') {
            apNextWord->pText[iLength] = apExpLine[auiLineOffset];
            ++iLength;
            ++auiLineOffset;
            // name.word: a name starting with a letter is a reference the rest is called on.
            if (apNextWord->pText[iLength - 1] == '.' && std::isalpha(apNextWord->pText[0])) {
                if (!abAllowVariable) {
                    ScriptError(apCompileData, "Syntax Error.  Reference not allowed in this context.");
                    return 0;
                }
                if (apNextWord->uiRefObjectIndex) {
                    ScriptError(apCompileData, "Syntax Error.  Invalid reference syntax.");
                    return 0;
                }
                apNextWord->pText[iLength - 1] = '\0';
                if (!apCompileData->GetReferencedObject(apNextWord, false, abAllowUnknownRef)) {
                    ScriptError(apCompileData, "Unknown object '%s'.", apNextWord->pText);
                    return 0;
                }
                return GetNextWord(apCompileData, apNextWord, apExpLine, auiLineOffset, true, abAllowUnknownRef);
            }
            apNextWord->pText[iLength] = '\0';
        }
    }

    if (abAllowVariable) {
        // A word called on a reference must be a variable or function, and the reference a quest or a reference;
        // quests have no functions.
        if (apNextWord->uiRefObjectIndex) {
            if (!apCompileData->GetVariable(apNextWord) && !GetFunction(apNextWord)) {
                ScriptError(apCompileData, "Unknown variable or function '%s'.", apNextWord->pText);
                return 0;
            }
            Script::RefVariable* pRefObject = apCompileData->GetReferencedObject(apNextWord->uiRefObjectIndex);
            TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(pRefObject->form);
            TESQuest* pQuest = dynamic_cast<TESQuest*>(pRefObject->form);
            if (!pRef && !pQuest && !abAllowUnknownRef) {
                ScriptError(
                    apCompileData,
                    "Syntax error.  Invalid reference '%s' (only quests and object references are allowed in this "
                    "context).",
                    pRefObject->form->GetFormEditorID()
                );
                return 0;
            }
            if (pQuest && apNextWord->cVariableType == 'X') {
                ScriptError(apCompileData, "Syntax error.  Functions may not be called on quests.");
                return 0;
            }
        }
        if (!apNextWord->cVariableType && !apCompileData->GetVariable(apNextWord)) {
            GetFunction(apNextWord);
        }
    }
    return iLength;
}

bool ScriptCompiler::CheckQuotes(const char* apExpLine, uint32_t auiLineOffset) {
    int32_t v4 = 0;
    while (apExpLine[auiLineOffset]) {
        if (apExpLine[auiLineOffset] == 34) {
            ++v4;
        }
        ++auiLineOffset;
    }
    return v4 >= 2 && (v4 & 1) == 0;
}

bool ScriptCompiler::IsValidVariableName(const char* apVarName) {
    SCRIPT_WORD Dst;
    if (!apVarName || !*apVarName) {
        return false;
    }
    if (TESForm::GetFormByEditorID(apVarName)) {
        return false;
    }
    Dst.~SCRIPT_WORD();
    ::new (&Dst) SCRIPT_WORD;
    BSstrcpy(Dst.pText, 512, apVarName);
    return !ScriptCompiler::GetFunction(&Dst);
}

bool ScriptCompiler::CheckParentheses(const char* apExpLine, uint32_t auiLineOffset) {
    int32_t v5 = 0;
    while (apExpLine[auiLineOffset]) {
        char v3 = apExpLine[auiLineOffset];
        if (v3 == 40) {
            ++v5;
        }
        else {
            if (v3 == 41) {
                --v5;
            }
        }
        ++auiLineOffset;
    }
    return v5 == 0;
}

// pOutput is the compiled line's untyped byte buffer: each parameter's 16-bit, 32-bit and double values are stored into
// it at any byte offset, hence the casts of its bytes.
bool ScriptCompiler::StandardCompile(
    const uint16_t asParamCount,
    const SCRIPT_PARAMETER* apParameters,
    SCRIPT_LINE* apScriptLine,
    ScriptCompileData* apCompileData
) {
    int16_t sCurrentParam;
    apScriptLine->uiOffset = 0;
    char* pActualParams = &apScriptLine->pOutput[apScriptLine->uiOutputSize];
    *reinterpret_cast<uint16_t*>(pActualParams) = asParamCount;
    apScriptLine->uiOutputSize += 2;
    for (sCurrentParam = 0; sCurrentParam < static_cast<int32_t>(asParamCount); ++sCurrentParam) {
        bool bLookForVariable = scriptParameterDefs[apParameters[sCurrentParam].eParamType].bCanBeVariable;
        SCRIPT_WORD currentWord;
        int32_t size = ScriptCompiler::GetNextWord(
            apCompileData, &currentWord, apScriptLine->pLine, apScriptLine->uiOffset, bLookForVariable, false
        );
        if (!size) {
            if (apParameters[sCurrentParam].bOptional) {
                *reinterpret_cast<uint16_t*>(pActualParams) = sCurrentParam;
                return true;
            }
            else {
                ScriptCompiler::ScriptError(
                    apCompileData, "Missing parameter %s.\r\nCompiled script not saved!",
                    apParameters[sCurrentParam].pParamName
                );
                return false;
            }
        }
        if (!bLookForVariable && (currentWord.uiRefObjectIndex || currentWord.cVariableType)) {
            ScriptCompiler::ScriptError(
                apCompileData, "Parameter %s may not be a variable.\r\nCompiled script not saved!",
                apParameters[sCurrentParam].pParamName
            );
            return false;
        }
        if (scriptParameterDefs[apParameters[sCurrentParam].eParamType].bReferencedObject &&
            (!apCompileData->GetReferencedObject(&currentWord, false, false) || !currentWord.uiRefObjectIndex)) {
            ScriptCompiler::ScriptError(
                apCompileData, "Item '%s' not found for parameter %s.\r\nCompiled script not saved!", &currentWord,
                apParameters[sCurrentParam].pParamName
            );
            return false;
        }
        if (scriptParameterDefs[apParameters[sCurrentParam].eParamType].bReferencedObject) {
            switch (apParameters[sCurrentParam].eParamType) {
                case SCRIPT_PARAM_INVENTORY_OBJECT: {
                    if (currentWord.uiVariableID) {
                        goto LABEL_274;
                    }
                    if (currentWord.pForm) {
                        uint8_t FormType = currentWord.pForm->GetFormType();
                        if (TESContainer::ContainerCanHoldType(FormType)) {
                            goto LABEL_274;
                        }
                    }
                    goto LABEL_21;
                }
                case SCRIPT_PARAM_OBJECTREF:
                case SCRIPT_PARAM_ACTOR:
                case SCRIPT_PARAM_MAP_MARKER:
                case SCRIPT_PARAM_CONTAINER_REF: {
                    SCRIPT_PARAM_TYPE eParamType;
                    TESObjectREFR* pRef;
                    if (currentWord.uiVariableID) {
                        goto LABEL_274;
                    }
                    pRef = dynamic_cast<TESObjectREFR*>(currentWord.pForm);
                    if (!pRef) {
                        ScriptCompiler::ScriptError(
                            apCompileData,
                            "Invalid object reference '%s' for parameter %s.\r\nCompiled script not saved!",
                            &currentWord, apParameters[sCurrentParam].pParamName
                        );
                        return false;
                    }
                    eParamType = apParameters[sCurrentParam].eParamType;
                    if (eParamType == SCRIPT_PARAM_ACTOR) {
                        if (!pRef->IsActor()) {
                            ScriptCompiler::ScriptError(
                                apCompileData, "Invalid actor '%s' for parameter %s.\r\nCompiled script not saved!",
                                &currentWord, apParameters[sCurrentParam].pParamName
                            );
                            return false;
                        }
                    }
                    else {
                        if (eParamType == SCRIPT_PARAM_MAP_MARKER) {
                            TESObjectSTAT* ObjectReference = static_cast<TESObjectSTAT*>(pRef->GetObjectReference());
                            if (ObjectReference != pMapMarker) {
                                ScriptCompiler::ScriptError(
                                    apCompileData,
                                    "Invalid map marker '%s' for parameter %s.\r\nCompiled script not saved!",
                                    &currentWord, apParameters[sCurrentParam].pParamName
                                );
                                return false;
                            }
                        }
                        else {
                            if (eParamType == SCRIPT_PARAM_CONTAINER_REF && !pRef->HasContainer()) {
                                ScriptCompiler::ScriptError(
                                    apCompileData,
                                    "Invalid container reference '%s' for parameter %s.\r\nCompiled script not saved!",
                                    &currentWord, apParameters[sCurrentParam].pParamName
                                );
                                return false;
                            }
                        }
                    }
                LABEL_274:;
                    apScriptLine->pOutput[apScriptLine->uiOutputSize++] = 114;
                    *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) =
                        currentWord.uiRefObjectIndex;
                    apScriptLine->uiOutputSize += 2;
                    break;
                }
                case SCRIPT_PARAM_SPELL_ITEM: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm &&
                         (dynamic_cast<SpellItem*>(currentWord.pForm) ||
                          dynamic_cast<TESObjectBOOK*>(currentWord.pForm)))) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid spell item '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_CELL: {
                    TESObjectCELL* pCell = dynamic_cast<TESObjectCELL*>(currentWord.pForm);
                    if (currentWord.uiVariableID || (pCell && pCell->GetInterior())) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid interior cell '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_MAGIC_ITEM: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && dynamic_cast<MagicItem*>(currentWord.pForm))) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid magic item '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_SOUND: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == SOUN_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid sound '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_TOPIC: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == DIAL_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid topic '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_QUEST: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == QUST_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid info '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_RACE: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == RACE_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid race '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_CLASS: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == CLAS_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid class '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_FACTION: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == FACT_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid faction '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_GLOBAL: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == GLOB_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid global '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_FURNITURE_OR_FORMLIST: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm &&
                         (currentWord.pForm->GetFormType() == FURN_ID ||
                          currentWord.pForm->GetFormType() == FLST_ID))) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData,
                        "Invalid furniture object/list '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_OBJECT: {
                    if (!currentWord.uiVariableID &&
                        (!currentWord.pForm || !dynamic_cast<TESObject*>(currentWord.pForm))) {
                        goto LABEL_33;
                    }
                    goto LABEL_274;
                }
                case SCRIPT_PARAM_ACTOR_BASE: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && dynamic_cast<TESActorBase*>(currentWord.pForm))) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid actor base '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_WORLD: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && dynamic_cast<TESWorldSpace*>(currentWord.pForm))) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid worldspace '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_PACKAGE: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && dynamic_cast<TESPackage*>(currentWord.pForm))) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid package '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_COMBAT_STYLE: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && dynamic_cast<TESCombatStyle*>(currentWord.pForm))) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid combat style '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_MAGIC_EFFECT: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && dynamic_cast<EffectSetting*>(currentWord.pForm))) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid effect setting '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_WEATHER: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == WTHR_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid weather '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_NPC: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == NPC__ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid NPC '%s' for parameter %s.\r\nCompiled script not saved!", &currentWord,
                        apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_OWNER: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm &&
                         (currentWord.pForm->GetFormType() == NPC__ID ||
                          currentWord.pForm->GetFormType() == FACT_ID))) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid owner '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_SHADER_EFFECT: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == EFSH_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid Effect Shader '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_FORMLIST: {
                    BGSListForm* pformlist = dynamic_cast<BGSListForm*>(currentWord.pForm);
                    if (currentWord.uiVariableID || pformlist) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid form list '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_MENUICON: {
                    BGSMenuIcon* picon = dynamic_cast<BGSMenuIcon*>(currentWord.pForm);
                    if (currentWord.uiVariableID || picon) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid menu icon '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_PERK: {
                    BGSPerk* pperk = dynamic_cast<BGSPerk*>(currentWord.pForm);
                    if (currentWord.uiVariableID || pperk) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid perk '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_NOTE: {
                    BGSNote* pperk_2 = dynamic_cast<BGSNote*>(currentWord.pForm);
                    if (currentWord.uiVariableID || pperk_2) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid note '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_IMAGESPACEMOD: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == IMAD_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData,
                        "Invalid Imagespace Modifier '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_IMAGESPACE: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == IMGS_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid Imagespace '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_ENCOUNTERZONE: {
                    BGSEncounterZone* pzone = dynamic_cast<BGSEncounterZone*>(currentWord.pForm);
                    if (currentWord.uiVariableID || pzone) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid encounter zone '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_IDLE_FORM: {
                    TESIdleForm* pidle = dynamic_cast<TESIdleForm*>(currentWord.pForm);
                    if (currentWord.uiVariableID || pidle) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid Idle form'%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_MESSAGE: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == MESG_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid message '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_INVOBJECT_OR_FORMLIST: {
                    if (currentWord.uiVariableID) {
                        goto LABEL_274;
                    }
                    if (currentWord.pForm) {
                        uint8_t v6 = currentWord.pForm->GetFormType();
                        if (TESContainer::ContainerCanHoldType(v6) || currentWord.pForm->GetFormType() == FLST_ID) {
                            goto LABEL_274;
                        }
                    }
                LABEL_21:;
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid inventory object '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_OBJECT_OR_FORMLIST: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm &&
                         (currentWord.pForm->IsObject() || currentWord.pForm->GetFormType() == FLST_ID))) {
                        goto LABEL_274;
                    }
                LABEL_33:;
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid object '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_MUSIC: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == MUSC_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid music type '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_NPC_OR_LEVCHAR: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm &&
                         (currentWord.pForm->GetFormType() == NPC__ID ||
                          currentWord.pForm->GetFormType() == LVLN_ID))) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData,
                        "Invalid NPC or LeveledCharacter '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_CREA_OR_LEVCREA: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm &&
                         (currentWord.pForm->GetFormType() == CREA_ID ||
                          currentWord.pForm->GetFormType() == LVLC_ID))) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData,
                        "Invalid Creature or LeveledCreature '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_LEVCHAR: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == LVLN_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid LeveledCharacter '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_LEVCREA: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == LVLC_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid LeveledCreature '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_LEVITEM: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == LVLI_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid LeveledItem '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_FORM: {
                    if (currentWord.uiVariableID || currentWord.pForm) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid form '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_REPUTATION: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == REPU_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid reputation '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_CASINO: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == CSNO_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid casino '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_CASINOCHIP: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == CHIP_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid casino chip '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_CHALLENGE: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == CHAL_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid challenge '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_CARAVANMONEY: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == CMNY_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid caravan money '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_CARAVANCARD: {
                    if (!currentWord.uiVariableID &&
                        (!currentWord.pForm || currentWord.pForm->GetFormType() != CCRD_ID)) {
                        goto LABEL_163;
                    }
                    goto LABEL_274;
                }
                case SCRIPT_PARAM_CARAVANDECK: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == CDCK_ID)) {
                        goto LABEL_274;
                    }
                LABEL_163:;
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid caravan card '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                case SCRIPT_PARAM_REGION: {
                    if (currentWord.uiVariableID ||
                        (currentWord.pForm && currentWord.pForm->GetFormType() == REGN_ID)) {
                        goto LABEL_274;
                    }
                    ScriptCompiler::ScriptError(
                        apCompileData, "Invalid region '%s' for parameter %s.\r\nCompiled script not saved!",
                        &currentWord, apParameters[sCurrentParam].pParamName
                    );
                    return false;
                }
                default: {
                    Warning(
                        "SCRIPTS: Param type '%d' (referenced object) unimplemented in "
                        "ScriptCompiler::StandardCompile.",
                        apParameters[sCurrentParam].eParamType
                    );
                    return false;
                }
            }
        }
        else {
            switch (apParameters[sCurrentParam].eParamType) {
                case SCRIPT_PARAM_CHAR: {
                    *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) = size;
                    apScriptLine->uiOutputSize += 2;
                    ourMemcpy(&apScriptLine->pOutput[apScriptLine->uiOutputSize], &currentWord, size);
                    apScriptLine->uiOutputSize += size;
                    continue;
                }
                case SCRIPT_PARAM_INT:
                case SCRIPT_PARAM_FLOAT:
                case SCRIPT_PARAM_STAGE: {
                    char* pOutput = apScriptLine->pOutput;
                    if (currentWord.uiRefObjectIndex) {
                        if (currentWord.cVariableType == 71) {
                            pOutput[apScriptLine->uiOutputSize] = 71;
                        }
                        else {
                            pOutput[apScriptLine->uiOutputSize] = 114;
                        }
                        *reinterpret_cast<uint16_t*>(&pOutput[++apScriptLine->uiOutputSize]) =
                            currentWord.uiRefObjectIndex;
                        apScriptLine->uiOutputSize += 2;
                    }
                    if (currentWord.cVariableType == 71) {
                        continue;
                    }
                    if (currentWord.uiVariableID) {
                        pOutput[apScriptLine->uiOutputSize++] = currentWord.cVariableType;
                        *reinterpret_cast<uint16_t*>(&pOutput[apScriptLine->uiOutputSize]) = currentWord.uiVariableID;
                        apScriptLine->uiOutputSize += 2;
                    }
                    else {
                        if (apParameters[sCurrentParam].eParamType == SCRIPT_PARAM_INT ||
                            apParameters[sCurrentParam].eParamType == SCRIPT_PARAM_STAGE) {
                            if (!IsStringInteger(currentWord.pText)) {
                                goto LABEL_288;
                            }
                            pOutput[apScriptLine->uiOutputSize++] = 110;
                            *reinterpret_cast<uint32_t*>(&pOutput[apScriptLine->uiOutputSize]) =
                                std::atol(currentWord.pText);
                            apScriptLine->uiOutputSize += 4;
                        }
                        else {
                            if (!IsStringFloat(currentWord.pText)) {
                            LABEL_288:;
                                ScriptCompiler::ScriptError(
                                    apCompileData,
                                    "Unknown variable '%s' for parameter %s.\r\nCompiled script not saved!",
                                    &currentWord, apParameters[sCurrentParam].pParamName
                                );
                                return false;
                            }
                            pOutput[apScriptLine->uiOutputSize++] = 122;
                            *reinterpret_cast<double*>(&pOutput[apScriptLine->uiOutputSize]) =
                                std::atof(currentWord.pText);
                            apScriptLine->uiOutputSize += 8;
                        }
                    }
                    continue;
                }
                case SCRIPT_PARAM_ACTOR_VALUE: {
                    ActorValue::Index efindActorValue = ActorValue::GetActorValueIndex(currentWord.pText);
                    if (efindActorValue == ActorValue::ACTOR_VALUE_COUNT) {
                        ScriptCompiler::ScriptError(
                            apCompileData, "Invalid actor value '%s' for parameter %s.\r\nCompiled script not saved!",
                            &currentWord, apParameters[sCurrentParam].pParamName
                        );
                        return false;
                    }
                    *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) = efindActorValue;
                    apScriptLine->uiOutputSize += 2;
                    continue;
                }
                case SCRIPT_PARAM_AXIS: {
                    char v14 = std::toupper(currentWord.pText[0]);
                    if (v14 != 88 && (v14 != 89 && v14 != 90)) {
                        ScriptCompiler::ScriptError(
                            apCompileData, "Axis (X,Y,Z) required for parameter %s.\r\nCompiled script not saved!",
                            apParameters[sCurrentParam].pParamName
                        );
                        return false;
                    }
                    apScriptLine->pOutput[apScriptLine->uiOutputSize++] = v14;
                    continue;
                }
                case SCRIPT_PARAM_ANIM_GROUP: {
                    ANIM_GROUP_ENUM animGroup = ANIM_GROUP_NONE;
                    ANIM_GROUP_ENUM egroup = ANIM_GROUP_IDLE;
                    while (true) {
                        if (egroup < ANIM_GROUP_COUNT) {
                            if (BSstricmp(currentWord.pText, animGroupInfo[egroup].pName)) {
                                egroup = static_cast<ANIM_GROUP_ENUM>(egroup + 1);
                                continue;
                            }
                            animGroup = egroup;
                        }
                        break;
                    }
                    if (animGroup == ANIM_GROUP_NONE) {
                        ScriptCompiler::ScriptError(
                            apCompileData,
                            "Animation group \"%s\" not found for parameter %s.\r\nCompiled script not saved!",
                            &currentWord, apParameters[sCurrentParam].pParamName
                        );
                        return false;
                    }
                    *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) = animGroup;
                    apScriptLine->uiOutputSize += 2;
                    continue;
                }
                case SCRIPT_PARAM_SEX: {
                    int32_t v13 = 2;
                    if (BSstricmp(currentWord.pText, sexName[0])) {
                        if (!BSstricmp(currentWord.pText, sexName[1])) {
                            v13 = 1;
                        }
                    }
                    else {
                        v13 = 0;
                    }
                    if (v13 == 2) {
                        ScriptCompiler::ScriptError(
                            apCompileData,
                            "Sex (Male, Female) required for parameter %s.\r\nCompiled script not saved!",
                            apParameters[sCurrentParam].pParamName
                        );
                        return false;
                    }
                    *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) = v13;
                    apScriptLine->uiOutputSize += 2;
                    continue;
                }
                case SCRIPT_PARAM_CRIME_TYPE: {
                    uint32_t v15;
                    if (!IsStringInteger(currentWord.pText) || (v15 = std::atol(currentWord.pText), v15 > 4)) {
                        ScriptCompiler::ScriptError(
                            apCompileData,
                            "Invalid crime type '%s' for parameter %s.  Crime type must be a numeric value from "
                            "0-%d.\r\nCompiled script not saved!",
                            &currentWord, apParameters[sCurrentParam].pParamName, 4
                        );
                        return false;
                    }
                    *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) = v15;
                    apScriptLine->uiOutputSize += 2;
                    continue;
                }
                case SCRIPT_PARAM_FORM_TYPE: {
                    uint8_t v10 = 0;
                    int32_t iobj = 0;
                    while (true) {
                        if (iobj < 87) {
                            if (BSstricmp(currentWord.pText, objectLabel[iobj])) {
                                ++iobj;
                                continue;
                            }
                            v10 = aObjectTypeFormTypes[iobj];
                        }
                        break;
                    }
                    if (!v10) {
                        ScriptCompiler::ScriptError(
                            apCompileData, "Form Type \"%s\" not found for parameter %s.\r\nCompiled script not saved!",
                            &currentWord, apParameters[sCurrentParam].pParamName
                        );
                        return false;
                    }
                    *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) = v10;
                    apScriptLine->uiOutputSize += 2;
                    break;
                }
                case SCRIPT_PARAM_MISC_STAT: {
                    int32_t MiscStatIDFromName = GetMiscStatIDFromName(currentWord.pText);
                    if (MiscStatIDFromName == 43) {
                        ScriptCompiler::ScriptError(
                            apCompileData, "Invalid misc stat '%s' for parameter %s.\r\nCompiled script not saved!",
                            &currentWord, apParameters[sCurrentParam].pParamName
                        );
                        return false;
                    }
                    *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) =
                        MiscStatIDFromName;
                    apScriptLine->uiOutputSize += 2;
                    continue;
                }
                case SCRIPT_PARAM_ALIGNMENT: {
                    int16_t i;
                    for (i = 0; i < 5 && BSstricmp(TESActorBaseData::pAlignmentNames[i], currentWord.pText); ++i) {
                        ;
                    }
                    if (i == 5) {
                        ScriptCompiler::ScriptError(
                            apCompileData, "Invalid alignment '%s' for parameter %s.\r\nCompiled script not saved!",
                            &currentWord, apParameters[sCurrentParam].pParamName
                        );
                        return false;
                    }
                    *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) = i;
                    apScriptLine->uiOutputSize += 2;
                    continue;
                }
                case SCRIPT_PARAM_EQUIPTYPE: {
                    int16_t sval;
                    for (sval = 0; sval < 14 && BSstricmp(BGSEquipType::pEquipTypeNames[sval], currentWord.pText);
                         ++sval) {
                        ;
                    }
                    if (sval == 14) {
                        ScriptCompiler::ScriptError(
                            apCompileData, "Invalid EquipType '%s' for parameter %s.\r\nCompiled script not saved!",
                            &currentWord, apParameters[sCurrentParam].pParamName
                        );
                        return false;
                    }
                    *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) = sval;
                    apScriptLine->uiOutputSize += 2;
                    continue;
                }
                case SCRIPT_PARAM_CRITSTAGE: {
                    int16_t j;
                    for (j = 0; j < 5 && BSstricmp(pCriticalStageNames[j], currentWord.pText); ++j) {
                        ;
                    }
                    if (j == 5) {
                        ScriptCompiler::ScriptError(
                            apCompileData, "Invalid CriticalStage '%s' for parameter %s.\r\nCompiled script not saved!",
                            &currentWord, apParameters[sCurrentParam].pParamName
                        );
                        return false;
                    }
                    *reinterpret_cast<uint16_t*>(&apScriptLine->pOutput[apScriptLine->uiOutputSize]) = j;
                    apScriptLine->uiOutputSize += 2;
                    continue;
                }
                default: {
                    Warning(
                        "SCRIPTS: Param type '%d' unimplemented in ScriptCompiler::StandardCompile.",
                        apParameters[sCurrentParam].eParamType
                    );
                    return false;
                }
            }
        }
    }
    if (apScriptLine->uiOffset >= apScriptLine->uiSize) {
        return true;
    }
    ScriptCompiler::ScriptError(
        apCompileData, "Expected end of line.\r\nCompiled script not saved!", apParameters[sCurrentParam].pParamName
    );
    return false;
}

bool ScriptCompileData::GetVariable(SCRIPT_WORD* apCurrentWord) {
    int32_t size = BSstrlen(apCurrentWord->pText);
    if (apCurrentWord->pForm) {
        Script* pScript = nullptr;
        TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(apCurrentWord->pForm);
        if (pRef) {
            pRef->InitScript();
            pScript = pRef->GetScriptPointer();
        }
        else {
            if (apCurrentWord->pForm) {
                if (apCurrentWord->pForm->GetFormType() == QUST_ID) {
                    pScript = TESScriptableForm::GetFormScript(apCurrentWord->pForm);
                }
            }
        }
        if (pScript) {
            apCurrentWord->cVariableType = pScript->FindVariable(apCurrentWord->pText, &apCurrentWord->uiVariableID);
            if (apCurrentWord->cVariableType) {
                return true;
            }
            else {
                apCurrentWord->uiVariableID = 0;
                return false;
            }
        }
        else {
            apCurrentWord->uiVariableID = 0;
            return false;
        }
    }
    else {
        if (apCurrentWord->uiRefObjectIndex) {
            apCurrentWord->uiVariableID = 0;
            apCurrentWord->cVariableType = 0;
            return false;
        }
        else {
            BSSimpleList<ScriptVariable*>* pList = &listVariables;
            apCurrentWord->uiVariableID = 0;
            while (pList && pList->m_item) {
                ScriptVariable* pVar = pList->m_item;
                if (!_strnicmp(pVar->cName.pString, apCurrentWord->pText, size)) {
                    char nextchar = pVar->cName.pString[size];
                    if (!std::isalnum(nextchar) && (nextchar != 45 && nextchar != 95)) {
                        if (pVar->data.bIsInteger) {
                            apCurrentWord->cVariableType = 115;
                        }
                        else {
                            apCurrentWord->cVariableType = 102;
                        }
                        apCurrentWord->uiVariableID = pVar->data.uiID;
                        return true;
                    }
                }
                pList = pList->m_pkNext;
            }
            if (pCurrentScript &&
                (apCurrentWord->cVariableType =
                     pCurrentScript->FindVariable(apCurrentWord->pText, &apCurrentWord->uiVariableID)) != 0) {
                return true;
            }
            else {
                if (pDataHandler->GetGlobal(apCurrentWord->pText) && GetReferencedObject(apCurrentWord, false, false)) {
                    apCurrentWord->uiVariableID = 0;
                    apCurrentWord->cVariableType = 71;
                    return true;
                }
                else {
                    apCurrentWord->uiVariableID = 0;
                    apCurrentWord->cVariableType = 0;
                    return false;
                }
            }
        }
    }
}

bool ScriptCompileData::GetReferencedObject(SCRIPT_WORD* apCurrentWord, bool abAddRefVariable, bool abAllowUnknownRef) {
    bool bFound = false;
    uint32_t n = 1;
    BSSimpleList<Script::RefVariable*>* pList = &listRefObjects;
    while (pList && pList->m_item) {
        if (!pList->m_item->name.StrCmp(apCurrentWord->pText, true)) {
            apCurrentWord->uiRefObjectIndex = n;
            apCurrentWord->pForm = pList->m_item->form;
            if (abAddRefVariable) {
                pList->m_item->varIdx = apCurrentWord->uiVariableID;
            }
            else {
                apCurrentWord->uiVariableID = pList->m_item->varIdx;
            }
            bFound = true;
            break;
        }
        pList = pList->m_pkNext;
        ++n;
    }
    if (bFound) {
        return bFound;
    }
    void* pMemory = ourMalloc(sizeof(Script::RefVariable));
    Script::RefVariable* pRefObject = pMemory ? ::new (pMemory) Script::RefVariable : nullptr;
    if (abAddRefVariable) {
        bFound = true;
    }
    else {
        if (!BSstricmp(apCurrentWord->pText, "Player")) {
            pRefObject->form = pPlayer;
            bFound = true;
        }
        if (!bFound) {
            pRefObject->form = TESForm::GetFormByEditorID(apCurrentWord->pText);
            if (pRefObject->form) {
                if (pRefObject->form->GetFormType() == GMST_ID) {
                    pRefObject->form = nullptr;
                }
                else {
                    bFound = true;
                }
            }
        }
        if (!bFound) {
            char* pStop = nullptr;
            uint32_t uiNumericID = BSstrtoul(apCurrentWord->pText, &pStop, 16);
            if (uiNumericID && pStop && !*pStop) {
                pRefObject->form = TESForm::GetFormByNumericID(uiNumericID);
                if (pRefObject->form) {
                    bFound = true;
                }
            }
        }
        if (bFound && eCompilerIndex != SYSTEM_WINDOW_COMPILER) {
            TESObjectREFR* pTestRef = dynamic_cast<TESObjectREFR*>(pRefObject->form);
            if (pTestRef && !pTestRef->GetRefPersists()) {
                ScriptCompiler::ScriptError(
                    this, "Non-persistent reference '%s' cannot be used in a script.", apCurrentWord->pText
                );
                bFound = false;
            }
        }
    }
    if (bFound || abAllowUnknownRef) {
        bFound = true;
        if (abAddRefVariable) {
            pRefObject->form = nullptr;
            pRefObject->varIdx = apCurrentWord->uiVariableID;
        }
        else {
            apCurrentWord->pForm = pRefObject->form;
            pRefObject->varIdx = 0;
        }
        pRefObject->name.Set(apCurrentWord->pText, 0);
        ++header.refObjectCount;
        listRefObjects.AddTail(pRefObject);
        apCurrentWord->uiRefObjectIndex = header.refObjectCount;
    }
    else if (pRefObject) {
        delete pRefObject;
    }
    return bFound;
}

Script::RefVariable* ScriptCompileData::GetReferencedObject(uint32_t aiIndex) {
    BSSimpleList<Script::RefVariable*>* ecx0;
    if (!aiIndex) {
        return nullptr;
    }
    if (aiIndex > header.refObjectCount) {
        return nullptr;
    }
    uint32_t v3 = 1;
    for (ecx0 = QReferencedObjectList(); ecx0 && (!ecx0->IsEmpty() && v3 < aiIndex); ecx0 = ecx0->m_pkNext) {
        ++v3;
    }
    return ecx0->m_item;
}
