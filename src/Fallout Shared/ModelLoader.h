#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"
#include "BSShader/BSFadeNode.h"
#include "BSSystem/Archive.h"
#include "Fallout Shared/BSTaskThread.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Fallout Shared/KFModel.h"
#include "Fallout Shared/LockFreeMap.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/string.h"

#include <cctype>
#include <cstring>
#include <stdint.h>

class Animation;
class AnimIdle;
class AttachDistant3DTask;
class BipedAnim;
class BSFaceGenNiNode;
class BSStream;
class EffectSetting;
class LoadedFile;
class MagicItem;
class QueuedAnimIdle;
class QueuedHelmet;
class QueuedReference;
class QueuedReplacementKFList;
class TESNPC;
class TESObjectCELL;
class TESObjectTREE;
struct TESTextureListArray;
struct TREE_BILLBOARD_DATA;

template <class T>
class LockFreeQueue;

// A loaded model file, shared by everything that uses it and counted twice: by the clones in use and by the
// holders that keep it loaded on purpose.
class Model {
public:
    Model() : pFilename(nullptr), iRefCount(0), iManualRefCount(0), spObject3D(nullptr) {}
    Model(char* apFilename, NiNode* apObject3D);
    Model(const char* apFilename, BSStream* apStream, bool abPrepareObject, bool abFaceGenModel);
    ~Model();

    // The model's 3D, spObject3D.
    NiNode* GetModel();
    // Takes a manual reference (an atomic increment of iManualRefCount).
    void IncManualRefCount();
    // Atomically takes a reference.
    void IncRefCount();
    // Atomically drops a reference; the model is not deleted.
    void DecRefCount();
    // iRefCount plus iManualRefCount.
    int32_t GetRefCount() const;
    // Drops a manual reference (an atomic decrement of iManualRefCount).
    void DecManualRefCount();
    // Adds aiVal to iManualRefCount atomically.
    void ModManualRefCount(int32_t aiVal);
    void InitModel(const char* apFilename, BSStream* apStream, bool abPrepareObject, bool abFaceGenModel);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    char* pFilename;
    int32_t iRefCount;
    int32_t iManualRefCount;
    NiPointer<NiNode> spObject3D;
};

NiSmartPointer(Model);

// The engine's NiPointer of a model takes and releases references through the model's IncRefCount and DecRefCount.
inline void NiIncRefCount(Model* apModel) {
    apModel->IncRefCount();
}

inline void NiDecRefCount(Model* apModel) {
    apModel->DecRefCount();
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

        // A deleted node is linked into its interface's delete list through Data's storage, as the game does: the word
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

// Under the lock and in the task memory context: drops the interface manager with every thread's interface (the manager
// instance over LockFreeQueueInterface deletes them, as sub_6C74A0 does for LockFreeQueue<bhkRigidBody*>; the
// standalone manager does not know their class), frees every node with its item released; unless abCalledByDestructor,
// a new empty node and a new manager for as many threads.
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
    LockFreeQueueNode** ppReferencedNodes =
        static_cast<LockFreeQueueNode**>(ourMalloc(sizeof(LockFreeQueueNode*) * uiReferencedNodeCount));
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
    ourFree(ppReferencedNodes);
    pDeleteHead = pNewDeleteHead;
    iDeleteCount = uiNewDeleteCount;
}

// The task that attaches a reference's distant 3D on the main thread.
class AttachDistant3DTask : public IOTask {
public:
    AttachDistant3DTask(TESObjectREFR* apRef, NiNode* apNode);

    void Run() override {}
    void Finish() override {}
    void PostProcess() override;
    ~AttachDistant3DTask() override;

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    // The attached reference and its distant 3D node.
    TESObjectREFR* pRef;
    NiPointer<NiNode> spNode;
};

NiSmartPointer(AttachDistant3DTask);

// A file read into memory by a queued load, shared by the loads that use it.
class LoadedFile : public NiMemObject {
public:
    LoadedFile(char* a1, BSFile* a2);
    ~LoadedFile();

    // Takes a reference under the loaded-file lock.
    void IncRefCount();
    // Drops a reference under the loaded-file lock; the last one deletes the file.
    void DecRefCount();

    int32_t iRefCount;
    char* pFileName;
    BSFile* pFile;
    bool bInLoadedFileMap;
    bool bFileUsed;
    uint8_t padding0E[2];

    // The lock the reference counts are changed under.
    static BSCriticalSection LSAccessCrit;
};

NiSmartPointer(LoadedFile);

// The engine's NiPointer of a loaded file takes and releases references through the file's IncRefCount and DecRefCount.
inline void NiIncRefCount(LoadedFile* apFile) {
    apFile->IncRefCount();
}

