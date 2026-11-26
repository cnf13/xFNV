#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

class BSAnimGroupSequence;
class NiControllerSequence;
class TESObjectREFR;
class TESSound;

enum ANIM_GROUP_ACTION_TYPE : int32_t;
enum ANIM_GROUP_SECTION : int32_t;

// The weapon kinds an animation group can be for.
enum ANIM_GROUP_WEAPON_ENUM : uint32_t {
    AGW_NONE = 0,
    AGW_HAND_TO_HAND = 1,
    AGW_ONE_HAND_MELEE = 2,
    AGW_TWO_HAND_MELEE = 3,
    AGW_ONE_HAND_PISTOL = 4,
    AGW_TWO_HAND_RIFLE = 5,
    AGW_TWO_HAND_AUTOMATIC = 6,
    AGW_TWO_HAND_HANDLE = 7,
    AGW_TWO_HAND_LAUNCHER = 8,
    AGW_ONE_HAND_GRENADE = 9,
    AGW_ONE_HAND_MINE = 10,
    AGW_ONE_HAND_LUNCHBOX_MINE = 11,
    ANIM_GROUP_WEAPON_COUNT = 12,
};

// The movement kinds an animation group can be for.
enum ANIM_GROUP_MOVE_ENUM : uint32_t {
    AGM_NONE = 0,
    AGM_WALK = 1,
    AGM_SNEAK = 2,
    AGM_SWIM = 3,
    AGM_FLY = 4,
    ANIM_GROUP_MOVE_COUNT = 5,
};


