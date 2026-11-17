#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldConstraintUtil.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/Array/hkSmallArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpResponseModifier.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInfo.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "SDK/MSVC/string.h"

hkpModifierConstraintAtom* hkpWorldConstraintUtil::findLastModifier(hkpConstraintInstance* instance) {
    hkpModifierConstraintAtom* modifier = instance->m_constraintModifiers;
    if (!modifier) {
        return nullptr;
    }
    // The modifier atom types are the last ones, from TYPE_MODIFIER_SOFT_CONTACT on; the compare is unsigned.
    for (hkpConstraintAtom* child = modifier->m_child;
         child->m_type.m_storage >= hkpConstraintAtom::TYPE_MODIFIER_SOFT_CONTACT; child = modifier->m_child) {
        modifier = static_cast<hkpModifierConstraintAtom*>(child);
    }
    return modifier;
}

hkpModifierConstraintAtom*
hkpWorldConstraintUtil::findModifier(hkpConstraintInstance* a1, hkpConstraintAtom::AtomType a2) {
    hkpModifierConstraintAtom* result = a1->m_constraintModifiers;
    if (result) {
        if (((uint32_t)result->m_type.m_storage) == ((uint32_t)((int32_t)a2))) {
            return result;
        }
        while (1) {
            result = static_cast<hkpModifierConstraintAtom*>(result->m_child);
            if (((uint32_t)((int32_t)result->m_type.m_storage)) < 0x17u) {
                break;
            }
            if (((uint32_t)result->m_type.m_storage) == ((uint32_t)((int32_t)a2))) {
                return result;
            }
        }
    }
    return nullptr;
}

