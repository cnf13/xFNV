// ScriptCompiler: from the helpers that read words and lines up to whole scripts compiled to bytecode. The compiler
// is TESScriptCompiler.cpp as the game builds it; the forms, scripts and function tables are scriptcompiler/World's.
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ScriptCompileData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptVariable.h"
#include "scriptcompiler/World.h"
#include "support/Messages.h"

#include "doctest.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

    using namespace test;

    // The compiler's protected members, for the tests to call.
    struct Compiler : ScriptCompiler {
        using ScriptCompiler::CheckQuotes;
        using ScriptCompiler::Compile;
        using ScriptCompiler::CountBlockLines;
        using ScriptCompiler::GetFunction;
        using ScriptCompiler::GetNextExpression;
        using ScriptCompiler::GetNextLine;
        using ScriptCompiler::ReplaceVariablesAndFunctions;
        using ScriptCompiler::VerifyBlocks;
    };

    // Each test starts from world::Reset's world.
    struct Fixture {
        Fixture() {
            world::Reset();
        }

        const std::vector<std::string>& Warnings() const {
            return Reported().warnings;
        }

        Compiler compiler;
    };

    // The bytes as hex, so that a failed comparison shows where they differ.
    std::string Hex(const void* apBytes, size_t auiSize) {
        std::string kText;
        char acByte[4];
        for (size_t i = 0; i < auiSize; ++i) {
            std::snprintf(acByte, sizeof(acByte), i ? " %02x" : "%02x", static_cast<const uint8_t*>(apBytes)[i]);
            kText += acByte;
        }
        return kText;
    }

    std::string Hex(const std::vector<uint8_t>& arBytes) {
        return Hex(arBytes.data(), arBytes.size());
    }

    // Compiled bytes, written as the compiler lays them out: little-endian and packed.
    class Bytes {
    public:
        Bytes& U8(uint8_t aValue) {
            return Raw(&aValue, sizeof(aValue));
        }
        Bytes& Char(char aValue) {
            return Raw(&aValue, sizeof(aValue));
        }
        Bytes& U16(uint16_t aValue) {
            return Raw(&aValue, sizeof(aValue));
        }
        Bytes& U32(uint32_t aValue) {
            return Raw(&aValue, sizeof(aValue));
        }
        Bytes& F64(double aValue) {
            return Raw(&aValue, sizeof(aValue));
        }
        Bytes& Text(const char* apText) {
            return Raw(apText, std::strlen(apText));
        }
        Bytes& Append(const Bytes& arOther) {
            return Raw(arOther.m_bytes.data(), arOther.m_bytes.size());
        }

        size_t Size() const {
            return m_bytes.size();
        }
        std::string Hex() const {
            return ::Hex(m_bytes);
        }

    private:
        Bytes& Raw(const void* apData, size_t auiSize) {
            const auto* pData = static_cast<const uint8_t*>(apData);
            m_bytes.insert(m_bytes.end(), pData, pData + auiSize);
            return *this;
        }

        std::vector<uint8_t> m_bytes;
    };

    // A compiled line: the expression code, the size of the operands, the operands.
    Bytes Line(SCRIPT_OUTPUT aeExpression, const Bytes& arOperands = {}) {
        return Bytes().U16(static_cast<uint16_t>(aeExpression)).U16(static_cast<uint16_t>(arOperands.Size()))
            .Append(arOperands);
    }

    SCRIPT_PARAMETER Param(const char* apName, SCRIPT_PARAM_TYPE aeType, bool abOptional = false) {
        return {apName, aeType, abOptional, {}};
    }

    constexpr SCRIPT_OUTPUT Function(int32_t aiIndex) {
        return static_cast<SCRIPT_OUTPUT>(START_OF_FUNCTION_SECTION + aiIndex);
    }

    constexpr SCRIPT_OUTPUT Console(int32_t aiIndex) {
        return static_cast<SCRIPT_OUTPUT>(START_OF_CONSOLE_SECTION + aiIndex);
    }

    // A line queued as GetNextExpression queues it, for the block and if counts.
    SCRIPT_LINE* QueueLine(ScriptCompileData& arData, uint32_t auiNumber, SCRIPT_OUTPUT aeExpression) {
        auto* pLine = ::new (ourMalloc(sizeof(SCRIPT_LINE))) SCRIPT_LINE;
        pLine->uiLineNumber = auiNumber;
        pLine->sExpression = aeExpression;
        arData.listLines.AddTail(pLine);
        return pLine;
    }

} // namespace

// Lists of strings (the messages, the referenced objects) are shown in full when a check fails.
template <>
struct doctest::StringMaker<std::vector<std::string>> {
    static String convert(const std::vector<std::string>& arStrings) {
        std::string kText = "{";
        for (const std::string& kString : arStrings) {
            kText += (kText.size() > 1 ? ", \"" : "\"") + kString + "\"";
        }
        return (kText + "}").c_str();
    }
};

