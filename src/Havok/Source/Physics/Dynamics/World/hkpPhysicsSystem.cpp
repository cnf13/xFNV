#include "Havok/Source/Physics/Dynamics/World/hkpPhysicsSystem.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstanceClass.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomType.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"
#include "SDK/MSVC/string.h"

#include <xmmintrin.h>

static void finishLoadedObjecthkpPhysicsSystem(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
static const void* getVtablehkpPhysicsSystem();
static void cleanupLoadedObjecthkpPhysicsSystem(void* p);

hkpPhysicsSystem::MembersArray hkpPhysicsSystem::Members{
    {"rigidBodies",
     &hkpRigidBodyClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x8,
     nullptr},
    {"constraints",
     &hkpConstraintInstanceClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x14,
     nullptr},
    {"actions",
     &hkpActionClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x20,
     nullptr},
    {"phantoms",
     &hkpPhantomClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x2C,
     nullptr},
    {"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x38, nullptr},
    {"userData", nullptr, nullptr, {hkClassMember::TYPE_ULONG}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x3C, nullptr},
    {"active", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x40, nullptr}
};

static const void* getVtablehkpPhysicsSystem() {
    return hkGetVtableOf<hkpPhysicsSystem>();
}

static void cleanupLoadedObjecthkpPhysicsSystem(void* p) {
    static_cast<hkpPhysicsSystem*>(p)->~hkpPhysicsSystem();
}

// The type info's finish-loaded-object callback for hkpPhysicsSystem: runs its finish constructor (the vtable, then the
// name's finish constructor).
static void finishLoadedObjecthkpPhysicsSystem(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag aFlag;
        aFlag.m_finishing = finishing;
        ::new (p) hkpPhysicsSystem(aFlag);
    }
}

namespace {
    // The defaults the class record names: per member its offset into this struct (-1: no default), then the defaults
    // (active true).
    struct hkpPhysicsSystem_DefaultStruct {
        int32_t s_defaultOffsets[7];
        hkBool m_active;
    };
} // namespace
static const hkpPhysicsSystem_DefaultStruct hkpPhysicsSystem_Default{{-1, -1, -1, -1, -1, -1, 0x1C}, true};

hkClass hkpPhysicsSystemClass(
    "hkpPhysicsSystem",
    &hkReferencedObjectClass,
    68,
    nullptr,
    0,
    nullptr,
    0,
    hkpPhysicsSystem::Members,
    7,
    &hkpPhysicsSystem_Default,
    nullptr,
    0,
    0
);
hkTypeInfo hkpPhysicsSystemTypeInfo = {
    "hkpPhysicsSystem",          finishLoadedObjecthkpPhysicsSystem, cleanupLoadedObjecthkpPhysicsSystem,
    getVtablehkpPhysicsSystem(), &typeid(hkpPhysicsSystem),          sizeof(hkpPhysicsSystem)
};

// File-local helper: moves an AABB phantom's box by the translation, and gives a shape phantom its own transform times
// transformation. Other phantom types are left alone.
void tranformPhantom(hkpPhantom* phantom, const hkTransform& transformation) {
    switch (phantom->getType()) {
        case HK_PHANTOM_AABB: {
            hkpAabbPhantom* aabbPhantom = static_cast<hkpAabbPhantom*>(phantom);
            hkAabb aabb;
            aabb = aabbPhantom->m_aabb;
            __m128 translation = transformation.m_translation.m_quad;
            aabb.m_min.m_quad = _mm_add_ps(translation, aabb.m_min.m_quad);
            aabb.m_max.m_quad = _mm_add_ps(translation, aabb.m_max.m_quad);
            aabbPhantom->setAabb(aabb);
            break;
        }
        case HK_PHANTOM_SIMPLE_SHAPE:
        case HK_PHANTOM_CACHING_SHAPE: {
            hkpShapePhantom* shapePhantom = static_cast<hkpShapePhantom*>(phantom);
            hkTransform t = shapePhantom->m_motionState.m_transform;
            t.setMulEq(transformation);
            shapePhantom->setTransform(t);
            break;
        }
        default: break;
    }
}

hkpPhysicsSystem::hkpPhysicsSystem() : m_name(nullptr), m_userData(0), m_active(true) {}

// The name and the arrays free themselves.
hkpPhysicsSystem::~hkpPhysicsSystem() {
    removeAll();
}

// A copy of the system: a new system with this one's name, user data and active flag, a clone of every rigid body and
// phantom, and every constraint and action cloned onto the clones of the bodies and phantoms they use (a constraint to
// the world keeps no second body; a null action stays null).
hkpPhysicsSystem* hkpPhysicsSystem::clone() const {
    hkpPhysicsSystem* newSystem =
        static_cast<hkpPhysicsSystem*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpPhysicsSystem)));
    newSystem->m_memSizeAndFlags = sizeof(hkpPhysicsSystem);
    newSystem = ::new (newSystem) hkpPhysicsSystem;
    newSystem->m_name = m_name;
    newSystem->m_userData = m_userData;
    newSystem->m_active.m_bool = m_active.m_bool;
    newSystem->m_rigidBodies.setSize(m_rigidBodies.getSize());
    newSystem->m_phantoms.setSize(m_phantoms.getSize());
    newSystem->m_constraints.setSize(m_constraints.getSize());
    newSystem->m_actions.setSize(m_actions.getSize());
    for (int32_t i = 0; i < m_rigidBodies.getSize(); ++i) {
        newSystem->m_rigidBodies.m_data[i] = m_rigidBodies.m_data[i]->clone();
    }
    for (int32_t i = 0; i < m_phantoms.getSize(); ++i) {
        newSystem->m_phantoms.m_data[i] = m_phantoms.m_data[i]->clone();
    }
    for (int32_t i = 0; i < m_constraints.getSize(); ++i) {
        hkpConstraintInstance* constraint = m_constraints.m_data[i];
        const hkpEntity* entityA = constraint->getEntityA();
        // EXE-INLINED 0x629F10
        int32_t indexA = -1;
        for (int32_t j = 0; j < m_rigidBodies.getSize(); ++j) {
            if (m_rigidBodies.m_data[j] == entityA) {
                indexA = j;
                break;
            }
        }
        hkpRigidBody* newBodyA = newSystem->m_rigidBodies.m_data[indexA];
        hkpRigidBody* newBodyB;
        hkBool bConstrainedToWorld;
        if (constraint->isConstrainedToWorld(&bConstrainedToWorld)->m_bool) {
            newBodyB = nullptr;
        }
        else {
            const hkpEntity* entityB = constraint->getEntityB();
            // EXE-INLINED 0x629F10
            int32_t indexB = -1;
            for (int32_t j = 0; j < m_rigidBodies.getSize(); ++j) {
                if (m_rigidBodies.m_data[j] == entityB) {
                    indexB = j;
                    break;
                }
            }
            newBodyB = newSystem->m_rigidBodies.m_data[indexB];
        }
        newSystem->m_constraints.m_data[i] =
            constraint->clone(newBodyA, newBodyB, hkpConstraintInstance::CLONE_INSTANCES_ONLY);
    }
    for (int32_t i = 0; i < m_actions.getSize(); ++i) {
        hkpAction* action = m_actions.m_data[i];
        if (!action) {
            newSystem->m_actions.m_data[i] = nullptr;
            continue;
        }
        // EXE-INLINED none: hkArray<hkpEntity*>::hkArray
        XFNV_INLINED_LOCAL(hkArray<hkpEntity*>, entities, std::nullopt, std::nullopt);
        entities.m_data = nullptr;
        entities.m_size = 0;
        entities.m_capacityAndFlags = hkArrayBase<hkpEntity*>::DONT_DEALLOCATE_FLAG;
        action->getEntities(entities);
        // EXE-INLINED none: hkArray<hkpEntity*>::hkArray
        XFNV_INLINED_LOCAL(hkArray<hkpEntity*>, newEntities, std::nullopt, std::nullopt);
        newEntities.m_data = nullptr;
        newEntities.m_size = 0;
        newEntities.m_capacityAndFlags = hkArrayBase<hkpEntity*>::DONT_DEALLOCATE_FLAG;
        newEntities.setSize(entities.getSize());
        for (int32_t j = 0; j < newEntities.getSize(); ++j) {
            const hkpEntity* entity = entities.m_data[j];
            // EXE-INLINED 0x629F10
            int32_t index = -1;
            for (int32_t k = 0; k < m_rigidBodies.getSize(); ++k) {
                if (m_rigidBodies.m_data[k] == entity) {
                    index = k;
                    break;
                }
            }
            newEntities.m_data[j] = newSystem->m_rigidBodies.m_data[index];
        }
        // EXE-INLINED none: hkArray<hkpPhantom*>::hkArray
        XFNV_INLINED_LOCAL(hkArray<hkpPhantom*>, phantoms, std::nullopt, std::nullopt);
        phantoms.m_data = nullptr;
        phantoms.m_size = 0;
        phantoms.m_capacityAndFlags = hkArrayBase<hkpPhantom*>::DONT_DEALLOCATE_FLAG;
        action->getPhantoms(phantoms);
        // EXE-INLINED none: hkArray<hkpPhantom*>::hkArray
        XFNV_INLINED_LOCAL(hkArray<hkpPhantom*>, newPhantoms, std::nullopt, std::nullopt);
        newPhantoms.m_data = nullptr;
        newPhantoms.m_size = 0;
        newPhantoms.m_capacityAndFlags = hkArrayBase<hkpPhantom*>::DONT_DEALLOCATE_FLAG;
        newPhantoms.setSize(phantoms.getSize());
        for (int32_t j = 0; j < newPhantoms.getSize(); ++j) {
            const hkpPhantom* phantom = phantoms.m_data[j];
            // EXE-INLINED 0x629F10
            int32_t index = -1;
            for (int32_t k = 0; k < m_phantoms.getSize(); ++k) {
                if (m_phantoms.m_data[k] == phantom) {
                    index = k;
                    break;
                }
            }
            newPhantoms.m_data[j] = newSystem->m_phantoms.m_data[index];
        }
        newSystem->m_actions.m_data[i] = action->clone(newEntities, newPhantoms);
        // EXE-INLINED none: hkArray<hkpPhantom*>::~hkArray
        newPhantoms.clearAndDeallocate();
        // EXE-INLINED none: hkArray<hkpPhantom*>::~hkArray
        phantoms.clearAndDeallocate();
        // EXE-INLINED none: hkArray<hkpEntity*>::~hkArray
        newEntities.clearAndDeallocate();
        // EXE-INLINED none: hkArray<hkpEntity*>::~hkArray
        entities.clearAndDeallocate();
    }
    return newSystem;
}

