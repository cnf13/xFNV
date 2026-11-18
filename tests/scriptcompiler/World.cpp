#include "scriptcompiler/World.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/ScriptCompileData.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScriptVariable.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "support/Messages.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <dlfcn.h>
#include <iterator>
#include <typeinfo>
#include <unordered_set>

// The tables the game defines in TESScriptShared.cpp and TESAnimGroup.cpp; world::Reset fills them.
ScriptFunctionsTable scriptFunctions;
ScriptConsoleTable scriptConsole;
scriptBlocks_t scriptBlocks;
AnimGroupInfoArray animGroupInfo;

namespace {

    using test::FakeForm;
    using test::FakeScript;

    // A call the fakes do not stand in for: the test needs a fake that does.
    [[noreturn]] void Unsupported(const char* apWhat) {
        std::fprintf(stderr, "scriptcompiler test: %s is not faked (tests/scriptcompiler/World.cpp)\n", apWhat);
        std::abort();
    }

    struct State {
        std::deque<FakeForm> forms;
        std::deque<FakeScript> scripts;
        // every fake's address, for dynamic_cast
        std::unordered_set<const void*> fakes;
        std::deque<std::vector<SCRIPT_PARAMETER>> parameters;
        uint32_t uiNextFormID = 0x1000;
    };

    State& World() {
        static State kState;
        return kState;
    }

    FakeForm* AsFake(const void* apForm) {
        return static_cast<FakeForm*>(const_cast<void*>(apForm));
    }

    FakeScript* AsFakeScript(const void* apScript) {
        return static_cast<FakeScript*>(const_cast<void*>(apScript));
    }

    // ---- the fake forms' virtual table ------------------------------------------------------------------------------

    void UnexpectedVirtual(FakeForm*) {
        Unsupported("this virtual call on a form");
    }
    bool IsObject(FakeForm* apSelf) {
        return apSelf->bIsObject;
    }
    bool IsActor(FakeForm* apSelf) {
        return apSelf->bIsActor;
    }
    const char* GetFormEditorID(FakeForm* apSelf) {
        return apSelf->acEditorID;
    }

    // The slot of a virtual function: under the Itanium ABI a pointer to a virtual member holds 1 + the slot's byte
    // offset in the table.
    template <typename Member>
    size_t Slot(Member aMember) {
        struct {
            uintptr_t uiPointer;
            ptrdiff_t iAdjust;
        } kRaw;
        static_assert(sizeof(aMember) == sizeof(kRaw));
        std::memcpy(&kRaw, &aMember, sizeof(kRaw));
        return (kRaw.uiPointer - 1) / sizeof(void*);
    }

    // The two entries before the address point (offset to top, type info), then the virtual functions.
    void* const* VirtualTable() {
        static void* kTable[2 + 256];
        static const bool bBuilt = [] {
            for (void*& pEntry : kTable) {
                pEntry = reinterpret_cast<void*>(&UnexpectedVirtual);
            }
            const auto Set = [](size_t auiSlot, void* apFunction) {
                if (2 + auiSlot >= std::size(kTable)) {
                    Unsupported("a virtual slot past the fake table");
                }
                kTable[2 + auiSlot] = apFunction;
            };
            Set(Slot(&TESForm::IsObject), reinterpret_cast<void*>(&IsObject));
            Set(Slot(&TESForm::IsActor), reinterpret_cast<void*>(&IsActor));
            Set(Slot(&TESForm::GetFormEditorID), reinterpret_cast<void*>(&GetFormEditorID));
            return true;
        }();
        static_cast<void>(bBuilt);
        return &kTable[2];
    }

    void AddParameters(SCRIPT_FUNCTION& arEntry, std::initializer_list<SCRIPT_PARAMETER> aParameters) {
        if (aParameters.size() == 0) {
            arEntry.pParameters = nullptr;
            arEntry.sParamCount = 0;
            arEntry.pCompileFunction = nullptr;
            return;
        }
        std::vector<SCRIPT_PARAMETER>& kStored = World().parameters.emplace_back(aParameters);
        arEntry.pParameters = kStored.data();
        arEntry.sParamCount = static_cast<int16_t>(kStored.size());
        arEntry.pCompileFunction = &ScriptCompiler::StandardCompile;
    }

