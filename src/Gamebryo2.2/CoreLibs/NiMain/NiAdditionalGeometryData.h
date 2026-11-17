#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

// Extra per-vertex data streams for a geometry (tangent frames, blend weights and the like): raw data blocks plus a
// table of streams that say where in a block each stream's values start.
class NiAdditionalGeometryData : public NiObject {
public:
    enum Types : uint32_t {
        AGD_NITYPE_INVALID = 0,
        AGD_NITYPE_FLOAT1 = 1,
        AGD_NITYPE_FLOAT2 = 2,
        AGD_NITYPE_FLOAT3 = 3,
        AGD_NITYPE_FLOAT4 = 4,
        AGD_NITYPE_LONG1 = 5,
        AGD_NITYPE_LONG2 = 6,
        AGD_NITYPE_LONG3 = 7,
        AGD_NITYPE_LONG4 = 8,
        AGD_NITYPE_ULONG1 = 9,
        AGD_NITYPE_ULONG2 = 10,
        AGD_NITYPE_ULONG3 = 11,
        AGD_NITYPE_ULONG4 = 12,
        AGD_NITYPE_SHORT1 = 13,
        AGD_NITYPE_SHORT2 = 14,
        AGD_NITYPE_SHORT3 = 15,
        AGD_NITYPE_SHORT4 = 16,
        AGD_NITYPE_USHORT1 = 17,
        AGD_NITYPE_USHORT2 = 18,
        AGD_NITYPE_USHORT3 = 19,
        AGD_NITYPE_USHORT4 = 20,
        AGD_NITYPE_BYTE1 = 21,
        AGD_NITYPE_BYTE2 = 22,
        AGD_NITYPE_BYTE3 = 23,
        AGD_NITYPE_BYTE4 = 24,
        AGD_NITYPE_UBYTE1 = 25,
        AGD_NITYPE_UBYTE2 = 26,
        AGD_NITYPE_UBYTE3 = 27,
        AGD_NITYPE_UBYTE4 = 28,
        AGD_NITYPE_BLEND1 = 29,
        AGD_NITYPE_BLEND2 = 30,
        AGD_NITYPE_BLEND3 = 31,
        AGD_NITYPE_BLEND4 = 32,
        AGD_NITYPE_COUNT = 33,
        AGD_NITYPE_CUSTOM = 34,
    };

    // Where one stream's values live: block, offset in the block, element type, unit size and stride.
    class NiAGDDataStream {
    public:
        // All fields zero, consistency mutable (flags 2).
        NiAGDDataStream()
          : m_uFlags(2), m_uiType(0), m_uiUnitSize(0), m_uiTotalSize(0), m_uiStride(0), m_uiBlockIndex(0),
            m_uiBlockOffset(0) {}

        void LoadBinary(NiStream& kStream);
        void SaveBinary(NiStream& kStream);
        void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings);
        // Sets the consistency flags from eConsistency.
        void SetConsistency(int32_t eConsistency);

        uint8_t ConvertFromConsistency(int32_t eConsistency);
        void FillInByteSizeArray(uint32_t* pauiDataArray, uint32_t& uiIndex, uint32_t uiArraySize);

        uint8_t m_uFlags;
        uint32_t m_uiType;
        uint32_t m_uiUnitSize;
        uint32_t m_uiTotalSize;
        uint32_t m_uiStride;
        uint32_t m_uiBlockIndex;
        uint32_t m_uiBlockOffset;
    };

    // One raw data block; the virtuals manage its buffer. Allocated with Bethesda's global new (MemoryManager).
    class NiAGDDataBlock {
    public:
        NiAGDDataBlock();
        virtual ~NiAGDDataBlock();

        virtual uint8_t* AllocateBuffer(uint32_t uiBufferSize);
        virtual void ProtectBuffer(void*, uint32_t) {}
        virtual void UnprotectBuffer(void*, uint32_t) {}
        virtual void FreeBuffer(void* pvBuffer);

        // Reads the block in its streams' layout (auiBlockIndex picks the streams).
        void LoadBinary(
            NiStream& kStream,
            uint32_t uiBlockIdx,
            NiAGDDataStream* pkDataStreams,
            uint32_t uiNumStreams,
            uint16_t usVertexCount
        );
        void SaveBinary(
            NiStream& kStream,
            uint32_t uiBlockIdx,
            NiAGDDataStream* pkDataStreams,
            uint32_t uiNumStreams,
            uint16_t usVertexCount
        );
        bool IsEqual(const NiAdditionalGeometryData::NiAGDDataBlock* pkOther);
        void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings);

        // The block comes from Bethesda's global new (MemoryManager).
        static void operator delete(void* apMemory);

        uint32_t m_uiDataBlockSize;
        uint8_t* m_pucDataBlock;
        bool m_bOwnData;
        bool m_bBSHeap;
    };

    NiAdditionalGeometryData();
    NiAdditionalGeometryData(uint16_t usVertexCount);
    NiAdditionalGeometryData(int32_t aiVertexCount, uint32_t uiDataStreamCount);
    NiAdditionalGeometryData(uint16_t usVertexCount, uint32_t uiDataStreamCount, int32_t a3);
    ~NiAdditionalGeometryData() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    virtual bool GetVBReady() {
        return false;
    }

protected:
    virtual NiAdditionalGeometryData::NiAGDDataBlock* CreateAGDDataBlock();

public:
    void SetDataBlockCount(uint32_t uiCount);
    void SetDataStreamCount(uint32_t uiCount);
    bool SetDataBlock(uint32_t uiIndex, uint8_t* pucData, uint32_t uiTotalSize, bool bCopyData);
    bool SetDataStream(
        uint32_t uiStreamIndex,
        uint32_t uiBlockIndex,
        uint32_t uiBlockOffset,
        uint32_t uiType,
        int32_t aiCount,
        uint32_t uiUnitSize,
        uint32_t uiStride
    );
    uint8_t* LockDataBlock(uint32_t uiIndex, uint32_t& uiBlockSize);
    void UnlockDataBlock(uint32_t uiIndex);
    bool GetDataStream(
        uint32_t uiStreamIndex,
        uint8_t*& pucData,
        uint32_t& uiType,
        uint16_t& usCount,
        uint32_t& uiTotalSize,
        uint32_t& uiUnitSize,
        uint32_t& uiStride
    );
    // Returns the block count, zero-extended to 32 bits; callers compare the whole value as unsigned.
    uint32_t GetSize() const;
    uint16_t GetVertexCount() const;
    const uint8_t* GetDataBlock(uint32_t uiIndex, uint32_t& uiBlockSize);
    bool RemoveDataBlock(uint32_t uiIndex, bool bRemoveAssociatedStreams);

    // Allocates and constructs an empty object through the no-argument constructor.
    static NiObject* CreateObject();

    uint32_t m_uiRevID;
    uint16_t m_usVertexCount;
    uint32_t m_uiDataStreamCount;
    NiAGDDataStream* m_pkDataStreamEntries;
    uint32_t m_uiD3DStream;
    NiTPrimitiveArray<NiAGDDataBlock*> m_aDataBlocks;

    static NiRTTI ms_RTTI;
};

// The base array of the data blocks (the type is only complete here, so its instance is declared here).
using NiTArrayNiAGDDataBlockPtr =
    NiTArray<NiAdditionalGeometryData::NiAGDDataBlock*, NiTMallocInterface<NiAdditionalGeometryData::NiAGDDataBlock*>>;