// Builds the constraint's internal record on the stack, merges the entities' islands, files the record among the master
// entity's records (by priority, constraint uid, then the other entity's uid), cuts room for its runtime in the
// master's runtime buffer (moving the later records' runtimes up) and adds the constraint to the slave entity's slave
// list. The local hkConstraintInternal is only partly written before hkSmallArray<hkConstraintInternal>::insertAt
// copies the whole record into the master entity's array; its alignment padding is never written, so uninitialised
// stack bytes reach the heap, as the game does.
void hkpWorldConstraintUtil::addConstraint(hkpWorld* world, hkpConstraintInstance* constraint) {
    constraint->addReference();

    hkpConstraintData::ConstraintInfo info;
    constraint->getDataRw()->getConstraintInfo(info);

    hkConstraintInternal intern;
    intern.m_constraint = constraint;
    intern.m_entities[0] = constraint->m_entities[0];
    intern.m_entities[1] = constraint->m_entities[1];
    if (constraint->m_constraintModifiers) {
        intern.m_atoms = constraint->m_constraintModifiers;
        intern.m_atomsSize = constraint->m_constraintModifiers->m_modifierAtomSize;
    }
    else {
        intern.m_atoms = info.m_atoms;
        intern.m_atomsSize = static_cast<uint16_t>(info.m_sizeOfAllAtoms);
    }
    intern.m_priority.m_storage = constraint->m_priority.m_storage;

    // A fixed entity A is the slave; otherwise the islands of two non-fixed entities are merged.
    if (intern.m_entities[0]->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED) {
        if (intern.m_entities[1]->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED &&
            intern.m_entities[0]->m_simulationIsland != intern.m_entities[1]->m_simulationIsland) {
            hkpWorldOperationUtil::mergeIslands(
                intern.m_entities[0]->m_world, intern.m_entities[0], intern.m_entities[1]
            );
        }
    }
    intern.m_whoIsMaster = intern.m_entities[0]->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED;
    hkpEntity* master = intern.m_entities[intern.m_whoIsMaster];
    hkpSimulationIsland* island = master->m_simulationIsland;
    constraint->m_owner = island;
    island->m_numConstraints++;

    const uint16_t atomType = intern.m_atoms->m_type.m_storage;
    intern.m_isNormalType.m_bool = true;
    intern.m_callbackRequest = hkpConstraintAtom::CALLBACK_REQUEST_NONE;
    if (atomType == hkpConstraintAtom::TYPE_BRIDGE) {
        intern.m_callbackRequest = hkpConstraintAtom::CALLBACK_REQUEST_SETUP_PPU_ONLY;
        hkBool callbackRequired;
        if (constraint->getDataRw()->isBuildJacobianCallbackRequired(&callbackRequired)->m_bool) {
            intern.m_callbackRequest |= hkpConstraintAtom::CALLBACK_REQUEST_SETUP_CALLBACK;
        }
        intern.m_isNormalType.m_bool = constraint->getType() == hkpConstraintInstance::TYPE_NORMAL;
    }
    else if (atomType == hkpConstraintAtom::TYPE_PULLEY) {
        intern.m_callbackRequest = hkpConstraintAtom::CALLBACK_REQUEST_SETUP_PPU_ONLY;
    }

    uint8_t usedFlags = 0;
    if (constraint->m_constraintModifiers) {
        intern.m_callbackRequest |= static_cast<uint8_t>(hkpModifierConstraintAtom::addAllModifierDataToConstraintInfo(
            constraint->m_constraintModifiers, info, usedFlags
        ));
    }
    if (atomType == hkpConstraintAtom::TYPE_CONTACT) {
        const uint8_t flags = static_cast<uint8_t>(
            constraint->m_entities[1]->m_responseModifierFlags | constraint->m_entities[0]->m_responseModifierFlags
        );
        info.m_sizeOfSchemas +=
            hkpResponseModifier::tableOfAdditionalSchemaSizes[static_cast<uint8_t>(flags & ~usedFlags) & 7];
    }

    intern.m_sizeOfSchemas = 0;
    intern.m_numSolverResults = 0;
    intern.m_numSolverElemTemps = 0;
    constraint->m_internal = &intern;
    constraint->getOwner()->addConstraintInfo(constraint, info);

    // Files the record after every record of lower priority, constraint uid or other-entity uid.
    hkSmallArray<hkConstraintInternal>& masters = master->m_constraintsMaster;
    const int32_t numMasters = masters.getSize();
    int32_t insertIndex = 0;
    for (; insertIndex < numMasters; insertIndex++) {
        const hkConstraintInternal& other = masters.m_data[insertIndex];
        if (intern.m_priority.m_storage < other.m_priority.m_storage) {
            break;
        }
        if (intern.m_priority.m_storage > other.m_priority.m_storage) {
            continue;
        }
        if (intern.m_constraint->m_uid < other.m_constraint->m_uid) {
            break;
        }
        if (intern.m_constraint->m_uid > other.m_constraint->m_uid) {
            continue;
        }
        if (intern.getOtherEntity(master)->m_uid < other.getOtherEntity(master)->m_uid) {
            break;
        }
    }
    // Without a reallocation only the records from the insertion point on move.
    int32_t firstMoved = numMasters < masters.getCapacity() ? insertIndex : 0;
    masters.insertAt(insertIndex, intern);
    for (; firstMoved < masters.getSize(); firstMoved++) {
        masters.m_data[firstMoved].m_constraint->m_internal = &masters.m_data[firstMoved];
    }

    hkConstraintInternal* record = constraint->m_internal;
    hkpConstraintData::RuntimeInfo runtimeInfo;
    record->m_constraint->getDataRw()->getRuntimeInfo(constraint->m_wantRuntime, runtimeInfo);
    runtimeInfo.m_sizeOfExternalRuntime = (runtimeInfo.m_sizeOfExternalRuntime + 15) & ~15;
    record->m_runtimeSize = static_cast<uint16_t>(runtimeInfo.m_sizeOfExternalRuntime);
    if (runtimeInfo.m_sizeOfExternalRuntime) {
        hkArray<uint8_t>& runtimes = master->m_constraintRuntime;
        const int32_t oldSize = runtimes.m_size;
        uint8_t* oldData = runtimes.m_data;
        runtimes.reserveExactly(oldSize + runtimeInfo.m_sizeOfExternalRuntime);
        runtimes.setSizeUnchecked(oldSize + runtimeInfo.m_sizeOfExternalRuntime);
        uint8_t* runtime = runtimes.m_data;
        ptrdiff_t shift = runtimes.m_data - oldData;
        // The records before the new one keep their runtimes, moved with the buffer; the new runtime goes after
        // the last of them.
        for (int32_t i = 0; i < insertIndex; i++) {
            hkConstraintInternal& other = masters.m_data[i];
            if (other.m_runtime) {
                other.m_runtime = static_cast<uint8_t*>(other.m_runtime) + shift;
                runtime = static_cast<uint8_t*>(other.m_runtime) + other.m_runtimeSize;
            }
        }
        hkString::memMove(
            runtime + record->m_runtimeSize, runtime,
            static_cast<int32_t>(runtimes.m_data + runtimes.m_size - record->m_runtimeSize - runtime)
        );
        shift += record->m_runtimeSize;
        record->m_runtime = runtime;
        for (int32_t i = insertIndex + 1; i < masters.getSize(); i++) {
            hkConstraintInternal& other = masters.m_data[i];
            other.m_runtime = other.m_runtime ? static_cast<uint8_t*>(other.m_runtime) + shift : nullptr;
        }
    }
    else {
        record->m_runtime = nullptr;
    }
    record->m_constraint->getDataRw()->addInstance(constraint, record->m_runtime, record->m_runtimeSize);

    hkpEntity* slave = record->m_entities[1 - record->m_whoIsMaster];
    record->m_slaveIndex = static_cast<uint16_t>(slave->m_constraintsSlave.m_size);
    {
        // EXE-INLINED none: hkArray<hkpConstraintInstance*>::pushBack
        hkArray<hkpConstraintInstance*>& slaves = slave->m_constraintsSlave;
        hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&slaves);
        if (slaves.m_size == slaves.getCapacity()) {
            hkArrayUtil::_reserveMore(alloc, &slaves, sizeof(hkpConstraintInstance*));
        }
        slaves.m_data[slaves.m_size] = constraint;
        slaves.m_size++;
    }
}

