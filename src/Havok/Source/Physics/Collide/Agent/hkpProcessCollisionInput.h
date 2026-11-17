#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"

struct hkpAgent1nSector;
class hkpCollisionAgentConfig;
class hkpCollisionQualityInfo;

// The collision input of a processing step: the step's times, the quality table, the spare agent sector and the agent
// configuration. hkPadSpu<T> members are plain T on PC. Not polymorphic.
class hkpProcessCollisionInput : public hkpCollisionInput {
public:
	// The default constructor is the implicit one, as the world's constructor and the stack copies (filled by the copy
	// constructor) use it.
	hkpProcessCollisionInput() = default;
	hkpProcessCollisionInput(const hkpProcessCollisionInput& a1);

	hkStepInfo m_stepInfo;
	// Mutable because hkpSimulation::processAgentCollideDiscrete sets it through its const input.
	mutable hkpCollisionQualityInfo* m_collisionQualityInfo;
	// Mutable because hkAgent1nMachine_Process takes and returns the spare sector through its const input.
	mutable hkpAgent1nSector* m_spareAgentSector;
	void* m_dynamicsInfo;
	hkBool m_enableDeprecatedWelding;
	hkBool m_allowToSkipConfirmedCallbacks;
	hkpCollisionAgentConfig* m_config;
};
