#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"

#include "Havok/Source/Common/Base/Container/Array/hkSmallArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkSmallArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalRotationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTranslationsConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpResponseModifier.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/BallAndSocket/hkpBallAndSocketConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Hinge/hkpHingeConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/LimitedHinge/hkpLimitedHingeConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPlane/hkpPointToPlaneConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Prismatic/hkpPrismaticConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Ragdoll/hkpRagdollConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/StiffSpring/hkpStiffSpringConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Wheel/hkpWheelConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Breakable/hkpBreakableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Util/hkpConstraintCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Util/hkpConstraintDataCloningUtil.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Util/hkpConstraintDataUtils.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintListener.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldConstraintUtil.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

static hkVector4* getPivotA(hkpConstraintData* a1);
static hkVector4* getPivotB(hkpConstraintData* a1);

// The pivot of body A in its body's space: a breakable or malleable constraint's wrapped data is followed; the
// ball-and-socket group's pivot or the transform group's frame A translation; hkVector4Zero for any other type. The
// groups are the same as setPivot's.
static hkVector4* getPivotA(hkpConstraintData* a1) {
    hkpConstraintData* pData = a1;
    for (;;) {
        switch (pData->getType()) {
            case hkpConstraintData::CONSTRAINT_TYPE_BALLANDSOCKET:
            case hkpConstraintData::CONSTRAINT_TYPE_STIFFSPRING:
            case hkpConstraintData::CONSTRAINT_TYPE_PULLEY:
                return &static_cast<hkpBallAndSocketConstraintData*>(pData)->m_atoms.m_pivots.m_translationA;
            case hkpConstraintData::CONSTRAINT_TYPE_HINGE:
            case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE:
            case hkpConstraintData::CONSTRAINT_TYPE_PRISMATIC:
            case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL:
            case hkpConstraintData::CONSTRAINT_TYPE_WHEEL:
            case hkpConstraintData::CONSTRAINT_TYPE_POINTTOPLANE:
                return &static_cast<hkpLimitedHingeConstraintData*>(pData)
                            ->m_atoms.m_transforms.m_transformA.m_translation;
            case hkpConstraintData::CONSTRAINT_TYPE_BREAKABLE:
                pData = static_cast<hkpBreakableConstraintData*>(pData)->m_constraintData;
                continue;
            case hkpConstraintData::CONSTRAINT_TYPE_MALLEABLE:
                pData = static_cast<hkpMalleableConstraintData*>(pData)->getWrappedConstraintData();
                continue;
            default: return &hkVector4Zero;
        }
    }
}

// The pivot of body B in its body's space, as getPivotA.
static hkVector4* getPivotB(hkpConstraintData* a1) {
    hkpConstraintData* pData = a1;
    for (;;) {
        switch (pData->getType()) {
            case hkpConstraintData::CONSTRAINT_TYPE_BALLANDSOCKET:
            case hkpConstraintData::CONSTRAINT_TYPE_STIFFSPRING:
            case hkpConstraintData::CONSTRAINT_TYPE_PULLEY:
                return &static_cast<hkpBallAndSocketConstraintData*>(pData)->m_atoms.m_pivots.m_translationB;
            case hkpConstraintData::CONSTRAINT_TYPE_HINGE:
            case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE:
            case hkpConstraintData::CONSTRAINT_TYPE_PRISMATIC:
            case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL:
            case hkpConstraintData::CONSTRAINT_TYPE_WHEEL:
            case hkpConstraintData::CONSTRAINT_TYPE_POINTTOPLANE:
                return &static_cast<hkpLimitedHingeConstraintData*>(pData)
                            ->m_atoms.m_transforms.m_transformB.m_translation;
            case hkpConstraintData::CONSTRAINT_TYPE_BREAKABLE:
                pData = static_cast<hkpBreakableConstraintData*>(pData)->m_constraintData;
                continue;
            case hkpConstraintData::CONSTRAINT_TYPE_MALLEABLE:
                pData = static_cast<hkpMalleableConstraintData*>(pData)->getWrappedConstraintData();
                continue;
            default: return &hkVector4Zero;
        }
    }
}