hkBool* hkpPhysicsSystem::hasContacts(hkBool* arResult) {
    arResult->m_bool = false;
    return arResult;
}

// Drops the system's reference to each of its rigid bodies, phantoms, constraints and actions, and empties the arrays.
// Each array's size is read once, before its loop, and its data at every element.
void hkpPhysicsSystem::removeAll() {
    int32_t numRigidBodies = m_rigidBodies.m_size;
    for (int32_t i = 0; i < numRigidBodies; ++i) {
        if (m_rigidBodies.m_data[i]) {
            m_rigidBodies.m_data[i]->removeReference();
        }
    }
    m_rigidBodies.m_size = 0;
    int32_t numPhantoms = m_phantoms.m_size;
    for (int32_t i = 0; i < numPhantoms; ++i) {
        if (m_phantoms.m_data[i]) {
            m_phantoms.m_data[i]->removeReference();
        }
    }
    m_phantoms.m_size = 0;
    int32_t numConstraints = m_constraints.m_size;
    for (int32_t i = 0; i < numConstraints; ++i) {
        if (m_constraints.m_data[i]) {
            m_constraints.m_data[i]->removeReference();
        }
    }
    m_constraints.m_size = 0;
    int32_t numActions = m_actions.m_size;
    for (int32_t i = 0; i < numActions; ++i) {
        if (m_actions.m_data[i]) {
            m_actions.m_data[i]->removeReference();
        }
    }
    m_actions.m_size = 0;
}

