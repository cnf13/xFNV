#include "Fallout Shared/TESAnimGroup.h"

#include "BSAudio/BSWin32Audio.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout/AI/CameraShake.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKeyExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "SDK/MSVC/string.h"

#include <cstdint>

AnimGroupInfoArray animGroupInfo{
    {"Idle", false, AGS_IDLE, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"DynamicIdle", false, AGS_IDLE, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"SpecialIdle",
     false,
     AGS_SPECIAL_IDLE,
     AGAT_SPECIAL_IDLE_LOOP,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"Forward", false, AGS_MOVEMENT, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"Backward", false, AGS_MOVEMENT, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"Left", false, AGS_MOVEMENT, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"Right", false, AGS_MOVEMENT, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"FastForward", false, AGS_MOVEMENT, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"FastBackward", false, AGS_MOVEMENT, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"FastLeft", false, AGS_MOVEMENT, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"FastRight", false, AGS_MOVEMENT, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"DodgeForward", false, AGS_MOVEMENT, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"DodgeBack", false, AGS_MOVEMENT, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"DodgeLeft", false, AGS_MOVEMENT, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"DodgeRight", false, AGS_MOVEMENT, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"TurnLeft", false, AGS_MOVEMENT, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"TurnRight", false, AGS_MOVEMENT, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"Aim", false, AGS_WEAPON, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"AimUp", false, AGS_WEAPON_UP, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"AimDown", false, AGS_WEAPON_DOWN, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"AimIS", false, AGS_WEAPON, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"AimISUp", false, AGS_WEAPON_UP, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"AimISDown", false, AGS_WEAPON_DOWN, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"Holster", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"Equip", false, AGS_WEAPON, AGAT_EQUIP, {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_NONE}},
    {"Unequip", false, AGS_WEAPON, AGAT_UNEQUIP, {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_NONE}},
    {"AttackLeft",
     true,
     AGS_WEAPON,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackLeftUp",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackLeftDown",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackLeftIS",
     true,
     AGS_WEAPON,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackLeftISUp",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackLeftISDown",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackRight",
     true,
     AGS_WEAPON,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackRightUp",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackRightDown",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackRightIS",
     true,
     AGS_WEAPON,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackRightISUp",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackRightISDown",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack3",
     true,
     AGS_WEAPON,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack3Up",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack3Down",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack3IS",
     true,
     AGS_WEAPON,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack3ISUp",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack3ISDown",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack4",
     true,
     AGS_WEAPON,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack4Up",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack4Down",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack4IS",
     true,
     AGS_WEAPON,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack4ISUp",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack4ISDown",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack5",
     true,
     AGS_WEAPON,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack5Up",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack5Down",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack5IS",
     true,
     AGS_WEAPON,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack5ISUp",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack5ISDown",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack6",
     true,
     AGS_WEAPON,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack6Up",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack6Down",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack6IS",
     true,
     AGS_WEAPON,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack6ISUp",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack6ISDown",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack7",
     true,
     AGS_WEAPON,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack7Up",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack7Down",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack7IS",
     true,
     AGS_WEAPON,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack7ISUp",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack7ISDown",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack8",
     true,
     AGS_WEAPON,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack8Up",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack8Down",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack8IS",
     true,
     AGS_WEAPON,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack8ISUp",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack8ISDown",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackLoop", false, AGS_WEAPON, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"AttackLoopUp", false, AGS_WEAPON_UP, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"AttackLoopDown", false, AGS_WEAPON_DOWN, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"AttackLoopIS", false, AGS_WEAPON, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"AttackLoopISUp", false, AGS_WEAPON_UP, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"AttackLoopISDown", false, AGS_WEAPON_DOWN, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"AttackSpin",
     true,
     AGS_WEAPON,
     AGAT_ATTACK_SPIN,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"AttackSpinUp",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK_SPIN,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"AttackSpinDown",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_SPIN,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"AttackSpinIS",
     true,
     AGS_WEAPON,
     AGAT_ATTACK_SPIN,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"AttackSpinISUp",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK_SPIN,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"AttackSpinISDown",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_SPIN,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"AttackSpin2",
     true,
     AGS_WEAPON,
     AGAT_ATTACK_SPIN,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"AttackSpin2Up",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK_SPIN,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"AttackSpin2Down",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_SPIN,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"AttackSpin2IS",
     true,
     AGS_WEAPON,
     AGAT_ATTACK_SPIN,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"AttackSpin2ISUp",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK_SPIN,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"AttackSpin2ISDown",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_SPIN,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"AttackPower", false, AGS_WEAPON, AGAT_ATTACK_POWER, {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_NONE}},
    {"AttackForwardPower",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_POWER,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_NONE}},
    {"AttackBackPower",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_POWER,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_NONE}},
    {"AttackLeftPower",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_POWER,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_NONE}},
    {"AttackRightPower",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_POWER,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_NONE}},
    {"AttackCustom1Power",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_POWER,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_NONE}},
    {"AttackCustom2Power",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_POWER,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_NONE}},
    {"AttackCustom3Power",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_POWER,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_NONE}},
    {"AttackCustom4Power",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_POWER,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_NONE}},
    {"AttackCustom5Power",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_POWER,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_NONE}},
    {"PlaceMine",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_MINE,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"PlaceMineUp",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_MINE,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"PlaceMineDown",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_MINE,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"PlaceMineIS",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_MINE,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"PlaceMineISUp",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_MINE,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"PlaceMineISDown",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_MINE,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"PlaceMine2",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_MINE,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"PlaceMine2Up",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_MINE,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"PlaceMine2Down",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_MINE,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"PlaceMine2IS",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_MINE,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"PlaceMine2ISUp",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_MINE,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"PlaceMine2ISDown",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_MINE,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_NONE}},
    {"AttackThrow",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrowUp",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrowDown",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrowIS",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrowISUp",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrowISDown",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow2",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow2Up",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow2Down",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow2IS",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow2ISUp",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow2ISDown",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow3",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow3Up",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow3Down",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow3IS",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow3ISUp",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow3ISDown",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow4",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow4Up",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow4Down",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow4IS",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow4ISUp",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow4ISDown",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow5",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow5Up",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow5Down",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow5IS",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow5ISUp",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow5ISDown",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack9",
     true,
     AGS_WEAPON,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack9Up",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack9Down",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack9IS",
     true,
     AGS_WEAPON,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack9ISUp",
     true,
     AGS_WEAPON_UP,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Attack9ISDown",
     true,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow6",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow6Up",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow6Down",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow6IS",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow6ISUp",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow6ISDown",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow7",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow7Up",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow7Down",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow7IS",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow7ISUp",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow7ISDown",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow8",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow8Up",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow8Down",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow8IS",
     false,
     AGS_WEAPON,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow8ISUp",
     false,
     AGS_WEAPON_UP,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"AttackThrow8ISDown",
     false,
     AGS_WEAPON_DOWN,
     AGAT_ATTACK_THROW,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"Counter",
     true,
     AGS_WEAPON,
     AGAT_ATTACK_POWER,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"stomp",
     true,
     AGS_WEAPON,
     AGAT_ATTACK_POWER,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_SPECIAL_IDLE_STOP, AGA_ATTACK_STOP}},
    {"BlockIdle", false, AGS_LEFT_ARM, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"BlockHit", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"Recoil", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadWStart", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadXStart", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadYStart", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadZStart", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadA", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadB", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadC", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadD", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadE", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadF", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadG", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadH", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadI", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadJ", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadK", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadL", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadM", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadN", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadO", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadP", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadQ", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadR", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadS", false, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"ReloadW", false, AGS_WEAPON, AGAT_LOOPING_RELOAD, {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_NONE}},
    {"ReloadX", false, AGS_WEAPON, AGAT_LOOPING_RELOAD, {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_NONE}},
    {"ReloadY", false, AGS_WEAPON, AGAT_LOOPING_RELOAD, {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_NONE}},
    {"ReloadZ", false, AGS_WEAPON, AGAT_LOOPING_RELOAD, {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_NONE}},
    {"JamA", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamB", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamC", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamD", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamE", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamF", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamG", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamH", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamI", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamJ", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamK", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamL", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamM", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamN", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamO", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamP", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamQ", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamR", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamS", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamW", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamX", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamY", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JamZ", true, AGS_WEAPON, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"Stagger", true, AGS_MOVEMENT, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"Death", true, AGS_WHOLE_BODY, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"Talking", false, AGS_TALKING, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"PipBoy", false, AGS_LEFT_ARM, AGAT_ATTACK_POWER, {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_NONE}},
    {"JumpStart", false, AGS_MOVEMENT, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JumpLoop", false, AGS_MOVEMENT, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JumpLand", false, AGS_MOVEMENT, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"HandGrip1", false, AGS_LEFT_ARM, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"HandGrip2", false, AGS_LEFT_ARM, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"HandGrip3", false, AGS_LEFT_ARM, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"HandGrip4", false, AGS_LEFT_ARM, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"HandGrip5", false, AGS_LEFT_ARM, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"HandGrip6", false, AGS_LEFT_ARM, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JumpLoopForward", false, AGS_MOVEMENT, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JumpLoopBackward", false, AGS_MOVEMENT, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JumpLoopLeft", false, AGS_MOVEMENT, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JumpLoopRight", false, AGS_MOVEMENT, AGAT_LOOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"PipBoyChild",
     false,
     AGS_LEFT_ARM,
     AGAT_ATTACK_POWER,
     {AGA_START, AGA_STOP, AGA_SPECIAL_IDLE_LOOP_STOP, AGA_NONE}},
    {"JumpLandForward", false, AGS_MOVEMENT, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JumpLandBackward", false, AGS_MOVEMENT, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JumpLandLeft", false, AGS_MOVEMENT, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}},
    {"JumpLandRight", false, AGS_MOVEMENT, AGAT_START_STOP, {AGA_START, AGA_STOP, AGA_NONE}}
};

