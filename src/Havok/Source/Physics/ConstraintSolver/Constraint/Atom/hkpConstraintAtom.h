#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpSimpleContactConstraintInfo.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkContactPoint;
struct hkpBodyVelocity;
class hkpConstraintData;
struct hkpConstraintInfo;
class hkpConstraintQueryIn;
class hkpConstraintQueryOut;
class hkpContactPointPropertiesStream;
struct hkpSimpleConstraintInfoInitInput;

void hkBridgeConstraintAtom_callData(
    hkpConstraintData* m_constraintData,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
);

// The base of the atoms a constraint is built from: every atom starts with its type, which the solver switches on.
struct hkpConstraintAtom {
    enum AtomType : int32_t {
        TYPE_INVALID = 0x0,
        TYPE_BRIDGE = 0x1,
        TYPE_SET_LOCAL_TRANSFORMS = 0x2,
        TYPE_SET_LOCAL_TRANSLATIONS = 0x3,
        TYPE_SET_LOCAL_ROTATIONS = 0x4,
        TYPE_BALL_SOCKET = 0x5,
        TYPE_STIFF_SPRING = 0x6,
        TYPE_LIN = 0x7,
        TYPE_LIN_SOFT = 0x8,
        TYPE_LIN_LIMIT = 0x9,
        TYPE_LIN_FRICTION = 0xa,
        TYPE_LIN_MOTOR = 0xb,
        TYPE_2D_ANG = 0xc,
        TYPE_ANG = 0xd,
        TYPE_ANG_LIMIT = 0xe,
        TYPE_TWIST_LIMIT = 0xf,
        TYPE_CONE_LIMIT = 0x10,
        TYPE_ANG_FRICTION = 0x11,
        TYPE_ANG_MOTOR = 0x12,
        TYPE_RAGDOLL_MOTOR = 0x13,
        TYPE_PULLEY = 0x14,
        TYPE_OVERWRITE_PIVOT = 0x15,
        TYPE_CONTACT = 0x16,
        TYPE_MODIFIER_SOFT_CONTACT = 0x17,
        TYPE_MODIFIER_MASS_CHANGER = 0x18,
        TYPE_MODIFIER_VISCOUS_SURFACE = 0x19,
        TYPE_MODIFIER_MOVING_SURFACE = 0x1a,
        TYPE_MODIFIER_IGNORE_CONSTRAINT = 0x1b,
        TYPE_MODIFIER_CENTER_OF_MASS_CHANGER = 0x1c,
        TYPE_MAX = 0x1d,
    };

    enum CallbackRequest : int32_t {
        CALLBACK_REQUEST_NONE = 0x0,
        CALLBACK_REQUEST_NEW_CONTACT_POINT = 0x1,
        CALLBACK_REQUEST_SETUP_PPU_ONLY = 0x2,
        CALLBACK_REQUEST_SETUP_CALLBACK = 0x4,
        CALLBACK_REQUEST_CONTACT_POINT_CALLBACK = 0x8,
    };

    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    int32_t isModifierType() const {
        return m_type.m_storage >= TYPE_MODIFIER_SOFT_CONTACT;
    }

    hkEnum<AtomType, uint16_t> m_type;
};

// The base of the atoms that change how a constraint behaves: the atom's size, the size of the atom it wraps and the
// wrapped atom. The data starts on a 16-byte boundary, so the 14 bytes after the type are the compiler's padding and
// every atom built on this one is 16-byte aligned.
struct hkpModifierConstraintAtom : public hkpConstraintAtom {
    // Adds what this modifier needs to cinfo and ORs its callback flags into usedFlagsOut,
    // returning the callback request the constraint's internal record takes.
    int32_t addModifierDataToConstraintInfo(hkpConstraintInfo& cinfo, uint8_t& usedFlagsOut) const;

    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    static int32_t addAllModifierDataToConstraintInfo(
        hkpModifierConstraintAtom* modifier,
        hkpConstraintInfo& cinfo,
        uint8_t& usedFlagsOut
    );
    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    alignas(16) uint16_t m_modifierAtomSize;
    uint16_t m_childSize;
    hkpConstraintAtom* m_child;
    uint32_t m_pad[2];
};

// The modifier that disables the constraint it wraps (TYPE_MODIFIER_IGNORE_CONSTRAINT). It adds no data.
struct hkpIgnoreModifierConstraintAtom : public hkpModifierConstraintAtom {
    hkpIgnoreModifierConstraintAtom() {
        m_type.m_storage = TYPE_MODIFIER_IGNORE_CONSTRAINT;
        m_modifierAtomSize = sizeof(hkpIgnoreModifierConstraintAtom);
    }

    static void* operator new(size_t size) noexcept {
        return hkMemoryRouter::getInstance().heap().blockAlloc(static_cast<int32_t>(size));
    }

    // Takes the wrapped atoms' solver needs back out of info, unless the innermost child is a contact or bridge
    // atom (types 22 and 1); returns 0.
    int32_t getConstraintInfo(hkpConstraintInfo& info) const;

    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();
};

