#pragma once

#include "BSCore/BSSpinLock.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cstring>
#include <stdint.h>
#include <type_traits>

class MobileObject;
class PathingRequest;
class TESObjectREFR;

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
    void* sub_529300(char a1);
    void sub_529330();
    ThreadSpecificInterfaceManager* sub_6C71A0(char a1);
    void Shutdown();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    uint32_t uiMaxThreads;
    uint32_t uiTLSIndex;
    ThreadSpecificInterface* pInterfaces;
    volatile int32_t uiThreadCount;
};

// The pointer-width compare-and-swap the bodies below link and unlink nodes with: stores apExchange at *apDestination
// when it holds apCompare; true when it did. Declared with the int form in ModelLoader.h, which includes this header,
// and defined in ModelLoader.cpp. A node link (a MarkedPointer, its mark in bit 0) is swapped as an untyped pointer
// word, so the bodies pass the link's address as one.
bool CompareAndSwapEx(void* volatile* apDestination, void* apExchange, void* apCompare);
void* GetTlsValue(uint32_t dwTlsIndex);
void sub_6C74A0(ThreadSpecificInterfaceManager* apThis, [[maybe_unused]] void* apEdx);

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
        uint8_t padding0D[3];
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
        ~MarkedPointer() {
            pPointer = nullptr;
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

// The bucket table (null, unmarked pointers), the per-thread node table (three per thread), the interface manager
// and the lock.
template <class Key, class Data>
LockFreeMap<Key, Data>::LockFreeMap(uint32_t auiMaxThreads, uint32_t auiNumBuckets, uint32_t auiDeleteBatchSize)
  : InterfacedClass(),
    pReferencedNodes(static_cast<HashListNode**>(ourMalloc(sizeof(HashListNode*) * 3 * auiMaxThreads))),
    uiNumBuckets(auiNumBuckets), pBuckets(nullptr), uiDeleteBatchSize(auiDeleteBatchSize),
    pManager(new ThreadSpecificInterfaceManager(auiMaxThreads)), uiCount(0), kLock() {
    pBuckets = static_cast<MarkedPointer<HashListNode>*>(ourMalloc(sizeof(MarkedPointer<HashListNode>) * uiNumBuckets));
    for (uint32_t i = 0; i < uiNumBuckets; ++i) {
        ::new (&pBuckets[i]) MarkedPointer<HashListNode>();
    }
}

// Empties the map for good (RemoveAll drops the interface manager), then frees the bucket and node tables.
template <class Key, class Data>
LockFreeMap<Key, Data>::~LockFreeMap() {
    RemoveAll(true);
    ourFree(pBuckets);
    ourFree(pReferencedNodes);
}

template <class Key, class Data>
LockFreeMap<Key, Data>::LockFreeMapIterator::~LockFreeMapIterator() {}

// The calling thread's interface: the thread's three node references, allocated in the task memory context.
template <class Key, class Data>
InterfacedClass::BaseInterface* LockFreeMap<Key, Data>::AllocateInterface(uint32_t aiNum) {
    AutoMemContext kMemContext(MEM_CONTEXT_TASK_OVERHEAD);
    return new LockFreeMapInterface(
        this, &pReferencedNodes[3 * aiNum], &pReferencedNodes[3 * aiNum + 1], &pReferencedNodes[3 * aiNum + 2]
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
    HashListNode** ppReferencedNodes =
        static_cast<HashListNode**>(ourMalloc(sizeof(HashListNode*) * uiReferencedNodeCount));
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
    ourFree(ppReferencedNodes);
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
            }
            else {
                MarkedPointer<HashListNode> kUnmarkedNext(false, Next.GetPointer());
                if (!CompareAndSwapEx(
                        reinterpret_cast<void* volatile*>(&pPrev->pPointer), kUnmarkedNext.pPointer,
                        kUnmarkedCurrent.pPointer
                    )) {
                    break;
                }
                DeleteNode(Current.GetPointer());
            }
            Current.pPointer = Next.pPointer;
            *pReferencedNodes[1] = Next.GetPointer();
        }
    }
}

// Stores arData in the node of aKey when abReplaceExisting, or links a new node with a copy of the key (the owner's
// CopyKey) in front of the node Find stopped at; false when the key is there and kept. A new node left unused goes
// without its key copy being freed, deliberately, as the game does.
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
                reinterpret_cast<void* volatile*>(&Current->pNext.pPointer), kMarkedNext.pPointer,
                kUnmarkedNext.pPointer
            )) {
            continue;
        }
        MarkedPointer<HashListNode> kUnmarkedCurrent(false, Current.GetPointer());
        if (CompareAndSwapEx(
                reinterpret_cast<void* volatile*>(&pPrev->pPointer), kUnmarkedNext.pPointer, kUnmarkedCurrent.pPointer
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

// RemoveAt's unlinking for the node FindValue finds. Leaves the owner's count as it is, as the game does.
template <class Key, class Data>
bool LockFreeMap<Key, Data>::LockFreeMapInterface::Remove(uint32_t aiBucket, Data& arData) {
    bool bResult = false;
    while (FindValue(aiBucket, arData)) {
        MarkedPointer<HashListNode> kUnmarkedNext(false, Next.GetPointer());
        MarkedPointer<HashListNode> kMarkedNext(true, Next.GetPointer());
        if (!CompareAndSwapEx(
                reinterpret_cast<void* volatile*>(&Current->pNext.pPointer), kMarkedNext.pPointer,
                kUnmarkedNext.pPointer
            )) {
            continue;
        }
        MarkedPointer<HashListNode> kUnmarkedCurrent(false, Current.GetPointer());
        if (CompareAndSwapEx(
                reinterpret_cast<void* volatile*>(&pPrev->pPointer), kUnmarkedNext.pPointer, kUnmarkedCurrent.pPointer
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
            }
            else {
                MarkedPointer<HashListNode> kUnmarkedNext(false, Next.GetPointer());
                if (!CompareAndSwapEx(
                        reinterpret_cast<void* volatile*>(&pPrev->pPointer), kUnmarkedNext.pPointer,
                        kUnmarkedCurrent.pPointer
                    )) {
                    break;
                }
                DeleteNode(Current.GetPointer());
            }
            Current.pPointer = Next.pPointer;
            *pReferencedNodes[1] = Next.GetPointer();
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
