#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"

#include "BSCore/BSSpinLock.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSArchive.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSEntryMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

#include <cctype>

BSSpinLock BSTexturePalette::AddCrit;

// Releases the texture arTexture held, then searches the seven groups from the highest priority (group 6) down for the
// one that has the texture of apPath. On the last lock release level, the owning thread is cleared.
bool BSTexturePalette::GetTexture(const char* apPath, NiPointer<NiTexture>& arTexture) {
    if (!BSTexturePalette::pEntryMap) {
        return false;
    }
    BSTexturePalette::AddCrit.Lock(nullptr);
    const char* pTexturePath = &apPath[FindTexturePathOffset(apPath, true)];
    uint32_t uiContainer = BSTexturePalette::GetContainerNumber(pTexturePath);
    arTexture = nullptr;
    bool bResult = false;
    for (int32_t iGroup = 6; iGroup >= 0; --iGroup) {
        bResult = BSTexturePalette::pTexMap[iGroup].GetTexture(uiContainer, pTexturePath, &arTexture);
        if (bResult) {
            break;
        }
    }
    if (--BSTexturePalette::AddCrit.uiLockCount == 0) {
        BSTexturePalette::AddCrit.uiOwningThread = 0;
    }
    return bResult;
}

uint32_t BSTexturePalette::GetContainerNumber(const char* apPath) {
    uint32_t i;
    int32_t v5;
    const char* v1 = apPath;
    char v2 = *apPath;
    for (i = 0; *v1; i += v5) {
        int32_t v4 = std::tolower(v2);
        ++v1;
        if (v4 == 46 && !v1[3]) {
            break;
        }
        v5 = v4 + 32 * i;
        v2 = *v1;
    }
    return i % 713;
}

BSTexMapItem* BSTexturePalette::GetItemFor(const NiTexture* apTexture) {
    BSTexMapItem* pitem;
    const char* pname = (&apTexture->GetFilename())->GetHandle();
    uint32_t ioffset = FindTexturePathOffset(pname, true);
    uint32_t ContainerNumber = BSTexturePalette::GetContainerNumber(&pname[ioffset]);
    uint32_t I = 0;
    // The same bucket of each group's table, indexed by table: sizeof(BSTexMap) is pointer-dependent.
    do {
        pitem = BSTexturePalette::pTexMap[I].pItemA[ContainerNumber];
        if (pitem) {
            while (apTexture != pitem->spTexture.m_pObject) {
                pitem = pitem->pNext;
                if (!pitem) {
                    goto LABEL_7;
                }
            }
            if (pitem) {
                break;
            }
        }
    LABEL_7:;
        ++I;
    }
    while (I < 7);
    return pitem;
}

// Points arTexture at the loaded texture of the archive entry apEntry and returns the map's own result.
bool BSTexturePalette::GetTexture(const BSFileEntry* apEntry, NiPointer<NiTexture>& arTexture) {
    arTexture = nullptr;
    return BSTexturePalette::pEntryMap->GetAt(apEntry, arTexture);
}

void BSTexturePalette::SetTexture(NiTexture* apTex, const BSFileEntry* apEntry) {
    uint32_t v2;
    uint16_t v3;
    BSTexMapItem* v4;
    uint8_t v5;
    const BSFileEntry* entryForTexture;
    if (BSTexturePalette::pEntryMap) {
        (&BSTexturePalette::AddCrit)->Lock(nullptr);
        v2 = BSTexturePalette::DeterminePriority(apTex);
        v3 = (uint16_t)v2;
        v4 = (&BSTexturePalette::pTexMap[v2])->SetAt(apTex);
        v5 = (uint8_t)(v4->pFileEntry == nullptr);
        v4->sPriority = v3;
        if (v5) {
            entryForTexture = apEntry;
            if (apEntry || ((entryForTexture = BSTexturePalette::FindEntryForTexture(apTex)) != nullptr)) {
                BSTexturePalette::pEntryMap->SetAt(entryForTexture, v4);
            }
        }
        if (!(--BSTexturePalette::AddCrit.uiLockCount)) {
            BSTexturePalette::AddCrit.uiOwningThread = (uint32_t)0;
        }
    }
}

