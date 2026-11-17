#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeomData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsDynamicData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI NiTriStripsData::ms_RTTI("NiTriStripsData", &NiTriBasedGeomData::ms_RTTI);

namespace {

    // Takes auiCount indices from the object group's buffer when one is set, else from the heap.
    uint16_t* AllocateIndices(NiTriStripsData* apData, uint32_t auiCount) {
        if (apData->GetGroup()) {
            NiObjectGroup* pkGroup = apData->GetGroup();
            uint16_t* pusIndices = static_cast<uint16_t*>(pkGroup->m_pvFree);
            pkGroup->m_uiRefCount++;
            pkGroup->m_pvFree = pusIndices + auiCount;
            return pusIndices;
        }
        return static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * auiCount));
    }

} // namespace

NiTriStripsData::NiTriStripsData() : m_usStrips(0), m_pusStripLengths(nullptr), m_pusStripLists(nullptr) {}

NiTriStripsData::NiTriStripsData(
    int32_t usVertices,
    NiPoint3* pkVertex,
    NiPoint3* pkNormal,
    NiColorA* pkColor,
    NiPoint2* pkTexture,
    int32_t usNumTextureSets,
    int32_t eNBTMethod,
    int32_t usTriangles,
    int32_t usStrips,
    uint16_t* pusStripLengths,
    uint16_t* pusStripLists
)
  : NiTriBasedGeomData(
        usVertices,
        pkVertex,
        pkNormal,
        pkColor,
        pkTexture,
        usNumTextureSets,
        static_cast<NiGeometryData::DataFlags>(eNBTMethod),
        usTriangles
    ),
    m_usStrips(static_cast<uint16_t>(usStrips)), m_pusStripLengths(pusStripLengths), m_pusStripLists(pusStripLists) {}

// Data in a group releases the group twice, deliberately, as the game does; otherwise the strip arrays are this
// object's _NiMalloc blocks.
NiTriStripsData::~NiTriStripsData() {
    if (GetGroup()) {
        for (int32_t i = 0; i < 2; ++i) {
            NiObjectGroup* pkGroup = GetGroup();
            if (--pkGroup->m_uiRefCount == 0) {
                pkGroup->FreeVertexBuffer();
                delete pkGroup;
            }
        }
    }
    else {
        _NiFree(m_pusStripLengths);
        _NiFree(m_pusStripLists);
    }
}

const NiRTTI* NiTriStripsData::GetRTTI() const {
    return &ms_RTTI;
}

void NiTriStripsData::LoadBinary(NiStream& kStream) {
    NiTriBasedGeomData::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_usStrips);
    if (!m_usStrips) {
        return;
    }
    m_pusStripLengths = AllocateIndices(this, m_usStrips);
    NiStreamLoadBinary(kStream, m_pusStripLengths, m_usStrips);
    if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 0, 1, 17)) {
        bool bHasLists;
        NiStreamLoadBinary(kStream, bHasLists);
        if (!bHasLists) {
            return;
        }
    }
    uint16_t usListLength = static_cast<uint16_t>(m_usTriangles + 2 * m_usStrips);
    m_pusStripLists = AllocateIndices(this, usListLength);
    NiStreamLoadBinary(kStream, m_pusStripLists, usListLength);
}

void NiTriStripsData::LinkObject(NiStream& kStream) {
    NiTriBasedGeomData::LinkObject(kStream);
}

bool NiTriStripsData::RegisterStreamables(NiStream& kStream) {
    return NiTriBasedGeomData::RegisterStreamables(kStream);
}

void NiTriStripsData::SaveBinary(NiStream& kStream) {
    NiTriBasedGeomData::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_usStrips);
    if (!m_usStrips) {
        return;
    }
    NiStreamSaveBinary(kStream, m_pusStripLengths, m_usStrips);
    bool bHasLists = m_pusStripLists != nullptr;
    NiStreamSaveBinary(kStream, bHasLists);
    if (bHasLists) {
        NiStreamSaveBinary(kStream, m_pusStripLists, static_cast<uint16_t>(m_usTriangles + 2 * m_usStrips));
    }
}

bool NiTriStripsData::IsEqual(NiObject* pkObject) {
    if (!NiTriBasedGeomData::IsEqual(pkObject)) {
        return false;
    }
    NiTriStripsData* pkData = static_cast<NiTriStripsData*>(pkObject);
    if (m_usStrips != pkData->m_usStrips) {
        return false;
    }
    for (uint16_t i = 0; i < m_usStrips; i++) {
        if (m_pusStripLengths[i] != pkData->m_pusStripLengths[i]) {
            return false;
        }
    }
    uint16_t usListLength = static_cast<uint16_t>(m_usTriangles + 2 * m_usStrips);
    for (uint16_t i = 0; i < usListLength; i++) {
        if (m_pusStripLists[i] != pkData->m_pusStripLists[i]) {
            return false;
        }
    }
    return true;
}

