#pragma once

#include "BSCore/BSCoreMath.h"
#include "BSCore/BSCoreUtils.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSString.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/MessageHandler.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cstdint>
#include <cstring>

// The root of the classes that hand out a per-thread interface.
class InterfacedClass {
public:
    struct BaseInterface;

    // The common part of the per-thread interfaces: the map or queue that handed it out (a LockFreeMap or
    // LockFreeQueue of some instantiation, which is a template here).
    struct BaseInterface {
        InterfacedClass* pOwner;
    };

    InterfacedClass();
    virtual ~InterfacedClass();

    // Builds the per-thread interface of thread aiThread.
    virtual BaseInterface* AllocateInterface(uint32_t aiThread) = 0;

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
};

// Hands each thread its own interface object (thread-local storage index, one entry per thread). Declared for a single
// interface class rather than as a template over the interface class.
class ThreadSpecificInterfaceManager {
public:
    // One thread's entry in the manager's table: the thread and the interface it was handed.
    struct ThreadSpecificInterface {
        uint32_t iThreadID;
        InterfacedClass::BaseInterface* pInterface;
    };

    ThreadSpecificInterfaceManager(uint32_t auiMaxThreads);
    ~ThreadSpecificInterfaceManager();

    // The calling thread's interface for apInterfacedClass, the lock-free map or queue it belongs to (the interface is
    // a LockFreeMapInterface or LockFreeQueueInterface).
    InterfacedClass::BaseInterface* GetInterface(InterfacedClass* apInterfacedClass);
    // The calling thread's interface for apInterfacedClass: the next table slot, the interface it is handed, its thread
    // id and the thread's TLS value.
    InterfacedClass::BaseInterface* AddInterface(InterfacedClass* apInterfacedClass);
    // Does the same as AddInterface; called by LockFreeMap::GetInterface.
    InterfacedClass::BaseInterface* AddInterface_0(InterfacedClass* apInterfacedClass);
    // The most threads the manager hands interfaces to.
    uint32_t GetMaxThreads() {
        return uiMaxThreads;
    }
    void Shutdown();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    uint32_t uiMaxThreads;
    uint32_t uiTLSIndex;
    ThreadSpecificInterface* pInterfaces;
    volatile int32_t uiThreadCount;
};

inline InterfacedClass::InterfacedClass() {}

inline InterfacedClass::~InterfacedClass() {}

// The per-thread table (an empty entry per thread) and the thread-local storage index.
inline ThreadSpecificInterfaceManager::ThreadSpecificInterfaceManager(uint32_t auiMaxThreads)
  : uiMaxThreads(auiMaxThreads), uiTLSIndex(0), pInterfaces(nullptr), uiThreadCount(0) {
    pInterfaces = static_cast<ThreadSpecificInterface*>(ourMalloc(sizeof(ThreadSpecificInterface) * uiMaxThreads));
    for (uint32_t i = 0; i < uiMaxThreads; ++i) {
        pInterfaces[i].iThreadID = 0;
        pInterfaces[i].pInterface = nullptr;
    }
    uiTLSIndex = BSTlsAlloc();
}

// Frees the table and the thread-local storage index. The threads' interfaces are not deleted here: their class depends
// on the map or queue that handed them out, which this class does not know.
inline ThreadSpecificInterfaceManager::~ThreadSpecificInterfaceManager() {
    ourFree(pInterfaces);
    BSTlsFree(uiTLSIndex);
}

inline InterfacedClass::BaseInterface* ThreadSpecificInterfaceManager::GetInterface(InterfacedClass* apInterfacedClass) {
    InterfacedClass::BaseInterface* pInterface = static_cast<InterfacedClass::BaseInterface*>(GetTlsValue(uiTLSIndex));
    if (pInterface) {
        return pInterface;
    }
    return AddInterface(apInterfacedClass);
}

// The calling thread's interface for apInterfacedClass: the next table slot, the interface it is handed, its thread id
// and the thread's TLS value.
inline InterfacedClass::BaseInterface* ThreadSpecificInterfaceManager::AddInterface(InterfacedClass* apInterfacedClass) {
    InterfacedClass::BaseInterface* pInterface = nullptr;
    uint32_t uiThread = static_cast<uint32_t>(NiAtomicIncrement(const_cast<int32_t&>(uiThreadCount)) - 1);
    if (uiThread < uiMaxThreads) {
        pInterface = apInterfacedClass->AllocateInterface(uiThread);
        pInterfaces[uiThread].pInterface = pInterface;
        pInterfaces[uiThread].iThreadID = BSGetCurrentThreadID();
        SetTlsValue(uiTLSIndex, pInterface);
    }
    return pInterface;
}

// The pointer-width compare-and-swap the lock-free queues and maps link and unlink nodes with: stores apExchange at
// *apDestination when it holds apCompare; true when it did. A node link (a MarkedPointer, its mark in bit 0) is swapped
// as an untyped pointer word, so callers pass the link's address as one.
inline bool CompareAndSwapEx(void* volatile* apDestination, void* apExchange, void* apCompare) {
    bool bExchanged = BSAtomicCompareExchange(apDestination, apCompare, apExchange) == apCompare;
    InitializationFlow();
    return bExchanged;
}

// A lock-free hash map. The data is passed by address.
template <class Key, class Data>
class LockFreeMap : public InterfacedClass {
public:
    // A walk over the map's buckets: the bucket reached, the last key returned and the walk's state.
    class LockFreeMapIterator {
    public:
        // Bits of cFlags.
        enum ITERATOR_FLAG : uint8_t {
            FLAG_STARTED_BUCKET = 0x1,
            FLAG_FINISHED = 0x2,
        };

        LockFreeMapIterator() : iBucket(0), LastKey(), cFlags(0) {}
        virtual ~LockFreeMapIterator();

        // Forgets the last key returned. Defined per instantiation (see IOManager.cpp).
        virtual void ClearKey();

        // Whether the walk is over.
        bool GetFinished() const {
            return (cFlags & FLAG_FINISHED) != 0;
        }
        // Whether the walk has started its bucket: bit 0 of cFlags.
        bool GetStartedBucket() {
            return (cFlags & FLAG_STARTED_BUCKET) != 0;
        }

        // Sets or clears the started-bucket flag, bit 0 of cFlags.
        void SetStartedBucket(bool abValue) {
            if (abValue) {
                cFlags |= FLAG_STARTED_BUCKET;
            }
            else {
                cFlags &= ~FLAG_STARTED_BUCKET;
            }
        }

        // Sets or clears the finished flag, bit 1 of cFlags.
        void SetFinished(bool abValue) {
            if (abValue) {
                cFlags |= FLAG_FINISHED;
            }
            else {
                cFlags &= ~FLAG_FINISHED;
            }
        }

        // Forgets the last key, sets the walk's bucket to aiBucket and clears the walk's started and finished flags.
        void SetBucket(uint32_t aiBucket);

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }

        uint32_t iBucket;
        Key LastKey;
        uint8_t cFlags;
    };

    class HashListNode;

    // A node pointer whose low bit carries the deletion mark.
    template <class T>
    class MarkedPointer {
    public:
        MarkedPointer() : pPointer(nullptr) {}
        // The node pointer, then its deletion mark.
        MarkedPointer(bool abMark, T* apNode) : pPointer(apNode) {
            SetMark(abMark);
        }

        // Whether the node is marked for deletion: the pointer's low bit.
        bool GetMark() {
            return (reinterpret_cast<uintptr_t>(pPointer) & 1) != 0;
        }

        // Sets or clears the deletion mark (the pointer's low bit).
        void SetMark(bool abVal) {
            if (abVal) {
                pPointer = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(pPointer) | 1);
            }
            else {
                // Bit 0 only, of the raw address: a 32-bit mask would also clear the upper half of a 64-bit pointer.
                pPointer = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(pPointer) & ~static_cast<uintptr_t>(1));
            }
        }

        // The node pointer, through GetPointer.
        T* operator->();
        // The node pointer without the deletion mark. Defined per instantiation (see the specialization below).
        T* GetPointer();

        T* pPointer;
    };

    // A bucket entry: the key, its data and the next node, marked for deletion through pNext.
    class HashListNode {
    public:
        // kData shares its word with pNextDelete: the constructor starts kData's lifetime, the destructor ends it.
        HashListNode(Key aKey, const Data& arData) : kKey(aKey), kData(arData), pNext() {}
        ~HashListNode() {
            kData.~Data();
        }

        // Whether the node is marked for deletion (through pNext).
        bool GetMark() {
            return pNext.GetMark();
        }
        // Links the node into a deletion list through kData's word, which the deleted node no longer needs.
        void SetNextDelete(HashListNode* apNode) {
            pNextDelete = apNode;
        }

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }

        // The fields are not named Key and Data because the template parameters of the same name forbid it.
        Key kKey;

        // The deletion list's link reuses kData's word once the node is unlinked and its data released.
        union {
            Data kData;
            HashListNode* pNextDelete;
        };

        MarkedPointer<HashListNode> pNext;
    };

    // The per-thread view of the map (what GetInterface hands each thread): the nodes it holds referenced while it
    // walks a bucket, the deletion list and its count. It has no vtable.
    class LockFreeMapInterface : public InterfacedClass::BaseInterface {
    public:
        // The owner and the thread's three node references, the rest cleared.
        LockFreeMapInterface(
            LockFreeMap* apOwner,
            HashListNode** apReferencedNodes1,
            HashListNode** apReferencedNodes2,
            HashListNode** apReferencedNodes3
        )
          : pReferencedNodes{apReferencedNodes1, apReferencedNodes2, apReferencedNodes3}, pPrev(nullptr), Current(),
            Next(), pDeleteHead(nullptr), iDeleteCount(0) {
            pOwner = apOwner;
        }

        // Scans the deletion list until it is empty.
        ~LockFreeMapInterface() {
            while (iDeleteCount) {
                ScanForDeletedNodes();
            }
        }

        // Clears the three node references the thread holds.
        void ClearReferencedNodes() {
            *pReferencedNodes[0] = nullptr;
            *pReferencedNodes[1] = nullptr;
            *pReferencedNodes[2] = nullptr;
        }

        // Copies the data stored for aKey to arData; false when there is none.
        bool GetAt(Key aKey, Data& arData);
        // GetAt in bucket aiBucket.
        bool GetAt(uint32_t aiBucket, Key aKey, Data& arData);
        // Clears the node's data and puts it on the deletion list; a full batch is scanned for nodes no thread
        // references any more. Defined per instantiation (see ModelLoader.cpp).
        void DeleteNode(HashListNode* apNode);
        // Frees the nodes of the deletion list that no thread references. Defined per instantiation.
        void ScanForDeletedNodes();
        // Positions pPrev, Current and Next at aKey in bucket aiBucket, unlinking marked nodes on the way; true when
        // the key is there. Defined per instantiation (see BSTaskManager.cpp).
        bool Find(uint32_t aiBucket, Key aKey);
        // Stores arData for aKey in bucket aiBucket with a new node, or, when the key is there and
        // abReplaceExisting, into its node; true unless the key was there and is kept. Defined per instantiation
        // (see BSTaskManager.cpp).
        bool SetAt(uint32_t aiBucket, Key aKey, const Data& arData, bool abReplaceExisting);
        // SetAt into the bucket the owner's GetBucketForKey gives aKey. Defined per instantiation (see
        // ModelLoader.cpp).
        bool SetAt(Key aKey, const Data& arData, bool abReplaceExisting);
        // Removes the entry of aKey from the bucket GetBucketForKey gives it; true when there was one.
        bool RemoveAt(Key aKey);
        // Removes the entry of aKey from bucket aiBucket; true when there was one.
        bool RemoveAt(uint32_t aiBucket, Key aKey);
        // Removes the entry of bucket aiBucket whose data equals arData; true when there was one.
        bool Remove(uint32_t aiBucket, Data& arData);
        // Searches bucket aiBucket for an entry whose data equals arData, unlinking marked nodes on the way; true
        // when there is one.
        bool FindValue(uint32_t aiBucket, Data& arData);
        // Copies the first entry of bucket aiBucket to arKey and arData; false when the bucket is empty.
        bool GetFirst(uint32_t aiBucket, Key& arKey, Data& arData);
        // Copies the entry after arKey in bucket aiBucket to arKey and arData; false when there is none.
        bool GetNext(uint32_t aiBucket, Key& arKey, Data& arData);

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }

        HashListNode** pReferencedNodes[3];
        MarkedPointer<HashListNode>* pPrev;
        MarkedPointer<HashListNode> Current;
        MarkedPointer<HashListNode> Next;
        HashListNode* pDeleteHead;
        uint32_t iDeleteCount;
    };

    LockFreeMap(uint32_t auiMaxThreads, uint32_t auiNumBuckets, uint32_t auiDeleteBatchSize);
    ~LockFreeMap() override;

    BaseInterface* AllocateInterface(uint32_t aiNum) override;
    // Copies the data stored for aKey to arData; false when there is none.
    virtual bool GetAt(Key aKey, Data& arData);
    // Stores aData for aKey (replacing an existing entry only when abReplaceExisting); true when it was stored.
    virtual bool SetAt(Key aKey, const Data& aData, bool abReplaceExisting);
    virtual bool SetAt(uint32_t auiBucket, Key aKey, const Data& aData, bool abReplaceExisting);
    // Removes the entry of aKey; true when there was one.
    virtual bool RemoveAt(Key aKey);
    // Removes the entry holding aData from every bucket.
    virtual bool Remove(Data aData);
    // Removes the entry holding aData from bucket auiBucket.
    virtual bool Remove(uint32_t auiBucket, Data aData);
    // GetAt without the lock.
    virtual bool Func08(Key aKey, Data& arData);
    virtual uint32_t GetBucketForKey(Key aKey);
    virtual void DeleteKey(Key aKey);
    virtual Key CopyKey(Key aKey);
    virtual void CopyKeyDirect(Key aKey, Key& arCopiedKey);
    virtual bool IsKeyGreaterOrEqual(Key aKey1, Key aKey2);
    virtual bool IsKeyEqual(Key aKey1, Key aKey2);
    virtual void IncrementCount();
    virtual void DecrementCount();
    // The number of entries.
    virtual uint32_t GetCount();

    // The bucket at aiIndex. Defined per instantiation (see the specialization in ModelLoader.cpp).
    MarkedPointer<HashListNode>* GetBucket(int32_t aiIndex);
    // Frees a node unlinked for good: DeleteKey on its key, then the node with its data.
    void DeleteNode(HashListNode* apNode);
    // Copies the next entry of the walk to arKey and arData; returns false when there is none.
    bool GetNext(LockFreeMapIterator& arIterator, Key& arKey, Data& arData, bool abSwitchBuckets);
    // GetNext that also removes the entry it returns. The body is in the specializations below.
    bool GetNextAndRemove(LockFreeMapIterator& arIterator, Key& arKey, Data& arData, bool abSwitchBuckets);
    // Take, release and try the map's spin lock. The bodies are the specializations below. PC-only.
    void Lock();
    void Unlock();
    bool TryLock();
    // The calling thread's interface of the map: the manager's interface for this map.
    LockFreeMapInterface* GetInterface() {
        return static_cast<LockFreeMapInterface*>(pManager->GetInterface(this));
    }
    // How many deleted nodes a thread collects before it scans them.
    uint32_t GetDeleteBatchSize() {
        return uiDeleteBatchSize;
    }
    // The most threads the map hands interfaces to.
    uint32_t GetMaxThreads() {
        return pManager->GetMaxThreads();
    }
    // Empties every bucket under the lock and drops the interface manager; unless abCalledByDestructor, gives the map
    // a new one. Defined per instantiation (see the specializations in ModelLoader.h).
    void RemoveAll(bool abCalledByDestructor);

    HashListNode** pReferencedNodes;
    uint32_t uiNumBuckets;
    MarkedPointer<HashListNode>* pBuckets;
    uint32_t uiDeleteBatchSize;
    ThreadSpecificInterfaceManager* pManager;
    uint32_t uiCount;
    // The four bytes between the count and the lock are the lock's alignment padding, since kLock sits on the next
    // 16-byte boundary after uiCount. No LockFreeMap body reads or writes them.
    uint32_t padding1C;
    BSSpinLock kLock;
};

