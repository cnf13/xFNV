#pragma once

#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPointMaterial.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"

// The properties the dynamics keep for one contact point: the solver's results, the surface material and the
// engine's own word.
class hkpContactPointProperties : public hkpSolverResults, public hkContactPointMaterial {
public:
    using UserData = uint32_t;

    // The SDK's inline: the extended user data (the shape keys) follow the properties in the atom's untyped stream
    // memory.
    UserData* getStartOfExtendedUserData(const hkpSimpleContactConstraintAtom* atom) {
        return reinterpret_cast<UserData*>(this + 1);
    }

    float m_internalDataA;
};

// A contact point's properties inside an atom's property stream, followed by its extended user data; the stream's
// stride is the atom's, so it has no members and is read through hkpContactPointProperties.
class hkpContactPointPropertiesStream {
public:
    // The SDK's inline view of the entry as its leading properties: the stream is untyped atom memory.
    hkpContactPointProperties* asProperties() {
        return reinterpret_cast<hkpContactPointProperties*>(this);
    }

    const hkpContactPointProperties* asProperties() const {
        return reinterpret_cast<const hkpContactPointProperties*>(this);
    }
};

inline int32_t hkpSimpleContactConstraintAtom::getContactPointPropertiesStriding() const {
    return m_contactPointPropertiesStriding;
}

// The SDK's inline: the properties stream starts after the reserved contact points, in the atom's untyped memory.
inline hkpContactPointPropertiesStream*
hkpSimpleContactConstraintAtom::getContactPointPropertiesStream(int32_t i) const {
    const hkContactPoint* endContactPoint = getContactPoints() + m_numReservedContactPoints;
    const char* beginningOfProperties = reinterpret_cast<const char*>(endContactPoint);
    return const_cast<hkpContactPointPropertiesStream*>(reinterpret_cast<const hkpContactPointPropertiesStream*>(
        beginningOfProperties + i * getContactPointPropertiesStriding()
    ));
}
