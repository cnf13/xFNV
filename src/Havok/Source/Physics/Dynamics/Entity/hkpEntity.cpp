#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"

#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/Array/hkSmallArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkSmallArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/Util/hkStatisticsCollectorUtil.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Geometry/LocalFrame/hkLocalFrame.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnEntry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpDynamicsContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Common/hkpMaterial.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/Util/hkpEntityAabbUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityActivationListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityClass.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/ThinBoxMotion/hkpThinBoxMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpBoxMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpCharacterMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpFixedRigidMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpSphereMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationQueue.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/Destruction/BreakOffParts/hkpBreakOffPartsUtil.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <xmmintrin.h>

namespace {

    // The SDK's comparator of an entity's slave constraints: by constraint uid, then by the uid of the entity at the
    // constraint's other end.
    class cmpLessConstraint {
    public:
        explicit cmpLessConstraint(hkpEntity* thisEntity) : m_thisEntity(thisEntity) {}

        bool operator()(const hkpConstraintInstance* a, const hkpConstraintInstance* b) const {
            if (a->m_uid < b->m_uid) {
                return true;
            }
            if (a->m_uid > b->m_uid) {
                return false;
            }
            return a->getOtherEntity(m_thisEntity)->getUid() < b->getOtherEntity(m_thisEntity)->getUid();
        }

        hkpEntity* m_thisEntity;
    };

} // namespace

hkpEntity::ExtendedListeners::ExtendedListeners() {}

hkpEntity::ExtendedListeners::~ExtendedListeners() {}

// The SDK's finish constructor: hkpWorldObject's, the entity's arrays emptied, its motion as an hkpMaxSizeMotion,
// then, when finishing, the finish constructor of the motion type the entity really holds.
hkpEntity::hkpEntity(hkFinishLoadedObjectFlag aFlag) : hkpWorldObject(aFlag), m_motion(aFlag) {
    if (aFlag.m_finishing) {
        // The SDK's placement news on m_motion.
        switch (m_motion.m_type.m_storage) {
            case hkpMotion::MOTION_SPHERE_INERTIA: new (&m_motion) hkpSphereMotion(aFlag.m_finishing); break;
            case hkpMotion::MOTION_BOX_INERTIA: new (&m_motion) hkpBoxMotion(aFlag.m_finishing); break;
            case hkpMotion::MOTION_KEYFRAMED: new (&m_motion) hkpKeyframedRigidMotion(aFlag); break;
            case hkpMotion::MOTION_FIXED: new (&m_motion) hkpFixedRigidMotion(aFlag.m_finishing); break;
            case hkpMotion::MOTION_THIN_BOX_INERTIA: new (&m_motion) hkpThinBoxMotion(aFlag); break;
            case hkpMotion::MOTION_CHARACTER: new (&m_motion) hkpCharacterMotion(aFlag.m_finishing); break;
            default: break;
        }
    }
}

hkpEntity::hkpEntity(const hkpShape* shape)
  : hkpWorldObject(shape, BROAD_PHASE_ENTITY), m_limitContactImpulseUtil(nullptr), m_damageMultiplier(1.0f),
    m_breakableBody(nullptr), m_solverData(0), m_storageIndex(0xFFFF), m_simulationIsland(nullptr),
    m_numShapeKeysInContactPointProperties(0), m_uid(0xFFFFFFFF),
    m_spuCollisionCallback{nullptr, 0, SPU_SEND_CONTACT_POINT_ADDED_OR_PROCESS, 1}, m_localFrame(nullptr),
    m_extendedListeners(nullptr) {
    m_material.m_friction = 0.5f;
    m_material.m_restitution = 0.4f;
}

// The SDK's destructor: the limit contact impulse util is removed and deleted, the entity listeners are told, the
// extended listeners and the local frame's reference go; the compiler destroys the members, then hkpWorldObject.
hkpEntity::~hkpEntity() {
    // The SDK keeps the util in a void* member.
    hkpBreakOffPartsUtil::LimitContactImpulseUtil*
        limitUtil = static_cast<hkpBreakOffPartsUtil::LimitContactImpulseUtil*>(m_limitContactImpulseUtil);
    if (limitUtil) {
        removeContactListener(limitUtil);
        delete limitUtil;
        m_limitContactImpulseUtil = nullptr;
    }
    hkpEntityCallbackUtil::fireEntityDeleted(this);
    delete m_extendedListeners;
    if (m_localFrame) {
        m_localFrame->removeReference();
    }
    m_localFrame = nullptr;
}

