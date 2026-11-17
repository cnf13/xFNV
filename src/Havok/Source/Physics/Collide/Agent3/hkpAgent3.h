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
typedef void hkpAgentData;

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

    typedef hkpAgentData* (*CreateFunc)(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* freeMemory);
    typedef void (*DestroyFunc)(
        hkpAgentEntry* entry,
        hkpAgentData* agentData,
        hkpContactMgr* mgr,
        hkpConstraintOwner& constraintOwner,
        hkpCollisionDispatcher* dispatcher
    );
    typedef hkpAgentData* (*CleanupFunc)(
        hkpAgentEntry* entry,
        hkpAgentData* agentData,
        hkpContactMgr* mgr,
        hkpConstraintOwner& constraintOwner
    );
    typedef void (*RemovePointFunc)(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToRemove);
    typedef void (*CommitPotentialFunc)(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t newId);
    typedef void (*CreateZombieFunc)(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToBecomeZombie);
    // Callers (hkpWorldAgentUtil) pass the pair's contact manager before the constraint owner.
    typedef void (*UpdateFilterFunc)(
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
    typedef void (*CalcStatisticsFunc)(
        hkpAgentEntry* entry,
        hkpAgentData* agentData,
        const hkpCollisionInput& input,
        hkStatisticsCollector* collector
    );
    typedef void (*InvalidateTimFunc)(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCollisionInput& input);
    typedef void (*WarpTimeFunc)(
        hkpAgentEntry* entry,
        hkpAgentData* agentData,
        float oldTime,
        float newTime,
        const hkpCollisionInput& input
    );
    typedef void (*SepNormalFunc)(
        const hkpAgent3Input& input,
        hkpAgentEntry* entry,
        hkpAgentData* agentData,
        hkVector4& separatingNormalOut
    );
    typedef hkpAgentData* (*ProcessFunc)(
        const hkpAgent3ProcessInput& input,
        hkpAgentEntry* entry,
        hkpAgentData* agentData,
        hkVector4* separatingNormalOut,
        hkpProcessCollisionOutput& result
    );
} // namespace hkAgent3
