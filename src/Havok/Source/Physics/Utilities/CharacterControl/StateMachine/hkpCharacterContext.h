#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterStateManager.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpSurfaceInfo.h"

// A character's input to its state machine: the movement axes, the jump and ladder flags, what is under it, the step's
// timing, its position, velocity and gravity and the game's user data. The vectors put the struct on Havok's 16-byte
// alignment.
struct hkpCharacterInput {
    float m_inputLR;
    float m_inputUD;
    hkBool m_wantJump;
    hkVector4 m_up;
    hkVector4 m_forward;
    hkBool m_atLadder;
    hkpSurfaceInfo m_surfaceInfo;
    hkStepInfo m_stepInfo;
    hkVector4 m_position;
    hkVector4 m_velocity;
    hkVector4 m_characterGravity;
    hkUlong m_userData;
};

// What the state machine writes back: the character's velocity.
struct hkpCharacterOutput {
    hkVector4 m_velocity;
};

// A character's state machine: which state it is in now, which it was in before, and how its movement is filtered.
class hkpCharacterContext : public hkReferencedObject {
public:
    // The kind of character the context drives: a proxy or a rigid body.
    enum CharacterType : int32_t {
        HK_CHARACTER_PROXY = 0,
        HK_CHARACTER_RIGIDBODY = 1,
    };

    hkpCharacterContext(const hkpCharacterStateManager* manager, hkpCharacterStateType initialState);
    ~hkpCharacterContext() override;

    // The state the character is in now.
    hkpCharacterStateType getState();
    void setFilterParameters(const float gain, const float maxVelocity, const float maxAcceleration);
    void setState(hkpCharacterStateType newState, const hkpCharacterInput& input, hkpCharacterOutput& output);
    void update(const hkpCharacterInput& input, hkpCharacterOutput& output);
    void setFilterEnable(const hkBool status);
    void setCharacterType(const hkpCharacterContext::CharacterType newType);

    CharacterType m_characterType;
    hkpCharacterStateManager* m_stateManager;
    hkpCharacterStateType m_currentState;
    hkpCharacterStateType m_previousState;
    hkBool m_filterEnable;
    float m_maxLinearAcceleration;
    float m_maxLinearVelocity;
    float m_gain;
};