// Drops apTexture from the palette: its archive entry from the entry map, then its item from the group its priority
// names. Releasing the lock at the last level clears the owning thread.
bool BSTexturePalette::RemoveTexture(const NiTexture* apTexture) {
    if (!BSTexturePalette::pEntryMap) {
        return false;
    }
    BSTexturePalette::AddCrit.Lock(nullptr);
    BSTexMapItem* pItem = BSTexturePalette::GetItemFor(apTexture);
    bool bResult = false;
    if (pItem) {
        if (pItem->pFileEntry) {
            BSTexturePalette::pEntryMap->Remove(pItem->pFileEntry);
        }
        BSTexturePalette::pTexMap[pItem->sPriority].Remove(pItem);
        bResult = true;
    }
    if (--BSTexturePalette::AddCrit.uiLockCount == 0) {
        BSTexturePalette::AddCrit.uiOwningThread = 0;
    }
    return bResult;
}

void BSTexturePalette::PurgeUnusedTextures() {
    if (BSTexturePalette::pEntryMap) {
        BSTexMapIterator iterator;
        BSTexturePalette::AddCrit.Lock(nullptr);
        int32_t iPurged = 0;
        BSTexturePalette::bPartiallyPurgedS = false;
        if (BSTexturePalette::GetFirstPos(iterator)) {
            bool bFound;
            NiPointer<NiTexture> spTexture;
            BSTexMap* pTexMaps = BSTexturePalette::pTexMap;
            uint32_t iPriority = iterator.iPriority;
            BSTexMapItem* pItem = iterator.pItem;
            do {
                BSTexMapItem* pcurItem = pItem;
                uint32_t iCurPriority = iPriority;
                bool bNext = pTexMaps[iPriority].GetNext(iterator, spTexture);
                pTexMaps = BSTexturePalette::pTexMap;
                iPriority = iterator.iPriority;
                bFound = bNext;
                if (bNext) {
                    pItem = iterator.pItem;
                }
                else {
                    if (iterator.iPriority) {
                        while (true) {
                            BSTexMap* pGroup = &BSTexturePalette::pTexMap[--iPriority];
                            pItem = nullptr;
                            iterator.iIndex = 0;
                            iterator.pItem = nullptr;
                            if (pGroup->iCount) {
                                uint32_t i = 0;
                                while (!pGroup->pItemA[i]) {
                                    if (++i >= 713) {
                                        goto LABEL_12;
                                    }
                                }
                                pItem = pGroup->pItemA[i];
                                iPriority = pItem->sPriority;
                                iterator.iIndex = i;
                                iterator.pItem = pItem;
                            }
                        LABEL_12:;
                            bFound = pItem != nullptr;
                            if (pItem) {
                                break;
                            }
                            if (!iPriority) {
                                iterator.iPriority = 0;
                                goto LABEL_15;
                            }
                        }
                    }
                    else {
                        pItem = iterator.pItem;
                    LABEL_15:;
                        iPriority = -1;
                    }
                    iterator.iPriority = iPriority;
                }
                if ((spTexture.m_pObject) && (spTexture.m_pObject->m_uiRefCount == 2)) {
                    if (pcurItem->pFileEntry) {
                        BSTexturePalette::pEntryMap->Remove(pcurItem->pFileEntry);
                        pTexMaps = BSTexturePalette::pTexMap;
                    }
                    pTexMaps[iCurPriority].Remove(pcurItem);
                    ++iPurged;
                    pTexMaps = BSTexturePalette::pTexMap;
                }
            }
            while (bFound && iPurged < BSTexturePalette::iMaxTexturesToPurgeS);
        }
        if (!--BSTexturePalette::AddCrit.uiLockCount) {
            BSTexturePalette::AddCrit.uiOwningThread = 0;
        }
    }
}

