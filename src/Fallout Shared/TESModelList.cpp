#include "Fallout Shared/TESModelList.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSSystem/BSHash.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "SDK/MSVC/rtti.h"

void TESTextureListArray::ClearData() {
    TESTextureList* v2;
    uint32_t i;
    if (pTextureLists) {
        for (i = 0; i < ((uint32_t)((int32_t)iTextureListCount)); ++i) {
            v2 = pTextureLists[i];
            if (v2) {
                delete v2;
            }
        }
        ourFree(pTextureLists);
        pTextureLists = (TESTextureList**)nullptr;
    }
    iTextureListCount = (uint32_t)0;
}

// Reads the texture list array: the list count, then for each list its count and its 24-byte texture hash records. When
// the chunk is shorter than the four bytes of the count (malformed data), those bytes are initialised to 0 (allocated
// even for an empty chunk).
void TESTextureListArray::Load(TESModelList* apModelList, TESFile* apFile) {
    ClearData();
    const uint32_t uiSize = apFile->GetChunkSize();
    // A 16-byte aligned stack buffer of the chunk's size.
    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiSize < 4 ? 4 : uiSize, 128));
    // The game does not zero these bytes, so a chunk shorter than the count leaves them unwritten (see above).
    pBuffer[0] = 0;
    pBuffer[1] = 0;
    pBuffer[2] = 0;
    pBuffer[3] = 0;
    apFile->GetChunkData(pBuffer, uiSize);
    ourMemcpy(this, pBuffer, 4);
    if (apFile->QEndian()) {
        ENDIAN(iTextureListCount, false);
    }
    uint32_t uiOffset = 4;
    // The size is 0xFFFFFFFF when 4 * iTextureListCount overflows 32 bits, as the game does.
    pTextureLists = static_cast<TESTextureList**>(ourMalloc(
        (iTextureListCount >> 30) != 0 ? 0xFFFFFFFFu
                                       : static_cast<uint32_t>(sizeof(TESTextureList*) * iTextureListCount)
    ));
    BSSimpleList<const char*>* pNode = apModelList->GetNIFFileList();
    for (uint32_t i = 0; i < iTextureListCount; ++i) {
        const char* pModelName = nullptr;
        if (pNode) {
            pModelName = pNode->GetItem();
        }
        pNode = pNode->GetNext();
        void* pListMemory = ourMalloc(sizeof(TESTextureList));
        TESTextureList* pList = nullptr;
        if (pListMemory) {
            pList = ::new (pListMemory) TESTextureList;
        }
        pTextureLists[i] = pList;
        const uint8_t cTextureCount = static_cast<uint8_t>(pBuffer[uiOffset]);
        ++uiOffset;
        TESForm* pParentForm = dynamic_cast<TESForm*>(apModelList);
        // Walks the rest of the chunk buffer as raw bytes.
        pTextureLists[i]->LoadFromBuffer(
            reinterpret_cast<BSHashData*>(pBuffer + uiOffset), cTextureCount, pParentForm, pModelName
        );
        uiOffset += sizeof(BSHashData) * cTextureCount;
    }
}

TESTextureList* TESTextureListArray::GetAt(uint32_t aiIndex) {
    if (aiIndex >= iTextureListCount) {
        return nullptr;
    }

    return pTextureLists[aiIndex];
}

void TESTextureListArray::Copy(TESTextureListArray* apTextureListArray) {
    ClearData();
    if (apTextureListArray->iTextureListCount) {
        iTextureListCount = apTextureListArray->iTextureListCount;
        // new TESTextureList*[n]: SIZE_MAX when the size overflows.
        pTextureLists = static_cast<TESTextureList**>(ourMalloc(
            iTextureListCount <= SIZE_MAX / sizeof(TESTextureList*) ? sizeof(TESTextureList*) * iTextureListCount
                                                                    : SIZE_MAX
        ));
        for (uint32_t i = 0; i < iTextureListCount; ++i) {
            TESTextureList* v2;
            TESTextureList* v4 = static_cast<TESTextureList*>(ourMalloc(sizeof(TESTextureList)));
            if (v4) {
                v2 = ::new (v4) TESTextureList;
            }
            else {
                v2 = nullptr;
            }
            pTextureLists[i] = v2;
            pTextureLists[i]->Copy(apTextureListArray->pTextureLists[i]);
        }
    }
}

// Value-initialising TextureListArray leaves it as an empty list.
TESModelList::TESModelList() : BaseFormComponent(), nifFilenames(), TextureListArray() {}

TESModelList::~TESModelList() {
    TESModelList::ClearDataComponent();
}

