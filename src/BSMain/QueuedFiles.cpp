#include "BSMain/QueuedFiles.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedParents.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSArchive.h"
#include "BSSystem/BSHash.h"
#include "BSSystem/FileFinder.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/string.h"

// The queued files' child and parent arrays share this instance.
using QueuedFileArray = BSSimpleArray<NiPointer<QueuedFile>, 1024>;

// The file loads in the memory context of the thread that creates it.
QueuedFile::QueuedFile(IO_TASK_PRIORITY aePriority)
  : IOTask(aePriority), eContext(GetThreadTLSData()->eMemContext), spParent(nullptr), pChildren(nullptr),
    pAdditionalParents(nullptr) {}

void QueuedFile::Finish() {
    CheckFinished();
}

void QueuedFile::Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) {
    InterlockedIncrementInt32(&iRefCount);
    if (apTaskOwner) {
        if (spParent.m_pObject == apTaskOwner) {
            NiPointer<QueuedFile>* p_spParent = &spParent;
            if (spParent.m_pObject) {
                p_spParent->m_pObject->DecRefCount();
                p_spParent->m_pObject = nullptr;
                if (p_spParent->m_pObject) {
                    InterlockedIncrementInt32(&p_spParent->m_pObject->iRefCount);
                }
            }
        }
        else {
            if (pAdditionalParents) {
                for (uint32_t a2 = 0; a2 < pAdditionalParents->iSize; ++a2) {
                    if (pAdditionalParents->pBuffer[a2] == apTaskOwner) {
                        pAdditionalParents->RemoveFastReleasing(a2, 1);
                        break;
                    }
                }
                if (!pAdditionalParents->iSize) {
                    if (pAdditionalParents) {
                        delete pAdditionalParents;
                    }
                    pAdditionalParents = nullptr;
                }
            }
        }
    }
    else {
        NiPointer<QueuedFile>* v4 = &spParent;
        if (spParent.m_pObject) {
            v4->m_pObject->DecRefCount();
            v4->m_pObject = nullptr;
            if (v4->m_pObject) {
                InterlockedIncrementInt32(&v4->m_pObject->iRefCount);
            }
        }
        if (pAdditionalParents) {
            delete pAdditionalParents;
        }
        pAdditionalParents = nullptr;
    }
    if (spParent.m_pObject || pAdditionalParents) {
        if (aePreviousState == BS_TASK_STATE_QUEUED) {
            pIOManager->AddTask(this);
        }
        else {
            // EXE-INLINED 0x449190
            // The exe hands the state field to InterlockedCompareExchange as a plain LONG.
            InterlockedCompareExchange32(reinterpret_cast<volatile int32_t*>(&eState), aePreviousState, eState);
        }
    }
    else {
        if (pChildren) {
            uint32_t iSize = pChildren->iSize;
            for (uint32_t i = 0; i < iSize; ++i) {
                QueuedFile* pqueuedFile = pChildren->pBuffer[i].m_pObject;
                if (pqueuedFile) {
                    pIOManager->CancelTask(pqueuedFile, this);
                }
            }
        }
    }
    DecRefCount();
}

void QueuedFile::Requeue(IO_TASK_PRIORITY aeNewPriority) {
    if (pChildren) {
        uint32_t iSize = pChildren->iSize;
        for (uint32_t i = 0; i < iSize; ++i) {
            QueuedFile* m_pObject = pChildren->pBuffer[i].m_pObject;
            if (m_pObject) {
                m_pObject->Requeue(aeNewPriority);
            }
        }
    }
    pIOManager->RequeueTask(this, aeNewPriority);
}

void QueuedFile::CheckFinished() {
    if ((spParent.m_pObject || pAdditionalParents) && eState >= BS_TASK_STATE_FINISHED) {
        bool bFinished;
        if (pChildren) {
            bFinished = pChildren->iSize == pChildren->iNumChildrenFinished;
        }
        else {
            bFinished = true;
        }
        if (bFinished) {
            InterlockedIncrementInt32(&iRefCount);
            if (spParent.m_pObject) {
                spParent.m_pObject->NotifyChildFinished(this);
                NiPointer<QueuedFile>* p_spParent = &spParent;
                if (spParent.m_pObject) {
                    p_spParent->m_pObject->DecRefCount();
                    p_spParent->m_pObject = nullptr;
                    if (p_spParent->m_pObject) {
                        InterlockedIncrementInt32(&p_spParent->m_pObject->iRefCount);
                    }
                }
            }
            if (pAdditionalParents) {
                for (uint32_t i = 0; i < pAdditionalParents->iSize; ++i) {
                    pAdditionalParents->pBuffer[i].m_pObject->NotifyChildFinished(this);
                }
                if (pAdditionalParents) {
                    delete pAdditionalParents;
                }
                pAdditionalParents = nullptr;
            }
            DecRefCount();
        }
    }
}