void BSTexturePalette::Lock() {
    (&BSTexturePalette::AddCrit)->Lock(nullptr);
}

void BSTexturePalette::Unlock() {
    (&BSTexturePalette::AddCrit)->Unlock();
}

void BSTexturePalette::SetCheckPurgedList(bool abCheck) {
    bCheckPurgedTextureList = (bool)((int8_t)abCheck);
}

void BSTexturePalette::SetMaxTexturesToPurge(int32_t aNumTextures) {
    BSTexturePalette::iMaxTexturesToPurgeS = aNumTextures;
}

int32_t BSTexturePalette::GetMaxTexturesToPurge() {
    return BSTexturePalette::iMaxTexturesToPurgeS;
}

bool BSTexturePalette::TryLock() {
    return (bool)((uint8_t)(&BSTexturePalette::AddCrit)->TryLock());
}

bool BSTexturePalette::IsPartiallyPurged() {
    return (bool)((uint8_t)BSTexturePalette::bPartiallyPurgedS);
}

bool BSTexturePalette::RemoveTexture_0(const char* a1) {
    bool bres = false;
    if (!BSTexturePalette::pEntryMap) {
        return false;
    }
    NiPointer<NiTexture> sptex;
    BSTexturePalette::AddCrit.Lock(nullptr);
    if (BSTexturePalette::GetTexture(a1, sptex)) {
        bres = BSTexturePalette::RemoveTexture(sptex);
    }
    if (!--BSTexturePalette::AddCrit.uiLockCount) {
        BSTexturePalette::AddCrit.uiOwningThread = 0;
    }
    return bres;
}

void BSTexturePalette::RemoveAll() {
    if (BSTexturePalette::pEntryMap) {
        BSTexturePalette::AddCrit.Lock(nullptr);
        BSTexturePalette::pEntryMap->RemoveAll();
        for (uint32_t i = 0; i < 7; ++i) {
            BSTexturePalette::pTexMap[i].FreeTextures();
        }
        if (!--BSTexturePalette::AddCrit.uiLockCount) {
            BSTexturePalette::AddCrit.uiOwningThread = 0;
        }
    }
}

uint32_t BSTexturePalette::DeterminePriority(NiTexture* apTex) {
    uint32_t v1 = apTex->GetHeight();
    uint32_t v2 = apTex->GetWidth();
    if (v2 <= v1) {
        v2 = v1;
    }
    if (v2 >= 1024) {
        return (uint32_t)4;
    }
    if (v2 >= 512) {
        return (uint32_t)3;
    }
    if (v2 < 256) {
        return (uint32_t)((uint8_t)(v2 >= 128));
    }
    return (uint32_t)2;
}

bool BSTexturePalette::ArePathsEqual(const char* apRequestedPath, const char* apPath) {
    int32_t v5;
    int32_t v6;
    const char* v2 = apRequestedPath;
    char v3 = *apRequestedPath;
    if (!*apRequestedPath) {
        return true;
    }
    while (true) {
        v5 = std::tolower(v3);
        ++v2;
        v6 = std::tolower(*apPath++);
        if (v5 != 46) {
            break;
        }
        if (v6 != 46) {
            return false;
        }
        if (!v2[3]) {
            return true;
        }
    LABEL_7:;
        v3 = *v2;
        if (!*v2) {
            return true;
        }
    }
    if (v5 == v6) {
        goto LABEL_7;
    }
    return false;
}

