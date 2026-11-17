#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

class NiVBChip;
class NiVBSet;

// One vertex buffer of a set, with the chips carved from it; blocks with free space form a list in their set.
class NiVBBlock : public NiMemObject {
public:
    // Forgets the vertex buffer, the sizes and all the block's chips without freeing them.
    void ClearAllData();
    // . A chip of uiSize bytes carved from the block.
    NiVBChip* AllocateChip(uint32_t uiSize);
    // . Returns chip uiChipIndex to the block's free space.
    uint32_t FreeChip(uint32_t uiChipIndex);
    NiVBSet* GetSet() const {
        return m_pkSet;
    }
    IDirect3DVertexBuffer9* GetVB() const {
        return m_pkVB;
    }
    uint32_t GetSize() const {
        return m_uiSize;
    }
    uint32_t GetFVF() const {
        return m_uiFVF;
    }
    uint32_t GetFlags() const {
        return m_uiFlags;
    }
    uint32_t GetIndex() const {
        return m_uiIndex;
    }
    uint32_t GetCurrOffset() const {
        return m_uiCurrOffset;
    }
    uint32_t GetFreeBytes() const {
        return m_uiFreeBytes;
    }
    uint32_t GetTotalFreeBytes() const {
        return m_uiTotalFreeBytes;
    }
    void DestroyChipArray();

    // Frees the chips of the dump list.
    static void DeleteChips();
    // Allocates the chips of the dump list.
    static void AllocateChips();

    NiVBSet* m_pkSet;
    uint32_t m_uiIndex;
    IDirect3DVertexBuffer9* m_pkVB;
    uint32_t m_uiSize;
    uint32_t m_uiFVF;
    uint32_t m_uiFlags;
    uint32_t m_uiUsage;
    // D3DPOOL.
    uint32_t m_ePool;
    uint32_t m_uiCurrOffset;
    uint32_t m_uiFreeBytes;
    uint32_t m_uiTotalFreeBytes;
    NiTPrimitiveArray<NiVBChip*> m_apkChips;
    NiVBBlock* m_pkNextBlock;
    NiVBBlock* m_pkPrevBlock;

    static inline NiVBChip* ms_pkChipDump;
    static inline uint32_t ms_uiDefClipAllocSize = 32;
};
