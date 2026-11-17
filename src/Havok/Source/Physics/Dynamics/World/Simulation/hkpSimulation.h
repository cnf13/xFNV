#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkpAgentNnEntry;
class hkpEntity;
class hkpProcessCollisionInput;
class hkpProcessCollisionOutput;
class hkpRigidBody;
class hkpWorld;
class hkQuaternion;
struct hkStepInfo;

// Defined in hkpWorld.h, which forward-declares this class; the step methods return it.
enum hkpStepResult : int32_t;

// Sets rb's rotation (its motion's transform) to newRotation, then the translation so that the local centre of mass
// stays at the world one.
void setRotationAroundCentreOfMass(hkpRigidBody* rb, hkQuaternion& newRotation);
hkClass* getSimulationClassType();

// The world's stepping of time and collision detection (the discrete simulation; hkpContinuousSimulation adds time of
// impact handling).
class hkpSimulation : public hkReferencedObject {
public:
    enum FindContacts {
        FIND_CONTACTS_DEFAULT,
        FIND_CONTACTS_EXTRA,
    };

    enum ResetCollisionInformation : int32_t {
        RESET_TOI = 0x1,
        RESET_TIM = 0x2,
        RESET_AABB = 0x4,
        RESET_ALL = 0x7,
    };

    enum LastProcessingStep : int32_t {
        INTEGRATE = 0x0,
        COLLIDE = 0x1,
    };

    // What processAgentsOfEntities runs on each agent: a hkpSimulation member function, called with the simulation.
    using AgentEntityCallback = void (*)(
        hkpSimulation* simulation,
        hkpAgentNnEntry* entry,
        const hkpProcessCollisionInput& input,
        hkpProcessCollisionOutput& output
    );
    // The reflection's member table: the lastProcessingStep member's enum is null in the static data and is filled in
    // by a dynamic initializer.
    using MembersArray = hkClassMember[9];

    // The discrete simulation of world.
    hkpSimulation(hkpWorld* world);
    // The SDK's finish constructor of a loaded object: it keeps the fields the load wrote.
    hkpSimulation(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag) {}
    ~hkpSimulation() override;

    // The times are hkReal (a float).
    virtual hkpStepResult stepDeltaTime(float physicsDeltaTime);
    virtual hkpStepResult integrate(float physicsDeltaTime);
    virtual hkpStepResult collide();
    virtual hkpStepResult advanceTime();
    virtual void collideEntitiesDiscrete(
        hkpEntity** entities,
        int32_t numEntities,
        hkpWorld* world,
        const hkStepInfo& stepInfo,
        hkpSimulation::FindContacts findExtraContacts
    );
    virtual void resetCollisionInformationForEntities(
        hkpEntity** entities,
        int32_t numEntities,
        hkpWorld* world,
        hkpSimulation::ResetCollisionInformation resetInfo
    );

    virtual void
    assertThereIsNoCollisionInformationForEntities(hkpEntity** entities, int32_t numEntities, hkpWorld* world) {}

    virtual void removeCollisionInformationForAgent(hkpAgentNnEntry* agent) {}
    virtual void assertThereIsNoCollisionInformationForAgent(hkpAgentNnEntry* agent) {}
    virtual void reintegrateAndRecollideEntities(
        hkpEntity** entities,
        int32_t numEntities,
        hkpWorld* world,
        int32_t reintegrateRecollideMode
    );
    virtual void collideInternal(const hkStepInfo& stepInfoIn);
    virtual void warpTime(float deltaTime) {}

    // Runs the discrete narrow phase on the agents of the entities.
    void collideEntitiesNarrowPhaseDiscrete(
        hkpEntity** entities,
        int32_t numEntities,
        const hkpProcessCollisionInput& input,
        FindContacts findExtraContacts
    );
    bool isSimulationAtMarker();
    void setFrameTimeMarker(float a1);
    // Havok SDK signature (hkpDynamics.lib, tied by reloc).
    void processAgentResetCollisionInformation(
        hkpAgentNnEntry* entry,
        const hkpProcessCollisionInput& processInput,
        hkpProcessCollisionOutput& processOutput
    );
    void checkDeltaTimeIsOk(float deltaTime);
    // Runs every action of every active island, in island then action order.
    void applyActions();

protected:
    // Collides again over the step that failed for lack of memory, after rechecking the world's filter.
    hkpStepResult reCollideAfterStepFailure();
    // Snaps the simulate-time marker to the current time and returns the time to advance to.
    float snapSimulateTimeAndGetTimeToAdvanceTo();
    // Integrates the active islands over the step.
    hkpStepResult integrateInternal(const hkStepInfo& stepInfoIn);
    // Runs processFunc on each agent of the entities, once per agent.
    void processAgentsOfEntities(
        hkpEntity** entities,
        int32_t numEntities,
        const hkpProcessCollisionInput& input,
        AgentEntityCallback processFunc,
        FindContacts findExtraContacts
    );

public:
    void setCurrentPsiTime(float time) {
        m_currentPsiTime = time;
    }
    // Whether the simulation's current time is its PSI time.
    bool isSimulationAtPsi() const;
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    void processAgentCollideDiscrete(
        hkpAgentNnEntry* entry,
        const hkpProcessCollisionInput& processInput,
        hkpProcessCollisionOutput& processOutput
    );

    // The broad phase pass of a discrete step over the entities.
    static void collideEntitiesBroadPhaseDiscrete(hkpEntity** delPairs, int32_t newPairs, hkpWorld* assertBuf);

    uint32_t m_determinismCheckFrameCounter;
    hkpWorld* m_world;
    hkEnum<LastProcessingStep, uint8_t> m_lastProcessingStep;
    uint8_t padding11[3];
    float m_currentTime;
    float m_currentPsiTime;
    float m_physicsDeltaTime;
    float m_simulateUntilTime;
    float m_frameMarkerPsiSnap;
    uint32_t m_previousStepResult;

    static MembersArray Members;
};