void BSTexturePalette::Init() {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_GAMEBRIO;
    BSEntryMap* pMap = static_cast<BSEntryMap*>(MemoryManager_Singleton.Allocate(sizeof(BSEntryMap)));
    if (pMap) {
        std::memset(pMap->pItemA, 0, sizeof(pMap->pItemA));
        pMap->iCount = 0;
        BSTexturePalette::pEntryMap = pMap;
    }
    else {
        BSTexturePalette::pEntryMap = nullptr;
    }
    // Equivalent of new BSTexMap[7]: the element count, then the seven groups built by BSTexMap's constructor.
    PathMap* pPathMap = static_cast<PathMap*>(MemoryManager_Singleton.Allocate(sizeof(PathMap)));
    if (pPathMap) {
        pPathMap->uiSize = 7;
        rt::eh_vector_constructor_iterator(
            pPathMap->kGroups, sizeof(BSTexMap), 7, &xfnv::callMember<&BSTexMap::Create>
        );
        BSTexturePalette::pTexMap = pPathMap->kGroups;
        pTLSData->eMemContext = eMemContext;
    }
    else {
        pTLSData->eMemContext = eMemContext;
        BSTexturePalette::pTexMap = nullptr;
    }
    BSTexturePalette::bPartiallyPurgedS = false;
}

void BSTexturePalette::Kill() {
    BSTexturePalette::RemoveAll();
    BSEntryMap* v0 = BSTexturePalette::pEntryMap;
    if (BSTexturePalette::pEntryMap) {
        BSTexturePalette::pEntryMap->RemoveAll();
        MemoryManager_Singleton.Deallocate(v0);
    }
    BSTexturePalette::pEntryMap = nullptr;
    if (BSTexturePalette::pTexMap) {
        uint32_t* p_iCount = &BSTexturePalette::pTexMap[-1].iCount;
        BSTexMap* v2 = &BSTexturePalette::pTexMap[BSTexturePalette::pTexMap[-1].iCount];
        for (int32_t i = BSTexturePalette::pTexMap[-1].iCount - 1; i >= 0; --i) {
            (--v2)->FreeTextures();
        }
        MemoryManager_Singleton.Deallocate(p_iCount);
    }
    BSTexturePalette::pTexMap = nullptr;
}

bool BSTexturePalette::GetFirstPos(BSTexMapIterator& arIterator) {
    bool bres;
    uint32_t I = 7;
    do {
        BSTexMap* v2 = &BSTexturePalette::pTexMap[--I];
        arIterator.iIndex = 0;
        arIterator.pItem = nullptr;
        if (v2->iCount) {
            uint32_t v3 = 0;
            while (!v2->pItemA[v3]) {
                if (++v3 >= 713) {
                    goto LABEL_8;
                }
            }
            arIterator.iIndex = v3;
            BSTexMapItem* v4 = v2->pItemA[v3];
            arIterator.pItem = v4;
            arIterator.iPriority = v4->sPriority;
        }
    LABEL_8:;
        bres = arIterator.pItem != nullptr;
    }
    while (!arIterator.pItem && I * 2856);
    return bres;
}

BSTexMapItem* BSTexMap::SetAt(NiTexture* apValue) {
    BSTexMapItem* result;
    const char* m_kHandle = (&apValue->GetFilename())->GetHandle();
    uint32_t TexturePathOffset = FindTexturePathOffset(m_kHandle, true);
    const char* v6 = &m_kHandle[TexturePathOffset];
    int16_t usOffset = TexturePathOffset;
    uint32_t ContainerNumber = BSTexturePalette::GetContainerNumber(v6);
    BSTexMapItem* v8 = pItemA[ContainerNumber];
    if (v8) {
        while (true) {
            const NiFixedString* v9 = &v8->spTexture.m_pObject->GetFilename();
            if (BSTexturePalette::ArePathsEqual(v6, &v9->m_kHandle[v8->sNameBaseOffset])) {
                return v8;
            }
            v8 = v8->pNext;
            if (!v8) {
                goto LABEL_4;
            }
        }
    }
    else {
    LABEL_4:;
        BSTexMapItem* v10 = static_cast<BSTexMapItem*>(MemoryManager_Singleton.Allocate(sizeof(BSTexMapItem)));
        if (v10) {
            BSTexMapItem* v11 = pItemA[ContainerNumber];
            v10->spTexture.m_pObject = apValue;
            NiIncRefCount(apValue);
            v10->pFileEntry = nullptr;
            v10->pNext = v11;
            v10->sPriority = 0;
            v10->sNameBaseOffset = 0;
            result = v10;
        }
        else {
            result = nullptr;
        }
        result->sNameBaseOffset = usOffset;
        pItemA[ContainerNumber] = result;
        ++iCount;
    }
    return result;
}

