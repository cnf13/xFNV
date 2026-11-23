#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeomData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiTriBasedGeomData::ms_RTTI("NiTriBasedGeomData", &NiGeometryData::ms_RTTI);

NiTriBasedGeomData::NiTriBasedGeomData() : m_usTriangles(0) {}

NiTriBasedGeomData::NiTriBasedGeomData(
    int32_t usVertices,
    NiPoint3* pkVertex,
    NiPoint3* pkNormal,
    NiColorA* pkColor,
    NiPoint2* pkTexture,
    int32_t usNumTextureSets,
    NiGeometryData::DataFlags aeDataFlags,
    int32_t usTriangles
)
  : NiGeometryData(usVertices, pkVertex, pkNormal, pkColor, pkTexture, usNumTextureSets, aeDataFlags),
    m_usTriangles(static_cast<uint16_t>(usTriangles)) {}

NiTriBasedGeomData::~NiTriBasedGeomData() {}

void NiTriBasedGeomData::LoadBinary(NiStream& kStream) {
    NiGeometryData::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_usTriangles);
}

void NiTriBasedGeomData::LinkObject(NiStream& kStream) {
    NiGeometryData::LinkObject(kStream);
}

bool NiTriBasedGeomData::RegisterStreamables(NiStream& kStream) {
    return NiGeometryData::RegisterStreamables(kStream);
}

void NiTriBasedGeomData::SaveBinary(NiStream& kStream) {
    NiGeometryData::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_usTriangles);
}

bool NiTriBasedGeomData::IsEqual(NiObject* pkObject) {
    bool result = NiGeometryData::IsEqual(pkObject);
    if (result) {
        // Gamebryo's downcast: the base IsEqual has checked that apObject is of this class.
        NiTriBasedGeomData* pkData = static_cast<NiTriBasedGeomData*>(pkObject);
        return m_usTriangles == pkData->m_usTriangles;
    }
    return result;
}

void NiTriBasedGeomData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiGeometryData::GetViewerStrings(pkStrings);
    char* ViewerString = NiGetViewerString(NiTriBasedGeomData::ms_RTTI.GetName());
    uint32_t m_usSize = pkStrings->m_usSize;
    uint32_t m_usMaxSize = pkStrings->m_usMaxSize;
    char* v10 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        pkStrings->SetSize(m_usSize + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(m_usSize, v10);
    char* ViewerString_4 = NiGetViewerString("m_usTriangles", m_usTriangles);
    uint32_t v8 = pkStrings->m_usSize;
    uint32_t v9 = pkStrings->m_usMaxSize;
    v10 = ViewerString_4;
    if (v8 >= v9) {
        pkStrings->SetSize(v8 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v8, v10);
}

uint16_t NiTriBasedGeomData::GetTriCount() {
    return m_usTriangles;
}