inline void NiDecRefCount(LoadedFile* apFile) {
    apFile->DecRefCount();
}

// A queued load of a file read into memory whole, by path or of a file already loaded.
class QueuedFileLoad : public QueuedFileEntry {
public:
    QueuedFileLoad(LoadedFile* apFileName, IO_TASK_PRIORITY aePriority);
    QueuedFileLoad(const char* apFileName, IO_TASK_PRIORITY aePriority, IO_FILE_INDEX aeFileIndex);

    void Run() override;
    void Finish() override;
    void QueueMe() override;
    uint8_t GetFileIndex() override;
    ~QueuedFileLoad() override;

    static ARCHIVE_TYPE_INDEX GetArchiveType(IO_FILE_INDEX aeFileIndex);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    NiPointer<LoadedFile> spLoadedFile;
    IO_FILE_INDEX eFileIndex;
};

// A queued load of a FaceGen file (.egm, .tri), or a task for a FaceGen model already loaded.
class QueuedFaceGenFile : public QueuedFileLoad {
public:
    QueuedFaceGenFile(const char* apFileName, IO_TASK_PRIORITY aePriority, IO_FILE_INDEX aeFileIndex);
    QueuedFaceGenFile(LoadedFile* apLoadedFile, IO_TASK_PRIORITY aePriority);
    QueuedFaceGenFile(BSFaceGenModel* apModel, BSFaceGenModelMeshData* apMeshData);
    ~QueuedFaceGenFile() override;

    NiPointer<BSFaceGenModel> spFaceGenModel;
    NiPointer<BSFaceGenModelMeshData> spModelMeshData;
};

// A queued texture load, by path, by archive entry, or of a texture already in the palette.
class QueuedTexture : public QueuedFileEntry {
public:
    QueuedTexture(const char* apFileName, IO_TASK_PRIORITY aePriority);
    QueuedTexture(BSFileEntry* apFileEntry, IO_TASK_PRIORITY aePriority);
    QueuedTexture(NiTexture* apTexture, IO_TASK_PRIORITY aePriority);

    void Run() override;
    void Finish() override;
    void Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) override;
    bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
    void QueueMe() override;
    uint8_t GetFileIndex() override;
    ~QueuedTexture() override;

    // Sets whether the texture is in the queued texture map.
    void SetInQueuedTextureMap(bool abValue);
    // Whether the texture's file is tracked.
    bool GetTrackedFile();
    // Whether the texture is already in the palette.
    bool GetAlreadyInPalette();
    // Sets whether the texture is already in the palette.
    void SetAlreadyInPalette(bool abValue);
    bool GetLoadDegraded();
    void SetLoadDegraded(bool abValue);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    NiPointer<NiTexture> spTexture;
    uint8_t cFlags;
    uint8_t padding35[3];
};

// A queued model load, by path, by form model, or of a model already loaded.
class QueuedModel : public QueuedFileEntry {
public:
    QueuedModel(
        const char* apFileName,
        IO_TASK_PRIORITY aePriority,
        ENUM_LOD_MULT aeLODFadeMult,
        bool abPrepareAfterLoad,
        bool bAddRefCount
    );
    QueuedModel(
        TESModel* apModel,
        IO_TASK_PRIORITY aePriority,
        ENUM_LOD_MULT aeLODFadeMult,
        bool abPrepareAfterLoad,
        bool bAddRefCount
    );
    QueuedModel(Model* apModel, IO_TASK_PRIORITY aePriority);

    void Run() override;
    void Finish() override;
    bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
    void GenerateKey() override;
    void CheckFinished() override;
    uint8_t GetFileIndex() override;
    ~QueuedModel() override;
    void QueueMe() override;
    // The overload taking the texture list the model's textures are queued into.
    virtual void QueueMe(TESTextureList* apTextureList);

    void SetPrepareAfterLoad(bool abValue);
    void SetAddRefCount(bool abValue);
    bool GetFinishedQueuing();
    void SetFinishedQueuing(bool abValue);
    bool GetPrepareAfterLoad();
    bool GetForceLoad();
    bool GetFaceGenModel();
    bool GetAddRefCount();
    // Tests bit 8 of cFlags, the bit SetAlreadyInModelMap sets.
    bool GetAlreadyInModelMap();
    Model* GetModel();
    void SetFaceGenModel(bool abValue);
    void SetOverriddenVisualDistance(float afOverriddenVisualDistance);
    void SetForceLoad(bool abValue);
    void SetAlreadyInModelMap(bool abValue);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    NiPointer<Model> spModel;
    TESModel* pTESModel;
    ENUM_LOD_MULT eLODFadeMult;
    uint8_t cFlags;
    uint8_t padding3D[3];
    float mfOverriddenVisualDistance;
};

