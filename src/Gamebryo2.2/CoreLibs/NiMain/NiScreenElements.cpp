#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"

#include "Gamebryo2.2/CoreLibs/NiMain/BSResizableTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElementsData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiScreenElements::ms_RTTI("NiScreenElements", &NiTriShape::ms_RTTI);

NiScreenElements::NiScreenElements() {}

NiScreenElements::NiScreenElements(NiScreenElementsData* pkModelData) : NiTriShape(pkModelData) {}

NiScreenElements::~NiScreenElements() {}

const NiRTTI* NiScreenElements::GetRTTI() const {
    return &ms_RTTI;
}

// CopyMembers is called on this object through BSResizableTriShape::CopyMembers, a class it does not derive from; that
// body only calls NiTriShape::CopyMembers. It is deliberately called with a null clone when the allocation fails.
NiObject* NiScreenElements::CreateClone(NiCloningProcess& kCloning) {
    NiScreenElements* pkClone = new NiScreenElements;
    static_cast<BSResizableTriShape*>(static_cast<NiTriShape*>(this))
        ->CopyMembers(static_cast<BSResizableTriShape*>(static_cast<NiTriShape*>(pkClone)), kCloning);
    return pkClone;
}

void NiScreenElements::LoadBinary(NiStream& kStream) {
    NiTriShape::LoadBinary(kStream);
}

void NiScreenElements::LinkObject(NiStream& kStream) {
    NiTriShape::LinkObject(kStream);
}

bool NiScreenElements::RegisterStreamables(NiStream& kStream) {
    return NiTriShape::RegisterStreamables(kStream);
}

void NiScreenElements::SaveBinary(NiStream& kStream) {
    NiTriShape::SaveBinary(kStream);
}

bool NiScreenElements::IsEqual(NiObject* pkObject) {
    return NiTriShape::IsEqual(pkObject);
}

void NiScreenElements::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiTriShape::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

int32_t NiScreenElements::Insert(uint16_t usNumVertices, uint16_t usNumTriangles, const uint16_t* ausTriList) {
    NiGeometryData* modelData = NiGeometry::GetModelData();
    return static_cast<NiScreenElementsData*>(modelData)->Insert(usNumVertices, usNumTriangles, ausTriList);
}

bool NiScreenElements::SetRectangle(int32_t iPolygon, float fLeft, float fTop, float fWidth, float fHeight) {
    NiGeometryData* modelData = NiGeometry::GetModelData();
    return static_cast<NiScreenElementsData*>(modelData)->SetRectangle(iPolygon, fLeft, fTop, fWidth, fHeight);
}

void NiScreenElements::UpdateBound() {
    NiGeometryData* modelData = NiGeometry::GetModelData();
    static_cast<NiScreenElementsData*>(modelData)->UpdateBound();
}

void NiScreenElements::Draw(NiRenderer* pkRenderer) {
    RenderImmediate(pkRenderer);
}

NiObject* NiScreenElements::CreateObject() {
    return new NiScreenElements;
}
