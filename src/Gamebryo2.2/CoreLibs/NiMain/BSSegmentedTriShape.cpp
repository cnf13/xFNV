#include "Gamebryo2.2/CoreLibs/NiMain/BSSegmentedTriShape.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSResizableTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeomData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cstdint>

NiRTTI BSSegmentedTriShape::ms_RTTI("BSSegmentedTriShape", &NiTriShape::ms_RTTI);

// bIgnoreSegments is deliberately left unset, as the game does.
BSSegmentedTriShape::BSSegmentedTriShape()
  : NiTriShape(), pSegments(nullptr), iNumSegments(0), bSegmentsChanged(false) {}

// The segment table comes from Bethesda's heap; it is freed without a null test, as the game does.
BSSegmentedTriShape::~BSSegmentedTriShape() {
    MemoryManager_Singleton.Deallocate(pSegments);
}

const NiRTTI* BSSegmentedTriShape::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* BSSegmentedTriShape::CreateClone(NiCloningProcess& kCloning) {
    void* pkMemory = NiMemObject::operator new(sizeof(BSSegmentedTriShape));
    if (pkMemory) {
        BSSegmentedTriShape* pkObject = ::new (pkMemory) BSSegmentedTriShape;
        CopyMembers(pkObject, kCloning);
        return pkObject;
    }

    CopyMembers(nullptr, kCloning);
    return nullptr;
}

// Reads the segments; empty ones start at index 0. Every segment loads disabled, its flag byte is skipped.
void BSSegmentedTriShape::LoadBinary(NiStream& kStream) {
    NiTriShape::LoadBinary(kStream);
    if (pSegments) {
        MemoryManager_Singleton.Deallocate(pSegments);
    }
    NiStreamLoadBinary(kStream, iNumSegments);
    uint64_t ullSize = uint64_t{sizeof(Segment)} * iNumSegments;
    pSegments = static_cast<Segment*>(
        MemoryManager_Singleton.Allocate(ullSize >> 32 ? SIZE_MAX : static_cast<size_t>(ullSize))
    );
    for (uint32_t i = 0; i < iNumSegments; i++) {
        uint8_t ucFlags;
        NiStreamLoadBinary(kStream, ucFlags);
        NiStreamLoadBinary(kStream, pSegments[i].iStartIndex);
        NiStreamLoadBinary(kStream, pSegments[i].iNumPrimitives);
        pSegments[i].bEnabled = false;
        Segment& rkSegment = pSegments[i];
        if (rkSegment.iNumPrimitives) {
            rkSegment.iNumPrimitivesMerged = rkSegment.iNumPrimitives;
        }
        else {
            rkSegment.iNumPrimitivesMerged = 0;
            pSegments[i].iStartIndex = 0;
        }
    }
    bSegmentsChanged = true;
}

void BSSegmentedTriShape::LinkObject(NiStream& kStream) {
    NiTriShape::LinkObject(kStream);
}

bool BSSegmentedTriShape::RegisterStreamables(NiStream& kStream) {
    return NiTriShape::RegisterStreamables(kStream);
}

// Writes each segment with a zero flag byte.
void BSSegmentedTriShape::SaveBinary(NiStream& kStream) {
    NiTriShape::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, iNumSegments);
    for (uint32_t i = 0; i < iNumSegments; i++) {
        uint8_t ucFlags = 0;
        NiStreamSaveBinary(kStream, ucFlags);
        NiStreamSaveBinary(kStream, pSegments[i].iStartIndex);
        NiStreamSaveBinary(kStream, pSegments[i].iNumPrimitives);
    }
}

bool BSSegmentedTriShape::IsEqual(NiObject* pkObject) {
    return NiTriShape::IsEqual(pkObject);
}

void BSSegmentedTriShape::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiTriShape::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("Num Segments", iNumSegments));
    if (!pSegments) {
        return;
    }
    uint32_t uiEmpty = 0;
    for (uint32_t i = 0; i < iNumSegments; i++) {
        if (pSegments[i].iNumPrimitives) {
            pkStrings->Add(NiGetViewerString(pSegments[i].bEnabled ? "Enabled Segment" : "Disabled Segment", i));
        }
        else {
            uiEmpty++;
        }
    }
    pkStrings->Add(NiGetViewerString("Empty Segments", uiEmpty));
}

void BSSegmentedTriShape::RenderImmediate(NiRenderer* pkRenderer) {
    MergeEnabledSegments();
    NiTriShape::RenderImmediate(pkRenderer);
}

void BSSegmentedTriShape::OnlyRenderImmediate(NiRenderer* pkRenderer) {
    MergeEnabledSegments();
    NiTriShape::OnlyRenderImmediate(pkRenderer);
}

