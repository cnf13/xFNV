#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class BSFileEntry;
struct BSTexMapItem;
class NiTexture;

// The hash table of BSTexturePalette: the texture entries by key, chained per bucket. The table has 1001 buckets (the
// static iTableSize).
class BSEntryMap {
public:
    struct BSEntryMapItem {
        BSTexMapItem* pMapItem;
        // SetAt stores the bucket's head item in it.
        BSEntryMapItem* pNext;
    };

    // Reads the entry of apFileEntry at bucket aIndex into arTexture; true when the entry is in the table.
    bool GetAt(uint32_t aIndex, const BSFileEntry* apFileEntry, NiPointer<NiTexture>& arTexture);
    // Reads apEntry's entry from its hash slot into arTexture; true when the entry is in the table.
    bool GetAt(const BSFileEntry* apEntry, NiPointer<NiTexture>& arTexture);
    // Removes apEntry's item from its bucket chain, frees the item and clears its texture item's entry; returns the
    // texture item, null when the entry is not in the table.
    BSTexMapItem* Remove(const BSFileEntry* apEntry);
    // Frees every bucket item, clearing the entry each one points at, and empties the table.
    void RemoveAll();
    // Adds apFileEntry's texture to the bucket chain.
    void SetAt(const BSFileEntry* apEntry, BSTexMapItem* apValue);

    // The hash table slot of apKey: (key >> 4) % 1001.
    static uint32_t KeyToHashIndex(const BSFileEntry* apKey);

    BSEntryMapItem* pItemA[1001];
    uint32_t iCount;
};