// Takes the constraint out of its island (which gets a split check), the island's and the constraint's solver info, the
// slave entity's slave list (its last entry fills the gap) and the master entity: its runtime bytes are cut out of the
// master's runtime buffer, the following master records move down one, and each moved record's constraint and runtime
// pointers follow it. Drops the constraint's reference last.
void hkpWorldConstraintUtil::removeConstraint(hkpConstraintInstance* constraint) {
    hkpSimulationIsland* island = static_cast<hkpSimulationIsland*>(constraint->getOwner());
    island->m_splitCheckRequested = 1;
    hkConstraintInternal* intern = constraint->m_internal;
    // Only the three counts subConstraintInfo reads are set.
    hkpConstraintInfo info;
    info.m_sizeOfSchemas = intern->m_sizeOfSchemas;
    info.m_numSolverResults = intern->m_numSolverResults;
    info.m_numSolverElemTemps = intern->m_numSolverElemTemps;
    constraint->getOwner()->subConstraintInfo(constraint, info);
    island->m_numConstraints--;
    constraint->m_owner = nullptr;

    {
        const uint16_t slaveIndex = intern->m_slaveIndex;
        hkpEntity* slave = intern->m_entities[1 - intern->m_whoIsMaster];
        hkArray<hkpConstraintInstance*>& slaves = slave->m_constraintsSlave;
        hkpConstraintInstance* lastConstraint = slaves.m_data[slaves.m_size - 1];
        slaves.m_data[slaveIndex] = lastConstraint;
        slaves.popBack();
        lastConstraint->m_internal->m_slaveIndex = slaveIndex;
    }

    hkpEntity* master = intern->m_entities[intern->m_whoIsMaster];
    int32_t runtimeShift = 0;
    if (intern->m_runtime) {
        uint8_t* runtime = static_cast<uint8_t*>(intern->m_runtime);
        runtimeShift = -intern->m_runtimeSize;
        uint8_t* rest = runtime + intern->m_runtimeSize;
        hkString::memMove(
            runtime, rest,
            static_cast<int32_t>(master->m_constraintRuntime.m_data + master->m_constraintRuntime.m_size - rest)
        );
        master->m_constraintRuntime.m_size -= intern->m_runtimeSize;
        intern->m_runtime = nullptr;
    }
    constraint->m_internal = nullptr;

    hkSmallArray<hkConstraintInternal>& masters = master->m_constraintsMaster;
    hkConstraintInternal* lastInternal = &masters.m_data[masters.m_size - 1];
    for (hkConstraintInternal* i = intern; i < lastInternal; ++i) {
        *i = i[1];
        i->m_constraint->m_internal = i;
        i->m_runtime = i->m_runtime ? static_cast<uint8_t*>(i->m_runtime) + runtimeShift : nullptr;
    }
    masters.m_size--;
    constraint->removeReference();
}

