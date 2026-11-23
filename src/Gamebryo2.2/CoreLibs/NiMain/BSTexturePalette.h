#pragma once

#include "BSCore/BSSpinLock.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/ehvec.h"

class BSEntryMap;
class BSFileEntry;
class BSTexMap;
class NiTexture;

// One loaded texture of the palette: the texture, its archive entry and the next item of the hash chain.
struct BSTexMapItem {
    NiPointer<NiTexture> spTexture;
    BSFileEntry* pFileEntry;
    BSTexMapItem* pNext;
    uint16_t sPriority;
    int16_t sNameBaseOffset;
};

// A walk over the palette's hash table: the item reached, its bucket index and its priority.
// BSTexturePalette::GetFirstPos and BSTexMap::GetNext fill it, and PurgeUnusedTextures reads its three members.
struct BSTexMapIterator {
    BSTexMapItem* pItem;
    uint32_t iIndex;
    uint32_t iPriority;
};

// Bethesda's global table of loaded textures; all of it is static.
class BSTexturePalette {
public:
    // Seven texture groups and their count (defined after BSTexMap).
    struct PathMap;

    // Points arTexture at the loaded texture of apPath (released first); false when there is none.
    static bool GetTexture(const char* apPath, NiPointer<NiTexture>& arTexture);
    // The texture group number of the texture path apPath.
    static uint32_t GetContainerNumber(const char* apPath);
    // The palette item that holds apTexture, or null.
    static BSTexMapItem* GetItemFor(const NiTexture* apTexture);
    // Points arTexture at the loaded texture of the archive entry apEntry.
    static bool GetTexture(const BSFileEntry* apEntry, NiPointer<NiTexture>& arTexture);
    // Adds apTex to the palette, with its archive entry (looked up when null).
    static void SetTexture(NiTexture* apTex, const BSFileEntry* apEntry);
    // Drops apTexture from the palette; true when it was in it.
    static bool RemoveTexture(const NiTexture* apTexture);
    // Releases the textures nothing references any more.
    static void PurgeUnusedTextures();
    // The archive entry of apTex in the palette.
    static BSFileEntry* FindEntryForTexture(const NiTexture* apTex);
    static void Lock();
    static void Unlock();
    static void SetCheckPurgedList(bool abCheck);
    static void SetMaxTexturesToPurge(int32_t aNumTextures);
    static int32_t GetMaxTexturesToPurge();
    static bool TryLock();
    static bool IsPartiallyPurged();
    static bool RemoveTexture_0(const char* apName);
    static void RemoveAll();

protected:
    static uint32_t DeterminePriority(NiTexture* apTex);

public:
    static bool ArePathsEqual(const char* apRequestedPath, const char* apPath);
    static void Init();
    static void Kill();
    static bool GetFirstPos(BSTexMapIterator& arIterator);

    // The lock around adding textures to the palette.
    static BSSpinLock AddCrit;
    static inline int32_t iMaxTexturesToPurgeS = 25;
    static inline bool bPartiallyPurgedS;
    // The seven texture groups the palette keeps (the array BSTexturePalette::Init allocates).
    static inline BSTexMap* pTexMap;
    // The archive entries of the textures the palette keeps; null until Init.
    static inline BSEntryMap* pEntryMap;
};

// The palette's hash table: 713 chains of loaded textures and their count.
class BSTexMap {
public:
    // The item holding apTexture's path, added to the chain when it has none.
    BSTexMapItem* SetAt(NiTexture* apValue);
    // Reads the texture of auiContainer's group at apPath into aspTexture; true when found.
    bool GetTexture(uint32_t auiContainer, const char* apPath, NiPointer<NiTexture>* aspTexture);
    // Drops apValue from the chain of its bucket.
    void Remove(BSTexMapItem* apValue);
    // Releases every texture of the table.
    void FreeTextures();
    void* Create();
    bool GetNext(BSTexMapIterator& arIterator, NiPointer<NiTexture>& arValue);

    BSTexMapItem* pItemA[713];
    uint32_t iCount;
};

// Layout of the new BSTexMap[7] block: Init stores the count 7 in uiSize and builds the groups after it, and Kill reads
// the count just before the groups.
struct BSTexturePalette::PathMap {
    uint32_t uiSize;
    BSTexMap kGroups[7];
};

// The palette's purged-list flag; a free global, not a member.
inline bool bCheckPurgedTextureList;