    // A table entry with the code, named "" (the compiler compares every entry's names: none may be null), and
    // without parameters.
    SCRIPT_FUNCTION Unnamed(SCRIPT_OUTPUT aeCode) {
        SCRIPT_FUNCTION kEntry{};
        kEntry.pFunctionName = "";
        kEntry.pShortName = "";
        kEntry.eOutput = aeCode;
        return kEntry;
    }

    FakeForm* FindForm(const char* apEditorID, ENUM_FORM_ID aeType = NO_FORM) {
        for (FakeForm& kForm : World().forms) {
            if ((aeType == NO_FORM || kForm.eType == aeType) && !BSstricmp(kForm.acEditorID, apEditorID)) {
                return &kForm;
            }
        }
        return nullptr;
    }

} // namespace

namespace test {

    FakeForm::FakeForm(const char* apEditorID, ENUM_FORM_ID aeType, uint32_t auiClasses)
      : vptr(VirtualTable()), acEditorID(), eType(aeType), uiClasses(auiClasses) {
        std::snprintf(acEditorID, sizeof(acEditorID), "%s", apEditorID);
    }

    FakeScript::FakeScript(const char* apEditorID) : FakeForm(apEditorID, SCPT_ID, 0) {}

    FakeScript::~FakeScript() {
        for (Script::RefVariable* pRefObject : listRefObjects.ItemsUntilNull()) {
            delete pRefObject;
        }
    }

    namespace world {

        void Reset() {
            State& kWorld = World();
            kWorld.forms.clear();
            kWorld.scripts.clear();
            kWorld.fakes.clear();
            kWorld.parameters.clear();
            kWorld.uiNextFormID = 0x1000;
            Reported().Clear();

            for (int32_t i = 0; i < FUNCTION_COUNT; ++i) {
                scriptFunctions[i] = Unnamed(static_cast<SCRIPT_OUTPUT>(START_OF_FUNCTION_SECTION + i));
            }
            scriptFunctions[FUNCTION_COUNT] = Unnamed(END_OF_FUNCTION_SECTION);
            for (int32_t i = 0; i < CONSOLE_COUNT; ++i) {
                scriptConsole[i] = Unnamed(static_cast<SCRIPT_OUTPUT>(START_OF_CONSOLE_SECTION + i));
            }
            scriptConsole[CONSOLE_COUNT] = Unnamed(END_OF_CONSOLE_SECTION);
            for (int32_t i = 0; i < SCRIPT_BLOCK_COUNT; ++i) {
                scriptBlocks[i] = Unnamed(static_cast<SCRIPT_OUTPUT>(i));
            }
            for (ANIM_GROUP_INFO& kGroup : animGroupInfo) {
                kGroup = ANIM_GROUP_INFO();
                kGroup.pName = "";
            }

            FakeForm& kPlayer = AddReference("player");
            kPlayer.bIsActor = true;
            pPlayer = reinterpret_cast<PlayerCharacter*>(&kPlayer);
            pMapMarker = reinterpret_cast<TESObjectSTAT*>(&AddForm("MapMarker", STAT_ID, kObject));
        }

        FakeForm& AddForm(const char* apEditorID, ENUM_FORM_ID aeType, uint32_t auiClasses) {
            State& kWorld = World();
            FakeForm& kForm = kWorld.forms.emplace_back(apEditorID, aeType, auiClasses);
            kForm.uiFormID = kWorld.uiNextFormID++;
            kWorld.fakes.insert(&kForm);
            return kForm;
        }

        FakeForm& AddReference(const char* apEditorID) {
            return AddForm(apEditorID, REFR_ID, kReference);
        }

        FakeForm& AddGlobal(const char* apEditorID) {
            return AddForm(apEditorID, GLOB_ID);
        }

        FakeForm& AddQuest(const char* apEditorID, std::initializer_list<Variable> aVariables) {
            FakeForm& kQuestForm = AddForm(apEditorID, QUST_ID, kQuest);
            FakeScript& kScript = AddScript((std::string(apEditorID) + "Script").c_str());
            kScript.header.bIsQuestScript = true;
            kScript.variables = aVariables;
            kQuestForm.pScript = &kScript;
            return kQuestForm;
        }

        FakeScript& AddScript(const char* apEditorID, const char* apText) {
            State& kWorld = World();
            FakeScript& kScript = kWorld.scripts.emplace_back(apEditorID);
            kScript.uiFormID = kWorld.uiNextFormID++;
            kScript.text = apText;
            kWorld.fakes.insert(&kScript);
            return kScript;
        }

