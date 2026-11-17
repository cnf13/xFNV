#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeomData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeDynamicData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI NiTriShapeData::ms_RTTI("NiTriShapeData", &NiTriBasedGeomData::ms_RTTI);

NiTriShapeData::SharedNormalArray::SharedNormalArray()
  : m_usNumSharedNormals(0), m_pusSharedNormalIndexArray(nullptr) {}

// Frees this block's indices and the rest of the chain.
NiTriShapeData::SNAMemBlock::~SNAMemBlock() {
    _NiFree(m_pusBlock);
    delete m_pkNext;
}

void NiTriShapeData::SNAMemBlock::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

NiTriShapeData::NiTriShapeData()
  : m_uiTriListLength(0), m_pusTriList(nullptr), m_pkSharedNormals(nullptr), m_usSharedNormalsArraySize(0),
    m_pkSNAMemoryBlocks(nullptr) {}

NiTriShapeData::NiTriShapeData(
    int32_t usVertices,
    NiPoint3* pkVertex,
    NiPoint3* pkNormal,
    NiColorA* pkColor,
    NiPoint2* pkTexture,
    int32_t usNumTextureSets,
    NiGeometryData::DataFlags eNBTMethod,
    int32_t usTriangles,
    uint16_t* pusTriList
)
  : NiTriBasedGeomData(usVertices, pkVertex, pkNormal, pkColor, pkTexture, usNumTextureSets, eNBTMethod, usTriangles),
    m_uiTriListLength(3 * m_usTriangles), m_pusTriList(pusTriList), m_pkSharedNormals(nullptr),
    m_usSharedNormalsArraySize(0), m_pkSNAMemoryBlocks(nullptr) {}

// The triangle list and each block's indices are _NiMalloc blocks; the shared-normal array is raw MemoryManager
// storage (LoadBinary allocates it so).
NiTriShapeData::~NiTriShapeData() {
    if (m_pusTriList) {
        _NiFree(m_pusTriList);
    }
    delete m_pkSNAMemoryBlocks;
    MemoryManager_Singleton.Deallocate(m_pkSharedNormals);
}

const NiRTTI* NiTriShapeData::GetRTTI() const {
    return &ms_RTTI;
}

