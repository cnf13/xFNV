#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeomData.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

// Triangle-list geometry data: the index list (three indices per triangle) and the shared-normal groups.
class NiTriShapeData : public NiTriBasedGeomData {
public:
    // The vertices that share one normal.
    class SharedNormalArray : public NiMemObject {
    public:
        // No shared normals.
        SharedNormalArray();

        uint16_t m_usNumSharedNormals;
        uint16_t* m_pusSharedNormalIndexArray;
    };

    // A block of shared-normal index storage; blocks are chained through m_pkNext.
    class SNAMemBlock : public NiMemObject {
    public:
        ~SNAMemBlock();

        // Blocks are freed through MemoryManager, not NiMemObject's operators.
        static void operator delete(void* apMemory);

        uint16_t* m_pusBlock;
        uint16_t* m_pusFreeBlock;
        uint32_t m_uiBlockSize;
        uint32_t m_uiFreeBlockSize;
        SNAMemBlock* m_pkNext;
    };

    NiTriShapeData();
    // Takes ownership of the given arrays.
    NiTriShapeData(
        int32_t usVertices,
        NiPoint3* pkVertex,
        NiPoint3* pkNormal,
        NiColorA* pkColor,
        NiPoint2* pkTexture,
        int32_t usNumTextureSets,
        NiGeometryData::DataFlags eNBTMethod,
        int32_t usTriangles,
        uint16_t* pusTriList
    );
    ~NiTriShapeData() override;

    const NiRTTI* GetRTTI() const override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    NiTriShapeData* IsTriShapeData() override {
        return this;
    }
    void CalculateNormals() override;
    void GetTriangleIndices(uint16_t i, uint16_t& i0, uint16_t& i1, uint16_t& i2) const override;
    void GetStripData(
        uint16_t& usStrips,
        const uint16_t*& pusStripLengths,
        const uint16_t*& pusTriList,
        uint32_t& uiStripLengthSum
    ) const override;

    uint32_t GetTriListLength() const {
        return m_uiTriListLength;
    }
    uint16_t* GetTriList() const;
    // Swaps in a new triangle list.
    void Replace(uint16_t usTriangles, uint16_t* pusTriList);
    void SetData(uint16_t usTriangles, uint16_t* pusTriList);

    static NiObject* CreateObject();

    uint32_t m_uiTriListLength;
    uint16_t* m_pusTriList;
    SharedNormalArray* m_pkSharedNormals;
    uint16_t m_usSharedNormalsArraySize;
    SNAMemBlock* m_pkSNAMemoryBlocks;

    static NiRTTI ms_RTTI;
};

NiSmartPointer(NiTriShapeData);
