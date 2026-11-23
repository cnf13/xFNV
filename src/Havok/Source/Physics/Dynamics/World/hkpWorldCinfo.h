#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"

void finishLoadedObjecthkpWorldCinfo(void* p, int32_t finishing);
void cleanupLoadedObjecthkpWorldCinfo(void* p);

// The construction parameters of a Havok world.
class alignas(16) hkpWorldCinfo : public hkReferencedObject {
public:
    // How the world steps: discretely, with continuous collision, or across the job threads.
    enum SimulationType {
        SIMULATION_TYPE_INVALID = 0x0,
        SIMULATION_TYPE_DISCRETE = 0x1,
        SIMULATION_TYPE_CONTINUOUS = 0x2,
        SIMULATION_TYPE_MULTITHREADED = 0x3,
    };

    // What the broad phase border does with an entity that leaves the broad phase.
    enum BroadPhaseBorderBehaviour {
        BROADPHASE_BORDER_ASSERT = 0x0,
        BROADPHASE_BORDER_FIX_ENTITY = 0x1,
        BROADPHASE_BORDER_REMOVE_ENTITY = 0x2,
        BROADPHASE_BORDER_DO_NOTHING = 0x3,
    };

    // Whether contact points are kept when the solver cannot confirm them (the SDK's enum).
    enum ContactPointGeneration {
        CONTACT_POINT_ACCEPT_ALWAYS = 0x0,
        CONTACT_POINT_REJECT_DUBIOUS = 0x1,
        CONTACT_POINT_REJECT_MANY = 0x2,
    };

    // Which bodies the AABB tree world manager rebuilds the tree from.
    enum TreeUpdateType {
        REBUILD_ACTIVE = 0x0,
        REBUILD_ALL = 0x1,
    };

    // The solver presets setupSolverInfo picks from: 2, 4 or 8 iterations, each soft, medium or hard.
    enum SolverType : int32_t {
        SOLVER_TYPE_INVALID = 0x0,
        SOLVER_TYPE_2ITERS_SOFT = 0x1,
        SOLVER_TYPE_2ITERS_MEDIUM = 0x2,
        SOLVER_TYPE_2ITERS_HARD = 0x3,
        SOLVER_TYPE_4ITERS_SOFT = 0x4,
        SOLVER_TYPE_4ITERS_MEDIUM = 0x5,
        SOLVER_TYPE_4ITERS_HARD = 0x6,
        SOLVER_TYPE_8ITERS_SOFT = 0x7,
        SOLVER_TYPE_8ITERS_MEDIUM = 0x8,
        SOLVER_TYPE_8ITERS_HARD = 0x9,
        SOLVER_TYPE_MAX_ID = 0xA,
    };

    hkpWorldCinfo();
    // The finish constructor: defaults m_contactRestingVelocity when the load left it zero.
    hkpWorldCinfo(hkFinishLoadedObjectFlag aFlag);
    ~hkpWorldCinfo() override;

    // Sets m_solverIterations, m_solverTau and m_solverDamp to the preset st.
    void setupSolverInfo(SolverType st);

    // A cube broad phase of side size centred on the origin.
    void setBroadPhaseWorldSize(float sideLength);

    static const hkClass& staticClass();

    hkVector4 m_gravity;
    int32_t m_broadPhaseQuerySize;
    float m_contactRestingVelocity;
    hkEnum<BroadPhaseBorderBehaviour, int8_t> m_broadPhaseBorderBehaviour;
    hkBool m_mtPostponeAndSortBroadPhaseBorderCallbacks;
    // The 6 bytes after the flags are padding for m_broadPhaseWorldAabb's 16-byte alignment.
    hkAabb m_broadPhaseWorldAabb;
    hkBool m_useKdTree;
    hkBool m_useMultipleTree;
    hkEnum<TreeUpdateType, int8_t> m_treeUpdateType;
    hkBool m_autoUpdateKdTree;
    float m_collisionTolerance;
    hkReferencedObject* m_collisionFilter;
    hkReferencedObject* m_convexListFilter;
    float m_expectedMaxLinearVelocity;
    int32_t m_sizeOfToiEventQueue;
    float m_expectedMinPsiDeltaTime;
    hkReferencedObject* m_memoryWatchDog;
    uint32_t m_broadPhaseNumMarkers;
    hkEnum<ContactPointGeneration, int8_t> m_contactPointGeneration;
    hkBool m_allowToSkipConfirmedCallbacks;
    float m_solverTau;
    float m_solverDamp;
    uint32_t m_solverIterations;
    uint32_t m_solverMicrosteps;
    float m_maxConstraintViolation;
    hkBool m_forceCoherentConstraintOrderingInSolver;
    float m_snapCollisionToConvexEdgeThreshold;
    float m_snapCollisionToConcaveEdgeThreshold;
    hkBool m_enableToiWeldRejection;
    hkBool m_enableDeprecatedWelding;
    float m_iterativeLinearCastEarlyOutDistance;
    uint32_t m_iterativeLinearCastMaxIterations;
    uint8_t m_deactivationNumInactiveFramesSelectFlag0;
    uint8_t m_deactivationNumInactiveFramesSelectFlag1;
    uint8_t m_deactivationIntegrateCounter;
    hkBool m_shouldActivateOnRigidBodyTransformChange;
    float m_deactivationReferenceDistance;
    float m_toiCollisionResponseRotateNormal;
    uint32_t m_maxSectorsPerCollideTask;
    hkBool m_processToisMultithreaded;
    int32_t m_maxEntriesPerToiCollideTask;
    int32_t m_maxNumToiCollisionPairsSinglethreaded;
    float m_numToisTillAllowedPenetrationSimplifiedToi;
    float m_numToisTillAllowedPenetrationToi;
    float m_numToisTillAllowedPenetrationToiHigher;
    float m_numToisTillAllowedPenetrationToiForced;
    hkBool m_enableDeactivation;
    hkEnum<SimulationType, int8_t> m_simulationType;
    hkBool m_enableSimulationIslands;
    uint32_t m_minDesiredIslandSize;
    hkBool m_processActionsInSingleThread;
    hkBool m_allowIntegrationOfIslandsWithoutConstraintsInASeparateJob;
    float m_frameMarkerPsiSnap;
    hkBool m_fireCollisionCallbacks;
};

extern hkClass hkpWorldCinfoClass;

extern hkTypeInfo hkpWorldCinfoTypeInfo;