// Frees every model file name, then the texture lists.
void TESModelList::ClearDataComponent() {
    for (;;) {
        bool bEmpty;
        bEmpty = nifFilenames.IsEmpty();
        if (bEmpty) {
            break;
        }
        const char* pFilename;
        pFilename = nifFilenames.GetItem();
        nifFilenames.RemoveHead();
        ourFree(const_cast<char*>(pFilename));
    }
    TextureListArray.ClearData();
}

void TESModelList::CopyComponent(BaseFormComponent* apCopyFC) {
    TESModelList* pcopy = dynamic_cast<TESModelList*>(apCopyFC);
    if (pcopy) {
        ClearDataComponent();
        for (const char* pnifFilename : pcopy->nifFilenames) {
            if (pnifFilename) {
                AddModel(pnifFilename);
            }
        }
        TESTextureListArray* v2 = &pcopy->TextureListArray;
        TextureListArray.Copy(v2);
    }
}

bool TESModelList::CompareComponent(BaseFormComponent* apCompareFC) {
    BSSimpleList<const char*>* pnifFileListCompare;
    TESModelList* pcompare = dynamic_cast<TESModelList*>(apCompareFC);
    if (!pcompare) {
        return true;
    }
    BSSimpleList<const char*>* pnifFileList = &nifFilenames;
    for (pnifFileListCompare = &pcompare->nifFilenames; pnifFileList && pnifFileListCompare;
         pnifFileListCompare = pnifFileListCompare->m_pkNext) {
        const char* pnifFilename = pnifFileList->m_item;
        const char* pnifFilenameCompare = pnifFileListCompare->m_item;
        if ((!pnifFilename && pnifFilenameCompare) || (pnifFilename && !pnifFilenameCompare)) {
            return true;
        }
        if (pnifFilename && (pnifFilenameCompare && BSstricmp(pnifFilename, pnifFilenameCompare))) {
            return true;
        }
        pnifFileList = pnifFileList->m_pkNext;
    }
    return pnifFileList || pnifFileListCompare;
}

void TESModelList::AddModel(const char* apFilename) {
    if (!IsNIFFileInList(apFilename)) {
        int32_t iFilenameLen = BSstrlen(apFilename) + 1;
        char* pnifFilename = static_cast<char*>(ourMalloc(iFilenameLen));
        BSstrcpy(pnifFilename, iFilenameLen, apFilename);
        nifFilenames.AddTail(pnifFilename);
    }
}

void TESModelList::LoadModelListChunk(TESModelList* apModelList, TESFile* apFile) {
    if ((apFile) && (apModelList)) {
        CHUNK_ID chunk = apFile->GetTESChunk();
        if (chunk == NIFT_ID) {
            TextureListArray.Load(apModelList, apFile);
        }
        else {
            if (chunk == NIFZ_ID) {
                uint32_t ChunkSize = apFile->GetChunkSize();
                char* pmessage = static_cast<char*>(ourMalloc(ChunkSize));
                char* pcurrentFile = pmessage;
                apFile->GetChunkData(pmessage, 0);
                while (*pcurrentFile) {
                    AddModel(pcurrentFile);
                    pcurrentFile += BSstrlen(pcurrentFile) + 1;
                }
                ourFree(pmessage);
            }
        }
    }
}

void TESModelList::Save(CHUNK_ID aeModelListID, CHUNK_ID aeTextureListID) {
    if (!nifFilenames.IsEmpty()) {
        int32_t idataSize = 1;
        for (const char* pnifFilename : nifFilenames) {
            if (pnifFilename) {
                idataSize += BSstrlen(pnifFilename) + 1;
            }
        }
        char* pnifFilenames = static_cast<char*>(ourMalloc(idataSize));
        char* pcurrentFile = pnifFilenames;
        BSSimpleList<const char*>* pnifFileList_2 = &nifFilenames;
        uint32_t ilocalsize = idataSize;
        while (pnifFileList_2) {
            const char* pnifFilename_2 = pnifFileList_2->m_item;
            if (pnifFilename_2) {
                BSstrcpy(pcurrentFile, ilocalsize, pnifFilename_2);
                uint32_t isize = BSstrlen(pnifFilename_2) + 1;
                pcurrentFile += isize;
                ilocalsize -= isize;
            }
            pnifFileList_2 = pnifFileList_2->m_pkNext;
        }
        *pcurrentFile = 0;
        TESForm::AddChunkArray(aeModelListID, pnifFilenames, idataSize);
        ourFree(pnifFilenames);
    }
}

bool TESModelList::IsNIFFileInList(const char* apFilename) {
    if (nifFilenames.IsEmpty() || !apFilename) {
        return false;
    }
    for (const char* pnifFilename : nifFilenames) {
        if (pnifFilename && !BSstricmp(pnifFilename, apFilename)) {
            return true;
        }
    }
    return false;
}
