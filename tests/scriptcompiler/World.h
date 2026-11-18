#pragma once

// The game the script compiler is tested against: fake forms and scripts, and the function tables. World.cpp defines
// what TESScriptCompiler.cpp and ScriptCompileData.cpp call on forms and scripts (TESForm::GetFormType,
// Script::FindVariable...) to read these fakes, so no real form is ever built.

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/SCRIPT_PARAMETER.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESScriptCompiler.h"

#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

class TESObjectREFR;

namespace test {

    // The classes a fake form is, for dynamic_cast: one bit for each class the compiler casts a form to.
    enum FormClass : uint32_t {
        kReference = 1u << 0, // TESObjectREFR
        kQuest = 1u << 1,     // TESQuest
        kSpellItem = 1u << 2,
        kBook = 1u << 3,
        kCell = 1u << 4,
        kMagicItem = 1u << 5,
        kObject = 1u << 6,
        kActorBase = 1u << 7,
        kWorldSpace = 1u << 8,
        kPackage = 1u << 9,
        kCombatStyle = 1u << 10,
        kEffectSetting = 1u << 11,
        kListForm = 1u << 12,
        kMenuIcon = 1u << 13,
        kPerk = 1u << 14,
        kNote = 1u << 15,
        kEncounterZone = 1u << 16,
        kIdleForm = 1u << 17,
    };

    struct FakeScript;

    // A form as the compiler sees it. Its virtual calls go through vptr, which must come first, to World.cpp's
    // stand-ins; any other virtual call fails the test.
    struct FakeForm {
        FakeForm(const char* apEditorID, ENUM_FORM_ID aeType, uint32_t auiClasses);
        FakeForm(const FakeForm&) = delete;
        FakeForm& operator=(const FakeForm&) = delete;

        TESForm* Form() {
            return reinterpret_cast<TESForm*>(this);
        }
        TESObjectREFR* Reference() {
            return reinterpret_cast<TESObjectREFR*>(this);
        }

        void* const* vptr;
        char acEditorID[64];
        ENUM_FORM_ID eType;
        uint32_t uiClasses;
        // what TESForm::GetFormByNumericID finds it by
        uint32_t uiFormID = 0;
        // TESObjectREFR::GetRefPersists
        bool bPersists = true;
        // TESForm::IsActor and IsObject
        bool bIsActor = false;
        bool bIsObject = false;
        // TESObjectCELL::GetInterior
        bool bInterior = false;
        // TESObjectREFR::HasContainer
        bool bHasContainer = false;
        // TESObjectREFR::GetObjectReference
        FakeForm* pBaseObject = nullptr;
        // a reference's script (TESObjectREFR::GetScriptPointer) or a quest's (TESScriptableForm::GetFormScript)
        FakeScript* pScript = nullptr;
    };

    // A script variable: the ones a script already has, and the ones a compile hands Script::Init.
    struct Variable {
        std::string name;
        uint32_t uiID;
        bool bIsInteger;

        bool operator==(const Variable&) const = default;
    };

    // A script as the compiler sees it: its text and header going in, and what Script::Init received coming out.
    struct FakeScript : FakeForm {
        explicit FakeScript(const char* apEditorID);
        ~FakeScript();

        Script* AsScript() {
            return reinterpret_cast<Script*>(this);
        }

        // Script::GetText, GetHeader, FindVariable and GetReferencedObjectList.
        std::string text;
        SCRIPT_HEADER header{};
        std::vector<Variable> variables;
        BSSimpleList<Script::RefVariable*> listRefObjects;

        // What the last compile handed Script::Init: its name, header, compiled data, variables and referenced
        // objects (their names).
        bool bInitialised = false;
        std::string compiledName;
        SCRIPT_HEADER compiledHeader{};
        std::vector<uint8_t> compiled;
        std::vector<Variable> compiledVariables;
        std::vector<std::string> compiledRefObjects;
    };

    // The world the compiler works in. Reset puts it back to how every test starts:
    //
    // - no forms but the player ("player", pPlayer), and no messages reported;
    // - every function, console and block table entry named "" and without parameters, the tables' end markers
    //   set, and every animation group named "".
    //
    // A test then adds the forms and functions it uses.
    namespace world {

        void Reset();

        // A form with the editor ID, found by TESForm::GetFormByEditorID (ignoring case) and by its form ID.
        FakeForm& AddForm(const char* apEditorID, ENUM_FORM_ID aeType, uint32_t auiClasses = 0);
        // A persistent reference (REFR_ID, kReference).
        FakeForm& AddReference(const char* apEditorID);
        // A global (GLOB_ID), which TESDataHandler::GetGlobal finds too.
        FakeForm& AddGlobal(const char* apEditorID);
        // A quest (QUST_ID, kQuest) with a quest script that has the variables.
        FakeForm& AddQuest(const char* apEditorID, std::initializer_list<Variable> aVariables = {});
        // A script, not found by editor ID: the one a test compiles, or the one a form has.
        FakeScript& AddScript(const char* apEditorID, const char* apText = "");

        // Fills the function table entry of acCode (a console code from 0x100 or a script function code from 0x1000)
        // with the names and the parameters, which compile through ScriptCompiler::StandardCompile.
        SCRIPT_FUNCTION& DefineFunction(
            SCRIPT_OUTPUT aeCode,
            const char* apName,
            const char* apShortName,
            std::initializer_list<SCRIPT_PARAMETER> aParameters = {},
            bool abReferenceFunction = false
        );
        // Fills the block table entry of aeBlock, as DefineFunction does.
        SCRIPT_FUNCTION& DefineBlock(
            SCRIPT_BLOCK aeBlock,
            const char* apName,
            std::initializer_list<SCRIPT_PARAMETER> aParameters = {},
            bool abReferenceFunction = false
        );
        void NameAnimGroup(int32_t aiGroup, const char* apName);

    } // namespace world

} // namespace test
