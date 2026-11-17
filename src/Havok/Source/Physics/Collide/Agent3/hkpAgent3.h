#pragma once

#include <cstdint>

class hkpAgent3Input;
class hkpAgent3ProcessInput;
struct hkpAgentEntry;
class hkpCdBody;
class hkpCollisionDispatcher;
class hkpCollisionInput;
class hkpConstraintOwner;
class hkpContactMgr;
class hkpProcessCollisionOutput;
class hkStatisticsCollector;
class hkVector4;

// The SDK's hkpAgentData is void: the bytes of an agent's data, in front of its entry.
using hkpAgentData = void;

// The function types of a stream (agent3) collision agent: the dispatcher keeps one table of them per agent type.
// The constraint owners are hkCollisionConstraintOwner, Havok's name for hkpConstraintOwner here; the times are
// hkTime (a float) and the ids hkContactPointId (an unsigned short).
namespace hkAgent3 {
    // Whether the dispatcher may use an agent3 for the swapped pair of shape types as well.
    enum Symmetric {
        IS_SYMMETRIC,
        IS_NOT_SYMMETRIC,
        IS_NOT_SYMMETRIC_AND_FLIPPED,
    };

    // The command of an entry in an agent stream: what kind of record follows (a call into an agent, with or without a
    // cached time of impact, flipped or not, with or without a transform), or the stream's end and padding.
    enum StreamCommand {
        STREAM_NULL = 0,
        STREAM_END = 1,
        STREAM_CALL = 2,
        STREAM_CALL_FLIPPED = 3,
        STREAM_CALL_WITH_TIM = 4,
        STREAM_CALL_WITH_TIM_FLIPPED = 5,
        STREAM_CALL_AGENT = 6,
        TRANSFORM_FLAG = 8,
        STREAM_CALL_WITH_TRANSFORM = STREAM_CALL | TRANSFORM_FLAG,
        STREAM_CALL_FLIPPED_WITH_TRANSFORM = STREAM_CALL_FLIPPED | TRANSFORM_FLAG,
        STREAM_CALL_WITH_TIM_WITH_TRANSFORM = STREAM_CALL_WITH_TIM | TRANSFORM_FLAG,
        STREAM_CALL_WITH_TIM_FLIPPED_WITH_TRANSFORM = STREAM_CALL_WITH_TIM_FLIPPED | TRANSFORM_FLAG,
        STREAM_CALL_AGENT_WITH_TRANSFORM = STREAM_CALL_AGENT | TRANSFORM_FLAG,
    };

    using CreateFunc = hkpAgentData* (*)(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* freeMemory);
    using DestroyFunc = void (*)(
        hkpAgentEntry* entry,
        hkpAgentData* agentData,
        hkpContactMgr* mgr,
        hkpConstraintOwner& constraintOwner,
        hkpCollisionDispatcher* dispatcher
    );
    using CleanupFunc = hkpAgentData* (*)(
        hkpAgentEntry* entry,
        hkpAgentData* agentData,
        hkpContactMgr* mgr,
        hkpConstraintOwner& constraintOwner
    );
    using RemovePointFunc = void (*)(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToRemove);
    using CommitPotentialFunc = void (*)(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t newId);
    using CreateZombieFunc = void (*)(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToBecomeZombie);
    // Callers (hkpWorldAgentUtil) pass the pair's contact manager before the constraint owner.
    using UpdateFilterFunc = void (*)(
        hkpAgentEntry* entry,
        hkpAgentData* agentData,
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpContactMgr* mgr,
        hkpConstraintOwner& constraintOwner
    );
    // The agent3 bridge's calcStatisticsFunc reports the child agent stored at agentData to the collector, its fourth
    // argument.
    using CalcStatisticsFunc = void (*)(
        hkpAgentEntry* entry,
        hkpAgentData* agentData,
        const hkpCollisionInput& input,
        hkStatisticsCollector* collector
    );
    using InvalidateTimFunc = void (*)(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCollisionInput& input);
    using WarpTimeFunc = void (*)(
        hkpAgentEntry* entry,
        hkpAgentData* agentData,
        float oldTime,
        float newTime,
        const hkpCollisionInput& input
    );
    using SepNormalFunc = void (*)(
        const hkpAgent3Input& input,
        hkpAgentEntry* entry,
        hkpAgentData* agentData,
        hkVector4& separatingNormalOut
    );
    using ProcessFunc = hkpAgentData* (*)(
        const hkpAgent3ProcessInput& input,
        hkpAgentEntry* entry,
        hkpAgentData* agentData,
        hkVector4* separatingNormalOut,
        hkpProcessCollisionOutput& result
    );
} // namespace hkAgent3