hkpConstraintInstance::hkpConstraintInstance(
    hkpEntity* apEntityA,
    hkpEntity* apEntityB,
    hkpConstraintData* apData,
    ConstraintPriority aePriority
)
  : m_owner(nullptr), m_data(apData), m_constraintModifiers(nullptr), m_entities{apEntityA, apEntityB},
    m_wantRuntime(true), m_userData(0), m_internal(nullptr), m_uid(0xFFFFFFF0u) {
    m_priority.m_storage = aePriority;
    m_destructionRemapInfo.m_storage = 0;
    hkReferencedObject::lockAll();
    m_entities[0]->addReference();
    if (m_entities[1]) {
        m_entities[1]->addReference();
    }
    getDataRw()->addReference();
    hkReferencedObject::unlockAll();
}

hkpConstraintInstance::hkpConstraintInstance(ConstraintPriority aePriority)
  : m_owner(nullptr), m_constraintModifiers(nullptr), m_wantRuntime(true), m_userData(0), m_internal(nullptr),
    m_uid(0xFFFFFFF0u) {
    m_priority.m_storage = aePriority;
    m_destructionRemapInfo.m_storage = 0;
}

// The deletion told to the listeners, both entities released, the modifier atom chain returned to the thread's heap
// one atom at a time, then the constraint data released; the compiler frees the name and the listener array.
hkpConstraintInstance::~hkpConstraintInstance() {
    hkpConstraintCallbackUtil::fireConstraintDeleted(this);
    if (m_entities[0]) {
        m_entities[0]->removeReference();
    }
    if (m_entities[1]) {
        m_entities[1]->removeReference();
    }
    hkpModifierConstraintAtom* pAtom = m_constraintModifiers;
    if (pAtom) {
        while (pAtom->m_type.m_storage >= hkpConstraintAtom::TYPE_MODIFIER_SOFT_CONTACT) {
            hkpModifierConstraintAtom* pNext = static_cast<hkpModifierConstraintAtom*>(pAtom->m_child);
            hkMemoryRouter::getInstance().heap().blockFree(pAtom, static_cast<int32_t>(pAtom->m_modifierAtomSize));
            pAtom = pNext;
        }
    }
    if (m_data) {
        m_data->removeReference();
    }
}

void hkpConstraintInstance::entityAddedCallback(hkpEntity* entity) {
    ;
}

void hkpConstraintInstance::entityRemovedCallback(hkpEntity* entity) {
    if (m_owner) {
        hkpWorldOperationUtil::removeConstraintImmediately(
            entity->m_world, this, hkpWorldOperationUtil::DO_FIRE_CALLBACKS
        );
    }
}

void hkpConstraintInstance::entityDeletedCallback(hkpEntity* entity) {
    ;
}

void hkpConstraintInstance::enable() {
    hkpResponseModifier::enableConstraint(this, *this->getOwner());
}

void hkpConstraintInstance::disable() {
    hkpResponseModifier::disableConstraint(this, *this->getOwner());
}

// A copy of the constraint between the two new entities.
hkpConstraintInstance* hkpConstraintInstance::clone(
    hkpEntity* newEntityA,
    hkpEntity* newEntityB,
    hkpConstraintInstance::CloningMode mode
) const {
    hkpConstraintData* m_data;
    hkMemoryRouter* v6;
    hkpConstraintInstance* v7;
    hkpConstraintInstance* v8;
    hkpConstraintInstance* v11;
    if (mode) {
        if (mode == hkpConstraintInstance::CLONE_DATAS_WITH_MOTORS) {
            m_data = hkpConstraintDataCloningUtil::cloneIfCanHaveMotors(this->getDataRw());
            if (!m_data) {
                m_data = this->getDataRw();
                m_data->addReference();
            }
            hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
            hkpConstraintInstance* v10 =
                static_cast<hkpConstraintInstance*>(Instance->m_heap->blockAlloc(sizeof(hkpConstraintInstance)));
            // The size word is the native sizeof, because the heap frees by this word.
            v10->m_memSizeAndFlags = sizeof(hkpConstraintInstance);
            v8 = ::new (v10) hkpConstraintInstance(
                newEntityA, newEntityB, m_data,
                static_cast<hkpConstraintInstance::ConstraintPriority>(m_priority.m_storage)
            );
            goto LABEL_9;
        }
    LABEL_10:;
        hkMemoryRouter* v12 = &hkMemoryRouter::getInstance();
        hkpConstraintInstance* v13 =
            static_cast<hkpConstraintInstance*>(v12->m_heap->blockAlloc(sizeof(hkpConstraintInstance)));
        v13->m_memSizeAndFlags = sizeof(hkpConstraintInstance);
        v11 = ::new (v13) hkpConstraintInstance(
            newEntityA, newEntityB, this->getDataRw(),
            static_cast<hkpConstraintInstance::ConstraintPriority>(m_priority.m_storage)
        );
        goto LABEL_11;
    }
    hkBool bConstrainedToWorld;
    if (!isConstrainedToWorld(&bConstrainedToWorld)->m_bool) {
        goto LABEL_10;
    }
    m_data = hkpConstraintDataCloningUtil::deepClone(this->getDataRw());
    v6 = &hkMemoryRouter::getInstance();
    v7 = static_cast<hkpConstraintInstance*>(v6->m_heap->blockAlloc(sizeof(hkpConstraintInstance)));
    v7->m_memSizeAndFlags = sizeof(hkpConstraintInstance);
    v8 = ::new (v7) hkpConstraintInstance(
        newEntityA, nullptr, m_data, static_cast<hkpConstraintInstance::ConstraintPriority>(m_priority.m_storage)
    );
LABEL_9:;
    v11 = v8;
    m_data->removeReference();
LABEL_11:;
    v11->m_name.operator=(
        reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(m_name.m_stringAndFlag) & ~uintptr_t(1))
    );
    return v11;
}

