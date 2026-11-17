#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeDynamicData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiTriShapeDynamicData::ms_RTTI("NiTriShapeDynamicData", &NiTriShapeData::ms_RTTI);

NiTriShapeDynamicData::NiTriShapeDynamicData() : m_usActiveVertices(0), m_usActiveTriangles(0) {}

// The base is built over the active counts; the full counts are stored after it.
NiTriShapeDynamicData::NiTriShapeDynamicData(
    uint16_t usVertices,
    NiPoint3* pkVertex,
    NiPoint3* pkNormal,
    NiColorA* pkColor,
    NiPoint2* pkTexture,
    int32_t usNumTextureSets,
    NiGeometryData::DataFlags eNBTMethod,
    uint16_t usTriangles,
    uint16_t* pusTriList,
    int32_t usActiveVertices,
    int32_t usActiveTriangles
)
  : NiTriShapeData(
        usActiveVertices,
        pkVertex,
        pkNormal,
        pkColor,
        pkTexture,
        usNumTextureSets,
        eNBTMethod,
        usActiveTriangles,
        pusTriList
    ),
    m_usActiveVertices(static_cast<uint16_t>(usActiveVertices)),
    m_usActiveTriangles(static_cast<uint16_t>(usActiveTriangles)) {
    m_usTriangles = usTriangles;
    m_usVertices = usVertices;
    m_uiTriListLength = 3 * usTriangles;
}

NiTriShapeDynamicData::~NiTriShapeDynamicData() {}

const NiRTTI* NiTriShapeDynamicData::GetRTTI() const {
    return &ms_RTTI;
}

void NiTriShapeDynamicData::LoadBinary(NiStream& kStream) {
    NiTriShapeData::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_usActiveVertices);
    NiStreamLoadBinary(kStream, m_usActiveTriangles);
}

// NiTriStripsDynamicData uses the same function.
void NiTriShapeDynamicData::LinkObject(NiStream& kStream) {
    NiTriShapeData::LinkObject(kStream);
}

// NiTriStripsDynamicData uses the same function.
bool NiTriShapeDynamicData::RegisterStreamables(NiStream& kStream) {
    return NiTriShapeData::RegisterStreamables(kStream);
}

void NiTriShapeDynamicData::SaveBinary(NiStream& kStream) {
    NiTriShapeData::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_usActiveVertices);
    NiStreamSaveBinary(kStream, m_usActiveTriangles);
}

bool NiTriShapeDynamicData::IsEqual(NiObject* pkObject) {
    if (!NiTriShapeData::IsEqual(pkObject)) {
        return false;
    }
    NiTriShapeDynamicData* pkData = static_cast<NiTriShapeDynamicData*>(pkObject);
    return m_usActiveVertices == pkData->m_usActiveVertices && m_usActiveTriangles == pkData->m_usActiveTriangles;
}

void NiTriShapeDynamicData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiTriShapeData::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_usActiveVertices", m_usActiveVertices));
    pkStrings->Add(NiGetViewerString("m_usActiveTriangles", m_usActiveTriangles));
}

void NiTriShapeDynamicData::SetActiveVertexCount(uint16_t usActiveVertices) {
    m_usActiveVertices = usActiveVertices;
}

uint16_t NiTriShapeDynamicData::GetActiveVertexCount() const {
    return m_usActiveVertices;
}

void NiTriShapeDynamicData::SetActiveTriangleCount(uint16_t usActiveTriangles) {
    m_usActiveTriangles = usActiveTriangles;
}

uint16_t NiTriShapeDynamicData::GetActiveTriangleCount() const {
    return m_usActiveTriangles;
}

// As NiTriShapeData's, over the active triangles only.
void NiTriShapeDynamicData::GetStripData(
    uint16_t& usStrips,
    const uint16_t*& pusStripLengths,
    const uint16_t*& pusTriList,
    uint32_t& uiStripLengthSum
) const {
    usStrips = m_usActiveTriangles;
    pusStripLengths = nullptr;
    pusTriList = m_pusTriList;
    uiStripLengthSum = 3 * m_usActiveTriangles;
}

NiObject* NiTriShapeDynamicData::CreateObject() {
    NiTriShapeDynamicData* pkObject =
        static_cast<NiTriShapeDynamicData*>(NiMemObject::operator new(sizeof(NiTriShapeDynamicData)));
    if (!pkObject) {
        return 0;
    }
    return ::new (pkObject) NiTriShapeDynamicData;
}