// ---- the helpers -----------------------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(Fixture, "SkipWhiteSpace skips blanks, tabs and commas, and returns the character after them") {
    uint32_t uiOffset = 1;
    CHECK(ScriptCompiler::SkipWhiteSpace("a \t, ,b c", uiOffset) == 'b');
    CHECK(uiOffset == 6);

    // At a character that is not blank it stays.
    CHECK(ScriptCompiler::SkipWhiteSpace("a \t, ,b c", uiOffset) == 'b');
    CHECK(uiOffset == 6);

    uiOffset = 0;
    CHECK(ScriptCompiler::SkipWhiteSpace("  ", uiOffset) == '\0');
    CHECK(uiOffset == 2);
}

TEST_CASE_FIXTURE(Fixture, "CheckQuotes wants an even, nonzero number of quotes after the offset") {
    CHECK(Compiler::CheckQuotes("say \"hello\"", 0));
    CHECK(Compiler::CheckQuotes("\"a\" \"b\"", 0));
    CHECK_FALSE(Compiler::CheckQuotes("no quotes", 0));
    CHECK_FALSE(Compiler::CheckQuotes("\"one", 0));
    CHECK_FALSE(Compiler::CheckQuotes("\"a\" \"b", 0));
    // Only what follows the offset counts.
    CHECK_FALSE(Compiler::CheckQuotes("\"a\" \"b\"", 1));
}

TEST_CASE_FIXTURE(Fixture, "CheckParentheses only compares the counts") {
    CHECK(ScriptCompiler::CheckParentheses("(a + (b * c))", 0));
    CHECK(ScriptCompiler::CheckParentheses("no parentheses", 0));
    CHECK_FALSE(ScriptCompiler::CheckParentheses("(a + b", 0));
    CHECK_FALSE(ScriptCompiler::CheckParentheses("a + b)", 0));
    // A ')' before its '(' passes.
    CHECK(ScriptCompiler::CheckParentheses(")a(", 0));
    CHECK(ScriptCompiler::CheckParentheses("(a + b", 1));
}

TEST_CASE_FIXTURE(Fixture, "GetFunctionDef finds console functions from 0x100 and script functions from 0x1000") {
    CHECK(ScriptCompiler::GetFunctionDef(Console(0)) == &scriptConsole[0]);
    CHECK(ScriptCompiler::GetFunctionDef(Console(CONSOLE_COUNT - 1)) == &scriptConsole[CONSOLE_COUNT - 1]);
    CHECK(ScriptCompiler::GetFunctionDef(Function(0)) == &scriptFunctions[0]);
    CHECK(ScriptCompiler::GetFunctionDef(Function(FUNCTION_COUNT - 1)) == &scriptFunctions[FUNCTION_COUNT - 1]);

    // The tables' end markers are not functions.
    CHECK(ScriptCompiler::GetFunctionDef(Console(CONSOLE_COUNT)) == nullptr);
    CHECK(ScriptCompiler::GetFunctionDef(Function(FUNCTION_COUNT)) == nullptr);
    CHECK(ScriptCompiler::GetFunctionDef(FLOW_SET) == nullptr);
    CHECK(ScriptCompiler::GetFunctionDef(SCRIPT_DONE) == nullptr);
}

TEST_CASE_FIXTURE(Fixture, "GetFunction resolves flow keywords, script functions and console functions") {
    world::DefineFunction(Function(5), "GetSecondsPassed", "");
    world::DefineFunction(Console(3), "ToggleGodMode", "tgm");

    const auto Resolve = [](const char* apText) {
        SCRIPT_WORD kWord;
        std::strcpy(kWord.pText, apText);
        const bool bFound = Compiler::GetFunction(&kWord);
        return std::pair{bFound, bFound ? kWord.eFunctionCode : static_cast<SCRIPT_OUTPUT>(0)};
    };

    // By name or short name, ignoring case.
    CHECK(Resolve("set") == std::pair{true, FLOW_SET});
    CHECK(Resolve("SCN") == std::pair{true, FLOW_SCRIPTNAME});
    CHECK(Resolve("int") == std::pair{true, FLOW_SHORT});
    CHECK(Resolve("getsecondspassed") == std::pair{true, Function(5)});
    CHECK(Resolve("TGM") == std::pair{true, Console(3)});
    CHECK(Resolve("NoSuchFunction") == std::pair{false, static_cast<SCRIPT_OUTPUT>(0)});

    SCRIPT_WORD kWord;
    std::strcpy(kWord.pText, "If");
    REQUIRE(Compiler::GetFunction(&kWord));
    CHECK(kWord.cVariableType == 'X');
}

TEST_CASE_FIXTURE(Fixture, "IsValidVariableName rejects empty names, editor IDs and function names") {
    world::AddReference("DoorRef");
    world::DefineFunction(Function(0), "GetPos", "");

    CHECK(ScriptCompiler::IsValidVariableName("counter"));
    CHECK_FALSE(ScriptCompiler::IsValidVariableName(nullptr));
    CHECK_FALSE(ScriptCompiler::IsValidVariableName(""));
    CHECK_FALSE(ScriptCompiler::IsValidVariableName("doorref"));
    CHECK_FALSE(ScriptCompiler::IsValidVariableName("GetPos"));
    CHECK_FALSE(ScriptCompiler::IsValidVariableName("endif"));
}

