#pragma once

#include "BSHavok/bhkWorldNS.h"
#include "BSMain/Setting.h"
#include "Fallout/AI/ProcessLists.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <stdint.h>

class ImageSpaceModifierInstanceRB;
class Script;
struct SCRIPT_LINE;
struct SCRIPT_PARAMETER;
class ScriptCompileData;
class ScriptLocals;

// The bhkWorld::RecFunction Script::SetRigidBodyMassBase runs over a reference's collision objects: sets
// the mass of apCollision's rigid body, and of its stored dynamic motion, to the float in arData.
void SetRBMass(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
// BSShaderManager's shader error callback, which Script::ReloadShaders installs: prints the error text to the
// console.
void ConsoleShaderError(const char* apErrorString, const NiAVObject* apObject);
bool Cmd_IsLoadingMenuEnabled(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsActorRefEssential(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SetActorRefEssential(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IsNoHumanGoreExe(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_IncrementScriptedChallenge(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_PersistentRefCount(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_AlwaysShowActorSubtitles(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
uint32_t GetTextureDumpSize(NiTexture* apTexture);
void ToggleProjectileDebug();
bool SetIsLocationSpecificLoadScreensOnly(bool abOnly);
bool Cmd_GetInCharGen(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
// Returns _mbsnbcpy's result (the destination) and only reads Source.
char* mbsnbcpy_(char* Destination, const char* Source, uint32_t Count);
void Set(char abDump);
bool Cmd_GetContainerInventoryCount(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);
bool Cmd_SystemSave(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
);

// The mob operator of Script::ResetDialogueFlags: for each mob, removes the mob's EXTRA_SAY_ONCE_A_DAY_TOPIC_INFO
// extra.
class MobDialogueFlagResetter : public MobIterOperator {
public:
    MobDialogueFlagResetter();

    void Operate(MobileObject* apMob) override;
};

using LifeStateString_t = char* [6];
inline LifeStateString_t LifeStateString{
    const_cast<char*>("Alive"),
    const_cast<char*>("Dying"),
    const_cast<char*>("Dead"),
    const_cast<char*>("Unconscious"),
    const_cast<char*>("Reanimate"),
    const_cast<char*>("Restrained")
};
using SleepStateString_t = char* [11];
inline SleepStateString_t SleepStateString{
    const_cast<char*>(aNormal_0),
    const_cast<char*>("Load sit idle"),
    const_cast<char*>("Want to sit"),
    const_cast<char*>("Waiting for sit anim"),
    const_cast<char*>("Sitting"),
    const_cast<char*>("Want to stand"),
    const_cast<char*>("Loading sleep idle"),
    const_cast<char*>("Want to sleep"),
    const_cast<char*>("Waiting for sleep anim"),
    const_cast<char*>("Sleeping"),
    const_cast<char*>("Want to wake")
};
using KnockStateString_t = char* [7];
inline KnockStateString_t KnockStateString{
    const_cast<char*>(aNormal_0),
    const_cast<char*>("Explode"),
    const_cast<char*>("Explode Lead In"),
    const_cast<char*>("Knocked Out"),
    const_cast<char*>("Knock Out Lead In"),
    const_cast<char*>("Queued"),
    const_cast<char*>("Getting Up")
};
inline bool bMenusOn = true;
inline char bBordersOn;
using animGroupSectionNames_t = char* [8];
inline animGroupSectionNames_t animGroupSectionNames{
    const_cast<char*>("Idle"),
    const_cast<char*>("Movement"),
    const_cast<char*>("LeftArm"),
    const_cast<char*>("LeftHand"),
    const_cast<char*>("Weapon"),
    const_cast<char*>("WeaponUp"),
    const_cast<char*>("WeaponDown"),
    const_cast<char*>("SpecialIdle")
};
using pProcessTypeString_t = char* [4];
inline pProcessTypeString_t pProcessTypeString{
    const_cast<char*>("High"),
    const_cast<char*>("Middle High"),
    const_cast<char*>("Middle Low"),
    const_cast<char*>("Low")
};
using pAnimationActionStrings_t = char* [18];
inline pAnimationActionStrings_t pAnimationActionStrings{
    const_cast<char*>("Equip Weapon"),
    const_cast<char*>("Unequip Weapon"),
    const_cast<char*>("Attack"),
    const_cast<char*>("Attack Eject"),
    const_cast<char*>("Attack Follow Through"),
    const_cast<char*>("Attack Throw"),
    const_cast<char*>("Attack Throw Attach"),
    const_cast<char*>("Block"),
    const_cast<char*>("Recoil"),
    const_cast<char*>("Reload"),
    const_cast<char*>("Stagger"),
    const_cast<char*>("Dodge"),
    const_cast<char*>("Lower Body Anim"),
    const_cast<char*>("Special Idle"),
    const_cast<char*>("Script Animation"),
    const_cast<char*>("Reload Loop Start"),
    const_cast<char*>("Reload Loop End"),
    const_cast<char*>("Reload Loop")
};
inline bool s_ToggleSky;
inline bool g_bIsToggleWireframe;
inline ImageSpaceModifierInstanceRB* g_radialBlurIMOD;
using pGSMonth_t = SettingT<GameSettingCollection>* [12];
extern pGSMonth_t pGSMonth;
inline char g_bIsUseModdedWaterShaders;
inline float g_fWaterFresnelTerm;
inline float g_fWaterDisplaceDampener;
inline bool bTestLightOn;
inline char bDumpNextFrame;
inline char g_enableWireframeDecals;
