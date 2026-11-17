#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Resource.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"

#include <stdint.h>

class NiGeometryBufferData;
class NiGeometryData;
class NiPoint2;
class NiSkinInstance;
class NiVBChip;

// Owns the vertex buffers of the scene's geometry, by pool.
class NiDX9VertexBufferManager : public NiRefObject {
public:
    // One Direct3D vertex buffer.
    class NiDX9VBResource : public NiDX9Resource {
    public:
        uint32_t m_uiLength;
        uint32_t m_uiFVF;
        uint32_t m_uiRevID;
        IDirect3DVertexBuffer9* m_pkVB;
        D3DVERTEXBUFFER_DESC m_kVBDesc;
    };

    // The buffers of one vertex buffer id.
    class NiDX9VBInfo : public NiMemObject {
    public:
        uint32_t m_uiVBID;
        uint8_t m_ucIndex;
        NiDX9VBResource* m_pkVBRes[5];
    };

    NiDX9VertexBufferManager(IDirect3DDevice9* a1);
    ~NiDX9VertexBufferManager() override;

    void EndFrame();
    // Releases and recreates the buffer data's vertex chip of stream auiStream; false without a new chip.
    bool AllocateBufferSpace(NiGeometryBufferData* apBuffData, uint32_t auiStream);
    // Locks the vertex buffer and hands out the temporary buffer (holding a copy of the locked bytes unless the lock
    // discards or does not overwrite); the critical section stays entered until UnlockVB. Null when the lock fails.
    void* LockVB(IDirect3DVertexBuffer9* apVB, uint32_t auiOffset, uint32_t auiSize, uint32_t auiLockFlags);
    // Copies the temporary buffer back, leaves the critical section and unlocks the vertex buffer.
    bool UnlockVB(IDirect3DVertexBuffer9* apVB);
    // Returns the frame counter that EndFrame advances.
    uint32_t GetFrameID();
    // Packs a skin partition's vertices, weights, normals, colors and texture coordinates into apvLockedBuff, or into
    // apOldVBChip (when it still fits) or a new chip. Null when no chip can be made or unlocked.
    NiVBChip* PackSkinnedVB(
        NiGeometryData* apData,
        NiSkinInstance* apSkin,
        NiSkinPartition::Partition* apPartition,
        uint32_t auiDirtyFlags,
        NiVBChip* apOldVBChip,
        void* apvLockedBuff
    );
    // Packs the geometry data's vertices (software skinned with a skin instance), normals, colors and texture
    // coordinates the same way; apOldVBChip when there are no active vertices.
    NiVBChip* PackUntransformedVB(
        NiGeometryData* apData,
        NiSkinInstance* apSkin,
        uint32_t auiDirtyFlags,
        NiVBChip* apOldVBChip,
        void* apvLockedBuff
    );
    // Packs screen-space vertices (z 0, rhw 1), colors and texture coordinates the same way; apOldVBChip without
    // vertices or positions.
    NiVBChip* PackTransformedVB(
        NiGeometryBufferData* apBuffData,
        uint16_t ausVerts,
        const NiPoint2* apkPositions,
        const NiColorA* apkColors,
        const NiPoint2* apkTexCoords,
        uint32_t auiDirtyFlags,
        NiVBChip* apOldVBChip,
        void* apvLockedBuff
    );
    // The vertex format, stride and component offsets of the data (with blend weights for a partition's bones).
    void GenerateVertexParameters(
        NiGeometryData* apData,
        const NiSkinPartition::Partition* apPartition,
        uint32_t& auiFVF,
        uint32_t& auiVertexStride,
        uint32_t& auiPosOffset,
        uint32_t& auiWeightOffset,
        uint32_t& auiNormOffset,
        uint32_t& auiColorOffset,
        uint32_t& auiTexOffset
    );
    // Copies auiNumVerts elements of auiDataSize bytes from apucSrc (packed) to apucDest (auiVertexSize apart).
    void PackVB(
        uint8_t* apucDest,
        const uint8_t* apucSrc,
        uint32_t auiVertexSize,
        uint32_t auiDataSize,
        uint32_t auiNumVerts
    );
    // Releases and deletes every default-pool buffer and empties the default pool.
    void DestroyDefaultBuffers();

    IDirect3DDevice9* m_pkD3DDevice9;
    NiTPointerMap<uint32_t, NiDX9VBInfo*> m_kDefaultResources;
    NiTPointerMap<uint32_t, NiDX9VBInfo*> m_kManagedResources;
    NiTPointerMap<uint32_t, NiDX9VBInfo*> m_kSystemMemResources;
    uint32_t m_uiFrameID;
    void* m_pvTempBuffer;
    uint32_t m_uiTempBufferSize;
    void* m_pvLockedBuffer;
    uint32_t m_uiLockedBufferSize;
    NiCriticalSection m_kVBCriticalSection;
};