// The lock, then, in the task memory context, the bucket table (null, unmarked pointers), the per-thread node table
// (three per thread) and the interface manager.
template <class Key, class Data>
LockFreeMap<Key, Data>::LockFreeMap(uint32_t auiMaxThreads, uint32_t auiNumBuckets, uint32_t auiDeleteBatchSize)
  : InterfacedClass(), kLock() {
    AutoMemContext kMemContext(MEM_CONTEXT_TASK_OVERHEAD);
    uiCount = 0;
    uiNumBuckets = auiNumBuckets;
    pBuckets = new MarkedPointer<HashListNode>[uiNumBuckets];
    pReferencedNodes = new HashListNode*[3 * auiMaxThreads];
    uiDeleteBatchSize = auiDeleteBatchSize;
    pManager = new ThreadSpecificInterfaceManager(auiMaxThreads);
}

// Empties the map for good (RemoveAll drops the interface manager), then frees the bucket and node tables.
template <class Key, class Data>
LockFreeMap<Key, Data>::~LockFreeMap() {
    RemoveAll(true);
    delete[] pBuckets;
    delete[] pReferencedNodes;
}

template <class Key, class Data>
LockFreeMap<Key, Data>::LockFreeMapIterator::~LockFreeMapIterator() {}

// The calling thread's interface: the thread's three node references, allocated in the task memory context.
template <class Key, class Data>
InterfacedClass::BaseInterface* LockFreeMap<Key, Data>::AllocateInterface(uint32_t aiNum) {
    AutoMemContext kMemContext(MEM_CONTEXT_TASK_OVERHEAD);
    return new LockFreeMapInterface(
        this,
        &pReferencedNodes[3 * aiNum],
        &pReferencedNodes[3 * aiNum + 1],
        &pReferencedNodes[3 * aiNum + 2]
    );
}

template <class Key, class Data>
bool LockFreeMap<Key, Data>::GetAt(Key aKey, Data& arData) {
    kLock.Lock(nullptr);
    bool bResult = GetInterface()->GetAt(aKey, arData);
    kLock.Unlock();
    return bResult;
}

template <class Key, class Data>
bool LockFreeMap<Key, Data>::SetAt(Key aKey, const Data& aData, bool abReplaceExisting) {
    kLock.Lock(nullptr);
    bool bResult = GetInterface()->SetAt(aKey, aData, abReplaceExisting);
    kLock.Unlock();
    return bResult;
}

template <class Key, class Data>
bool LockFreeMap<Key, Data>::SetAt(uint32_t auiBucket, Key aKey, const Data& aData, bool abReplaceExisting) {
    kLock.Lock(nullptr);
    bool bResult = GetInterface()->SetAt(auiBucket, aKey, aData, abReplaceExisting);
    kLock.Unlock();
    return bResult;
}

template <class Key, class Data>
bool LockFreeMap<Key, Data>::RemoveAt(Key aKey) {
    kLock.Lock(nullptr);
    bool bResult = GetInterface()->RemoveAt(aKey);
    kLock.Unlock();
    return bResult;
}

// Tries each bucket's Remove under the lock until one removes the data.
template <class Key, class Data>
bool LockFreeMap<Key, Data>::Remove(Data aData) {
    bool bRemoved = false;
    kLock.Lock(nullptr);
    for (uint32_t i = 0; i < uiNumBuckets && !bRemoved; ++i) {
        bRemoved = Remove(i, aData);
    }
    kLock.Unlock();
    return bRemoved;
}

template <class Key, class Data>
bool LockFreeMap<Key, Data>::Remove(uint32_t auiBucket, Data aData) {
    kLock.Lock(nullptr);
    bool bResult = GetInterface()->Remove(auiBucket, aData);
    kLock.Unlock();
    return bResult;
}

template <class Key, class Data>
bool LockFreeMap<Key, Data>::Func08(Key aKey, Data& arData) {
    return GetInterface()->GetAt(aKey, arData);
}

// The key taken as a number (a pointer key's address, an integer key's low 32 bits) modulo the bucket count; a pointer
// key's raw address is divided.
template <class Key, class Data>
uint32_t LockFreeMap<Key, Data>::GetBucketForKey(Key aKey) {
    if constexpr (std::is_pointer_v<Key>) {
        return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(aKey) % uiNumBuckets);
    }
    else {
        return static_cast<uint32_t>(aKey) % uiNumBuckets;
    }
}

// The map keeps the caller's key itself, so there is nothing to free.
template <class Key, class Data>
void LockFreeMap<Key, Data>::DeleteKey([[maybe_unused]] Key aKey) {}

template <class Key, class Data>
Key LockFreeMap<Key, Data>::CopyKey(Key aKey) {
    return aKey;
}

template <class Key, class Data>
void LockFreeMap<Key, Data>::CopyKeyDirect(Key aKey, Key& arCopiedKey) {
    arCopiedKey = aKey;
}

template <class Key, class Data>
bool LockFreeMap<Key, Data>::IsKeyGreaterOrEqual(Key aKey1, Key aKey2) {
    return aKey1 >= aKey2;
}

template <class Key, class Data>
bool LockFreeMap<Key, Data>::IsKeyEqual(Key aKey1, Key aKey2) {
    return aKey1 == aKey2;
}

template <class Key, class Data>
void LockFreeMap<Key, Data>::IncrementCount() {
    NiAtomicIncrement(uiCount);
}

template <class Key, class Data>
void LockFreeMap<Key, Data>::DecrementCount() {
    NiAtomicDecrement(uiCount);
}

template <class Key, class Data>
uint32_t LockFreeMap<Key, Data>::GetCount() {
    return uiCount;
}

template <class Key, class Data>
void LockFreeMap<Key, Data>::LockFreeMapIterator::ClearKey() {
    LastKey = Key();
}

template <class Key, class Data>
void LockFreeMap<Key, Data>::LockFreeMapIterator::SetBucket(uint32_t aiBucket) {
    ClearKey();
    iBucket = aiBucket;
    cFlags &= ~FLAG_STARTED_BUCKET;
    cFlags &= ~FLAG_FINISHED;
}

template <class Key, class Data>
template <class T>
T* LockFreeMap<Key, Data>::MarkedPointer<T>::operator->() {
    return GetPointer();
}

// The mark lives in the pointer's bit 0, so the pointer is read as a raw address and the bit cleared.
template <class Key, class Data>
template <class T>
T* LockFreeMap<Key, Data>::MarkedPointer<T>::GetPointer() {
    return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(pPointer) & ~static_cast<uintptr_t>(1));
}

template <class Key, class Data>
auto LockFreeMap<Key, Data>::GetBucket(int32_t aiIndex) -> MarkedPointer<HashListNode>* {
    return &pBuckets[aiIndex];
}

template <class Key, class Data>
void LockFreeMap<Key, Data>::DeleteNode(HashListNode* apNode) {
    DeleteKey(apNode->kKey);
    delete apNode;
}

template <class Key, class Data>
bool LockFreeMap<Key, Data>::LockFreeMapInterface::GetAt(Key aKey, Data& arData) {
    uint32_t uiBucket = static_cast<LockFreeMap*>(pOwner)->GetBucketForKey(aKey);
    return GetAt(uiBucket, aKey, arData);
}

// The data of the found node, unless the node was marked for deletion after the copy.
template <class Key, class Data>
bool LockFreeMap<Key, Data>::LockFreeMapInterface::GetAt(uint32_t aiBucket, Key aKey, Data& arData) {
    bool bFound = Find(aiBucket, aKey);
    if (bFound) {
        arData = Current->kData;
        InitializationFlow();
        if (Current->GetMark()) {
            bFound = false;
        }
    }
    ClearReferencedNodes();
    return bFound;
}