bool BSTexMap::GetTexture(uint32_t auiContainer, const char* apPath, NiPointer<NiTexture>* aspTexture) {
    BSTexMapItem* v4 = pItemA[auiContainer];
    if (!v4) {
        return false;
    }
    while (true) {
        const NiFixedString* v5 = &v4->spTexture.m_pObject->GetFilename();
        if (BSTexturePalette::ArePathsEqual(apPath, &v5->m_kHandle[v4->sNameBaseOffset])) {
            break;
        }
        v4 = v4->pNext;
        if (!v4) {
            return false;
        }
    }
    *aspTexture = v4->spTexture;
    return true;
}

void BSTexMap::Remove(BSTexMapItem* apValue) {
    const char* m_kHandle = (&apValue->spTexture.m_pObject->GetFilename())->GetHandle();
    uint32_t TexturePathOffset = FindTexturePathOffset(m_kHandle, true);
    uint32_t ContainerNumber = BSTexturePalette::GetContainerNumber(&m_kHandle[TexturePathOffset]);
    BSTexMapItem* v6 = pItemA[ContainerNumber];
    if (v6) {
        BSTexMapItem* v7 = nullptr;
        while (v6 != apValue) {
            v7 = v6;
            v6 = v6->pNext;
            if (!v6) {
                return;
            }
        }
        if (v7) {
            v7->pNext = v6->pNext;
        }
        else {
            pItemA[ContainerNumber] = v6->pNext;
        }
        NiTexture* m_pObject = v6->spTexture.m_pObject;
        if (v6->spTexture.m_pObject) {
            NiDecRefCount(m_pObject);
        }
        MemoryManager_Singleton.Deallocate(v6);
        --iCount;
    }
}

void BSTexMap::FreeTextures() {
    BSTexMap* pThis = this;
    BSTexMap* v5 = pThis;
    for (uint32_t i = 0; i < 713; ++i) {
        BSTexMapItem* v2 = pThis->pItemA[i];
        if (v2) {
            do {
                BSTexMapItem* v3 = v2;
                NiTexture* m_pObject = v2->spTexture.m_pObject;
                v2 = v2->pNext;
                if (m_pObject) {
                    NiDecRefCount(m_pObject);
                }
                MemoryManager_Singleton.Deallocate(v3);
            }
            while (v2);
            pThis = v5;
        }
        pThis->pItemA[i] = nullptr;
    }
    pThis->iCount = 0;
}

void* BSTexMap::Create() {
    std::memset(pItemA, 0, sizeof(pItemA));
    iCount = (uint32_t)0;
    return this;
}

bool BSTexMap::GetNext(BSTexMapIterator& arIterator, NiPointer<NiTexture>& arValue) {
    BSTexMapItem* pItem = arIterator.pItem;
    if (arIterator.pItem) {
        NiTexture* m_pObject = arValue.m_pObject;
        if (arValue.m_pObject != pItem->spTexture.m_pObject) {
            if (m_pObject) {
                NiDecRefCount(m_pObject);
            }
            NiTexture* v5 = pItem->spTexture.m_pObject;
            arValue.m_pObject = pItem->spTexture.m_pObject;
            if (v5) {
                NiIncRefCount(v5);
            }
        }
        BSTexMapItem* pNext = arIterator.pItem->pNext;
        arIterator.pItem = pNext;
        if (!pNext && ++arIterator.iIndex < 713) {
            uint32_t iIndex;
            while (true) {
                iIndex = arIterator.iIndex;
                if (pItemA[iIndex]) {
                    break;
                }
                if (++arIterator.iIndex >= 713) {
                    return arIterator.pItem != nullptr;
                }
            }
            uint32_t v9 = arIterator.iIndex;
            arIterator.pItem = pItemA[iIndex];
        }
    }
    return arIterator.pItem != nullptr;
}