int32_t hkpPhysicsSystem::removeRigidBody(int32_t a1) {
    m_rigidBodies.m_data[a1]->removeReference();
    int32_t result = --m_rigidBodies.m_size;
    if (result != a1) {
        m_rigidBodies.m_data[a1] = m_rigidBodies.m_data[result];
    }
    return result;
}

int32_t hkpPhysicsSystem::removeConstraint(int32_t a1) {
    m_constraints.m_data[a1]->removeReference();
    int32_t result = --m_constraints.m_size;
    if (result != a1) {
        m_constraints.m_data[a1] = m_constraints.m_data[result];
    }
    return result;
}

int32_t hkpPhysicsSystem::removeAction(int32_t a1) {
    m_actions.m_data[a1]->removeReference();
    int32_t result = --m_actions.m_size;
    if (result != a1) {
        m_actions.m_data[a1] = m_actions.m_data[result];
    }
    return result;
}

void hkpPhysicsSystem::addRigidBody(hkpRigidBody* rb) {
    if (rb) {
        rb->addReference();
        hkArray<hkpRigidBody*, hkContainerHeapAllocator>* p_m_rigidBodies = &m_rigidBodies;
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        if (p_m_rigidBodies->m_size ==
            (p_m_rigidBodies->m_capacityAndFlags & hkArrayBase<hkpRigidBody*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_rigidBodies, sizeof(hkpRigidBody*));
        }
        p_m_rigidBodies->m_data[p_m_rigidBodies->m_size++] = rb;
    }
}