// Releases the node's data and links the node into the deletion list through the same word; a full batch is
// scanned.
template <class Key, class Data>
void LockFreeMap<Key, Data>::LockFreeMapInterface::DeleteNode(HashListNode* apNode) {
    apNode->kData = Data();
    apNode->SetNextDelete(pDeleteHead);
    pDeleteHead = apNode;
    ++iDeleteCount;
    if (iDeleteCount == static_cast<LockFreeMap*>(pOwner)->GetDeleteBatchSize()) {
        ScanForDeletedNodes();
    }
}

// Frees the listed nodes no thread references (through the owner's DeleteNode) and keeps the others listed. The
// references are copied first, in the task memory context.
template <class Key, class Data>
void LockFreeMap<Key, Data>::LockFreeMapInterface::ScanForDeletedNodes() {
    AutoMemContext kMemContext(MEM_CONTEXT_TASK_OVERHEAD);
    LockFreeMap* pMap = static_cast<LockFreeMap*>(pOwner);
    uint32_t uiReferencedNodeCount = 3 * pMap->GetMaxThreads();
    HashListNode** ppReferencedNodes = new HashListNode*[uiReferencedNodeCount];
    std::memcpy(ppReferencedNodes, pMap->pReferencedNodes, sizeof(HashListNode*) * uiReferencedNodeCount);
    HashListNode* pNewDeleteHead = nullptr;
    uint32_t uiNewDeleteCount = 0;
    while (pDeleteHead) {
        HashListNode* pNode = pDeleteHead;
        pDeleteHead = pNode->pNextDelete;
        bool bReferenced = false;
        for (uint32_t i = 0; i < uiReferencedNodeCount; ++i) {
            if (pNode == ppReferencedNodes[i]) {
                bReferenced = true;
                break;
            }
        }
        if (bReferenced) {
            pNode->SetNextDelete(pNewDeleteHead);
            pNewDeleteHead = pNode;
            ++uiNewDeleteCount;
        }
        else {
            pNode->SetNextDelete(nullptr);
            pMap->DeleteNode(pNode);
        }
    }
    delete[] ppReferencedNodes;
    pDeleteHead = pNewDeleteHead;
    iDeleteCount = uiNewDeleteCount;
}

// Walks bucket aiBucket from its head, unlinking the marked nodes on the way, to the first key not below aKey; true
// when it equals aKey. The walk starts again from the head whenever a link changed under it.
template <class Key, class Data>
bool LockFreeMap<Key, Data>::LockFreeMapInterface::Find(uint32_t aiBucket, Key aKey) {
    LockFreeMap* pMap = static_cast<LockFreeMap*>(pOwner);
    MarkedPointer<HashListNode>* pBucket = pMap->GetBucket(static_cast<int32_t>(aiBucket));
    while (true) {
        pPrev = pBucket;
        Current.pPointer = pPrev->pPointer;
        *pReferencedNodes[1] = Current.GetPointer();
        InitializationFlow();
        MarkedPointer<HashListNode> kUnmarkedHead(false, Current.GetPointer());
        if (pPrev->pPointer != kUnmarkedHead.pPointer) {
            continue;
        }
        while (true) {
            if (!Current.GetPointer()) {
                return false;
            }
            Next.pPointer = Current->pNext.pPointer;
            *pReferencedNodes[0] = Next.GetPointer();
            InitializationFlow();
            if (Next.pPointer != Current->pNext.pPointer) {
                break;
            }
            Key kCurrentKey = Current->kKey;
            MarkedPointer<HashListNode> kUnmarkedCurrent(false, Current.GetPointer());
            if (pPrev->pPointer != kUnmarkedCurrent.pPointer) {
                break;
            }
            if (!Next.GetMark()) {
                if (pMap->IsKeyGreaterOrEqual(kCurrentKey, aKey)) {
                    return pMap->IsKeyEqual(kCurrentKey, aKey);
                }
                pPrev = &Current->pNext;
                *pReferencedNodes[2] = Current.GetPointer();
                InitializationFlow();
            }
            else {
                MarkedPointer<HashListNode> kUnmarkedNext(false, Next.GetPointer());
                if (!CompareAndSwapEx(
                        reinterpret_cast<void* volatile*>(&pPrev->pPointer),
                        kUnmarkedNext.pPointer,
                        kUnmarkedCurrent.pPointer
                    )) {
                    break;
                }
                DeleteNode(Current.GetPointer());
            }
            Current.pPointer = Next.pPointer;
            *pReferencedNodes[1] = Next.GetPointer();
            InitializationFlow();
        }
    }
}

// Stores arData in the node of aKey when abReplaceExisting, or links a new node with a copy of the key (the owner's
// CopyKey) in front of the node Find stopped at; false when the key is there and kept. A new node left unused goes
// without its key copy being freed, deliberately.
template <class Key, class Data>
bool LockFreeMap<Key, Data>::LockFreeMapInterface::SetAt(
    uint32_t aiBucket,
    Key aKey,
    const Data& arData,
    bool abReplaceExisting
) {
    AutoMemContext kMemContext(MEM_CONTEXT_TASK_OVERHEAD);
    LockFreeMap* pMap = static_cast<LockFreeMap*>(pOwner);
    bool bResult = true;
    HashListNode* pNewNode = nullptr;
    while (true) {
        if (Find(aiBucket, aKey)) {
            delete pNewNode;
            if (abReplaceExisting) {
                Current->kData = arData;
            }
            else {
                bResult = false;
            }
            break;
        }
        if (!pNewNode) {
            pNewNode = new HashListNode(pMap->CopyKey(aKey), arData);
        }
        MarkedPointer<HashListNode> kExpected(false, Current.GetPointer());
        MarkedPointer<HashListNode> kNew(false, pNewNode);
        pNewNode->pNext.pPointer = kExpected.pPointer;
        if (CompareAndSwapEx(reinterpret_cast<void* volatile*>(&pPrev->pPointer), kNew.pPointer, kExpected.pPointer)) {
            pMap->IncrementCount();
            break;
        }
    }
    ClearReferencedNodes();
    return bResult;
}

template <class Key, class Data>
bool LockFreeMap<Key, Data>::LockFreeMapInterface::SetAt(Key aKey, const Data& arData, bool abReplaceExisting) {
    uint32_t uiBucket = static_cast<LockFreeMap*>(pOwner)->GetBucketForKey(aKey);
    return SetAt(uiBucket, aKey, arData, abReplaceExisting);
}

template <class Key, class Data>
bool LockFreeMap<Key, Data>::LockFreeMapInterface::RemoveAt(Key aKey) {
    uint32_t uiBucket = static_cast<LockFreeMap*>(pOwner)->GetBucketForKey(aKey);
    return RemoveAt(uiBucket, aKey);
}

// Marks the found node's link, then unlinks the node from pPrev; when pPrev changed meanwhile, Find runs again to
// unlink it on the way. The owner's count goes down.
template <class Key, class Data>
bool LockFreeMap<Key, Data>::LockFreeMapInterface::RemoveAt(uint32_t aiBucket, Key aKey) {
    bool bResult = false;
    while (Find(aiBucket, aKey)) {
        MarkedPointer<HashListNode> kUnmarkedNext(false, Next.GetPointer());
        MarkedPointer<HashListNode> kMarkedNext(true, Next.GetPointer());
        if (!CompareAndSwapEx(
                reinterpret_cast<void* volatile*>(&Current->pNext.pPointer),
                kMarkedNext.pPointer,
                kUnmarkedNext.pPointer
            )) {
            continue;
        }
        MarkedPointer<HashListNode> kUnmarkedCurrent(false, Current.GetPointer());
        if (CompareAndSwapEx(
                reinterpret_cast<void* volatile*>(&pPrev->pPointer),
                kUnmarkedNext.pPointer,
                kUnmarkedCurrent.pPointer
            )) {
            DeleteNode(Current.GetPointer());
        }
        else {
            Find(aiBucket, aKey);
        }
        static_cast<LockFreeMap*>(pOwner)->DecrementCount();
        bResult = true;
        break;
    }
    ClearReferencedNodes();
    return bResult;
}

