#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionQualityInfo.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

#include <stdint.h>

class hkpCdBodyPairCollector;
class hkpCdPointCollector;
class hkpCollisionAgent;
class hkpContactMgr;
class hkpContactMgrFactory;
class hkpLinearCastCollisionInput;

// The agent types every dispatcher registers first: the null agent and the agent3 bridge.
enum hkpCollisionDispatcherAgentType : int32_t { HK_AGENT_TYPE_NULL = 0, HK_AGENT_TYPE_BRIDGE = 1 };

// The table of collision agents per pair of shape types. Only the fields this code touches are declared.
class hkpCollisionDispatcher : public hkReferencedObject {
public:
    using CreateFunc = hkpCollisionAgent* (*)(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpContactMgr* mgr
    );
    using GetPenetrationsFunc = void (*)(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCdBodyPairCollector& collector
    );
    using GetClosestPointsFunc = void (*)(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCdPointCollector& output
    );
    using LinearCastFunc = void (*)(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpLinearCastCollisionInput& input,
        hkpCdPointCollector& castCollector,
        hkpCdPointCollector* startCollector
    );

    // The functions of one agent, registered for a pair of shape types.
    struct AgentFuncs {
        CreateFunc m_createFunc;
        GetPenetrationsFunc m_getPenetrationsFunc;
        GetClosestPointsFunc m_getClosestPointFunc;
        LinearCastFunc m_linearCastFunc;
        hkBool m_isFlipped;
        hkBool m_isPredictive;
    };

    // The functions of one stream (agent3) agent, for a pair of shape types. Aligned to 16 on its first member.
    struct Agent3Funcs {
        // The flags and the optional functions cleared; the rest is left for initAgentFunc.
        Agent3Funcs() : m_isPredictive(false), m_ignoreSymmetricVersion(false), m_reusePreviousEntry(false) {
            m_updateFilterFunc = nullptr;
            m_calcStatisticsFunc = nullptr;
            m_invalidateTimFunc = nullptr;
            m_warpTimeFunc = nullptr;
            m_sepNormalFunc = nullptr;
        }

        alignas(16) hkAgent3::CreateFunc m_createFunc;
        hkAgent3::DestroyFunc m_destroyFunc;
        hkAgent3::CleanupFunc m_cleanupFunc;
        hkAgent3::RemovePointFunc m_removePointFunc;
        hkAgent3::CommitPotentialFunc m_commitPotentialFunc;
        hkAgent3::CreateZombieFunc m_createZombieFunc;
        hkAgent3::UpdateFilterFunc m_updateFilterFunc;
        hkAgent3::CalcStatisticsFunc m_calcStatisticsFunc;
        hkAgent3::InvalidateTimFunc m_invalidateTimFunc;
        hkAgent3::WarpTimeFunc m_warpTimeFunc;
        hkAgent3::SepNormalFunc m_sepNormalFunc;
        hkAgent3::ProcessFunc m_processFunc;
        hkBool m_isPredictive;
        hkBool m_ignoreSymmetricVersion;
        hkBool m_reusePreviousEntry;
    };

    enum {
        HK_MAX_SHAPE_TYPE = 32,
        HK_MAX_AGENT2_TYPES = 64,
        HK_MAX_AGENT3_TYPES = 16,
    };

    // One cell of a debug table: the pair of shape types and the priority of the agent registered for it.
    struct DebugEntry {
        char m_typeA;
        char m_typeB;
        char m_priority;
    };

    typedef DebugEntry DebugTable[HK_MAX_SHAPE_TYPE][HK_MAX_SHAPE_TYPE];

    // A shape type that stands in for another when looking agents up (the inheritance registered with
    // registerAlternateShapeType).
    struct ShapeInheritance {
        hkpShapeType m_primaryType;
        hkpShapeType m_alternateType;
    };

    // The inputs of the collision quality tables' initialisation.
    struct InitCollisionQualityInfo {
        float m_gravityLength;
        float m_collisionTolerance;
        float m_minDeltaTime;
        float m_maxLinearVelocity;
        float m_numToisTillAllowedPenetrationSimplifiedToi;
        float m_numToisTillAllowedPenetrationToi;
        float m_numToisTillAllowedPenetrationToiHigher;
        float m_numToisTillAllowedPenetrationToiForced;
        uint16_t m_defaultConstraintPriority;
        uint16_t m_toiConstraintPriority;
        uint16_t m_toiHigherConstraintPriority;
        uint16_t m_toiForcedConstraintPriority;
        hkBool m_wantContinuousCollisionDetection;
        hkBool m_enableNegativeManifoldTims;
        hkBool m_enableNegativeToleranceToCreateNon4dContacts;
        hkBool m_enableToiWeldRejection;
    };