void hkpEntity::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    collector->addChunk(
        hkStatisticsCollector::MEM_ARRAY,
        "constraintsSlave",
        m_constraintsSlave.begin(),
        static_cast<int32_t>(sizeof(*m_constraintsSlave.begin())) * m_constraintsSlave.m_size,
        static_cast<int32_t>(sizeof(*m_constraintsSlave.begin())) * m_constraintsSlave.m_capacityAndFlags
    );
    hkStatisticsCollectorUtil::addClassContents(this, hkpEntityClass, collector);
    if ((m_contactListeners.m_capacityAndFlags & hkSmallArray<hkpContactListener*>::DONT_DEALLOCATE_FLAG) == 0 ||
        collector->m_includeUnmanagedArrayMemory) {
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "CollisionListnr",
            m_contactListeners.begin(),
            static_cast<int32_t>(sizeof(*m_contactListeners.begin())) * m_contactListeners.m_size,
            static_cast<int32_t>(sizeof(*m_contactListeners.begin())) *
                (m_contactListeners.m_capacityAndFlags & hkSmallArray<hkpContactListener*>::CAPACITY_MASK)
        );
    }
    hkpEntity::ExtendedListeners* m_extendedListeners = this->m_extendedListeners;
    if (m_extendedListeners) {
        if ((m_extendedListeners->m_entityListeners.m_capacityAndFlags &
             hkSmallArray<hkpEntityListener*>::DONT_DEALLOCATE_FLAG) == 0 ||
            collector->m_includeUnmanagedArrayMemory) {
            collector->addChunk(
                hkStatisticsCollector::MEM_ARRAY,
                "EntityListeners",
                m_extendedListeners->m_entityListeners.begin(),
                static_cast<int32_t>(sizeof(*m_extendedListeners->m_entityListeners.begin())) *
                    m_extendedListeners->m_entityListeners.m_size,
                static_cast<int32_t>(sizeof(*m_extendedListeners->m_entityListeners.begin())) *
                    (m_extendedListeners->m_entityListeners.m_capacityAndFlags &
                     hkSmallArray<hkpEntityListener*>::CAPACITY_MASK)
            );
        }
        hkpEntity::ExtendedListeners* v5 = this->m_extendedListeners;
        if ((v5->m_activationListeners.m_capacityAndFlags &
             hkSmallArray<hkpEntityActivationListener*>::DONT_DEALLOCATE_FLAG) == 0 ||
            collector->m_includeUnmanagedArrayMemory) {
            collector->addChunk(
                hkStatisticsCollector::MEM_ARRAY,
                "ActivatonListners",
                v5->m_activationListeners.begin(),
                static_cast<int32_t>(sizeof(*v5->m_activationListeners.begin())) * v5->m_activationListeners.m_size,
                static_cast<int32_t>(sizeof(*v5->m_activationListeners.begin())) *
                    (v5->m_activationListeners.m_capacityAndFlags &
                     hkSmallArray<hkpEntityActivationListener*>::CAPACITY_MASK)
            );
        }
    }
    collector->addReferencedObject("SavedMotion", m_motion.m_savedMotion, 0);
    hkpWorldObject::calcContentStatistics(collector, &hkpWorldObjectClass);
}

