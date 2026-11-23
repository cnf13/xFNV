#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/TESForm.h"

class Actor;
class BGSEncounterZone;
class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class BSBound;
class ExtraDataList;
struct SCRIPT_LOCAL;
struct SCRIPT_PARAMETER;
class ScriptCompileData;
class ScriptCompiler;
class ScriptLocals;
class ScriptVariable;
class TESIdleForm;
class TESObject;
class TESObjectREFR;
class TESQuest;

enum COMPILER_NAME : int32_t;

bool Cmd_GetIsCurrentWeather(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_IsWeaponSkillType(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_SameFactionAsPC(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
// Whether apRefObject and the actor apParam1 share a faction, into afReturnValue. apRefObject counts only when it is an
// actor; apParam1 is read as an actor without a test. Cmd_SameFactionAsPC passes the player.
bool Cmd_SameFaction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
// Stores 0 in afReturnValue and returns true: a condition function,
// typed by its table slot.
bool Cmd_SetArgTo0AndReturnTrue(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_SameRaceAsPC(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_SameSexAsPC(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_SameRace(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_SameSex(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_IsWeaponInList(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_GetDefaultOpen(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_GetFactionRelation(TESObjectREFR* apRefObject, Actor* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_IsLastIdlePlayed(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
void ClearPendingReferenceLists();

// Header of a script's compiled data blob.
struct SCRIPT_HEADER {
    // Byte-swaps the four counts read from a big-endian file; the flags are left alone.
    void Endian();

    // slots left empty by the compiler for variables created later
    uint32_t variableCount;
    // number of forms the compiled data references
    uint32_t refObjectCount;
    // size of the compiled data
    uint32_t dataSize;
    // the last local variable ID handed out
    uint32_t m_uiLastID;
    // the kind of script: a quest script, a magic effect script, or neither for an object script
    bool bIsQuestScript;
    bool bIsMagicEffectScript;
    // set once the source compiled cleanly
    bool bIsCompiled;
};

// A compiled script form: its source text, compiled data, referenced forms and local variables.
class Script : public TESForm {
public:
    // One form the script text references by editor name.
    struct RefVariable {
        RefVariable();
        RefVariable(const RefVariable* apSource);

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }

        // the editor name as written in the source
        BSStringT<char> name;
        // the form the name resolved to
        TESForm* form;
        // the local variable index the reference resolved to, if any
        uint32_t varIdx;
    };

    // The constructor, run over apMemory.
    Script();
    ~Script() override;

    void InitializeData() override;

    // Frees the compiled data, source text, reference list and local variables.
    void ClearData() override;

    bool Load(TESFile* pFile) override;
    void Save() override;
    bool SaveEdit(TESFile* pFile) override;
    void InitItem() override;

    using TESForm::Save;

    // Installs the editor name, compiled data, references and variables of a freshly compiled script.
    void Init(ScriptCompileData* apCompileData);

    // Initializes the reference list from the owning form.
    void InitItem(TESForm* apOwner);
    // Deletes every local variable and empties the list.
    void ClearVariableList();

    // Deletes every referenced object and empties the list; forgets the last reference lookup when it was ours.
    void ClearAllReferenceObjects();

    // Copies apScriptToCopy's header, compiled data, referenced objects, variables, text and initialized flag; the
    // unused-variable count starts over at 0.
    void CopyResultScript(Script* apScriptToCopy);

    // Clears the forms of the references that resolve through a local variable; forgets the last reference lookup
    // when it was ours.
    void ClearLocalVariableRefObjects();

    // Byte-swaps the compiled data on big-endian builds; does nothing on PC.
    void EndianData();

    // The referenced object with 1-based index aiIndex, its form refreshed from the local variable it names in
    // apScriptLocals; the last lookup is cached per thread.
    RefVariable* GetReferencedObject(uint32_t aiIndex, ScriptLocals* apScriptLocals);
    // Replaces the compiled data with auiDataSize bytes copied from apCompileData.
    void SetCompileData(uint32_t auiDataSize, char* apCompileData);

    // Replaces the source text with a copy of newText.
    void SetText(const char* newText);

    // The name of local variable auiID, or null after a warning when it does not exist.
    const char* GetVariableName(uint32_t auiID);

    // Looks a local variable up by name; the returned character is 's' or 'f'.
    char FindVariable(const char* varName, uint32_t* auiID);
    BSSimpleList<SCRIPT_LOCAL*>* CreateLocalList();
    BSSimpleList<ScriptVariable*>* GetVariableList();

    // The compiled-data header.
    SCRIPT_HEADER* GetHeader();
    // The quest script's GetSecondsPassed buffer.
    float GetQuestScriptGetSecondsBuffer();
    // The source text.
    char* GetText();
    BSSimpleList<RefVariable*>* GetReferencedObjectList();
    // Sets the header's compiled flag.
    void SetIsCompiled(bool abCompiled);
    char* GetCompiledData();
    // True when apCompileData's name, header, compiled data, variables or references differ from this script's.
    bool NeedsUpdate(ScriptCompileData* apCompileData);
    // Adds afTime to the profiler timer.
    void AddProfilerTime(float afTime);
    // The quest this script belongs to.
    void SetOwnerQuest(TESQuest* apQuest);

    // A new set of local variables for the script.
    ScriptLocals* CloneLocals();

    // Runs the script on ownerObject with its locals; a quest script waits out its delay first.
    bool
    Run(TESObjectREFR* ownerObject, ScriptLocals* ownerVars, TESObjectREFR* ownerContainer, bool abIsPartialScript);

    // Compiles the script's text as a partial script with compiler under compilerIndex and runs it once on
    // ownerObject; the system window compiler echoes condition results to the console meanwhile.
    void CompileAndRun(ScriptCompiler* compiler, COMPILER_NAME compilerIndex, TESObjectREFR* ownerObject);
    void RunScriptEffectStart(TESObjectREFR* apTargetRef, ScriptLocals* apEffectLocals);
    void RunScriptEffectFinish(TESObjectREFR* apTargetRef, ScriptLocals* apEffectLocals);
    void RunScriptEffectUpdate(TESObjectREFR* apTargetRef, ScriptLocals* apEffectLocals, float afSecondsElapsed);
    bool GetIsCompiled();
    void SaveResultScript();
    // True when apCompare's header, compiled data, text or referenced forms differ from this script's.
    bool CompareResultScripts(Script* apCompare);

    // Evaluates one compiled operand (a constant, a local variable, a referenced object or a function call) at
    // *auiDataOffset into *adValue. With abEndian set on a script not yet initialized, a function call only has its
    // parameters parsed.
    static bool GetValue(
        double& adValue,
        const char* apCompiledData,
        uint32_t& auiDataOffset,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        bool abEndian
    );

    static void GetNumericIDFromDouble(uint32_t& aiID, const double& adVal);
    // Whether apRef is waiting to be enabled.
    static bool IsPendingEnabledReference(TESObjectREFR* apRef);
    // Whether apRef is waiting to be disabled with a fade.
    static bool IsPendingFadeDisabledReference(TESObjectREFR* apRef);
    // Sets the base scale of apRef.
    static void SetScaleBase(TESObjectREFR* apRefObject, float afValue);
    // Damages apRef's object by afDamage.
    static void DamageObjectBase(TESObjectREFR* apRef, float afValue);
    // Matches the face geometry of apCharacter to apGeometry.
    static void MatchFaceGeometryBase(TESObjectREFR* apRef, TESObjectREFR* apMatchActor, int32_t aiValue);
    // The GetInZone condition.
    static bool
    GetInZoneConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    // The GetPermanentActorValue condition; apParam1 holds the ActorValue::Index.
    static bool GetPermanentActorValueConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    // The GetBaseActorValue evaluation; it passes aeActorValue on to Actor::GetBaseActorValue and
    // ActorValue::GetActorValueScriptName as their ActorValue::Index.
    static bool
    GetBaseActorValue_Eval(Actor* apActor, ActorValue::Index aeActorValue, int32_t aiArg2, double* apResult);
    // The GetAlarmed condition.
    static bool
    GetAlarmedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    // The IsWeaponOut condition.
    static bool
    IsWeaponOutConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetOpenStateConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    // The GetThreatRatio condition.
    static bool
    GetThreatRatioConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    // The GetRadiationLevel condition.
    static bool GetRadiationLevel(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    // The IsCombatTarget condition.
    static bool
    IsCombatTargetConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetWeaponAnimTypeConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetCurrentAIProcedureConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetActorCrimePlayerEnemyConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    GetActorsInHighConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    HasLoaded3DConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetFatiguePercentageConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetHealthPercentageConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    GetScaleConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetLOSConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetDisabledConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetLockedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetLockLevelConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetIsLockBrokenConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetDiseaseConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetVampireConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetClothingValueConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    // The seconds since aiParam1, a script, last ran.
    static bool GetSecondsPassedConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    // Whether apRefObject's NPC base is of race aiParam1.
    static bool
    GetIsRaceConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    // Whether the actor apRefObject is in faction aiParam1.
    static bool
    GetInFactionConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    // The armor rating of apRefObject's upper-body armor.
    static bool GetArmorRatingUpperConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    GetDetectedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetDeadConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetItemCountConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetEquippedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetGoldConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetSleepingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetSittingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetFurnitureMarkerIDConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    GetQuestRunningConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetIsPleasantConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetIsCloudyConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetIsRainingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetIsSnowingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetWeatherPercentConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    GetAttackedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetIsCreatureConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetIsPlayableRaceConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    IsInListConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetIsChildConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetIsUsedItemConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetUsedItemLevelConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetUsedItemActivateConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    GetRestrainedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetRandomPercentConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    GetLevelConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetArmorRatingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetDeadCountConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetAlertConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsFacingUpConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsLeftUpConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetKnockedStateConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetCurrentAIPackageConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    IsWaitingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsIdlePlayingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsGuardConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsTalkingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetTrespassWarningLevelConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    IsTrespassingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetAmountStolenSoldConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetGroupMemberCountConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    GetWindSpeedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsInMyOwnedCellConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool IsContinuingPackagePCNearConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    CanHaveFlamesConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    HasFlamesConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetDayOfWeekConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsPCSleepingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsPCAMurdererConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetDetectionLevelConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    IsSwimmingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetDestroyedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    HasMagicEffectConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetVATSModeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetVATSTargetHeightConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    GetSandmanConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetCannibalConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    HasBeenEatenConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetGhostConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetOffersServiceConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    GetBarterGoldConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool IsActorUsingATorchConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    IsTimePassingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsSneakingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsRunningConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetFriendHitConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsInInteriorConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsActorEvilConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsActorVictimConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetIdleDoneOnceConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetWhichServiceConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool IsActorRidingHorseConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool IsInDangerousWaterConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    IsXboxConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsPS3ConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsPCConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsEssentialConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsActorConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool IsPlayerMovingIntoNewSpaceConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    GetTimeDeadConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetLastPlayerActionConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetLastHitCriticalConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    GetCauseofDeathConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetKillingBlowLimbConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool HasFriendDispositionConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    GetIsMovingConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetAnimActionConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetGroupTargetCountConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetPlantedExplosiveConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool IsActorTalkingThroughActivatorConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetDialogueEmotionConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetDialogueEmotionValueConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    IsWaterObjectConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    HasPerkConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetPlayerTeammateConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetActorFactionPlayerEnemyConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetMinorCrimeCountConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetMajorCrimeCountConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetDestructionStageConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetIsUsedItemEquipTypeConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetAggroRadiusViolatedConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetMapMarkerVisibleConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetWeaponHealthPercConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool IsGreetingPlayerConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetXPForNextLevelConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    GetIgnoreCrimeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsGoreDisabledConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsHardcoreConditionalFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool IsAlwaysHardcoreConditionalFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    GetIsTurningConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);

    // Parses the parameters of a script function at auiOffset into the trailing arguments, one pointer per parsed
    // parameter; nothing is stored while apScript is not initialized.
    static bool ParseParameters(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        uint32_t& auiOffset,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        ...
    );

    // Expands the script variables in apText into *apOut, reading the command's own parameters.
    static bool ParseString(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double* afReturnValue,
        uint32_t* auiOffset,
        const char* apText,
        BSStringT<char>* apOut
    );
    // The ShowMessage script command: shows a message form as a message box with its buttons, or as a HUD message.
    static bool ShowMessage(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );

    // The thread's bSystemOutput.
    static bool VerboseOutput();

    // Whether condition and script functions echo their result to the console; CompileAndRun sets it.
    static inline thread_local bool bSystemOutput;
    // The victim of the crime being evaluated, set by SetCrimeVictim.
    static inline thread_local TESForm* pCrimeVictim;
    // The last GetReferencedObject lookup: its result, index, locals and script.
    static inline thread_local RefVariable* pLastRefObject;
    static inline thread_local uint32_t iLastRefSearchIndex = 0xFFFFFFFF;
    static inline thread_local ScriptLocals* pLastScriptLocals;
    static inline thread_local Script* pLastRefSearchScript;

    // Script functions of TESScriptFunctions.cpp, each taking the compiled parameters.
    static bool PrintHDRParamFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool AddScriptPackageFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // The Activate script command: the reference activates the parsed activator (else its action ref), at most 5 levels
    // deep, setting and clearing its action flags around TESObjectREFR::Activate unless the second parsed parameter is
    // set; refuses the player.
    static bool ActivateFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // Copies the class and level of the parsed reference's NPC base onto the calling actor's NPC base and has the
    // latter recompute its values.
    static bool DuplicateNPCStatsFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // Script functions that parse their parameters and hand them to the matching condition function.
    static bool GetDisposition(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetPCFactionMurderFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetPlayerEnemyofFactionFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetPCFactionAttackFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsPlayerActionActiveFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetTalkingActivatorActorFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetHasNoteFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ExistsFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // Moves apMoveRef to apTargetRef offset by (afX, afY, afZ).
    static void
    MoveToFunctionBase(TESObjectREFR* apMoveRef, TESObjectREFR* apTargetRef, float afX, float afY, float afZ);
    static bool MoveToFunction(
        SCRIPT_PARAMETER* apParameters,
        char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double* afReturnValue,
        uint32_t* auiOffset
    );
    static bool MoveToFadeFunction(
        SCRIPT_PARAMETER* apParameters,
        char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double* afReturnValue,
        uint32_t* auiOffset
    );
    static bool WaitFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ShowSPECIALBookMenu(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ShowLoveTesterMenu(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool AutoDisplayObjectives(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetMerchantContainer(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool PlaceAtMeHealthPercentFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetObjectiveDisplayedFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );

    // The script functions whose bodies are in TESScriptFunctions.cpp. Each takes the compiled parameters, the calling
    // reference and container, the script and its locals, the return value slot and the offset the parameters are
    // parsed from.
    static bool OutputArchiveProfile(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetActorAlpha(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetRigidBodyMass(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // Sets the rigid body mass of apRefObject's physics body.
    static void SetRigidBodyMassBase(TESObjectREFR* apRefObject, float afValue);
    // Prints the reference's full name and deletes it; reads only its third argument.
    static bool DeleteReference(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleCastShadows(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // Console command: prints or sets the reference's FaceGen expressions, phonemes and modifiers, rebuilds or
    // cycles its face coordinates ("coord"), or resets its morphs ("reset").
    static bool ModifyFaceGen(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsCodeRedeemedFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool RedeemCodeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // Parses one integer and
    // makes the radio dialogue-only when it is 1, else not.
    static bool SetRadioToDialogueOnlyModeFunction(
        SCRIPT_PARAMETER* apParameters,
        char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double* afReturnValue,
        uint32_t* auiOffset
    );
    // Console command: sets the falloff angles and opacities (-1 keeps one unchanged) of the no-lighting shaders on
    // the reference's 3D; "No Reference Selected" without a reference.
    static bool SetTargetFalloff(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // Console command: sets the UFO camera's speed multiplier setting.
    static bool SetUFOCamSpeedMult(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // Console command: sets the local and LOD tree mipmap biases, printing the old and the new values.
    static bool SetTreeMipmapBias(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // Console command: projects a blood or impact decal from the camera onto the reference.
    static bool AddDecal(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // Script opcodes that parse their axis or type and hand the reference to their condition function.
    static bool GetPosFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetStartingPosFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetAngleFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetStartingAngleFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetIsSex(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetPCIsSex(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetIsUsedItemType(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // Script opcode: opens and plays the music type it parses.
    static bool PlayMusicFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // Script opcode: places count copies of the parsed object at the reference; returns the last one's form ID.
    static bool PlaceAtMeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // Places aiCount copies of apObject at apPlaceAtRef, aiDistance away in aiDirection, at
    // afHealthPerc health; returns the last reference placed.
    static TESObjectREFR* PlaceAtMeFunctionBase(
        TESObjectREFR* apPlaceAtRef,
        TESObject* apObject,
        int32_t aiCount,
        int32_t aiDistance,
        int32_t aiDirection,
        float afHealthPerc
    );
    // Console command: turns the audio update thread on or off.
    static bool SetAudioMultiThreading(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // Script opcode: shows (a positive value) or hides the scripted loading menu.
    static bool EnableLoadingMenu(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // Condition functions: the reference's position, starting position, angle and starting
    // angle on the axis apParam1 ('X', 'Y' or 'Z'), and whether the used item is of type apParam1.
    static bool
    GetPosConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetAngleConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetStartingPosConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetStartingAngleConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetIsUsedItemTypeConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static void CopyReferencedObjectList(
        BSSimpleList<RefVariable*>* pCopyFromList,
        BSSimpleList<RefVariable*>* pCopyToList,
        Script* apOwner
    );
    static void CopyVariableList(
        BSSimpleList<ScriptVariable*>* pCopyFromList,
        BSSimpleList<ScriptVariable*>* pCopyToList,
        Script* apOwner
    );
    static bool IsPendingDisabledReference(TESObjectREFR* apRef);
    // Empties the four pending reference lists under ScriptRefListCrit and reloads them from apLoadGameBuffer.
    static void LoadPendingReferenceLists(BGSLoadGameBuffer* apLoadGameBuffer);
    // Forgets the per-thread last referenced-object lookup.
    static void ClearOptimizations();

    // True while the game runs its per-frame script processing pass.
    static bool GetProcessScripts();
    // Turns the per-frame script processing pass on or off.
    static void SetProcessScripts(bool abProcessScripts);
    // Console commands: PrintNPCDialog, ToggleMotionBlur on the reference's 3D, TogglePrimitives, ToggleScripts and
    // ToggleTrees. Each returns true.
    static bool PrintNPCDialog(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleMotionBlur(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool TogglePrimitivesFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleScriptsFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleTrees(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );

    // Copies the 4 bytes at apID (a form ID) into the low half of *adVal.
    static void PutNumericIDInDouble(const uint32_t& aiID, double& adVal);

    // Sets aiFlag on the action list apExtra carries for apActionForm.
    static bool SetActionFlag(TESForm* apActionForm, ExtraDataList* apExtra, uint32_t aiFlag);
    // Queues apRef to be disabled, faded out with abFade.
    static void AddPendingDisabledReference(TESObjectREFR* apRef, bool abFade);

    // Gives apOwnerObject's script a start action for the list apExtra carries.
    static void InitActionList(TESObjectREFR* ownerObject, ExtraDataList* apExtra);

    // Script opcode: the menu mode state parsed from the parameters.
    static bool MenuModeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );

    // Console command: shows or hides the grass and reports the new state.
    static bool ToggleGrassFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );

    // Console command: prints the float it parses.
    static bool ShowFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );

    // Script opcodes that hand the reference to their condition function (IsTrespassing: true without one).
    static bool IsTrespassingFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetAmountStolenSoldFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetGroupMemberCountFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetPlayerTeammateCountFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );

    // Script opcode: sets the reference's critical stage when it is an actor.
    static bool SetCriticalStageFunction(
        SCRIPT_PARAMETER* apParameters,
        char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double* afReturnValue,
        uint32_t* auiOffset
    );

    // Script opcode: true outside of menus and the pause fade.
    static bool GameModeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );

    // Script opcode: 1 while the current script effect has started.
    static char ScriptEffectStart(
        SCRIPT_PARAMETER* apParameters,
        char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double* afReturnValue,
        uint32_t* auiOffset
    );

    // Script opcode: sets the world and first-person fields of view (at most 160; 0 means 75, and a negative value
    // v means 1 / -v) on the scene graph, the shader manager, the player and the default FOV settings.
    static bool SetCameraFOV(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );

    // Event helper: sets *afReturnValue to 1 when apLocals holds aiFlag for apActionForm.
    static void CheckFlagFunction(
        uint32_t aiFlag,
        TESObjectREFR* apRefObject,
        TESForm* apActionForm,
        ScriptLocals* apScriptLocals,
        double& afReturnValue
    );

    // Condition function: whether the player carries the note apParam1.
    static bool
    GetHasNoteConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);

    // Condition function: the hit location (damage limb) of apRefObject's last hit.
    static bool
    GetHitLocationConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);

    // Condition function: whether menu apParam1 is up (the top menu under the old rule), or any menu when it is 0.
    static bool
    GetMenuModeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);

    // Condition function: whether the reference apParam2 (else apRefObject) is in a cell whose editor ID starts with
    // that of cell apParam1; the last answer is cached.
    static bool
    GetInCellConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);

    // Condition function: apRefObject's distance to the reference apParam1.
    static bool
    GetDistanceConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);

    // Condition function: actor value apParam1 of the actor apRefObject (its base's while it is disabled).
    static bool
    GetActorValueConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);

    // Condition function: whether the actor apRefObject (else the reference apParam1) has spoken to the player.
    static bool
    GetTalkedToPCConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);

    // Condition function: 100 when the actor apRefObject should attack the actor apParam1, else 0.
    static bool
    GetShouldAttackConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);

    // Condition function: whether apRefObject's NPC base form has sex apParam1.
    static bool
    GetIsSexConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);

    // Condition function: whether apRefObject's base form (its original leveled base first) is apParam1; the last
    // lookup is cached.
    static bool
    GetIsIDConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);

    // Condition function: whether the actor apRefObject is in combat.
    static bool
    IsInCombatConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);

    // Condition function: the walking speed of the actor apRefObject.
    static bool
    GetWalkSpeedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);

    // Script opcode: plays the parsed sound form in 2D, as a system sound when the second parameter is positive.
    static bool PlaySoundFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );

    // Script opcode: prints the memory of the loaded area's scene data in MB.
    static bool TestCode(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );

    // Script opcodes: quit the game; purge the buffered cells; set Actor::bAllPointsVisible and
    // bhkAvoidBox::bNoAvoidance from the parsed integer; set the particle limit of emitters (below 0x10000).
    static bool QuitGame(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool PurgeCellBuffers(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetAllVisible(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetNoAvoidance(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetEmitterParticleMax(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleWaterSystem(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetSceneIsComplex(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleDecalRendering(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ShowTutorialMenu(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // Script opcodes: flip the grab IK (of apRefObject's ragdoll, else the global setting), flip the ragdoll
    // animation setting, and set apRefObject's scale (queued on a multi-threaded machine).
    static bool ToggleGrabIK(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // Script opcode: flips the look IK of apRefObject's ragdoll, else the global setting.
    static bool ToggleLookIK(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleRagdollAnim(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetScaleFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );

    // Script opcode: sets the global time multiplier and whether the audio ignores it.
    static bool SetGlobalTimeMultiplierFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );

    // Script opcode: plays the parsed sound form in 3D at apRefObject, following its 3D.
    static bool PlaySound3DFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );

    // Script opcode: the walking speed of the actor apRefObject.
    static bool GetWalkSpeedFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );

    // Script opcode: queues the hacking menu for the terminal apRefObject (no reference when it is not one).
    static bool ShowHackingMiniGame(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );

    // Script opcode: starts the parsed number of screen blood splatters.
    static bool TriggerScreenBlood(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );

    // Script opcode: closes the console and queues the player name entry menu.
    static bool GetPlayerName(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );

    // Clears the values cached for the condition functions between checks.
    static void ClearCachedValues();
    // Enables, disables, fade-disables and deletes the references queued for it.
    static void RunDelayedScriptActionsOnReferences();
    static bool ToggleShadowVolumesFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static void ClearButtonPressed();
    static void HandleMessageMenu();
    static bool ToggleFullScreenMotionBlur(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool
    IsPC1stPersonConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetButtonPressedFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleCharControllerShapeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SkipAnimFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetTalkedToPC(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ForceRadioStationUpdateFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetNoRumors(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool RevertWorld(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleMaterialGeometryFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleRefractionDebug(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool TriggerHUDShudder(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool DumpMenuPackingInfo(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetWeaponAnimType(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetCurrentAIProcedureFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleWaterRadius(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool Reset3DStateFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool DebugCombatPlan(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ClearActorsFactionsPlayerEnemyFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetActorCrimePlayerEnemyFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetActorsInHighFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ShowRenderPasses(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool TriggerLODApocalypseFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool
    GetCurrentTimeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool IsCurrentFurnitureRefConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    GetIsRefConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetUnconsciousConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    // The angle (degrees, -180 to 180) from the mobile reference's heading to the direction of the reference apParam1.
    static bool
    GetHeadingAngleConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetTalkingActivatorActorConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetPlayerTeammateCountConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool IsPlayerGrabbedRefConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetVATSRightAreaFreeConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetVATSLeftAreaFreeConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetVATSBackAreaFreeConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetVATSFrontAreaFreeConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetVATSRightTargetVisibleConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetVATSLeftTargetVisibleConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetVATSBackTargetVisibleConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetVATSFrontTargetVisibleConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    ChallengeLockedConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetChallengeCompletedConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool ToggleVerboseFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleDemigodModeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleSafeZoneFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetAllReachable(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool HasLoaded3DFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetKillingBlowLimbFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool HasFriendDispositionFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetScaleFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ScriptEffectUpdateFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ScriptEffectElapsedTimeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetDisease(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetVampire(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetClothingValue(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetDead(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetGold(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetSleeping(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetSitting(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetFurnitureMarkerID(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetAlarmed(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetIsPleasant(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetIsRaining(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetIsSnowing(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetWindSpeedFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetIsCreature(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetIsChild(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetUsedItemLevelFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetUsedItemActivateFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetRandomPercent(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetLevel(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetArmorRating(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsGuard(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsTalkingFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetTrespassWarningLevelFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsInMyOwnedCellFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsContinuingPackagePCNearFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool CanHaveFlamesFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool HasFlamesFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetOpenStateFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetDayOfWeekFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsPCSleepingFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsPCAMurdererFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsSwimming(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetPlayerSleepHours(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetDestroyedFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsActorEvilFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsActorVictimFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetIdleDoneOnceFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetNoRumorsFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetWhichServiceFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsActorRidingHorseFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsInDangerousWaterFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetLastPlayerActionFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetBroadcastState(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetBroadcastState(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsActorTalkingThroughActivatorFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool RemoveMerchantContainer(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ClearLocationMarkersFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetDialogueEmotionFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetDialogueEmotionValueFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsWaterObjectFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetActorFactionPlayerEnemyFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetPlayerGrabbedRefFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetDestructionStageFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetRadiationLevelFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetMapMarkerVisibleFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetWeaponHealthPercFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ClearNoActivationSound(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsGreetingPlayerFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetIgnoreCrimeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ResetPipboyManagerFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ForceTerminalBackFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool RadioShutOffFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetCurrentTimeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetUnconsciousFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool DumpNiUpdates(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static void RemoveDelayedScriptActionReference(TESObjectREFR* apRef);
    static bool RefreshINI(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetOpenState(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ResetInventoryFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool AddFlamesFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool PlayBinkFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool DamageActorValueFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool StopCombatAlarmOnActorFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool RefreshShaders(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetArmorRatingUpperFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool FreezeRendererAccumulation(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleBorders(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static void SetAngleBase(TESObjectREFR* apRefObject, char acAxis, float afValue);
    static bool GetGroupTargetCountFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static void AddPendingEnabledReference(TESObjectREFR* apRef);
    static bool ReloadCurrentClimate(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ForceCloseFile(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetSecondsPassedFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool StopCombatFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool RemoveFlamesFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool RevokeCodeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetIsCurrentPackageConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static void SavePendingReferences(BGSSaveGameBuffer* apBuffer);
    static bool ToggleOcclusion(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetActorValueInfoFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetPosFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static void SetPosBase(TESObjectREFR* apRefObject, char acAxis, float afValue);
    static bool ShowVarsFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool StartConversation(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool CompleteAllQuestStages(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetObjectiveCompletedFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetMPSParticleMax(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsCurrentFurnitureObjConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    GetStageConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetStageDoneConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetFactionRankDifferenceConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    GetInSameCellConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetIsVoiceTypeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetFactionRankConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetGlobalValueConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetDispositionConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetCrimeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    IsSpellTargetConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetSpellUsageNumberConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool IsPlayerActionActiveConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    IsLimbGoneConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetKillerConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool
    GetKillerObjectConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetFactionCombatReactionConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool
    GetIsObjectTypeConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
    static bool GetObjectiveDisplayedConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool IsPlayerTagSkillConditionFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetCasinoWinningsLevelConditionalFunction(
        TESObjectREFR* apRefObject,
        void* apParam1,
        void* apParam2,
        double& afReturnValue
    );
    static bool GetDistanceFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetInZoneFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool AddItemFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool RemoveItemFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleDialogSubtitle(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // Deliberately keeps the game's uninitialised read when ParseParameters fails (see the body).
    static bool OutputMemStats(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool OutputMemStatsFullWalk(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleMapMarkersFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleGodModeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleEmotionsFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool RemoveImageSpaceModifier(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool TestLocalMap(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleMagicStatsFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SaveWorldFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleMenus(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleLeavesFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleDetectionStats(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetTargetRefraction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetMotionBlur(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetVelFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ShowViewerStrings(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleLODLand(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetLODObjectDistanceFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool HighlightBodyPart(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool HighlightAdditionalReference(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool DeactivateAllHighlights(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool AddOcclusionPlane(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetGlobalRadialBlur(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleAI(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleFlyCam(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleNavMesh(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool PickRefByIDFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetActorValueFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetPermanentActorValueFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetActorValueFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ModActorValueFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool RestoreActorValueFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ForceActorValueFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsLimbGoneFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetLightingPassesFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetFogDepthFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetImageSpaceGlowParamFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetHUDGlowConstants(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool HairTint(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool RotateFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetAngleFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool PlayGroupFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool LoopGroupFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsAnimPlayingFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool StartCombatFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool AddSpell_Execute(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool StreamMusicFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // The PrintLine call in the body reads arguments that are never passed.
    static bool CalcLowPathToPointFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ShowQuestVarsFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ShowAnim(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool EnableFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool DisableFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SaveGame_Execute(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool CopySaves(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool DumpModelMap(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SexChange(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsCurrentFurnitureRef(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsCurrentFurnitureObj(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetTalkedToPCParam(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetStage(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetIsCurrentWeatherFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetLockLevelFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetIsVoiceTypeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SameFaction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SameRace(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SameSex(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SameRaceAsPC(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SameSexAsPC(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetQuestCompleted(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetStage(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetStageDone(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetFactionRankDifference(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetInFaction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetPCIsClass(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetPCIsRace(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetGlobalValue(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetShouldAttack(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetInCellParam(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetInCell(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetPCInFaction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetIsID(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsInListFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetIsUsedItem(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetIsRef(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetFactionRank(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetFactionRank(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ModDisposition(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetDeadCount(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ShowMap(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SayFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool PlayVATSCameras(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ShowQuestLog(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool LockFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool UnlockFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetCrimeKnownFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetCrimeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool MoveToQuestTargetFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool WakeUpPCFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetCombatStyleFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SelectPlayerSpellFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetEssentialFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetQuestObjectFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetDetectionLevelFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool StopWaitingFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetPlayerSleepHours(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ModAmountStolenSoldFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetPCExpelledFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetPCExpelledFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetDestroyedFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetSecuritronExpression(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SwapTextureOnRef(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetUnconsciousFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetRestrainedFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetCanPCFastTravelFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ModPCMiscStat(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetPCMiscStat(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetNoRumorsFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool TriggerHitShaderFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool AddAchievement(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool FlushPersistantActors(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleFogOfWar(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool OutputLocalMapPictures(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool WasteMemory(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SendCrimeAlarmFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ForceGroupStrategy(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool PushActorAwayFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetObjectiveCompletedFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetObjectiveDisplayedFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool PipboyRadio(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetNPCRadio(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool StartRadioConversation(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool PlaceLocationMarkerFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetIsObjectTypeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool MarkForDelete(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetPlayerTagSkillFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsPlayerTagSkillFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsPlayerGrabbedRefFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool PlaceLeveledActorAtMeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ForceActiveQuestFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool MatchFaceGeometryFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetIsUsedItemEquipTypeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool AddItemHealthPercentFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SendAssaultAlarmFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetThreatRatioFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool MatchRaceFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetPCYoungFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetPCToddlerFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetConcussedFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetVATSTargetFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetZoneRespawnsFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool CreateDetectionEventFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IgnoreCrimeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsCombatTargetFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    // The body deliberately keeps the uninitialised read on stairs.
    static bool AttachAshPileFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetMinimalUseFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetPCCanUsePowerArmorFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetReputationThreshold(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetLastExtDoorActivated(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetTaskThreadSleepFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsImageSpaceActive(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ResetXP_Execute(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool PlaceAtReticle(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ShowSleepWaitMenu(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static void SetCrimeVictim(TESForm* apform);
    static bool SaveIniFiles(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ReloadCurrentWeather(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool TestSeenData(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleFullHelp(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleSkyFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleCollisionFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleWireframeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleFootIK(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleDebugTextFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ShowScenegraphFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleEOFImageSpace(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleAiDetection(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleActorAI(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsActorsAIOff(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleAISchedules(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool TogglePathingInfoFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleGrassUpdateFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool SetAtStartFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool PlayExplosionFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ScriptEffectFinishFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetIsCloudy(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetAttacked(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool RemoveScriptPackageFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetActionRefFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ResetFallDamageTimerFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsPlayersLastRiddenHorseFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ClearAdaptedLight(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleVATSLight(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ResetDialogueFlags(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ToggleDepthBiasFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool DisableAllMinesFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool KillQuestUpdates(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetHeadingAngleFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsWeaponOutFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetDefaultOpenFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsTimePassingFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetCellOwnershipFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsSneakingFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsInInteriorFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetKillerFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetKillerObjectFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetFactionCombatReactionFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetFactionRelationFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool IsLastIdlePlayedFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetIsAlignmentFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetVATSRightAreaFreeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetVATSLeftAreaFreeFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetReputationFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetReputationPctFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool ChallengeLockedFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );
    static bool GetChallengeCompletedFunction(
        const SCRIPT_PARAMETER* apParameters,
        const char* apCompiledParams,
        TESObjectREFR* apRefObject,
        TESObjectREFR* apContainer,
        Script* apScript,
        ScriptLocals* apScriptLocals,
        double& afReturnValue,
        uint32_t& auiOffset
    );

    SCRIPT_HEADER m_header;
    // The script's source text.
    char* m_text;
    // the compiled data
    char* m_data;
    float fProfilerTimer;
    float fQuestScriptDelay;
    float fQuestScriptGetSecondsBuffer;
    TESQuest* pOwnerQuest;
    // the forms the compiled data references
    BSSimpleList<RefVariable*> listRefObjects;
    // the script's local variables
    BSSimpleList<ScriptVariable*> listVariables;

    static BSCriticalSection ScriptRefListCrit;
    static BSSimpleList<TESObjectREFR*> ReferencesToDisable;
    static BSSimpleList<TESObjectREFR*> ReferencesToEnable;
    static BSSimpleList<TESObjectREFR*> ReferencesToDelete;
    static BSSimpleList<TESObjectREFR*> ReferencesToFadeDisable;
    // The button the player picked in the last message box; -1 for none.
    static inline char m_cButtonValue = -1;
    // The owner of the message box whose button is being waited for (0 for none).
    static inline int32_t m_iButtonOwner;
};

// Whether the pathing info is shown (what Script::TogglePathingInfoFunction flips).
inline bool g_bIsShowPathingInfo;

inline bool bIsMagicStats;
inline bool bIsConversationStats;
inline bool bIsProjectileDebug;

// Set while the game runs its per-frame script processing pass.
inline bool bIsScriptProcessing = true;