// RemoveAt's unlinking for the node FindValue finds. Leaves the owner's count as it is.
template <class Key, class Data>
bool LockFreeMap<Key, Data>::LockFreeMapInterface::Remove(uint32_t aiBucket, Data& arData) {
    bool bResult = false;
    while (FindValue(aiBucket, arData)) {
        MarkedPointer<HashListNode> kUnmarkedNext(false, Next.GetPointer());
        MarkedPointer<HashListNode> kMarkedNext(true, Next.GetPointer());
        if (!CompareAndSwapEx(
                reinterpret_cast<void* volatile*>(&Current->pNext.pPointer),
                kMarkedNext.pPointer,
                kUnmarkedNext.pPointer
            )) {
            continue;
        }
        MarkedPointer<HashListNode> kUnmarkedCurrent(false, Current.GetPointer());
        if (CompareAndSwapEx(
                reinterpret_cast<void* volatile*>(&pPrev->pPointer),
                kUnmarkedNext.pPointer,
                kUnmarkedCurrent.pPointer
            )) {
            DeleteNode(Current.GetPointer());
        }
        else {
            FindValue(aiBucket, arData);
        }
        bResult = true;
        break;
    }
    ClearReferencedNodes();
    return bResult;
}

// Find's walk, to the first unmarked node whose data equals arData. Each node's data is copied before the walk
// checks that pPrev still links the node, then compared.
template <class Key, class Data>
bool LockFreeMap<Key, Data>::LockFreeMapInterface::FindValue(uint32_t aiBucket, Data& arData) {
    MarkedPointer<HashListNode>* pBucket = static_cast<LockFreeMap*>(pOwner)->GetBucket(static_cast<int32_t>(aiBucket));
    while (true) {
        pPrev = pBucket;
        Current.pPointer = pPrev->pPointer;
        *pReferencedNodes[1] = Current.GetPointer();
        InitializationFlow();
        MarkedPointer<HashListNode> kUnmarkedHead(false, Current.GetPointer());
        if (pPrev->pPointer != kUnmarkedHead.pPointer) {
            continue;
        }
        while (true) {
            if (!Current.GetPointer()) {
                return false;
            }
            Next.pPointer = Current->pNext.pPointer;
            *pReferencedNodes[0] = Next.GetPointer();
            InitializationFlow();
            if (Next.pPointer != Current->pNext.pPointer) {
                break;
            }
            Data kCurrentData = Current->kData;
            MarkedPointer<HashListNode> kUnmarkedCurrent(false, Current.GetPointer());
            if (pPrev->pPointer != kUnmarkedCurrent.pPointer) {
                break;
            }
            if (!Next.GetMark()) {
                if (kCurrentData == arData) {
                    return true;
                }
                pPrev = &Current->pNext;
                *pReferencedNodes[2] = Current.GetPointer();
                InitializationFlow();
            }
            else {
                MarkedPointer<HashListNode> kUnmarkedNext(false, Next.GetPointer());
                if (!CompareAndSwapEx(
                        reinterpret_cast<void* volatile*>(&pPrev->pPointer),
                        kUnmarkedNext.pPointer,
                        kUnmarkedCurrent.pPointer
                    )) {
                    break;
                }
                DeleteNode(Current.GetPointer());
            }
            Current.pPointer = Next.pPointer;
            *pReferencedNodes[1] = Next.GetPointer();
            InitializationFlow();
        }
    }
}

// The bucket's head entry, the key through the owner's CopyKeyDirect; the walk starts again when the head changed
// under it or was marked after the copy.
template <class Key, class Data>
bool LockFreeMap<Key, Data>::LockFreeMapInterface::GetFirst(uint32_t aiBucket, Key& arKey, Data& arData) {
    LockFreeMap* pMap = static_cast<LockFreeMap*>(pOwner);
    bool bResult = false;
    while (true) {
        pPrev = pMap->GetBucket(static_cast<int32_t>(aiBucket));
        Current.pPointer = pPrev->pPointer;
        *pReferencedNodes[1] = Current.GetPointer();
        InitializationFlow();
        MarkedPointer<HashListNode> kUnmarkedHead(false, Current.GetPointer());
        if (pPrev->pPointer != kUnmarkedHead.pPointer) {
            continue;
        }
        if (!Current.GetPointer()) {
            bResult = false;
            break;
        }
        pMap->CopyKeyDirect(Current->kKey, arKey);
        arData = Current->kData;
        bResult = true;
        InitializationFlow();
        if (!Current->GetMark()) {
            break;
        }
    }
    ClearReferencedNodes();
    return bResult;
}

// The entry after arKey: Find leaves it in Next when arKey is there and in Current when not. A marked entry sends
// the search back to Find; one marked after its copy clears arData again.
template <class Key, class Data>
bool LockFreeMap<Key, Data>::LockFreeMapInterface::GetNext(uint32_t aiBucket, Key& arKey, Data& arData) {
    LockFreeMap* pMap = static_cast<LockFreeMap*>(pOwner);
    bool bResult = false;
    while (true) {
        MarkedPointer<HashListNode>* pEntry = Find(aiBucket, arKey) ? &Next : &Current;
        HashListNode* pNode = pEntry->GetPointer();
        if (!pNode) {
            bResult = false;
            break;
        }
        if (pNode->GetMark()) {
            continue;
        }
        pMap->CopyKeyDirect(pNode->kKey, arKey);
        arData = pNode->kData;
        bResult = true;
        InitializationFlow();
        if (!pNode->GetMark()) {
            break;
        }
        arData = Data();
    }
    ClearReferencedNodes();
    return bResult;
}

// The walk's next entry under the lock: the rest of the walk's bucket, then (when abSwitchBuckets) the next buckets;
// the walk is finished past the last bucket.
template <class Key, class Data>
bool LockFreeMap<Key, Data>::GetNext(LockFreeMapIterator& arIterator, Key& arKey, Data& arData, bool abSwitchBuckets) {
    bool bResult = false;
    kLock.Lock(nullptr);
    while (!arIterator.GetFinished()) {
        if (arIterator.GetStartedBucket()) {
            bResult = GetInterface()->GetNext(arIterator.iBucket, arIterator.LastKey, arData);
        }
        else {
            bResult = GetInterface()->GetFirst(arIterator.iBucket, arIterator.LastKey, arData);
            arIterator.SetStartedBucket(true);
        }
        if (bResult) {
            arKey = arIterator.LastKey;
            break;
        }
        if (++arIterator.iBucket >= uiNumBuckets) {
            arIterator.SetFinished(true);
            break;
        }
        if (!abSwitchBuckets) {
            break;
        }
        arIterator.SetStartedBucket(false);
    }
    kLock.Unlock();
    return bResult;
}

// GetNext, then the removal of the entry it returned, both under the (recursive) lock.
template <class Key, class Data>
bool LockFreeMap<Key, Data>::GetNextAndRemove(
    LockFreeMapIterator& arIterator,
    Key& arKey,
    Data& arData,
    bool abSwitchBuckets
) {
    kLock.Lock(nullptr);
    bool bResult = GetNext(arIterator, arKey, arData, abSwitchBuckets);
    if (bResult) {
        bResult = GetInterface()->RemoveAt(arKey);
    }
    kLock.Unlock();
    return bResult;
}

template <class Key, class Data>
void LockFreeMap<Key, Data>::Lock() {
    kLock.Lock(nullptr);
}

template <class Key, class Data>
void LockFreeMap<Key, Data>::Unlock() {
    kLock.Unlock();
}

template <class Key, class Data>
bool LockFreeMap<Key, Data>::TryLock() {
    return kLock.TryLock();
}