        SCRIPT_FUNCTION& DefineFunction(
            SCRIPT_OUTPUT aeCode,
            const char* apName,
            const char* apShortName,
            std::initializer_list<SCRIPT_PARAMETER> aParameters,
            bool abReferenceFunction
        ) {
            SCRIPT_FUNCTION* pEntry = ScriptCompiler::GetFunctionDef(aeCode);
            if (!pEntry) {
                Unsupported("a function code outside the function tables");
            }
            pEntry->pFunctionName = apName;
            pEntry->pShortName = apShortName;
            pEntry->bReferenceFunction = abReferenceFunction;
            AddParameters(*pEntry, aParameters);
            return *pEntry;
        }

        SCRIPT_FUNCTION& DefineBlock(
            SCRIPT_BLOCK aeBlock,
            const char* apName,
            std::initializer_list<SCRIPT_PARAMETER> aParameters,
            bool abReferenceFunction
        ) {
            SCRIPT_FUNCTION& kEntry = scriptBlocks[aeBlock];
            kEntry.pFunctionName = apName;
            kEntry.bReferenceFunction = abReferenceFunction;
            AddParameters(kEntry, aParameters);
            return kEntry;
        }

        void NameAnimGroup(int32_t aiGroup, const char* apName) {
            animGroupInfo[aiGroup].pName = apName;
        }

    } // namespace world

} // namespace test

// ---- dynamic_cast ----------------------------------------------------------------------------------------------------
//
// The type info of each class the compiler casts to is defined in that class's own file, which is not built here:
// these stand in for it, each with its mangled name. They hold only what std::type_info::name() reads. MagicItem's is
// the compiler's own (an inline key function), so it is told apart by its name too.

// Not in an anonymous namespace: an object of a type with internal linkage would have internal linkage too, and the
// compiler's references would not find it.
struct FakeTypeInfo {
    const void* pVirtualTable;
    const char* pName;
};

#define XFNV_FAKE_TYPE_INFO(NAME) extern FakeTypeInfo kTypeInfo_##NAME __asm__("_ZTI" #NAME); \
    FakeTypeInfo kTypeInfo_##NAME = {nullptr, #NAME};
XFNV_FAKE_TYPE_INFO(7TESForm)
XFNV_FAKE_TYPE_INFO(11TESFullName)
XFNV_FAKE_TYPE_INFO(14EffectItemList)
XFNV_FAKE_TYPE_INFO(13TESObjectREFR)
XFNV_FAKE_TYPE_INFO(8TESQuest)
XFNV_FAKE_TYPE_INFO(9SpellItem)
XFNV_FAKE_TYPE_INFO(13TESObjectBOOK)
XFNV_FAKE_TYPE_INFO(13TESObjectCELL)
XFNV_FAKE_TYPE_INFO(9TESObject)
XFNV_FAKE_TYPE_INFO(12TESActorBase)
XFNV_FAKE_TYPE_INFO(13TESWorldSpace)
XFNV_FAKE_TYPE_INFO(10TESPackage)
XFNV_FAKE_TYPE_INFO(14TESCombatStyle)
XFNV_FAKE_TYPE_INFO(13EffectSetting)
XFNV_FAKE_TYPE_INFO(11BGSListForm)
XFNV_FAKE_TYPE_INFO(11BGSMenuIcon)
XFNV_FAKE_TYPE_INFO(7BGSPerk)
XFNV_FAKE_TYPE_INFO(7BGSNote)
XFNV_FAKE_TYPE_INFO(16BGSEncounterZone)
XFNV_FAKE_TYPE_INFO(11TESIdleForm)
#undef XFNV_FAKE_TYPE_INFO

