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
#include <cstring>

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
    ScriptCompileData kCompileData;
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
    return Compile(apScript, &kCompileData);
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
            const auto pLines = apCompileData->QScriptLineList();
            if (!pLines || pLines->IsEmpty()) {
                ScriptError(apCompileData, "You need to start scripts with \"scriptname ScriptName\"");
                return false;
            }

            const auto eExpression = pLines->GetItem()->sExpression;
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

    for (SCRIPT_LINE* pScriptLine : apCompileData->QScriptLineList()->ItemsUntilNull()) {
        apCompileData->uiLastLineNumber = pScriptLine->uiLineNumber;
        if (!CompileLine(apScript, apCompileData, pScriptLine)) {
            return false;
        }
    }

    apCompileData->header.dataSize = apCompileData->uiOutputOffset;
    if (!apCompileData->VerifyReferencedObjectList()) {
        return false;
    }

    apScript->Init(apCompileData);
    apScript->SetIsCompiled(true);
    return true;
}

namespace {

    // The compiled output is a packed byte stream: its 16-bit and 32-bit values sit unaligned, so they are copied in
    // and out of it.
    template <typename T>
    T ReadPacked(const char* apSource) {
        T kValue;
        std::memcpy(&kValue, apSource, sizeof(T));
        return kValue;
    }

    template <typename T>
    void WritePacked(char* apDest, T aValue) {
        std::memcpy(apDest, &aValue, sizeof(T));
    }

    // Writes aValue into apBuffer at auiOffset and moves the offset past it.
    template <typename T>
    void AppendPacked(char* apBuffer, uint32_t& auiOffset, T aValue) {
        WritePacked(&apBuffer[auiOffset], aValue);
        auiOffset += sizeof(T);
    }

    // Copies auiSize bytes into apBuffer at auiOffset and moves the offset past them.
    void AppendBytes(char* apBuffer, uint32_t& auiOffset, const char* apBytes, uint32_t auiSize) {
        ourMemcpy(&apBuffer[auiOffset], apBytes, auiSize);
        auiOffset += auiSize;
    }

    // Appends aValue to the line's compiled operands.
    template <typename T>
    void Emit(SCRIPT_LINE* apScriptLine, T aValue) {
        AppendPacked(apScriptLine->pOutput, apScriptLine->uiOutputSize, aValue);
    }

    void EmitBytes(SCRIPT_LINE* apScriptLine, const char* apBytes, uint32_t auiSize) {
        AppendBytes(apScriptLine->pOutput, apScriptLine->uiOutputSize, apBytes, auiSize);
    }

} // namespace