// The parent is released by spParent's destructor and the task counted out by BSTask's.
QueuedFile::~QueuedFile() {
    delete pChildren;
    delete pAdditionalParents;
}

void QueuedFile::AddChild(QueuedFile* apChild) {
    XFNV_INLINED_LOCAL(NiPointer<QueuedFile>, spChild, std::nullopt, std::nullopt);
    if (!pChildren) {
        QueuedChildren* pNewChildren;
        void* apMemory = MemoryManager_Singleton.Allocate(sizeof(QueuedChildren));
        if (apMemory) {
            pNewChildren = ::new (apMemory) QueuedChildren;
        }
        else {
            pNewChildren = nullptr;
        }
        pChildren = pNewChildren;
    }
    spChild.m_pObject = apChild;
    if (apChild) {
        InterlockedIncrementInt32(&spChild.m_pObject->iRefCount);
    }
    pChildren->Add(spChild);
    if (spChild.m_pObject) {
        spChild.m_pObject->DecRefCount();
    }
}

void QueuedFile::AddAdditionalParent(QueuedFile* apAdditionalParent) {
    XFNV_INLINED_LOCAL(NiPointer<QueuedFile>, spAdditionalParent, std::nullopt, std::nullopt);
    if (!pAdditionalParents) {
        QueuedParents* pNewParents;
        void* apMemory = MemoryManager_Singleton.Allocate(sizeof(QueuedParents));
        if (apMemory) {
            pNewParents = ::new (apMemory) QueuedParents;
        }
        else {
            pNewParents = nullptr;
        }
        pAdditionalParents = pNewParents;
    }
    else {
        // A parent already in the list is not added again.
        QueuedParents* pParents = pAdditionalParents;
        int32_t iIndex = -1;
        for (uint32_t i = 0; i < pParents->iSize; ++i) {
            if (pParents->pBuffer && pParents->pBuffer[i].m_pObject == apAdditionalParent) {
                iIndex = i;
                break;
            }
        }
        if (iIndex != -1) {
            return;
        }
    }
    spAdditionalParent.m_pObject = apAdditionalParent;
    if (apAdditionalParent) {
        InterlockedIncrementInt32(&spAdditionalParent.m_pObject->iRefCount);
    }
    pAdditionalParents->Add(spAdditionalParent);
    if (spAdditionalParent.m_pObject) {
        spAdditionalParent.m_pObject->DecRefCount();
    }
    apAdditionalParent->AddChild(this);
}

// Room for 5 children reserved.
QueuedChildren::QueuedChildren() : BSSimpleArray(5u, 0u), iNumChildrenFinished(0) {}

// The array's items are released by BSSimpleArray's destructor.
QueuedChildren::~QueuedChildren() {}

QueuedFileEntry::QueuedFileEntry(IO_TASK_PRIORITY aePriority)
  : QueuedFile(aePriority), pFileName(nullptr), pFileEntry(nullptr) {}

QueuedFileEntry::~QueuedFileEntry() {
    MemoryManager_Singleton.Deallocate(pFileName);
}

void QueuedFileEntry::GenerateKey() {
    if (pFileEntry) {
        uint16_t a1_6 = InterlockedIncrementInt32(&IOTask::iStaticCounter);
        uint8_t a1_5 = static_cast<uint8_t>(static_cast<uint32_t>(Key) >> 16);
        uint32_t a1 = pFileEntry->iOffset & 0x7FFFFFFF;
        // The file index is sign-extended before being shifted into the key's top byte.
        int64_t v1 = static_cast<int64_t>(static_cast<int8_t>(GetFileIndex())) << 56;
        uint64_t v2 = static_cast<uint64_t>(a1) << 24;
        int32_t v3 = v2 | v1;
        int32_t v4 = static_cast<uint32_t>(v2 >> 32) | static_cast<uint32_t>(v1 >> 32);
        uint64_t v5 = static_cast<uint64_t>(a1_5) << 16;
        Key = static_cast<int64_t>(
            (static_cast<uint64_t>(Key) & 0xFFFFFFFF00000000ull) |
            (static_cast<uint64_t>(static_cast<uint32_t>((a1_6 | (v5 | v3)))))
        );
        Key = static_cast<int64_t>(
            (static_cast<uint64_t>(Key) & 0xFFFFFFFFull) |
            (static_cast<uint64_t>((static_cast<uint32_t>(v5 >> 32) | v4)) << 32)
        );
    }
    else {
        IOTask::GenerateKey();
    }
}

