#include "Fallout Shared/MultiBoundMarkerData.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundAABB.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSMultiBoundOBB.h"
#include "BSMain/BSMultiBoundRoom.h"
#include "BSMain/BSMultiBoundShape.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTAbstractPoolAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"

#include <cmath>

MultiBoundMarkerData::MultiBoundMarkerData() : HalfExtents(MultiBoundMarkerData::DefHalfExtents) {}

const NiPoint3& MultiBoundMarkerData::GetHalfExtents() {
    return HalfExtents;
}

// Reads the XMBO chunk (the half extents) of apFile's current record; byte swaps it from a big-endian file. False
// when there is no file or the chunk is not XMBO.
bool MultiBoundMarkerData::Load(TESFile* apFile) {
    bool bLoaded = false;
    if (apFile && apFile->GetTESChunk() == XMBO_ID) {
        apFile->GetChunkData(this, 0xC);
        if (apFile->QEndian()) {
            ENDIAN(HalfExtents.x, false);
            ENDIAN(HalfExtents.y, false);
            ENDIAN(HalfExtents.z, false);
        }
        bLoaded = true;
    }
    return bLoaded;
}

void MultiBoundMarkerData::Copy(MultiBoundMarkerData* apCopy) {
    if (apCopy) {
        *this = *apCopy;
    }
}

void MultiBoundMarkerData::Save() {
    if (!IsDefault()) {
        TESForm::AddChunkArray(XMBO_ID, &HalfExtents.x, 3u);
    }
}

bool MultiBoundMarkerData::Compare(MultiBoundMarkerData* apCompare) {
    return !apCompare || HalfExtents != apCompare->HalfExtents;
}

bool MultiBoundMarkerData::IsDefault() {
    return !(HalfExtents != DefHalfExtents);
}

BSMultiBound* MultiBoundMarkerData::CreateMultiBoundForRef(TESObjectREFR* apRef) {
    NiMatrix3 kOrientation;
    BSMultiBound* pMultiBound = nullptr;
    if (apRef) {
        BSMultiBoundAABB* pShape = nullptr;
        if (!(std::fabs(apRef->GetAngleOnReference().x) < 0.009999999776482582) ||
            !(std::fabs(apRef->GetAngleOnReference().y) < 0.009999999776482582) ||
            !(std::fabs(apRef->GetAngleOnReference().z) < 0.009999999776482582)) {
            pShape = new BSMultiBoundOBB;
            kOrientation = apRef->GetOrientation();
            static_cast<BSMultiBoundOBB*>(pShape)->SetAxis(kOrientation);
            pShape->SetHalfExtents(apRef->GetMultiBoundHalfExtent());
        }
        else {
            pShape = new BSMultiBoundAABB;
            pShape->SetHalfExtents(apRef->GetMultiBoundHalfExtent());
        }
        const NiPoint3* pCenter = &apRef->GetLocationOnReference();
        pShape->SetCenter(*pCenter);
        pMultiBound = new BSMultiBound;
        pMultiBound->SetShape(pShape);
    }
    return pMultiBound;
}

BSMultiBoundRoom* MultiBoundMarkerData::CreateRoomForRef(TESObjectREFR* apRef) {
    BSMultiBoundRoom* pBound = nullptr;
    if (apRef) {
        BSMultiBoundShape* pShape = nullptr;
        if (!(std::fabs(apRef->GetAngleOnReference().x) < 0.009999999776482582) ||
            !(std::fabs(apRef->GetAngleOnReference().y) < 0.009999999776482582) ||
            !(std::fabs(apRef->GetAngleOnReference().z) < 0.009999999776482582)) {
            pShape = new BSMultiBoundOBB;
            static_cast<BSMultiBoundOBB*>(pShape)->BSMultiBoundOBB::SetAxis(apRef->GetOrientation());
            static_cast<BSMultiBoundAABB*>(pShape)->BSMultiBoundAABB::SetHalfExtents(apRef->GetMultiBoundHalfExtent());
        }
        else {
            pShape = new BSMultiBoundAABB;
            static_cast<BSMultiBoundAABB*>(pShape)->BSMultiBoundAABB::SetHalfExtents(apRef->GetMultiBoundHalfExtent());
        }
        const NiPoint3* pCenter = &apRef->GetLocationOnReference();
        pShape->SetCenter(*pCenter);
        pBound = new BSMultiBoundRoom;
        BSMultiBound* pMultiBound = new BSMultiBound;
        pMultiBound->SetShape(pShape);
        pBound->SetMultiBound(pMultiBound);
    }
    return pBound;
}

