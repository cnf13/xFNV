#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInfo.h"

#include <stdint.h>

struct hkpConstraintAtom;
class hkpConstraintInstance;
class hkpConstraintQueryIn;
class hkpConstraintQueryOut;
class hkpSolverResults;

// The description of what a constraint does: its type and the atoms the solver runs. Each constraint kind derives from
// it.
class hkpConstraintData : public hkReferencedObject {
public:
    enum ConstraintType : int32_t {
        CONSTRAINT_TYPE_BALLANDSOCKET = 0x0,
        CONSTRAINT_TYPE_HINGE = 0x1,
        CONSTRAINT_TYPE_LIMITEDHINGE = 0x2,
        CONSTRAINT_TYPE_POINTTOPATH = 0x3,
        CONSTRAINT_TYPE_PRISMATIC = 0x6,
        CONSTRAINT_TYPE_RAGDOLL = 0x7,
        CONSTRAINT_TYPE_STIFFSPRING = 0x8,
        CONSTRAINT_TYPE_WHEEL = 0x9,
        CONSTRAINT_TYPE_GENERIC = 0xa,
        CONSTRAINT_TYPE_CONTACT = 0xb,
        CONSTRAINT_TYPE_BREAKABLE = 0xc,
        CONSTRAINT_TYPE_MALLEABLE = 0xd,
        CONSTRAINT_TYPE_POINTTOPLANE = 0xe,
        CONSTRAINT_TYPE_PULLEY = 0xf,
        CONSTRAINT_TYPE_ROTATIONAL = 0x10,
        CONSTRAINT_TYPE_HINGE_LIMITS = 0x12,
        CONSTRAINT_TYPE_RAGDOLL_LIMITS = 0x13,
        CONSTRAINT_TYPE_CUSTOM = 0x14,
        BEGIN_CONSTRAINT_CHAIN_TYPES = 0x64,
        CONSTRAINT_TYPE_STIFF_SPRING_CHAIN = 0x64,
        CONSTRAINT_TYPE_BALL_SOCKET_CHAIN = 0x65,
        CONSTRAINT_TYPE_POWERED_CHAIN = 0x66,
    };

    // The solver memory a constraint needs, with the atoms it is made of.
    struct ConstraintInfo : public hkpConstraintInfo {
        hkpConstraintAtom* m_atoms;
        uint32_t m_sizeOfAllAtoms;
    };

    // How much runtime memory a constraint needs and how many solver results it owns.
    struct RuntimeInfo {
        int32_t m_sizeOfExternalRuntime;
        int32_t m_numSolverResults;
    };

    hkpConstraintData() : m_userData(0) {}
    // The SDK's finish constructor: a loaded object keeps the fields the load wrote.
    hkpConstraintData(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag) {}

    virtual void setMaxLinearImpulse(float maxImpulse) {}
    // The largest impulse the constraint may apply: HK_REAL_MAX, 3.40282e+38f.
    virtual float getMaxLinearImpulse() const {
        return 3.40282e+38f;
    }
    virtual void setBodyToNotify(int32_t bodyIdx) {}
    virtual uint8_t getNotifiedBodyIndex() const {
        return 0xff;
    }
    // Whether the constraint's atoms are consistent.
    virtual hkBool isValid() const = 0;
    // The ConstraintType.
    virtual int32_t getType() const = 0;
    // Fills infoOut with the runtime this constraint needs.
    virtual void getRuntimeInfo(hkBool abWantRuntime, hkpConstraintData::RuntimeInfo& arInfoOut) const = 0;
    // The solver results kept in runtime; the base returns runtime.
    virtual hkpSolverResults* getSolverResults(void* runtime);
    // Clears the runtime memory.
    virtual void addInstance(hkpConstraintInstance* constraint, void* runtime, int32_t sizeOfRuntime) const;
    virtual void buildJacobian(const hkpConstraintQueryIn& in, hkpConstraintQueryOut& out) {}

    // Whether buildJacobianCallback must run; the base writes false into arResult.
    virtual hkBool isBuildJacobianCallbackRequired() const {
        return false;
    }

    virtual void buildJacobianCallback(const hkpConstraintQueryIn& in) {}
    // Fills arInfoOut with the solver memory the constraint needs.
    virtual void getConstraintInfo(hkpConstraintData::ConstraintInfo& arInfoOut) const = 0;
    ~hkpConstraintData() override {}

    uintptr_t getUserData() const {
        return m_userData;
    }
    void setUserData(uintptr_t data) {
        m_userData = data;
    }
    // The SDK's inline hook for when the constraint's instance is removed: nothing to do.
    void removeInstance(hkpConstraintInstance* constraint, void* runtime, int32_t sizeOfRuntime) const {}

    // Fills infoOut from the sizeOfAllAtoms bytes of atoms at atoms.
    static void getConstraintInfoUtil(const hkpConstraintAtom* atoms, int32_t sizeOfAllAtoms, ConstraintInfo& infoOut);

    // The SDK's hkUlong, a pointer-sized integer: bhkConstraint stores its hkConstraintCinfo's address here.
    uintptr_t m_userData;
};

// The class's reflection record.
extern hkClass hkpConstraintDataClass;
