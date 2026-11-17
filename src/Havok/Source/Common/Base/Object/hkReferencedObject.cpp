#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/DebugUtil/MultiThreadCheck/hkMultiThreadCheck.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/Util/hkStatisticsCollectorUtil.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObjectLock.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "SDK/MSVC/stdlib.h"

static void finishLoadedObjecthkReferencedObject(void* p, int32_t finishing);
// Havok SDK signature (hkBase.lib, tied by reloc,target).
static const void* getVtablehkReferencedObject();
static void cleanupLoadedObjecthkReferencedObject(void* p);

// The hkBaseObject record deliberately carries hkReferencedObject's type descriptor and size, as the game does.
hkTypeInfo hkBaseObjectTypeInfo = {
    "hkBaseObject",          finishLoadedObjecthkBaseObject, cleanupLoadedObjecthkBaseObject,
    getVtablehkBaseObject(), &typeid(hkReferencedObject),    sizeof(hkReferencedObject)
};
hkThreadLocalData<int32_t> s_intLockCount;

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode hkSingletonRegisterhkReferencedObjectLock(
    "hkReferencedObjectLock",
    &hkReferencedObjectLockcreate,
    reinterpret_cast<void**>(&hkReferencedObjectLock::s_instance)
);

// The class record's member table.
using hkReferencedObjectClassMemberArray = const hkClassMember[2];
static hkReferencedObjectClassMemberArray hkReferencedObjectClass_Members{
    {"memSizeAndFlags",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT16},
     {hkClassMember::TYPE_VOID},
     0,
     hkClassMember::SERIALIZE_IGNORED,
     0x4,
     nullptr},
    {"referenceCount",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT16},
     {hkClassMember::TYPE_VOID},
     0,
     hkClassMember::SERIALIZE_IGNORED,
     0x6,
     nullptr}
};
hkClass hkReferencedObjectClass(
    "hkReferencedObject",
    &hkBaseObjectClass,
    8,
    nullptr,
    0,
    nullptr,
    0,
    hkReferencedObjectClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkReferencedObjectTypeInfo = {
    "hkReferencedObject",          finishLoadedObjecthkReferencedObject, cleanupLoadedObjecthkReferencedObject,
    getVtablehkReferencedObject(), &typeid(hkReferencedObject),          sizeof(hkReferencedObject)
};

// The type info's finish-loaded-object callback for hkReferencedObject: runs its finish constructor.
static void finishLoadedObjecthkReferencedObject(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag aFlag;
        aFlag.m_finishing = finishing;
        ::new (p) hkReferencedObject(aFlag);
    }
}

static const void* getVtablehkReferencedObject() {
    return hkGetVtableOf<hkReferencedObject>();
}

static void cleanupLoadedObjecthkReferencedObject(void* p) {
    static_cast<hkReferencedObject*>(p)->~hkReferencedObject();
}

// Adds the object's members to the collector when its class is given, is not hkReferencedObject itself and has
// contents.
void hkReferencedObject::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    if (cls && cls != &hkReferencedObjectClass) {
        hkBool bHasContents;
        if (hkStatisticsCollectorUtil::hasContents(&bHasContents, *cls)->m_bool) {
            hkStatisticsCollectorUtil::addClassContentsAll(this, *cls, collector);
        }
    }
}

// The reference count is only touched when the object carries a size and flags word; the last reference deletes the
// object through its deleting destructor.
void hkReferencedObject::removeReferenceLockUnchecked() const {
    if (m_memSizeAndFlags != 0) {
        hkReferencedObject* pThis = const_cast<hkReferencedObject*>(this);
        --pThis->m_referenceCount;
        if (pThis->m_referenceCount == 0) {
            delete pThis;
        }
    }
}

// Takes one more reference without the lock; objects without a size and flags word are not counted. Const in the
// KEEP: the exe's body touches only ecx and the flags, and its caller addReferences (0xC90670) keeps its cursor in
// eax and its count in edx across the call; clang does not guarantee either register.
void hkReferencedObject::addReferenceLockUnchecked() const {
    if (m_memSizeAndFlags != 0) {
        ++const_cast<hkReferencedObject*>(this)->m_referenceCount;
    }
}

// Drops one reference; the last one deletes the object through its deleting destructor without the lock. Objects
// without a size and flags word are not counted. In auto lock mode a thread that does not hold the lock yet takes
// it around the decrement.
void hkReferencedObject::removeReference() const {
    if (m_memSizeAndFlags != 0) {
        if (m_referenceCount == 1) {
            m_referenceCount = 0;
            delete this;
            return;
        }
        hkReferencedObjectLock* pLock = hkReferencedObjectLock::s_instance;
        if (pLock->m_lockMode != LOCK_MODE_AUTO || TlsGetValue(s_intLockCount.m_slotID) == (void*)0x23DF4554) {
            --m_referenceCount;
        }
        else {
            // EXE-INLINED 0xC90490
            if (TlsGetValue(s_intLockCount.m_slotID) == (void*)0x23DF4554) {
                ++pLock->m_lockCount;
            }
            else {
                EnterCriticalSection(&pLock->m_criticalSection.m_section);
                pLock->m_lockCount = 1;
                TlsSetValue(s_intLockCount.m_slotID, (void*)0x23DF4554);
            }
            --m_referenceCount;
            // EXE-INLINED 0xC904E0
            if (--pLock->m_lockCount <= 0) {
                TlsSetValue(s_intLockCount.m_slotID, nullptr);
                LeaveCriticalSection(&pLock->m_criticalSection.m_section);
            }
        }
    }
}