bool MultiBoundMarkerData::MultiBoundIntersectsCell(TESObjectREFR* apRef, int32_t aiCellX, int32_t aiCellY) {
    if (apRef && apRef->IsMultiBoundRef()) {
        NiPoint3 kY;
        NiPoint3 kZ;
        NiPoint3 boundverts[8];
        tagRECT kCellRect;
        NiPoint3 kX;
        // The box's four corners and the three edges out of each, as vertex indices: 32 signed bytes, the last 16 zero.
        int8_t edges[32];
        // A cell is 4096 units wide.
        kCellRect.left = aiCellX << 12;
        kCellRect.right = (aiCellX << 12) + 4096;
        kCellRect.bottom = aiCellY << 12;
        kCellRect.top = (aiCellY << 12) + 4096;
        NiPoint3 ext = apRef->GetMultiBoundHalfExtent();
        NiPoint3 pos = apRef->GetLocationOnReference();
        NiMatrix3 rot = apRef->GetOrientation();
        rot.GetCol(0, kX);
        kX *= ext.x;
        rot.GetCol(1u, kY);
        kY *= ext.y;
        rot.GetCol(2u, kZ);
        kZ *= ext.z;
        boundverts[0] = pos + kX + kY + kZ;
        boundverts[1] = pos + kX + kY - kZ;
        boundverts[2] = pos + kX - kY + kZ;
        boundverts[3] = pos + kX - kY - kZ;
        boundverts[4] = pos - kX + kY + kZ;
        boundverts[5] = pos - kX + kY - kZ;
        boundverts[6] = pos - kX - kY + kZ;
        boundverts[7] = pos - kX - kY - kZ;
        edges[0] = 0;
        edges[1] = 1;
        edges[2] = 2;
        edges[3] = 4;
        edges[4] = 3;
        edges[5] = 1;
        edges[6] = 2;
        edges[7] = 7;
        edges[8] = 5;
        edges[9] = 1;
        edges[10] = 4;
        edges[11] = 7;
        edges[12] = 6;
        edges[13] = 2;
        edges[14] = 4;
        edges[15] = 7;
        for (int32_t k = 16; k < 32; ++k) {
            edges[k] = 0;
        }
        for (int32_t i = 0; i < 4; ++i) {
            NiPoint3 startpt = boundverts[edges[4 * i]];
            for (int32_t j = 1; j < 4; ++j) {
                NiPoint3 ignore;
                NiPoint3 endpt = boundverts[edges[4 * i + j]];
                if (LineVsRect(kCellRect, startpt, endpt, ignore)) {
                    return true;
                }
            }
        }
    }
    return false;
}

void BSMultiBoundOBB::SetAxis(const NiMatrix3& arAxis) {
    Axis = arAxis;
}

void NiAVObject::AttachProperty(NiProperty* pkProperty) {
    if (pkProperty->Type() < static_cast<int32_t>(NiProperty::MAX_TYPES)) {
        NiPointer<NiProperty> spProperty(pkProperty);
        m_kPropertyList.AddHead(spProperty);
    }
}

NiAlphaProperty::~NiAlphaProperty() {}

NiProperty::NiProperty() {}

void NiTAbstractPoolAllocator<uint32_t>::CreateFreeMem() {
    uint32_t uiPrev;
    NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem = static_cast<NiTAbstractPoolAllocator<uint32_t>::AllocNode*>(
        ourMalloc(
            NiTAbstractPoolAllocator<uint32_t>::ms_uiBlockSize <=
                    SIZE_MAX / sizeof(NiTAbstractPoolAllocator<uint32_t>::AllocNode)
                ? sizeof(NiTAbstractPoolAllocator<uint32_t>::AllocNode) *
                    NiTAbstractPoolAllocator<uint32_t>::ms_uiBlockSize
                : SIZE_MAX
        )
    );
    NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem[NiTAbstractPoolAllocator<uint32_t>::ms_uiBlockSize - 1]
        .m_pkNext = nullptr;
    uint32_t i = 1;
    do {
        NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem[i]
            .m_pkNext = &NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem[i + 1];
        uiPrev = i++;
    }
    while (uiPrev < NiTAbstractPoolAllocator<uint32_t>::ms_uiBlockSize - 2);

    if (NiTAbstractPoolAllocator<uint32_t>::ms_pkBlockHeader) {
        NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem
            ->m_pkNext = NiTAbstractPoolAllocator<uint32_t>::ms_pkBlockHeader;
        NiTAbstractPoolAllocator<uint32_t>::ms_pkBlockHeader = NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem;
    }
    else {
        NiTAbstractPoolAllocator<uint32_t>::ms_pkBlockHeader = NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem;
        NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem->m_pkNext = nullptr;
    }
    ++NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem;
}