// The SDK's deallocateInternalArrays: the limit contact impulse util is removed and deleted, then every array of the
// entity that is empty gives its storage back; the listener arrays when all their entries are null, and the extended
// listeners when both of theirs did.
void hkpEntity::deallocateInternalArrays() {
    // The SDK keeps the util in a void* member.
    hkpBreakOffPartsUtil::LimitContactImpulseUtil*
        limitUtil = static_cast<hkpBreakOffPartsUtil::LimitContactImpulseUtil*>(m_limitContactImpulseUtil);
    if (limitUtil) {
        removeContactListener(limitUtil);
        delete limitUtil;
        m_limitContactImpulseUtil = nullptr;
    }
    if (m_collidable.m_collisionEntries.m_size == 0) {
        m_collidable.m_collisionEntries.clearAndDeallocate();
    }
    if (m_constraintsMaster.m_size == 0) {
        m_constraintsMaster.clearAndDeallocate();
    }
    if (m_constraintsSlave.m_size == 0) {
        m_constraintsSlave.clearAndDeallocate();
    }
    if (m_constraintRuntime.m_size == 0) {
        m_constraintRuntime.clearAndDeallocate();
    }
    // Removed listeners leave null entries: an array is freed only when all of them are null.
    bool contactListenersLeft = false;
    for (hkpContactListener* listener : m_contactListeners) {
        if (listener) {
            contactListenersLeft = true;
            break;
        }
    }
    if (!contactListenersLeft) {
        m_contactListeners.clearAndDeallocate();
    }
    ExtendedListeners* extendedListeners = m_extendedListeners;
    bool activationListenersLeft = false;
    if (extendedListeners) {
        for (hkpEntityActivationListener* listener : extendedListeners->m_activationListeners) {
            if (listener) {
                activationListenersLeft = true;
                break;
            }
        }
        if (!activationListenersLeft) {
            extendedListeners->m_activationListeners.clearAndDeallocate();
        }
    }
    extendedListeners = m_extendedListeners;
    bool entityListenersLeft = false;
    if (extendedListeners) {
        for (hkpEntityListener* listener : extendedListeners->m_entityListeners) {
            if (listener) {
                entityListenersLeft = true;
                break;
            }
        }
        if (!entityListenersLeft) {
            extendedListeners->m_entityListeners.clearAndDeallocate();
        }
    }
    if (!activationListenersLeft && !entityListenersLeft) {
        extendedListeners = m_extendedListeners;
        if (extendedListeners) {
            delete extendedListeners;
        }
        m_extendedListeners = nullptr;
    }
    if (m_actions.m_size == 0) {
        m_actions.clearAndDeallocate();
    }
}

void hkpEntity::addEntityListener(hkpEntityListener* el) {
    hkpEntityListener** m_data;
    if (!this->m_extendedListeners) {
        hkpEntity::ExtendedListeners* v4 = static_cast<hkpEntity::ExtendedListeners*>(
            hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpEntity::ExtendedListeners))
        );
        if (v4) {
            v4->m_activationListeners.m_data = nullptr;
            v4->m_activationListeners.m_size = 0;
            v4->m_activationListeners
                .m_capacityAndFlags = hkSmallArray<hkpEntityActivationListener*>::DONT_DEALLOCATE_FLAG;
            v4->m_entityListeners.m_data = nullptr;
            v4->m_entityListeners.m_size = 0;
            v4->m_entityListeners.m_capacityAndFlags = hkSmallArray<hkpEntityListener*>::DONT_DEALLOCATE_FLAG;
        }
        else {
            v4 = nullptr;
        }
        this->m_extendedListeners = v4;
    }
    hkpEntity::ExtendedListeners* m_extendedListeners = this->m_extendedListeners;
    uint16_t m_size = m_extendedListeners->m_entityListeners.m_size;
    hkSmallArray<hkpEntityListener*>* p_m_entityListeners = &m_extendedListeners->m_entityListeners;
    int32_t v8 = 0;
    if (m_size) {
        m_data = p_m_entityListeners->begin();
        while (*m_data) {
            ++v8;
            ++m_data;
            if (v8 >= p_m_entityListeners->m_size) {
                v8 = -1;
                break;
            }
        }
    }
    else {
        v8 = -1;
    }
    if (v8 >= 0) {
        p_m_entityListeners->m_data[v8] = el;
    }
    else {
        if (m_size == (p_m_entityListeners->m_capacityAndFlags & hkSmallArray<hkpEntityListener*>::CAPACITY_MASK)) {
            hkSmallArrayUtil::_reserveMore(p_m_entityListeners, sizeof(hkpEntityListener*));
        }
        p_m_entityListeners->m_data[p_m_entityListeners->m_size++] = el;
    }
}