// ---- reading lines and words -----------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(Fixture, "GetNextLine skips blank lines, leading blanks and comments") {
    ScriptCompileData kData;
    std::string kInput = "\r\n  ; a comment\r\n\t  set x to 1   ; and another\r\nreturn";
    kData.input = kInput.data();

    SCRIPT_LINE kFirst;
    kFirst.uiLineNumber = 1;
    const int32_t iRead = Compiler::GetNextLine(&kData, &kFirst);
    CHECK(std::string(kFirst.pLine) == "set x to 1");
    CHECK(kFirst.uiSize == 10);
    // Two lines were skipped, and the third ended at the comment's line break.
    CHECK(kFirst.uiLineNumber == 3);
    CHECK(kInput.substr(iRead) == "return");

    kData.uiInputOffset += iRead;
    SCRIPT_LINE kSecond;
    // A line the input ends on is read with the input's terminator.
    CHECK(Compiler::GetNextLine(&kData, &kSecond) == 7);
    CHECK(std::string(kSecond.pLine) == "return");
}

TEST_CASE_FIXTURE(Fixture, "GetNextLine returns 0 at the end of the input and for a line that is too long") {
    ScriptCompileData kData;
    std::string kBlank = "  \r\n ; nothing here";
    kData.input = kBlank.data();
    SCRIPT_LINE kLine;
    CHECK(Compiler::GetNextLine(&kData, &kLine) == 0);
    CHECK(kLine.eScriptError == ERROR_SCRIPT_NOERROR);

    std::string kLong(600, 'w');
    kData.input = kLong.data();
    SCRIPT_LINE kTooLong;
    CHECK(Compiler::GetNextLine(&kData, &kTooLong) == 0);
    CHECK(kTooLong.eScriptError == ERROR_SCRIPT_LINE_TOO_LONG);
    REQUIRE(Warnings().size() == 1);
    CHECK(Warnings()[0].find("Max script line length (512 characters) exceeded.") != std::string::npos);
}

TEST_CASE_FIXTURE(Fixture, "GetNextWord reads words, quoted names and variables") {
    ScriptCompileData kData;
    ScriptVariable* pVariable = ::new (ourMalloc(sizeof(ScriptVariable))) ScriptVariable;
    pVariable->cName.Set("counter", 0);
    pVariable->data.uiID = 3;
    pVariable->data.bIsInteger = true;
    kData.listVariables.AddTail(pVariable);

    char acLine[] = "counter, \"Some Name\" other_word-2";
    uint32_t uiOffset = 0;

    SCRIPT_WORD kCounter;
    CHECK(ScriptCompiler::GetNextWord(&kData, &kCounter, acLine, uiOffset, true, false) == 7);
    CHECK(std::string(kCounter.pText) == "counter");
    CHECK(kCounter.cVariableType == 's');
    CHECK(kCounter.uiVariableID == 3);

    SCRIPT_WORD kQuoted;
    CHECK(ScriptCompiler::GetNextWord(&kData, &kQuoted, acLine, uiOffset, false, false) == 9);
    CHECK(std::string(kQuoted.pText) == "Some Name");
    // The quotes are put back.
    CHECK(std::string(acLine) == "counter, \"Some Name\" other_word-2");

    SCRIPT_WORD kOther;
    CHECK(ScriptCompiler::GetNextWord(&kData, &kOther, acLine, uiOffset, true, false) == 12);
    CHECK(std::string(kOther.pText) == "other_word-2");
    CHECK(kOther.cVariableType == 0);

    SCRIPT_WORD kNone;
    CHECK(ScriptCompiler::GetNextWord(&kData, &kNone, acLine, uiOffset, true, false) == 0);
}

TEST_CASE_FIXTURE(Fixture, "GetNextWord resolves a variable on a quest through its reference") {
    world::AddQuest("MyQuest", {{"stage", 7, true}});
    ScriptCompileData kData;
    char acLine[] = "MyQuest.stage";
    uint32_t uiOffset = 0;

    SCRIPT_WORD kWord;
    CHECK(ScriptCompiler::GetNextWord(&kData, &kWord, acLine, uiOffset, true, false) == 5);
    CHECK(std::string(kWord.pText) == "stage");
    CHECK(kWord.uiRefObjectIndex == 1);
    CHECK(kWord.cVariableType == 's');
    CHECK(kWord.uiVariableID == 7);
    CHECK(kData.header.refObjectCount == 1);
}