// Inserts the modifier atom s into the constraint's modifier chain and adds what it needs to the owner's and the
// constraint's solver info. A moving-surface atom goes after the last modifier, an atom that already has an ignore
// modifier goes inside it; otherwise s becomes the chain's head and wraps the constraint's own atoms, which are the
// data's until the constraint has an internal record.
void hkpWorldConstraintUtil::addModifier(
    hkpConstraintInstance* instance,
    hkpConstraintOwner& constraintOwner,
    hkpModifierConstraintAtom* newModifier
) {
    constraintOwner.checkAccessRw();
    hkpModifierConstraintAtom* lastModifier = findLastModifier(instance);
    hkConstraintInternal* ci = instance->m_internal;
    hkpConstraintData::ConstraintInfo info;
    if (newModifier->m_type.m_storage == hkpConstraintAtom::TYPE_MODIFIER_MOVING_SURFACE && lastModifier) {
        newModifier->m_child = lastModifier->m_child;
        newModifier->m_childSize = lastModifier->m_childSize;
        lastModifier->m_child = newModifier;
        lastModifier->m_childSize = newModifier->m_modifierAtomSize;
    }
    else if (
        instance->m_constraintModifiers &&
        instance->m_constraintModifiers->m_type.m_storage == hkpConstraintAtom::TYPE_MODIFIER_IGNORE_CONSTRAINT
    ) {
        newModifier->m_child = instance->m_constraintModifiers->m_child;
        newModifier->m_childSize = instance->m_constraintModifiers->m_childSize;
        instance->m_constraintModifiers->m_child = newModifier;
        instance->m_constraintModifiers->m_childSize = newModifier->m_modifierAtomSize;
    }
    else {
        instance->m_constraintModifiers = newModifier;
        if (ci) {
            newModifier->m_child = ci->m_atoms;
            newModifier->m_childSize = ci->m_atomsSize;
            ci->m_atoms = newModifier;
            ci->m_atomsSize = newModifier->m_modifierAtomSize;
        }
        else {
            // The constraint is not in a world yet: its data still holds the atoms, and only the first four fields
            // of the info (its hkpConstraintInfo part) are zeroed.
            instance->m_data->getConstraintInfo(info);
            newModifier->m_child = info.m_atoms;
            newModifier->m_childSize = static_cast<uint16_t>(info.m_sizeOfAllAtoms);
            return;
        }
    }
    if (!ci) {
        return;
    }
    info.m_maxSizeOfSchema = 0;
    info.m_sizeOfSchemas = 0;
    info.m_numSolverResults = 0;
    info.m_numSolverElemTemps = 0;
    uint8_t usedFlags = 0;
    int32_t callbackRequest = newModifier->addModifierDataToConstraintInfo(info, usedFlags);
    constraintOwner.addConstraintInfo(instance, info);
    const uint8_t flags =
        static_cast<uint8_t>(
            instance->m_entities[1]->m_responseModifierFlags | instance->m_entities[0]->m_responseModifierFlags
        ) &
        usedFlags;
    const uint16_t additional = hkpResponseModifier::tableOfAdditionalSchemaSizes[flags & 7];
    constraintOwner.m_constraintInfo.m_sizeOfSchemas -= additional;
    // m_internal is reloaded after addConstraintInfo for each of the two stores, and tested before the second.
    instance->m_internal->m_sizeOfSchemas -= additional;
    if (hkConstraintInternal* internal = instance->m_internal) {
        internal->m_callbackRequest |= static_cast<uint8_t>(callbackRequest);
    }
}