// The animation groups (TESAnimGroup::GetType).
enum ANIM_GROUP_ENUM : int32_t {
    ANIM_GROUP_NONE = 255,
    ANIM_GROUP_IDLE = 0,
    ANIM_GROUP_DYNAMIC_IDLE = 1,
    ANIM_GROUP_SPECIAL_IDLE = 2,
    ANIM_GROUP_MOVE_FORWARD = 3,
    ANIM_GROUP_BEGIN_MOVEMENT = 3,
    ANIM_GROUP_MOVE_BACK = 4,
    ANIM_GROUP_MOVE_LEFT = 5,
    ANIM_GROUP_MOVE_RIGHT = 6,
    ANIM_GROUP_FAST_FORWARD = 7,
    ANIM_GROUP_FAST_BACK = 8,
    ANIM_GROUP_FAST_LEFT = 9,
    ANIM_GROUP_FAST_RIGHT = 10,
    ANIM_GROUP_DODGE_FORWARD = 11,
    ANIM_GROUP_DODGE_BACK = 12,
    ANIM_GROUP_DODGE_LEFT = 13,
    ANIM_GROUP_DODGE_RIGHT = 14,
    ANIM_GROUP_TURN_LEFT = 15,
    ANIM_GROUP_TURN_RIGHT = 16,
    ANIM_GROUP_END_MOVEMENT = 16,
    ANIM_GROUP_AIM = 17,
    ANIM_GROUP_AIM_START = 17,
    ANIM_GROUP_AIM_UP = 18,
    ANIM_GROUP_AIM_DOWN = 19,
    ANIM_GROUP_AIM_IS = 20,
    ANIM_GROUP_AIM_IS_UP = 21,
    ANIM_GROUP_AIM_IS_DOWN = 22,
    ANIM_GROUP_AIM_STOP = 22,
    ANIM_GROUP_HOLSTER = 23,
    ANIM_GROUP_EQUIP = 24,
    ANIM_GROUP_WEAPONS_START = 24,
    ANIM_GROUP_UNEQUIP = 25,
    ANIM_GROUP_ATTACK_LEFT = 26,
    ANIM_GROUP_ATTACK_LEFT_UP = 27,
    ANIM_GROUP_ATTACK_LEFT_DOWN = 28,
    ANIM_GROUP_ATTACK_LEFT_IS = 29,
    ANIM_GROUP_ATTACK_LEFT_IS_UP = 30,
    ANIM_GROUP_ATTACK_LEFT_IS_DOWN = 31,
    ANIM_GROUP_ATTACK_RIGHT = 32,
    ANIM_GROUP_ATTACK_RIGHT_UP = 33,
    ANIM_GROUP_ATTACK_RIGHT_DOWN = 34,
    ANIM_GROUP_ATTACK_RIGHT_IS = 35,
    ANIM_GROUP_ATTACK_RIGHT_IS_UP = 36,
    ANIM_GROUP_ATTACK_RIGHT_IS_DOWN = 37,
    ANIM_GROUP_ATTACK_3 = 38,
    ANIM_GROUP_ATTACK_3_UP = 39,
    ANIM_GROUP_ATTACK_3_DOWN = 40,
    ANIM_GROUP_ATTACK_3_IS = 41,
    ANIM_GROUP_ATTACK_3_IS_UP = 42,
    ANIM_GROUP_ATTACK_3_IS_DOWN = 43,
    ANIM_GROUP_ATTACK_4 = 44,
    ANIM_GROUP_ATTACK_4_UP = 45,
    ANIM_GROUP_ATTACK_4_DOWN = 46,
    ANIM_GROUP_ATTACK_4_IS = 47,
    ANIM_GROUP_ATTACK_4_IS_UP = 48,
    ANIM_GROUP_ATTACK_4_IS_DOWN = 49,
    ANIM_GROUP_ATTACK_5 = 50,
    ANIM_GROUP_ATTACK_5_UP = 51,
    ANIM_GROUP_ATTACK_5_DOWN = 52,
    ANIM_GROUP_ATTACK_5_IS = 53,
    ANIM_GROUP_ATTACK_5_IS_UP = 54,
    ANIM_GROUP_ATTACK_5_IS_DOWN = 55,
    ANIM_GROUP_ATTACK_6 = 56,
    ANIM_GROUP_ATTACK_6_UP = 57,
    ANIM_GROUP_ATTACK_6_DOWN = 58,
    ANIM_GROUP_ATTACK_6_IS = 59,
    ANIM_GROUP_ATTACK_6_IS_UP = 60,
    ANIM_GROUP_ATTACK_6_IS_DOWN = 61,
    ANIM_GROUP_ATTACK_7 = 62,
    ANIM_GROUP_ATTACK_7_UP = 63,
    ANIM_GROUP_ATTACK_7_DOWN = 64,
    ANIM_GROUP_ATTACK_7_IS = 65,
    ANIM_GROUP_ATTACK_7_IS_UP = 66,
    ANIM_GROUP_ATTACK_7_IS_DOWN = 67,
    ANIM_GROUP_ATTACK_8 = 68,
    ANIM_GROUP_ATTACK_8_UP = 69,
    ANIM_GROUP_ATTACK_8_DOWN = 70,
    ANIM_GROUP_ATTACK_8_IS = 71,
    ANIM_GROUP_ATTACK_8_IS_UP = 72,
    ANIM_GROUP_ATTACK_8_IS_DOWN = 73,
    ANIM_GROUP_ATTACK_LOOP = 74,
    ANIM_GROUP_ATTACK_LOOP_UP = 75,
    ANIM_GROUP_ATTACK_LOOP_DOWN = 76,
    ANIM_GROUP_ATTACK_LOOP_IS = 77,
    ANIM_GROUP_ATTACK_LOOP_IS_UP = 78,
    ANIM_GROUP_ATTACK_LOOP_IS_DOWN = 79,
    ANIM_GROUP_ATTACK_SPIN = 80,
    ANIM_GROUP_ATTACK_SPIN_UP = 81,
    ANIM_GROUP_ATTACK_SPIN_DOWN = 82,
    ANIM_GROUP_ATTACK_SPIN_IS = 83,
    ANIM_GROUP_ATTACK_SPIN_IS_UP = 84,
    ANIM_GROUP_ATTACK_SPIN_IS_DOWN = 85,
    ANIM_GROUP_ATTACK_SPIN_2 = 86,
    ANIM_GROUP_ATTACK_SPIN_2_UP = 87,
    ANIM_GROUP_ATTACK_SPIN_2_DOWN = 88,
    ANIM_GROUP_ATTACK_SPIN_2_IS = 89,
    ANIM_GROUP_ATTACK_SPIN_2_IS_UP = 90,
    ANIM_GROUP_ATTACK_SPIN_2_IS_DOWN = 91,
    ANIM_GROUP_ATTACK_NORMAL_POWER = 92,
    ANIM_GROUP_ATTACK_POWER_START = 92,
    ANIM_GROUP_ATTACK_FORWARD_POWER = 93,
    ANIM_GROUP_ATTACK_BACK_POWER = 94,
    ANIM_GROUP_ATTACK_LEFT_POWER = 95,
    ANIM_GROUP_ATTACK_RIGHT_POWER = 96,
    ANIM_GROUP_ATTACK_CUSTOM_1_POWER = 97,
    ANIM_GROUP_ATTACK_CUSTOM_2_POWER = 98,
    ANIM_GROUP_ATTACK_CUSTOM_3_POWER = 99,
    ANIM_GROUP_ATTACK_CUSTOM_4_POWER = 100,
    ANIM_GROUP_ATTACK_CUSTOM_5_POWER = 101,
    ANIM_GROUP_POWER_ATTACK_STOP = 102,
    ANIM_GROUP_PLACE_MINE = 102,
    ANIM_GROUP_PLACE_MINE_UP = 103,
    ANIM_GROUP_PLACE_MINE_DOWN = 104,
    ANIM_GROUP_PLACE_MINE_IS = 105,
    ANIM_GROUP_PLACE_MINE_IS_UP = 106,
    ANIM_GROUP_PLACE_MINE_IS_DOWN = 107,
    ANIM_GROUP_PLACE_MINE_2 = 108,
    ANIM_GROUP_PLACE_MINE_2_UP = 109,
    ANIM_GROUP_PLACE_MINE_2_DOWN = 110,
    ANIM_GROUP_PLACE_MINE_2_IS = 111,
    ANIM_GROUP_PLACE_MINE_2_IS_UP = 112,
    ANIM_GROUP_PLACE_MINE_2_IS_DOWN = 113,
    ANIM_GROUP_ATTACK_THROW = 114,
    ANIM_GROUP_ATTACK_THROW_UP = 115,
    ANIM_GROUP_ATTACK_THROW_DOWN = 116,
    ANIM_GROUP_ATTACK_THROW_IS = 117,
    ANIM_GROUP_ATTACK_THROW_IS_UP = 118,
    ANIM_GROUP_ATTACK_THROW_IS_DOWN = 119,
    ANIM_GROUP_ATTACK_THROW_2 = 120,
    ANIM_GROUP_ATTACK_THROW_2_UP = 121,
    ANIM_GROUP_ATTACK_THROW_2_DOWN = 122,
    ANIM_GROUP_ATTACK_THROW_2_IS = 123,
    ANIM_GROUP_ATTACK_THROW_2_IS_UP = 124,
    ANIM_GROUP_ATTACK_THROW_2_IS_DOWN = 125,
    ANIM_GROUP_ATTACK_THROW_3 = 126,
    ANIM_GROUP_ATTACK_THROW_3_UP = 127,
    ANIM_GROUP_ATTACK_THROW_3_DOWN = 128,
    ANIM_GROUP_ATTACK_THROW_3_IS = 129,
    ANIM_GROUP_ATTACK_THROW_3_IS_UP = 130,
    ANIM_GROUP_ATTACK_THROW_3_IS_DOWN = 131,
    ANIM_GROUP_ATTACK_THROW_4 = 132,
    ANIM_GROUP_ATTACK_THROW_4_UP = 133,
    ANIM_GROUP_ATTACK_THROW_4_DOWN = 134,
    ANIM_GROUP_ATTACK_THROW_4_IS = 135,
    ANIM_GROUP_ATTACK_THROW_4_IS_UP = 136,
    ANIM_GROUP_ATTACK_THROW_4_IS_DOWN = 137,
    ANIM_GROUP_ATTACK_THROW_5 = 138,
    ANIM_GROUP_ATTACK_THROW_5_UP = 139,
    ANIM_GROUP_ATTACK_THROW_5_DOWN = 140,
    ANIM_GROUP_ATTACK_THROW_5_IS = 141,
    ANIM_GROUP_ATTACK_THROW_5_IS_UP = 142,
    ANIM_GROUP_ATTACK_THROW_5_IS_DOWN = 143,
    ANIM_GROUP_ATTACK_9 = 144,
    ANIM_GROUP_ATTACK_9_UP = 145,
    ANIM_GROUP_ATTACK_9_DOWN = 146,
    ANIM_GROUP_ATTACK_9_IS = 147,
    ANIM_GROUP_ATTACK_9_IS_UP = 148,
    ANIM_GROUP_ATTACK_9_IS_DOWN = 149,
    ANIM_GROUP_ATTACK_THROW_6 = 150,
    ANIM_GROUP_ATTACK_THROW_6_UP = 151,
    ANIM_GROUP_ATTACK_THROW_6_DOWN = 152,
    ANIM_GROUP_ATTACK_THROW_6_IS = 153,
    ANIM_GROUP_ATTACK_THROW_6_IS_UP = 154,
    ANIM_GROUP_ATTACK_THROW_6_IS_DOWN = 155,
    ANIM_GROUP_ATTACK_THROW_7 = 156,
    ANIM_GROUP_ATTACK_THROW_7_UP = 157,
    ANIM_GROUP_ATTACK_THROW_7_DOWN = 158,
    ANIM_GROUP_ATTACK_THROW_7_IS = 159,
    ANIM_GROUP_ATTACK_THROW_7_IS_UP = 160,
    ANIM_GROUP_ATTACK_THROW_7_IS_DOWN = 161,
    ANIM_GROUP_ATTACK_THROW_8 = 162,
    ANIM_GROUP_ATTACK_THROW_8_UP = 163,
    ANIM_GROUP_ATTACK_THROW_8_DOWN = 164,
    ANIM_GROUP_ATTACK_THROW_8_IS = 165,
    ANIM_GROUP_ATTACK_THROW_8_IS_UP = 166,
    ANIM_GROUP_ATTACK_THROW_8_IS_DOWN = 167,
    ANIM_GROUP_COUNTER = 168,
    ANIM_GROUP_COUNTER_ATTACK = 168,
    ANIM_GROUP_WEAPONS_STOP = 168,
    ANIM_GROUP_STOMP = 169,
    ANIM_GROUP_BLOCK_IDLE = 170,
    ANIM_GROUP_BLOCK_HIT = 171,
    ANIM_GROUP_RECOIL = 172,
    ANIM_GROUP_LOOPING_RELOAD_START_ANIMS = 173,
    ANIM_GROUP_RELOAD_W_START = 173,
    ANIM_GROUP_RELOAD_X_START = 174,
    ANIM_GROUP_RELOAD_Y_START = 175,
    ANIM_GROUP_RELOAD_Z_START = 176,
    ANIM_GROUP_RELOAD_START = 177,
    ANIM_GROUP_RELOAD_A = 177,
    ANIM_GROUP_RELOAD_B = 178,
    ANIM_GROUP_RELOAD_C = 179,
    ANIM_GROUP_RELOAD_D = 180,
    ANIM_GROUP_RELOAD_E = 181,
    ANIM_GROUP_RELOAD_F = 182,
    ANIM_GROUP_RELOAD_G = 183,
    ANIM_GROUP_RELOAD_H = 184,
    ANIM_GROUP_RELOAD_I = 185,
    ANIM_GROUP_RELOAD_J = 186,
    ANIM_GROUP_RELOAD_K = 187,
    ANIM_GROUP_RELOAD_L = 188,
    ANIM_GROUP_RELOAD_M = 189,
    ANIM_GROUP_RELOAD_N = 190,
    ANIM_GROUP_RELOAD_O = 191,
    ANIM_GROUP_RELOAD_P = 192,
    ANIM_GROUP_RELOAD_Q = 193,
    ANIM_GROUP_RELOAD_R = 194,
    ANIM_GROUP_RELOAD_S = 195,
    ANIM_GROUP_RELOAD_W = 196,
    ANIM_GROUP_RELOAD_X = 197,
    ANIM_GROUP_RELOAD_Y = 198,
    ANIM_GROUP_RELOAD_Z = 199,
    ANIM_GROUP_LOOPING_RELOAD_START = 196,
    ANIM_GROUP_RELOAD_END = 199,
    ANIM_GROUP_JAM_A = 200,
    ANIM_GROUP_JAM_B = 201,
    ANIM_GROUP_JAM_C = 202,
    ANIM_GROUP_JAM_D = 203,
    ANIM_GROUP_JAM_E = 204,
    ANIM_GROUP_JAM_F = 205,
    ANIM_GROUP_JAM_G = 206,
    ANIM_GROUP_JAM_H = 207,
    ANIM_GROUP_JAM_I = 208,
    ANIM_GROUP_JAM_J = 209,
    ANIM_GROUP_JAM_K = 210,
    ANIM_GROUP_JAM_L = 211,
    ANIM_GROUP_JAM_M = 212,
    ANIM_GROUP_JAM_N = 213,
    ANIM_GROUP_JAM_O = 214,
    ANIM_GROUP_JAM_P = 215,
    ANIM_GROUP_JAM_Q = 216,
    ANIM_GROUP_JAM_R = 217,
    ANIM_GROUP_JAM_S = 218,
    ANIM_GROUP_JAM_W = 219,
    ANIM_GROUP_JAM_X = 220,
    ANIM_GROUP_JAM_Y = 221,
    ANIM_GROUP_JAM_Z = 222,
    ANIM_GROUP_STAGGER = 223,
    ANIM_GROUP_DEATH = 224,
    ANIM_GROUP_TALKING = 225,
    ANIM_GROUP_PIPBOY = 226,
    ANIM_GROUP_PIP_BOY = 226,
    ANIM_GROUP_JUMP_START = 227,
    ANIM_GROUP_JUMP_LOOP = 228,
    ANIM_GROUP_JUMP_LAND = 229,
    ANIM_GROUP_HAND_GRIP_1 = 230,
    ANIM_GROUP_HAND_GRIP_2 = 231,
    ANIM_GROUP_HAND_GRIP_3 = 232,
    ANIM_GROUP_HAND_GRIP_4 = 233,
    ANIM_GROUP_HAND_GRIP_5 = 234,
    ANIM_GROUP_HAND_GRIP_6 = 235,
    ANIM_GROUP_JUMP_LOOP_FORWARD = 236,
    ANIM_GROUP_JUMP_LOOP_BACKWARD = 237,
    ANIM_GROUP_JUMP_LOOP_LEFT = 238,
    ANIM_GROUP_JUMP_LOOP_RIGHT = 239,
    ANIM_GROUP_PIPBOY_CHILD = 240,
    ANIM_GROUP_PIP_BOY_CHILD = 240,
    ANIM_GROUP_JUMP_LAND_FORWARD = 241,
    ANIM_GROUP_JUMP_LAND_BACKWARD = 242,
    ANIM_GROUP_JUMP_LAND_LEFT = 243,
    ANIM_GROUP_JUMP_LAND_RIGHT = 244,
    ANIM_GROUP_COUNT = 245,
    ANIM_GROUP_POWERATTACK_START = 92,
    ANIM_GROUP_POWERATTACK_COUNT = 10,
    ANIM_GROUP_POWERATTACK_AI_COUNT = 5,
    ANIM_GROUP_RELOAD_COUNT = 23,
    // No group; HighProcess::ProcessUseWeapon tests an attack group against -1. Distinct from ANIM_GROUP_NONE, which is
    // 255.
    ANIM_GROUP_INVALID = -1,
};