char* GetNextLineInString(char* Str) {
    char* pcLine;
    for (pcLine = std::strchr(Str, 10); pcLine && *pcLine && (*pcLine == 13 || *pcLine == 10); ++pcLine) {
        ;
    }
    if (pcLine && *pcLine) {
        return pcLine;
    }

    return nullptr;
}

void GetMoveHandAndPowerArmorTypeFromAnimName(
    const char* apPath,
    ANIM_GROUP_MOVE_ENUM* aeMove,
    ANIM_GROUP_WEAPON_ENUM* aeWeapon,
    bool* abIsPowerArmor
) {
    const char* plastSlash = std::strrchr(apPath, '\\');
    *abIsPowerArmor = false;
    *aeMove = AGM_NONE;
    *aeWeapon = AGW_NONE;
    if (plastSlash) {
        const char* pcName = plastSlash + 1;
        if (!_strnicmp(pcName, "PA", BSstrlen("PA"))) {
            *abIsPowerArmor = true;
            pcName += BSstrlen("PA");
        }
        for (uint32_t n = 1; n < 4; ++n) {
            if (!_strnicmp(pcName, animMoveInfo[n], BSstrlen(animMoveInfo[n]))) {
                *aeMove = static_cast<ANIM_GROUP_MOVE_ENUM>(n);
                pcName += BSstrlen(animMoveInfo[n]);
                break;
            }
        }
        for (uint32_t n = 1; n < 12; ++n) {
            if (!_strnicmp(pcName, animWeaponInfo[n], BSstrlen(animWeaponInfo[n]))) {
                *aeWeapon = static_cast<ANIM_GROUP_WEAPON_ENUM>(n);
                return;
            }
        }
    }
}

