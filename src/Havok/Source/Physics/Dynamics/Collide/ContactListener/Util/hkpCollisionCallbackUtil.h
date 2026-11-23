#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/Util/hkpEndOfStepCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldDeletionListener.h"

class hkpConstraintInstance;
class hkpWorld;

// The per-world helper that gives contact managers the end of step callbacks: it counts the users of the world's
// instance and watches the constraints and the world for the managers they create.
class hkpCollisionCallbackUtil : public hkReferencedObject,
                                 public hkpConstraintListener,
                                 public hkpWorldDeletionListener {
public:
    hkpCollisionCallbackUtil();
    ~hkpCollisionCallbackUtil() override;

    // hkpConstraintListener's vtable at +0x8.
    void constraintAddedCallback(hkpConstraintInstance* constraint) override;
    // hkpConstraintListener's vtable at +0x8.
    void constraintRemovedCallback(hkpConstraintInstance* constraint) override;
    // hkpWorldDeletionListener's vtable at +0xC.
    void worldDeletedCallback(hkpWorld* world) override;

    // hkReferencedObject's, which the listener bases' own would make ambiguous.
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }
    // Havok SDK. Counts one more user of world's callback util, creating and installing it for the first.
    static void addCollisionCallbackUtil(hkpWorld* world);
    // Havok SDK. Drops one user of world's callback util, removing and releasing it after the last.
    static void removeCollisionCallbackUtil(hkpWorld* world);

    uint32_t m_count;
    hkpEndOfStepCallbackUtil m_endOfStepCallbackUtil;
};
