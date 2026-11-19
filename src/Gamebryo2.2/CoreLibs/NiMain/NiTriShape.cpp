#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeomData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiTriShape::ms_RTTI("NiTriShape", &NiTriBasedGeom::ms_RTTI);

NiTriShape::NiTriShape() {}

// The shape owns new shape data built over the given arrays.
NiTriShape::NiTriShape(
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
  : NiTriBasedGeom(new NiTriShapeData(
        usVertices,
        pkVertex,
        pkNormal,
        pkColor,
        pkTexture,
        usNumTextureSets,
        eNBTMethod,
        usTriangles,
        pusTriList
    )) {}

NiTriShape::NiTriShape(NiTriShapeData* apModelData) : NiTriBasedGeom(apModelData) {}

const NiRTTI* NiTriShape::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiTriShape::CreateClone(NiCloningProcess& kCloning) {
    NiTriShape* pkClone = new NiTriShape;
    CopyMembers(pkClone, kCloning);
    return pkClone;
}

// NiTriStrips uses the same function.
void NiTriShape::LoadBinary(NiStream& kStream) {
    NiTriBasedGeom::LoadBinary(kStream);
}

// NiTriStrips uses the same function.
void NiTriShape::LinkObject(NiStream& kStream) {
    NiTriBasedGeom::LinkObject(kStream);
}

// NiTriStrips uses the same function.
bool NiTriShape::RegisterStreamables(NiStream& kStream) {
    return NiTriBasedGeom::RegisterStreamables(kStream);
}

// NiTriStrips uses the same function.
void NiTriShape::SaveBinary(NiStream& kStream) {
    NiTriBasedGeom::SaveBinary(kStream);
}

// NiTriStrips uses the same function.
bool NiTriShape::IsEqual(NiObject* pkObject) {
    return NiTriBasedGeom::IsEqual(pkObject);
}

void NiTriShape::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiTriBasedGeom::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// Draws the shape when the renderer is inside a frame with a render target group set.
void NiTriShape::RenderImmediate(NiRenderer* apkRenderer) {
    NiGeometry::RenderImmediate(apkRenderer);
    NiRenderer::EFrameState eFrameState = apkRenderer->m_eFrameState;
    if ((eFrameState == NiRenderer::FRAMESTATE_INSIDE_FRAME ||
         eFrameState == NiRenderer::FRAMESTATE_INSIDE_OFFSCREEN_FRAME) &&
        apkRenderer->m_bRenderTargetGroupActive) {
        static_cast<NiDX9Renderer*>(apkRenderer)->Do_RenderShape(this);
    }
}

// As RenderImmediate, but draws without setting up the render state. The base call is deliberately
// NiGeometry::RenderImmediate, as the game does.
void NiTriShape::OnlyRenderImmediate(NiRenderer* apkRenderer) {
    NiGeometry::RenderImmediate(apkRenderer);
    NiRenderer::EFrameState eFrameState = apkRenderer->m_eFrameState;
    if ((eFrameState == NiRenderer::FRAMESTATE_INSIDE_FRAME ||
         eFrameState == NiRenderer::FRAMESTATE_INSIDE_OFFSCREEN_FRAME) &&
        apkRenderer->m_bRenderTargetGroupActive) {
        static_cast<NiDX9Renderer*>(apkRenderer)->Do_OnlyRenderShape(this);
    }
}

void NiTriShape::GetModelTriangle(uint16_t usTriangle, NiPoint3*& pkP0, NiPoint3*& pkP1, NiPoint3*& pkP2) {
    NiTriShapeData* pkData = static_cast<NiTriShapeData*>(static_cast<NiGeometryData*>(m_spModelData));
    NiPoint3* pkVertex = pkData->m_pkVertex;
    uint16_t* pusTriList = pkData->GetTriList();
    // The index is kept in 16 bits: 3 * usTriangle and each step past it wrap at 0x10000, as the game does.
    uint16_t usIndex = static_cast<uint16_t>(3 * usTriangle);
    pkP0 = &pkVertex[pusTriList[usIndex]];
    usIndex = static_cast<uint16_t>(usIndex + 1);
    pkP1 = &pkVertex[pusTriList[usIndex]];
    usIndex = static_cast<uint16_t>(usIndex + 1);
    pkP2 = &pkVertex[pusTriList[usIndex]];
}

void NiTriShape::CopyMembers(NiTriShape* pkDest, NiCloningProcess& kCloning) {
    NiGeometry::CopyMembers(pkDest, kCloning);
}

NiTriShape* NiTriShape::Create(NiTriShapeData* a1) {
    return new NiTriShape(a1);
}

NiObject* NiTriShape::CreateObject() {
    return new NiTriShape;
}
