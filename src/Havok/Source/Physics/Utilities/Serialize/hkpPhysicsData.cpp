#include "Havok/Source/Physics/Utilities/Serialize/hkpPhysicsData.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpAgentRegisterUtil.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/SaveContactPoints/hkpSaveContactPointsUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpPhysicsSystem.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCinfo.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpPhysicsSystemWithContacts.h"
#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpSerializedAgentNnEntry.h"

static void finishLoadedObjecthkpPhysicsData(void* p, int32_t finishing);
// Havok SDK signature (hkpUtilities.lib, tied by reloc,target).
static const void* getVtablehkpPhysicsData();
static void cleanupLoadedObjecthkpPhysicsData(void* p);
static int32_t tryMoveEntity(hkpPhysicsSystem* fromSystem, hkpEntity* entity, hkpPhysicsSystem* toSystem);

hkpPhysicsData::MembersArray hkpPhysicsData::Members{
    {"worldCinfo",
     &hkpWorldCinfoClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x8,
     nullptr},
    {"systems",
     &hkpPhysicsSystemClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0xC,
     nullptr}
};
hkClass hkpPhysicsDataClass(
    "hkpPhysicsData",
    &hkReferencedObjectClass,
    24,
    nullptr,
    0,
    nullptr,
    0,
    hkpPhysicsData::Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpPhysicsDataTypeInfo = {
    "hkpPhysicsData",
    finishLoadedObjecthkpPhysicsData,
    cleanupLoadedObjecthkpPhysicsData,
    getVtablehkpPhysicsData(),
    &typeid(hkpPhysicsData),
    sizeof(hkpPhysicsData)
};

// The type info's finish-loaded-object callback for hkpPhysicsData: runs its finish constructor.
static void finishLoadedObjecthkpPhysicsData(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag aFlag;
        aFlag.m_finishing = finishing;
        ::new (p) hkpPhysicsData(aFlag);
    }
}

static const void* getVtablehkpPhysicsData() {
    return hkGetVtableOf<hkpPhysicsData>();
}

static void cleanupLoadedObjecthkpPhysicsData(void* p) {
    static_cast<hkpPhysicsData*>(p)->~hkpPhysicsData();
}

static int32_t tryMoveEntity(hkpPhysicsSystem* fromSystem, hkpEntity* entity, hkpPhysicsSystem* toSystem) {
    int32_t v7;
    hkpRigidBody** v10;
    int32_t m_size = fromSystem->m_rigidBodies.m_size;
    int32_t v5 = 0;
    if (m_size <= 0) {
        v7 = -1;
    }
    else {
        hkpRigidBody** m_data = fromSystem->m_rigidBodies.begin();
        while (*m_data != entity) {
            ++v5;
            ++m_data;
            if (v5 >= m_size) {
                v5 = -1;
                break;
            }
        }
        v7 = v5;
    }
    int32_t v8 = toSystem->m_rigidBodies.m_size;
    int32_t result = 0;
    if (v8 <= 0) {
        goto add_to_target;
    }
    v10 = toSystem->m_rigidBodies.begin();
    while (*v10 != entity) {
        ++result;
        ++v10;
        if (result >= v8) {
            goto add_to_target;
        }
    }
    if (result == -1) {
    add_to_target:;
        if (v7 != -1) {
            char v11 = toSystem->m_active.m_bool || entity->isActive().m_bool;
            toSystem->m_active.m_bool = v11;
            toSystem->addRigidBody(static_cast<hkpRigidBody*>(entity));
            return fromSystem->removeRigidBody(v7);
        }
    }
    return result;
}

hkClass* getPhysicsDataClass() {
    return &hkpPhysicsDataClass;
}

hkpPhysicsData::hkpPhysicsData() : m_worldCinfo(nullptr), m_systems() {}

// Drops the world info's and each system's reference; the systems array goes with its member.
hkpPhysicsData::~hkpPhysicsData() {
    if (m_worldCinfo) {
        m_worldCinfo->removeReference();
    }
    for (hkpPhysicsSystem* pSystem : m_systems) {
        pSystem->removeReference();
    }
}

// The first system whose name matches, case-insensitively; systems without a name are skipped.
hkpPhysicsSystem* hkpPhysicsData::findPhysicsSystemByName(const char* name) const {
    for (hkpPhysicsSystem* pSystem : m_systems) {
        const char* pcName = pSystem->m_name.cString();
        if (pcName && !hkString::strCasecmp(pcName, name)) {
            return pSystem;
        }
    }
    return nullptr;
}