// A queued animation (KF) load, by path or of an animation already loaded.
class QueuedKF : public QueuedFileEntry {
public:
    QueuedKF(const char* apFileName, IO_TASK_PRIORITY aePriority);
    QueuedKF(KFModel* apKFModel, IO_TASK_PRIORITY aePriority);
    ~QueuedKF() override;

    void Run() override;
    void Finish() override;
    bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
    void QueueMe() override;
    uint8_t GetFileIndex() override;

    bool GetAlreadyInPalette();
    void SetAlreadyInKFMap(bool abValue);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    NiPointer<KFModel> spKFModel;
    uint8_t cFlags;
    uint8_t padding35[3];
};

// A queued load of an idle's animation for a reference.
class QueuedAnimIdle : public QueuedKF {
public:
    QueuedAnimIdle(const char* apFileName, IO_TASK_PRIORITY aePriority, AnimIdle* apAnimIdle, TESObjectREFR* apRef);
    ~QueuedAnimIdle() override;

    void Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) override;
    bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
    void PostProcess() override;
    void QueueMe() override;
    void CheckFinished() override;

    TESObjectREFR* pQueuedForRef;
    NiPointer<AnimIdle> spAnimIdle;
};

// A queued load of the helmet an actor wears: its models, FaceGen models and clones per biped slot.
class QueuedHelmet : public QueuedFile {
public:
    QueuedHelmet(BipedAnim* apBipedAnim, IO_TASK_PRIORITY aePriority, BIPED_OBJECT aeBipedObject);

    void Run() override;
    void Finish() override;
    void Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) override;
    bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
    void PostProcess() override;
    void QueueMe() override;
    void CheckFinished() override;
    ~QueuedHelmet() override;

    // Marks the helmet queued.
    void SetFinishedQueuing(bool abValue);
    // Whether the helmet has finished queuing.
    bool GetFinishedQueuing();
    // Attaches the helmet to the biped.
    void Attach();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    BipedAnim* pBipedAnim;
    NiPointer<QueuedModel> spQueuedHelmetModel[BIPED_OBJECT_COUNT];
    NiPointer<BSFaceGenModel> spHelmetFaceGenModel[BIPED_OBJECT_COUNT];
    NiPointer<NiNode> spClonedHelmetNode[BIPED_OBJECT_COUNT];
    TESObjectREFR* pRef;
    // The class is 8-byte aligned (BSTask<int64_t>'s Key), so the bytes after this flag are tail padding and there is
    // no member after cFlags.
    uint8_t cFlags;
};

// A queued load of a magic item's effect models.
class QueuedMagicItem : public QueuedFile {
public:
    QueuedMagicItem(MagicItem* apMagicItem, EffectSetting* apAVEffectSetting, IO_TASK_PRIORITY aePriority);
    ~QueuedMagicItem() override;

    void QueueMe() override;
    void CheckFinished() override;

    MagicItem* pMagicItem;
    EffectSetting* pEffectSetting;
};

// A queued load of an NPC's FaceGen head.
class QueuedHead : public QueuedFile {
public:
    QueuedHead(TESNPC* apNPC, IO_TASK_PRIORITY aePriority);
    // Declared so the NiPointer members of incomplete types are not destroyed here.
    ~QueuedHead() override;

    void Run() override;
    bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
    void QueueMe() override;
    void CheckFinished() override;

    // Attaches the head to the reference's biped.
    void Attach(TESObjectREFR* apRef, BipedAnim* apBipedAnim);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    TESNPC* pNPC;
    NiPointer<BSFaceGenNiNode> spBipedNode;
    NiPointer<BSFaceGenNiNode> spSkinnedNode;
};

// A queued load of a reference's 3D: its model, then the clone attached to the reference.
class QueuedReference : public QueuedFile {
public:
    QueuedReference(TESObjectREFR* apRef, IO_TASK_PRIORITY aePriority);

    void Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) override;
    bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
    void PostProcess() override;
    void QueueMe() override;
    void CheckFinished() override;
    virtual void QueueModels();
    virtual void UseDistant3D();
    virtual void AttachDistant3D(NiNode* apNode);
    virtual bool BackgroundClone();
    virtual void Attach();
    virtual void FinishAttach() {}
    ~QueuedReference() override;

    NiAVObject* GetCloned3D();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    TESObjectREFR* pRef;
    NiPointer<QueuedModel> spQueuedModel;
    NiPointer<Model> spModel;
    NiPointer<NiAVObject> spCloned3D;
    NiPointer<AttachDistant3DTask> spAttachDistant3DTask;
};