// The SDK takes the atoms as const and stores them in the constraint's non-const atom pointers; an atom that heads
// the modifier chain is a modifier.
void hkpWorldConstraintUtil::updateFatherOfMovedAtom(
    hkpConstraintInstance* instance,
    const hkpConstraintAtom* oldAtom,
    const hkpConstraintAtom* updatedAtom,
    int32_t updatedSizeOfAtom
) {
    hkpModifierConstraintAtom* modifier = instance->m_constraintModifiers;
    hkConstraintInternal* cInternal = instance->m_internal;
    if (modifier) {
        if (modifier == oldAtom) {
            instance->m_constraintModifiers =
                static_cast<hkpModifierConstraintAtom*>(const_cast<hkpConstraintAtom*>(updatedAtom));
            if (cInternal) {
                cInternal->m_atoms = const_cast<hkpConstraintAtom*>(updatedAtom);
                cInternal->m_atomsSize = static_cast<uint16_t>(updatedSizeOfAtom);
            }
        }
        else {
            while (modifier->m_child != oldAtom) {
                modifier = static_cast<hkpModifierConstraintAtom*>(modifier->m_child);
            }
            modifier->m_child = const_cast<hkpConstraintAtom*>(updatedAtom);
            modifier->m_childSize = static_cast<uint16_t>(updatedSizeOfAtom);
        }
    }
    else if (cInternal) {
        cInternal->m_atoms = const_cast<hkpConstraintAtom*>(updatedAtom);
        cInternal->m_atomsSize = static_cast<uint16_t>(updatedSizeOfAtom);
    }
}

// The constraint between the two entities, looked up among the constraints each of them masters.
hkpConstraintInstance* hkpWorldConstraintUtil::getConstraint(const hkpEntity* entityA, const hkpEntity* entityB) {
    const hkpEntity* entities[2] = {entityA, entityB};
    for (int32_t e = 0; e < 2; ++e) {
        const hkpEntity* entity = entities[e];
        const hkpEntity* otherEntity = entities[1 - e];
        const hkSmallArray<hkConstraintInternal>& masters = entity->getConstraintMasters();
        for (int32_t i = 0; i < masters.m_size; ++i) {
            const hkConstraintInternal* c = &masters.m_data[i];
            if (c->getOtherEntity(entity) == otherEntity) {
                return c->m_constraint;
            }
        }
    }
    return nullptr;
}

// Unlinks the first modifier of the given type from the constraint's chain (stopping at the first atom that is no
// modifier), takes what it added out of the owner's and the constraint's solver info when the constraint is in a
// world, and frees it. The atom types are compared as words, unsigned.
void hkpWorldConstraintUtil::removeAndDeleteModifier(
    hkpConstraintInstance* instance,
    hkpConstraintOwner& constraintOwner,
    hkpConstraintAtom::AtomType type
) {
    constraintOwner.checkAccessRw();
    hkpModifierConstraintAtom* modifier = instance->m_constraintModifiers;
    if (!modifier) {
        return;
    }
    hkConstraintInternal* cInternal = instance->m_internal;
    if (modifier->m_type.m_storage == type) {
        hkpConstraintAtom* child = modifier->m_child;
        if (cInternal) {
            cInternal->m_atoms = child;
            cInternal->m_atomsSize = modifier->m_childSize;
        }
        instance->m_constraintModifiers = child->m_type.m_storage >= hkpConstraintAtom::TYPE_MODIFIER_SOFT_CONTACT
            ? static_cast<hkpModifierConstraintAtom*>(child)
            : nullptr;
    }
    else {
        hkpModifierConstraintAtom* father = modifier;
        hkpConstraintAtom* atom = modifier->m_child;
        for (;;) {
            if (atom->m_type.m_storage < hkpConstraintAtom::TYPE_MODIFIER_SOFT_CONTACT) {
                return;
            }
            modifier = static_cast<hkpModifierConstraintAtom*>(atom);
            if (modifier->m_type.m_storage == type) {
                break;
            }
            father = modifier;
            atom = modifier->m_child;
        }
        father->m_child = modifier->m_child;
        father->m_childSize = modifier->m_childSize;
    }
    if (cInternal) {
        hkpConstraintInfo info;
        info.clear();
        uint8_t usedFlags = 0;
        modifier->addModifierDataToConstraintInfo(info, usedFlags);
        usedFlags =
            static_cast<uint8_t>(
                instance->m_entities[1]->m_responseModifierFlags | instance->m_entities[0]->m_responseModifierFlags
            ) &
            usedFlags;
        const uint16_t additional = hkpResponseModifier::tableOfAdditionalSchemaSizes[usedFlags & 7];
        constraintOwner.m_constraintInfo.m_sizeOfSchemas += additional;
        instance->m_internal->m_sizeOfSchemas += additional;
        constraintOwner.subConstraintInfo(cInternal->m_constraint, info);
    }
    const int32_t modifierAtomSize = modifier->m_modifierAtomSize;
    hkMemoryRouter::getInstance().m_heap->blockFree(modifier, modifierAtomSize);
}