TESAnimGroup::TESAnimGroup(uint16_t asType)
  : times(nullptr), speed(NiPoint3::ZERO), cMorphKey(0), cBlendFrames(0), cBlendInFrames(0), cBlendOutFrames(0),
    cDecal(0), pParentName(nullptr), soundCount(0), sounds(nullptr) {
    SetType_(static_cast<ANIM_GROUP_ENUM>(asType));
}

TESAnimGroup::~TESAnimGroup() {
    ourFree(times);
    times = nullptr;
    ourFree(sounds);
    sounds = nullptr;
    soundCount = 0;
}

// The time of the group's key for groupAction; 0 with a warning when the action is out of range or the time is NaN,
// and 0 when the group has no times. The upper bound is an unsigned compare.
float TESAnimGroup::GetTime(ANIM_GROUP_ACTION groupAction) {
    if (groupAction < 0) {
        Warning(
            "ANIMATION: Invalid anim group action (action %d too small) in %s (%04X).",
            groupAction,
            animGroupInfo[GetType(sType)].pName,
            sType
        );
        return 0.0f;
    }
    if (static_cast<uint32_t>(groupAction) >= frameCount) {
        Warning(
            "ANIMATION: Invalid anim group action (action %d too big %d max) in %s (%04x).",
            groupAction,
            frameCount,
            animGroupInfo[GetType(sType)].pName,
            sType
        );
        return 0.0f;
    }
    if (!times) {
        return 0.0f;
    }
    if (std::isnan(times[groupAction])) {
        Warning("ANIMATION: Time %d in group %04X is QNAN", groupAction, sType);
        return 0.0f;
    }
    return times[groupAction];
}

ANIM_GROUP_ENUM TESAnimGroup::GetType() const {
    return GetType(sType);
}

ANIM_GROUP_MOVE_ENUM TESAnimGroup::GetMove() {
    return GetMove(sType);
}

ANIM_GROUP_WEAPON_ENUM TESAnimGroup::GetWeapon() {
    return GetWeapon(sType);
}

bool TESAnimGroup::IsPowerAttack() {
    int32_t iType = GetType(sType);
    return iType >= ANIM_GROUP_ATTACK_POWER_START && iType <= ANIM_GROUP_ATTACK_CUSTOM_5_POWER;
}

bool TESAnimGroup::IsJumpingLoopAnim() {
    int32_t iType = GetType(sType);
    return iType == ANIM_GROUP_JUMP_LOOP ||
        (iType >= ANIM_GROUP_JUMP_LOOP_FORWARD && iType <= ANIM_GROUP_JUMP_LOOP_RIGHT);
}

bool TESAnimGroup::SetTime(uint32_t aiIndex, float afValue) {
    if (!times) {
        return false;
    }
    if (aiIndex >= frameCount) {
        return false;
    }
    times[aiIndex] = afValue;
    return true;
}