// The switch builds the line's operands in apScriptLine->pOutput; what follows it appends the line to the script's
// compiled output.
bool ScriptCompiler::CompileLine(Script* apScript, ScriptCompileData* apCompileData, SCRIPT_LINE* apScriptLine) {
    // Locates the open block's length field for the block's 'end', which is a later call: the offset 'begin' was
    // compiled at plus the field's offset in that line's operands. 0 outside a block.
    static int32_t ibeginEndJumpOffset;

    apScriptLine->uiOutputSize = 0;
    apScriptLine->uiOffset = 0;
    switch (apScriptLine->sExpression) {
        case FLOW_BEGIN: {
            SCRIPT_WORD kBlockName;
            if (!GetNextWord(apCompileData, &kBlockName, apScriptLine->pLine, apScriptLine->uiOffset, false, false)) {
                ScriptError(apCompileData, "Syntax Error.  Missing block type in 'begin' command.");
                return false;
            }
            const SCRIPT_FUNCTION* pBlock = nullptr;
            for (const SCRIPT_FUNCTION& kCandidate : scriptBlocks) {
                if (!BSstricmp(kCandidate.pFunctionName, kBlockName.pText) ||
                    !BSstricmp(kCandidate.pShortName, kBlockName.pText)) {
                    pBlock = &kCandidate;
                    break;
                }
            }
            if (!pBlock) {
                ScriptError(apCompileData, "Syntax Error.  Invalid block type in 'begin' command.");
                return false;
            }
            Emit(apScriptLine, static_cast<uint16_t>(pBlock->eOutput));
            if (CountBlockLines(apCompileData, apScriptLine) == -1) {
                ScriptError(apCompileData, "Mismatched begin/end block.");
                return false;
            }
            // The block's length field, a placeholder until the block's parameters are compiled.
            const uint32_t uiLengthField = apScriptLine->uiOutputSize;
            ibeginEndJumpOffset = uiLengthField + apCompileData->uiOutputOffset;
            Emit(apScriptLine, uint32_t{0x55555555});
            if (pBlock->bReferenceFunction && apCompileData->header.bIsQuestScript) {
                ScriptError(apCompileData, "Invalid block type for quest script.");
                return false;
            }
            if (pBlock->pParameters) {
                // The parameters compile from the rest of the text, as a line of their own.
                SCRIPT_LINE kParameters;
                BSstrcpy(kParameters.pLine, sizeof(kParameters.pLine), &apScriptLine->pLine[apScriptLine->uiOffset]);
                kParameters.uiLineNumber = apScriptLine->uiLineNumber;
                if (!pBlock->pCompileFunction(pBlock->sParamCount, pBlock->pParameters, &kParameters, apCompileData)) {
                    return false;
                }
                AppendBytes(
                    apScriptLine->pOutput, apScriptLine->uiOutputSize, kParameters.pOutput, kParameters.uiOutputSize
                );
            }
            // The field takes the offset the line's operands end at; the block's 'end' turns it into a length.
            WritePacked(
                &apScriptLine->pOutput[uiLengthField], apScriptLine->uiOutputSize + apCompileData->uiOutputOffset
            );
            break;
        }
        case FLOW_END: {
            if (!ibeginEndJumpOffset) {
                ScriptError(apCompileData, "Syntax Error.  Failed to store the 'begin' 'end' jump block.");
                return false;
            }
            // In the compiled output the field sits behind its line's four-byte header.
            char* pLengthField = &apCompileData->pOutput[ibeginEndJumpOffset + 4];
            WritePacked(pLengthField, apCompileData->uiOutputOffset - ReadPacked<uint32_t>(pLengthField));
            ibeginEndJumpOffset = 0;
            break;
        }
        case FLOW_SHORT:
        case FLOW_LONG:
        case FLOW_FLOAT:
        case FLOW_REF: {
            SCRIPT_WORD kName;
            if (!GetNextWord(apCompileData, &kName, apScriptLine->pLine, apScriptLine->uiOffset, false, false)) {
                ScriptError(apCompileData, "Missing variable name in variable declaration.");
                return false;
            }
            if (!IsValidVariableName(kName.pText)) {
                ScriptError(apCompileData, "Invalid variable name '%s'.\r\nCompiled script not saved!", kName.pText);
                return false;
            }
            ScriptVariable* pVariable = ::new (ourMalloc(sizeof(ScriptVariable))) ScriptVariable;
            pVariable->data.bIsInteger =
                apScriptLine->sExpression != FLOW_FLOAT && apScriptLine->sExpression != FLOW_REF;
            pVariable->cName.Set(kName.pText, 0);
            // A variable the script already has keeps its ID.
            if (!apScript->FindVariable(pVariable->cName.pString, &pVariable->data.uiID)) {
                pVariable->data.uiID = ++apCompileData->header.m_uiLastID;
            }
            apCompileData->listVariables.AddTail(pVariable);
            if (apScriptLine->sExpression == FLOW_REF) {
                kName.uiVariableID = pVariable->data.uiID;
                apCompileData->GetReferencedObject(&kName, true, false);
            }
            // A declaration only registers its variable: nothing of it goes into the compiled output.
            return true;
        }
        case FLOW_SET: {
            SCRIPT_WORD kVariable;
            if (!GetNextWord(apCompileData, &kVariable, apScriptLine->pLine, apScriptLine->uiOffset, true, true)) {
                ScriptError(apCompileData, "Missing variable name in set command.");
                return false;
            }
            if (!kVariable.cVariableType || kVariable.cVariableType == 'X') {
                ScriptError(apCompileData, "Unknown variable '%s'.", kVariable.pText);
                return false;
            }
            SCRIPT_WORD kTo;
            GetNextWord(apCompileData, &kTo, apScriptLine->pLine, apScriptLine->uiOffset, false, false);
            if (std::tolower(kTo.pText[0]) != 't' || std::tolower(kTo.pText[1]) != 'o') {
                ScriptError(apCompileData, "Syntax Error.  Missing \"to\" in set command.");
                return false;
            }
            // The variable: the reference it is reached through, if any, then the variable itself.
            if (kVariable.uiRefObjectIndex) {
                Emit(apScriptLine, kVariable.cVariableType == 'G' ? 'G' : 'r');
                Emit(apScriptLine, static_cast<uint16_t>(kVariable.uiRefObjectIndex));
            }
            if (kVariable.uiVariableID) {
                Emit(apScriptLine, kVariable.cVariableType);
                Emit(apScriptLine, static_cast<uint16_t>(kVariable.uiVariableID));
            }
            if (!CheckParentheses(apScriptLine->pLine, apScriptLine->uiOffset)) {
                ScriptError(apCompileData, "Syntax Error.  Mismatched parentheses in set command.");
                return false;
            }
            SkipWhiteSpace(apScriptLine->pLine, apScriptLine->uiOffset);
            char pInfix[512]{};
            uint32_t uiSize = ReplaceVariablesAndFunctions(
                apCompileData, apScriptLine, &apScriptLine->pLine[apScriptLine->uiOffset], pInfix
            );
            if (!uiSize) {
                ScriptError(apCompileData, "Syntax Error.  Missing expression in set command.");
                return false;
            }
            // The conversion leaves the postfix expression's size in uiSize.
            CExpression kExpression;
            const char* pPostfix = kExpression.InfixToPostfix(pInfix, uiSize);
            if (kExpression.m_ExpressionError) {
                ScriptError(apCompileData, "InfixToPostfix Error.");
                return false;
            }
            Emit(apScriptLine, static_cast<uint16_t>(uiSize));
            AppendBytes(apScriptLine->pOutput, apScriptLine->uiOutputSize, pPostfix, uiSize);
            break;
        }
        case FLOW_IF:
        case FLOW_ELSEIF: {
            if (!CheckParentheses(apScriptLine->pLine, 0)) {
                ScriptError(apCompileData, "Mismatched parentheses.");
                return false;
            }
            uint32_t uiStart = 0;
            SkipWhiteSpace(apScriptLine->pLine, uiStart);
            char pInfix[512]{};
            uint32_t uiSize =
                ReplaceVariablesAndFunctions(apCompileData, apScriptLine, &apScriptLine->pLine[uiStart], pInfix);
            if (!uiSize) {
                return false;
            }
            // The conversion leaves the postfix expression's size in uiSize.
            CExpression kExpression;
            const char* pPostfix = kExpression.InfixToPostfix(pInfix, uiSize);
            if (kExpression.m_ExpressionError) {
                ScriptError(apCompileData, expressionErrors[kExpression.m_ExpressionError]);
                return false;
            }
            const int32_t iLines = CountIfLines(apCompileData, apScriptLine);
            if (iLines == -1) {
                ScriptError(apCompileData, "Mismatched if/then/else block.");
                return false;
            }
            Emit(apScriptLine, static_cast<uint16_t>(iLines));
            Emit(apScriptLine, static_cast<uint16_t>(uiSize));
            AppendBytes(apScriptLine->pOutput, apScriptLine->uiOutputSize, pPostfix, uiSize);
            break;
        }
        case FLOW_ELSE: {
            const int32_t iLines = CountIfLines(apCompileData, apScriptLine);
            if (iLines == -1) {
                ScriptError(apCompileData, "Mismatched if/then/else block.");
                return false;
            }
            Emit(apScriptLine, static_cast<uint16_t>(iLines));
            break;
        }
        // These have no operands.
        case FLOW_ENDIF:
        case FLOW_REFERENCE_FUNCTION:
        case FLOW_SCRIPTNAME:
        case FLOW_RETURN: break;
        default: {
            const SCRIPT_FUNCTION* pFunction = GetFunctionDef(apScriptLine->sExpression);
            if (!pFunction) {
                ScriptError(apCompileData, "Unknown function code %d.", apScriptLine->sExpression);
                return false;
            }
            if (pFunction->bReferenceFunction && apCompileData->header.bIsQuestScript &&
                !apScriptLine->uiRefObjectIndex) {
                ScriptError(apCompileData, "Reference function requires explicit reference in quest script.");
                return false;
            }
            if (pFunction->bReferenceFunction && apScriptLine->uiRefObjectIndex) {
                const Script::RefVariable* pReference =
                    apCompileData->GetReferencedObject(apScriptLine->uiRefObjectIndex);
                if (!dynamic_cast<TESObjectREFR*>(pReference->form) && !pReference->varIdx) {
                    ScriptError(
                        apCompileData,
                        "Syntax error.  Invalid reference '%s' (only object references and reference variables are "
                        "allowed in this context).",
                        pReference->name.String()
                    );
                    return false;
                }
            }
            if (pFunction->pParameters &&
                !pFunction->pCompileFunction(
                    pFunction->sParamCount, pFunction->pParameters, apScriptLine, apCompileData
                )) {
                return false;
            }
            break;
        }
    }

    // The line in the compiled output: the reference it is called on, if any, then its expression code, the size of
    // its operands and the operands.
    if (apCompileData->uiOutputOffset + apScriptLine->uiOutputSize + 10 >= 0x4000) {
        ScriptError(apCompileData, "MAX_SCRIPT_SIZE exceeded.\r\nCompiled script not saved!");
        return false;
    }
    char* pOutput = apCompileData->pOutput;
    uint32_t& uiOutputOffset = apCompileData->uiOutputOffset;
    if (apScriptLine->uiRefObjectIndex) {
        AppendPacked(pOutput, uiOutputOffset, static_cast<uint16_t>(FLOW_REFERENCE_FUNCTION));
        AppendPacked(pOutput, uiOutputOffset, static_cast<uint16_t>(apScriptLine->uiRefObjectIndex));
    }
    AppendPacked(pOutput, uiOutputOffset, static_cast<uint16_t>(apScriptLine->sExpression));
    AppendPacked(pOutput, uiOutputOffset, static_cast<uint16_t>(apScriptLine->uiOutputSize));
    AppendBytes(pOutput, uiOutputOffset, apScriptLine->pOutput, apScriptLine->uiOutputSize);
    return true;
}

