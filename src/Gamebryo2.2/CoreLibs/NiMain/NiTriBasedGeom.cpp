#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeomData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiTriBasedGeom::ms_RTTI("NiTriBasedGeom", &NiGeometry::ms_RTTI);

NiTriBasedGeom::NiTriBasedGeom() {}

NiTriBasedGeom::NiTriBasedGeom(NiTriBasedGeomData* pkModelData) : NiGeometry(pkModelData) {}

NiTriBasedGeom::~NiTriBasedGeom() {}

void NiTriBasedGeom::LoadBinary(NiStream& kStream) {
    NiGeometry::LoadBinary(kStream);
}

void NiTriBasedGeom::LinkObject(NiStream& kStream) {
    NiGeometry::LinkObject(kStream);
}

bool NiTriBasedGeom::RegisterStreamables(NiStream& kStream) {
    return NiGeometry::RegisterStreamables(kStream);
}

void NiTriBasedGeom::SaveBinary(NiStream& kStream) {
    NiGeometry::SaveBinary(kStream);
}

bool NiTriBasedGeom::IsEqual(NiObject* pkObject) {
    return NiGeometry::IsEqual(pkObject);
}

void NiTriBasedGeom::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiGeometry::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.m_pcName));
}

void NiTriBasedGeom::SetModelData(NiGeometryData* pkModelData) {
    NiGeometry::SetModelData(pkModelData);
}

uint16_t NiTriBasedGeom::GetActiveTriangleCount() const {
    NiGeometryData* modelData = NiGeometry::GetModelData();
    return static_cast<NiTriBasedGeomData*>(modelData)->GetActiveTriangleCount();
}

uint16_t NiTriBasedGeom::GetTriCount() {
    NiGeometryData* v2 = m_spModelData;
    return static_cast<NiTriBasedGeomData*>(v2)->GetTriCount();
}