hkpEntity::ExtendedListeners* hkpEntity::getExtendedListeners() {
    if (!m_extendedListeners) {
        m_extendedListeners = new hkpEntity::ExtendedListeners;
    }
    return m_extendedListeners;
}

void hkpEntity::activate() {
    hkpSimulationIsland* m_simulationIsland;
    hkpWorld* m_world;
    m_simulationIsland = this->m_simulationIsland;
    if (((!m_simulationIsland) || (m_simulationIsland->m_activeMark == 0)) && (m_motion.m_type.m_storage != 5)) {
        m_world = this->m_world;
        if (m_world) {
            m_motion.m_deactivationNumInactiveFrames[0] = 0;
            m_motion.m_deactivationNumInactiveFrames[1] = 0;
            hkpWorldOperationUtil::markIslandActive(m_world, m_simulationIsland);
            m_motion.setWorldSelectFlagsNeg(
                this->m_world->m_dynamicsStepInfo.m_solverInfo.m_deactivationNumInactiveFramesSelectFlag[0],
                this->m_world->m_dynamicsStepInfo.m_solverInfo.m_deactivationNumInactiveFramesSelectFlag[1],
                this->m_world->m_dynamicsStepInfo.m_solverInfo.m_deactivationIntegrateCounter
            );
        }
    }
}

void hkpEntity::addEntityActivationListener(hkpEntityActivationListener* el) {
    hkpEntityActivationListener** m_data;
    if (!this->m_extendedListeners) {
        hkpEntity::ExtendedListeners* v4 = static_cast<hkpEntity::ExtendedListeners*>(
            hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpEntity::ExtendedListeners))
        );
        if (v4) {
            v4->m_activationListeners.m_data = nullptr;
            v4->m_activationListeners.m_size = 0;
            v4->m_activationListeners
                .m_capacityAndFlags = hkSmallArray<hkpEntityActivationListener*>::DONT_DEALLOCATE_FLAG;
            v4->m_entityListeners.m_data = nullptr;
            v4->m_entityListeners.m_size = 0;
            v4->m_entityListeners.m_capacityAndFlags = hkSmallArray<hkpEntityListener*>::DONT_DEALLOCATE_FLAG;
        }
        else {
            v4 = nullptr;
        }
        this->m_extendedListeners = v4;
    }
    hkpEntity::ExtendedListeners* m_extendedListeners = this->m_extendedListeners;
    uint16_t m_size = m_extendedListeners->m_activationListeners.m_size;
    int32_t v7 = 0;
    if (m_size) {
        m_data = m_extendedListeners->m_activationListeners.begin();
        while (*m_data) {
            ++v7;
            ++m_data;
            if (v7 >= m_extendedListeners->m_activationListeners.m_size) {
                v7 = -1;
                break;
            }
        }
    }
    else {
        v7 = -1;
    }
    if (v7 >= 0) {
        m_extendedListeners->m_activationListeners.m_data[v7] = el;
    }
    else {
        if (m_size ==
            (m_extendedListeners->m_activationListeners.m_capacityAndFlags &
             hkSmallArray<hkpEntityActivationListener*>::CAPACITY_MASK)) {
            hkSmallArrayUtil::_reserveMore(m_extendedListeners, sizeof(hkpEntityActivationListener*));
        }
        m_extendedListeners->m_activationListeners.m_data[m_extendedListeners->m_activationListeners.m_size++] = el;
    }
}

void hkpEntity::addContactListener(hkpContactListener* cl) {
    hkpContactListener** m_data;
    hkSmallArray<hkpContactListener*>* p_m_contactListeners = &m_contactListeners;
    int32_t m_size = m_contactListeners.m_size;
    int32_t v4 = 0;
    if (m_size) {
        m_data = p_m_contactListeners->begin();
        while (*m_data) {
            ++v4;
            ++m_data;
            if (v4 >= m_size) {
                v4 = -1;
                break;
            }
        }
    }
    else {
        v4 = -1;
    }
    if (v4 >= 0) {
        p_m_contactListeners->m_data[v4] = cl;
    }
    else {
        if (p_m_contactListeners->m_size ==
            (p_m_contactListeners->m_capacityAndFlags & hkSmallArray<hkpContactListener*>::CAPACITY_MASK)) {
            hkSmallArrayUtil::_reserveMore(p_m_contactListeners, sizeof(hkpContactListener*));
        }
        p_m_contactListeners->m_data[p_m_contactListeners->m_size++] = cl;
    }
}