// KEEP: niread: from 10.0.1.17 on, the exe reads the has-list byte into the stream argument's own slot (it sets that
// byte to 1 only for older files), and each shared-normal group's count into a stack slot nothing initialised: a
// short read (malformed data) leaves the stream's address bits or frame garbage driving the allocation, the loop
// and the stored counts. src cannot express either; porting.md's unwritten-slot exception covers GetChunkData only.
// Reads the index list and the shared-normal groups; the groups' indices are carved out of chained SNAMemBlocks.
void NiTriShapeData::LoadBinary(NiStream& kStream) {
    NiTriBasedGeomData::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_uiTriListLength);
    bool bHasList;
    if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 0, 1, 17)) {
        NiStreamLoadBinary(kStream, bHasList);
    }
    else {
        bHasList = true;
    }
    if (bHasList && m_uiTriListLength) {
        m_pusTriList = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * m_uiTriListLength));
        NiStreamLoadBinary(kStream, m_pusTriList, m_uiTriListLength);
    }

    NiStreamLoadBinary(kStream, m_usSharedNormalsArraySize);
    if (!m_usSharedNormalsArraySize) {
        return;
    }

    // new SharedNormalArray[n].
    SharedNormalArray* pkArrays = static_cast<SharedNormalArray*>(
        MemoryManager_Singleton.Allocate(sizeof(SharedNormalArray) * m_usSharedNormalsArraySize)
    );
    if (pkArrays) {
        rt::eh_vector_constructor_iterator(
            pkArrays, sizeof(SharedNormalArray), m_usSharedNormalsArraySize,
            &fnvd::constructElement<NiTriShapeData::SharedNormalArray>
        );
    }
    m_pkSharedNormals = pkArrays;

    SNAMemBlock* pkFirst = static_cast<SNAMemBlock*>(MemoryManager_Singleton.Allocate(sizeof(SNAMemBlock)));
    if (pkFirst) {
        uint32_t uiSize = m_usSharedNormalsArraySize;
        pkFirst->m_pusBlock = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * uiSize));
        pkFirst->m_pusFreeBlock = pkFirst->m_pusBlock;
        pkFirst->m_uiBlockSize = uiSize;
        pkFirst->m_uiFreeBlockSize = uiSize;
        pkFirst->m_pkNext = nullptr;
    }
    m_pkSNAMemoryBlocks = pkFirst;

    for (uint16_t i = 0; i < m_usSharedNormalsArraySize; i++) {
        uint16_t usCount;
        NiStreamLoadBinary(kStream, usCount);
        uint16_t* pusIndices = nullptr;
        if (usCount) {
            // Take the indices from the first block with room, or from a new head block twice the old head's size.
            SNAMemBlock* pkBlock = m_pkSNAMemoryBlocks;
            while (pkBlock && usCount > pkBlock->m_uiFreeBlockSize) {
                pkBlock = pkBlock->m_pkNext;
            }
            if (!pkBlock) {
                pkBlock = static_cast<SNAMemBlock*>(MemoryManager_Singleton.Allocate(sizeof(SNAMemBlock)));
                if (pkBlock) {
                    uint32_t uiSize = 2 * m_pkSNAMemoryBlocks->m_uiBlockSize;
                    pkBlock->m_pusBlock = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * uiSize));
                    pkBlock->m_pusFreeBlock = pkBlock->m_pusBlock;
                    pkBlock->m_uiBlockSize = uiSize;
                    pkBlock->m_uiFreeBlockSize = uiSize;
                    pkBlock->m_pkNext = nullptr;
                }
                pkBlock->m_pkNext = m_pkSNAMemoryBlocks;
                m_pkSNAMemoryBlocks = pkBlock;
            }
            pusIndices = pkBlock->m_pusFreeBlock;
            pkBlock->m_uiFreeBlockSize -= usCount;
            pkBlock->m_pusFreeBlock = pusIndices + usCount;
            NiStreamLoadBinary(kStream, pusIndices, usCount);
        }
        SharedNormalArray& rkArray = m_pkSharedNormals[i];
        if (usCount && pusIndices) {
            rkArray.m_pusSharedNormalIndexArray = pusIndices;
        }
        else {
            usCount = 0;
            rkArray.m_pusSharedNormalIndexArray = nullptr;
        }
        rkArray.m_usNumSharedNormals = usCount;
    }
}

// NiTriStripsData uses the same function.
void NiTriShapeData::LinkObject(NiStream& kStream) {
    NiTriBasedGeomData::LinkObject(kStream);
}

// NiTriStripsData uses the same function.
bool NiTriShapeData::RegisterStreamables(NiStream& kStream) {
    return NiTriBasedGeomData::RegisterStreamables(kStream);
}

void NiTriShapeData::SaveBinary(NiStream& kStream) {
    NiTriBasedGeomData::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uiTriListLength);
    bool bHasList = m_pusTriList != nullptr;
    NiStreamSaveBinary(kStream, bHasList);
    if (bHasList) {
        NiStreamSaveBinary(kStream, m_pusTriList, m_uiTriListLength);
    }
    NiStreamSaveBinary(kStream, m_usSharedNormalsArraySize);
    for (uint16_t i = 0; i < m_usSharedNormalsArraySize; i++) {
        // The 2-byte count is kept zero-extended in a 32-bit variable; its low word is written and then read back from
        // that variable.
        uint32_t uiCount = m_pkSharedNormals[i].m_usNumSharedNormals;
        uint32_t uiSize = 2;
        kStream.m_pkOstr->BinaryWrite(&uiCount, 2, &uiSize, 1);
        uint16_t usCount = static_cast<uint16_t>(uiCount);
        if (usCount) {
            NiStreamSaveBinary(kStream, m_pkSharedNormals[i].m_pusSharedNormalIndexArray, usCount);
        }
    }
}

bool NiTriShapeData::IsEqual(NiObject* pkObject) {
    if (!NiTriBasedGeomData::IsEqual(pkObject)) {
        return false;
    }
    NiTriShapeData* pkData = static_cast<NiTriShapeData*>(pkObject);
    if (m_uiTriListLength != pkData->m_uiTriListLength) {
        return false;
    }
    return std::memcmp(m_pusTriList, pkData->m_pusTriList, sizeof(uint16_t) * m_uiTriListLength) == 0;
}

void NiTriShapeData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiTriBasedGeomData::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_uiTriListLength", m_uiTriListLength));
    pkStrings->Add(NiGetViewerString("m_pusTriList", static_cast<const void*>(m_pusTriList)));
}