// The modifiers a contact constraint's end-of-collision-response callback reaches.
// The modifier that softens a contact (SDK: TYPE_MODIFIER_SOFT_CONTACT).
struct hkpSoftContactModifierConstraintAtom : public hkpModifierConstraintAtom {
    hkpSoftContactModifierConstraintAtom() {
        m_tau = 0.1f;
        m_type.m_storage = TYPE_MODIFIER_SOFT_CONTACT;
        m_modifierAtomSize = sizeof(hkpSoftContactModifierConstraintAtom);
        m_maxAcceleration = 20.0f;
    }

    static void* operator new(size_t size) noexcept {
        return hkMemoryRouter::getInstance().heap().blockAlloc(static_cast<int32_t>(size));
    }

    // Restores the bodies' velocities after the collision response.
    // The SDK's name (`names.py search collisionResponseEndCallback` finds no member of the atoms).
    // Havok SDK signature (hkpDynamics.lib, tied by reloc).
    void collisionResponseEndCallback(
        const hkContactPoint& cp,
        float impulseApplied,
        hkpSimpleConstraintInfoInitInput& inA,
        hkpBodyVelocity& velA,
        hkpSimpleConstraintInfoInitInput& inB,
        hkpBodyVelocity& velB
    );
    // Saves velA and velB (their linear and angular vectors) into s_savedLinearVelocityA..s_savedAngularVelocityB,
    // which collisionResponseEndCallback blends back in by m_tau.
    // hkpSimpleContactConstraintData::collisionResponseBeginCallback calls it for TYPE_MODIFIER_SOFT_CONTACT atoms.
    void collisionResponseBeginCallback(
        const hkContactPoint& cp,
        hkpSimpleConstraintInfoInitInput& inA,
        hkpBodyVelocity& velA,
        hkpSimpleConstraintInfoInitInput& inB,
        hkpBodyVelocity& velB
    );

    static const hkClass& staticClass();

    // Members from the SDK; offsets from hkSolverBuildJacobianFromAtomsNotContact.
    float m_tau;
    float m_maxAcceleration;
};

// The modifier that damps a contact's rolling (SDK: TYPE_MODIFIER_VISCOUS_SURFACE). It adds no data.
struct hkpViscousSurfaceModifierConstraintAtom : public hkpModifierConstraintAtom {
    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();
};

// The modifier that scales the inverse masses the solver sees for the constraint's two bodies (SDK:
// TYPE_MODIFIER_MASS_CHANGER).
struct hkpMassChangerModifierConstraintAtom : public hkpModifierConstraintAtom {
    hkpMassChangerModifierConstraintAtom() {
        m_type.m_storage = TYPE_MODIFIER_MASS_CHANGER;
        m_modifierAtomSize = sizeof(hkpMassChangerModifierConstraintAtom);
    }

    static void* operator new(size_t size) noexcept {
        return hkMemoryRouter::getInstance().heap().blockAlloc(static_cast<int32_t>(size));
    }

    // Does nothing.
    void collisionResponseEndCallback(
        const hkContactPoint& cp,
        float impulseApplied,
        hkpSimpleConstraintInfoInitInput& inA,
        hkpBodyVelocity& velA,
        hkpSimpleConstraintInfoInitInput& inB,
        hkpBodyVelocity& velB
    );
    // Havok SDK signature (hkpDynamics.lib, tied by reloc).
    void collisionResponseBeginCallback(
        const hkContactPoint& cp,
        hkpSimpleConstraintInfoInitInput& inA,
        hkpBodyVelocity& velA,
        hkpSimpleConstraintInfoInitInput& inB,
        hkpBodyVelocity& velB
    );

    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    hkVector4 m_factorA;
    hkVector4 m_factorB;
};

// The modifier that gives a contact surface a velocity (SDK: TYPE_MODIFIER_MOVING_SURFACE).
struct hkpMovingSurfaceModifierConstraintAtom : public hkpModifierConstraintAtom {
    hkpMovingSurfaceModifierConstraintAtom() {
        m_type.m_storage = TYPE_MODIFIER_MOVING_SURFACE;
        m_modifierAtomSize = sizeof(hkpMovingSurfaceModifierConstraintAtom);
    }

    static void* operator new(size_t size) noexcept {
        return hkMemoryRouter::getInstance().heap().blockAlloc(static_cast<int32_t>(size));
    }

    // Takes the surface velocity back out of the bodies' velocities.
    // The SDK's name.
    // Havok SDK signature (hkpDynamics.lib, tied by reloc).
    void collisionResponseEndCallback(
        const hkContactPoint& cp,
        float impulseApplied,
        hkpSimpleConstraintInfoInitInput& inA,
        hkpBodyVelocity& velA,
        hkpSimpleConstraintInfoInitInput& inB,
        hkpBodyVelocity& velB
    );
    // Adds the surface velocity, less its part along the contact normal, to velB's linear velocity.
    // hkpSimpleContactConstraintData::collisionResponseBeginCallback calls it for TYPE_MODIFIER_MOVING_SURFACE atoms.
    void collisionResponseBeginCallback(
        const hkContactPoint& cp,
        hkpSimpleConstraintInfoInitInput& inA,
        hkpBodyVelocity& velA,
        hkpSimpleConstraintInfoInitInput& inB,
        hkpBodyVelocity& velB
    );

    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    // The SDK's member; offset from hkSolverBuildJacobianFromAtomsNotContact.
    hkVector4 m_velocity;
};

