#pragma once

#include "Havok/Source/Common/Base/DebugUtil/MultiThreadCheck/hkMultiThreadCheck.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"

// The lock that serialises reference counting of shared Havok objects across threads; its single instance is
// Havok's hkSingleton<hkReferencedObjectLock>.
class hkReferencedObjectLock : public hkReferencedObject, public hkSingleton<hkReferencedObjectLock> {
public:
    hkReferencedObjectLock();
    ~hkReferencedObjectLock() override;

    hkReferencedObject::LockMode m_lockMode;
    int32_t m_lockCount;
    hkMultiThreadCheck m_multiThreadCheck;
    hkCriticalSection m_criticalSection;

    // hkSingleton<hkReferencedObjectLock>::s_instance.
    static inline hkReferencedObjectLock* s_instance;
};