uint8_t* hkpEntity::setMaxLinearVelocity(float afMaxLinearVelocity) {
    hkpMotion* RagdollList = getMotion();
    hkMotionState* FollowUpInfoList = &RagdollList->m_motionState;
    return &(FollowUpInfoList->m_maxLinearVelocity = afMaxLinearVelocity).m_value;
}

void hkpEntity::removeEntityListener(hkpEntityListener* el) {
    if (!this->m_extendedListeners) {
        hkpEntity::ExtendedListeners* v4 = static_cast<hkpEntity::ExtendedListeners*>(
            hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpEntity::ExtendedListeners))
        );
        if (v4) {
            v4->m_activationListeners.m_data = nullptr;
            v4->m_activationListeners.m_size = 0;
            v4->m_activationListeners
                .m_capacityAndFlags = hkSmallArray<hkpEntityActivationListener*>::DONT_DEALLOCATE_FLAG;
            v4->m_entityListeners.m_data = nullptr;
            v4->m_entityListeners.m_size = 0;
            v4->m_entityListeners.m_capacityAndFlags = hkSmallArray<hkpEntityListener*>::DONT_DEALLOCATE_FLAG;
        }
        else {
            v4 = nullptr;
        }
        this->m_extendedListeners = v4;
    }
    hkpEntity::ExtendedListeners* m_extendedListeners = this->m_extendedListeners;
    int32_t m_size = m_extendedListeners->m_entityListeners.m_size;
    hkpEntityListener*** p_m_data = &m_extendedListeners->m_entityListeners.m_data;
    int32_t v8 = 0;
    if (m_size) {
        hkpEntityListener** v9 = *p_m_data;
        while (*v9 != el) {
            ++v8;
            ++v9;
            if (v8 >= m_size) {
                goto clear_before_buffer;
            }
        }
        (*p_m_data)[v8] = nullptr;
    }
    else {
    clear_before_buffer:;
        *(*p_m_data - 1) = nullptr;
    }
}

// A listener not in the array gives index -1, and the entry just before the buffer is cleared.
void hkpEntity::removeEntityActivationListener(hkpEntityActivationListener* el) {
    hkpEntity::ExtendedListeners* m_extendedListeners = this->m_extendedListeners;
    int32_t v3 = 0;
    if (m_extendedListeners->m_activationListeners.m_size) {
        hkpEntityActivationListener** m_data = m_extendedListeners->m_activationListeners.begin();
        while (*m_data != el) {
            ++v3;
            ++m_data;
            if (v3 >= m_extendedListeners->m_activationListeners.m_size) {
                v3 = -1;
                break;
            }
        }
    }
    else {
        v3 = -1;
    }
    m_extendedListeners->m_activationListeners.m_data[v3] = 0;
}

// When the listener is not found, the entry at index -1 (the indexOf result) is cleared.
void hkpEntity::removeContactListener(hkpContactListener* cl) {
    int32_t v2 = 0;
    if (m_contactListeners.m_size) {
        hkpContactListener** m_data = m_contactListeners.begin();
        while (*m_data != cl) {
            ++v2;
            ++m_data;
            if (v2 >= m_contactListeners.m_size) {
                goto clear_before_buffer;
            }
        }
        m_contactListeners.m_data[v2] = 0;
    }
    else {
    clear_before_buffer:;
        m_contactListeners.begin()[-1] = 0;
    }
}