// Under the lock and in the task memory context: drops the interface manager with every thread's interface, frees every
// node (data released, key through DeleteKey) and empties the buckets; unless abCalledByDestructor, a new manager for
// as many threads. The standalone manager does not know the interfaces' class, so the map deletes them here.
template <class Key, class Data>
void LockFreeMap<Key, Data>::RemoveAll(bool abCalledByDestructor) {
    AutoMemContext kMemContext(MEM_CONTEXT_TASK_OVERHEAD);
    if (!pManager) {
        return;
    }
    kLock.Lock(nullptr);
    uint32_t uiMaxThreads = GetMaxThreads();
    for (uint32_t i = 0; i < pManager->GetMaxThreads(); ++i) {
        delete static_cast<LockFreeMapInterface*>(pManager->pInterfaces[i].pInterface);
    }
    delete pManager;
    pManager = nullptr;
    uiCount = 0;
    for (uint32_t i = 0; i < uiNumBuckets; ++i) {
        HashListNode* pNode = pBuckets[i].GetPointer();
        pBuckets[i].pPointer = nullptr;
        while (pNode) {
            HashListNode* pNext = pNode->pNext.GetPointer();
            pNode->pNext.pPointer = nullptr;
            pNode->kData = Data();
            DeleteNode(pNode);
            pNode = pNext;
        }
    }
    if (!abCalledByDestructor) {
        pManager = new ThreadSpecificInterfaceManager(uiMaxThreads);
    }
    kLock.Unlock();
}

// A lock-free map keyed by file name.
template <class Data>
class LockFreeStringMap : public LockFreeMap<const char*, Data> {
public:
    // A walk that keeps a copy of the last key in its own buffer (LastKey points at pBuffer).
    class LockFreeStringMapIterator : public LockFreeMap<const char*, Data>::LockFreeMapIterator {
    public:
        // The key buffer is empty and LastKey points at it.
        LockFreeStringMapIterator() {
            this->LastKey = pBuffer;
            pBuffer[0] = 0;
        }

        // Forgets the last key returned: empties the key buffer.
        void ClearKey() override {
            pBuffer[0] = 0;
        }

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }

        char pBuffer[0x104];
    };

    LockFreeStringMap(uint32_t aiMaxThreads, uint32_t aiNumBuckets, uint32_t aiDeleteBatchSize)
      : LockFreeMap<const char*, Data>(aiMaxThreads, aiNumBuckets, aiDeleteBatchSize) {}

    // Empties the map while this class's DeleteKey still frees the keys.
    ~LockFreeStringMap() override {
        this->RemoveAll(true);
    }

    // The key's bucket: the string's hash (h = h * 33 + c over its chars) modulo uiNumBuckets. The model, KF and file
    // maps share one body.
    uint32_t GetBucketForKey(const char* apKey) override;
    // Frees a key CopyKey made, with ourFree. One body for the string maps and the case-insensitive ones.
    void DeleteKey(const char* apKey) override;
    // A copy of the key the map keeps: BSstrlen + 1 bytes from ourMalloc under an AutoMemContext, filled with BSstrcpy.
    // One body for the three maps.
    const char* CopyKey(const char* apKey) override;
    // Copies the key into the buffer arpCopiedKey points at (an iterator's key buffer), at most 0x100 bytes, with
    // BSstrcpy. One body for the string maps and the case-insensitive ones.
    void CopyKeyDirect(const char* apKey, const char*& arpCopiedKey) override;
    // The keys are compared as strings with strcmp (>= 0).
    bool IsKeyGreaterOrEqual(const char* apKey1, const char* apKey2) override;
    // The keys are compared as strings with strcmp (equal when it returns 0).
    bool IsKeyEqual(const char* apKey1, const char* apKey2) override;

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
};

// The string's hash: h = h * 33 + c over its chars, each sign-extended.
template <class Data>
uint32_t LockFreeStringMap<Data>::GetBucketForKey(const char* apKey) {
    uint32_t uiHash = 0;
    while (*apKey) {
        uiHash = *apKey++ + 33 * uiHash;
    }
    return uiHash % this->uiNumBuckets;
}

// The key is the copy CopyKey allocated.
template <class Data>
void LockFreeStringMap<Data>::DeleteKey(const char* apKey) {
    ourFree(const_cast<char*>(apKey));
}

template <class Data>
const char* LockFreeStringMap<Data>::CopyKey(const char* apKey) {
    AutoMemContext kMemContext(MEM_CONTEXT_TASK_OVERHEAD);
    uint32_t uiSize = BSstrlen(apKey) + 1;
    char* pCopy = static_cast<char*>(ourMalloc(uiSize));
    BSstrcpy(pCopy, static_cast<int32_t>(uiSize), apKey);
    return pCopy;
}

// The destination key points at the iterator's buffer, which the copy writes through.
template <class Data>
void LockFreeStringMap<Data>::CopyKeyDirect(const char* apKey, const char*& arpCopiedKey) {
    BSstrcpy(const_cast<char*>(arpCopiedKey), 256, apKey);
}

template <class Data>
bool LockFreeStringMap<Data>::IsKeyGreaterOrEqual(const char* apKey1, const char* apKey2) {
    return std::strcmp(apKey1, apKey2) >= 0;
}

template <class Data>
bool LockFreeStringMap<Data>::IsKeyEqual(const char* apKey1, const char* apKey2) {
    return std::strcmp(apKey1, apKey2) == 0;
}

// A lock-free map keyed by file name, compared without case: each override lower-cases the key, character by character
// through the CRT's tolower, before LockFreeMap's own slot or the hash sees it.
template <class Data>
class LockFreeCaseInsensitiveStringMap : public LockFreeStringMap<Data> {
public:
    using LockFreeStringMap<Data>::SetAt;

    LockFreeCaseInsensitiveStringMap(uint32_t aiMaxThreads, uint32_t aiNumBuckets, uint32_t aiDeleteBatchSize)
      : LockFreeStringMap<Data>(aiMaxThreads, aiNumBuckets, aiDeleteBatchSize) {}

    ~LockFreeCaseInsensitiveStringMap() override;

    // GetAt with the key lower-cased into a 1000-byte buffer.
    bool GetAt(const char* apKey, Data& aData) override;
    // The key overload of SetAt with the key lower-cased into a 1000-byte buffer.
    bool SetAt(const char* apKey, const Data& aData, bool abReplaceExisting) override;
    // RemoveAt with the key lower-cased into a 1000-byte buffer.
    bool RemoveAt(const char* apKey) override;
    // The lower-cased string's hash (h = h * 33 + c) modulo uiNumBuckets.
    uint32_t GetBucketForKey(const char* apKey) override;
    // Returns a lower-cased copy of the key, BSstrlen + 1 bytes from ourMalloc under an AutoMemContext.
    const char* CopyKey(const char* apKey) override;
};
template <class Data>
LockFreeCaseInsensitiveStringMap<Data>::~LockFreeCaseInsensitiveStringMap() {}

// Each override lower-cases the key character by character through tolower (each char passed sign-extended) before
// LockFreeMap's own member or the hash sees it.
template <class Data>
bool LockFreeCaseInsensitiveStringMap<Data>::GetAt(const char* apKey, Data& aData) {
    char acLowerKey[1000];
    char* pDest = acLowerKey;
    while (*apKey) {
        *pDest++ = static_cast<char>(std::tolower(*apKey++));
    }
    *pDest = 0;
    return LockFreeMap<const char*, Data>::GetAt(acLowerKey, aData);
}

template <class Data>
bool LockFreeCaseInsensitiveStringMap<Data>::SetAt(const char* apKey, const Data& aData, bool abReplaceExisting) {
    char acLowerKey[1000];
    char* pDest = acLowerKey;
    while (*apKey) {
        *pDest++ = static_cast<char>(std::tolower(*apKey++));
    }
    *pDest = 0;
    return LockFreeMap<const char*, Data>::SetAt(acLowerKey, aData, abReplaceExisting);
}

template <class Data>
bool LockFreeCaseInsensitiveStringMap<Data>::RemoveAt(const char* apKey) {
    char acLowerKey[1000];
    char* pDest = acLowerKey;
    while (*apKey) {
        *pDest++ = static_cast<char>(std::tolower(*apKey++));
    }
    *pDest = 0;
    return LockFreeMap<const char*, Data>::RemoveAt(acLowerKey);
}