// The keys of a group TESAnimGroup::GetTime reads; each kind of group numbers its own actions from 0.
enum ANIM_GROUP_ACTION : int32_t {
    AGA_NONE = -1,
    AGA_START = 0,
    AGA_STOP = 1,
    AGA_LOOP_START = 0,
    AGA_LOOP_STOP = 1,
    AGA_SPECIAL_IDLE_START = 0,
    AGA_SPECIAL_IDLE_LOOP_START = 1,
    AGA_SPECIAL_IDLE_LOOP_STOP = 2,
    AGA_SPECIAL_IDLE_STOP = 3,
    AGA_LOOPING_RELOAD_START = 0,
    AGA_LOOPING_RELOAD_LOOP = 1,
    AGA_LOOPING_RELOAD_STOP = 2,
    AGA_ATTACK_START = 0,
    AGA_ATTACK_HIT = 1,
    AGA_ATTACK_EJECT = 2,
    AGA_ATTACK_POWER_STOP = 2,
    AGA_ATTACK_SWITCH = 3,
    AGA_ATTACK_STOP = 4,
    AGA_ATTACK_MINE_START = 0,
    AGA_ATTACK_MINE_RELEASE = 1,
    AGA_ATTACK_MINE_ATTACH = 2,
    AGA_ATTACK_MINE_STOP = 3,
    AGA_ATTACK_SPIN_START = 0,
    AGA_ATTACK_SPIN_FIRE = 1,
    AGA_ATTACK_SPIN_LOOP = 2,
    AGA_ATTACK_SPIN_STOP = 3,
    AGA_ATTACK_THROW_START = 0,
    AGA_ATTACK_THROW_HOLD = 1,
    AGA_ATTACK_THROW_RELEASE = 2,
    AGA_ATTACK_THROW_ATTACH = 3,
    AGA_ATTACK_THROW_STOP = 4,
    ANIM_GROUP_ACTION_COUNT = 5,
    AGA_BLOCK_START = 0,
    AGA_BLOCK_ATTACK = 1,
    AGA_BLOCK_STOP = 2,
    AGA_EQUIP_START = 0,
    AGA_EQUIP_ATTACH = 1,
    AGA_EQUIP_STOP = 2,
    AGA_UNEQUIP_START = 0,
    AGA_UNEQUIP_DETACH = 1,
    AGA_UNEQUIP_STOP = 2,
    AGA_CAST_START = 0,
    AGA_CAST_HIT = 1,
    AGA_CAST_STOP = 2,
};