hkpRigidBody* hkpPhysicsData::findRigidBodyByName(const char* name) const {
    for (int32_t v2 = 0; v2 < m_systems.m_size; ++v2) {
        hkpPhysicsSystem* v3 = m_systems.m_data[v2];
        for (int32_t v4 = 0; v4 < v3->m_rigidBodies.m_size; ++v4) {
            hkpRigidBody* v5 = v3->m_rigidBodies.m_data[v4];
            if ((reinterpret_cast<uintptr_t>(v5->m_name.m_stringAndFlag) & ~static_cast<uintptr_t>(1)) != 0 &&
                !hkString::strCasecmp(
                    reinterpret_cast<const char*>(
                        reinterpret_cast<uintptr_t>(v5->m_name.m_stringAndFlag) & ~static_cast<uintptr_t>(1)
                    ),
                    name
                )) {
                return v5;
            }
        }
    }
    return nullptr;
}

void hkpPhysicsData::populateFromWorld(const hkpWorld* world, int32_t saveContactPoints) {
    if (!m_worldCinfo) {
        m_worldCinfo = new hkpWorldCinfo;
    }
    world->getCinfo(*m_worldCinfo);
    hkArray<hkpPhysicsSystem*, hkContainerHeapAllocator>* p_m_systems = &m_systems;
    world->getWorldAsSystems(m_systems);
    if (static_cast<uint8_t>(saveContactPoints)) {
        hkpSaveContactPointsUtil::SavePointsInput input;
        input.m_useEntityIds = false;
        input.m_getIdForEntity = nullptr;
        hkpPhysicsSystemWithContacts* v8 = new hkpPhysicsSystemWithContacts;
        hkpSaveContactPointsUtil::saveContactPoints(input, world, v8);
        v8->m_active.m_bool = 0;
        p_m_systems->pushBack(v8);
    }
}

hkpWorld* hkpPhysicsData::createWorld(hkBool registerAllAgents) {
    hkpWorldCinfo info;
    hkpWorld* v7 = new hkpWorld(m_worldCinfo ? *m_worldCinfo : info, 0x111D4u);
    if (registerAllAgents.m_bool) {
        hkpAgentRegisterUtil::registerAllAgents(v7->m_collisionDispatcher);
    }
    for (int32_t i = 0; i < m_systems.m_size; ++i) {
        v7->addPhysicsSystem(m_systems.m_data[i]);
        hkpPhysicsSystem* v10 = m_systems.m_data[i];
        if (v10->hasContacts().m_bool) {
            hkpSaveContactPointsUtil::LoadPointsInput input;
            hkpPhysicsSystem** m_data = m_systems.begin();
            input.m_removeSerializedAgentsWhenLoaded = false;
            input.m_zeroUserDataInContactPointProperties = false;
            input.m_getEntityFromId = nullptr;
            input.m_fireContactPointAddedCallbacks = true;
            input.m_fireContactPointCallbacks = true;
            input.m_passCollidablePointersInCollisionCallbacks = false;
            hkpSaveContactPointsUtil::loadContactPoints(
                input,
                static_cast<hkpPhysicsSystemWithContacts*>(m_data[i]),
                v7
            );
        }
    }
    hkpWorldCinfo* v12 = this->m_worldCinfo;
    if (v12 && v12->m_collisionFilter) {
        v7->updateCollisionFilterOnWorld(
            HK_UPDATE_FILTER_ON_WORLD_FULL_CHECK,
            HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS
        );
    }

    return v7;
}