void TESAnimGroup::SetSpeed(NiPoint3* apSpeed) {
    speed = *apSpeed;
}

float TESAnimGroup::GetSpeed() {
    return speed.Length();
}

NiPoint3 TESAnimGroup::GetDeltaPerSecond() {
    return speed;
}

void TESAnimGroup::SetDecal(uint8_t acDecal) {
    cDecal = acDecal;
}

bool TESAnimGroup::IsAimAnim() {
    return TESAnimGroup::IsAimAction(sType);
}

void TESAnimGroup::SetBlend(uint8_t acBlendFrames) {
    cBlendFrames = acBlendFrames;
}

void TESAnimGroup::SetBlendOutFrames(uint8_t acBlendFrames) {
    cBlendOutFrames = acBlendFrames;
}

void TESAnimGroup::SetSoundTime(uint32_t aiIndex, float afValue) {
    if (aiIndex < soundCount && sounds) {
        sounds[aiIndex].fTime = afValue;
    }
}

void TESAnimGroup::SetSoundObject(uint32_t aiIndex, TESSound* apSound) {
    if (aiIndex < soundCount && sounds) {
        sounds[aiIndex].pSound = apSound;
    }
}

void TESAnimGroup::SetSoundVolume(uint32_t aiIndex, float afValue) {
    if (aiIndex < soundCount && sounds) {
        sounds[aiIndex].cVolume = static_cast<uint8_t>(static_cast<int32_t>(afValue * 250.0));
    }
}

void TESAnimGroup::SetSoundPitch(uint32_t aiIndex, float afValue) {
    if (aiIndex < soundCount && sounds) {
        sounds[aiIndex].fPitch = afValue;
    }
}

GROUP_SOUND* TESAnimGroup::GetSound(uint32_t index) {
    if (index >= soundCount) {
        return nullptr;
    }

    return &sounds[index];
}