// Takes one more reference; objects without a size and flags word are not counted. In auto lock mode a thread that
// does not hold the lock yet takes it around the increment.
void hkReferencedObject::addReference() const {
    if (m_memSizeAndFlags != 0) {
        hkReferencedObjectLock* pLock = hkReferencedObjectLock::s_instance;
        if (pLock->m_lockMode != LOCK_MODE_AUTO || TlsGetValue(s_intLockCount.m_slotID) == (void*)0x23DF4554) {
            ++m_referenceCount;
        }
        else {
            // EXE-INLINED 0xC90490
            if (TlsGetValue(s_intLockCount.m_slotID) == (void*)0x23DF4554) {
                ++pLock->m_lockCount;
            }
            else {
                EnterCriticalSection(&pLock->m_criticalSection.m_section);
                pLock->m_lockCount = 1;
                TlsSetValue(s_intLockCount.m_slotID, (void*)0x23DF4554);
            }
            ++m_referenceCount;
            // EXE-INLINED 0xC904E0
            if (--pLock->m_lockCount <= 0) {
                TlsSetValue(s_intLockCount.m_slotID, nullptr);
                LeaveCriticalSection(&pLock->m_criticalSection.m_section);
            }
        }
    }
}

// Takes one more reference on each object of the run, without the lock when the thread already holds it (or when
// locking is off), otherwise inside the lock with the thread's nesting slot set.
void hkReferencedObject::addReferences(
    const hkReferencedObject* const* objects,
    int32_t numObjects,
    int32_t pointerStriding
) {
    hkReferencedObjectLock* pLock = hkReferencedObjectLock::s_instance;
    // The run is untyped memory walked at a byte stride, reading a pointer at each step.
    if (pLock->m_lockMode == LOCK_MODE_NONE || TlsGetValue(s_intLockCount.m_slotID) == (void*)0x23DF4554) {
        const char* pCursor = reinterpret_cast<const char*>(objects);
        for (int32_t i = numObjects; i > 0; --i) {
            (*reinterpret_cast<hkReferencedObject* const*>(pCursor))->addReferenceLockUnchecked();
            pCursor += pointerStriding;
        }
        return;
    }
    // EXE-INLINED 0xC90490
    if (TlsGetValue(s_intLockCount.m_slotID) == (void*)0x23DF4554) {
        ++pLock->m_lockCount;
    }
    else {
        EnterCriticalSection(&pLock->m_criticalSection.m_section);
        pLock->m_lockCount = 1;
        TlsSetValue(s_intLockCount.m_slotID, (void*)0x23DF4554);
    }
    const char* pCursor = reinterpret_cast<const char*>(objects);
    for (int32_t i = numObjects; i > 0; --i) {
        (*reinterpret_cast<hkReferencedObject* const*>(pCursor))->addReferenceLockUnchecked();
        pCursor += pointerStriding;
    }
    // EXE-INLINED 0xC904E0
    if (--pLock->m_lockCount <= 0) {
        TlsSetValue(s_intLockCount.m_slotID, nullptr);
        LeaveCriticalSection(&pLock->m_criticalSection.m_section);
    }
}

// Takes the reference lock for the calling thread: a thread that already holds it only nests once more.
void hkReferencedObject::lockAll() {
    hkReferencedObjectLock* pLock = hkReferencedObjectLock::s_instance;
    if (TlsGetValue(s_intLockCount.m_slotID) == (void*)0x23DF4554) {
        ++pLock->m_lockCount;
    }
    else {
        EnterCriticalSection(&pLock->m_criticalSection.m_section);
        pLock->m_lockCount = 1;
        TlsSetValue(s_intLockCount.m_slotID, (void*)0x23DF4554);
    }
}

void hkReferencedObject::unlockAll() {
    hkReferencedObjectLock* v0 = hkReferencedObjectLock::s_instance;
    --hkReferencedObjectLock::s_instance->m_lockCount;
    if (v0->m_lockCount <= 0) {
        TlsSetValue(s_intLockCount.m_slotID, 0);
        LeaveCriticalSection(&v0->m_criticalSection.m_section);
    }
}

void hkReferencedObject::setLockMode(hkReferencedObject::LockMode a1) {
    hkReferencedObjectLock::s_instance->m_lockMode = static_cast<hkReferencedObject::LockMode>((int32_t)a1);
}

