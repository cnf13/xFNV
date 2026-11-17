#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

class NiVBBlock;
class NiVBChip;

// The vertex buffer blocks of one vertex format, with a list of the blocks that still have free space.
class NiVBSet : public NiMemObject {
public:
    // The default block size and free-space threshold.
    enum : int32_t {
        NIVBSET_DEF_BLOCKSIZE = 0x10000,
        NIVBSET_DEF_FREETHRESHOLD = 0x80,
    };

    // The set's flags.
    enum : int32_t {
        NIVBSET_FLAG_SOFTWAREVP = -0x7FFFFFFF - 1,
    };

    NiVBSet();

    uint32_t GetBlockSize() const {
        return m_uiBlockSize;
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
    uint32_t GetUsage() const {
        return m_uiUsage;
    }
    uint32_t GetPool() const {
        return m_ePool;
    }
    void RemoveBlockFromFreeSpaceList(NiVBBlock* pkVBBlock);
    void InsertBlockIntoFreeSpaceList(NiVBBlock* pkVBBlock);
    void FreeBlock(NiVBBlock* pkBlock);
    // . A new block of at least uiSize bytes, or null.
    NiVBBlock* CreateBlock(uint32_t uiSize);
    // A chip of uiSize bytes from a block with room (or a new block); null when no block can be made.
    NiVBChip* AllocateChip(uint32_t uiSize, bool bForceNewBlock);
    // Frees chip uiChipIndex of block uiBlockIndex and re-lists or frees the block as its free space changes.
    void FreeChip(uint32_t uiBlockIndex, uint32_t uiChipIndex);
    void E943C0();

protected:
    NiVBBlock* FindBlockWithFreeSpace(uint32_t uiFreeSize);

public:
    // A new set with the given block size, device, vertex format, flags and index; null when out of memory.
    static NiVBSet*
    Create(uint32_t uiBlockSize, IDirect3DDevice9* pkD3DDevice, uint32_t uiFVF, uint32_t uiFlags, uint32_t uiIndex);
    // Dynamic sets (flag 0x80000000) get D3DUSAGE_DYNAMIC; the pool is managed unless
    // the renderer uses Direct3D9Ex.
    static void SetPoolAndUsage(NiVBSet* apSet);
    static void DeleteBlocks();
    static void AllocateBlocks();

    uint32_t m_uiIndex;
    uint32_t m_uiFVF;
    uint32_t m_uiFlags;
    uint32_t m_uiBlockSize;
    IDirect3DDevice9* m_pkD3DDevice;
    uint32_t m_uiUsage;
    // D3DPOOL.
    uint32_t m_ePool;
    NiTPrimitiveArray<NiVBBlock*> m_apkBlocks;
    NiVBBlock* m_pkBlockList;
    uint32_t m_uiFreeSpaceThreshold;

    static inline uint32_t ms_uiDefBlockAllocSize = 32;
    static inline uint32_t ms_uiDefBlockSize = 0x10000;
    static inline uint32_t ms_uiDefFreeSpaceThreshold = 0x80;
    static inline NiVBBlock* ms_pkBlockDump;
};