void TESAnimGroup::PlaySounds(
    TESObjectREFR* apRef,
    float afStartTime,
    float afStopTime,
    BSAnimGroupSequence* apSequence
) {
    uint32_t inumKeys;
    float fSpeedMult = 1.0f;
    Animation* pAnimation = apRef->GetAnimation();
    bool bWrapped = afStartTime > static_cast<double>(afStopTime);
    for (uint32_t n = 0; n < soundCount; ++n) {
        GROUP_SOUND* pgroupSound = GetSound(n);
        float fTime = pgroupSound->fTime;
        char cPlay = 0;
        if (bWrapped && (afStartTime < static_cast<double>(fTime) || afStopTime >= static_cast<double>(fTime))) {
            cPlay = 1;
        }
        else if (afStartTime < static_cast<double>(fTime) && afStopTime >= static_cast<double>(fTime)) {
            cPlay = 1;
        }
        if (cPlay) {
            if (pgroupSound->pSound) {
                bool ab2DSound = (pgroupSound->pSound->GetData().iFlags & TESSound::FLAG_2D) != 0;
                uint32_t iloopingFlags = TESSound::ConvertLoopFlag(pgroupSound->pSound->GetData().iFlags);
                NiPoint3 loc = apRef->GetLocationOnReference();
                if (iloopingFlags || FalloutAudio::SoundDistanceTest(pgroupSound->pSound, loc, 1.0f)) {
                    BSGameSound::SoundFlags ePersonFlag;
                    if (pAnimation && GetType() >= ANIM_GROUP_RELOAD_START && GetType() <= ANIM_GROUP_RELOAD_Z) {
                        double dReloadSpeed = apRef->GetAnimation()->GetReloadSpeed();
                        fSpeedMult = dReloadSpeed * fSpeedMult;
                    }

                    if (apRef != pPlayer || pPlayer->IsActually3rdPerson()) {
                        ePersonFlag = BSGameSound::SOUND_FLAG_NONE;
                    }
                    else {
                        ePersonFlag = BSGameSound::SOUND_FLAG_FIRST_PERSON;
                    }
                    TESSound* pSound = pgroupSound->pSound;
                    uint32_t uiFlags = iloopingFlags |
                        ((pPlayer != apRef ? 0 : BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE) |
                         (ePersonFlag |
                          (BSGameSound::SOUND_FLAG_IS_3D - ab2DSound | BSGameSound::SOUND_FLAG_ONE_SHOT))) |
                        BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN;
                    const char* pcFileName = pSound->GetFilename();
                    BSSoundHandle soundHand = BSAudio::QInstance()
                                                  .GetSoundHandleByFilename(pcFileName, uiFlags, pSound);
                    soundHand.SetPosition(loc.x, loc.y, loc.z);
                    float fVolume = static_cast<double>(pgroupSound->eType) / 255.0;
                    soundHand.SetVolume(fVolume);
                    soundHand.SetObjectToFollow(apRef->Get3D());
                    soundHand.SetModFrequency(pgroupSound->fPitch * fSpeedMult);
                    soundHand.Play(TESSound::ConvertLoopFlag(pgroupSound->pSound->GetData().iFlags) != 0);
                }
            }
            else {
                switch (pgroupSound->eType) {
                    case 0:
                    case 1:
                    case 2:
                    case 3: {
                        ImpactMixer::PlayFootstep(apRef, pgroupSound->eType);
                        break;
                    }
                    case 4:
                    case 5:
                    case 6:
                    case 23:
                    case 24:
                    case 25:
                    case 26:
                    case 27:
                    case 28:
                    case 29:
                    case 30: {
                        if (apRef->IsActor() && apRef && apRef->IsCreature()) {
                            TESSound* pcreatureSound;
                            TESCreature* pcreature = static_cast<TESCreature*>(apRef->GetObjectReference());
                            // The sound events from 22 on map to the creature sound types 11 on.
                            if (pgroupSound->eType >= 22u) {
                                pcreatureSound = pcreature->PickCreatureSound(
                                    static_cast<CREATURE_SOUND_TYPE>(pgroupSound->eType - 11)
                                );
                            }
                            else {
                                pcreatureSound = pcreature->PickCreatureSound(
                                    static_cast<CREATURE_SOUND_TYPE>(pgroupSound->eType)
                                );
                            }
                            NiPoint3 loc = apRef->GetLocationOnReference();
                            if (pcreatureSound && FalloutAudio::SoundDistanceTest(pcreatureSound, loc, 1.0f)) {
                                uint32_t uiFlags = (pPlayer != apRef ? 0 : BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE) |
                                    (BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                                     BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN);
                                uint32_t uiFormID = pcreatureSound->iFormID;
                                BSSoundHandle soundHand = BSAudio::QInstance().GetSoundHandleByNumericID(
                                    uiFormID,
                                    uiFlags
                                );
                                if (pAnimation &&
                                    GetType() >= ANIM_GROUP_RELOAD_START &&
                                    GetType() <= ANIM_GROUP_RELOAD_Z) {
                                    double dReloadSpeed = apRef->GetAnimation()->GetReloadSpeed();
                                    fSpeedMult = dReloadSpeed * fSpeedMult;
                                }

                                soundHand.SetPosition(loc.x, loc.y, loc.z);
                                soundHand.SetObjectToFollow(apRef->Get3D());
                                soundHand.SetVolume(0.98f);
                                float fFrequency = fSpeedMult * 1.0;
                                soundHand.SetModFrequency(fFrequency);
                                soundHand.Play(false);
                            }
                        }

                        break;
                    }
                    case 9: {
                        ImpactMixer::PlayEquipSound(apRef, 9);
                        break;
                    }
                    case 10: {
                        ImpactMixer::PlayEquipSound(apRef, 10);
                        break;
                    }
                    case 11: {
                        if (apSequence) {
                            NiTextKey* pkey = apSequence->GetTextKeys()->GetKeys(inumKeys);
                            if (inumKeys) {
                                // iEnumData: the text key's index in its low byte, the offset into its text above.
                                const char* pkeyText = pkey[static_cast<uint8_t>(pgroupSound->iEnumData)]
                                                           .m_kText.GetHandleString();
                                int32_t ioffset = (pgroupSound->iEnumData & 0xFFFFFF00u) >> 8;
                                float fmult = std::atof(&pkeyText[ioffset + 18]);
                                if (fmult > 0.0) {
                                    ShakeCamera(fmult, apRef->GetLocationOnReference());
                                }
                            }
                        }
                        break;
                    }
                    case 12: {
                        NiTextKey* pkey = apSequence->GetTextKeys()->GetKeys(inumKeys);
                        float ffadeSeconds = 1.0f;
                        if (inumKeys) {
                            const char* pkeyText = pkey[static_cast<uint8_t>(pgroupSound->iEnumData)]
                                                       .m_kText.GetHandleString();
                            int32_t ioffset = (pgroupSound->iEnumData & 0xFFFFFF00u) >> 8;
                            ffadeSeconds = std::atof(&pkeyText[ioffset + 17]);
                        }
                        NiNode* p3D = apRef->Get3D();
                        BSAudio::QInstance()
                            .StopMovingSounds(p3D, ffadeSeconds, BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN);
                        break;
                    }
                    case 13: {
                        pPlayer->PlayImpactBasedHitShader(apRef);
                        break;
                    }
                    case 16: {
                        if (apRef->IsActor()) {
                            TESObjectWEAP* pweapon = static_cast<Actor*>(apRef)->Actor::GetCurrentWeapon();
                            if (pweapon) {
                                TESSound* psound = pweapon->Get3DLoopingShotSound();
                                if (psound) {
                                    uint32_t uiFlags = (pPlayer != apRef ? 0
                                                                         : BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE) |
                                        (BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN | BSGameSound::SOUND_FLAG_ONE_SHOT |
                                         BSGameSound::SOUND_FLAG_IS_3D);
                                    uint32_t uiFormID = psound->iFormID;
                                    BSSoundHandle soundHand = BSAudio::QInstance().GetSoundHandleByNumericID(
                                        uiFormID,
                                        uiFlags
                                    );
                                    NiPoint3 loc = apRef->GetLocationOnReference();
                                    soundHand.SetPosition(loc.x, loc.y, loc.z);
                                    soundHand.SetObjectToFollow(apRef->Get3D());
                                    soundHand.SetVolume(0.98f);
                                    float fFrequency = fSpeedMult * 1.0;
                                    soundHand.SetModFrequency(fFrequency);
                                    soundHand.Play(false);
                                }
                            }
                        }
                        break;
                    }
                    case 17:
                    case 18:
                    case 19:
                    case 20:
                    case 21:
                    case 22: {
                        if (apRef->IsActor()) {
                            ItemChange* pItem = static_cast<Actor*>(apRef)->Actor::GetArmorBeingWorn(BO_UPPER_BODY);
                            if (pItem) {
                                TESObjectARMO* pArmor = static_cast<TESObjectARMO*>(pItem->pContainerObj);
                                TESSound* pFoleySound = pArmor->GetFoleySound(
                                    static_cast<ENUM_FOLEY_SOUND>(pgroupSound->eType)
                                );
                                NiPoint3 loc = apRef->GetLocationOnReference();
                                if (pFoleySound && FalloutAudio::SoundDistanceTest(pFoleySound, loc, 1.0f)) {
                                    BSSoundHandle soundHand;
                                    if (apRef == pPlayer) {
                                        uint32_t uiFormID = pFoleySound->iFormID;
                                        soundHand = BSAudio::QInstance().GetSoundHandleByNumericID(
                                            uiFormID,
                                            BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN |
                                                BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE |
                                                BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IS_2D
                                        );
                                    }
                                    else {
                                        uint32_t uiFormID = pFoleySound->iFormID;
                                        soundHand = BSAudio::QInstance().GetSoundHandleByNumericID(
                                            uiFormID,
                                            BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN |
                                                BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IS_3D
                                        );
                                        soundHand.SetPosition(loc.x, loc.y, loc.z);
                                        soundHand.SetObjectToFollow(apRef->Get3D());
                                    }
                                    soundHand.SetVolume(0.98f);
                                    float fFrequency = fSpeedMult * 1.0;
                                    soundHand.SetModFrequency(fFrequency);
                                    soundHand.Play(false);
                                }

                                if (pItem) {
                                    delete pItem;
                                }
                            }
                        }
                        break;
                    }
                    default: {
                        continue;
                    }
                }
            }
        }
    }
}

