#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLinesData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiLines::ms_RTTI("NiLines", &NiGeometry::ms_RTTI);

// No model data: a stream load sets it.
NiLines::NiLines() {}

NiLines::NiLines(
    int32_t ausVertices,
    NiPoint3* apVertex,
    NiColorA* apColor,
    NiPoint2* apTexture,
    int32_t ausNumTextureSets,
    NiGeometryData::DataFlags aeNBTMethod,
    uint8_t* apFlags
)
  : NiGeometry(new NiLinesData(ausVertices, apVertex, apColor, apTexture, ausNumTextureSets, aeNBTMethod, apFlags)) {}

NiLines::~NiLines() {}

NiObject* NiLines::CreateClone(NiCloningProcess& kCloning) {
    NiLines* pkClone = new NiLines;
    NiGeometry::CopyMembers(pkClone, kCloning);
    return pkClone;
}

// Other geometry classes without stream data of their own use the same function.
void NiLines::LoadBinary(NiStream& kStream) {
    NiGeometry::LoadBinary(kStream);
}

// Other geometry classes without links of their own use the same function.
void NiLines::LinkObject(NiStream& kStream) {
    NiGeometry::LinkObject(kStream);
}

// Other geometry classes without streamables of their own use the same function.
bool NiLines::RegisterStreamables(NiStream& kStream) {
    return NiGeometry::RegisterStreamables(kStream);
}

// Other geometry classes without stream data of their own use the same function.
void NiLines::SaveBinary(NiStream& kStream) {
    NiGeometry::SaveBinary(kStream);
}

// Other geometry classes without members of their own use the same function.
bool NiLines::IsEqual(NiObject* pkObject) {
    return NiGeometry::IsEqual(pkObject);
}

void NiLines::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiGeometry::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// Draws the lines when the renderer is inside a frame with a render target group set.
void NiLines::RenderImmediate(NiRenderer* apkRenderer) {
    NiGeometry::RenderImmediate(apkRenderer);
    NiRenderer::EFrameState eFrameState = apkRenderer->m_eFrameState;
    if ((eFrameState == NiRenderer::FRAMESTATE_INSIDE_FRAME ||
         eFrameState == NiRenderer::FRAMESTATE_INSIDE_OFFSCREEN_FRAME) &&
        apkRenderer->m_bRenderTargetGroupActive) {
        static_cast<NiDX9Renderer*>(apkRenderer)->Do_RenderLines(this);
    }
}

NiObject* NiLines::CreateObject() {
    return new NiLines;
}
