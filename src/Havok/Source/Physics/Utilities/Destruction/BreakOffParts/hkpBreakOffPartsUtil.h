#pragma once

#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Collide/Deprecated/hkpCollisionListener.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpContactImpulseLimitBreachedListener.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldDeletionListener.h"

#include <stdint.h>

class hkCriticalSection;
class hkpBreakOffPartsListener;
class hkpEntity;
class hkpWorld;

// Breaks marked shape pieces off entities whose contact impulse exceeds their limit. Only partly declared: hkpEntity,
// which keeps an entity's util in m_limitContactImpulseUtil, only removes it as a contact listener and deletes it.
class hkpBreakOffPartsUtil : public hkReferencedObject,
                             public hkpContactImpulseLimitBreachedListener,
                             public hkpEntityListener,
                             public hkpWorldDeletionListener,
                             public hkpConstraintListener {
public:
    // The contact listener the util adds to each breakable entity: the impulse limits of its shape pieces.
    class LimitContactImpulseUtil : public hkReferencedObject, public hkpCollisionListener {
    public:
        ~LimitContactImpulseUtil() override;

        // The SDK's hkReferencedObject allocator; hkpCollisionListener declares an operator delete too, so the
        // lookup needs the class's own.
        static void operator delete(void* apMemory) {
            hkReferencedObject::deleteObject(apMemory);
        }

        // The SDK's HK_ALIGN16.
        alignas(16) hkpEntity* m_entity;
        hkUFloat8 m_maxImpulse;
        float m_maxConstraintImpulse;

    private:
        // The SDK's hkPointerMap<hkpShapeKey, hkUint8>.
        hkPointerMap<uint32_t, uint8_t> m_shapeKeyToMaxImpulse;
    };

protected:
    ~hkpBreakOffPartsUtil() override;

public:
    // Uses the hkReferencedObject allocator; every listener base declares an operator delete too, so the lookup needs
    // the class's own.
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }

    hkpWorld* m_world;
    hkCriticalSection* m_criticalSection;
    hkpBreakOffPartsListener* m_breakOffPartsListener;
};