// The sound slots of an animation group's keys (SetSoundEnum).
enum ANIM_ENUM : int32_t {
    ANIM_ENUM_LEFT = 0,
    ANIM_ENUM_RIGHT = 1,
    ANIM_ENUM_BACKLEFT = 2,
    ANIM_ENUM_BACKRIGHT = 3,
    ANIM_ENUM_IDLE = 4,
    ANIM_ENUM_AWARE = 5,
    ANIM_ENUM_ATTACK = 6,
    ANIM_ENUM_HIT = 7,
    ANIM_ENUM_DEATH = 8,
    ANIM_ENUM_EQUIP = 9,
    ANIM_ENUM_UNEQUIP = 10,
    ANIM_ENUM_CAMERA_SHAKE = 11,
    ANIM_ENUM_STOP_SOUNDS = 12,
    ANIM_ENUM_HIT_SHADER = 13,
    ANIM_ENUM_RIDER_SOFT = 14,
    ANIM_ENUM_RIDER_LOUD = 15,
    ANIM_ENUM_WEAPON_1 = 16,
    ANIM_ENUM_FOLEY_SNEAK = 17,
    ANIM_ENUM_FOLEY_WALK = 18,
    ANIM_ENUM_FOLEY_RUN = 19,
    ANIM_ENUM_ARMOR_SNEAK = 20,
    ANIM_ENUM_ARMOR_WALK = 21,
    ANIM_ENUM_ARMOR_RUN = 22,
    ANIM_ENUM_AUX_1 = 23,
    ANIM_ENUM_AUX_2 = 24,
    ANIM_ENUM_AUX_3 = 25,
    ANIM_ENUM_AUX_4 = 26,
    ANIM_ENUM_AUX_5 = 27,
    ANIM_ENUM_AUX_6 = 28,
    ANIM_ENUM_AUX_7 = 29,
    ANIM_ENUM_AUX_8 = 30,
    ANIM_ENUM_COUNT = 31,
};

