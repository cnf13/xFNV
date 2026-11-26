#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/IOManager.h"
#include "BSSystem/Archive.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class QueuedChildren;
class QueuedParents;

// A file load run by the IO manager. A queued file can wait on child loads and has a parent it reports to.
class QueuedFile : public IOTask {
public:
    QueuedFile(IO_TASK_PRIORITY aePriority);

    void Run() override {}
    void Finish() override;
    void Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) override;
    void Requeue(IO_TASK_PRIORITY aeNewPriority) override;
    // Queues the load; most subclasses do their own work first.
    virtual void QueueMe() {}
    virtual void NotifyChildFinished(QueuedFile* apChild);
    virtual void CheckFinished();
    ~QueuedFile() override;

    // Holds apParent and adds this file to its parent's child list.
    void SetParent(QueuedFile* apParent);
    // Adds apChild to the child list, creating the list first.
    void AddChild(QueuedFile* apChild);
    // Adds apAdditionalParent to the additional parents unless it is there already, then adds this file to its
    // children.
    void AddAdditionalParent(QueuedFile* apAdditionalParent);
    // Whether every child load finished: no children, or as many finished as there are.
    bool GetAllChildrenFinished();
    uint32_t GetChildCount();

    static void operator delete(void* apMemory) {
        MemoryManager::Instance().Deallocate(apMemory);
    }

    MEM_CONTEXT eContext;
    NiPointer<QueuedFile> spParent;
    QueuedChildren* pChildren;
    QueuedParents* pAdditionalParents;
};

NiSmartPointer(QueuedFile);

// The child loads a queued file waits for, with the count of those that finished.
class QueuedChildren : public BSSimpleArray<NiPointer<QueuedFile>, 1024> {
public:
    QueuedChildren();
    ~QueuedChildren() override;

    // Returns iNumChildrenFinished.
    uint32_t GetChildCountFinished();
    // Atomically increments iNumChildrenFinished.
    void AddFinishedChild();

    static void operator delete(void* apMemory) {
        MemoryManager::Instance().Deallocate(apMemory);
    }

    uint32_t iNumChildrenFinished;
};

// A queued load of one file, named by path or by archive entry.
class QueuedFileEntry : public QueuedFile {
public:
    QueuedFileEntry(IO_TASK_PRIORITY aePriority);
    ~QueuedFileEntry() override;

    void GenerateKey() override;
    // Which kind of file the entry loads (an IO_FILE_INDEX).
    virtual uint8_t GetFileIndex() = 0;

    // Writes a description of the entry into apDescription.
    bool GetDescription(char* apDescription, size_t aiBufferSize, const char* apFileEntryTypeDescription);
    BSFileEntry* GetFileEntry() {
        return pFileEntry;
    }
    void SetFileName(const char* apFileName);
    void SetFileEntry(BSFileEntry* apEntry);
    void GetLookupFileName(ARCHIVE_TYPE_INDEX aeArchiveTypeIndex, char* apLookupFileName);
    // The file of the entry: the file of its archive entry, else the loose file at its lookup name; null when
    // neither exists.
    BSFile* GetFile(ARCHIVE_TYPE_INDEX aeArchiveTypeIndex, ARCHIVE_TYPE aeArchiveType);
    void LookupFileEntry(ARCHIVE_TYPE_INDEX aeArchiveTypeIndex);

    char* pFileName;
    BSFileEntry* pFileEntry;
};

inline int32_t iQueuedFileCount;

inline void QueuedFile::NotifyChildFinished(QueuedFile* apChild) {
    pChildren->AddFinishedChild();
    CheckFinished();
}

inline void QueuedChildren::AddFinishedChild() {
    NiAtomicIncrement(iNumChildrenFinished);
}

inline void QueuedFile::SetParent(QueuedFile* apParent) {
    this->spParent = apParent;
    if (this->spParent) {
        this->spParent->AddChild(this);
    }
}

inline bool QueuedFile::GetAllChildrenFinished() {
    if (!pChildren) {
        return true;
    }
    uint32_t uiSize = pChildren->GetSize();
    return uiSize == pChildren->GetChildCountFinished();
}

inline uint32_t QueuedFile::GetChildCount() {
    if (pChildren) {
        return pChildren->GetSize();
    }

    return 0;
}

inline uint32_t QueuedChildren::GetChildCountFinished() {
    return iNumChildrenFinished;
}