SCRIPT_OUTPUT ScriptCompiler::GetNextExpression(ScriptCompileData* apCompileData) {
    if (apCompileData->uiInputOffset >= std::strlen(apCompileData->input)) {
        return SCRIPT_DONE;
    }

    SCRIPT_LINE* pScriptLine = ::new (ourMalloc(sizeof(SCRIPT_LINE))) SCRIPT_LINE;
    pScriptLine->uiLineNumber = apCompileData->uiLastLineNumber + 1;
    // A line that is not queued is freed, and the input ends with it.
    const auto Discard = [pScriptLine] {
        ourFree(pScriptLine);
        return SCRIPT_DONE;
    };

    const int32_t iLineSize = GetNextLine(apCompileData, pScriptLine);
    if (!iLineSize) {
        apCompileData->eLastError = pScriptLine->eScriptError;
        return Discard();
    }
    apCompileData->uiInputOffset += iLineSize;

    // The first word names the expression the line starts with.
    SCRIPT_WORD kWord;
    if (!GetNextWord(apCompileData, &kWord, pScriptLine->pLine, pScriptLine->uiOffset, true, true)) {
        ScriptError(apCompileData, "Syntax Error\r\n%s\r\nCould not parse this line.", pScriptLine->pLine);
        return Discard();
    }
    pScriptLine->uiRefObjectIndex = kWord.uiRefObjectIndex;
    if (kWord.cVariableType != 'X') {
        ++apCompileData->uiLastLineNumber;
        ScriptError(apCompileData, "Script command \"%s\" not found.", kWord.pText);
        apCompileData->eLastError = ERROR_SCRIPT_UNHANDLEDCOMMAND;
        return Discard();
    }
    // Console functions only compile in the console.
    if (kWord.eFunctionCode >= START_OF_CONSOLE_SECTION && kWord.eFunctionCode < END_OF_CONSOLE_SECTION &&
        apCompileData->eCompilerIndex != SYSTEM_WINDOW_COMPILER) {
        ScriptError(apCompileData, "Script command \"%s\" is a console-only command.", kWord.pText);
        return Discard();
    }

    // The line keeps only what follows its first word.
    pScriptLine->sExpression = kWord.eFunctionCode;
    BSstrcpy(pScriptLine->pLine, sizeof(pScriptLine->pLine), &pScriptLine->pLine[pScriptLine->uiOffset]);
    pScriptLine->uiSize -= pScriptLine->uiOffset;
    apCompileData->uiLastLineNumber = pScriptLine->uiLineNumber;

    if (pScriptLine->sExpression == FLOW_SCRIPTNAME) {
        uint32_t uiOffset = 0;
        SCRIPT_WORD kName;
        if (!GetNextWord(apCompileData, &kName, pScriptLine->pLine, uiOffset, false, false)) {
            ScriptError(apCompileData, "Syntax Error.  Missing script name.");
            apCompileData->eLastError = ERROR_SCRIPT_SCRIPT_NAME_MISSING;
            return Discard();
        }
        apCompileData->cScriptName.Set(kName.pText, 0);
    }

    if ((pScriptLine->sExpression == FLOW_IF || pScriptLine->sExpression == FLOW_ELSEIF) && !pScriptLine->pLine[0]) {
        ScriptError(apCompileData, "Syntax Error.  No condition specified for 'if' or 'elseif'.");
        apCompileData->eLastError = ERROR_SCRIPT_EXPRESSION;
        return Discard();
    }

    apCompileData->QScriptLineList()->AddTail(pScriptLine);
    return pScriptLine->sExpression;
}

bool ScriptCompiler::VerifyBlocks(ScriptCompileData* apCompileData) {
    const auto Fail = [apCompileData](const SCRIPT_LINE* apLine, const char* apError) {
        apCompileData->uiLastLineNumber = apLine->uiLineNumber;
        ScriptError(apCompileData, apError);
        return false;
    };

    bool bInsideBlock = false;
    for (const SCRIPT_LINE* pLine : apCompileData->listLines.ItemsUntilNull()) {
        switch (pLine->sExpression) {
            case FLOW_BEGIN: {
                if (bInsideBlock) {
                    return Fail(pLine, "Nested begin/end blocks are not allowed.");
                }
                bInsideBlock = true;
                break;
            }
            case FLOW_END: {
                if (!bInsideBlock) {
                    return Fail(pLine, "End without matching Begin.");
                }
                bInsideBlock = false;
                break;
            }
            // The script's name and its variables are declared outside the blocks.
            case FLOW_SHORT:
            case FLOW_LONG:
            case FLOW_FLOAT:
            case FLOW_SCRIPTNAME:
            case FLOW_REF: break;
            default: {
                if (!bInsideBlock) {
                    return Fail(pLine, "All script commands must be inside a begin/end block.");
                }
                break;
            }
        }
    }
    return true;
}

int32_t ScriptCompiler::CountBlockLines(ScriptCompileData* apCompileData, SCRIPT_LINE* apStartingLine) {
    int32_t iCount = 0;
    for (const SCRIPT_LINE* pLine : apCompileData->QScriptLineList()->ItemsUntilNull()) {
        // Lines up to and including the one the block starts at are not part of the block.
        if (pLine->uiLineNumber <= apStartingLine->uiLineNumber) {
            continue;
        }
        switch (pLine->sExpression) {
            case SCRIPT_DONE: {
                ScriptError(
                    apCompileData, "Begin without matching end starting on line %d.", apStartingLine->uiLineNumber
                );
                return -1;
            }
            case FLOW_BEGIN: ScriptError(apCompileData, "Nested begin/end blocks are not allowed."); return -1;
            case FLOW_END: return iCount;
            // A variable declaration opens no block, so it neither counts nor ends one.
            case FLOW_SHORT:
            case FLOW_LONG:
            case FLOW_FLOAT: break;
            default: ++iCount; break;
        }
    }
    ScriptError(apCompileData, "Mismatched begin/end block starting on line %d.", apStartingLine->uiLineNumber);
    return -1;
}