// Reads the entry of apFileEntry at bucket aIndex into arTexture: release the old object, then hold the new one.
bool BSEntryMap::GetAt(uint32_t aIndex, const BSFileEntry* apFileEntry, NiPointer<NiTexture>& arTexture) {
    BSEntryMapItem* pItem = pItemA[aIndex];
    if (!pItem) {
        return false;
    }
    while (apFileEntry != pItem->pMapItem->pFileEntry) {
        pItem = pItem->pNext;
        if (!pItem) {
            return false;
        }
    }
    arTexture = pItem->pMapItem->spTexture;
    return true;
}

// Computes the hash ((key >> 4) % 1001) directly instead of calling KeyToHashIndex. Unlinks the entry's item, frees it,
// clears the entry it pointed at and returns that texture item.
BSTexMapItem* BSEntryMap::Remove(const BSFileEntry* apEntry) {
    // The whole pointer, as SetAt and KeyToHashIndex hash it: a 32-bit truncation picks another bucket on x64.
    uint32_t uiIndex = static_cast<uint32_t>((reinterpret_cast<uintptr_t>(apEntry) >> 4) % 1001);
    BSEntryMapItem* pItem = pItemA[uiIndex];
    if (!pItem) {
        return nullptr;
    }
    BSEntryMapItem* pPrev = nullptr;
    while (pItem->pMapItem->pFileEntry != apEntry) {
        pPrev = pItem;
        pItem = pItem->pNext;
        if (!pItem) {
            return nullptr;
        }
    }
    BSEntryMapItem* pNext = pItem->pNext;
    if (pPrev) {
        pPrev->pNext = pNext;
    }
    else {
        pItemA[uiIndex] = pNext;
    }
    BSTexMapItem* pMapItem = pItem->pMapItem;
    --iCount;
    MemoryManager_Singleton.Deallocate(pItem);
    pMapItem->pFileEntry = nullptr;
    return pMapItem;
}

// Reads each item's texture item first, then the chain's next pointer, and clears that texture item's archive entry
// before freeing the bucket item.
void BSEntryMap::RemoveAll() {
    for (uint32_t i = 0; i < 1001; ++i) {
        BSEntryMapItem* pItem = pItemA[i];
        while (pItem) {
            BSEntryMapItem* pNext = pItem->pNext;
            pItem->pMapItem->pFileEntry = nullptr;
            MemoryManager_Singleton.Deallocate(pItem);
            pItem = pNext;
        }
        pItemA[i] = nullptr;
    }
    iCount = 0;
}

void BSEntryMap::SetAt(const BSFileEntry* apEntry, BSTexMapItem* apValue) {
    // The entry is taken const but kept in a non-const field, so the const is cast away.
    apValue->pFileEntry = const_cast<BSFileEntry*>(apEntry);
    // EXE-INLINED 0x43C530
    // The hash key is the entry's address itself, taken as an integer.
    uint32_t iindex = static_cast<uint32_t>((reinterpret_cast<uintptr_t>(apEntry) >> 4) % 1001);
    BSEntryMap::BSEntryMapItem* pNewItem =
        static_cast<BSEntryMap::BSEntryMapItem*>(MemoryManager_Singleton.Allocate(sizeof(BSEntryMap::BSEntryMapItem)));
    if (pNewItem) {
        BSEntryMap::BSEntryMapItem* pHead = pItemA[iindex];
        pNewItem->pMapItem = apValue;
        pNewItem->pNext = pHead;
        pItemA[iindex] = pNewItem;
    }
    else {
        pItemA[iindex] = nullptr;
    }
    ++iCount;
}