void TESAnimGroup::SetType_(ANIM_GROUP_ENUM aeType) {
    int32_t i;
    sType = aeType;
    for (i = 0; i < 5 && *animGroupActionNames[i][animGroupInfo[TESAnimGroup::GetType(aeType)].eAction]; ++i) {
        ;
    }
    frameCount = i;
    if (times) {
        ourFree(times);
    }
    if (frameCount) {
        times = static_cast<float*>(
            ourMalloc(frameCount <= SIZE_MAX / sizeof(float) ? sizeof(float) * frameCount : SIZE_MAX)
        );
        ourMemset(times, 0, sizeof(float) * frameCount);
    }
    else {
        times = nullptr;
    }
}

void TESAnimGroup::SetSoundCount(uint32_t aiCount) {
    uint32_t uiSoundCount = this->soundCount;
    this->soundCount = aiCount;
    if (this->soundCount) {
        if (uiSoundCount) {
            void* pOldSounds = ourMalloc(sizeof(GROUP_SOUND) * uiSoundCount);
            ourMemcpy(pOldSounds, sounds, sizeof(GROUP_SOUND) * uiSoundCount);
            sounds = static_cast<GROUP_SOUND*>(ourRealloc(sounds, sizeof(GROUP_SOUND) * this->soundCount));
            uint32_t uiCopyCount = uiSoundCount;
            if (uiSoundCount > this->soundCount) {
                uiCopyCount = this->soundCount;
            }
            ourMemcpy(sounds, pOldSounds, sizeof(GROUP_SOUND) * uiCopyCount);
            ourFree(pOldSounds);
            for (uint32_t i = uiSoundCount; i < this->soundCount; ++i) {
                sounds[i].pSound = nullptr;
                sounds[i].fTime = 0.0;
                sounds[i].eType = -6;
                sounds[i].fPitch = 1.0;
            }
        }
        else {
            sounds = static_cast<GROUP_SOUND*>(ourMalloc(sizeof(GROUP_SOUND) * this->soundCount));
            for (uint32_t j = 0; j < this->soundCount; ++j) {
                sounds[j].pSound = nullptr;
                sounds[j].fTime = 0.0;
                sounds[j].eType = -6;
                sounds[j].fPitch = 1.0;
            }
        }
    }
    else {
        ourFree(sounds);
        sounds = nullptr;
    }
}

