#pragma once

#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Reconstruction.hpp"

struct hkpAgent1nSector;
class hkpCollisionAgentConfig;
class hkpCollisionQualityInfo;

// The collision input of a processing step: the step's times, the quality table, the spare agent sector and the
// agent configuration. The PDB's hkPadSpu<T> members are plain T on PC. Not polymorphic.
class hkpProcessCollisionInput : public hkpCollisionInput {
public:
	// The hybrid has no zero-parameter Construct: the implicit constructor, as the world's constructor and the
	// stack copies (filled by the copy constructor's reconstruction) use it.
	hkpProcessCollisionInput() = default;
	hkpProcessCollisionInput(const hkpProcessCollisionInput& a1);

	hkStepInfo m_stepInfo;
	// Mutable as in the SDK header: hkpSimulation::processAgentCollideDiscrete 0xCF92C0 sets it through its const
	// input.
	mutable hkpCollisionQualityInfo* m_collisionQualityInfo;
	// Mutable as in the SDK header: hkAgent1nMachine_Process 0xDA4AA0 takes and returns the spare sector through its
	// const input.
	mutable hkpAgent1nSector* m_spareAgentSector;
	void* m_dynamicsInfo;
	hkBool m_enableDeprecatedWelding;
	hkBool m_allowToSkipConfirmedCallbacks;
	hkpCollisionAgentConfig* m_config;
};