// Destructor of the IO task queue interface: calls ScanForDeletedNodes while iDeleteCount is not 0.
void LockFreeQueue_Destruct_44F400(LockFreeQueue<NiPointer<IOTask>>::LockFreeQueueInterface* apThis);
// DeleteNode of the IO task queue interface, called by Pop: releases the node's NiPointer, links the node at
// pDeleteHead through its first word, then calls LockFreeQueue_ScanForDeletedNodes_C42B80 once iDeleteCount reaches the
// owner's batch size.
void LockFreeQueue_DeleteNode_C3F090(
    LockFreeQueue<NiPointer<IOTask>>::LockFreeQueueInterface* apThis,
    LockFreeQueue<NiPointer<IOTask>>::LockFreeQueueNode* apNode
);
// ScanForDeletedNodes of the IO task queue interface: copies the 2 * max threads referenced nodes, frees each node of
// the delete list no thread references (BSTask<long long>::DecRefCount on its data, then MemoryManager::Deallocate),
// and keeps the others.
void LockFreeQueue_ScanForDeletedNodes_C42B80(LockFreeQueue<NiPointer<IOTask>>::LockFreeQueueInterface* apThis);
// The flag helpers of the queued loads.
void SetFlag1(bool abValue, uint8_t* apFlags);
void SetFlag2(bool abValue, uint8_t* apFlags);
bool HasFlag2(uint8_t aFlags);
bool IsFlag1(uint8_t aFlags);
uint8_t* SetFlag4(bool abValue, uint8_t* apFlags);
// Whether flag 4 is set.
bool MLGetFlag4(uint8_t aFlags);
// Allocates a thread-local storage index through the kernel32 import; returns the index, or
// 0xFFFFFFFF when none is free.
uint32_t BSTlsAlloc();
// Frees a TLS index through the kernel32 import; returns its result.
void BSTlsFree(uint32_t aiTlsIndex);
// The interface's GetFirst, called by the map's GetNext: the bucket's first unmarked node's key is copied through
// CopyKeyDirect and its data through NiPointer's assignment.
bool LockFreeMap_GetFirst_906AF0(
    LockFreeMap<TESObjectREFR*, NiPointer<QueuedReference>>::LockFreeMapInterface* apThis,
    uint32_t aiBucket,
    TESObjectREFR*& arKey,
    NiPointer<QueuedReference>& arData
);
// Finds arKey, then copies the next unmarked node's key and data as GetFirst does.
bool LockFreeMap_GetNext_906C10(
    LockFreeMap<TESObjectREFR*, NiPointer<QueuedReference>>::LockFreeMapInterface* apThis,
    uint32_t aiBucket,
    TESObjectREFR*& arKey,
    NiPointer<QueuedReference>& arData
);
bool MLGetFlag_8(uint8_t aFlags);
bool CompareAndSwapEx(int32_t* apDestination, int32_t aiExchange, int32_t aiCompare);
// The pointer-width form of CompareAndSwapEx, used for the node pointers of the lock-free queues and maps.
bool CompareAndSwapEx(void* volatile* apDestination, void* apExchange, void* apCompare);
// Copy for a float vector: memmove_s of the elements from [apFirst, apLast) to apDest, returning the end of the copy.
float* CopyOpt(float* apFirst, float* apLast, float* apDest);
void QueuedReplacementKFList_43DC50(QueuedFile* apThis, [[maybe_unused]] void* apEdx);
// Destroys the float vector: _Tidy, then the base destructor. It is shared by the 4-byte element vectors
// (FffTriStatMorphS's vector<uint32_t> too), so apThis is untyped.
void DestroyFloatVector(void* apThis, [[maybe_unused]] void* apEdx);
bool MLGetFlag_16(uint8_t aFlags);
void MLSetFlag_16(char a1, uint8_t* a2);
bool MLGetFlag_32(uint8_t aFlags);
void MLSetFlag_32(char a1, uint8_t* a2);
int32_t SetTlsValue(uint32_t aiTlsIndex, void* apValue);

NiSmartPointer(QueuedReference);

// A queued tree reference.
class QueuedTree : public QueuedReference {
public:
    QueuedTree(TESObjectREFR* apRef, IO_TASK_PRIORITY aePriority);

    void QueueModels() override;
    void UseDistant3D() override;
};

// A queued actor reference.
class QueuedActor : public QueuedReference {
public:
    QueuedActor(TESObjectREFR* apRef, IO_TASK_PRIORITY aePriority);

    void QueueMe() override;
    void QueueModels() override;
    ~QueuedActor() override;
};

// A queued creature reference.
class QueuedCreature : public QueuedActor {
public:
    QueuedCreature(TESObjectREFR* apRef, IO_TASK_PRIORITY aePriority);

    void QueueModels() override;
    ~QueuedCreature() override;
};