// A cast of a fake form succeeds when the form has the target class's bit; any other cast is the C++ runtime's.
extern "C" void* __dynamic_cast(
    const void* apSource,
    const void* apSourceType,
    const void* apTargetType,
    std::ptrdiff_t aiHint
) {
    if (World().fakes.contains(apSource)) {
        static constexpr struct {
            const char* pName;
            uint32_t uiClass;
        } kClasses[] = {
            {"13TESObjectREFR", test::kReference},    {"8TESQuest", test::kQuest},
            {"9SpellItem", test::kSpellItem},         {"13TESObjectBOOK", test::kBook},
            {"13TESObjectCELL", test::kCell},         {"9MagicItem", test::kMagicItem},
            {"9TESObject", test::kObject},            {"12TESActorBase", test::kActorBase},
            {"13TESWorldSpace", test::kWorldSpace},   {"10TESPackage", test::kPackage},
            {"14TESCombatStyle", test::kCombatStyle}, {"13EffectSetting", test::kEffectSetting},
            {"11BGSListForm", test::kListForm},       {"11BGSMenuIcon", test::kMenuIcon},
            {"7BGSPerk", test::kPerk},                {"7BGSNote", test::kNote},
            {"16BGSEncounterZone", test::kEncounterZone}, {"11TESIdleForm", test::kIdleForm},
        };
        const char* pTarget = static_cast<const std::type_info*>(apTargetType)->name();
        for (const auto& kClass : kClasses) {
            if (!std::strcmp(pTarget, kClass.pName)) {
                return AsFake(apSource)->uiClasses & kClass.uiClass ? const_cast<void*>(apSource) : nullptr;
            }
        }
        Unsupported("a dynamic_cast of a form to this class");
    }
    using Cast = void* (*)(const void*, const void*, const void*, std::ptrdiff_t);
    static const auto pRuntimeCast = reinterpret_cast<Cast>(dlsym(RTLD_NEXT, "__dynamic_cast"));
    return pRuntimeCast(apSource, apSourceType, apTargetType, aiHint);
}

// ---- forms -----------------------------------------------------------------------------------------------------------

ENUM_FORM_ID TESForm::GetFormType() const {
    return AsFake(this)->eType;
}

uint32_t TESForm::GetFormEditorIDLength() {
    return std::strlen(AsFake(this)->acEditorID);
}

TESForm* TESForm::GetFormByEditorID(const char* apSearchID) {
    FakeForm* pForm = FindForm(apSearchID);
    return pForm ? pForm->Form() : nullptr;
}

TESForm* TESForm::GetFormByNumericID(uint32_t aiSearchID) {
    for (FakeForm& kForm : World().forms) {
        if (kForm.uiFormID == aiSearchID) {
            return kForm.Form();
        }
    }
    return nullptr;
}

void TESObjectREFR::InitScript() {}

Script* TESObjectREFR::GetScriptPointer() {
    FakeScript* pScript = AsFake(this)->pScript;
    return pScript ? pScript->AsScript() : nullptr;
}

bool TESObjectREFR::GetRefPersists() {
    return AsFake(this)->bPersists;
}

TESContainer* TESObjectREFR::HasContainer() {
    return AsFake(this)->bHasContainer ? reinterpret_cast<TESContainer*>(this) : nullptr;
}

TESBoundObject* TESObjectREFR::GetObjectReference() {
    return reinterpret_cast<TESBoundObject*>(AsFake(this)->pBaseObject);
}

bool TESObjectCELL::GetInterior() const {
    return AsFake(this)->bInterior;
}

Script* TESScriptableForm::GetFormScript(TESForm* apForm) {
    FakeScript* pScript = AsFake(apForm)->pScript;
    return pScript ? pScript->AsScript() : nullptr;
}

TESGlobal* TESDataHandler::GetGlobal(const char* apSearchID) {
    return reinterpret_cast<TESGlobal*>(FindForm(apSearchID, GLOB_ID));
}

// TESContainer.cpp's.
bool TESContainer::ContainerCanHoldType(uint8_t aType) {
    switch (aType) {
        case 0x18:
        case 0x19:
        case 0x1A:
        case 0x1D:
        case 0x1E:
        case 0x1F:
        case 0x28:
        case 0x29:
        case 0x2E:
        case 0x2F:
        case 0x31:
        case 0x32:
        case 0x34:
        case 0x67:
        case 0x6C:
        case 0x73:
        case 0x74: return true;
        default: return false;
    }
}

// ---- scripts ---------------------------------------------------------------------------------------------------------

// TESScript.cpp's.
Script::RefVariable::RefVariable() : form(nullptr), varIdx(0) {}

void ScriptVariable::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void Script::SetIsCompiled(bool abCompiled) {
    AsFakeScript(this)->header.bIsCompiled = abCompiled;
}

SCRIPT_HEADER* Script::GetHeader() {
    return &AsFakeScript(this)->header;
}

char* Script::GetText() {
    return AsFakeScript(this)->text.data();
}

// Script.cpp's, on the fake's variables.
char Script::FindVariable(const char* varName, uint32_t* auiID) {
    for (const test::Variable& kVariable : AsFakeScript(this)->variables) {
        if (!BSstricmp(kVariable.name.c_str(), varName)) {
            *auiID = kVariable.uiID;
            return kVariable.bIsInteger ? 's' : 'f';
        }
    }
    *auiID = 0;
    return 0;
}

