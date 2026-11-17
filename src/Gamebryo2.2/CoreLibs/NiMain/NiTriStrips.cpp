#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeomData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiTriStrips::ms_RTTI("NiTriStrips", &NiTriBasedGeom::ms_RTTI);

NiTriStrips::NiTriStrips() {}

NiTriStrips::NiTriStrips(NiTriStripsData* a1) : NiTriBasedGeom(a1) {}

// The strip mesh owns new strips data built over the given arrays.
NiTriStrips::NiTriStrips(
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
  : NiTriBasedGeom(new NiTriStripsData(
        usVertices,
        pkVertex,
        pkNormal,
        pkColor,
        pkTexture,
        usNumTextureSets,
        eNBTMethod,
        usTriangles,
        usStrips,
        pusStripLengths,
        pusStripLists
    )) {}

// Nothing of its own to release: the model data goes with NiGeometry's destructor.
NiTriStrips::~NiTriStrips() {}

const NiRTTI* NiTriStrips::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiTriStrips::CreateClone(NiCloningProcess& kCloning) {
    NiTriStrips* pkObject = static_cast<NiTriStrips*>(NiMemObject::operator new(sizeof(NiTriStrips)));
    if (pkObject) {
        ::new (pkObject) NiTriStrips;
    }
    CopyMembers(pkObject, kCloning);
    return pkObject;
}

void NiTriStrips::LoadBinary(NiStream& kStream) {
    NiTriBasedGeom::LoadBinary(kStream);
}

void NiTriStrips::LinkObject(NiStream& kStream) {
    NiTriBasedGeom::LinkObject(kStream);
}

bool NiTriStrips::RegisterStreamables(NiStream& kStream) {
    return NiTriBasedGeom::RegisterStreamables(kStream);
}

void NiTriStrips::SaveBinary(NiStream& kStream) {
    NiTriBasedGeom::SaveBinary(kStream);
}

bool NiTriStrips::IsEqual(NiObject* pObject) {
    return NiTriBasedGeom::IsEqual(pObject);
}

void NiTriStrips::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiTriBasedGeom::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// Draws the strips when the renderer is inside a frame with a render target group set.
void NiTriStrips::RenderImmediate(NiRenderer* apkRenderer) {
    NiGeometry::RenderImmediate(apkRenderer);
    NiRenderer::EFrameState eFrameState = apkRenderer->m_eFrameState;
    if ((eFrameState == NiRenderer::FRAMESTATE_INSIDE_FRAME ||
         eFrameState == NiRenderer::FRAMESTATE_INSIDE_OFFSCREEN_FRAME) &&
        apkRenderer->m_bRenderTargetGroupActive) {
        static_cast<NiDX9Renderer*>(apkRenderer)->Do_RenderTristrips(this);
    }
}

// As RenderImmediate, but draws without setting up the render state. The base call is deliberately RenderImmediate, as
// the game does.
void NiTriStrips::OnlyRenderImmediate(NiRenderer* apkRenderer) {
    NiGeometry::RenderImmediate(apkRenderer);
    NiRenderer::EFrameState eFrameState = apkRenderer->m_eFrameState;
    if ((eFrameState == NiRenderer::FRAMESTATE_INSIDE_FRAME ||
         eFrameState == NiRenderer::FRAMESTATE_INSIDE_OFFSCREEN_FRAME) &&
        apkRenderer->m_bRenderTargetGroupActive) {
        static_cast<NiDX9Renderer*>(apkRenderer)->Do_OnlyRenderTristrips(this);
    }
}

void NiTriStrips::GetModelTriangle(uint16_t usTriangle, NiPoint3*& pkP0, NiPoint3*& pkP1, NiPoint3*& pkP2) {
    NiTriBasedGeomData* pkData = static_cast<NiTriBasedGeomData*>(static_cast<NiGeometryData*>(m_spModelData));
    NiPoint3* pkVertex = pkData->m_pkVertex;
    uint16_t usI0;
    uint16_t usI1;
    uint16_t usI2;
    pkData->GetTriangleIndices(usTriangle, usI0, usI1, usI2);
    pkP0 = &pkVertex[usI0];
    pkP1 = &pkVertex[usI1];
    pkP2 = &pkVertex[usI2];
}

void NiTriStrips::CopyMembers(NiTriStrips* pkDest, NiCloningProcess& kCloning) {
    NiGeometry::CopyMembers(pkDest, kCloning);
}

NiObject* NiTriStrips::CreateObject() {
    NiTriStrips* pkObject = static_cast<NiTriStrips*>(NiMemObject::operator new(sizeof(NiTriStrips)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiTriStrips;
}