// A queued NPC reference, with its head and helmet loads.
class QueuedCharacter : public QueuedActor {
public:
    QueuedCharacter(TESObjectREFR* apRef, IO_TASK_PRIORITY aePriority);

    void QueueModels() override;
    bool BackgroundClone() override;
    void Attach() override;
    void FinishAttach() override;
    ~QueuedCharacter() override;

    // Queues the head parts.
    void AttachHeadParts();

    NiPointer<QueuedHead> spQueuedHead;
    NiPointer<QueuedHelmet> spQueuedHelmet;
};

// The queued player reference.
class QueuedPlayer : public QueuedCharacter {
public:
    QueuedPlayer(IO_TASK_PRIORITY aePriority);

    void QueueModels() override;
    void Attach() override;
    ~QueuedPlayer() override;
};

// The thread that clones loaded models for queued references. Only some of its entry points are declared.
class BackgroundCloneThread : public BSTaskThread {
public:
    BackgroundCloneThread(uint32_t aiMaxThreads);
    ~BackgroundCloneThread() override;

    // The thread's loop body.
    void ThreadUpdate() override;

    // Queues apQueuedReference for cloning and wakes the thread.
    void AddQueuedReference(QueuedReference* apQueuedReference);
    // The references waiting to be cloned plus the one being cloned.
    uint32_t GetQueuedCount();
    // Pause and resume the cloning.
    void Pause();
    void Resume();
    void SetExiting();

    // Frees through ourFree, not BSTaskThread's MemoryManager::Deallocate.
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    bool bExit;
    int32_t iRunningCount;
    LockFreeQueue<NiPointer<QueuedReference>>* pCloneReferencesQueue;
};

// Queues model, animation, texture and reference loads on the IO manager and keeps the maps of what is loaded or
// queued.
class ModelLoader {
public:
    ModelLoader();
    ~ModelLoader();

    // Queues the load of a tree's model.
    void QueueTreeModel(
        TESObjectREFR* apRef,
        TESObjectTREE* apTree,
        IO_TASK_PRIORITY aePriority,
        QueuedFile* apCounter,
        ENUM_LOD_MULT aeLODMult
    );
    // Queues the load of a tree's model, handing the queued model back through
    // aspQueuedModel.
    void QueueTreeModel(
        TESObjectREFR* apRef,
        TESObjectTREE* apTree,
        NiPointer<QueuedModel>& aspQueuedModel,
        IO_TASK_PRIORITY aePriority,
        QueuedFile* apCounter,
        ENUM_LOD_MULT aeLODMult
    );
    // Queues the load of apObj's model.
    void
    QueueBoundObject(TESBoundObject* apObj, IO_TASK_PRIORITY aePriority, QueuedFile* apParent, TESObjectREFR* apRefr);
    // Removes apAnim's queued replacement KF list from the map.
    void RemoveReplacementKFListFromMap(Animation* apAnim);
    // Releases the key frame model called apName.
    void ReleaseKF(const char* apKFFileName, bool abDeleteIfUnreferenced);
    // Whether textures queued by this thread load at a degraded quality.
    bool GetLoadTexturesDegraded();
    // Sets the flag for this thread; the previous value.
    bool SetLoadTexturesDegraded(bool abValue);

    // Remove the entry of the key from the map; true when there was one.
    void RemoveAnimIdleFromMap(AnimIdle* apAnimIdle);
    void RemoveHelmetFromMap(TESObjectREFR* apRef);
    // Hands apQueuedReference to the background clone thread.
    void AddBackgroundCloneReference(QueuedReference* apQueuedReference);
    void RemoveReferenceFromMap(TESObjectREFR* apRef);

    // Reuses the queued load of apTextureFileEntry if it is not running yet: adds apParent as one more parent and
    // raises the load's priority to aePriority; false when there is no such load.
    bool ReuseQueuedTexture(BSFileEntry* apTextureFileEntry, IO_TASK_PRIORITY aePriority, QueuedFile* apParent);
    // Queue a texture load unless the texture is already in the palette at the right quality; false when nothing
    // was queued and there is no parent to report to.
    bool QueueTexture(const char* apTextureFileName, IO_TASK_PRIORITY aePriority, QueuedFile* apParent);
    bool QueueTexture(BSFileEntry* apTextureFileEntry, IO_TASK_PRIORITY aePriority, QueuedFile* apParent);