BSSimpleList<Script::RefVariable*>* Script::GetReferencedObjectList() {
    return &AsFakeScript(this)->listRefObjects;
}

void Script::CopyReferencedObjectList(
    BSSimpleList<RefVariable*>* pCopyFromList,
    BSSimpleList<RefVariable*>* pCopyToList,
    Script*
) {
    for (const RefVariable* pSource : pCopyFromList->ItemsUntilNull()) {
        RefVariable* pCopy = ::new (ourMalloc(sizeof(RefVariable))) RefVariable;
        pCopy->name.Set(pSource->name.String(), 0);
        pCopy->form = pSource->form;
        pCopy->varIdx = pSource->varIdx;
        pCopyToList->AddTail(pCopy);
    }
}

void Script::Init(ScriptCompileData* apCompileData) {
    FakeScript& kScript = *AsFakeScript(this);
    kScript.bInitialised = true;
    const char* pName = apCompileData->cScriptName.String();
    kScript.compiledName = pName ? pName : "";
    kScript.compiledHeader = apCompileData->header;
    const auto* pData = reinterpret_cast<const uint8_t*>(apCompileData->pOutput);
    kScript.compiled.assign(pData, pData + apCompileData->header.dataSize);
    kScript.compiledVariables.clear();
    for (const ScriptVariable* pVariable : apCompileData->listVariables.ItemsUntilNull()) {
        kScript.compiledVariables.push_back({pVariable->cName.String(), pVariable->data.uiID, pVariable->data.bIsInteger});
    }
    kScript.compiledRefObjects.clear();
    for (const RefVariable* pRefObject : apCompileData->listRefObjects.ItemsUntilNull()) {
        kScript.compiledRefObjects.emplace_back(pRefObject->name.String());
    }
}

bool Script::GetValue(double&, const char*, uint32_t&, TESObjectREFR*, TESObjectREFR*, Script*, ScriptLocals*, bool) {
    Unsupported("Script::GetValue");
}

// ---- lookups ---------------------------------------------------------------------------------------------------------

// The script names of a few actor values; ACTOR_VALUE_COUNT for any other name, as the game's.
ActorValue::Index ActorValue::GetActorValueIndex(char* acScriptName) {
    static constexpr struct {
        const char* pName;
        ActorValue::Index eIndex;
    } kValues[] = {
        {"Strength", STRENGTH}, {"Perception", PERCEPTION}, {"Endurance", ENDURANCE}, {"Charisma", CHARISMA},
        {"Intelligence", INTELLIGENCE}, {"Agility", AGILITY}, {"Luck", LUCK}, {"Health", HEALTH},
    };
    for (const auto& kValue : kValues) {
        if (!BSstricmp(kValue.pName, acScriptName)) {
            return kValue.eIndex;
        }
    }
    return ACTOR_VALUE_COUNT;
}

// MiscStats.cpp's.
int32_t GetMiscStatIDFromName(char* a1) {
    for (int32_t i = 0; i < MiscStatManager::MS_EQUIPMENT_DAMAGE_REPAIRED; ++i) {
        if (MiscStatManager::StatNames[i] && !BSstricmp(MiscStatManager::StatNames[i], a1)) {
            return i;
        }
    }
    Warning("MENUS: Miscellaneous stat %s could not be found.  Returning MS_COUNT.", a1);
    return MiscStatManager::MS_EQUIPMENT_DAMAGE_REPAIRED;
}

char* NavMeshInfo::GetCellOrCoords(char* apFirst, int32_t) {
    return apFirst;
}

// ---- reporting -------------------------------------------------------------------------------------------------------

void Interface::PrintLine(const char* apFormat, ...) {
    char acBuffer[4096];
    va_list kArgs;
    va_start(kArgs, apFormat);
    std::vsnprintf(acBuffer, sizeof(acBuffer), apFormat, kArgs);
    va_end(kArgs);
    test::Reported().console.emplace_back(acBuffer);
}

void InitializationFlow(...) {}

// ---- what ScriptVariable's loading calls, which no test does ---------------------------------------------------------

bool TESFile::QEndian() const {
    Unsupported("TESFile::QEndian");
}

CHUNK_ID TESFile::GetTESChunk() {
    Unsupported("TESFile::GetTESChunk");
}

bool TESFile::GetChunkData(void*, uint32_t) {
    Unsupported("TESFile::GetChunkData");
}