    // The dispatcher's entry for one agent3 type: the registered functions and how the swapped pair is served.
    struct Agent3FuncsIntern : Agent3Funcs {
        hkAgent3::Symmetric m_symmetric;
        uint8_t padding44[0xC];
    };

    // The dispatcher's four agent tables.
    enum TableType : int32_t { TABLE_TYPE_AGENT2, TABLE_TYPE_AGENT2_PRED, TABLE_TYPE_AGENT3, TABLE_TYPE_AGENT3_PRED };

    // Whether an agent is looked up in the predictive (continuous) table.
    enum IsAgentPredictive : int32_t { IS_NOT_PREDICTIVE, IS_PREDICTIVE };

    // A registered agent for a pair of shape types, as the registration checks keep it.
    struct AgentEntry {
        hkpShapeType m_typeA;
        hkpShapeType m_typeB;
        hkBool m_checkEnabled;
        AgentFuncs m_createFunc;
    };

    // A registered agent3 for a pair of shape types, as the registration checks keep it.
    struct Agent3Entry {
        hkpShapeType m_typeA;
        hkpShapeType m_typeB;
        hkBool m_checkEnabled;
        Agent3Funcs m_funcs;
    };

    // The collision quality levels: the rows of the collision quality info table.
    enum CollisionQualityLevel : int32_t {
        COLLISION_QUALITY_INVALID,
        COLLISION_QUALITY_PSI,
        COLLISION_QUALITY_SIMPLIFIED_TOI,
        COLLISION_QUALITY_TOI,
        COLLISION_QUALITY_TOI_HIGHER,
        COLLISION_QUALITY_TOI_FORCED,
        COLLISION_QUALITY_CHARACTER,
        COLLISION_QUALITY_TMP_EXPAND_MANIFOLD
    };

    // The SDK's hkChar index of m_collisionQualityInfo: callers sign-extend a byte index into it.
    using CollisionQualityIndex = int8_t;

    hkpCollisionDispatcher(CreateFunc defaultCreationFunction, hkpContactMgrFactory* defaultContactMgrFactory);
    ~hkpCollisionDispatcher() override;

    void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;

    // Havok SDK.
    void registerCollisionAgent(AgentFuncs& f, hkpShapeType typeA, hkpShapeType typeB);

    // Havok SDK: registers into the dispatcher's agent2 tables only.
    void registerCollisionAgent2(AgentFuncs& f, hkpShapeType typeA, hkpShapeType typeB);

    // Nonzero when type is registered as also being alternateType.
    uint32_t hasAlternateType(hkpShapeType type, hkpShapeType alternateType) const {
        return m_hasAlternateType[type] & (1 << alternateType);
    }

    // Returns the agent3 type registered.
    int32_t registerAgent3(Agent3Funcs& f, hkpShapeType typeA, hkpShapeType typeB);

    // Havok SDK.
    void registerAlternateShapeType(hkpShapeType primaryType, hkpShapeType alternateType);

    // Whether registering an agent checks it against the ones before.
    void setEnableChecks(hkBool checkEnabled);
    // Reports, per shape type, the agent registered against every other type, continuous then discrete.
    void debugPrintTable();
    // Fills the collision quality tables from input.
    void initCollisionQualityInfo(InitCollisionQualityInfo& input);

protected:
    // Empties every table, puts the null agents in entry 0 of both function tables and registers hkAgent3Bridge.
    void resetCreationFunctions();

public:
    void registerContactMgrFactory(hkpContactMgrFactory* fac, int32_t responseA, int32_t responseB);
    // Enters agentType for the pair and, recursively, for the pairs of the shape types registered as alternates of
    // typeA and typeB.
    void internalRegisterCollisionAgent(
        uint8_t agentTypesTable[HK_MAX_SHAPE_TYPE][HK_MAX_SHAPE_TYPE],
        int32_t agentType,
        hkpShapeType typeA,
        hkpShapeType typeB,
        hkpShapeType origA,
        hkpShapeType origB,
        DebugTable* debugTable,
        int32_t depth
    );
    // Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
    void updateHasAlternateType(hkpShapeType primaryType, hkpShapeType alternateType, int32_t depth);
    // Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
    void disableDebugging();

