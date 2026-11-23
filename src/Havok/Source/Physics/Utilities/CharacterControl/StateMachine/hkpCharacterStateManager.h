#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

class hkpCharacterState;

// The character states a character controller can register a handler for.
enum hkpCharacterStateType {
    HK_CHARACTER_ON_GROUND = 0x0,
    HK_CHARACTER_JUMPING = 0x1,
    HK_CHARACTER_IN_AIR = 0x2,
    HK_CHARACTER_CLIMBING = 0x3,
    HK_CHARACTER_FLYING = 0x4,
    HK_CHARACTER_USER_STATE_0 = 0x5,
    HK_CHARACTER_USER_STATE_1 = 0x6,
    HK_CHARACTER_USER_STATE_2 = 0x7,
    HK_CHARACTER_USER_STATE_3 = 0x8,
    HK_CHARACTER_USER_STATE_4 = 0x9,
    HK_CHARACTER_USER_STATE_5 = 0xA,
    HK_CHARACTER_MAX_STATE_ID = 0xB,
};

// The state machine of a character controller: the state handler registered for each character state type.
class hkpCharacterStateManager : public hkReferencedObject {
public:
    hkpCharacterStateManager();
    ~hkpCharacterStateManager() override;

    hkpCharacterState* getState(hkpCharacterStateType stateType);
    void registerState(hkpCharacterState* apState, hkpCharacterStateType aeState);

    hkpCharacterState* m_registeredState[HK_CHARACTER_MAX_STATE_ID];
};