hkReferencedObject::LockMode hkReferencedObject::getLockMode() {
    return static_cast<hkReferencedObject::LockMode>((uint32_t)hkReferencedObjectLock::s_instance->m_lockMode);
}

hkCriticalSection* hkReferencedObject::getLockCriticalSection() {
    return &hkReferencedObjectLock::s_instance->m_criticalSection;
}

// The SDK's operator delete: the block goes back to the heap allocator with the size the object carries.
void hkReferencedObject::deleteObject(void* p) {
    hkReferencedObject* pObject = static_cast<hkReferencedObject*>(p);
    hkMemoryRouter::getInstance().m_heap->blockFree(p, pObject->m_memSizeAndFlags);
}

// The SDK's operator new: a heap block whose size is stored in the object's m_memSizeAndFlags.
void* hkReferencedObject::newObject(uint32_t auiSize) {
    hkReferencedObject* pObject = static_cast<hkReferencedObject*>(
        hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(auiSize))
    );
    pObject->m_memSizeAndFlags = static_cast<uint16_t>(auiSize);
    return pObject;
}

// Drops one reference from each object of the run, without the lock when locking is off or the thread already holds
// it, otherwise inside the lock with the thread's nesting slot set.
void hkReferencedObject::removeReferences(
    const hkReferencedObject* const* objects,
    int32_t numObjects,
    int32_t pointerStriding
) {
    hkReferencedObjectLock* pLock = hkReferencedObjectLock::s_instance;
    // The run is untyped memory walked at a byte stride, reading a pointer at each step.
    if (pLock->m_lockMode == LOCK_MODE_NONE || TlsGetValue(s_intLockCount.m_slotID) == (void*)0x23DF4554) {
        const char* pCursor = reinterpret_cast<const char*>(objects);
        for (int32_t i = numObjects; i > 0; --i) {
            const hkReferencedObject* pObject = *reinterpret_cast<const hkReferencedObject* const*>(pCursor);
            pObject->removeReferenceLockUnchecked();
            pCursor += pointerStriding;
        }
        return;
    }
    // EXE-INLINED 0xC90490
    if (TlsGetValue(s_intLockCount.m_slotID) == (void*)0x23DF4554) {
        ++pLock->m_lockCount;
    }
    else {
        EnterCriticalSection(&pLock->m_criticalSection.m_section);
        pLock->m_lockCount = 1;
        TlsSetValue(s_intLockCount.m_slotID, (void*)0x23DF4554);
    }
    const char* pCursor = reinterpret_cast<const char*>(objects);
    for (int32_t i = numObjects; i > 0; --i) {
        const hkReferencedObject* pObject = *reinterpret_cast<const hkReferencedObject* const*>(pCursor);
        pObject->removeReferenceLockUnchecked();
        pCursor += pointerStriding;
    }
    // EXE-INLINED 0xC904E0
    if (--pLock->m_lockCount <= 0) {
        TlsSetValue(s_intLockCount.m_slotID, nullptr);
        LeaveCriticalSection(&pLock->m_criticalSection.m_section);
    }
}

void hkReferencedObject::lockInit(hkReferencedObject::LockMode lockMode) {
    if (hkReferencedObjectLock::s_instance) {
        hkReferencedObjectLock::s_instance->m_lockMode = lockMode;
    }
    else {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        hkReferencedObjectLock* v2 =
            static_cast<hkReferencedObjectLock*>(Instance->m_heap->blockAlloc(sizeof(hkReferencedObjectLock)));
        v2->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkReferencedObjectLock));
        hkReferencedObjectLock* v3 = ::new (v2) hkReferencedObjectLock;
        v3->m_lockMode = lockMode;
        if (hkReferencedObjectLock::s_instance) {
            hkReferencedObjectLock::s_instance->removeReferenceLockUnchecked();
        }
        hkReferencedObjectLock::s_instance = v3;
    }
}

const void* getVtablehkBaseObject() {
    return hkGetVtableOf<hkBaseObject>();
}

void finishLoadedObjecthkBaseObject([[maybe_unused]] void* p, [[maybe_unused]] int32_t finishing) {
    ;
}

void cleanupLoadedObjecthkBaseObject([[maybe_unused]] void* p) {
    ;
}

hkReferencedObject* hkReferencedObjectLockcreate() {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkReferencedObjectLock* v1 =
        static_cast<hkReferencedObjectLock*>(Instance->m_heap->blockAlloc(sizeof(hkReferencedObjectLock)));
    v1->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkReferencedObjectLock));
    return ::new (v1) hkReferencedObjectLock;
}

hkClass* getReferencedObjectClass() {
    return &hkReferencedObjectClass;
}

// hkMultiThreadCheck's own constructor is assumed to leave it unmarked with the checks off.
hkReferencedObjectLock::hkReferencedObjectLock()
  : m_lockMode(hkReferencedObject::LOCK_MODE_AUTO), m_lockCount(0), m_criticalSection(4000) {
    m_multiThreadCheck.enableChecks();
}