void hkpEntity::deactivate() {
    hkpSimulationIsland* m_simulationIsland = this->getSimulationIsland();
    if (m_simulationIsland && (m_simulationIsland->m_activeMark != 0)) {
        if (m_simulationIsland->m_splitCheckRequested != 0 || m_simulationIsland->m_sparseEnabled != 0) {
            hkpWorld* m_world = this->m_world;
            uint32_t m_minDesiredIslandSize = m_world->m_minDesiredIslandSize;
            m_world->m_minDesiredIslandSize = 0;
            this->getSimulationIsland()->m_sparseEnabled = 0;
            this->getSimulationIsland()->m_splitCheckRequested = 1;
            hkpWorldOperationUtil::splitSimulationIsland(this->m_world, this->getSimulationIsland());
            this->m_world->m_minDesiredIslandSize = m_minDesiredIslandSize;
        }
        hkpSimulationIsland* v5 = this->getSimulationIsland();
        int32_t v6 = 0;
        if (v5->m_entities.m_size > 0) {
            hkpEntity** m_data = v5->m_entities.begin();
            do {
                hkpEntity* v8 = *m_data;
                XFNV_M128_F32(v8->m_motion.m_deactivationRefPosition[0].m_quad, 3) = HK_REAL_MAX;
                ++v6;
                XFNV_M128_F32(v8->m_motion.m_deactivationRefPosition[1].m_quad, 3) = HK_REAL_MAX;
                ++m_data;
            }
            while (v6 < v5->m_entities.m_size);
        }
        hkpWorldOperationUtil::markIslandInactive(this->m_world, v5);
    }
}

// While the world's critical operations are locked, the activation is queued; otherwise it runs now.
void hkpEntity::activateAsCriticalOperation() {
    hkpWorld* pWorld = m_world;
    if (pWorld && pWorld->m_criticalOperationsLockCount) {
        hkWorldOperation::ActivateEntity kOperation;
        kOperation.m_type.m_storage = hkWorldOperation::ACTIVATE_ENTITY;
        kOperation.m_entity = this;
        pWorld->queueOperation(kOperation);
        return;
    }
    activate();
}

// While the world's critical operations are locked, the deactivation is queued; otherwise it runs now.
void hkpEntity::deactivateAsCriticalOperation() {
    hkpWorld* pWorld = m_world;
    if (pWorld && pWorld->m_criticalOperationsLockCount) {
        hkWorldOperation::DeactivateEntity kOperation;
        kOperation.m_type.m_storage = hkWorldOperation::DEACTIVATE_ENTITY;
        kOperation.m_entity = this;
        pWorld->queueOperation(kOperation);
        return;
    }
    deactivate();
}

int32_t hkpEntity::getNumConstraints() const {
    return m_constraintsSlave.m_size + m_constraintsMaster.m_size;
}

hkpConstraintInstance* hkpEntity::getConstraint(int32_t i) {
    int32_t numMaster = m_constraintsMaster.m_size;
    if (i < numMaster) {
        return m_constraintsMaster.m_data[i].m_constraint;
    }
    return m_constraintsSlave.m_data[i - numMaster];
}

const hkpConstraintInstance* hkpEntity::getConstraint(int32_t i) const {
    int32_t numMaster = m_constraintsMaster.m_size;
    if (i < numMaster) {
        return m_constraintsMaster.m_data[i].m_constraint;
    }
    return m_constraintsSlave.m_data[i - numMaster];
}

void hkpEntity::getAllConstraints(hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& constraints) {
    int32_t NumConstraints = getNumConstraints();
    constraints.setSize(NumConstraints);
    int32_t v7 = 0;
    int32_t v8 = 0;
    int32_t v9 = 0;
    if (m_constraintsMaster.m_size) {
        do {
            constraints.m_data[v8] = m_constraintsMaster.m_data[v7].m_constraint;
            ++v9;
            ++v8;
            ++v7;
        }
        while (v9 < m_constraintsMaster.m_size);
    }
    int32_t v10 = 0;
    if (m_constraintsSlave.m_size > 0) {
        int32_t v11 = v8;
        do {
            constraints.m_data[v11++] = m_constraintsSlave.m_data[v10++];
        }
        while (v10 < m_constraintsSlave.m_size);
    }
}