TEST_CASE_FIXTURE(Fixture, "GetNextWord reports a reference where none is allowed") {
    world::AddQuest("MyQuest", {{"stage", 7, true}});
    ScriptCompileData kData;
    char acLine[] = "MyQuest.stage";
    uint32_t uiOffset = 0;

    SCRIPT_WORD kWord;
    CHECK(ScriptCompiler::GetNextWord(&kData, &kWord, acLine, uiOffset, false, false) == 0);
    REQUIRE(Warnings().size() == 1);
    CHECK(Warnings()[0].ends_with("Syntax Error.  Reference not allowed in this context."));
}

// ---- blocks ----------------------------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(Fixture, "VerifyBlocks wants every command inside one level of begin/end") {
    SUBCASE("well formed") {
        ScriptCompileData kData;
        QueueLine(kData, 1, FLOW_SCRIPTNAME);
        QueueLine(kData, 2, FLOW_SHORT);
        QueueLine(kData, 3, FLOW_BEGIN);
        QueueLine(kData, 4, FLOW_SET);
        QueueLine(kData, 5, FLOW_END);
        CHECK(Compiler().VerifyBlocks(&kData));
        CHECK(Warnings().empty());
    }
    SUBCASE("a command outside a block") {
        ScriptCompileData kData;
        QueueLine(kData, 1, FLOW_SCRIPTNAME);
        QueueLine(kData, 2, FLOW_SET);
        CHECK_FALSE(Compiler().VerifyBlocks(&kData));
        CHECK(kData.uiLastLineNumber == 2);
        REQUIRE(Warnings().size() == 1);
        CHECK(Warnings()[0].ends_with("All script commands must be inside a begin/end block."));
    }
    SUBCASE("nested blocks") {
        ScriptCompileData kData;
        QueueLine(kData, 1, FLOW_BEGIN);
        QueueLine(kData, 2, FLOW_BEGIN);
        CHECK_FALSE(Compiler().VerifyBlocks(&kData));
        REQUIRE(Warnings().size() == 1);
        CHECK(Warnings()[0].ends_with("Nested begin/end blocks are not allowed."));
    }
    SUBCASE("an end without a begin") {
        ScriptCompileData kData;
        QueueLine(kData, 1, FLOW_END);
        CHECK_FALSE(Compiler().VerifyBlocks(&kData));
        REQUIRE(Warnings().size() == 1);
        CHECK(Warnings()[0].ends_with("End without matching Begin."));
    }
}

TEST_CASE_FIXTURE(Fixture, "CountIfLines counts the lines up to the if's own else, elseif or endif") {
    ScriptCompileData kData;
    SCRIPT_LINE* pIf = QueueLine(kData, 1, FLOW_IF);
    QueueLine(kData, 2, FLOW_SET);
    QueueLine(kData, 3, FLOW_IF); // nested: counted with everything up to its endif
    QueueLine(kData, 4, FLOW_RETURN);
    QueueLine(kData, 5, FLOW_ENDIF);
    QueueLine(kData, 6, FLOW_SHORT); // a declaration is not counted
    SCRIPT_LINE* pElse = QueueLine(kData, 7, FLOW_ELSE);
    QueueLine(kData, 8, FLOW_SET);
    QueueLine(kData, 9, FLOW_ENDIF);

    CHECK(kData.listLines.ItemsUntilNull().begin() != kData.listLines.ItemsUntilNull().end());
    CHECK(Compiler().CountIfLines(&kData, pIf) == 4);
    CHECK(Compiler().CountIfLines(&kData, pElse) == 1);
    CHECK(Warnings().empty());
}

TEST_CASE_FIXTURE(Fixture, "CountIfLines fails without an endif and past ten nested ifs") {
    SUBCASE("no endif") {
        ScriptCompileData kData;
        SCRIPT_LINE* pIf = QueueLine(kData, 1, FLOW_IF);
        QueueLine(kData, 2, FLOW_SET);
        CHECK(Compiler().CountIfLines(&kData, pIf) == -1);
        REQUIRE(Warnings().size() == 1);
        CHECK(Warnings()[0].ends_with("Mismatched if/else/endif block starting on line 1."));
    }
    SUBCASE("too deep") {
        ScriptCompileData kData;
        SCRIPT_LINE* pIf = QueueLine(kData, 1, FLOW_IF);
        for (uint32_t i = 2; i <= 11; ++i) {
            QueueLine(kData, i, FLOW_IF);
        }
        CHECK(Compiler().CountIfLines(&kData, pIf) == -1);
        REQUIRE(Warnings().size() == 1);
        CHECK(Warnings()[0].ends_with("Max nesting of 10 exceeded."));
    }
}

TEST_CASE_FIXTURE(Fixture, "CountBlockLines counts the lines up to the block's end") {
    ScriptCompileData kData;
    SCRIPT_LINE* pBegin = QueueLine(kData, 1, FLOW_BEGIN);
    QueueLine(kData, 2, FLOW_SET);
    QueueLine(kData, 3, FLOW_IF);
    QueueLine(kData, 4, FLOW_ENDIF);
    QueueLine(kData, 5, FLOW_END);
    QueueLine(kData, 6, FLOW_BEGIN);
    CHECK(Compiler().CountBlockLines(&kData, pBegin) == 3);
}