void TESAnimGroup::SetParentRootNode(const char* apName) {
    pParentName = apName;
}

void TESAnimGroup::SetupPriorities(BSAnimGroupSequence* apSequence) {
    for (int32_t i = 0; i < 8; ++i) {
        if (pSoundPriorityBoneName[i]) {
            cSectionPriority[i] = apSequence->GetPriority(pSoundPriorityBoneName[i], nullptr);
        }
    }
}

void TESAnimGroup::SetSoundEnum(uint32_t aiIndex, ANIM_ENUM aeType, uint32_t aiEnumData) {
    if (aiIndex <= soundCount && sounds) {
        sounds[aiIndex].eType = aeType;
        sounds[aiIndex].iEnumData = aiEnumData;
    }
}

bool TESAnimGroup::IsAttackAction(uint16_t asGroup) {
    int32_t type = TESAnimGroup::GetType(asGroup);
    // The game tests only 255; there, types 245 to 254 read the string pointers after animGroupInfo
    // (animGroupActionNames), which match no action type. Here, every type past the table returns false.
    if (type >= ANIM_GROUP_COUNT) {
        return false;
    }
    if (type < 74 || type > 79) {
        return TESAnimGroup::IsAttackAction(animGroupInfo[type].eAction);
    }
    return true;
}

bool TESAnimGroup::IsAttackAction(ANIM_GROUP_ACTION_TYPE aeType) {
    return aeType >= AGAT_ATTACK && aeType <= AGAT_ATTACK_SPIN;
}

bool TESAnimGroup::IsAttackSpinAction(ANIM_GROUP_ACTION_TYPE aeType) {
    return aeType == AGAT_ATTACK_SPIN;
}

bool TESAnimGroup::IsAttackSpinAction(uint16_t asGroup) {
    ANIM_GROUP_ENUM eType = GetType(asGroup);
    // Any type past the end of animGroupInfo (including 0xFF) matches no action type, so it returns false instead of
    // indexing past animGroupInfo.
    if (eType >= ANIM_GROUP_COUNT) {
        return false;
    }
    return IsAttackSpinAction(animGroupInfo[eType].eAction);
}

bool TESAnimGroup::IsAimAction(uint16_t asGroup) {
    if (asGroup == 0xFF) {
        return false;
    }
    int32_t iType = GetType(asGroup);
    return iType >= ANIM_GROUP_AIM && iType <= ANIM_GROUP_AIM_IS_DOWN;
}

ANIM_GROUP_ENUM TESAnimGroup::GetType(uint16_t asType) {
    return static_cast<ANIM_GROUP_ENUM>(asType & TYPE_MASK);
}

ANIM_GROUP_MOVE_ENUM TESAnimGroup::GetMove(uint16_t asType) {
    return static_cast<ANIM_GROUP_MOVE_ENUM>((asType & MOVE_MASK) >> MOVE_POS);
}

ANIM_GROUP_WEAPON_ENUM TESAnimGroup::GetWeapon(uint16_t asType) {
    return static_cast<ANIM_GROUP_WEAPON_ENUM>((asType & WEAPON_MASK) >> WEAPON_POS);
}

uint16_t TESAnimGroup::AnimGroup(
    ANIM_GROUP_MOVE_ENUM aeMove,
    ANIM_GROUP_WEAPON_ENUM aeWeapon,
    ANIM_GROUP_ENUM aeType,
    bool abPowerArmor
) {
    return static_cast<uint16_t>(
        aeType + ((static_cast<uint16_t>(aeMove) << MOVE_POS) + (abPowerArmor << POWER_ARMOR_POS)) +
        (static_cast<uint16_t>(aeWeapon) << WEAPON_POS)
    );
}

bool TESAnimGroup::IsPowerAttackAction(ANIM_GROUP_ACTION_TYPE aeType) {
    return aeType == AGAT_ATTACK_POWER;
}

bool TESAnimGroup::IsPowerAttackAction(uint16_t asGroup) {
    ANIM_GROUP_ENUM eType = GetType(asGroup);
    // As IsAttackSpinAction: past animGroupInfo the game reads string pointers, which match no action type, so an
    // out-of-range eType returns false.
    if (eType >= ANIM_GROUP_COUNT) {
        return false;
    }
    return IsPowerAttackAction(animGroupInfo[eType].eAction);
}

bool TESAnimGroup::IsIdleAnim(uint16_t asGroup) {
    int32_t iType = GetType(asGroup);
    return iType >= ANIM_GROUP_IDLE && (iType <= ANIM_GROUP_DYNAMIC_IDLE || iType == ANIM_GROUP_BLOCK_IDLE);
}

bool TESAnimGroup::IsReloadAction(ANIM_GROUP_ENUM aeType) {
    return aeType >= ANIM_GROUP_RELOAD_A && aeType <= ANIM_GROUP_RELOAD_Z;
}