void hkpConstraintInstance::replaceEntity(hkpEntity* oldEntity, hkpEntity* newEntity) {
    bool v3 = oldEntity == m_entities[0];
    newEntity->addReference();
    if (oldEntity) {
        oldEntity->removeReference();
    }
    m_entities[!v3] = newEntity;
}

// Whether entity B is missing or is entity A's world's fixed rigid body.
hkBool* hkpConstraintInstance::isConstrainedToWorld(hkBool* arResult) const {
    hkpWorld* pWorld;
    hkpEntity* pEntityB = m_entities[1];
    if (!pEntityB ||
        ((pWorld = m_entities[0]->m_world) != nullptr &&
         static_cast<const void*>(pEntityB) == static_cast<const void*>(pWorld->m_fixedRigidBody))) {
        arResult->m_bool = 1;
    }
    else {
        arResult->m_bool = 0;
    }
    return arResult;
}

// The master entity's island: entity A, or entity B when A is fixed. getMasterEntity's test is written out inline (a
// fixed first entity loses), then the entity's island is read.
hkpSimulationIsland* hkpConstraintInstance::getSimulationIsland() {
    hkpEntity* pMaster = m_entities[0];
    if (pMaster->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
        pMaster = m_entities[1];
    }
    return pMaster->getSimulationIsland();
}

void hkpConstraintInstance::getPivotsInWorld(hkVector4& pivotAinW, hkVector4& pivotBinW) const {
    hkVector4 a3;
    hkVector4 v11;
    hkVector4* v4 = getPivotA(this->getDataRw());
    hkpConstraintData* m_data = this->getDataRw();
    a3.m_quad = v4->m_quad;
    hkVector4* v5 = getPivotB(m_data);
    hkpEntity* v6 = m_entities[1];
    hkpEntity* v7 = m_entities[0];
    v11.m_quad = v5->m_quad;
    hkTransform* pMotion = v6->m_collidable.m_motion.pMotion;
    pivotAinW.setTransformedPos(*v7->m_collidable.m_motion.pMotion, a3);
    pivotBinW.setTransformedPos(*pMotion, v11);
}

void hkpConstraintInstance::addConstraintListener(hkpConstraintListener* listener) {
    hkpConstraintListener** m_data;
    uint16_t m_size = m_listeners.m_size;
    hkSmallArray<hkpConstraintListener*>* p_m_listeners = &m_listeners;
    int32_t v4 = 0;
    if (!m_size) {
        goto LABEL_5;
    }
    m_data = p_m_listeners->begin();
    while (*m_data) {
        ++v4;
        ++m_data;
        if (v4 >= p_m_listeners->m_size) {
            goto LABEL_5;
        }
    }
    if (v4 >= 0) {
        p_m_listeners->m_data[v4] = listener;
    }
    else {
    LABEL_5:;
        if (m_size == (p_m_listeners->m_capacityAndFlags & hkSmallArray<hkpConstraintListener*>::CAPACITY_MASK)) {
            hkSmallArrayUtil::_reserveMore(p_m_listeners, sizeof(hkpConstraintListener*));
        }
        p_m_listeners->m_data[p_m_listeners->m_size++] = listener;
    }
}