    // Queue a model load, or a task that waits on the model already loaded; aspQueuedModel receives the task.
    void QueueModel(
        TESModel* apModel,
        IO_TASK_PRIORITY aePriority,
        QueuedFile* apParent,
        ENUM_LOD_MULT aeLODFadeMult,
        bool abPrepareAfterLoad,
        bool abAddRefCount,
        bool abFaceGenModel
    );
    void QueueModel(
        TESModel* apModel,
        NiPointer<QueuedModel>& aspQueuedModel,
        IO_TASK_PRIORITY aePriority,
        QueuedFile* apParent,
        ENUM_LOD_MULT aeLODFadeMult,
        bool abPrepareAfterLoad,
        bool abAddRefCount,
        bool abFaceGenModel,
        float afOverriddenVisualDistance
    );
    void QueueModel(
        const char* apModelFileName,
        IO_TASK_PRIORITY aePriority,
        QueuedFile* apParent,
        ENUM_LOD_MULT aeLODFadeMult,
        bool abPrepareAfterLoad,
        bool abAddRefCount,
        bool abFaceGenModel
    );
    void QueueModel(
        const char* apModelFileName,
        NiPointer<QueuedModel>& aspQueuedModel,
        IO_TASK_PRIORITY aePriority,
        QueuedFile* apParent,
        ENUM_LOD_MULT aeLODFadeMult,
        bool abPrepareAfterLoad,
        bool abAddRefCount,
        bool abFaceGenModel
    );
    void QueueModel(
        const char* apModelFileName,
        TESTextureList* apTextureList,
        IO_TASK_PRIORITY aePriority,
        QueuedFile* apParent,
        ENUM_LOD_MULT aeLODFadeMult,
        bool abPrepareAfterLoad,
        bool abAddRefCount,
        bool abFaceGenModel
    );
    void QueueModel(
        const char* apModelFileName,
        TESTextureList* apTextureList,
        NiPointer<QueuedModel>& aspQueuedModel,
        IO_TASK_PRIORITY aePriority,
        QueuedFile* apParent,
        ENUM_LOD_MULT aeLODFadeMult,
        bool abPrepareAfterLoad,
        bool abAddRefCount,
        bool abFaceGenModel
    );

    // Queues an animation load, or a task that waits on the animation already loaded; false when the animation
    // is loaded and there is no parent to report to.
    bool QueueKF(const char* apKFFileName, IO_TASK_PRIORITY aePriority, QueuedFile* apParent);
    // Queues the load of a reference's 3D, or moves its queued load to aePriority.
    void QueueReference(TESObjectREFR* apRef, IO_TASK_PRIORITY aePriority, bool abAllowQueueReferenceQueuing);
    // Queues the model of a bound object, for apQueuedForRef when it is not null.
    void QueueBoundObject(
        TESBoundObject* apBoundObject,
        NiPointer<QueuedModel>& aspQueuedModel,
        IO_TASK_PRIORITY aePriority,
        QueuedFile* apParent,
        TESObjectREFR* apQueuedForRef
    );
    // Queues the load of an NPC's head.
    void
    QueueHead(TESNPC* apNPC, NiPointer<QueuedHead>& aspQueuedHead, IO_TASK_PRIORITY aePriority, QueuedFile* apParent);

    // The model to load for apBoundObject (as placed by apQueuedForRef).
    TESModel* GetModelForBoundObject(TESBoundObject* apBoundObject, TESObjectREFR* apQueuedForRef);
    // The queued load of apFileEntry, or null.
    QueuedTexture* GetQueuedTextureFromMap(BSFileEntry* apFileEntry);

