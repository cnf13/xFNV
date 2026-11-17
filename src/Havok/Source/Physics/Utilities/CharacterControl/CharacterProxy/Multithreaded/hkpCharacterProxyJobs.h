#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJob.h"
#include "Havok/Source/Common/Base/Thread/Semaphore/hkSemaphoreBusyWait.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxyListener.h"

class hkpCharacterProxy;
struct hkpCharacterProxyIntegrateCommand;
class hkpRigidBody;
struct hkStepInfo;

// The header the character proxy jobs of one batch share: the number of the batch's jobs still open (the SDK's
// HK_ALIGN16 counter, which pads the struct to 0x10).
struct hkpCharacterProxyJobHeader {
	alignas(16) mutable int32_t m_openJobs;
};

// The base of the character proxy jobs; the m_jobSubType byte is its JobSubType.
struct hkpCharacterProxyJob : public hkJob {
	// Values from the Havok SDK.
	enum JobSubType : int32_t {
		CHARACTER_PROXY_JOB_INTEGRATE = 0x0,
		CHARACTER_PROXY_JOB_END = 0x1,
	};

	hkSemaphoreBusyWait* m_semaphore;
	hkpCharacterProxyJobHeader* m_sharedJobHeaderOnPpu;
	uint32_t* m_jobDoneFlag;
};

// One object interaction a multithreaded integrate reports: the impulse and where it applies, and the body it hit
// (hkpCharacterProxyIntegrateCommand::m_objectInteraction points at an array of them).
struct hkpCharacterProxyInteractionResults : hkpCharacterObjectInteractionResult {
	hkpRigidBody* m_collidingBody;
};

// The job that integrates a batch of character proxies.
struct hkpCharacterProxyIntegrateJob : public hkpCharacterProxyJob {
	// Values from the Havok SDK.
	enum {
		MAX_MAXIMUM_INTERACTIONS_PER_CHARACTER = 10,
		MAXIMUM_NUMBER_OF_COMMANDS_PER_TASK = 128,
	};

	const hkpCharacterProxyIntegrateCommand* m_commandArray;
	hkpCharacterProxy** m_characterArrayPtr;
	int32_t m_numCommands;
	const hkStepInfo* m_stepInfo;
	const hkVector4 m_worldGravity;
	const int32_t m_maxInteractions;
	const int32_t m_numCommandsPerTask;
};