bool QueuedFileEntry::GetDescription(char* apDescription, size_t aiBufferSize, const char* apFileEntryTypeDescription) {
    if (pFileEntry && pFileName) {
        BSsprintf(
            apDescription, aiBufferSize, "Queued %s %s with file entry offset %i and size %i",
            apFileEntryTypeDescription, pFileName, pFileEntry->iOffset & 0x7FFFFFFF, pFileEntry->iSize & 0x3FFFFFFF
        );
    }
    else {
        if (pFileEntry) {
            BSsprintf(
                apDescription, aiBufferSize, "Queued %s with file entry offset %i and size %i",
                apFileEntryTypeDescription, pFileEntry->iOffset & 0x7FFFFFFF, pFileEntry->iSize & 0x3FFFFFFF
            );
        }
        else {
            BSsprintf(apDescription, aiBufferSize, "Queued %s %s", apFileEntryTypeDescription, pFileName);
        }
    }
    if (pChildren) {
        for (uint32_t i = 0; i < pChildren->iSize; ++i) {
            if (pChildren->pBuffer[i].m_pObject) {
                char v7[13000];
                QueuedFile* m_pObject = pChildren->pBuffer[i].m_pObject;
                if (m_pObject->GetDescription(v7, 0x32C8u)) {
                    strcat_s(apDescription, aiBufferSize, "\n * ");
                    strcat_s(apDescription, aiBufferSize, v7);
                }
            }
        }
    }
    return true;
}

void QueuedFileEntry::SetFileName(const char* apFileName) {
    if (apFileName) {
        size_t Size = std::strlen(apFileName) + 1;
        pFileName = static_cast<char*>(MemoryManager_Singleton.Allocate(Size));
        strcpy_s(pFileName, Size, apFileName);
    }
}

void QueuedFileEntry::SetFileEntry(BSFileEntry* apEntry) {
    pFileEntry = apEntry;
}

void QueuedFileEntry::GetLookupFileName(ARCHIVE_TYPE_INDEX aeArchiveTypeIndex, char* apLookupFileName) {
    if (aeArchiveTypeIndex) {
        strcpy_s(apLookupFileName, 260, pFileName);
    }
    else {
        StandardizeModelPath(pFileName, apLookupFileName, 0x104u);
    }
}

// The file of the entry: the file of its archive entry, else the loose file at its lookup name.
BSFile* QueuedFileEntry::GetFile(ARCHIVE_TYPE_INDEX aeArchiveTypeIndex, ARCHIVE_TYPE aeArchiveType) {
    ArchiveFile* pFile = nullptr;
    if (pFileEntry) {
        pFile = ArchiveManager::GetFileByFileEntry(aeArchiveTypeIndex, pFileEntry, (uint32_t)-1, nullptr);
    }
    if (pFileName && !pFile) {
        char pcLookupFileName[260];
        GetLookupFileName(aeArchiveTypeIndex, pcLookupFileName);
        pFile = static_cast<ArchiveFile*>(FileFinder::GetFile(pcLookupFileName, NiFile::READ_ONLY, -1, aeArchiveType));
    }
    return pFile;
}

void QueuedFileEntry::LookupFileEntry(ARCHIVE_TYPE_INDEX aeArchiveTypeIndex) {
    BSHash directoryHash;
    BSHash fileHash;
    if (pFileName) {
        char apFileName[264];
        GetLookupFileName(aeArchiveTypeIndex, apFileName);
        BSHash::MakeDirAndFile(apFileName, directoryHash, fileHash);
        BSFileEntry* pfileEntry =
            ArchiveManager::GetFileEntryForFileFromAllArchives(aeArchiveTypeIndex, directoryHash, fileHash, apFileName);
        SetFileEntry(pfileEntry);
    }
}

// Room for 5 parents reserved.
QueuedParents::QueuedParents() : BSSimpleArray(5u, 0u) {}

// The array's items are released by BSSimpleArray's destructor.
QueuedParents::~QueuedParents() {}

void QueuedParents::RemoveFastReleasing(int32_t a2, int32_t a3) {
    uint32_t v3;
    if (iSize - a2 - a3 >= a3) {
        v3 = a3;
    }
    else {
        v3 = iSize - a2 - a3;
    }
    NiPointer<QueuedFile>* v5 = &pBuffer[a2];
    for (uint32_t i = 0; i < a3; ++i) {
        if (v5[i].m_pObject) {
            v5[i].m_pObject->DecRefCount();
        }
    }
    _MoveItems(&pBuffer[a2], &pBuffer[iSize] - v3, v3);
    iSize -= a3;
}

// Frees through the engine's heap.
void QueuedParents::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}