uint32_t ScriptCompiler::ReplaceVariablesAndFunctions(
    ScriptCompileData* apCompileData,
    const SCRIPT_LINE* apScriptLine,
    char* apPostfix,
    char* apExpLine
) {
    const uint32_t uiLength = BSstrlen(apPostfix);
    uint32_t uiOut = 0;
    for (uint32_t i = 0; i < uiLength; ++i) {
        // A word, or a quoted name, is replaced by its compiled form; reading it moves i past it.
        if (std::isalpha(apPostfix[i]) || apPostfix[i] == '"') {
            SCRIPT_WORD kWord;
            if (!GetNextWord(apCompileData, &kWord, apPostfix, i, true, true)) {
                return 0;
            }
            // The reference the word is reached through, if any.
            if (kWord.uiRefObjectIndex) {
                AppendPacked(apExpLine, uiOut, kWord.cVariableType == 'G' ? 'G' : 'r');
                AppendPacked(apExpLine, uiOut, static_cast<uint16_t>(kWord.uiRefObjectIndex));
            }
            if (kWord.uiVariableID) {
                AppendPacked(apExpLine, uiOut, kWord.cVariableType);
                AppendPacked(apExpLine, uiOut, static_cast<uint16_t>(kWord.uiVariableID));
            }
            else if (kWord.cVariableType == 'X') {
                // A function call: its code, then the size of its compiled parameters and the parameters.
                AppendPacked(apExpLine, uiOut, 'X');
                AppendPacked(apExpLine, uiOut, static_cast<uint16_t>(kWord.eFunctionCode));
                const SCRIPT_FUNCTION* pFunction = GetFunctionDef(kWord.eFunctionCode);
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
                    // The parameters compile from the rest of the expression, as a line of their own.
                    SCRIPT_LINE kParameters;
                    BSstrcpy(kParameters.pLine, sizeof(kParameters.pLine), apPostfix + i);
                    kParameters.uiLineNumber = apScriptLine->uiLineNumber;
                    if (!pFunction->pCompileFunction(
                            static_cast<uint16_t>(pFunction->sParamCount), pFunction->pParameters, &kParameters,
                            apCompileData
                        )) {
                        return 0;
                    }
                    AppendPacked(apExpLine, uiOut, static_cast<uint16_t>(kParameters.uiOutputSize));
                    AppendBytes(apExpLine, uiOut, kParameters.pOutput, kParameters.uiOutputSize);
                    i += kParameters.uiOffset;
                }
                else {
                    AppendPacked(apExpLine, uiOut, uint16_t{0});
                }
            }
            else if (kWord.cVariableType != 'G') {
                // Any other word must name an object, which is compiled as the reference to it.
                if (kWord.uiRefObjectIndex || !apCompileData->GetReferencedObject(&kWord, false, false)) {
                    ScriptError(apCompileData, "Syntax Error.  Unknown command '%s'.", kWord.pText);
                    return 0;
                }
                AppendPacked(apExpLine, uiOut, 'Z');
                AppendPacked(apExpLine, uiOut, static_cast<uint16_t>(kWord.uiRefObjectIndex));
            }
        }
        if (i < uiLength) {
            AppendPacked(apExpLine, uiOut, apPostfix[i]);
        }
    }
    // The terminator goes one byte past the end; the callers' buffers are zeroed, so the byte at the end is 0 too.
    apExpLine[uiOut + 1] = '\0';
    return uiOut;
}

int32_t ScriptCompiler::CountIfLines(ScriptCompileData* apCompileData, SCRIPT_LINE* apStartingLine) {
    constexpr int32_t iMaxNesting = 10;
    int32_t iCount = 0;
    // The ifs opened inside the block: an else, elseif or endif belongs to the innermost one.
    int32_t iNested = 0;
    for (const SCRIPT_LINE* pLine : apCompileData->listLines.ItemsUntilNull()) {
        if (pLine->uiLineNumber <= apStartingLine->uiLineNumber) {
            continue;
        }
        switch (pLine->sExpression) {
            case SCRIPT_DONE: ScriptError(apCompileData, "Mismatched if/else/endif."); return -1;
            // A variable declaration is not counted.
            case FLOW_SHORT:
            case FLOW_LONG:
            case FLOW_FLOAT: break;
            case FLOW_IF: {
                ++iCount;
                if (++iNested >= iMaxNesting) {
                    ScriptError(apCompileData, "Max nesting of %d exceeded.", iMaxNesting);
                    return -1;
                }
                break;
            }
            // The block ends at its own else, elseif or endif, which is not counted.
            case FLOW_ELSE:
            case FLOW_ELSEIF: {
                if (!iNested) {
                    return iCount;
                }
                ++iCount;
                break;
            }
            case FLOW_ENDIF: {
                if (!iNested) {
                    return iCount;
                }
                --iNested;
                ++iCount;
                break;
            }
            default: ++iCount; break;
        }
    }
    ScriptError(apCompileData, "Mismatched if/else/endif block starting on line %d.", apStartingLine->uiLineNumber);
    return -1;
}

SCRIPT_FUNCTION* ScriptCompiler::GetFunctionDef(SCRIPT_OUTPUT aeExpression) {
    if (aeExpression >= START_OF_CONSOLE_SECTION && aeExpression < END_OF_CONSOLE_SECTION) {
        return &scriptConsole[aeExpression - START_OF_CONSOLE_SECTION];
    }
    if (aeExpression >= START_OF_FUNCTION_SECTION && aeExpression < END_OF_FUNCTION_SECTION) {
        return &scriptFunctions[aeExpression - START_OF_FUNCTION_SECTION];
    }
    return nullptr;
}

char ScriptCompiler::SkipWhiteSpace(const char* apExpLine, uint32_t& auiLineOffset) {
    // Commas separate parameters, so they are skipped with the blanks.
    while (apExpLine[auiLineOffset] == '\t' || apExpLine[auiLineOffset] == ' ' || apExpLine[auiLineOffset] == ',') {
        ++auiLineOffset;
    }
    return apExpLine[auiLineOffset];
}

// The message is passed to PrintLine/Warning as their format.
void ScriptCompiler::ScriptError(ScriptCompileData* apCompileData, const char* text, ...) {
    char cBuffer[260];
    va_list kArgs;
    va_start(kArgs, text);
    BSvsprintf(cBuffer, text, kArgs);
    va_end(kArgs);

    BSStringT<char> kMessage;
    kMessage.SPrintF(
        "SCRIPTS: Script '%s', line %d:\n%s", apCompileData->cScriptName.String(), apCompileData->uiLastLineNumber,
        cBuffer
    );
    char* pMessage = NavMeshInfo::GetCellOrCoords(const_cast<char*>(kMessage.String()), 0);
    if (apCompileData->eCompilerIndex == SYSTEM_WINDOW_COMPILER) {
        Interface::PrintLine(pMessage);
    }
    else {
        Warning(pMessage);
    }
}