    // The agent's closest-points function for the pair of shape types.
    GetClosestPointsFunc getGetClosestPointsFunc(hkpShapeType typeA, hkpShapeType typeB) const {
        int32_t idx = m_agent2Types[typeA][typeB];
        return m_agent2Func[idx].m_getClosestPointFunc;
    }

    // The agent's penetrations function for the pair of shape types (the SDK's inline).
    GetPenetrationsFunc getGetPenetrationsFunc(hkpShapeType typeA, hkpShapeType typeB) const {
        int32_t idx = m_agent2Types[typeA][typeB];
        return m_agent2Func[idx].m_getPenetrationsFunc;
    }

    // The agent's linear-cast function for the pair of shape types (the SDK's inline).
    LinearCastFunc getLinearCastFunc(hkpShapeType typeA, hkpShapeType typeB) const {
        int32_t idx = m_agent2Types[typeA][typeB];
        return m_agent2Func[idx].m_linearCastFunc;
    }

    // The agent's creation function for the pair of shape types, from the predictive table when predictive is set
    // (the SDK's inline).
    CreateFunc
    getCollisionAgentCreationFunction(hkpShapeType typeA, hkpShapeType typeB, IsAgentPredictive predictive) const {
        int32_t idx = (predictive ? m_agent2TypesPred : m_agent2Types)[typeA][typeB];
        return m_agent2Func[idx].m_createFunc;
    }

    // A new agent for the pair, from the predictive table when the input asks for predictive agents.
    hkpCollisionAgent* getNewCollisionAgent(
        const hkpCdBody& collA,
        const hkpCdBody& collB,
        const hkpCollisionInput& environment,
        hkpContactMgr* mgr
    ) const {
        const hkpShapeType typeA = collA.getShape()->m_type;
        const hkpShapeType typeB = collB.getShape()->m_type;
        const IsAgentPredictive predictive = static_cast<IsAgentPredictive>(
            environment.m_createPredictiveAgents.m_storage
        );
        CreateFunc f = getCollisionAgentCreationFunction(typeA, typeB, predictive);
        return f(collA, collB, environment, mgr);
    }

    hkpCollisionQualityInfo* getCollisionQualityInfo(CollisionQualityIndex index) {
        return &m_collisionQualityInfo[index];
    }

    void registerContactMgrFactoryWithAll(hkpContactMgrFactory* fac, int32_t responseA);

    CreateFunc m_defaultCollisionAgent;
    hkpContactMgrFactory* m_contactMgrFactory[8][8];
    // Alignment before the 16-byte aligned m_hasAlternateType.
    uint8_t padding10C[4];
    uint32_t m_hasAlternateType[HK_MAX_SHAPE_TYPE];
    int32_t m_numAgent2Types;
    uint8_t padding194[0xC];
    uint8_t m_agent2Types[HK_MAX_SHAPE_TYPE][HK_MAX_SHAPE_TYPE];
    uint8_t m_agent2TypesPred[HK_MAX_SHAPE_TYPE][HK_MAX_SHAPE_TYPE];
    AgentFuncs m_agent2Func[HK_MAX_AGENT2_TYPES];
    int32_t m_numAgent3Types;
    uint8_t paddingEA4[0xC];
    uint8_t m_agent3Types[HK_MAX_SHAPE_TYPE][HK_MAX_SHAPE_TYPE];
    uint8_t m_agent3TypesPred[HK_MAX_SHAPE_TYPE][HK_MAX_SHAPE_TYPE];
    Agent3FuncsIntern m_agent3Func[HK_MAX_AGENT3_TYPES];
    // The table is 10 x 10 (HK_COLLIDABLE_QUALITY_MAX is 10) and the info array is 16-byte aligned.
    int8_t m_collisionQualityTable[10][10];
    uint8_t padding1C14[0xC];
    hkpCollisionQualityInfo m_collisionQualityInfo[8];
    hkBool m_collisionAgentRegistered;
    hkBool m_agent3Registered;
    hkBool m_midphaseAgent3Registered;
    hkBool m_checkEnabled;
    hkArray<ShapeInheritance> m_shapeInheritance;
    DebugTable* m_debugAgent2Table;
    DebugTable* m_debugAgent2TablePred;
    DebugTable* m_debugAgent3Table;
    DebugTable* m_debugAgent3TablePred;
    float m_expectedMaxLinearVelocity;
    float m_expectedMinPsiDeltaTime;
    uint8_t padding1E48[8];
};
