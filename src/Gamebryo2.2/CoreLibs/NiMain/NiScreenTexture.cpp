#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenTexture.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI NiScreenTexture::ms_RTTI("NiScreenTexture", &NiObject::ms_RTTI);

NiScreenTexture::NiScreenTexture() : m_spTexProp(nullptr), m_usDirtyFlags(0), m_pkBuffData(nullptr) {}

// The texturing property and the rectangle set go with their members' destructors.
NiScreenTexture::~NiScreenTexture() {
    if (NiRenderer::ms_pkRenderer) {
        NiRenderer::ms_pkRenderer->PurgeScreenTexture(this);
    }
}

const NiRTTI* NiScreenTexture::GetRTTI() const {
    return &ms_RTTI;
}

// CopyMembers is called with a null clone when the allocation fails, as the game does.
NiObject* NiScreenTexture::CreateClone(NiCloningProcess& kCloning) {
    NiScreenTexture* pkObject = static_cast<NiScreenTexture*>(NiMemObject::operator new(sizeof(NiScreenTexture)));
    if (pkObject) {
        ::new (pkObject) NiScreenTexture;
        CopyMembers(pkObject, kCloning);
        return pkObject;
    }

    CopyMembers(nullptr, kCloning);
    return nullptr;
}

// Each rectangle is six 2-byte transfers and its color; the texturing property link is queued for LinkObject.
// KEEP: the rectangle count and the six 2-byte fields are read into locals nothing initialised (only the color is
// zeroed, by NiColorA's constructor), so a short read (malformed data) drives the loop with, and stores, stack garbage.
void NiScreenTexture::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);
    uint32_t uiNumRects;
    NiStreamLoadBinary(kStream, uiNumRects);
    for (uint32_t i = 0; i < uiNumRects; i++) {
        ScreenRect kRect;
        NiStreamLoadBinary(kStream, kRect.m_sPixTop);
        NiStreamLoadBinary(kStream, kRect.m_sPixLeft);
        NiStreamLoadBinary(kStream, kRect.m_usPixWidth);
        NiStreamLoadBinary(kStream, kRect.m_usPixHeight);
        NiStreamLoadBinary(kStream, kRect.m_usTexTop);
        NiStreamLoadBinary(kStream, kRect.m_usTexLeft);
        kRect.m_kColor.LoadBinary(kStream);
        m_kScreenRects.Add(kRect);
    }
    kStream.ReadLinkID();
}

// The linked object is taken as the texturing property without a type check.
void NiScreenTexture::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);
    m_spTexProp = static_cast<NiTexturingProperty*>(kStream.GetObjectFromLinkID());
}

// The texturing property is registered without a null check.
bool NiScreenTexture::RegisterStreamables(NiStream& kStream) {
    if (!NiObject::RegisterStreamables(kStream)) {
        return false;
    }

    m_spTexProp->RegisterStreamables(kStream);
    return true;
}

void NiScreenTexture::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);
    uint32_t uiNumRects = m_kScreenRects.m_uiUsed;
    NiStreamSaveBinary(kStream, uiNumRects);
    for (uint32_t i = 0; i < uiNumRects; i++) {
        ScreenRect& rkRect = m_kScreenRects.m_pBase[i];
        NiStreamSaveBinary(kStream, rkRect.m_sPixTop);
        NiStreamSaveBinary(kStream, rkRect.m_sPixLeft);
        NiStreamSaveBinary(kStream, rkRect.m_usPixWidth);
        NiStreamSaveBinary(kStream, rkRect.m_usPixHeight);
        NiStreamSaveBinary(kStream, rkRect.m_usTexTop);
        NiStreamSaveBinary(kStream, rkRect.m_usTexLeft);
        NiColorA kColor = rkRect.m_kColor;
        kColor.SaveBinary(kStream);
    }
    kStream.SaveLinkID(m_spTexProp);
}

// A rectangle's color is compared only when all six of its position and size fields differ, as the game does, so
// rectangles sharing any field never make the textures unequal. The texturing properties are deliberately compared
// without a null check.
bool NiScreenTexture::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }

    NiScreenTexture* pkOther = static_cast<NiScreenTexture*>(pkObject);
    uint32_t uiNumRects = m_kScreenRects.m_uiUsed;
    if (uiNumRects != pkOther->m_kScreenRects.m_uiUsed) {
        return false;
    }

    for (uint32_t i = 0; i < uiNumRects; i++) {
        const ScreenRect& rkA = m_kScreenRects.m_pBase[i];
        const ScreenRect& rkB = pkOther->m_kScreenRects.m_pBase[i];
        if (rkA.m_sPixTop != rkB.m_sPixTop && rkA.m_sPixLeft != rkB.m_sPixLeft &&
            rkA.m_usPixWidth != rkB.m_usPixWidth && rkA.m_usPixHeight != rkB.m_usPixHeight &&
            rkA.m_usTexTop != rkB.m_usTexTop && rkA.m_usTexLeft != rkB.m_usTexLeft && rkA.m_kColor != rkB.m_kColor) {
            return false;
        }
    }

    return m_spTexProp->IsEqual(pkOther->m_spTexProp);
}

void NiScreenTexture::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiObject::GetViewerStrings(pStrings);
}

void NiScreenTexture::CopyMembers(NiScreenTexture* pkDest, NiCloningProcess& kCloning) {
    NiObject::CopyMembers(pkDest, kCloning);

    for (uint32_t i = 0; i < m_kScreenRects.GetSize(); i++) {
        ScreenRect kNewRect;
        const ScreenRect& kRect = m_kScreenRects.GetAt(i);

        kNewRect.m_sPixTop = kRect.m_sPixTop;
        kNewRect.m_sPixLeft = kRect.m_sPixLeft;
        kNewRect.m_usPixWidth = kRect.m_usPixWidth;
        kNewRect.m_usPixHeight = kRect.m_usPixHeight;
        kNewRect.m_usTexTop = kRect.m_usTexTop;
        kNewRect.m_usTexLeft = kRect.m_usTexLeft;
        kNewRect.m_kColor = kRect.m_kColor;

        // EXE-INLINED none: NiTSet::Add. The exe copies the rect into its slot (rep movsd) before it counts it
        // (0xA95828, 0xA9582A); NiTSet::Add's m_pBase[m_uiUsed++] stores the count first.
        NiTPrimitiveSet<ScreenRect>& kDestRects = pkDest->m_kScreenRects;
        if (kDestRects.m_uiUsed == kDestRects.m_uiAlloced) {
            kDestRects.Realloc();
        }
        kDestRects.m_pBase[kDestRects.m_uiUsed] = kNewRect;
        kDestRects.m_uiUsed++;
    }

    pkDest->m_spTexProp = m_spTexProp;
    pkDest->m_usDirtyFlags = m_usDirtyFlags;
}

NiObject* NiScreenTexture::CreateObject() {
    void* pkObject = NiMemObject::operator new(sizeof(NiScreenTexture));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiScreenTexture;
}