// The SDK's sortConstraintsSlavesDeterministically: a fixed entity sorts its slave constraints, then each constraint's
// master entry (on the entity at the constraint's other end) gets the constraint's new slave index.
void hkpEntity::sortConstraintsSlavesDeterministically() {
    if (m_motion.m_type == hkpMotion::MOTION_FIXED) {
        hkAlgorithm::quickSort(m_constraintsSlave.m_data, m_constraintsSlave.m_size, cmpLessConstraint(this));
        for (int32_t i = 0; i < m_constraintsSlave.m_size; ++i) {
            hkpConstraintInstance* constraint = m_constraintsSlave.m_data[i];
            hkSmallArray<hkConstraintInternal>& masters = constraint->getOtherEntity(this)->m_constraintsMaster;
            for (hkConstraintInternal& master : masters) {
                if (master.m_constraint == constraint) {
                    master.m_slaveIndex = static_cast<uint16_t>(i);
                    break;
                }
            }
        }
    }
}

hkpDynamicsContactMgr* hkpEntity::findContactMgrTo(const hkpEntity* entity) const {
    hkpLinkedCollidable::CollisionEntry* i;
    int32_t m_size = m_collidable.m_collisionEntries.m_size;
    int32_t v3 = 0;
    if (m_size <= 0) {
        return nullptr;
    }
    for (i = m_collidable.m_collisionEntries.m_data; i->m_partner != &entity->m_collidable; ++i) {
        if (++v3 >= m_size) {
            return nullptr;
        }
    }
    return static_cast<hkpDynamicsContactMgr*>(i->m_agentEntry->m_contactMgr);
}

// Whether the entity's island is active.
hkBool hkpEntity::isActive() const {
    hkBool result;
    hkpSimulationIsland* m_simulationIsland = this->getSimulationIsland();
    if (m_simulationIsland) {
        result.m_bool = m_simulationIsland->m_activeMark;
        return result;
    }

    result.m_bool = 0;
    return result;
}

void hkpEntity::setCachedShapeData(const hkpWorld* world, const hkpShape* shape) {
    if (m_collidable.m_boundingVolumeData.m_childShapeAabbs) {
        m_collidable.m_boundingVolumeData.deallocate();
    }
    if (world && shape) {
        const hkpShapeContainer* v6;
        uint32_t v4 = world->m_collisionDispatcher->m_hasAlternateType[shape->m_type];
        char v5 = 0;
        if ((v4 & 128) == 0) {
            if ((v4 & 256) == 0) {
                return;
            }
            v6 = shape->getContainer();
            v5 = 1;
        }
        else {
            v6 = shape->getContainer();
        }
        uint8_t m_type = m_motion.m_type.m_storage;
        if ((m_type != 5 && m_type != 4) || !v5) {
            int32_t v8 = v6->getNumChildShapes();
            m_collidable.m_boundingVolumeData.allocate(v8);
            m_collidable.m_boundingVolumeData.m_min[0] = 1;
            m_collidable.m_boundingVolumeData.m_max[0u] = 0;
            if (m_motion.m_type.m_storage == 5) {
                updateCachedAabb();
            }
        }
    }
}

void hkpEntity::updateCachedAabb() {
    hkpEntity* entities;
    entities = this;
    hkpWorld* m_world = this->m_world;
    if (m_world && m_collidable.m_shape) {
        entities = this;
        hkpEntityAabbUtil::entityBatchRecalcAabb(m_world->m_collisionInput, &entities, 1);
    }
}

const hkSmallArray<hkConstraintInternal>& hkpEntity::getConstraintMastersImpl() {
    return m_constraintsMaster;
}

hkSmallArray<hkConstraintInternal>& hkpEntity::getConstraintMastersRwImpl() {
    return m_constraintsMaster;
}

const hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& hkpEntity::getConstraintSlavesImpl() {
    return m_constraintsSlave;
}

double hkpEntity::getMotionMassInv() {
    return XFNV_M128_F32(
        _mm_shuffle_ps(m_motion.m_inertiaAndMassInv.m_quad, m_motion.m_inertiaAndMassInv.m_quad, 255),
        0
    );
}

hkBool* hkEntity_isActive(const hkpEntity* entity, hkBool* apResult) {
    hkpSimulationIsland* m_simulationIsland = entity->getSimulationIsland();
    if (m_simulationIsland) {
        apResult->m_bool = m_simulationIsland->m_activeMark;
    }
    else {
        apResult->m_bool = 0;
    }
    return apResult;
}