// ---- parameters ------------------------------------------------------------------------------------------------------

TEST_CASE_FIXTURE(Fixture, "StandardCompile compiles numbers, axes and actor values") {
    const SCRIPT_PARAMETER kParameters[] = {
        Param("Count", SCRIPT_PARAM_INT), Param("Amount", SCRIPT_PARAM_FLOAT), Param("Axis", SCRIPT_PARAM_AXIS),
        Param("ActorValue", SCRIPT_PARAM_ACTOR_VALUE), Param("Sex", SCRIPT_PARAM_SEX)};
    ScriptCompileData kData;
    SCRIPT_LINE kLine;
    std::strcpy(kLine.pLine, "12, 2.5 y Health female");
    kLine.uiSize = std::strlen(kLine.pLine);

    REQUIRE(ScriptCompiler::StandardCompile(5, kParameters, &kLine, &kData));
    const Bytes kExpected = Bytes()
                                .U16(5) // the parameter count
                                .Char('n')
                                .U32(12)
                                .Char('z')
                                .F64(2.5)
                                .Char('Y')
                                .U16(ActorValue::HEALTH)
                                .U16(1);
    CHECK(Hex(kLine.pOutput, kLine.uiOutputSize) == kExpected.Hex());
    CHECK(Warnings().empty());
}

TEST_CASE_FIXTURE(Fixture, "StandardCompile stops at the first optional parameter left out") {
    const SCRIPT_PARAMETER kParameters[] = {
        Param("Count", SCRIPT_PARAM_INT), Param("Flag", SCRIPT_PARAM_INT, true), Param("Other", SCRIPT_PARAM_INT, true)};
    ScriptCompileData kData;
    SCRIPT_LINE kLine;
    std::strcpy(kLine.pLine, "3");
    kLine.uiSize = 1;

    REQUIRE(ScriptCompiler::StandardCompile(3, kParameters, &kLine, &kData));
    // The count says how many were given.
    CHECK(Hex(kLine.pOutput, kLine.uiOutputSize) == Bytes().U16(1).Char('n').U32(3).Hex());
}

TEST_CASE_FIXTURE(Fixture, "StandardCompile reports what it cannot compile") {
    ScriptCompileData kData;
    SCRIPT_LINE kLine;
    const auto Compile = [&](std::initializer_list<SCRIPT_PARAMETER> aParameters, const char* apText) {
        Reported().Clear();
        std::strcpy(kLine.pLine, apText);
        kLine.uiSize = std::strlen(apText);
        kLine.uiOutputSize = 0;
        const bool bCompiled = ScriptCompiler::StandardCompile(
            static_cast<uint16_t>(aParameters.size()), aParameters.begin(), &kLine, &kData
        );
        REQUIRE(Warnings().size() == (bCompiled ? 0u : 1u));
        return bCompiled ? std::string() : Warnings()[0].substr(Warnings()[0].find('\n') + 1);
    };

    CHECK(Compile({Param("Count", SCRIPT_PARAM_INT)}, "") == "Missing parameter Count.\r\nCompiled script not saved!");
    CHECK(
        Compile({Param("Count", SCRIPT_PARAM_INT)}, "many") ==
        "Unknown variable 'many' for parameter Count.\r\nCompiled script not saved!"
    );
    CHECK(Compile({Param("Axis", SCRIPT_PARAM_AXIS)}, "w") == "Axis (X,Y,Z) required for parameter Axis.\r\nCompiled script not saved!");
    CHECK(
        Compile({Param("Stat", SCRIPT_PARAM_ACTOR_VALUE)}, "Sneakiness") ==
        "Invalid actor value 'Sneakiness' for parameter Stat.\r\nCompiled script not saved!"
    );
    CHECK(
        Compile({Param("Crime", SCRIPT_PARAM_CRIME_TYPE)}, "5") ==
        "Invalid crime type '5' for parameter Crime.  Crime type must be a numeric value from 0-4.\r\nCompiled "
        "script not saved!"
    );
    CHECK(Compile({Param("Count", SCRIPT_PARAM_INT)}, "1 2") == "Expected end of line.\r\nCompiled script not saved!");
}