template <class Data>
uint32_t LockFreeCaseInsensitiveStringMap<Data>::GetBucketForKey(const char* apKey) {
    uint32_t uiHash = 0;
    while (*apKey) {
        uiHash = std::tolower(*apKey++) + 33 * uiHash;
    }
    return uiHash % this->uiNumBuckets;
}

template <class Data>
const char* LockFreeCaseInsensitiveStringMap<Data>::CopyKey(const char* apKey) {
    AutoMemContext kMemContext(MEM_CONTEXT_TASK_OVERHEAD);
    char* pCopy = static_cast<char*>(ourMalloc(BSstrlen(apKey) + 1));
    char* pDest = pCopy;
    while (*apKey) {
        *pDest++ = static_cast<char>(std::tolower(*apKey++));
    }
    *pDest = 0;
    return pCopy;
}

// A lock-free FIFO queue.
template <class T>
class LockFreeQueue : public InterfacedClass {
public:
    // A queued element: the next node and the queued item.
    class LockFreeQueueNode {
    public:
        LockFreeQueueNode() : pNext(nullptr), Data() {}

        // A deleted node is linked into its interface's delete list through Data's storage: the word
        // at Data is the next node. The items are pointers and NiPointers, released before the link is stored.
        LockFreeQueueNode* GetNextDelete() {
            static_assert(sizeof(T) >= sizeof(LockFreeQueueNode*), "the delete link lives in Data");
            LockFreeQueueNode* pNextDelete;
            std::memcpy(&pNextDelete, static_cast<const void*>(&Data), sizeof(pNextDelete));
            return pNextDelete;
        }

        void SetNextDelete(LockFreeQueueNode* apNextDelete) {
            static_assert(sizeof(T) >= sizeof(LockFreeQueueNode*), "the delete link lives in Data");
            std::memcpy(static_cast<void*>(&Data), &apNextDelete, sizeof(apNextDelete));
        }

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }

        LockFreeQueueNode* pNext;
        T Data;
    };

    // The per-thread interface of a queue: the nodes the thread has pinned and the nodes it has deleted.
    class LockFreeQueueInterface : public InterfacedClass::BaseInterface {
    public:
        LockFreeQueueInterface(LockFreeQueue* apOwner, LockFreeQueueNode** apNode, LockFreeQueueNode** apData)
          : InterfacedClass::BaseInterface{apOwner}, pReferencedNodes{apNode, apData}, iDeleteCount(0),
            pDeleteHead(nullptr) {}

        ~LockFreeQueueInterface() {
            while (iDeleteCount) {
                ScanForDeletedNodes();
            }
        }

        // Appends arData at the tail: a node from the task memory context, linked in by CompareAndSwapEx, then the
        // owner's count incremented. Defined per instantiation (see the specializations below).
        void Push(const T& arData);
        // Takes the item at the head into arData; false when the queue is empty. The
        // instantiation's body is the specialization below.
        bool Pop(T& arData);
        // Puts apNode on the delete list and scans it once the list reaches the owner's batch size.
        // Defined per instantiation (see the specialization below).
        void DeleteNode(LockFreeQueueNode* apNode);
        // Frees the nodes of the delete list no thread has pinned and keeps the pinned ones.
        // Defined per instantiation (see the specialization below).
        void ScanForDeletedNodes();

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }

        LockFreeQueueNode** pReferencedNodes[2];
        uint32_t iDeleteCount;
        LockFreeQueueNode* pDeleteHead;
    };

    LockFreeQueue(uint32_t aiMaxThreads, uint32_t aiDeleteBatchSize);
    ~LockFreeQueue() override;

    // Builds the per-thread interface of thread aiNum.
    BaseInterface* AllocateInterface(uint32_t aiNum) override;
    // Slots 2-4: the queued item count.
    virtual void IncrementCount();
    virtual void DecrementCount();
    virtual uint32_t GetCount();

    // Takes the item at the head into arData through the calling thread's interface, under the lock; false when the
    // queue is empty.
    bool Pop(T& arData);
    // Appends aData at the tail through the calling thread's interface, under the lock.
    void Push(const T& aData);
    // Frees every node and the interface manager under the lock; unless abCalledByDestructor, gives the queue a new
    // empty node and a new interface manager for as many threads.
    void RemoveAll(bool abCalledByDestructor);

    // The calling thread's interface of the queue.
    LockFreeQueueInterface* GetInterface() {
        return static_cast<LockFreeQueueInterface*>(pInterfaceManager->GetInterface(this));
    }

    // The most threads the queue hands interfaces to (the manager at +0x14, as in LockFreeMap).
    uint32_t GetMaxThreads() {
        return pInterfaceManager->GetMaxThreads();
    }

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    LockFreeQueueNode* pHead;
    LockFreeQueueNode* pTail;
    uint32_t iDeleteBatchSize;
    LockFreeQueueNode** pReferencedNodes;
    ThreadSpecificInterfaceManager* pInterfaceManager;
    uint32_t iCount;
    // The four bytes between the count and the lock are the lock's alignment padding, because BSSpinLock is 32-byte
    // aligned. No code reads or writes them.
    uint32_t padding1C;
    BSSpinLock kLock;
};

// The queue starts with one empty node, as head and tail, and a pinned-node slot pair per thread.
template <class T>
LockFreeQueue<T>::LockFreeQueue(uint32_t aiMaxThreads, uint32_t aiDeleteBatchSize)
  : pHead(new LockFreeQueueNode), pTail(pHead), iDeleteBatchSize(aiDeleteBatchSize),
    pReferencedNodes(static_cast<LockFreeQueueNode**>(ourMalloc(2 * aiMaxThreads * sizeof(LockFreeQueueNode*)))),
    pInterfaceManager(new ThreadSpecificInterfaceManager(aiMaxThreads)), iCount(0) {}

template <class T>
LockFreeQueue<T>::~LockFreeQueue() {
    RemoveAll(true);
    ourFree(pReferencedNodes);
}

// The interface of thread aiNum: its two pinned-node slots, allocated in the task memory context.
template <class T>
InterfacedClass::BaseInterface* LockFreeQueue<T>::AllocateInterface(uint32_t aiNum) {
    AutoMemContext kMemContext(MEM_CONTEXT_TASK_OVERHEAD);
    return new LockFreeQueueInterface(this, &pReferencedNodes[2 * aiNum], &pReferencedNodes[2 * aiNum + 1]);
}

template <class T>
void LockFreeQueue<T>::IncrementCount() {
    NiAtomicIncrement(iCount);
}

template <class T>
void LockFreeQueue<T>::DecrementCount() {
    NiAtomicDecrement(iCount);
}

template <class T>
uint32_t LockFreeQueue<T>::GetCount() {
    return iCount;
}

template <class T>
bool LockFreeQueue<T>::Pop(T& arData) {
    kLock.Lock(nullptr);
    bool bResult = GetInterface()->Pop(arData);
    kLock.Unlock();
    return bResult;
}

template <class T>
void LockFreeQueue<T>::Push(const T& aData) {
    kLock.Lock(nullptr);
    GetInterface()->Push(aData);
    kLock.Unlock();
}

// Under the lock and in the task memory context: drops the interface manager with every thread's interface (the
// standalone manager does not know their class, so the queue deletes them here), frees every node with its item
// released; unless abCalledByDestructor, a new empty node and a new manager for as many threads.
template <class T>
void LockFreeQueue<T>::RemoveAll(bool abCalledByDestructor) {
    kLock.Lock(nullptr);
    AutoMemContext kMemContext(MEM_CONTEXT_TASK_OVERHEAD);
    uint32_t uiMaxThreads = GetMaxThreads();
    if (pInterfaceManager) {
        for (uint32_t i = 0; i < pInterfaceManager->GetMaxThreads(); ++i) {
            delete static_cast<LockFreeQueueInterface*>(pInterfaceManager->pInterfaces[i].pInterface);
        }
        delete pInterfaceManager;
    }
    pInterfaceManager = nullptr;
    iCount = 0;
    while (pHead) {
        LockFreeQueueNode* pNext = pHead->pNext;
        pHead->Data = T();
        delete pHead;
        pHead = pNext;
    }
    if (!abCalledByDestructor) {
        pHead = new LockFreeQueueNode;
        pTail = pHead;
        pInterfaceManager = new ThreadSpecificInterfaceManager(uiMaxThreads);
    }
    kLock.Unlock();
}

