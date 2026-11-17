#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkSmallArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintListener.h"

#include <xmmintrin.h>

class hkpConstraintData;
class hkpConstraintOwner;
class hkpEntity;
struct hkpModifierConstraintAtom;
class hkpRigidBody;
class hkpSimulationIsland;
class hkpWorld;
class hkTransform;

// A constraint between two entities.
class hkpConstraintInstance : public hkReferencedObject {
public:
    enum ConstraintPriority : int32_t {
        PRIORITY_INVALID = 0x0,
        PRIORITY_PSI = 0x1,
        PRIORITY_SIMPLIFIED_TOI_UNUSED = 0x2,
        PRIORITY_TOI = 0x3,
        PRIORITY_TOI_HIGHER = 0x4,
        PRIORITY_TOI_FORCED = 0x5,
    };

    enum OnDestructionRemapInfo : int32_t {
        ON_DESTRUCTION_REMAP = 0x0,
        ON_DESTRUCTION_REMOVE = 0x1,
        ON_DESTRUCTION_RESET_REMOVE = 0x2,
    };

    enum InstanceType : int32_t {
        TYPE_NORMAL = 0x0,
        TYPE_CHAIN = 0x1,
    };

    enum AddReferences : int32_t {
        DO_NOT_ADD_REFERENCES = 0x0,
        DO_ADD_REFERENCES = 0x1,
    };

    enum CloningMode : int32_t {
        CLONE_INSTANCES_ONLY = 0x0,
        CLONE_DATAS_WITH_MOTORS = 0x1,
        CLONE_FORCE_SHALLOW = 0x2,
    };

    // What the SDK substitutes for the listeners array when serializing: the small array's three words.
    class SmallArraySerializeOverrideType {
    public:
        static const hkClass& staticClass();

        void* m_data;
        uint16_t m_size;
        uint16_t m_capacityAndFlags;
    };

    // The reflection's member table: its dynamic initializer stores the two enum members' enums.
    using MembersArray = hkClassMember[12];

    hkpConstraintInstance(
        hkpEntity* apEntityA,
        hkpEntity* apEntityB,
        hkpConstraintData* apData,
        ConstraintPriority aePriority
    );
    // The constructor the chain instance builds on: no entities or data.
    hkpConstraintInstance(ConstraintPriority aePriority);