// One sound of an animation group: played at fTime into the animation.
struct GROUP_SOUND {
    float fTime;

    union {
        uint8_t cVolume;
        uint8_t eType;
    };

    uint8_t padding05[3];

    union {
        float fPitch;
        uint32_t iEnumData;
    };

    TESSound* pSound;
};

// An animation group: the timing, sections and sounds of one kind of animation.
class TESAnimGroup : public NiRefObject {
public:
    using AnimGroupSound = GROUP_SOUND;

    // The fields of a group id: the group type in the low byte, then the weapon, the movement and power armour.
    enum : uint16_t {
        TYPE_MASK = 0x00FF,
        WEAPON_MASK = 0x0F00,
        WEAPON_POS = 8,
        MOVE_MASK = 0x7000,
        MOVE_POS = 12,
        POWER_ARMOR_MASK = 0x8000,
        POWER_ARMOR_POS = 15,
    };

    TESAnimGroup(uint16_t asType);
    ~TESAnimGroup() override;

    // Whether the group's id is an attack.
    bool IsAttackAction();
    // The time of the group's key for groupAction (0 when out of range or NaN, with a warning).
    float GetTime(ANIM_GROUP_ACTION groupAction);
    // The group's type: the low byte of its group id.
    ANIM_GROUP_ENUM GetType() const;
    // The group's movement kind, from its group id.
    ANIM_GROUP_MOVE_ENUM GetMove();
    // The group's weapon kind, from its group id.
    ANIM_GROUP_WEAPON_ENUM GetWeapon();
    // Whether the group's type is a power attack.
    bool IsPowerAttack();
    // Whether the group's type is a jump loop.
    bool IsJumpingLoopAnim();
    bool SetTime(uint32_t aiIndex, float afValue);
    void SetSpeed(NiPoint3* apSpeed);
    float GetSpeed();
    // The group's movement speed per second.
    NiPoint3 GetDeltaPerSecond();
    // The blend-in frames, or the blend frames when those are more.
    uint8_t GetBlendInFrames();
    // The blend-out frames, or the blend frames when those are more.
    uint8_t GetBlendOutFrames();
    void SetDecal(uint8_t acDecal);
    uint8_t GetDecal();
    // cSectionPriority[aeSection].
    uint8_t GetSectionPriority(ANIM_GROUP_SECTION aeSection);
    bool IsAimAnim();
    void SetBlend(uint8_t acBlendFrames);
    // Stores the argument's byte as it is (LoadAnimGroup passes atoi's low byte).
    void SetBlendOutFrames(uint8_t acBlendFrames);
    void SetSoundTime(uint32_t aiIndex, float afValue);
    void SetSoundObject(uint32_t aiIndex, TESSound* apSound);
    void SetSoundVolume(uint32_t aiIndex, float afValue);
    void SetSoundPitch(uint32_t aiIndex, float afValue);
    GROUP_SOUND* GetSound(uint32_t index);
    void PlaySounds(TESObjectREFR* apRef, float afStartTime, float afStopTime, BSAnimGroupSequence* apSequence);
    void SetType_(ANIM_GROUP_ENUM aeType);
    void SetSoundCount(uint32_t aiCount);
    void SetParentRootNode(const char* apName);
    void SetSoundEnum(uint32_t aiIndex, ANIM_ENUM aeType, uint32_t aiEnumData);

