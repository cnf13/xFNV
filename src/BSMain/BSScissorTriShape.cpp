#include "BSMain/BSScissorTriShape.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSResizableTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"

NiRTTI BSScissorTriShape::ms_RTTI("BSScissorTriShape", &NiTriShape::ms_RTTI);

// The default constructor deliberately leaves the scissor rectangle unset, as the game does.
BSScissorTriShape::BSScissorTriShape() {}

BSScissorTriShape::BSScissorTriShape(
    uint16_t usVertices,
    NiPoint3* pkVertex,
    NiPoint3* pkNormal,
    NiColorA* pkColor,
    NiPoint2* pkTexture,
    uint16_t usNumTextureSets,
    NiGeometryData::DataFlags eNBTMethod,
    uint16_t usTriangles,
    uint16_t* pusTriList,
    int32_t aiLeft,
    int32_t aiTop,
    int32_t aiRight,
    int32_t aiBottom
)
  : NiTriShape(
        usVertices,
        pkVertex,
        pkNormal,
        pkColor,
        pkTexture,
        usNumTextureSets,
        eNBTMethod,
        usTriangles,
        pusTriList
    ),
    kRect{aiLeft, aiTop, aiRight, aiBottom} {}

BSScissorTriShape::BSScissorTriShape(int32_t a1, int32_t a2, int32_t a3, int32_t a4) : kRect{a1, a2, a3, a4} {}

BSScissorTriShape::~BSScissorTriShape() {}

void BSScissorTriShape::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiTriShape::GetViewerStrings(pkStrings);
}

const NiRTTI* BSScissorTriShape::GetRTTI() const {
    return &BSScissorTriShape::ms_RTTI;
}

NiObject* BSScissorTriShape::CreateClone(NiCloningProcess& arCloneData) {
    BSScissorTriShape* pNode;
    BSScissorTriShape* v5 = static_cast<BSScissorTriShape*>(NiMemObject::operator new(sizeof(BSScissorTriShape)));
    if (v5) {
        pNode = ::new (v5) BSScissorTriShape;
    }
    else {
        pNode = nullptr;
    }
    CopyMembers(pNode, arCloneData);
    return pNode;
}

// The renderer is the DX9 renderer: SetRenderState and SetScissorRect are called on its device through the COM
// interface, reloading m_pkD3DDevice9 at each call.
void BSScissorTriShape::RenderImmediate(NiRenderer* pkRenderer) {
    NiDX9Renderer* pkDX9Renderer = static_cast<NiDX9Renderer*>(pkRenderer);
    bool bScissorTestSet = false;
    if ((kRect.bottom || (kRect.top || (kRect.left || kRect.right))) &&
        ((kRect.left < 0 && kRect.right < 0) ||
         ((kRect.left > 1 && kRect.right > 1) ||
          ((kRect.top < 0 && kRect.bottom < 0) || (kRect.top > 1 && kRect.bottom > 1))))) {
        pkDX9Renderer->m_pkD3DDevice9->SetRenderState(D3DRS_SCISSORTESTENABLE, 1);
        pkDX9Renderer->m_pkD3DDevice9->SetScissorRect(&kRect);
        bScissorTestSet = true;
    }
    NiTriShape::RenderImmediate(pkRenderer);
    if (bScissorTestSet) {
        pkDX9Renderer->m_pkD3DDevice9->SetRenderState(D3DRS_SCISSORTESTENABLE, 0);
    }
}

void BSScissorTriShape::SetScissorRect(int32_t aiLeft, int32_t aiTop, int32_t aiRight, int32_t aiBottom) {
    kRect.left = aiLeft;
    kRect.top = aiTop;
    kRect.right = aiRight;
    kRect.bottom = aiBottom;
}

// Calls BSResizableTriShape::CopyMembers on the same object, a class this one does not derive from, as the game does;
// its body only touches NiTriShape's members.
void BSScissorTriShape::CopyMembers(BSScissorTriShape* pDest, NiCloningProcess& arCloneData) {
    pDest->kRect = kRect;
    static_cast<BSResizableTriShape*>(static_cast<NiTriShape*>(this))
        ->CopyMembers(static_cast<BSResizableTriShape*>(static_cast<NiTriShape*>(pDest)), arCloneData);
}

NiTriShape::~NiTriShape() {}