    // The SDK's finish constructor of a loaded object: an empty listener array, the name's finish constructor and
    // an invalid uid.
    hkpConstraintInstance(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_name(aFlag) {
        m_uid = 0xFFFFFFF0;
    }

    ~hkpConstraintInstance() override;

    virtual void entityAddedCallback(hkpEntity* entity);
    virtual void entityRemovedCallback(hkpEntity* entity);
    virtual void entityDeletedCallback(hkpEntity* entity);
    virtual hkpConstraintInstance::InstanceType getType() const {
        return TYPE_NORMAL;
    }

    // Re-enables the constraint through hkpResponseModifier::enableConstraint.
    void enable();

    // Disables the constraint through hkpResponseModifier::disableConstraint.
    void disable();

    // The rest of the hkpConstraintInstance members. The accessors are inline; the others are declared only.
    hkpEntity* getEntityA() const {
        return m_entities[0];
    }
    hkpEntity* getEntityB() const {
        return m_entities[1];
    }
    hkpEntity* getEntity(int32_t aiIndex) const {
        return m_entities[aiIndex];
    }
    const hkpConstraintData* getData() const {
        return m_data;
    }
    hkpConstraintData* getDataRw() const {
        return m_data;
    }
    hkpModifierConstraintAtom* getConstraintModifiers() const {
        return m_constraintModifiers;
    }
    hkUlong getUserData() const {
        return m_userData;
    }
    void setUserData(hkUlong data) {
        m_userData = data;
    }
    hkpConstraintOwner* getOwner() const {
        return m_owner;
    }
    void setOwner(hkpConstraintOwner* owner) {
        m_owner = owner;
    }
    hkConstraintInternal* getInternal() {
        return m_internal;
    }
    const char* getName() const {
        return m_name.cString();
    }
    hkpRigidBody* getRigidBodyA() const {
        return reinterpret_cast<hkpRigidBody*>(m_entities[0]);
    }
    hkpRigidBody* getRigidBodyB() const {
        return reinterpret_cast<hkpRigidBody*>(m_entities[1]);
    }

    // The SDK's hkSelectOther: the entity that is not apEntity, by xor of the three addresses.
    hkpEntity* getOtherEntity(const hkpEntity* entity) const {
        return reinterpret_cast<hkpEntity*>(
            reinterpret_cast<uintptr_t>(entity) ^ reinterpret_cast<uintptr_t>(m_entities[0]) ^
            reinterpret_cast<uintptr_t>(m_entities[1])
        );
    }

    hkpEntity* getMasterEntity() const {
        return m_internal->m_entities[m_internal->m_whoIsMaster];
    }
    hkpEntity* getSlaveEntity() const {
        return m_internal->m_entities[1 - m_internal->m_whoIsMaster];
    }
    // The solver's runtime memory of the constraint (the SDK's getRuntime reads it from the internal data).
    void* getRuntime() const {
        return m_internal->m_runtime;
    }
    ConstraintPriority getPriority() const {
        return static_cast<ConstraintPriority>(m_priority.m_storage);
    }
    hkpConstraintInstance* clone(hkpEntity* newEntityA, hkpEntity* newEntityB, CloningMode mode) const;
    void replaceEntity(hkpEntity* oldEntity, hkpEntity* newEntity);
    hkBool* isConstrainedToWorld(hkBool* arResult) const;
    hkpSimulationIsland* getSimulationIsland();
    void getPivotsInWorld(hkVector4& pivotAinW, hkVector4& pivotBinW) const;
    void addConstraintListener(hkpConstraintListener* listener);
    void removeConstraintListener(hkpConstraintListener* listener);
    void setWantRuntime(hkBool b) {
        m_wantRuntime = b;
    }
    hkBool getWantRuntime() const {
        return m_wantRuntime;
    }
    void setPriority(ConstraintPriority priority);
    void setName(const char* name) {
        m_name = name;
    }
    void setVirtualMassInverse(const hkVector4& invMassA, const hkVector4& invMassB);
    void setEnabled(hkBool state);
    hkBool* isEnabled(hkBool* arResult);
    void pointNullsToFixedRigidBody();
    void setFixedRigidBodyPointersToZero(hkpWorld* world);
    hkpConstraintData* getDataCalled() const {
        return m_data;
    }
    hkpEntity* getEntityACalled() const {
        return m_entities[0];
    }
    // Same as getEntityB.
    hkpEntity* getEntityBCalled() const {
        return m_entities[1];
    }
    bool* isEnabled(bool* a1);
    // Switches on the data's type and calls each constraint data's setInBodySpace; hkpPhysicsSystem's transform calls
    // it per constraint with its own transform.
    void transform(const hkTransform& transformation);

    static const hkClass& staticClass();

    // The island or world that holds the constraint; null until it is added.
    hkpConstraintOwner* m_owner;
    hkpConstraintData* m_data;
    hkpModifierConstraintAtom* m_constraintModifiers;
    // The constrained entities; the second may be null for a constraint to the world.
    hkpEntity* m_entities[2];
    hkEnum<ConstraintPriority, uint8_t> m_priority;
    hkBool m_wantRuntime;
    hkEnum<OnDestructionRemapInfo, uint8_t> m_destructionRemapInfo;
    uint8_t padding1F;
    hkSmallArray<hkpConstraintListener*> m_listeners;
    hkStringPtr m_name;
    // The game keeps its bhkConstraint wrapper's address here (bhkConstraint::Link).
    hkUlong m_userData;
    hkConstraintInternal* m_internal;
    uint32_t m_uid;

    static MembersArray Members;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpConstraintInstanceSmallArraySerializeOverrideTypeTypeInfo;