void hkpPhysicsSystem::addConstraint(hkpConstraintInstance* c) {
    if (c) {
        c->addReference();
        hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>* p_m_constraints = &m_constraints;
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        if (p_m_constraints->m_size ==
            (p_m_constraints->m_capacityAndFlags & hkArrayBase<hkpConstraintInstance*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_constraints, sizeof(hkpConstraintInstance*));
        }
        p_m_constraints->m_data[p_m_constraints->m_size++] = c;
    }
}

void hkpPhysicsSystem::addAction(hkpAction* a) {
    if (a) {
        a->addReference();
        hkArray<hkpAction*, hkContainerHeapAllocator>* p_m_actions = &m_actions;
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        if (p_m_actions->m_size == (p_m_actions->m_capacityAndFlags & hkArrayBase<hkpAction*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_actions, sizeof(hkpAction*));
        }
        p_m_actions->m_data[p_m_actions->m_size++] = a;
    }
}

// Moves every rigid body, constraint and phantom of the system by transformation: each body's transform becomes
// transformation times its own.
void hkpPhysicsSystem::transform(const hkTransform& transformation) {
    for (int32_t i = 0; i < m_rigidBodies.m_size; ++i) {
        hkTransform t = transformation;
        t.setMulEq(m_rigidBodies.m_data[i]->m_motion.m_motionState.m_transform);
        m_rigidBodies.m_data[i]->setTransform(t);
    }
    for (int32_t i = 0; i < m_constraints.m_size; ++i) {
        m_constraints.m_data[i]->transform(transformation);
    }
    for (int32_t i = 0; i < m_phantoms.m_size; ++i) {
        tranformPhantom(m_phantoms.m_data[i], transformation);
    }
}

int32_t hkpPhysicsSystem::removePhantom(int32_t a1) {
    m_phantoms.m_data[a1]->removeReference();
    int32_t result = --m_phantoms.m_size;
    if (result != a1) {
        m_phantoms.m_data[a1] = m_phantoms.m_data[result];
    }
    return result;
}

void hkpPhysicsSystem::removeNullPhantoms() {
    for (int32_t i = m_phantoms.m_size - 1; i >= 0; --i) {
        if (!m_phantoms.m_data[i]) {
            int32_t v2 = 4 * (--m_phantoms.m_size - i);
            hkpPhantom** v3 = &m_phantoms.m_data[i];
            if (v2 > 0) {
                uint32_t v4 = (static_cast<uint32_t>(v2 - 1) >> 2) + 1;
                do {
                    *v3 = v3[1];
                    ++v3;
                    --v4;
                }
                while (v4);
            }
        }
    }
}

// The SDK's member-wise copy: the four arrays through hkArray's inline operator=, then the name, the user data and
// the active flag.
void hkpPhysicsSystem::copy(const hkpPhysicsSystem& toCopy) {
    // EXE-INLINED none: hkArray<hkpRigidBody*>::operator=
    m_rigidBodies.copyFromArray(hkMemoryRouter::getInstance().heap(), toCopy.m_rigidBodies, hkTypeIsPod());
    // EXE-INLINED none: hkArray<hkpPhantom*>::operator=
    m_phantoms.copyFromArray(hkMemoryRouter::getInstance().heap(), toCopy.m_phantoms, hkTypeIsPod());
    // EXE-INLINED none: hkArray<hkpConstraintInstance*>::operator=
    m_constraints.copyFromArray(hkMemoryRouter::getInstance().heap(), toCopy.m_constraints, hkTypeIsPod());
    // EXE-INLINED none: hkArray<hkpAction*>::operator=
    m_actions.copyFromArray(hkMemoryRouter::getInstance().heap(), toCopy.m_actions, hkTypeIsPod());
    m_name = toCopy.m_name;
    m_userData = toCopy.m_userData;
    m_active.m_bool = toCopy.m_active.m_bool;
}

void hkpPhysicsSystem::addPhantom(hkpPhantom* p) {
    if (p) {
        p->addReference();
        hkArray<hkpPhantom*, hkContainerHeapAllocator>* p_m_phantoms = &m_phantoms;
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        if (p_m_phantoms->m_size == (p_m_phantoms->m_capacityAndFlags & hkArrayBase<hkpPhantom*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_phantoms, sizeof(hkpPhantom*));
        }
        p_m_phantoms->m_data[p_m_phantoms->m_size++] = p;
    }
}

const hkClass& hkpPhysicsSystem::staticClass() {
    return hkpPhysicsSystemClass;
}