// Sums the unit face normal of every triangle into its three vertices (and into the vertices sharing each one's
// normal), then normalizes them.
void NiTriShapeData::CalculateNormals() {
    CreateNormals(true);
    uint16_t usTriangles = GetActiveTriangleCount();
    for (uint16_t i = 0; i < usTriangles; i++) {
        uint16_t usI0 = m_pusTriList[3 * i];
        uint16_t usI1 = m_pusTriList[3 * i + 1];
        uint16_t usI2 = m_pusTriList[3 * i + 2];
        NiPoint3& rkV0 = m_pkVertex[usI0];
        NiPoint3& rkV1 = m_pkVertex[usI1];
        NiPoint3& rkV2 = m_pkVertex[usI2];
        float fE1x = rkV1.x - rkV0.x;
        float fE1y = rkV1.y - rkV0.y;
        float fE1z = rkV1.z - rkV0.z;
        float fE2x = rkV2.x - rkV1.x;
        float fE2y = rkV2.y - rkV1.y;
        float fE2z = rkV2.z - rkV1.z;
        NiPoint3 kNormal;
        kNormal.x = fE2z * fE1y - fE2y * fE1z;
        kNormal.y = fE1z * fE2x - fE2z * fE1x;
        kNormal.z = fE1x * fE2y - fE2x * fE1y;
        NiPoint3::UnitizeVector(kNormal);

        uint16_t ausCorners[3] = {usI0, usI1, usI2};
        for (uint16_t usCorner : ausCorners) {
            m_pkNormal[usCorner].x += kNormal.x;
            m_pkNormal[usCorner].y += kNormal.y;
            m_pkNormal[usCorner].z += kNormal.z;
        }

        if (m_pkSharedNormals && m_usSharedNormalsArraySize == m_usVertices) {
            for (uint16_t usCorner : ausCorners) {
                SharedNormalArray& rkShared = m_pkSharedNormals[usCorner];
                for (int32_t j = rkShared.m_usNumSharedNormals - 1; j >= 0; j--) {
                    NiPoint3& rkNormal = m_pkNormal[rkShared.m_pusSharedNormalIndexArray[j]];
                    rkNormal.x += kNormal.x;
                    rkNormal.y += kNormal.y;
                    rkNormal.z += kNormal.z;
                }
            }
        }
    }
    NiPoint3::UnitizeVectors(m_pkNormal, GetActiveVertexCount(), sizeof(NiPoint3));
    MarkAsChanged(NORMAL_MASK);
}

void NiTriShapeData::GetTriangleIndices(uint16_t i, uint16_t& i0, uint16_t& i1, uint16_t& i2) const {
    i0 = m_pusTriList[3 * i];
    i1 = m_pusTriList[3 * i + 1];
    i2 = m_pusTriList[3 * i + 2];
}

// A triangle list is reported as one strip per triangle with no lengths.
void NiTriShapeData::GetStripData(
    uint16_t& usStrips,
    const uint16_t*& pusStripLengths,
    const uint16_t*& pusTriList,
    uint32_t& uiStripLengthSum
) const {
    usStrips = m_usTriangles;
    pusStripLengths = nullptr;
    pusTriList = m_pusTriList;
    uiStripLengthSum = 3 * m_usTriangles;
}

uint16_t* NiTriShapeData::GetTriList() const {
    return m_pusTriList;
}

void NiTriShapeData::Replace(uint16_t usTriangles, uint16_t* pusTriList) {
    uint16_t* m_pusTriList = this->m_pusTriList;
    if (pusTriList != m_pusTriList) {
        if (m_pusTriList) {
            _NiFree(this->m_pusTriList);
        }
    }
    m_usTriangles = usTriangles;
    this->m_pusTriList = pusTriList;
    m_uiTriListLength = 3 * usTriangles;
}

void NiTriShapeData::SetData(uint16_t usTriangles, uint16_t* pusTriList) {
    m_usTriangles = usTriangles;
    m_uiTriListLength = 3 * usTriangles;
    m_pusTriList = pusTriList;
}

NiObject* NiTriShapeData::CreateObject() {
    NiTriShapeData* pkObject = static_cast<NiTriShapeData*>(NiMemObject::operator new(sizeof(NiTriShapeData)));
    if (!pkObject) {
        return 0;
    }
    return ::new (pkObject) NiTriShapeData;
}