    // Queues the load of the helmet apBipedAnim's actor wears in aeBipedObject, unless one is queued; aspQueuedHelmet
    // receives the queued load (null when it was already queued or could not be added).
    void QueueHelmet(
        BipedAnim* apBipedAnim,
        NiPointer<QueuedHelmet>& aspQueuedHelmet,
        IO_TASK_PRIORITY aePriority,
        QueuedFile* apParent,
        BIPED_OBJECT aeBipedObject
    );
    // Queues the load of apAnimIdle's animation for apQueuedForRef unless one is queued; null.
    KFModel* QueueAnimIdle(
        const char* apFilename,
        AnimIdle* apAnimIdle,
        IO_TASK_PRIORITY aePriority,
        TESObjectREFR* apQueuedForRef
    );
    // Releases the KF file apFilename, deleting it when unreferenced: ReleaseKF(apFilename, true).
    void ReleaseKFFile(const char* apFilename);
    // Drops aiNumDecrements manual references on the model loaded from apModelFileName, then, with
    // abDeleteIfUnreferenced, deletes it when nothing references it.
    void ReleaseModel(const char* apModelFileName, bool abDeleteIfUnreferenced, uint32_t aiNumDecrements);
    // Cancels the queued 3D load of apRef and its helmet load.
    void CancelReference(TESObjectREFR* apRef);
    // Whether apRef's 3D load is queued.
    bool IsReferenceQueued(TESObjectREFR* apRef);
    // Queues the animations of apModel: a dead reference's skeleton gets its idle, holster and death KFs; otherwise a
    // creature skeleton (or abForceAnimationLoad) its KF file list, an NPC its idle animations, and a TESAnimation
    // model its special animations.
    void QueueAnimations(
        TESModel* apModel,
        IO_TASK_PRIORITY aePriority,
        QueuedFile* apParent,
        TESObjectREFR* apQueuedForRef,
        bool abCreature,
        bool abForceAnimationLoad
    );
    // Empty in the PC build.
    void UpgradeTextures(NiAVObject* apObject);
    // Queue the models (with their alternate textures) or animations of a file list, under apBasePath when it is
    // not null; QueueKFList frees each name and empties the list.
    void QueueModelList(
        BSSimpleList<const char*>* apModelList,
        TESTextureListArray* apTextureListArray,
        IO_TASK_PRIORITY aePriority,
        QueuedFile* apParent,
        const char* apBasePath
    );
    void QueueKFList(
        BSSimpleList<const char*>* apKFList,
        IO_TASK_PRIORITY aePriority,
        QueuedFile* apParent,
        const char* apBasePath
    );
    // Moves each queued reference load to its cell's priority (critical for the 3D-critical references of
    // apCriticalCell).
    void UpdateQueuedReferencePriorities(TESObjectCELL* apCriticalCell);
    // The number of queued reference loads.
    uint32_t GetQueuedReferenceCount();
    // Sets the window title back once nothing is loading (in windowed mode).
    void DisplayDebugLoadingText();
    // The number of references the background clone thread has to clone.
    uint32_t GetBackgroundCloningCount();
    // Pause and resume the background clone thread.
    void PauseBackgroundCloning();
    void ResumeBackgroundCloning();
    // Add an entry to the model (KF) map without replacing one; true when it was added.
    bool AddModel(const char* apModelFileName, Model* apModel);
    bool AddKF(const char* apKFFileName, KFModel* apKFModel);
    // Loads a model (animation) on this thread unless it is loaded; the model's 3D (the animation), null when the
    // load failed. LoadFile takes a reference unless abDontIncrementRefCount, LoadKF a manual one.
    NiNode* LoadFile(
        const char* apFilename,
        ENUM_LOD_MULT aeLODFadeMult,
        bool abPrepareObject,
        bool abSuppressWarning,
        bool abFaceGenModel,
        bool abDontIncrementRefCount
    );
    KFModel* LoadKF(const char* apFilename);