// The listener's slot is cleared, not removed; a listener not in the list deliberately clears the slot before the array
// (indexOf's -1), as the game does.
void hkpConstraintInstance::removeConstraintListener(hkpConstraintListener* listener) {
    // EXE-INLINED none: hkSmallArray<hkpConstraintListener*>::indexOf
    int32_t iSize = m_listeners.m_size;
    int32_t i = 0;
    for (; i < iSize; ++i) {
        if (m_listeners.m_data[i] == listener) {
            break;
        }
    }
    if (i >= iSize) {
        i = -1;
    }
    m_listeners.m_data[i] = nullptr;
}

void hkpConstraintInstance::setPriority(hkpConstraintInstance::ConstraintPriority priority) {
    m_priority.m_storage = priority;
    hkConstraintInternal* m_internal = this->getInternal();
    if (m_internal) {
        m_internal->m_priority.m_storage = priority;
    }
}

// Havok SDK: the existing mass changer modifier gets the factors, else a new one is added to the constraint.
void hkpConstraintInstance::setVirtualMassInverse(const hkVector4& invMassA, const hkVector4& invMassB) {
    hkpMassChangerModifierConstraintAtom* pModifier = static_cast<hkpMassChangerModifierConstraintAtom*>(
        hkpWorldConstraintUtil::findModifier(this, hkpConstraintAtom::TYPE_MODIFIER_MASS_CHANGER)
    );
    if (pModifier) {
        pModifier->m_factorA = invMassA;
        pModifier->m_factorB = invMassB;
        return;
    }
    // EXE-INLINED none: hkpMassChangerModifierConstraintAtom::operator new, the constructor (the type and the
    // modifier's size)
    pModifier = static_cast<hkpMassChangerModifierConstraintAtom*>(
        hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpMassChangerModifierConstraintAtom))
    );
    if (pModifier) {
        pModifier->m_type.m_storage = hkpConstraintAtom::TYPE_MODIFIER_MASS_CHANGER;
        pModifier->m_modifierAtomSize = sizeof(hkpMassChangerModifierConstraintAtom);
    }
    pModifier->m_factorA = invMassA;
    pModifier->m_factorB = invMassB;
    hkpWorldConstraintUtil::addModifier(this, *getOwner(), pModifier);
}

void hkpConstraintInstance::setEnabled(hkBool state) {
    if (state.m_bool) {
        enable();
    }
    else {
        disable();
    }
}

// Points each null entity at the fixed rigid body of the other entity's world, when the other entity is in one.
void hkpConstraintInstance::pointNullsToFixedRigidBody() {
    for (int32_t i = 0; i < 2; ++i) {
        if (!m_entities[i]) {
            hkpEntity* pOther = getOtherEntity(m_entities[i]);
            if (pOther) {
                hkpWorld* pWorld = pOther->m_world;
                if (pWorld) {
                    hkpRigidBody* pFixed = pWorld->m_fixedRigidBody;
                    m_entities[i] = pFixed;
                    pFixed->addReference();
                }
            }
        }
    }
}

void hkpConstraintInstance::setFixedRigidBodyPointersToZero(hkpWorld* world) {
    hkpEntity** m_entities = this->m_entities;
    int32_t v3 = 2;
    do {
        if (*m_entities == world->m_fixedRigidBody) {
            (*m_entities)->removeReference();
            *m_entities = nullptr;
        }
        ++m_entities;
        --v3;
    }
    while (v3);
}

bool* hkpConstraintInstance::isEnabled(bool* a1) {
    bool v2 = hkpWorldConstraintUtil::findModifier(this, hkpConstraintAtom::TYPE_MODIFIER_IGNORE_CONSTRAINT) == nullptr;
    *a1 = v2;
    return a1;
}