void NiTriStripsData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiTriBasedGeomData::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_usStrips", m_usStrips));
    pkStrings->Add(NiGetViewerString("m_pusStripLengths", static_cast<const void*>(m_pusStripLengths)));
    pkStrings->Add(NiGetViewerString("m_pusStripLists", static_cast<const void*>(m_pusStripLists)));
}

// The strip lengths and lists live in the same block as the vertex data. The list length is a 16-bit sum.
uint32_t NiTriStripsData::GetBlockAllocationSize() const {
    return sizeof(uint16_t) * (m_usStrips + static_cast<uint16_t>(m_usTriangles + 2 * m_usStrips)) +
        NiGeometryData::GetBlockAllocationSize();
}

// Sums the unit face normal of every non-degenerate strip triangle into its three vertices, then normalizes them.
// Odd triangles of a strip are wound the other way round.
void NiTriStripsData::CalculateNormals() {
    CreateNormals(true);
    const uint16_t* pusList = m_pusStripLists;
    for (uint16_t i = 0; i < m_usStrips; i++) {
        for (uint16_t j = 2; j < m_pusStripLengths[i]; j++) {
            uint16_t usI0 = pusList[j - 2];
            uint16_t usI1;
            uint16_t usI2;
            if (j & 1) {
                usI1 = pusList[j];
                usI2 = pusList[j - 1];
            }
            else {
                usI1 = pusList[j - 1];
                usI2 = pusList[j];
            }
            if (usI0 == usI1 || usI1 == usI2 || usI2 == usI0) {
                continue;
            }

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
        }
        pusList += m_pusStripLengths[i];
    }
    NiPoint3::UnitizeVectors(m_pkNormal, m_usVertices, sizeof(NiPoint3));
    MarkAsChanged(NORMAL_MASK);
}

// Walks the strips to the one holding the triangle; odd triangles of a strip swap their first two corners.
void NiTriStripsData::GetTriangleIndices(uint16_t i, uint16_t& i0, uint16_t& i1, uint16_t& i2) const {
    const uint16_t* pusList = m_pusStripLists;
    const uint16_t* pusLength = m_pusStripLengths;
    while (i >= static_cast<uint16_t>(*pusLength - 2)) {
        i -= static_cast<uint16_t>(*pusLength - 2);
        pusList += *pusLength;
        pusLength++;
    }
    if (i & 1) {
        i0 = pusList[i + 1];
        i1 = pusList[i];
    }
    else {
        i0 = pusList[i];
        i1 = pusList[i + 1];
    }
    i2 = pusList[i + 2];
}

void NiTriStripsData::GetStripData(
    uint16_t& usStrips,
    const uint16_t*& pusStripLengths,
    const uint16_t*& pusTriList,
    uint32_t& uiStripLengthSum
) const {
    usStrips = m_usStrips;
    pusStripLengths = m_pusStripLengths;
    pusTriList = m_pusStripLists;
    uiStripLengthSum = static_cast<uint16_t>(m_usTriangles + 2 * m_usStrips);
}

void NiTriStripsData::Replace(uint16_t usStrips, uint16_t* pusStripLengths, uint16_t* pusStripLists) {
    uint16_t* v8;
    if (GetGroup()) {
        bool v7;
        if (pusStripLengths != this->GetStripLengths()) {
            NiObjectGroup* v5 = GetGroup();
            v7 = v5->m_uiRefCount-- == 1;
            if (v7) {
                v5->FreeVertexBuffer();
                NiMemObject::operator delete(v5, 16u);
            }
        }
        v8 = pusStripLists;
        if (pusStripLists != this->GetStripLists()) {
            NiObjectGroup* v9 = GetGroup();
            v7 = v9->m_uiRefCount-- == 1;
            if (v7) {
                v9->FreeVertexBuffer();
                NiMemObject::operator delete(v9, 16u);
            }
        }
    }
    else {
        if (pusStripLengths != this->GetStripLengths()) {
            _NiFree(this->GetStripLengths());
        }
        v8 = pusStripLists;
        if (pusStripLists != this->GetStripLists()) {
            _NiFree(this->GetStripLists());
        }
    }
    m_usStrips = usStrips;
    m_pusStripLengths = pusStripLengths;
    m_pusStripLists = v8;
    m_usTriangles = 0;
    if (usStrips) {
        int32_t uiStrips = usStrips;
        do {
            m_usTriangles += *pusStripLengths++ - 2;
            --uiStrips;
        }
        while (uiStrips);
    }
}

NiObject* NiTriStripsData::CreateObject() {
    NiTriStripsData* pkObject = static_cast<NiTriStripsData*>(NiMemObject::operator new(sizeof(NiTriStripsData)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiTriStripsData;
}