TEST_CASE_FIXTURE(Fixture, "StandardCompile checks what a referenced-object parameter names") {
    FakeForm& kChest = world::AddReference("ChestRef");
    kChest.bHasContainer = true;
    world::AddReference("RockRef");
    world::AddForm("Stimpak", ALCH_ID, kObject);
    world::AddReference("TempRef").bPersists = false;

    ScriptCompileData kData;
    SCRIPT_LINE kLine;
    const auto Compile = [&](SCRIPT_PARAM_TYPE aeType, const char* apText) {
        Reported().Clear();
        const SCRIPT_PARAMETER kParameter = Param("Target", aeType);
        std::strcpy(kLine.pLine, apText);
        kLine.uiSize = std::strlen(apText);
        kLine.uiOutputSize = 0;
        const bool bCompiled = ScriptCompiler::StandardCompile(1, &kParameter, &kLine, &kData);
        return bCompiled ? std::string() : Warnings().back().substr(Warnings().back().find('\n') + 1);
    };

    // A referenced object is compiled as its index in the script's referenced objects.
    CHECK(Compile(SCRIPT_PARAM_CONTAINER_REF, "ChestRef").empty());
    CHECK(Hex(kLine.pOutput, kLine.uiOutputSize) == Bytes().U16(1).Char('r').U16(1).Hex());
    CHECK(Compile(SCRIPT_PARAM_INVENTORY_OBJECT, "Stimpak").empty());
    CHECK(Hex(kLine.pOutput, kLine.uiOutputSize) == Bytes().U16(1).Char('r').U16(2).Hex());
    // The same object again is the same index.
    CHECK(Compile(SCRIPT_PARAM_OBJECTREF, "chestref").empty());
    CHECK(Hex(kLine.pOutput, kLine.uiOutputSize) == Bytes().U16(1).Char('r').U16(1).Hex());
    CHECK(kData.header.refObjectCount == 2);

    CHECK(
        Compile(SCRIPT_PARAM_CONTAINER_REF, "RockRef") ==
        "Invalid container reference 'RockRef' for parameter Target.\r\nCompiled script not saved!"
    );
    CHECK(
        Compile(SCRIPT_PARAM_ACTOR, "RockRef") ==
        "Invalid actor 'RockRef' for parameter Target.\r\nCompiled script not saved!"
    );
    CHECK(Compile(SCRIPT_PARAM_ACTOR, "player").empty());
    CHECK(
        Compile(SCRIPT_PARAM_INVENTORY_OBJECT, "RockRef") ==
        "Invalid inventory object 'RockRef' for parameter Target.\r\nCompiled script not saved!"
    );
    CHECK(
        Compile(SCRIPT_PARAM_OBJECTREF, "Nowhere") ==
        "Item 'Nowhere' not found for parameter Target.\r\nCompiled script not saved!"
    );
    CHECK(
        Compile(SCRIPT_PARAM_OBJECTREF, "TempRef") ==
        "Item 'TempRef' not found for parameter Target.\r\nCompiled script not saved!"
    );
    CHECK(Warnings().front().ends_with("Non-persistent reference 'TempRef' cannot be used in a script."));
}

// ---- whole scripts ---------------------------------------------------------------------------------------------------

namespace {

    // The world the script tests compile in: a GameMode and an OnActivate block, and a few functions.
    struct ScriptFixture : Fixture {
        ScriptFixture() {
            world::DefineBlock(SCRIPT_BLOCK_GAME, "GameMode");
            world::DefineBlock(
                SCRIPT_BLOCK_ONACTIVATE, "OnActivate", {Param("ActivatingRef", SCRIPT_PARAM_ACTOR, true)}, true
            );
            world::DefineFunction(Function(0), "GetSecondsPassed", "");
            world::DefineFunction(Function(1), "SetStage", "", {Param("Quest", SCRIPT_PARAM_QUEST), Param("Stage", SCRIPT_PARAM_STAGE)});
            world::DefineFunction(Function(2), "Activate", "", {Param("ActionRef", SCRIPT_PARAM_OBJECTREF, true)}, true);
            world::DefineFunction(Function(3), "PlayGroup", "", {Param("AnimGroup", SCRIPT_PARAM_ANIM_GROUP), Param("Flags", SCRIPT_PARAM_INT)}, true);
            world::DefineFunction(Console(0), "ToggleGodMode", "tgm");
            world::NameAnimGroup(ANIM_GROUP_EQUIP, "Equip");
        }

        // Compiles the text as the editor compiles a full script.
        bool CompileScript(const char* apText) {
            FakeScript& kScript = world::AddScript("TestScript", apText);
            script = &kScript;
            ScriptCompileData kData;
            kData.input = kScript.text.data();
            return compiler.Compile(kScript.AsScript(), &kData);
        }

        FakeScript* script = nullptr;
    };

} // namespace

TEST_CASE_FIXTURE(ScriptFixture, "Compile turns a script into its bytecode") {
    REQUIRE(CompileScript(
        "scn TestScript\r\n"
        "short counter\r\n"
        "float timer\r\n"
        "\r\n"
        "begin GameMode\r\n"
        "  set timer to GetSecondsPassed\r\n"
        "  if counter\r\n"
        "    return\r\n"
        "  endif\r\n"
        "end\r\n"
    ));
    CHECK(Warnings().empty());
    REQUIRE(script->bInitialised);
    CHECK(script->header.bIsCompiled);
    CHECK(script->compiledName == "TestScript");
    // The declarations only add variables: nothing of them is compiled.
    CHECK(script->compiledVariables == std::vector<Variable>{{"counter", 1, true}, {"timer", 2, false}});
    CHECK(script->compiledHeader.m_uiLastID == 2);
    CHECK(script->compiledHeader.refObjectCount == 0);

    const Bytes kExpected =
        Bytes()
            .Append(Line(FLOW_SCRIPTNAME))
            // The block's type, then its length: its lines' bytes and its end line's.
            .Append(Line(FLOW_BEGIN, Bytes().U16(SCRIPT_BLOCK_GAME).U32(35 + 4)))
            // set: the variable ('f', ID 2), then the postfix expression's size and the expression, a call of
            // GetSecondsPassed ('X', its code, the size of its parameters).
            .Append(Line(FLOW_SET, Bytes().Char('f').U16(2).U16(6).Text(" X").U16(Function(0)).U16(0)))
            // if: the lines it skips when false (the return), then its expression, the variable 's' ID 1.
            .Append(Line(FLOW_IF, Bytes().U16(1).U16(4).Text(" s").U16(1)))
            .Append(Line(FLOW_RETURN))
            .Append(Line(FLOW_ENDIF))
            .Append(Line(FLOW_END));
    CHECK(Hex(script->compiled) == kExpected.Hex());
    CHECK(script->compiledHeader.dataSize == kExpected.Size());
}