// A constraint to the world (isConstrainedToWorld) has its frame in body B, the world, moved by transformation: pivot B
// transformed as a position and B's axes rotated as directions, then the frames set again through the type's
// setInBodySpace. Breakable and malleable data are looked through. Each frame vector the setter takes is copied into a
// local first; a ball and socket's pivot B and a ragdoll's axes in B are transformed in place in the data, as they are
// taken by reference.
void hkpConstraintInstance::transform(const hkTransform& transformation) {
    hkBool bConstrainedToWorld;
    if (!isConstrainedToWorld(&bConstrainedToWorld)->m_bool) {
        return;
    }
    hkpConstraintData* data = getDataRw();
    for (;;) {
        switch (data->getType()) {
            case hkpConstraintData::CONSTRAINT_TYPE_BREAKABLE:
                data = static_cast<hkpBreakableConstraintData*>(data)->m_constraintData;
                continue;
            case hkpConstraintData::CONSTRAINT_TYPE_MALLEABLE:
                data = static_cast<hkpMalleableConstraintData*>(data)->getWrappedConstraintData();
                continue;
            case hkpConstraintData::CONSTRAINT_TYPE_BALLANDSOCKET: {
                hkpBallAndSocketConstraintData* pBallAndSocket = static_cast<hkpBallAndSocketConstraintData*>(data);
                hkVector4& pivotB = pBallAndSocket->m_atoms.m_pivots.m_translationB;
                pivotB.setTransformedPos(transformation, pivotB);
                pBallAndSocket->setInBodySpace(pBallAndSocket->m_atoms.m_pivots.m_translationA, pivotB);
                return;
            }
            case hkpConstraintData::CONSTRAINT_TYPE_HINGE: {
                hkpHingeConstraintData* pHinge = static_cast<hkpHingeConstraintData*>(data);
                const hkTransform& transformA = pHinge->m_atoms.m_transforms.m_transformA;
                const hkTransform& transformB = pHinge->m_atoms.m_transforms.m_transformB;
                hkVector4 pivotA = transformA.m_translation;
                hkVector4 pivotB = transformB.m_translation;
                hkVector4 axisA = transformA.m_rotation.m_col0;
                hkVector4 axisB = transformB.m_rotation.m_col0;
                pivotB.setTransformedPos(transformation, pivotB);
                axisB.setRotatedDir(transformation.m_rotation, axisB);
                pHinge->setInBodySpace(pivotA, pivotB, axisA, axisB);
                return;
            }
            case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE: {
                hkpLimitedHingeConstraintData* pHinge = static_cast<hkpLimitedHingeConstraintData*>(data);
                const hkTransform& transformA = pHinge->m_atoms.m_transforms.m_transformA;
                const hkTransform& transformB = pHinge->m_atoms.m_transforms.m_transformB;
                hkVector4 pivotA = transformA.m_translation;
                hkVector4 pivotB = transformB.m_translation;
                hkVector4 axisA = transformA.m_rotation.m_col0;
                hkVector4 axisB = transformB.m_rotation.m_col0;
                hkVector4 axisAPerp = transformA.m_rotation.m_col1;
                hkVector4 axisBPerp = transformB.m_rotation.m_col1;
                pivotB.setTransformedPos(transformation, pivotB);
                axisB.setRotatedDir(transformation.m_rotation, axisB);
                axisBPerp.setRotatedDir(transformation.m_rotation, axisBPerp);
                pHinge->setInBodySpace(pivotA, pivotB, axisA, axisB, axisAPerp, axisBPerp);
                return;
            }
            case hkpConstraintData::CONSTRAINT_TYPE_PRISMATIC: {
                hkpPrismaticConstraintData* pPrismatic = static_cast<hkpPrismaticConstraintData*>(data);
                const hkTransform& transformA = pPrismatic->m_atoms.m_transforms.m_transformA;
                const hkTransform& transformB = pPrismatic->m_atoms.m_transforms.m_transformB;
                hkVector4 pivotA = transformA.m_translation;
                hkVector4 pivotB = transformB.m_translation;
                hkVector4 axisA = transformA.m_rotation.m_col0;
                hkVector4 axisB = transformB.m_rotation.m_col0;
                hkVector4 axisAPerp = transformA.m_rotation.m_col1;
                hkVector4 axisBPerp = transformB.m_rotation.m_col1;
                pivotB.setTransformedPos(transformation, pivotB);
                axisB.setRotatedDir(transformation.m_rotation, axisB);
                axisBPerp.setRotatedDir(transformation.m_rotation, axisBPerp);
                pPrismatic->setInBodySpace(pivotA, pivotB, axisA, axisB, axisAPerp, axisBPerp);
                return;
            }
            case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL: {
                hkpRagdollConstraintData* pRagdoll = static_cast<hkpRagdollConstraintData*>(data);
                hkTransform& transformA = pRagdoll->m_atoms.m_transforms.m_transformA;
                hkTransform& transformB = pRagdoll->m_atoms.m_transforms.m_transformB;
                hkVector4 pivotA = transformA.m_translation;
                hkVector4 pivotB = transformB.m_translation;
                pivotB.setTransformedPos(transformation, pivotB);
                transformB.m_rotation.m_col1.setRotatedDir(transformation.m_rotation, transformB.m_rotation.m_col1);
                transformB.m_rotation.m_col0.setRotatedDir(transformation.m_rotation, transformB.m_rotation.m_col0);
                pRagdoll->setInBodySpace(
                    pivotA, pivotB, transformA.m_rotation.m_col1, transformB.m_rotation.m_col1,
                    transformA.m_rotation.m_col0, transformB.m_rotation.m_col0
                );
                return;
            }
            case hkpConstraintData::CONSTRAINT_TYPE_STIFFSPRING: {
                hkpStiffSpringConstraintData* pSpring = static_cast<hkpStiffSpringConstraintData*>(data);
                hkVector4 pivotA = pSpring->m_atoms.m_pivots.m_translationA;
                float fRestLength = pSpring->m_atoms.m_spring.m_length;
                hkVector4 pivotB = pSpring->m_atoms.m_pivots.m_translationB;
                pivotB.setTransformedPos(transformation, pivotB);
                // EXE-INLINED none: hkpStiffSpringConstraintData::setInBodySpace
                pSpring->m_atoms.m_spring.m_length = fRestLength;
                pSpring->m_atoms.m_pivots.m_translationA = pivotA;
                pSpring->m_atoms.m_pivots.m_translationB = pivotB;
                return;
            }
            case hkpConstraintData::CONSTRAINT_TYPE_WHEEL: {
                hkpWheelConstraintData* pWheel = static_cast<hkpWheelConstraintData*>(data);
                const hkpSetLocalTransformsConstraintAtom& suspensionBase = pWheel->m_atoms.m_suspensionBase;
                const hkpSetLocalRotationsConstraintAtom& steeringBase = pWheel->m_atoms.m_steeringBase;
                hkVector4 pivotA = suspensionBase.m_transformA.m_translation;
                hkVector4 pivotB = suspensionBase.m_transformB.m_translation;
                hkVector4 suspensionAxisB = suspensionBase.m_transformB.m_rotation.m_col0;
                hkVector4 axleA = steeringBase.m_rotationA.m_col0;
                hkVector4 axleB = steeringBase.m_rotationB.m_col0;
                pivotB.setTransformedPos(transformation, pivotB);
                hkVector4 steeringAxisB = steeringBase.m_rotationB.m_col1;
                suspensionAxisB.setRotatedDir(transformation.m_rotation, suspensionAxisB);
                axleB.setRotatedDir(transformation.m_rotation, axleB);
                steeringAxisB.setRotatedDir(transformation.m_rotation, steeringAxisB);
                pWheel->setInBodySpace(pivotA, pivotB, axleA, axleB, suspensionAxisB, steeringAxisB);
                return;
            }
            case hkpConstraintData::CONSTRAINT_TYPE_POINTTOPLANE: {
                hkpPointToPlaneConstraintData* pPlane = static_cast<hkpPointToPlaneConstraintData*>(data);
                const hkTransform& transformA = pPlane->m_atoms.m_transforms.m_transformA;
                const hkTransform& transformB = pPlane->m_atoms.m_transforms.m_transformB;
                hkVector4 pivotA = transformA.m_translation;
                hkVector4 pivotB = transformB.m_translation;
                hkVector4 planeNormalB = transformB.m_rotation.getColumn(pPlane->m_atoms.m_lin.m_axisIndex);
                pivotB.setTransformedPos(transformation, pivotB);
                planeNormalB.setRotatedDir(transformation.m_rotation, planeNormalB);
                pPlane->setInBodySpace(pivotA, pivotB, planeNormalB);
                return;
            }
            default: return;
        }
    }
}