// The modifier that moves the centres of mass the solver sees for the constraint's two bodies (SDK:
// TYPE_MODIFIER_CENTER_OF_MASS_CHANGER).
struct hkpCenterOfMassChangerModifierConstraintAtom : public hkpModifierConstraintAtom {
    hkpCenterOfMassChangerModifierConstraintAtom() {
        m_type.m_storage = TYPE_MODIFIER_CENTER_OF_MASS_CHANGER;
        m_modifierAtomSize = sizeof(hkpCenterOfMassChangerModifierConstraintAtom);
    }

    static void* operator new(size_t size) noexcept {
        return hkMemoryRouter::getInstance().heap().blockAlloc(static_cast<int32_t>(size));
    }

    // Restores the bodies' centres of mass after the collision response.
    // The SDK's name.
    // Havok SDK signature (hkpDynamics.lib, tied by reloc).
    void collisionResponseEndCallback(
        const hkContactPoint& cp,
        float impulseApplied,
        hkpSimpleConstraintInfoInitInput& inA,
        hkpBodyVelocity& velA,
        hkpSimpleConstraintInfoInitInput& inB,
        hkpBodyVelocity& velB
    );
    // Havok SDK signature (hkpDynamics.lib, tied by reloc).
    void collisionResponseBeginCallback(
        const hkContactPoint& cp,
        hkpSimpleConstraintInfoInitInput& inA,
        hkpBodyVelocity& velA,
        hkpSimpleConstraintInfoInitInput& inB,
        hkpBodyVelocity& velB
    );

    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    hkVector4 m_displacementA;
    hkVector4 m_displacementB;
};

// The atom of a simple contact constraint: the counts of its contact points and their properties, which follow the
// atom in memory, and the constraint's info. The data starts on a 16-byte boundary (m_info's alignment).
struct hkpSimpleContactConstraintAtom : public hkpConstraintAtom {
    // The finish constructor hkpSimpleContactConstraintAtom(hkFinishLoadedObjectFlag), when finishing, sets
    // m_contactPointPropertiesStriding to 4 * (5 + m_numUserDatasForBodyA + m_numUserDatasForBodyB).
    // finishLoadedObjecthkpSerializedAgentNnEntry runs it on hkpSerializedAgentNnEntry::m_atom. The finish constructor
    // hides the implicit default one, which the atom's holders need.
    hkpSimpleContactConstraintAtom() = default;
    hkpSimpleContactConstraintAtom(hkFinishLoadedObjectFlag f);

    // The SDK's inline getContactPoints: the contact points follow the atom in its untyped stream memory.
    hkContactPoint* getContactPoints() const {
        return const_cast<hkContactPoint*>(reinterpret_cast<const hkContactPoint*>(this + 1));
    }

    // The SDK's inline property stream accessors: the properties follow the reserved contact points, one per
    // m_contactPointPropertiesStriding bytes. Defined in hkpContactPointProperties.h, as in the SDK.
    inline int32_t getContactPointPropertiesStriding() const;
    inline hkpContactPointPropertiesStream* getContactPointPropertiesStream(int32_t i) const;

    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    uint16_t m_sizeOfAllAtoms;
    uint16_t m_numContactPoints;
    uint16_t m_numReservedContactPoints;
    uint8_t m_numUserDatasForBodyA;
    uint8_t m_numUserDatasForBodyB;
    uint8_t m_contactPointPropertiesStriding;
    uint16_t m_maxNumContactPoints;
    hkpSimpleContactConstraintDataInfo m_info;
};

inline hkQuadReal s_savedLinearVelocityA{};
inline hkQuadReal s_savedAngularVelocityA{};
inline hkQuadReal s_savedLinearVelocityB{};
inline hkQuadReal s_savedAngularVelocityB{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpConstraintAtomTypeInfo;
extern hkTypeInfo hkpSimpleContactConstraintAtomTypeInfo;
extern hkTypeInfo hkpModifierConstraintAtomTypeInfo;
extern hkTypeInfo hkpSoftContactModifierConstraintAtomTypeInfo;
extern hkTypeInfo hkpMassChangerModifierConstraintAtomTypeInfo;
extern hkTypeInfo hkpCenterOfMassChangerModifierConstraintAtomTypeInfo;
extern hkTypeInfo hkpViscousSurfaceModifierConstraintAtomTypeInfo;
extern hkTypeInfo hkpMovingSurfaceModifierConstraintAtomTypeInfo;
extern hkTypeInfo hkpIgnoreModifierConstraintAtomTypeInfo;
extern hkClass hkpConstraintAtomClass;