    // Copies the model loaded from apFilename to aspOutModel (null when there is none); true when there is one.
    bool FindModel(const char* apFilename, NiPointer<Model>& aspOutModel);
    // Adds the names of the mesh files matching apSearchName in apBaseFilename's directory to apFileList (a new
    // list when it is null); the list.
    BSSimpleList<const char*>*
    BuildFileList(const char* apSearchName, const char* apBaseFilename, BSSimpleList<const char*>* apFileList);
    // The .kf files of an idle directory or of apBaseFilename's directory (with its locomotion and hurt
    // animations when abAddLocomotion), registered with the idle manager; the list.
    BSSimpleList<const char*>* BuildKFFileList(
        const char* apBaseFilename,
        bool abAddLocomotion,
        bool abAddMTAnims,
        ANIM_GROUP_WEAPON_ENUM aeWeaponGroup
    );
    // Adds a heap copy of every name in apList to apDestList (a new list when it is null); the list.
    BSSimpleList<const char*>*
    CopyFilenameList(BSSimpleList<const char*>* apList, BSSimpleList<const char*>* apDestList);
    // Whether apCell has critical references queued.
    bool IsFileQueuedForCell(TESObjectCELL* apCell);
    // Queues the load of a FaceGen file, or a task for the file already loaded, under apParent.
    void QueueFaceGenFile(
        const char* apFileName,
        IO_TASK_PRIORITY aePriority,
        QueuedFile* apParent,
        IO_FILE_INDEX aeFileIndex
    );
    // Queue the .egm (.tri) files of apModel for head part aiHeadPart, or tasks for the FaceGen models already
    // cached.
    void QueueEGMFile(TESModel* apModel, IO_TASK_PRIORITY aePriority, QueuedFile* apParent, int32_t aiHeadPart);
    void QueueTRIFile(TESModel* apModel, IO_TASK_PRIORITY aePriority, QueuedFile* apParent, int32_t aiHeadPart);
    // The loaded-file map: add (without replacing), remove and look up by file name.
    bool AddLoadedFileToMap(const char* apFileName, LoadedFile* apLoadedFile);
    void RemoveLoadedFileFromMap(const char* apFileName);
    LoadedFile* GetLoadedFileFromMap(const char* apFileName);
    // The animation loaded from apFilename, or null.
    KFModel* FindKFModel(const char* apFilename);
    // Deletes the models and animations nothing references (not the power-attack animations); while a
    // save game loads, outside a memory pass, only marks the free as delayed.
    void ClearUnusedModels(bool abMemoryPass);
    // Adds apRef's addon nodes to apNode when its BSXFlags ask for them.
    void LoadAddonNodes(TESObjectREFR* apRef, NiNode* apNode);
    // Loads the addon nodes of apNode.
    void LoadAddons(NiNode* apNode);
    // Runs the queued distant-3D attach tasks; true.
    bool UpdateAttachDistant3DQueue();
    // Queues the references waiting to be queued until the time budget runs out; whether all were queued.
    bool UpdateReferencesToQueue();
    // The queued-texture map: add (without replacing) and remove by archive entry.
    bool AddQueuedTextureToMap(BSFileEntry* apFileEntry, QueuedTexture* apQueuedTexture);
    void RemoveQueuedTextureFromMap(BSFileEntry* apFileEntry);
    // Releases one reference on the model loaded from apModelFileName, deleting it when unreferenced.
    void ReleaseFile(const char* apModelFileName);
    // The model map's spin lock: take, release, try (PC-only).
    void EnterModelMapCS();
    void UnlockModelMap();
    bool TryLockModelMap();
    void SetExiting();
    void QueueReplacementKFList(
        BSSimpleList<char const*>* apKFList,
        Animation* apAnim,
        IO_TASK_PRIORITY aePriority,
        QueuedFile* apParent,
        const char* apBasePath
    );

protected:
    // private in the engine; protected here so the replacement table can name it.
    // Deletes apModel and removes apModelFileName from the model map when nothing references the model; while a
    // save game loads, only marks the free as delayed.
    void TryAndRemoveModel(Model* apModel, const char* apModelFileName);

public:
    void QueueAttachDistant3DTask(AttachDistant3DTask* apTask);
    void OutputModelMapContents(BSFile* apFile, bool abShowZeroCountModels);
    void QueueMagicItem(MagicItem* apMagicItem, EffectSetting* apAVEffectSetting, IO_TASK_PRIORITY aePriority);
    void CancelReplacementKFList(Animation* apAnim);
    void CancelAnimIdle(AnimIdle* apAnimIdle);
    void CancelReferencesForCell(TESObjectCELL* apCell);
    void QueueCreatureParts(
        TESModel* apModel,
        IO_TASK_PRIORITY aePriority,
        QueuedFile* apParent,
        TESObjectREFR* apQueuedForRef
    );
    void QueueTreeBillboard(const char* apFilename, TREE_BILLBOARD_DATA* apBillboardData);
    void CancelAllFiles();

    // The IO manager's callbacks, on the global model loader.
    static void UpdateQueueCallback();
    static bool UpdateSecondaryQueueCallback();
    static void LoadQueuedPriorityCallback(float afPercent);
    static uint32_t GetExternalCountCallback();
    static void PauseCallback();
    static void ResumeCallback();
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

protected:
    // The thread's bLoadTexturesDegraded, a thread-local static.
    static bool& GetThreadLoadTexturesDegraded();

public:
    LockFreeCaseInsensitiveStringMap<Model*>* pModelMap;
    LockFreeCaseInsensitiveStringMap<KFModel*>* pKFModelMap;
    LockFreeMap<TESObjectREFR*, NiPointer<QueuedReference>>* pQueuedReferencesMap;
    LockFreeMap<TESObjectREFR*, NiPointer<QueuedReference>>* pReferencesToQueueMap;
    LockFreeMap<AnimIdle*, NiPointer<QueuedAnimIdle>>* pQueuedAnimIdleMap;
    LockFreeMap<Animation*, NiPointer<QueuedReplacementKFList>>* pQueuedReplacementKFListMap;
    LockFreeMap<TESObjectREFR*, NiPointer<QueuedHelmet>>* pQueuedHelmetMap;
    LockFreeQueue<NiPointer<AttachDistant3DTask>>* pAttachDistant3DTaskQueue;
    LockFreeMap<BSFileEntry*, NiPointer<QueuedTexture>>* pQueuedTextureMap;
    LockFreeCaseInsensitiveStringMap<LoadedFile*>* pLoadedFileMap;
    BackgroundCloneThread* pBackgroundCloneThread;
    bool bHasDelayedFree;
    uint8_t padding2D[3];
};

inline ModelLoader* pModelLoader;

// The game settings ModelLoader.cpp defines.
extern SettingT<INISettingCollection> fLODFadeOutPercent;
extern SettingT<INISettingCollection> fLODBoundRadiusMult;
