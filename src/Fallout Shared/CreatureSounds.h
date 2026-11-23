#pragma once

#include "BSCore/BSSimpleList.h"

class TESCreature;
class TESFile;
class TESSound;

// The kinds of sound a creature plays.
enum CREATURE_SOUND_TYPE : int32_t {
    CREA_SOUND_LEFT = 0,
    CREA_SOUND_RIGHT = 1,
    CREA_SOUND_BACKLEFT = 2,
    CREA_SOUND_BACKRIGHT = 3,
    CREA_SOUND_IDLE = 4,
    CREA_SOUND_AWARE = 5,
    CREA_SOUND_ATTACK = 6,
    CREA_SOUND_HIT = 7,
    CREA_SOUND_DEATH = 8,
    CREA_SOUND_WEAPON = 9,
    CREA_SOUND_MOVEMENT = 10,
    CREA_SOUND_AWAKE = 11,
    CREA_SOUND_AUX1 = 12,
    CREA_SOUND_AUX2 = 13,
    CREA_SOUND_AUX3 = 14,
    CREA_SOUND_AUX4 = 15,
    CREA_SOUND_AUX5 = 16,
    CREA_SOUND_AUX6 = 17,
    CREA_SOUND_AUX7 = 18,
    CREA_SOUND_AUX8 = 19,
    CREA_SOUND_JUMP = 20,
    CREA_SOUND_LOOP = 21,
    CREA_SOUND_COUNT = 22,
};

// One sound of a creature and its percent chance of playing.
struct CREATURE_SOUND {
    TESSound* pSound;
    uint8_t iChance;
};

// A creature's own sounds: one list per sound type.
class CreatureSounds {
public:
    CreatureSounds();
    ~CreatureSounds();

    // Frees the entries of the sound list aiType and deletes the list.
    void FreeCreatureSoundList(CREATURE_SOUND_TYPE aeType);
    // Replaces the lists with copies of apCopy's.
    void Copy(CreatureSounds* apCopy);

    TESSound* PickCreatureSound(CREATURE_SOUND_TYPE aeType);
    void FreeAllCreatureSoundLists();
    // Reads one sound (CSDI form ID or CSDF file name) and its chance (CSDC) from apFile into the aeType list;
    // false when apFile is null or no chance chunk follows.
    bool LoadCreatureSound(TESFile* apFile, CREATURE_SOUND_TYPE aeType, TESCreature* apOwner);
    // Adds apSound to the aeType list.
    void AddCreatureSound(CREATURE_SOUND* apSound, CREATURE_SOUND_TYPE aeType);

protected:
    // The list of sound type aeType, or null when aeType is out of range.
    BSSimpleList<CREATURE_SOUND*>* GetCreatureSoundList(CREATURE_SOUND_TYPE aeType);

public:
    void Save();
    bool Compare(CreatureSounds* apCompare);

    // Deleting frees through ourFree.
    static void operator delete(void* apMemory);
    static void CopyCreatureSound(CREATURE_SOUND* apCopyTo, CREATURE_SOUND* apCopyFrom);

    BSSimpleList<CREATURE_SOUND*>* pSoundListArray[CREA_SOUND_COUNT];
};