// Sorts the input system's rigid bodies, phantoms, constraints and actions into the output's unconstrained fixed,
// keyframed and moving bodies, its phantoms and its constrained systems. A working copy of the input is emptied body by
// body: each body grows a system with every constraint and action that reaches it (and the bodies those reach); one
// with neither gives its body to the unconstrained system of its motion type. Each new system is allocated as
// hkReferencedObject's operator new does.
void hkpPhysicsData::splitPhysicsSystems(
    const hkpPhysicsSystem* inputSystemConst,
    hkpPhysicsData::SplitPhysicsSystemsOutput& output
) {
    hkpPhysicsSystem* inputSystem = new hkpPhysicsSystem;
    for (hkpRigidBody* body : inputSystemConst->m_rigidBodies) {
        inputSystem->addRigidBody(body);
    }
    for (hkpAction* action : inputSystemConst->m_actions) {
        inputSystem->addAction(action);
    }
    for (hkpConstraintInstance* constraint : inputSystemConst->m_constraints) {
        inputSystem->addConstraint(constraint);
    }
    for (hkpPhantom* phantom : inputSystemConst->m_phantoms) {
        inputSystem->addPhantom(phantom);
    }

    hkpPhysicsSystem* movingSystem = new hkpPhysicsSystem;
    movingSystem->m_name = "Unconstrained Rigid Bodies";
    movingSystem->m_userData = inputSystem->m_userData;
    movingSystem->m_active.m_bool = false;
    hkpPhysicsSystem* fixedSystem = new hkpPhysicsSystem;
    fixedSystem->m_name = "Fixed Rigid Bodies";
    fixedSystem->m_userData = inputSystem->m_userData;
    fixedSystem->m_active.m_bool = false;
    hkpPhysicsSystem* keyframedSystem = new hkpPhysicsSystem;
    keyframedSystem->m_name = "Keyframed Rigid Bodies";
    keyframedSystem->m_userData = inputSystem->m_userData;
    keyframedSystem->m_active.m_bool = false;

    while (inputSystem->m_rigidBodies.m_size > 0) {
        hkpPhysicsSystem* currentSystem = new hkpPhysicsSystem;
        currentSystem->m_name = "Constrained System";
        currentSystem->m_userData = inputSystem->m_userData;
        currentSystem->m_active = inputSystem->m_rigidBodies.m_data[0]->isActive();
        currentSystem->addRigidBody(inputSystem->m_rigidBodies.m_data[0]);
        inputSystem->removeRigidBody(0);

        bool bSomethingAdded;
        do {
            bSomethingAdded = false;
            for (int32_t rb = 0; rb < currentSystem->m_rigidBodies.m_size; ++rb) {
                hkpRigidBody* body = currentSystem->m_rigidBodies.m_data[rb];
                for (int32_t c = 0; c < inputSystem->m_constraints.m_size;) {
                    hkpConstraintInstance* constraint = inputSystem->m_constraints.m_data[c];
                    if (constraint->getEntityA() == body || constraint->getEntityB() == body) {
                        bSomethingAdded = true;
                        currentSystem->addConstraint(constraint);
                        hkpEntity* other = inputSystem->m_constraints.m_data[c]->getOtherEntity(body);
                        if (other && !other->isFixed().m_bool) {
                            tryMoveEntity(inputSystem, other, currentSystem);
                        }
                        inputSystem->removeConstraint(c);
                    }
                    else {
                        ++c;
                    }
                }
                for (int32_t a = 0; a < inputSystem->m_actions.m_size;) {
                    hkArray<hkpEntity*> entities;
                    inputSystem->m_actions.m_data[a]->getEntities(entities);
                    bool bRemoved = false;
                    for (int32_t e = 0; e < entities.m_size; ++e) {
                        if (entities.m_data[e] == body) {
                            bSomethingAdded = true;
                            currentSystem->addAction(inputSystem->m_actions.m_data[a]);
                            inputSystem->removeAction(a);
                            bRemoved = true;
                            for (int32_t e2 = 0; e2 < entities.m_size; ++e2) {
                                hkpEntity* entity = entities.m_data[e2];
                                if (entity != body && !entity->isFixed().m_bool) {
                                    tryMoveEntity(inputSystem, entity, currentSystem);
                                }
                            }
                        }
                    }
                    if (!bRemoved) {
                        ++a;
                    }
                }
            }
        }
        while (bSomethingAdded);

        if (currentSystem->m_constraints.m_size || currentSystem->m_actions.m_size) {
            output.m_constrainedSystems.pushBack(currentSystem);
        }
        else {
            hkpRigidBody* rb = currentSystem->m_rigidBodies.m_data[0];
            const bool bActive = !rb->getSimulationIsland() || rb->isActive().m_bool;
            hkpPhysicsSystem* targetSystem;
            const uint8_t ucMotionType = rb->m_motion.m_type.m_storage;
            if (ucMotionType == hkpMotion::MOTION_KEYFRAMED) {
                keyframedSystem->m_active.m_bool = keyframedSystem->m_active.m_bool || bActive;
                targetSystem = keyframedSystem;
            }
            else if (ucMotionType == hkpMotion::MOTION_FIXED) {
                targetSystem = fixedSystem;
            }
            else {
                movingSystem->m_active.m_bool = movingSystem->m_active.m_bool || bActive;
                targetSystem = movingSystem;
            }
            targetSystem->addRigidBody(rb);
            delete currentSystem;
        }
    }

    if (keyframedSystem->m_rigidBodies.m_size) {
        output.m_unconstrainedKeyframedBodies = keyframedSystem;
    }
    else {
        delete keyframedSystem;
        output.m_unconstrainedKeyframedBodies = nullptr;
    }
    if (fixedSystem->m_rigidBodies.m_size) {
        output.m_unconstrainedFixedBodies = fixedSystem;
    }
    else {
        delete fixedSystem;
        output.m_unconstrainedFixedBodies = nullptr;
    }
    if (movingSystem->m_rigidBodies.m_size) {
        output.m_unconstrainedMovingBodies = movingSystem;
    }
    else {
        delete movingSystem;
        output.m_unconstrainedMovingBodies = nullptr;
    }
    if (inputSystem->m_phantoms.m_size > 0) {
        hkpPhysicsSystem* phantomSystem = new hkpPhysicsSystem;
        phantomSystem->m_name = "Phantoms";
        phantomSystem->m_userData = inputSystem->m_userData;
        output.m_phantoms = phantomSystem;
        for (hkpPhantom* phantom : inputSystem->m_phantoms) {
            phantomSystem->addPhantom(phantom);
        }
    }
    else {
        output.m_phantoms = nullptr;
    }
    delete inputSystem;
}

hkBool hkpPhysicsSystemWithContacts::hasContacts() {
    return true;
}