    // Whether the group number asGroup is an attack.
    static bool IsAttackAction(uint16_t asGroup);
    // Whether the action type aeType is an attack.
    static bool IsAttackAction(ANIM_GROUP_ACTION_TYPE aeType);
    // Whether the action type aeType is a spin attack.
    static bool IsAttackSpinAction(ANIM_GROUP_ACTION_TYPE aeType);
    // The same test for a group id: false for 0xFF, else on its group type's action.
    static bool IsAttackSpinAction(uint16_t asGroup);
    static bool IsAimAction(uint16_t asGroup);
    static ANIM_GROUP_ENUM GetType(uint16_t asType);
    static ANIM_GROUP_MOVE_ENUM GetMove(uint16_t asType);
    static ANIM_GROUP_WEAPON_ENUM GetWeapon(uint16_t asType);
    // The group id of a movement, weapon and group type.
    static uint16_t
    AnimGroup(ANIM_GROUP_MOVE_ENUM aeMove, ANIM_GROUP_WEAPON_ENUM aeWeapon, ANIM_GROUP_ENUM aeType, bool abPowerArmor);
    // Whether the action type aeType is a power attack.
    static bool IsPowerAttackAction(ANIM_GROUP_ACTION_TYPE aeType);
    // The same test for a group id: false for 0xFF, else on its group type's action.
    static bool IsPowerAttackAction(uint16_t asGroup);
    // Whether the group id's type is a plain, dynamic or block idle.
    static bool IsIdleAnim(uint16_t asGroup);
    // Whether the group type is a reload.
    static bool IsReloadAction(ANIM_GROUP_ENUM aeType);
    // Whether the group type is an iron sights attack.
    static bool IsIronSightsAction(ANIM_GROUP_ENUM aeGroup);
    // The same test for a group id: false for 0xFF, else on its GetType.
    static bool IsIronSightsAction(uint16_t asGroup);
    // apPath is only passed to strrchr; LoadAnimGroup passes its const char* on. Writes the move type through aeMove
    // and the weapon type through aeWeapon, into LoadAnimGroup's move and weapon locals.
    static void ParseAnimGroupFilename(
        const char* apPath,
        ANIM_GROUP_MOVE_ENUM* aeMove,
        ANIM_GROUP_WEAPON_ENUM* aeWeapon,
        bool* abIsPowerArmor
    );
    // apFilename is the const char* from KFModel::InitSequence, passed on to strrchr.
    static TESAnimGroup* LoadAnimGroup(NiControllerSequence* apSequence, const char* apFilename);

private:
    static char* GetTextKey(char* Str);
    void FillPriorityArray(NiControllerSequence* apSequence);

public:
    uint8_t cSectionPriority[8];
    uint16_t sType;
    uint32_t frameCount;
    float* times;
    NiPoint3 speed;
    char cMorphKey;
    uint8_t cBlendFrames;
    uint8_t cBlendInFrames;
    uint8_t cBlendOutFrames;
    uint8_t cDecal;
    const char* pParentName;
    uint32_t soundCount;
    GROUP_SOUND* sounds;
};

