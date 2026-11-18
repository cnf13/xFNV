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
static int32_t tryMoveEntity(hkpPhysicsSystem* a1, hkpEntity* a2, hkpPhysicsSystem* r_esi);

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
    "hkpPhysicsData",          finishLoadedObjecthkpPhysicsData, cleanupLoadedObjecthkpPhysicsData,
    getVtablehkpPhysicsData(), &typeid(hkpPhysicsData),          sizeof(hkpPhysicsData)
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

static int32_t tryMoveEntity(hkpPhysicsSystem* a1, hkpEntity* a2, hkpPhysicsSystem* r_esi) {
    int32_t v7;
    hkpRigidBody** v10;
    int32_t m_size = a1->m_rigidBodies.m_size;
    int32_t v5 = 0;
    if (m_size <= 0) {
    LABEL_5:;
        v7 = -1;
    }
    else {
        hkpRigidBody** m_data = a1->m_rigidBodies.begin();
        while (*m_data != a2) {
            ++v5;
            ++m_data;
            if (v5 >= m_size) {
                goto LABEL_5;
            }
        }
        v7 = v5;
    }
    int32_t v8 = r_esi->m_rigidBodies.m_size;
    int32_t result = 0;
    if (v8 <= 0) {
        goto LABEL_13;
    }
    v10 = r_esi->m_rigidBodies.begin();
    while (*v10 != a2) {
        ++result;
        ++v10;
        if (result >= v8) {
            goto LABEL_13;
        }
    }
    if (result == -1) {
    LABEL_13:;
        if (v7 != -1) {
            char v11 = r_esi->m_active.m_bool || a2->isActive().m_bool;
            r_esi->m_active.m_bool = v11;
            r_esi->addRigidBody(static_cast<hkpRigidBody*>(a2));
            return a1->removeRigidBody(v7);
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
    for (int32_t i = 0; i < m_systems.m_size; ++i) {
        m_systems.m_data[i]->removeReference();
    }
}

// The first system whose name matches, case-insensitively; systems without a name are skipped.
hkpPhysicsSystem* hkpPhysicsData::findPhysicsSystemByName(const char* name) const {
    for (int32_t i = 0; i < m_systems.m_size; ++i) {
        const char* pcName = m_systems.m_data[i]->m_name.cString();
        if (pcName && !hkString::strCasecmp(pcName, name)) {
            return m_systems.m_data[i];
        }
    }
    return nullptr;
}

hkpRigidBody* hkpPhysicsData::findRigidBodyByName(const char* name) const {
    const hkpPhysicsData* pThis = this;
    hkpPhysicsSystem* v3;
    int32_t v4;
    hkpRigidBody* v5;
    int32_t v2 = 0;
    const hkpPhysicsData* v7 = pThis;
    if (pThis->m_systems.m_size <= 0) {
        return nullptr;
    }
    while (true) {
        v3 = pThis->m_systems.m_data[v2];
        v4 = 0;
        if (v3->m_rigidBodies.m_size > 0) {
            break;
        }
    LABEL_7:;
        if (++v2 >= pThis->m_systems.m_size) {
            return nullptr;
        }
    }
    while (true) {
        v5 = v3->m_rigidBodies.m_data[v4];
        if ((reinterpret_cast<uintptr_t>(v5->m_name.m_stringAndFlag) & ~uintptr_t(1)) != 0) {
            break;
        }
    LABEL_6:;
        if (++v4 >= v3->m_rigidBodies.m_size) {
            goto LABEL_7;
        }
    }
    if (hkString::strCasecmp(
            reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(v5->m_name.m_stringAndFlag) & ~uintptr_t(1)), name
        )) {
        pThis = v7;
        goto LABEL_6;
    }
    return v5;
}

void hkpPhysicsData::populateFromWorld(const hkpWorld* world, int32_t saveContactPoints) {
    if (!m_worldCinfo) {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        hkpWorldCinfo* v5 = static_cast<hkpWorldCinfo*>(Instance->m_heap->blockAlloc(sizeof(hkpWorldCinfo)));
        v5->m_memSizeAndFlags = sizeof(hkpWorldCinfo);
        m_worldCinfo = ::new (v5) hkpWorldCinfo;
    }
    world->getCinfo(*m_worldCinfo);
    hkArray<hkpPhysicsSystem*, hkContainerHeapAllocator>* p_m_systems = &m_systems;
    world->getWorldAsSystems(m_systems);
    if (static_cast<uint8_t>(saveContactPoints)) {
        hkpSaveContactPointsUtil::SavePointsInput input;
        input.m_useEntityIds = false;
        input.m_getIdForEntity = nullptr;
        hkMemoryRouter* v7 = &hkMemoryRouter::getInstance();
        hkpPhysicsSystemWithContacts* v8 =
            static_cast<hkpPhysicsSystemWithContacts*>(v7->m_heap->blockAlloc(sizeof(hkpPhysicsSystemWithContacts)));
        v8->m_memSizeAndFlags = sizeof(hkpPhysicsSystemWithContacts);
        ::new (v8) hkpPhysicsSystemWithContacts;
        hkpSaveContactPointsUtil::saveContactPoints(input, world, v8);
        v8->m_active.m_bool = 0;
        hkMemoryRouter* v9 = &hkMemoryRouter::getInstance();
        if (p_m_systems->m_size == (p_m_systems->m_capacityAndFlags & hkArrayBase<hkpPhysicsSystem*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*v9->m_heap, p_m_systems, sizeof(hkpPhysicsSystem*));
        }
        p_m_systems->m_data[p_m_systems->m_size++] = v8;
    }
}

hkpWorld* hkpPhysicsData::createWorld(hkBool registerAllAgents) {
    hkpWorld* v4;
    hkpWorldCinfo* m_worldCinfo;
    hkBool hasContactsResult;
    hkpWorldCinfo info;
    if (this->m_worldCinfo) {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        v4 = static_cast<hkpWorld*>(Instance->m_heap->blockAlloc(sizeof(hkpWorld)));
        v4->m_memSizeAndFlags = sizeof(hkpWorld);
        m_worldCinfo = this->m_worldCinfo;
    }
    else {
        hkMemoryRouter* v3 = &hkMemoryRouter::getInstance();
        v4 = static_cast<hkpWorld*>(v3->m_heap->blockAlloc(sizeof(hkpWorld)));
        v4->m_memSizeAndFlags = sizeof(hkpWorld);
        m_worldCinfo = &info;
    }
    hkpWorld* v7 = ::new (v4) hkpWorld(*m_worldCinfo, 0x111D4u);
    if (registerAllAgents.m_bool) {
        hkpAgentRegisterUtil::registerAllAgents(v7->m_collisionDispatcher);
    }
    for (int32_t i = 0; i < m_systems.m_size; ++i) {
        v7->addPhysicsSystem(m_systems.m_data[i]);
        hkpPhysicsSystem* v10 = m_systems.m_data[i];
        if (v10->hasContacts(&hasContactsResult)->m_bool) {
            hkpSaveContactPointsUtil::LoadPointsInput input;
            hkpPhysicsSystem** m_data = m_systems.begin();
            input.m_removeSerializedAgentsWhenLoaded = false;
            input.m_zeroUserDataInContactPointProperties = false;
            input.m_getEntityFromId = nullptr;
            input.m_fireContactPointAddedCallbacks = true;
            input.m_fireContactPointCallbacks = true;
            input.m_passCollidablePointersInCollisionCallbacks = false;
            hkpSaveContactPointsUtil::loadContactPoints(
                input, static_cast<hkpPhysicsSystemWithContacts*>(m_data[i]), v7
            );
        }
    }
    hkpWorldCinfo* v12 = this->m_worldCinfo;
    if ((v12) && (v12->m_collisionFilter)) {
        v7->updateCollisionFilterOnWorld(
            HK_UPDATE_FILTER_ON_WORLD_FULL_CHECK, HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS
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
    // EXE-INLINED 0x56D280
    hkpPhysicsSystem* inputSystem =
        static_cast<hkpPhysicsSystem*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpPhysicsSystem)));
    inputSystem->m_memSizeAndFlags = sizeof(hkpPhysicsSystem);
    inputSystem = ::new (inputSystem) hkpPhysicsSystem;
    for (int32_t i = 0; i < inputSystemConst->m_rigidBodies.m_size; ++i) {
        inputSystem->addRigidBody(inputSystemConst->m_rigidBodies.m_data[i]);
    }
    for (int32_t i = 0; i < inputSystemConst->m_actions.m_size; ++i) {
        inputSystem->addAction(inputSystemConst->m_actions.m_data[i]);
    }
    for (int32_t i = 0; i < inputSystemConst->m_constraints.m_size; ++i) {
        inputSystem->addConstraint(inputSystemConst->m_constraints.m_data[i]);
    }
    for (int32_t i = 0; i < inputSystemConst->m_phantoms.m_size; ++i) {
        inputSystem->addPhantom(inputSystemConst->m_phantoms.m_data[i]);
    }

    // EXE-INLINED 0x56D280
    hkpPhysicsSystem* movingSystem =
        static_cast<hkpPhysicsSystem*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpPhysicsSystem)));
    movingSystem->m_memSizeAndFlags = sizeof(hkpPhysicsSystem);
    movingSystem = ::new (movingSystem) hkpPhysicsSystem;
    movingSystem->m_name = "Unconstrained Rigid Bodies";
    movingSystem->m_userData = inputSystem->m_userData;
    movingSystem->m_active.m_bool = false;
    // EXE-INLINED 0x56D280
    hkpPhysicsSystem* fixedSystem =
        static_cast<hkpPhysicsSystem*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpPhysicsSystem)));
    fixedSystem->m_memSizeAndFlags = sizeof(hkpPhysicsSystem);
    fixedSystem = ::new (fixedSystem) hkpPhysicsSystem;
    fixedSystem->m_name = "Fixed Rigid Bodies";
    fixedSystem->m_userData = inputSystem->m_userData;
    fixedSystem->m_active.m_bool = false;
    // EXE-INLINED 0x56D280
    hkpPhysicsSystem* keyframedSystem =
        static_cast<hkpPhysicsSystem*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpPhysicsSystem)));
    keyframedSystem->m_memSizeAndFlags = sizeof(hkpPhysicsSystem);
    keyframedSystem = ::new (keyframedSystem) hkpPhysicsSystem;
    keyframedSystem->m_name = "Keyframed Rigid Bodies";
    keyframedSystem->m_userData = inputSystem->m_userData;
    keyframedSystem->m_active.m_bool = false;

    while (inputSystem->m_rigidBodies.m_size > 0) {
        // EXE-INLINED 0x56D280
        hkpPhysicsSystem* currentSystem =
            static_cast<hkpPhysicsSystem*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpPhysicsSystem)));
        currentSystem->m_memSizeAndFlags = sizeof(hkpPhysicsSystem);
        currentSystem = ::new (currentSystem) hkpPhysicsSystem;
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
                        // EXE-INLINED none: hkpEntity::isFixed
                        if (other && other->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED) {
                            tryMoveEntity(inputSystem, other, currentSystem);
                        }
                        inputSystem->removeConstraint(c);
                    }
                    else {
                        ++c;
                    }
                }
                for (int32_t a = 0; a < inputSystem->m_actions.m_size;) {
                    // EXE-INLINED none: hkArray<hkpEntity*>::hkArray
                    XFNV_INLINED_LOCAL(hkArray<hkpEntity*>, entities, std::nullopt, std::nullopt);
                    entities.m_data = nullptr;
                    entities.m_size = 0;
                    entities.m_capacityAndFlags = hkArrayBase<hkpEntity*>::DONT_DEALLOCATE_FLAG;
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
                                // EXE-INLINED none: hkpEntity::isFixed
                                if (entity != body && entity->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED) {
                                    tryMoveEntity(inputSystem, entity, currentSystem);
                                }
                            }
                        }
                    }
                    if (!bRemoved) {
                        ++a;
                    }
                    // EXE-INLINED none: hkArray<hkpEntity*>::~hkArray
                    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
                    entities.m_size = 0;
                    if (entities.m_capacityAndFlags >= 0) {
                        pHeap->bufFree(
                            entities.m_data,
                            static_cast<int32_t>(
                                (entities.m_capacityAndFlags & hkArrayBase<hkpEntity*>::CAPACITY_MASK) *
                                sizeof(hkpEntity*)
                            )
                        );
                    }
                    entities.m_data = nullptr;
                    entities.m_capacityAndFlags = hkArrayBase<hkpEntity*>::DONT_DEALLOCATE_FLAG;
                }
            }
        }
        while (bSomethingAdded);

        if (currentSystem->m_constraints.m_size || currentSystem->m_actions.m_size) {
            // EXE-INLINED none: hkArray<hkpPhysicsSystem*>::pushBack
            hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
            if (output.m_constrainedSystems.m_size ==
                (output.m_constrainedSystems.m_capacityAndFlags & hkArrayBase<hkpPhysicsSystem*>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(
                    *pRouter->m_heap, &output.m_constrainedSystems, static_cast<int32_t>(sizeof(hkpPhysicsSystem*))
                );
            }
            output.m_constrainedSystems.m_data[output.m_constrainedSystems.m_size++] = currentSystem;
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
        // EXE-INLINED 0x56D280
        hkpPhysicsSystem* phantomSystem =
            static_cast<hkpPhysicsSystem*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpPhysicsSystem)));
        phantomSystem->m_memSizeAndFlags = sizeof(hkpPhysicsSystem);
        phantomSystem = ::new (phantomSystem) hkpPhysicsSystem;
        phantomSystem->m_name = "Phantoms";
        phantomSystem->m_userData = inputSystem->m_userData;
        output.m_phantoms = phantomSystem;
        for (int32_t i = 0; i < inputSystem->m_phantoms.m_size; ++i) {
            phantomSystem->addPhantom(inputSystem->m_phantoms.m_data[i]);
        }
    }
    else {
        output.m_phantoms = nullptr;
    }
    delete inputSystem;
}

hkBool* hkpPhysicsSystemWithContacts::hasContacts(hkBool* arResult) {
    arResult->m_bool = true;
    return arResult;
}