// Allocates the node in the task memory context, then links it after the tail with compare-and-swap, helping a lagging
// tail along first; the tail is moved to the new node last. The tail is pinned while it is read. A node link is swapped
// as an untyped pointer word, so the link's address is passed as one.
template <class T>
void LockFreeQueue<T>::LockFreeQueueInterface::Push(const T& arData) {
    LockFreeQueue* pQueue = static_cast<LockFreeQueue*>(pOwner);
    AutoMemContext kMemContext(MEM_CONTEXT_TASK_OVERHEAD);
    LockFreeQueueNode* pNewNode = new LockFreeQueueNode;
    pNewNode->Data = arData;
    LockFreeQueueNode* pTail;
    while (true) {
        pTail = pQueue->pTail;
        *pReferencedNodes[0] = pTail;
        if (pTail != pQueue->pTail) {
            continue;
        }
        LockFreeQueueNode* pNext = pTail->pNext;
        if (pTail != pQueue->pTail) {
            continue;
        }
        if (pNext) {
            CompareAndSwapEx(reinterpret_cast<void* volatile*>(&pQueue->pTail), pNext, pTail);
            continue;
        }
        if (CompareAndSwapEx(reinterpret_cast<void* volatile*>(&pTail->pNext), pNewNode, nullptr)) {
            pQueue->IncrementCount();
            break;
        }
    }
    CompareAndSwapEx(reinterpret_cast<void* volatile*>(&pQueue->pTail), pNewNode, pTail);
    *pReferencedNodes[0] = nullptr;
}

// The head is a dummy node: the item is the next node's, and that node becomes the new dummy head once the
// compare-and-swap moves the head to it; the old head goes to the delete list. Head and next are pinned while read.
// The links are swapped as untyped pointer words, as in Push.
template <class T>
bool LockFreeQueue<T>::LockFreeQueueInterface::Pop(T& arData) {
    LockFreeQueue* pQueue = static_cast<LockFreeQueue*>(pOwner);
    LockFreeQueueNode* pHead;
    while (true) {
        pHead = pQueue->pHead;
        *pReferencedNodes[0] = pHead;
        if (pHead != pQueue->pHead) {
            continue;
        }
        LockFreeQueueNode* pTail = pQueue->pTail;
        LockFreeQueueNode* pNext = pHead->pNext;
        *pReferencedNodes[1] = pNext;
        if (pHead != pQueue->pHead) {
            continue;
        }
        if (!pNext) {
            *pReferencedNodes[0] = nullptr;
            arData = T();
            return false;
        }
        if (pHead == pTail) {
            CompareAndSwapEx(reinterpret_cast<void* volatile*>(&pQueue->pTail), pNext, pTail);
            continue;
        }
        arData = pNext->Data;
        if (CompareAndSwapEx(reinterpret_cast<void* volatile*>(&pQueue->pHead), pNext, pHead)) {
            pQueue->DecrementCount();
            pNext->Data = T();
            break;
        }
    }
    *pReferencedNodes[0] = nullptr;
    *pReferencedNodes[1] = nullptr;
    DeleteNode(pHead);
    return true;
}

// Releases the node's item and links the node into the delete list through the same word; a full batch is scanned.
template <class T>
void LockFreeQueue<T>::LockFreeQueueInterface::DeleteNode(LockFreeQueueNode* apNode) {
    apNode->Data = T();
    apNode->SetNextDelete(pDeleteHead);
    pDeleteHead = apNode;
    ++iDeleteCount;
    if (iDeleteCount == static_cast<LockFreeQueue*>(pOwner)->iDeleteBatchSize) {
        ScanForDeletedNodes();
    }
}

// Frees the listed nodes no thread has pinned and keeps the others listed. The pinned nodes are copied first, in the
// task memory context.
template <class T>
void LockFreeQueue<T>::LockFreeQueueInterface::ScanForDeletedNodes() {
    AutoMemContext kMemContext(MEM_CONTEXT_TASK_OVERHEAD);
    LockFreeQueue* pQueue = static_cast<LockFreeQueue*>(pOwner);
    uint32_t uiReferencedNodeCount = 2 * pQueue->GetMaxThreads();
    LockFreeQueueNode** ppReferencedNodes = new LockFreeQueueNode*[uiReferencedNodeCount];
    std::memcpy(ppReferencedNodes, pQueue->pReferencedNodes, sizeof(LockFreeQueueNode*) * uiReferencedNodeCount);
    LockFreeQueueNode* pNewDeleteHead = nullptr;
    uint32_t uiNewDeleteCount = 0;
    while (pDeleteHead) {
        LockFreeQueueNode* pNode = pDeleteHead;
        pDeleteHead = pNode->GetNextDelete();
        bool bReferenced = false;
        for (uint32_t i = 0; i < uiReferencedNodeCount; ++i) {
            if (pNode == ppReferencedNodes[i]) {
                bReferenced = true;
                break;
            }
        }
        if (bReferenced) {
            pNode->SetNextDelete(pNewDeleteHead);
            pNewDeleteHead = pNode;
            ++uiNewDeleteCount;
        }
        else {
            pNode->SetNextDelete(nullptr);
            delete pNode;
        }
    }
    delete[] ppReferencedNodes;
    pDeleteHead = pNewDeleteHead;
    iDeleteCount = uiNewDeleteCount;
}

// One lock-free queue per task priority.
template <class T>
class LockFreePriorityQueue {
public:
    LockFreePriorityQueue(uint32_t aiPriorityCount, uint32_t aiMaxThreads, uint32_t aiDeleteBatchSize);
    // Slot 0: the deleting destructor.
    virtual ~LockFreePriorityQueue();

    virtual void IncrementCount();
    virtual void DecrementCount();
    // The number of queued elements.
    virtual uint32_t GetCount();

    // Queues the element at aiPriority.
    void Insert(int32_t aiPriority, T* apTask);
    // Empties every priority queue.
    void Clear();
    bool Pop(T& arData);

    static void operator delete(void* apMemory);

    // The queues, one per priority.
    LockFreeQueue<T>** pQueues;
    int32_t iPriorityCount;
    int32_t iCount;
};

// Deleting frees through the engine's heap.
template <class T>
void LockFreePriorityQueue<T>::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}

// Queues the element at aiPriority and counts it.
template <class T>
void LockFreePriorityQueue<T>::Insert(int32_t aiPriority, T* apTask) {
    pQueues[aiPriority]->Push(*apTask);
    IncrementCount();
}

// Empties every priority queue. The count is cleared first; the index compare is unsigned.
template <class T>
void LockFreePriorityQueue<T>::Clear() {
    iCount = 0;
    for (uint32_t i = 0; i < static_cast<uint32_t>(iPriorityCount); ++i) {
        pQueues[i]->RemoveAll(false);
    }
}

template <class T>
void LockFreePriorityQueue<T>::IncrementCount() {
    InterlockedIncrementInt32(&iCount);
}

template <class T>
void LockFreePriorityQueue<T>::DecrementCount() {
    InterlockedDecrementInt32(&iCount);
}

template <class T>
uint32_t LockFreePriorityQueue<T>::GetCount() {
    return iCount;
}

// Takes the first element of the highest priority (the lowest index) that has one.
template <class T>
bool LockFreePriorityQueue<T>::Pop(T& arData) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(iPriorityCount); ++i) {
        if (pQueues[i]->Pop(arData)) {
            DecrementCount();
            return true;
        }
    }
    return false;
}

inline bool CompareAndSwapEx(int32_t* apDestination, int32_t aiExchange, int32_t aiCompare) {
    bool bExchanged = BSAtomicCompareExchange(apDestination, aiCompare, aiExchange) == aiCompare;
    InitializationFlow();
    return bExchanged;
}