// Walks the segments from the second-to-last down, reloading pSegments after each store into a segment. The bool fields
// are tested as whole bytes and bIsEnabled is copied into bEnabledPostMerge raw, so they are read as bytes.
void BSSegmentedTriShape::MergeEnabledSegments() {
    if (!reinterpret_cast<const uint8_t&>(bSegmentsChanged)) {
        return;
    }
    Segment* pLast = &pSegments[iNumSegments - 1];
    uint32_t uiNumPrimitives = pLast->iNumPrimitives;
    if (uiNumPrimitives) {
        pLast->iNumPrimitivesMerged = uiNumPrimitives;
        reinterpret_cast<uint8_t&>(pLast->bEnabledPostMerge) = reinterpret_cast<const uint8_t&>(pLast->bEnabled);
    }
    else {
        pLast->bEnabledPostMerge = false;
    }
    for (int32_t i = static_cast<int32_t>(iNumSegments - 2); i >= 0; --i) {
        Segment* pSegment = &pSegments[i];
        uint8_t ucEnabled = reinterpret_cast<const uint8_t&>(pSegment->bEnabled);
        if (!ucEnabled || !reinterpret_cast<const uint8_t&>(pSegment[1].bEnabledPostMerge)) {
            if (pSegment->iNumPrimitives) {
                reinterpret_cast<uint8_t&>(pSegment->bEnabledPostMerge) = ucEnabled;
                pSegments[i].iNumPrimitivesMerged = pSegments[i].iNumPrimitives;
                continue;
            }
            if (!reinterpret_cast<const uint8_t&>(pSegment[1].bEnabledPostMerge)) {
                pSegment->bEnabledPostMerge = false;
                continue;
            }
        }
        // Merge this segment with the next one, which is drawn through it.
        pSegment[1].bEnabledPostMerge = false;
        pSegments[i].bEnabledPostMerge = true;
        Segment* pMerged = pSegments;
        pMerged[i].iNumPrimitivesMerged = pMerged[i].iNumPrimitives + pMerged[i + 1].iNumPrimitivesMerged;
        pMerged = &pSegments[i];
        if (!pMerged->iNumPrimitives) {
            pMerged->iStartIndex = pMerged[1].iStartIndex;
        }
    }
    bSegmentsChanged = false;
}

uint32_t BSSegmentedTriShape::GetNumSegments() {
    if (bIgnoreSegments) {
        return (uint32_t)1;
    }

    return (uint32_t)((int32_t)iNumSegments);
}

bool BSSegmentedTriShape::GetSegmentEmpty(uint32_t aiSegment) {
    return (pSegments[aiSegment].iNumPrimitives == 0);
}

void BSSegmentedTriShape::EnableSegment(uint32_t aiSegment) {
    pSegments[aiSegment].bEnabled = (pSegments[aiSegment].iNumPrimitives != 0);
    bSegmentsChanged = true;
}

void BSSegmentedTriShape::DisableSegment(uint32_t aiSegment) {
    pSegments[aiSegment].bEnabled = false;
    bSegmentsChanged = true;
}

// The model data is a NiTriBasedGeomData (NiTriBasedGeom casts it so); its triangle count is read directly.
void BSSegmentedTriShape::EnableAllSegments() {
    pSegments->bEnabledPostMerge = true;
    pSegments->iNumPrimitivesMerged = static_cast<NiTriBasedGeomData*>(m_spModelData.m_pObject)->m_usTriangles;
    uint32_t v1 = 1;
    if (iNumSegments > 1) {
        int32_t v2 = 1;
        do {
            pSegments[v2].bEnabledPostMerge = false;
            BSSegmentedTriShape::Segment* v3 = &pSegments[v2];
            if (v3->iNumPrimitives) {
                v3->bEnabled = true;
            }
            ++v1;
            ++v2;
        }
        while (v1 < iNumSegments);
    }
    bSegmentsChanged = false;
}

void BSSegmentedTriShape::CopyMembers(BSSegmentedTriShape* pkDest, NiCloningProcess& kCloning) {
    // Deliberately calls BSResizableTriShape::CopyMembers on this object, as the game does, although this class does
    // not derive from BSResizableTriShape; that function only calls NiTriShape::CopyMembers.
    static_cast<BSResizableTriShape*>(static_cast<NiTriShape*>(this))
        ->CopyMembers(static_cast<BSResizableTriShape*>(static_cast<NiTriShape*>(pkDest)), kCloning);
    if (pkDest->pSegments) {
        MemoryManager_Singleton.Deallocate(pkDest->pSegments);
    }
    if (pSegments) {
        pkDest->iNumSegments = iNumSegments;
        pkDest->bSegmentsChanged = bSegmentsChanged;
        pkDest->pSegments = static_cast<Segment*>(MemoryManager_Singleton.Allocate(
            iNumSegments <= SIZE_MAX / sizeof(Segment) ? sizeof(Segment) * iNumSegments : SIZE_MAX
        ));
        for (uint32_t i = 0; i < iNumSegments; ++i) {
            pkDest->pSegments[i] = pSegments[i];
        }
    }
    else {
        pkDest->pSegments = nullptr;
        pkDest->iNumSegments = 0;
    }
}

void BSSegmentedTriShape::SetSystemDisabled(bool abDisable) {
    BSSegmentedTriShape::bSystemDisabled = (bool)((int8_t)abDisable);
}

NiObject* BSSegmentedTriShape::CreateObject() {
    return new BSSegmentedTriShape;
}