NiSmartPointer(TESAnimGroup);

// The animation group weapon kind of each TESObjectWEAP weapon type (GetWeaponType, 14 types).
using WeaponToAnimGroupWeaponTable = ANIM_GROUP_WEAPON_ENUM[14];
inline WeaponToAnimGroupWeaponTable weaponToAnimGroupWeapon{
    AGW_HAND_TO_HAND,
    AGW_ONE_HAND_MELEE,
    AGW_TWO_HAND_MELEE,
    AGW_ONE_HAND_PISTOL,
    AGW_ONE_HAND_PISTOL,
    AGW_TWO_HAND_RIFLE,
    AGW_TWO_HAND_AUTOMATIC,
    AGW_TWO_HAND_RIFLE,
    AGW_TWO_HAND_HANDLE,
    AGW_TWO_HAND_LAUNCHER,
    AGW_ONE_HAND_GRENADE,
    AGW_ONE_HAND_MINE,
    AGW_ONE_HAND_LUNCHBOX_MINE,
    AGW_ONE_HAND_GRENADE
};

// The file name stem of each animation group weapon kind ("" for none), indexed by ANIM_GROUP_WEAPON_ENUM. The entry
// count is the enum's.
using AnimWeaponInfoTable = const char* [ANIM_GROUP_WEAPON_COUNT];
inline AnimWeaponInfoTable
    animWeaponInfo{"", "H2H", "1HM", "2HM", "1HP", "2HR", "2HA", "2HH", "2HL", "1GT", "1MD", "1LM"};

// The bone names the sound priorities are looked up by.
using SoundPriorityBoneNameTable = char* [8];
inline SoundPriorityBoneNameTable pSoundPriorityBoneName{
    const_cast<char*>("Bip01 Pelvis"),
    const_cast<char*>("Bip01"),
    const_cast<char*>("Bip01 L Forearm"),
    const_cast<char*>("Bip01 Head"),
    const_cast<char*>("Weapon"),
    nullptr,
    nullptr,
    const_cast<char*>("Bip01 Neck1")
};