bool TESAnimGroup::IsIronSightsAction(ANIM_GROUP_ENUM aeGroup) {
    int8_t cIronSights;
    switch (aeGroup) {
        case ANIM_GROUP_AIM_IS:
        case ANIM_GROUP_AIM_IS_UP:
        case ANIM_GROUP_AIM_IS_DOWN:
        case ANIM_GROUP_ATTACK_LEFT_IS:
        case ANIM_GROUP_ATTACK_LEFT_IS_UP:
        case ANIM_GROUP_ATTACK_LEFT_IS_DOWN:
        case ANIM_GROUP_ATTACK_RIGHT_IS:
        case ANIM_GROUP_ATTACK_RIGHT_IS_UP:
        case ANIM_GROUP_ATTACK_RIGHT_IS_DOWN:
        case ANIM_GROUP_ATTACK_3_IS:
        case ANIM_GROUP_ATTACK_3_IS_UP:
        case ANIM_GROUP_ATTACK_3_IS_DOWN:
        case ANIM_GROUP_ATTACK_4_IS:
        case ANIM_GROUP_ATTACK_4_IS_UP:
        case ANIM_GROUP_ATTACK_4_IS_DOWN:
        case ANIM_GROUP_ATTACK_5_IS:
        case ANIM_GROUP_ATTACK_5_IS_UP:
        case ANIM_GROUP_ATTACK_5_IS_DOWN:
        case ANIM_GROUP_ATTACK_6_IS:
        case ANIM_GROUP_ATTACK_6_IS_UP:
        case ANIM_GROUP_ATTACK_6_IS_DOWN:
        case ANIM_GROUP_ATTACK_7_IS:
        case ANIM_GROUP_ATTACK_7_IS_UP:
        case ANIM_GROUP_ATTACK_7_IS_DOWN:
        case ANIM_GROUP_ATTACK_8_IS:
        case ANIM_GROUP_ATTACK_8_IS_UP:
        case ANIM_GROUP_ATTACK_8_IS_DOWN:
        case ANIM_GROUP_ATTACK_LOOP_IS:
        case ANIM_GROUP_ATTACK_LOOP_IS_UP:
        case ANIM_GROUP_ATTACK_LOOP_IS_DOWN:
        case ANIM_GROUP_ATTACK_SPIN_IS:
        case ANIM_GROUP_ATTACK_SPIN_IS_UP:
        case ANIM_GROUP_ATTACK_SPIN_IS_DOWN:
        case ANIM_GROUP_ATTACK_SPIN_2_IS:
        case ANIM_GROUP_ATTACK_SPIN_2_IS_UP:
        case ANIM_GROUP_ATTACK_SPIN_2_IS_DOWN:
        case ANIM_GROUP_PLACE_MINE_IS:
        case ANIM_GROUP_PLACE_MINE_IS_UP:
        case ANIM_GROUP_PLACE_MINE_IS_DOWN:
        case ANIM_GROUP_PLACE_MINE_2_IS:
        case ANIM_GROUP_PLACE_MINE_2_IS_UP:
        case ANIM_GROUP_PLACE_MINE_2_IS_DOWN:
        case ANIM_GROUP_ATTACK_THROW_IS:
        case ANIM_GROUP_ATTACK_THROW_IS_UP:
        case ANIM_GROUP_ATTACK_THROW_IS_DOWN:
        case ANIM_GROUP_ATTACK_THROW_2_IS:
        case ANIM_GROUP_ATTACK_THROW_2_IS_UP:
        case ANIM_GROUP_ATTACK_THROW_2_IS_DOWN:
        case ANIM_GROUP_ATTACK_THROW_3_IS:
        case ANIM_GROUP_ATTACK_THROW_3_IS_UP:
        case ANIM_GROUP_ATTACK_THROW_3_IS_DOWN:
        case ANIM_GROUP_ATTACK_THROW_4_IS:
        case ANIM_GROUP_ATTACK_THROW_4_IS_UP:
        case ANIM_GROUP_ATTACK_THROW_4_IS_DOWN:
        case ANIM_GROUP_ATTACK_THROW_5_IS:
        case ANIM_GROUP_ATTACK_THROW_5_IS_UP:
        case ANIM_GROUP_ATTACK_THROW_5_IS_DOWN:
        case ANIM_GROUP_ATTACK_9_IS:
        case ANIM_GROUP_ATTACK_9_IS_UP:
        case ANIM_GROUP_ATTACK_9_IS_DOWN: {
            cIronSights = 1;
            break;
        }
        default: {
            cIronSights = 0;
            break;
        }
    }
    return cIronSights != 0;
}

bool TESAnimGroup::IsIronSightsAction(uint16_t asGroup) {
    if (asGroup == 255) {
        return false;
    }
    int32_t type = TESAnimGroup::GetType(asGroup);
    return TESAnimGroup::IsIronSightsAction(static_cast<ANIM_GROUP_ENUM>(type));
}