TEST_CASE_FIXTURE(ScriptFixture, "Compile writes how many lines each branch of an if skips") {
    REQUIRE(CompileScript(
        "scn TestScript\r\n"
        "short x\r\n"
        "begin GameMode\r\n"
        "if x\r\n"
        "set x to 1\r\n"
        "set x to 2\r\n"
        "elseif x\r\n"
        "return\r\n"
        "else\r\n"
        "endif\r\n"
        "end\r\n"
    ));
    const Bytes kSet1 = Line(FLOW_SET, Bytes().Char('s').U16(1).U16(2).Text(" 1"));
    const Bytes kSet2 = Line(FLOW_SET, Bytes().Char('s').U16(1).U16(2).Text(" 2"));
    const Bytes kBody = Bytes()
                            .Append(Line(FLOW_IF, Bytes().U16(2).U16(4).Text(" s").U16(1)))
                            .Append(kSet1)
                            .Append(kSet2)
                            .Append(Line(FLOW_ELSEIF, Bytes().U16(1).U16(4).Text(" s").U16(1)))
                            .Append(Line(FLOW_RETURN))
                            .Append(Line(FLOW_ELSE, Bytes().U16(0)))
                            .Append(Line(FLOW_ENDIF));
    const Bytes kExpected = Bytes()
                                .Append(Line(FLOW_SCRIPTNAME))
                                .Append(Line(FLOW_BEGIN, Bytes().U16(SCRIPT_BLOCK_GAME).U32(kBody.Size() + 4)))
                                .Append(kBody)
                                .Append(Line(FLOW_END));
    CHECK(Hex(script->compiled) == kExpected.Hex());
}

TEST_CASE_FIXTURE(ScriptFixture, "Compile compiles function calls, their parameters and the references they run on") {
    world::AddQuest("MyQuest");
    world::AddReference("ChestRef");
    REQUIRE(CompileScript(
        "scn TestScript\r\n"
        "begin OnActivate player\r\n"
        "SetStage MyQuest 10\r\n"
        "ChestRef.Activate player\r\n"
        "ChestRef.PlayGroup Equip 1\r\n"
        "end\r\n"
    ));
    CHECK(Warnings().empty());
    // Each object the script names is referenced once. All the lines are read before any is compiled, so the
    // references the lines' first words name come first, then the objects the parameters name.
    CHECK(script->compiledRefObjects == std::vector<std::string>{"ChestRef", "player", "MyQuest"});
    CHECK(script->compiledHeader.refObjectCount == 3);

    const Bytes kBody =
        Bytes()
            // The parameter count, then each parameter: a referenced object by its index, a number as 'n' and
            // its value.
            .Append(Line(Function(1), Bytes().U16(2).Char('r').U16(3).Char('n').U32(10)))
            // A function run on a reference: the reference's index, then the call.
            .U16(FLOW_REFERENCE_FUNCTION)
            .U16(1)
            .Append(Line(Function(2), Bytes().U16(1).Char('r').U16(2)))
            .U16(FLOW_REFERENCE_FUNCTION)
            .U16(1)
            .Append(Line(Function(3), Bytes().U16(2).U16(ANIM_GROUP_EQUIP).Char('n').U32(1)));
    // The block's parameters follow its length.
    const Bytes kExpected =
        Bytes()
            .Append(Line(FLOW_SCRIPTNAME))
            .Append(Line(
                FLOW_BEGIN, Bytes().U16(SCRIPT_BLOCK_ONACTIVATE).U32(kBody.Size() + 4).U16(1).Char('r').U16(2)
            ))
            .Append(kBody)
            .Append(Line(FLOW_END));
    CHECK(Hex(script->compiled) == kExpected.Hex());
}

