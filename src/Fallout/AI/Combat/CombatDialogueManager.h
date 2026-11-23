#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/TESTopic.h"

#include <stdint.h>

class CombatController;
class TESObjectREFR;

// Starts the combat dialogue (taunts, warnings and the like) of the actors in combat. Only some of the members are
// declared.
class CombatDialogueManager {
public:
    // Has apCombatController's attacker say the combat topic to its target. The DIALOGUE_TYPE value is passed on to the
    // speaker's dialogue. The topic stays an int because its enum depends on the type (DIALOGUE_COMBAT for DT_COMBAT,
    // the detection topics for DT_DETECTION).
    bool StartDialogue(
        CombatController* apCombatController,
        DIALOGUE_TYPE aeDialogueType,
        int32_t aiDialogueTopic,
        bool abInterrupt
    );
    // Has apSpeaker say the combat topic to apTarget.
    bool StartDialogue(
        TESObjectREFR* apSpeaker,
        TESObjectREFR* apTarget,
        DIALOGUE_TYPE aeDialogueType,
        int32_t aiDialogueTopic,
        bool abInterrupt,
        CombatController* apCombatController
    );

    // Allocates the singleton once (the class has no data).
    static void Create();
    // Frees the singleton.
    static void Destroy();

    static inline int32_t uiLastCombatAttackSoundTime;
};

inline CombatDialogueManager* pCombatDialogueManager;

extern SettingT<INISettingCollection> bDisableCombatDialogue;