int32_t ScriptCompiler::GetNextLine(ScriptCompileData* apCompileData, SCRIPT_LINE* pScriptLine) {
    constexpr uint32_t uiMaxLength = sizeof(pScriptLine->pLine);
    const char* pInput = &apCompileData->input[apCompileData->uiInputOffset];
    int32_t iOffset = 0;
    // Leading blanks are skipped until the first letter, digit or quote.
    bool bSkipping = true;
    while (true) {
        // The character is kept unsigned so isalnum gets it zero-extended.
        uint8_t cChar = pInput[iOffset++];
        if (cChar == ';') {
            // A comment runs to the end of the line.
            while (cChar && cChar != '\n') {
                cChar = pInput[iOffset++];
            }
        }
        else if (cChar && cChar != '\r') {
            if (pScriptLine->uiSize + 1 >= uiMaxLength) {
                ScriptError(
                    apCompileData, "Max script line length (%d characters) exceeded.", static_cast<int32_t>(uiMaxLength)
                );
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

        // The end of a line: the input's end, a carriage return or a comment's end. A newline right after the
        // carriage return or comment is part of the same line break; a bare newline is an ordinary character.
        if (cChar && pInput[iOffset] == '\n') {
            ++iOffset;
        }
        if (bSkipping) {
            if (!cChar) {
                return 0;
            }
        }
        else {
            // A line with code ends without trailing blanks.
            while (pScriptLine->uiSize &&
                   (pScriptLine->pLine[pScriptLine->uiSize - 1] == ' ' ||
                    pScriptLine->pLine[pScriptLine->uiSize - 1] == '\t')) {
                pScriptLine->pLine[--pScriptLine->uiSize] = '\0';
            }
            if (pScriptLine->uiSize) {
                pScriptLine->pLine[pScriptLine->uiSize] = '\0';
                return iOffset;
            }
        }
        // An empty line: the line starts over on the next one.
        ++pScriptLine->uiLineNumber;
        pScriptLine->uiSize = 0;
        ourMemset(pScriptLine->pLine, 0, uiMaxLength);
    }
}

bool ScriptCompiler::GetFunction(SCRIPT_WORD* apWord) {
    const auto Resolve = [apWord](const SCRIPT_FUNCTION& arFunction) {
        apWord->cVariableType = 'X';
        apWord->eFunctionCode = arFunction.eOutput;
        return true;
    };

    // The flow keywords are compared with the table name first, the function tables with the word first.
    for (const SCRIPT_FUNCTION& kFlow : flow) {
        if (!BSstricmp(kFlow.pFunctionName, apWord->pText) || !BSstricmp(kFlow.pShortName, apWord->pText)) {
            return Resolve(kFlow);
        }
    }
    for (int32_t i = 0; i < FUNCTION_COUNT; ++i) {
        const SCRIPT_FUNCTION& kFunction = scriptFunctions[i];
        if (!BSstricmp(apWord->pText, kFunction.pFunctionName) || !BSstricmp(apWord->pText, kFunction.pShortName)) {
            return Resolve(kFunction);
        }
    }
    for (int32_t i = 0; i < CONSOLE_COUNT; ++i) {
        const SCRIPT_FUNCTION& kFunction = scriptConsole[i];
        if (!BSstricmp(apWord->pText, kFunction.pFunctionName) || !BSstricmp(apWord->pText, kFunction.pShortName)) {
            return Resolve(kFunction);
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
    // name.word: the name is a reference the word after it is called on. Checks a reference may be named here.
    const auto CanNameReference = [&] {
        if (!abAllowVariable) {
            ScriptError(apCompileData, "Syntax Error.  Reference not allowed in this context.");
            return false;
        }
        if (apNextWord->uiRefObjectIndex) {
            ScriptError(apCompileData, "Syntax Error.  Invalid reference syntax.");
            return false;
        }
        return true;
    };
    // Resolves the name read as the reference, then reads the word called on it.
    const auto ReadWordOnReference = [&] -> int32_t {
        if (!apCompileData->GetReferencedObject(apNextWord, false, abAllowUnknownRef)) {
            ScriptError(apCompileData, "Unknown object '%s'.", apNextWord->pText);
            return 0;
        }
        return GetNextWord(apCompileData, apNextWord, apExpLine, auiLineOffset, true, abAllowUnknownRef);
    };

    SkipWhiteSpace(apExpLine, auiLineOffset);
    int32_t iLength = 0;

    // A quoted word is the text between the quotes; the quotes are blanked out while it is copied.
    if (apExpLine[auiLineOffset] == '"') {
        if (!CheckQuotes(apExpLine, auiLineOffset)) {
            ScriptError(apCompileData, "Syntax Error.  Mismatched quotes.");
            return 0;
        }
        char* pOpen = const_cast<char*>(&apExpLine[auiLineOffset]);
        *pOpen = ' ';
        char* pClose = std::strchr(pOpen, '"');
        *pClose = '\0';
        BSstrcpy(apNextWord->pText, sizeof(apNextWord->pText), pOpen + 1);
        iLength = BSstrlen(apNextWord->pText);
        *pOpen = '"';
        *pClose = '"';
        auiLineOffset += iLength + 2;

        if (apExpLine[auiLineOffset] == '.') {
            if (!CanNameReference()) {
                return 0;
            }
            ++auiLineOffset;
            return ReadWordOnReference();
        }
        apNextWord->pText[iLength] = '\0';
    }

    // An unquoted word, or an empty quoted one, is read from the line.
    if (!iLength) {
        while (std::isalnum(apExpLine[auiLineOffset]) || apExpLine[auiLineOffset] == '.' ||
               apExpLine[auiLineOffset] == '-' || apExpLine[auiLineOffset] == '_') {
            apNextWord->pText[iLength++] = apExpLine[auiLineOffset++];
            // Only a name starting with a letter can be a reference.
            if (apNextWord->pText[iLength - 1] == '.' && std::isalpha(apNextWord->pText[0])) {
                if (!CanNameReference()) {
                    return 0;
                }
                apNextWord->pText[iLength - 1] = '\0';
                return ReadWordOnReference();
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
            const Script::RefVariable* pReference = apCompileData->GetReferencedObject(apNextWord->uiRefObjectIndex);
            const bool bIsReference = dynamic_cast<TESObjectREFR*>(pReference->form) != nullptr;
            const bool bIsQuest = dynamic_cast<TESQuest*>(pReference->form) != nullptr;
            if (!bIsReference && !bIsQuest && !abAllowUnknownRef) {
                ScriptError(
                    apCompileData,
                    "Syntax error.  Invalid reference '%s' (only quests and object references are allowed in this "
                    "context).",
                    pReference->form->GetFormEditorID()
                );
                return 0;
            }
            if (bIsQuest && apNextWord->cVariableType == 'X') {
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
    int32_t iQuotes = 0;
    for (; apExpLine[auiLineOffset]; ++auiLineOffset) {
        if (apExpLine[auiLineOffset] == '"') {
            ++iQuotes;
        }
    }
    return iQuotes >= 2 && iQuotes % 2 == 0;
}

bool ScriptCompiler::IsValidVariableName(const char* apVarName) {
    // A variable may not be named like a form or a function.
    if (!apVarName || !*apVarName || TESForm::GetFormByEditorID(apVarName)) {
        return false;
    }
    SCRIPT_WORD kWord;
    BSstrcpy(kWord.pText, sizeof(kWord.pText), apVarName);
    return !GetFunction(&kWord);
}

bool ScriptCompiler::CheckParentheses(const char* apExpLine, uint32_t auiLineOffset) {
    // Only the counts are compared, so a ')' before its '(' passes.
    int32_t iDepth = 0;
    for (; apExpLine[auiLineOffset]; ++auiLineOffset) {
        if (apExpLine[auiLineOffset] == '(') {
            ++iDepth;
        }
        else if (apExpLine[auiLineOffset] == ')') {
            --iDepth;
        }
    }
    return iDepth == 0;
}

namespace {

    bool HasFormType(const TESForm* apForm, ENUM_FORM_ID aeType) {
        return apForm && apForm->GetFormType() == aeType;
    }

    template <typename T>
    bool IsA(TESForm* apForm) {
        return dynamic_cast<T*>(apForm) != nullptr;
    }

    // The index of the first of apNames that equals apName, ignoring case; -1 for none.
    template <typename Name, size_t N>
    int16_t FindName(Name (&apNames)[N], const char* apName) {
        for (int16_t i = 0; i < static_cast<int16_t>(N); ++i) {
            if (!BSstricmp(apNames[i], apName)) {
                return i;
            }
        }
        return -1;
    }

    // Checks that the object a referenced-object parameter names is one the parameter takes, and reports it when it
    // is not. What a variable holds is only known when the script runs, so a variable is taken.
    bool CheckReferencedObject(
        ScriptCompileData* apCompileData,
        const SCRIPT_PARAMETER& arParam,
        const SCRIPT_WORD& arWord
    ) {
        TESForm* pForm = arWord.pForm;
        const bool bVariable = arWord.uiVariableID != 0;
        const auto Reject = [&](const char* apFormat) {
            ScriptCompiler::ScriptError(apCompileData, apFormat, arWord.pText, arParam.pParamName);
            return false;
        };

        switch (arParam.eParamType) {
            case SCRIPT_PARAM_OBJECTREF:
            case SCRIPT_PARAM_ACTOR:
            case SCRIPT_PARAM_MAP_MARKER:
            case SCRIPT_PARAM_CONTAINER_REF: {
                if (bVariable) {
                    return true;
                }
                TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(pForm);
                if (!pRef) {
                    return Reject("Invalid object reference '%s' for parameter %s.\r\nCompiled script not saved!");
                }
                if (arParam.eParamType == SCRIPT_PARAM_ACTOR) {
                    return pRef->IsActor() ||
                        Reject("Invalid actor '%s' for parameter %s.\r\nCompiled script not saved!");
                }
                if (arParam.eParamType == SCRIPT_PARAM_MAP_MARKER) {
                    return static_cast<TESObjectSTAT*>(pRef->GetObjectReference()) == pMapMarker ||
                        Reject("Invalid map marker '%s' for parameter %s.\r\nCompiled script not saved!");
                }
                if (arParam.eParamType == SCRIPT_PARAM_CONTAINER_REF) {
                    return pRef->HasContainer() ||
                        Reject("Invalid container reference '%s' for parameter %s.\r\nCompiled script not saved!");
                }
                return true;
            }
            case SCRIPT_PARAM_INVENTORY_OBJECT:
                return bVariable || (pForm && TESContainer::ContainerCanHoldType(pForm->GetFormType())) ||
                    Reject("Invalid inventory object '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_SPELL_ITEM:
                return bVariable || IsA<SpellItem>(pForm) || IsA<TESObjectBOOK>(pForm) ||
                    Reject("Invalid spell item '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_CELL: {
                const TESObjectCELL* pCell = dynamic_cast<TESObjectCELL*>(pForm);
                return bVariable || (pCell && pCell->GetInterior()) ||
                    Reject("Invalid interior cell '%s' for parameter %s.\r\nCompiled script not saved!");
            }
            case SCRIPT_PARAM_MAGIC_ITEM:
                return bVariable || IsA<MagicItem>(pForm) ||
                    Reject("Invalid magic item '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_SOUND:
                return bVariable || HasFormType(pForm, SOUN_ID) ||
                    Reject("Invalid sound '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_TOPIC:
                return bVariable || HasFormType(pForm, DIAL_ID) ||
                    Reject("Invalid topic '%s' for parameter %s.\r\nCompiled script not saved!");
            // The message calls a quest an info.
            case SCRIPT_PARAM_QUEST:
                return bVariable || HasFormType(pForm, QUST_ID) ||
                    Reject("Invalid info '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_RACE:
                return bVariable || HasFormType(pForm, RACE_ID) ||
                    Reject("Invalid race '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_CLASS:
                return bVariable || HasFormType(pForm, CLAS_ID) ||
                    Reject("Invalid class '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_FACTION:
                return bVariable || HasFormType(pForm, FACT_ID) ||
                    Reject("Invalid faction '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_GLOBAL:
                return bVariable || HasFormType(pForm, GLOB_ID) ||
                    Reject("Invalid global '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_FURNITURE_OR_FORMLIST:
                return bVariable || HasFormType(pForm, FURN_ID) || HasFormType(pForm, FLST_ID) ||
                    Reject("Invalid furniture object/list '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_OBJECT:
                return bVariable || IsA<TESObject>(pForm) ||
                    Reject("Invalid object '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_ACTOR_BASE:
                return bVariable || IsA<TESActorBase>(pForm) ||
                    Reject("Invalid actor base '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_WORLD:
                return bVariable || IsA<TESWorldSpace>(pForm) ||
                    Reject("Invalid worldspace '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_PACKAGE:
                return bVariable || IsA<TESPackage>(pForm) ||
                    Reject("Invalid package '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_COMBAT_STYLE:
                return bVariable || IsA<TESCombatStyle>(pForm) ||
                    Reject("Invalid combat style '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_MAGIC_EFFECT:
                return bVariable || IsA<EffectSetting>(pForm) ||
                    Reject("Invalid effect setting '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_WEATHER:
                return bVariable || HasFormType(pForm, WTHR_ID) ||
                    Reject("Invalid weather '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_NPC:
                return bVariable || HasFormType(pForm, NPC__ID) ||
                    Reject("Invalid NPC '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_OWNER:
                return bVariable || HasFormType(pForm, NPC__ID) || HasFormType(pForm, FACT_ID) ||
                    Reject("Invalid owner '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_SHADER_EFFECT:
                return bVariable || HasFormType(pForm, EFSH_ID) ||
                    Reject("Invalid Effect Shader '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_FORMLIST:
                return bVariable || IsA<BGSListForm>(pForm) ||
                    Reject("Invalid form list '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_MENUICON:
                return bVariable || IsA<BGSMenuIcon>(pForm) ||
                    Reject("Invalid menu icon '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_PERK:
                return bVariable || IsA<BGSPerk>(pForm) ||
                    Reject("Invalid perk '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_NOTE:
                return bVariable || IsA<BGSNote>(pForm) ||
                    Reject("Invalid note '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_IMAGESPACEMOD:
                return bVariable || HasFormType(pForm, IMAD_ID) ||
                    Reject("Invalid Imagespace Modifier '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_IMAGESPACE:
                return bVariable || HasFormType(pForm, IMGS_ID) ||
                    Reject("Invalid Imagespace '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_ENCOUNTERZONE:
                return bVariable || IsA<BGSEncounterZone>(pForm) ||
                    Reject("Invalid encounter zone '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_IDLE_FORM:
                return bVariable || IsA<TESIdleForm>(pForm) ||
                    Reject("Invalid Idle form'%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_MESSAGE:
                return bVariable || HasFormType(pForm, MESG_ID) ||
                    Reject("Invalid message '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_INVOBJECT_OR_FORMLIST:
                return bVariable ||
                    (pForm &&
                     (TESContainer::ContainerCanHoldType(pForm->GetFormType()) || HasFormType(pForm, FLST_ID))) ||
                    Reject("Invalid inventory object '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_OBJECT_OR_FORMLIST:
                return bVariable || (pForm && (pForm->IsObject() || HasFormType(pForm, FLST_ID))) ||
                    Reject("Invalid object '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_MUSIC:
                return bVariable || HasFormType(pForm, MUSC_ID) ||
                    Reject("Invalid music type '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_NPC_OR_LEVCHAR:
                return bVariable || HasFormType(pForm, NPC__ID) || HasFormType(pForm, LVLN_ID) ||
                    Reject("Invalid NPC or LeveledCharacter '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_CREA_OR_LEVCREA:
                return bVariable || HasFormType(pForm, CREA_ID) || HasFormType(pForm, LVLC_ID) ||
                    Reject("Invalid Creature or LeveledCreature '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_LEVCHAR:
                return bVariable || HasFormType(pForm, LVLN_ID) ||
                    Reject("Invalid LeveledCharacter '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_LEVCREA:
                return bVariable || HasFormType(pForm, LVLC_ID) ||
                    Reject("Invalid LeveledCreature '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_LEVITEM:
                return bVariable || HasFormType(pForm, LVLI_ID) ||
                    Reject("Invalid LeveledItem '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_FORM:
                return bVariable || pForm ||
                    Reject("Invalid form '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_REPUTATION:
                return bVariable || HasFormType(pForm, REPU_ID) ||
                    Reject("Invalid reputation '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_CASINO:
                return bVariable || HasFormType(pForm, CSNO_ID) ||
                    Reject("Invalid casino '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_CASINOCHIP:
                return bVariable || HasFormType(pForm, CHIP_ID) ||
                    Reject("Invalid casino chip '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_CHALLENGE:
                return bVariable || HasFormType(pForm, CHAL_ID) ||
                    Reject("Invalid challenge '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_CARAVANMONEY:
                return bVariable || HasFormType(pForm, CMNY_ID) ||
                    Reject("Invalid caravan money '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_CARAVANCARD:
                return bVariable || HasFormType(pForm, CCRD_ID) ||
                    Reject("Invalid caravan card '%s' for parameter %s.\r\nCompiled script not saved!");
            // The message calls a deck a card.
            case SCRIPT_PARAM_CARAVANDECK:
                return bVariable || HasFormType(pForm, CDCK_ID) ||
                    Reject("Invalid caravan card '%s' for parameter %s.\r\nCompiled script not saved!");
            case SCRIPT_PARAM_REGION:
                return bVariable || HasFormType(pForm, REGN_ID) ||
                    Reject("Invalid region '%s' for parameter %s.\r\nCompiled script not saved!");
            default: {
                Warning(
                    "SCRIPTS: Param type '%d' (referenced object) unimplemented in ScriptCompiler::StandardCompile.",
                    arParam.eParamType
                );
                return false;
            }
        }
    }

    // Compiles a parameter that is not a referenced object into the line's operands, and reports it when the word is
    // not a value the parameter takes. aiSize is the word's length.
    bool CompileValue(
        ScriptCompileData* apCompileData,
        SCRIPT_LINE* apScriptLine,
        const SCRIPT_PARAMETER& arParam,
        SCRIPT_WORD& arWord,
        int32_t aiSize
    ) {
        const auto Reject = [&](const char* apFormat) {
            ScriptCompiler::ScriptError(apCompileData, apFormat, arWord.pText, arParam.pParamName);
            return false;
        };

        switch (arParam.eParamType) {
            case SCRIPT_PARAM_CHAR: {
                Emit(apScriptLine, static_cast<uint16_t>(aiSize));
                EmitBytes(apScriptLine, arWord.pText, aiSize);
                return true;
            }
            case SCRIPT_PARAM_INT:
            case SCRIPT_PARAM_FLOAT:
            case SCRIPT_PARAM_STAGE: {
                // A variable: the reference it is reached through, if any, then the variable itself; a global is
                // compiled as its reference alone.
                if (arWord.uiRefObjectIndex) {
                    Emit(apScriptLine, arWord.cVariableType == 'G' ? 'G' : 'r');
                    Emit(apScriptLine, static_cast<uint16_t>(arWord.uiRefObjectIndex));
                }
                if (arWord.cVariableType == 'G') {
                    return true;
                }
                if (arWord.uiVariableID) {
                    Emit(apScriptLine, arWord.cVariableType);
                    Emit(apScriptLine, static_cast<uint16_t>(arWord.uiVariableID));
                    return true;
                }
                // Otherwise a number: 'n' and a 32-bit integer, or 'z' and a double.
                const char* pUnknown = "Unknown variable '%s' for parameter %s.\r\nCompiled script not saved!";
                if (arParam.eParamType == SCRIPT_PARAM_INT || arParam.eParamType == SCRIPT_PARAM_STAGE) {
                    if (!IsStringInteger(arWord.pText)) {
                        return Reject(pUnknown);
                    }
                    Emit(apScriptLine, 'n');
                    Emit(apScriptLine, static_cast<uint32_t>(std::atol(arWord.pText)));
                    return true;
                }
                if (!IsStringFloat(arWord.pText)) {
                    return Reject(pUnknown);
                }
                Emit(apScriptLine, 'z');
                Emit(apScriptLine, std::atof(arWord.pText));
                return true;
            }
            case SCRIPT_PARAM_ACTOR_VALUE: {
                const ActorValue::Index eActorValue = ActorValue::GetActorValueIndex(arWord.pText);
                if (eActorValue == ActorValue::ACTOR_VALUE_COUNT) {
                    return Reject("Invalid actor value '%s' for parameter %s.\r\nCompiled script not saved!");
                }
                Emit(apScriptLine, static_cast<uint16_t>(eActorValue));
                return true;
            }
            case SCRIPT_PARAM_AXIS: {
                const char cAxis = static_cast<char>(std::toupper(arWord.pText[0]));
                if (cAxis != 'X' && cAxis != 'Y' && cAxis != 'Z') {
                    ScriptCompiler::ScriptError(
                        apCompileData, "Axis (X,Y,Z) required for parameter %s.\r\nCompiled script not saved!",
                        arParam.pParamName
                    );
                    return false;
                }
                Emit(apScriptLine, cAxis);
                return true;
            }
            case SCRIPT_PARAM_ANIM_GROUP: {
                ANIM_GROUP_ENUM eGroup = ANIM_GROUP_NONE;
                for (int32_t i = ANIM_GROUP_IDLE; i < ANIM_GROUP_COUNT; ++i) {
                    if (!BSstricmp(arWord.pText, animGroupInfo[i].pName)) {
                        eGroup = static_cast<ANIM_GROUP_ENUM>(i);
                        break;
                    }
                }
                if (eGroup == ANIM_GROUP_NONE) {
                    return Reject("Animation group \"%s\" not found for parameter %s.\r\nCompiled script not saved!");
                }
                Emit(apScriptLine, static_cast<uint16_t>(eGroup));
                return true;
            }
            case SCRIPT_PARAM_SEX: {
                uint16_t usSex;
                if (!BSstricmp(arWord.pText, sexName[0])) {
                    usSex = 0;
                }
                else if (!BSstricmp(arWord.pText, sexName[1])) {
                    usSex = 1;
                }
                else {
                    ScriptCompiler::ScriptError(
                        apCompileData, "Sex (Male, Female) required for parameter %s.\r\nCompiled script not saved!",
                        arParam.pParamName
                    );
                    return false;
                }
                Emit(apScriptLine, usSex);
                return true;
            }
            case SCRIPT_PARAM_CRIME_TYPE: {
                constexpr int32_t iMaxCrimeType = 4;
                const bool bNumber = IsStringInteger(arWord.pText);
                const uint32_t uiCrimeType = bNumber ? static_cast<uint32_t>(std::atol(arWord.pText)) : 0;
                if (!bNumber || uiCrimeType > static_cast<uint32_t>(iMaxCrimeType)) {
                    ScriptCompiler::ScriptError(
                        apCompileData,
                        "Invalid crime type '%s' for parameter %s.  Crime type must be a numeric value from "
                        "0-%d.\r\nCompiled script not saved!",
                        arWord.pText, arParam.pParamName, iMaxCrimeType
                    );
                    return false;
                }
                Emit(apScriptLine, static_cast<uint16_t>(uiCrimeType));
                return true;
            }
            case SCRIPT_PARAM_FORM_TYPE: {
                uint8_t ucFormType = 0;
                for (size_t i = 0; i < std::size(objectLabel); ++i) {
                    if (!BSstricmp(arWord.pText, objectLabel[i])) {
                        ucFormType = aObjectTypeFormTypes[i];
                        break;
                    }
                }
                if (!ucFormType) {
                    return Reject("Form Type \"%s\" not found for parameter %s.\r\nCompiled script not saved!");
                }
                Emit(apScriptLine, static_cast<uint16_t>(ucFormType));
                return true;
            }
            case SCRIPT_PARAM_MISC_STAT: {
                // The lookup returns the number of statistics for a name it does not know.
                const int32_t iStat = GetMiscStatIDFromName(arWord.pText);
                if (iStat == MiscStatManager::MS_EQUIPMENT_DAMAGE_REPAIRED) {
                    return Reject("Invalid misc stat '%s' for parameter %s.\r\nCompiled script not saved!");
                }
                Emit(apScriptLine, static_cast<uint16_t>(iStat));
                return true;
            }
            case SCRIPT_PARAM_ALIGNMENT: {
                const int16_t sAlignment = FindName(TESActorBaseData::pAlignmentNames, arWord.pText);
                if (sAlignment < 0) {
                    return Reject("Invalid alignment '%s' for parameter %s.\r\nCompiled script not saved!");
                }
                Emit(apScriptLine, static_cast<uint16_t>(sAlignment));
                return true;
            }
            case SCRIPT_PARAM_EQUIPTYPE: {
                const int16_t sEquipType = FindName(BGSEquipType::pEquipTypeNames, arWord.pText);
                if (sEquipType < 0) {
                    return Reject("Invalid EquipType '%s' for parameter %s.\r\nCompiled script not saved!");
                }
                Emit(apScriptLine, static_cast<uint16_t>(sEquipType));
                return true;
            }
            case SCRIPT_PARAM_CRITSTAGE: {
                const int16_t sStage = FindName(pCriticalStageNames, arWord.pText);
                if (sStage < 0) {
                    return Reject("Invalid CriticalStage '%s' for parameter %s.\r\nCompiled script not saved!");
                }
                Emit(apScriptLine, static_cast<uint16_t>(sStage));
                return true;
            }
            default: {
                Warning(
                    "SCRIPTS: Param type '%d' unimplemented in ScriptCompiler::StandardCompile.", arParam.eParamType
                );
                return false;
            }
        }
    }

} // namespace

bool ScriptCompiler::StandardCompile(
    const uint16_t asParamCount,
    const SCRIPT_PARAMETER* apParameters,
    SCRIPT_LINE* apScriptLine,
    ScriptCompileData* apCompileData
) {
    apScriptLine->uiOffset = 0;
    // The number of parameters leads them; when an optional one is left out it becomes the number given.
    const uint32_t uiCountField = apScriptLine->uiOutputSize;
    Emit(apScriptLine, asParamCount);
    for (int16_t sParam = 0; sParam < asParamCount; ++sParam) {
        const SCRIPT_PARAMETER& kParam = apParameters[sParam];
        const SCRIPT_PARAMETER_DEF& kParamDef = scriptParameterDefs[kParam.eParamType];
        const bool bCanBeVariable = kParamDef.bCanBeVariable;
        SCRIPT_WORD kWord;
        const int32_t iSize =
            GetNextWord(apCompileData, &kWord, apScriptLine->pLine, apScriptLine->uiOffset, bCanBeVariable, false);
        if (!iSize) {
            if (kParam.bOptional) {
                WritePacked(&apScriptLine->pOutput[uiCountField], static_cast<uint16_t>(sParam));
                return true;
            }
            ScriptError(apCompileData, "Missing parameter %s.\r\nCompiled script not saved!", kParam.pParamName);
            return false;
        }
        if (!bCanBeVariable && (kWord.uiRefObjectIndex || kWord.cVariableType)) {
            ScriptError(
                apCompileData, "Parameter %s may not be a variable.\r\nCompiled script not saved!", kParam.pParamName
            );
            return false;
        }

        if (kParamDef.bReferencedObject) {
            if (!apCompileData->GetReferencedObject(&kWord, false, false) || !kWord.uiRefObjectIndex) {
                ScriptError(
                    apCompileData, "Item '%s' not found for parameter %s.\r\nCompiled script not saved!", kWord.pText,
                    kParam.pParamName
                );
                return false;
            }
            if (!CheckReferencedObject(apCompileData, kParam, kWord)) {
                return false;
            }
            // The object is compiled as the reference to it.
            Emit(apScriptLine, 'r');
            Emit(apScriptLine, static_cast<uint16_t>(kWord.uiRefObjectIndex));
        }
        else if (!CompileValue(apCompileData, apScriptLine, kParam, kWord, iSize)) {
            return false;
        }
    }

    if (apScriptLine->uiOffset < apScriptLine->uiSize) {
        ScriptError(apCompileData, "Expected end of line.\r\nCompiled script not saved!");
        return false;
    }
    return true;
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
            if ((apCurrentWord->pForm) && (apCurrentWord->pForm->GetFormType() == QUST_ID)) {
                pScript = TESScriptableForm::GetFormScript(apCurrentWord->pForm);
            }
        }
        if (pScript) {
            apCurrentWord->cVariableType = pScript->FindVariable(apCurrentWord->pText, &apCurrentWord->uiVariableID);
            if (apCurrentWord->cVariableType) {
                return true;
            }

            apCurrentWord->uiVariableID = 0;
            return false;
        }

        apCurrentWord->uiVariableID = 0;
        return false;
    }

    if (apCurrentWord->uiRefObjectIndex) {
        apCurrentWord->uiVariableID = 0;
        apCurrentWord->cVariableType = 0;
        return false;
    }

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

    if (pDataHandler->GetGlobal(apCurrentWord->pText) && GetReferencedObject(apCurrentWord, false, false)) {
        apCurrentWord->uiVariableID = 0;
        apCurrentWord->cVariableType = 71;
        return true;
    }

    apCurrentWord->uiVariableID = 0;
    apCurrentWord->cVariableType = 0;
    return false;
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