TEST_CASE_FIXTURE(ScriptFixture, "Compile reports a script it cannot compile, and leaves the script alone") {
    // What the compile reported, without the "SCRIPTS: Script 'name', line n:" each message starts with.
    const auto Errors = [&](const char* apText) {
        Reported().Clear();
        CHECK_FALSE(CompileScript(apText));
        CHECK_FALSE(script->bInitialised);
        CHECK_FALSE(script->header.bIsCompiled);
        std::vector<std::string> kErrors;
        for (const std::string& kWarning : Warnings()) {
            kErrors.push_back(kWarning.substr(kWarning.find('\n') + 1));
        }
        return kErrors;
    };
    using Messages = std::vector<std::string>;

    CHECK(Errors("begin GameMode\r\nend\r\n") == Messages{"You need to start scripts with \"scriptname ScriptName\""});
    // A script without a name yet is named "(null)" in the message, which is printf's text for a null string.
    CHECK(Warnings()[0] == "SCRIPTS: Script '(null)', line 0:\nYou need to start scripts with \"scriptname ScriptName\"");

    CHECK(Errors("scn TestScript\r\nbegin GameMode\r\nJump\r\nend\r\n") == Messages{"Script command \"Jump\" not found."});
    CHECK(Warnings()[0].starts_with("SCRIPTS: Script 'TestScript', line 3:\n"));

    // A console command stops the reading but is not an error of the input: the lines read so far are compiled,
    // and here the block they open has no end.
    CHECK(
        Errors("scn TestScript\r\nbegin GameMode\r\ntgm\r\nend\r\n") ==
        Messages{
            "Script command \"tgm\" is a console-only command.", "Mismatched begin/end block starting on line 2.",
            "Mismatched begin/end block."
        }
    );

    CHECK(Errors("scn TestScript\r\nreturn\r\n") == Messages{"All script commands must be inside a begin/end block."});
    CHECK(Errors("scn TestScript\r\nbegin GameMode\r\nset y to 1\r\nend\r\n") == Messages{"Unknown variable 'y'."});
    CHECK(
        Errors("scn TestScript\r\nbegin NoSuchBlock\r\nend\r\n") ==
        Messages{"Syntax Error.  Invalid block type in 'begin' command."}
    );
    CHECK(
        Errors("scn TestScript\r\nshort Activate\r\n") ==
        Messages{"Invalid variable name 'Activate'.\r\nCompiled script not saved!"}
    );
}

TEST_CASE_FIXTURE(ScriptFixture, "A line ends at a carriage return: a bare newline is part of the line") {
    // The editor saves scripts with CRLF line breaks. Here everything after the name is read as part of the scriptname
    // line, which compiles to the name alone.
    REQUIRE(CompileScript("scn TestScript\nbegin GameMode\nJump\nend\n"));
    CHECK(script->compiledName == "TestScript");
    CHECK(Hex(script->compiled) == Line(FLOW_SCRIPTNAME).Hex());
}

TEST_CASE_FIXTURE(ScriptFixture, "A quest script must name the reference a reference function runs on") {
    FakeScript& kScript = world::AddScript("QuestScript", "scn QuestScript\r\nbegin GameMode\r\nActivate\r\nend\r\n");
    kScript.header.bIsQuestScript = true;
    ScriptCompileData kData;
    kData.input = kScript.text.data();
    CHECK_FALSE(compiler.Compile(kScript.AsScript(), &kData));
    REQUIRE(Warnings().size() == 1);
    CHECK(Warnings()[0].ends_with("Reference function requires explicit reference in quest script."));
}

TEST_CASE_FIXTURE(ScriptFixture, "CompilePartialScript compiles console lines, and reports to the console") {
    FakeScript& kScript = world::AddScript("", "tgm\r\n");
    REQUIRE(compiler.CompilePartialScript(kScript.AsScript(), nullptr, SYSTEM_WINDOW_COMPILER, false));
    CHECK(kScript.compiledName == "SysWindowCompileAndRun");
    CHECK(Hex(kScript.compiled) == Line(Console(0)).Hex());

    kScript.text = "Jump\r\n";
    CHECK_FALSE(compiler.CompilePartialScript(kScript.AsScript(), nullptr, SYSTEM_WINDOW_COMPILER, false));
    CHECK(Warnings().empty());
    CHECK(
        Reported().console ==
        std::vector<std::string>{"SCRIPTS: Script 'SysWindowCompileAndRun', line 1:\nScript command \"Jump\" not found."}
    );
}

TEST_CASE_FIXTURE(ScriptFixture, "CompilePartialScript reads the variables of the script its owner runs") {
    FakeScript& kOwnerScript = world::AddScript("OwnerScript");
    kOwnerScript.variables = {{"health", 4, false}};
    FakeForm& kOwner = world::AddReference("OwnerRef");
    kOwner.pScript = &kOwnerScript;

    FakeScript& kResult = world::AddScript("", "set health to 5\r\n");
    REQUIRE(compiler.CompilePartialScript(kResult.AsScript(), kOwner.Reference(), DIALOGUE_COMPILER, false));
    CHECK(kResult.compiledName == "OwnerScript");
    CHECK(Hex(kResult.compiled) == Line(FLOW_SET, Bytes().Char('f').U16(4).U16(2).Text(" 5")).Hex());
}
